// Copyright (c) 2026 Pixelforge Ports contributors
#include "nova2.h"
#include "native_bindings.h"
#include "display_config.h"
#include "khronos/gles2.h"
#include "fix_path.h"
#include "app_exit.h"
#include "crash.h"
#include "fb_probe.h"
#include "cursor_draw.h"
#include "so_util.h"
#include "atc_decompress.h"
#include <atomic>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <unistd.h>
#include <sys/mman.h>
#include <vector>

so_module *nova_module=nullptr;
JNIEnv *nova_env=nullptr;
SDL_Window *nova_window=nullptr;
int nova_width=640,nova_height=480;
std::string nova_donor_root;
std::string nova_asset_root;
static ReentrantHook pak_find_file_hook{};
static ReentrantHook pak_open_file_hook{};
static ReentrantHook pak_read_file_hook{};
static ReentrantHook atc_header_hook{};
static ReentrantHook atc_image_hook{};
static ReentrantHook atc_texture_data_hook{};
static ReentrantHook image_load_data_hook{};
static ReentrantHook texture_map_hook{};
static ReentrantHook pixel_convert_hook{};
static bool pak_trace_enabled=false;
static thread_local bool tracing_texture_data=false;
static std::atomic<unsigned> pak_trace_count{0};
static std::atomic<unsigned> pak_read_trace_count{0};
static std::atomic<unsigned> atc_header_trace_count{0};
static std::atomic<unsigned> atc_image_trace_count{0};
static std::atomic<unsigned> atc_texture_data_trace_count{0};
static std::atomic<unsigned> image_load_data_trace_count{0};
static std::atomic<unsigned> texture_map_trace_count{0};
static std::atomic<unsigned> pixel_convert_trace_count{0};

static void patch_no_touch_control_scheme(so_module *module) {
    // Route the game's own scheme selection through Xperia scheme 8. That
    // scheme keeps the game's touch buttons hidden while accepting touchpad
    // input from the handheld controls.
    uintptr_t address=module->text_base+0x0028a714;
    auto *instructions=reinterpret_cast<uint32_t *>(address);
    static const uint32_t expected[]={
        0xe59c0000,0xe5801008,0xe59c0000,0xe5900008
    };
    if(std::memcmp(instructions,expected,sizeof(expected))!=0) {
        fprintf(stderr,"N.O.V.A. 2: controller control scheme patch skipped (unexpected code)\n");
        return;
    }

    uintptr_t stub_address=so_alloc_arena(module,0x01ffffff,address,16);
    if(!stub_address) {
        fprintf(stderr,"N.O.V.A. 2: controller control scheme patch has no nearby code space\n");
        return;
    }
    int64_t offset=static_cast<int64_t>(stub_address)-static_cast<int64_t>(address+8);
    if((offset&3)!=0 || offset < -0x02000000LL || offset > 0x01fffffcLL) {
        fprintf(stderr,"N.O.V.A. 2: controller control scheme patch is out of branch range\n");
        return;
    }

    auto *stub=reinterpret_cast<uint32_t *>(stub_address);
    stub[0]=expected[0]; // ldr r0,[ip]
    stub[1]=0xe3a01008;   // mov r1,#8
    stub[2]=0xe51ff004;   // ldr pc,[pc,#-4]
    stub[3]=static_cast<uint32_t>(address+4);
    __builtin___clear_cache(reinterpret_cast<char *>(stub_address),
                            reinterpret_cast<char *>(stub_address+16));

    long page_size=sysconf(_SC_PAGESIZE);
    if(page_size<=0) return;
    uintptr_t page=address & ~static_cast<uintptr_t>(page_size-1);
    if(mprotect(reinterpret_cast<void *>(page),static_cast<size_t>(page_size),
                PROT_READ|PROT_WRITE|PROT_EXEC)!=0) {
        fprintf(stderr,"N.O.V.A. 2: controller control scheme patch failed: %s\n",strerror(errno));
        return;
    }
    instructions[0]=0xea000000|
        (static_cast<uint32_t>(offset>>2)&0x00ffffff);
    __builtin___clear_cache(reinterpret_cast<char *>(address),
                            reinterpret_cast<char *>(address+4));
    if(mprotect(reinterpret_cast<void *>(page),static_cast<size_t>(page_size),
                PROT_READ|PROT_EXEC)!=0)
        fprintf(stderr,"N.O.V.A. 2: warning: could not restore code page permissions: %s\n",strerror(errno));
    fprintf(stderr,"N.O.V.A. 2: Xperia controller scheme enforced; touch fire control hidden\n");
}

static void patch_jump_tutorial_null_control(so_module *module) {
    // Verified v1.0.3 ARM code: the selected touch control returns no jump
    // widget on a physical gamepad, but StartJumpGlow dereferences it. Mark
    // the tutorial as scheme 8 (the game's existing skip-touch-controls case)
    // and return through its epilogue so Update stops this tutorial safely.
    uintptr_t address=module->text_base+0x002ae2cc;
    auto *instructions=reinterpret_cast<uint32_t *>(address);
    if(instructions[0]!=0xe3530008 || instructions[1]!=0x0a000028) {
        fprintf(stderr,"N.O.V.A. 2: jump tutorial guard skipped (unexpected code)\n");
        return;
    }
    uintptr_t stub_address=so_alloc_arena(module,0x01ffffff,address,24);
    if(!stub_address) {
        fprintf(stderr,"N.O.V.A. 2: jump tutorial guard has no nearby code space\n");
        return;
    }
    int64_t branch_offset=static_cast<int64_t>(stub_address)-
                          static_cast<int64_t>(address+8);
    if((branch_offset&3)!=0 || branch_offset < -0x02000000LL ||
       branch_offset > 0x01fffffcLL) {
        fprintf(stderr,"N.O.V.A. 2: jump tutorial guard is out of branch range\n");
        return;
    }
    auto *stub=reinterpret_cast<uint32_t *>(stub_address);
    stub[0]=0xe3a03008; // mov r3,#8
    stub[1]=0xe5843038; // str r3,[r4,#56] (tutorial control-scheme field)
    stub[2]=0xe51ff004; // ldr pc,[pc,#-4]
    stub[3]=static_cast<uint32_t>(module->text_base+0x002ae378);
    __builtin___clear_cache(reinterpret_cast<char *>(stub_address),
                            reinterpret_cast<char *>(stub_address+16));
    long page_size=sysconf(_SC_PAGESIZE);
    if(page_size<=0) return;
    uintptr_t page=address & ~static_cast<uintptr_t>(page_size-1);
    if(mprotect(reinterpret_cast<void *>(page),static_cast<size_t>(page_size),
                PROT_READ|PROT_WRITE|PROT_EXEC)!=0) {
        fprintf(stderr,"N.O.V.A. 2: jump tutorial guard failed: %s\n",strerror(errno));
        return;
    }
    // cmp r0,#0; beq to the guarded tutorial state update above.
    instructions[0]=0xe3500000;
    instructions[1]=0x0a000000 |
        (static_cast<uint32_t>(branch_offset>>2)&0x00ffffff);
    __builtin___clear_cache(reinterpret_cast<char *>(address),
                            reinterpret_cast<char *>(address+8));
    if(mprotect(reinterpret_cast<void *>(page),static_cast<size_t>(page_size),
                PROT_READ|PROT_EXEC)!=0)
        fprintf(stderr,"N.O.V.A. 2: warning: could not restore code page permissions: %s\n",strerror(errno));
    fprintf(stderr,"N.O.V.A. 2: controller-safe jump tutorial enabled\n");
}

static void patch_tutorial_prompt_null_guard(so_module *module,
        uintptr_t site_offset,const uint32_t expected[2],
        uintptr_t skip_offset,const char *stage) {
    // The two replaced instructions are replayed in the trampoline when a view
    // exists. A missing view selects scheme 8 and skips only the touch prompt.
    uintptr_t address=module->text_base+site_offset;
    auto *instructions=reinterpret_cast<uint32_t *>(address);
    if(std::memcmp(instructions,expected,sizeof(uint32_t)*2)!=0) {
        fprintf(stderr,"N.O.V.A. 2: %s tutorial guard skipped (unexpected code)\n",stage);
        return;
    }

    uintptr_t stub_address=so_alloc_arena(module,0x01ffffff,address,40);
    if(!stub_address) {
        fprintf(stderr,"N.O.V.A. 2: %s tutorial guard has no nearby code space\n",stage);
        return;
    }
    auto branch_fits=[](uintptr_t from,uintptr_t to) {
        int64_t offset=static_cast<int64_t>(to)-static_cast<int64_t>(from+8);
        return (offset&3)==0 && offset>=-0x02000000LL && offset<=0x01fffffcLL;
    };
    const uintptr_t skip_prompt=module->text_base+skip_offset;
    const uintptr_t resume=address+8;
    if(!branch_fits(address,stub_address) ||
       !branch_fits(stub_address+4,stub_address+24)) {
        fprintf(stderr,"N.O.V.A. 2: %s tutorial guard is out of branch range\n",stage);
        return;
    }

    auto branch_word=[&](uintptr_t from,uintptr_t to,uint32_t opcode) {
        int64_t offset=static_cast<int64_t>(to)-static_cast<int64_t>(from+8);
        return opcode|(static_cast<uint32_t>(offset>>2)&0x00ffffff);
    };
    auto *stub=reinterpret_cast<uint32_t *>(stub_address);
    stub[0]=0xe3500000; // cmp r0,#0
    stub[1]=branch_word(stub_address+4,stub_address+24,0x1a000000); // bne replay
    stub[2]=0xe3a03008; // mov r3,#8
    stub[3]=0xe5843038; // str r3,[r4,#56] (no-touch control scheme)
    stub[4]=0xe51ff004; // ldr pc,[pc,#-4]
    stub[5]=static_cast<uint32_t>(skip_prompt);
    stub[6]=expected[0];
    stub[7]=expected[1];
    stub[8]=0xe51ff004; // ldr pc,[pc,#-4]
    stub[9]=static_cast<uint32_t>(resume);
    __builtin___clear_cache(reinterpret_cast<char *>(stub_address),
                            reinterpret_cast<char *>(stub_address+40));

    long page_size=sysconf(_SC_PAGESIZE);
    if(page_size<=0) return;
    uintptr_t page=address & ~static_cast<uintptr_t>(page_size-1);
    if(mprotect(reinterpret_cast<void *>(page),static_cast<size_t>(page_size),
                PROT_READ|PROT_WRITE|PROT_EXEC)!=0) {
        fprintf(stderr,"N.O.V.A. 2: %s tutorial guard failed: %s\n",stage,strerror(errno));
        return;
    }
    instructions[0]=branch_word(address,stub_address,0xea000000); // b stub
    __builtin___clear_cache(reinterpret_cast<char *>(address),
                            reinterpret_cast<char *>(address+4));
    if(mprotect(reinterpret_cast<void *>(page),static_cast<size_t>(page_size),
                PROT_READ|PROT_EXEC)!=0)
        fprintf(stderr,"N.O.V.A. 2: warning: could not restore code page permissions: %s\n",strerror(errno));
    fprintf(stderr,"N.O.V.A. 2: controller-safe %s tutorial prompt enabled\n",stage);
}

static void patch_tutorial_control_prompts(so_module *module) {
    static const uint32_t move_expected[]={0xe1d073fa,0xe7953003};
    static const uint32_t shoot_expected[]={0xe1d073fa,0xe7963003};
    static const uint32_t rotate_expected[]={0xe1d073fa,0xe7963003};
    // These sites follow each handler's existing scheme-8 branch, so normal
    // touch prompts retain their original path and only null views are skipped.
    // Start after their PC-relative literal loads; replay two plain operations.
    patch_tutorial_prompt_null_guard(module,0x002ae70c,move_expected,
                                     0x002ae764,"move");
    patch_tutorial_prompt_null_guard(module,0x002b0e2c,shoot_expected,
                                     0x002b0e74,"shoot");
    patch_tutorial_prompt_null_guard(module,0x002aff74,rotate_expected,
                                     0x002afec0,"rotate");
}

so_module *port_guest_module() {return nova_module;}
extern "C" void viewport_scale_init(int,int);

using PakFindFile = int (ABI_ATTR *)(void *,const char *);
static int ABI_ATTR trace_pak_find_file(void *reader,const char *name) {
    rehook_unhook(&pak_find_file_hook);
    int result=reinterpret_cast<PakFindFile>(pak_find_file_hook.addr)(reader,name);
    rehook_hook(&pak_find_file_hook);
    if(!pak_trace_enabled || !reader || !name) return result;
    if(!strstr(name,"menu") && !strstr(name,"Menu") && !strstr(name,"MENU") &&
       !strstr(name,"interface") && !strstr(name,"Interface") && !strstr(name,"INTERFACE") &&
       !strstr(name,"gameloft") && !strstr(name,"GameLoft") && !strstr(name,"GAMELOFT") &&
       !strstr(name,"2D_") && !strstr(name,"2d_") && !strstr(name,".atc") &&
       !strstr(name,".ATC") && !strstr(name,".tga") && !strstr(name,".TGA") &&
       !strstr(name,".bsprite")) return result;
    unsigned serial=pak_trace_count.fetch_add(1,std::memory_order_relaxed);
    if(serial>=800) return result;
    uintptr_t base=*reinterpret_cast<uintptr_t *>(static_cast<char *>(reader)+32);
    uintptr_t end=*reinterpret_cast<uintptr_t *>(static_cast<char *>(reader)+36);
    size_t count=end>=base?(end-base)/16:0;
    const char *match="";
    if(result>=0 && static_cast<size_t>(result)<count)
        match=*reinterpret_cast<const char **>(base+static_cast<size_t>(result)*16+8);
    fprintf(stderr,"TRACE: pak find '%s' -> %d/%lu flags=%u,%u match='%s'\n",
        name,result,(unsigned long)count,
        static_cast<unsigned char>(static_cast<char *>(reader)[44]),
        static_cast<unsigned char>(static_cast<char *>(reader)[45]),match);
    return result;
}

using PakOpenFile = void *(ABI_ATTR *)(void *,const char *);
static void *ABI_ATTR trace_pak_open_file(void *reader,const char *name) {
    rehook_unhook(&pak_open_file_hook);
    void *result=reinterpret_cast<PakOpenFile>(pak_open_file_hook.addr)(reader,name);
    rehook_hook(&pak_open_file_hook);
    if(pak_trace_enabled && reader && name &&
       (strstr(name,".atc") || strstr(name,".ATC") ||
        strstr(name,".bsprite") || strstr(name,".BSPRITE"))) {
        unsigned serial=pak_trace_count.fetch_add(1,std::memory_order_relaxed);
        if(serial<800)
            fprintf(stderr,"TRACE: pak open '%s' -> %s (%p)\n",
                name,result?"ok":"null",result);
    }
    return result;
}

using PakReadFile = unsigned (ABI_ATTR *)(void *,void *,unsigned);
static unsigned ABI_ATTR trace_pak_read_file(void *file,void *buffer,unsigned size) {
    rehook_unhook(&pak_read_file_hook);
    unsigned result=reinterpret_cast<PakReadFile>(pak_read_file_hook.addr)(file,buffer,size);
    rehook_hook(&pak_read_file_hook);
    if(!pak_trace_enabled || !file || !buffer) return result;
    const char *name=*reinterpret_cast<const char **>(static_cast<char *>(file)+32);
    if(!name || (!strstr(name,".atc") && !strstr(name,".ATC") &&
                 !strstr(name,".bsprite") && !strstr(name,".BSPRITE"))) return result;
    unsigned serial=pak_read_trace_count.fetch_add(1,std::memory_order_relaxed);
    if(serial>=160) return result;
    fprintf(stderr,"TRACE: pak read '%s' requested=%u returned=%u data=",name,size,result);
    const auto *bytes=static_cast<const unsigned char *>(buffer);
    for(unsigned i=0;i<result && i<16;i++) fprintf(stderr,"%02x",bytes[i]);
    fputc('\n',stderr);
    return result;
}

using AtcLoadTextureHeader = bool (ABI_ATTR *)(void *,void *,void *);
static bool ABI_ATTR trace_atc_load_texture_header(void *loader,void *file,void *desc) {
    rehook_unhook(&atc_header_hook);
    bool result=reinterpret_cast<AtcLoadTextureHeader>(atc_header_hook.addr)(loader,file,desc);
    rehook_hook(&atc_header_hook);
    unsigned serial=atc_header_trace_count.fetch_add(1,std::memory_order_relaxed);
    if(pak_trace_enabled && serial<128) {
        const char *name=file?*reinterpret_cast<const char **>(static_cast<char *>(file)+32):nullptr;
        if(name && desc)
            fprintf(stderr,"TRACE: ATC header '%s' -> %d format=%u width=%u height=%u\n",
                name,result,*reinterpret_cast<unsigned *>(static_cast<char *>(desc)+4),
                *reinterpret_cast<unsigned *>(static_cast<char *>(desc)+16),
                *reinterpret_cast<unsigned *>(static_cast<char *>(desc)+20));
        else fprintf(stderr,"TRACE: ATC header file=%p desc=%p -> %d\n",file,desc,result);
    }
    return result;
}

using AtcLoadImage = void *(ABI_ATTR *)(void *,void *,void *);
static void *ABI_ATTR trace_atc_load_image(void *result_storage,void *loader,void *file) {
    rehook_unhook(&atc_image_hook);
    void *result=reinterpret_cast<AtcLoadImage>(atc_image_hook.addr)(result_storage,loader,file);
    rehook_hook(&atc_image_hook);
    unsigned serial=atc_image_trace_count.fetch_add(1,std::memory_order_relaxed);
    if(pak_trace_enabled && serial<128) {
        const char *name=file?*reinterpret_cast<const char **>(static_cast<char *>(file)+32):nullptr;
        void *image=result_storage?*reinterpret_cast<void **>(result_storage):nullptr;
        fprintf(stderr,"TRACE: ATC image '%s' -> %s (%p)\n",
            name?name:"?",image?"ok":"null",image);
    }
    return result;
}

using AtcLoadTextureData = bool (ABI_ATTR *)(void *,void *,void *,void *);
static bool ABI_ATTR trace_atc_load_texture_data(void *loader,void *file,
                                                 void *texture_ref,void *desc) {
    rehook_unhook(&atc_texture_data_hook);
    bool result=reinterpret_cast<AtcLoadTextureData>(atc_texture_data_hook.addr)(
        loader,file,texture_ref,desc);
    rehook_hook(&atc_texture_data_hook);
    unsigned serial=atc_texture_data_trace_count.fetch_add(1,std::memory_order_relaxed);
    if(pak_trace_enabled && serial<128) {
        const char *name=file?*reinterpret_cast<const char **>(static_cast<char *>(file)+32):nullptr;
        void *texture=texture_ref?*reinterpret_cast<void **>(texture_ref):nullptr;
        fprintf(stderr,"TRACE: ATC texture data '%s' -> %d texture=%p desc=%p\n",
            name?name:"?",result,texture,desc);
    }
    return result;
}

using ImageLoadData = bool (ABI_ATTR *)(void *,void *,void *,void *);
static bool ABI_ATTR trace_image_load_data(void *file,void *data_info,
                                            void *desc,void *texture_ref) {
    rehook_unhook(&image_load_data_hook);
    tracing_texture_data=true;
    bool result=reinterpret_cast<ImageLoadData>(image_load_data_hook.addr)(
        file,data_info,desc,texture_ref);
    tracing_texture_data=false;
    rehook_hook(&image_load_data_hook);
    unsigned serial=image_load_data_trace_count.fetch_add(1,std::memory_order_relaxed);
    if(pak_trace_enabled && serial<128) {
        const char *name=file?*reinterpret_cast<const char **>(static_cast<char *>(file)+32):nullptr;
        void *texture=texture_ref?*reinterpret_cast<void **>(texture_ref):nullptr;
        unsigned fmt=desc?*reinterpret_cast<unsigned *>(static_cast<char *>(desc)+4):0;
        unsigned width=desc?*reinterpret_cast<unsigned *>(static_cast<char *>(desc)+16):0;
        unsigned height=desc?*reinterpret_cast<unsigned *>(static_cast<char *>(desc)+20):0;
        fprintf(stderr,"TRACE: image loadData '%s' -> %d format=%u %ux%u texture=%p info=%p\n",
            name?name:"?",result,fmt,width,height,texture,data_info);
    }
    return result;
}

using TextureMap = void *(ABI_ATTR *)(void *,unsigned,unsigned,unsigned char);
static void *ABI_ATTR trace_texture_map(void *texture,unsigned access,
                                        unsigned face,unsigned char mip) {
    rehook_unhook(&texture_map_hook);
    void *result=reinterpret_cast<TextureMap>(texture_map_hook.addr)(texture,access,face,mip);
    rehook_hook(&texture_map_hook);
    if(pak_trace_enabled && tracing_texture_data) {
        unsigned serial=texture_map_trace_count.fetch_add(1,std::memory_order_relaxed);
        if(serial<128)
            fprintf(stderr,"TRACE: texture map object=%p access=%u face=%u mip=%u -> %p\n",
                texture,access,face,static_cast<unsigned>(mip),result);
    }
    return result;
}

using PixelConvert = bool (ABI_ATTR *)(unsigned,const void *,unsigned,unsigned,
                                       void *,unsigned,unsigned,unsigned,bool);
static bool ABI_ATTR convert_atc_pixel_data(unsigned src_format,const void *src,
        unsigned src_pitch,unsigned dst_format,void *dst,unsigned dst_pitch,
        unsigned width,unsigned height,bool flip) {
    auto convert=reinterpret_cast<PixelConvert>(pixel_convert_hook.addr);
    bool result=false;
    unsigned decode_format=0;
    if(src_format==21 || src_format==22) {
        const std::size_t block_bytes=src_format==22?16u:8u;
        const std::size_t blocks_x=(static_cast<std::size_t>(width)+3u)/4u;
        const std::size_t blocks_y=(static_cast<std::size_t>(height)+3u)/4u;
        if(src && dst && width && height && width<=static_cast<unsigned>(INT_MAX) &&
           height<=static_cast<unsigned>(INT_MAX) &&
           blocks_x<=std::numeric_limits<std::size_t>::max()/block_bytes &&
           blocks_y<=std::numeric_limits<std::size_t>::max()/(blocks_x*block_bytes) &&
           static_cast<std::size_t>(src_pitch)>=blocks_x*block_bytes &&
           static_cast<std::size_t>(width)<=std::numeric_limits<std::size_t>::max()/4u &&
           static_cast<std::size_t>(height)<=std::numeric_limits<std::size_t>::max()/(static_cast<std::size_t>(width)*4u)) {
            const std::size_t compressed_row=blocks_x*block_bytes;
            const std::size_t compressed_size=compressed_row*blocks_y;
            const std::size_t rgba_pitch=static_cast<std::size_t>(width)*4u;
            const std::size_t rgba_size=rgba_pitch*height;
            std::vector<std::uint8_t> packed;
            std::vector<std::uint8_t> rgba;
            const void *input=src;
            try {
                if(static_cast<std::size_t>(src_pitch)!=compressed_row) {
                    packed.resize(compressed_size);
                    const auto *rows=static_cast<const std::uint8_t *>(src);
                    for(std::size_t row=0;row<blocks_y;row++)
                        std::memcpy(packed.data()+row*compressed_row,
                                    rows+row*src_pitch,compressed_row);
                    input=packed.data();
                }
                rgba.resize(rgba_size);
            } catch(const std::bad_alloc &) {
                rgba.clear();
            } catch(const std::length_error &) {
                rgba.clear();
            }
            bool decoded=false;
            if(!rgba.empty())
                decoded=src_format==22
                    ? atc::decode_rgba_explicit(input,compressed_size,
                        static_cast<int>(width),static_cast<int>(height),rgba.data())
                    : atc::decode_rgb(input,compressed_size,
                        static_cast<int>(width),static_cast<int>(height),rgba.data());
            if(decoded) {
                decode_format=14; // R8G8B8A8
                if(dst_format==decode_format &&
                   static_cast<std::size_t>(dst_pitch)>=rgba_pitch &&
                   static_cast<std::size_t>(dst_pitch)<=
                       std::numeric_limits<std::size_t>::max()/height) {
                    auto *output=static_cast<std::uint8_t *>(dst);
                    for(unsigned row=0;row<height;row++) {
                        const unsigned source_row=flip?height-1-row:row;
                        std::memcpy(output+static_cast<std::size_t>(row)*dst_pitch,
                            rgba.data()+static_cast<std::size_t>(source_row)*rgba_pitch,
                            rgba_pitch);
                    }
                    result=true;
                } else if(dst_format!=decode_format) {
                    rehook_unhook(&pixel_convert_hook);
                    result=convert(decode_format,rgba.data(),
                        static_cast<unsigned>(rgba_pitch),dst_format,dst,dst_pitch,
                        width,height,flip);
                    rehook_hook(&pixel_convert_hook);
                }
            }
        }
    } else {
        rehook_unhook(&pixel_convert_hook);
        result=convert(src_format,src,src_pitch,dst_format,dst,dst_pitch,
                       width,height,flip);
        rehook_hook(&pixel_convert_hook);
    }
    if(pak_trace_enabled && src_format>=21) {
        unsigned serial=pixel_convert_trace_count.fetch_add(1,std::memory_order_relaxed);
        if(serial>=128) return result;
        using GetFormatStrings = const char **(ABI_ATTR *)(void *);
        auto get_strings=reinterpret_cast<GetFormatStrings>(
            nova_module->text_base+0x005af0dc);
        const char **names=get_strings(nullptr);
        const char *src_name=names?names[src_format]:nullptr;
        const char *dst_name=names?names[dst_format]:nullptr;
        fprintf(stderr,"TRACE: pixel convert src=%u(%s) pitch=%u -> dst=%u(%s) "
            "%ux%u dstpitch=%u flip=%u decoded-as=%u result=%d srcptr=%p dstptr=%p\n",
            src_format,src_name?src_name:"?",src_pitch,dst_format,
            dst_name?dst_name:"?",width,height,dst_pitch,flip?1:0,decode_format,
            result,src,dst);
    }
    return result;
}

extern "C" int so_after_relocate(so_module *module) {
    nova_module=module;
    crash_report_init(module,"libnova2.so");
    int missing=0;
    for(int n=0;n<module->num_dynsym;n++) {
        auto &s=module->dynsym[n];
        if(s.st_shndx!=SHN_UNDEF || ELF32_ST_BIND(s.st_info)==STB_WEAK) continue;
        const char *name=module->dynstr+s.st_name;
        if(*name && !so_resolve_link(module,name)) {
            fprintf(stderr,"Unresolved game import: %s\n",name); missing++;
        }
    }
    if(missing) return -1;
    patch_no_touch_control_scheme(module);
    patch_jump_tutorial_null_control(module);
    patch_tutorial_control_prompts(module);
    uintptr_t convert_address=module->text_base+0x005c5db8;
    uint32_t convert_first=*reinterpret_cast<uint32_t *>(convert_address);
    if(convert_first==0xe92d4ff0) {
        rehook_new(module,&pixel_convert_hook,convert_address,
            reinterpret_cast<uintptr_t>(&convert_atc_pixel_data));
        rehook_hook(&pixel_convert_hook);
        fprintf(stderr,"N.O.V.A. 2: ATC CPU decode enabled\n");
    } else {
        fprintf(stderr,"N.O.V.A. 2: ATC CPU decode unavailable (unexpected converter prologue %08x)\n",
            convert_first);
    }
    pak_trace_enabled=getenv("NOVA2_TRACE_PAK")!=nullptr;
    if(pak_trace_enabled) {
        uintptr_t address=module->text_base+0x002c0240;
        uintptr_t open_address=module->text_base+0x002c0e24;
        uintptr_t read_address=module->text_base+0x0053415c;
        uintptr_t atc_header_address=module->text_base+0x005d13b4;
        uintptr_t atc_image_address=module->text_base+0x005d1568;
        uintptr_t atc_texture_data_address=module->text_base+0x005d1958;
        uintptr_t image_load_data_address=module->text_base+0x005d259c;
        uintptr_t texture_map_address=module->text_base+0x005c8ee4;
        uint32_t first=*reinterpret_cast<uint32_t *>(address);
        uint32_t open_first=*reinterpret_cast<uint32_t *>(open_address);
        uint32_t read_first=*reinterpret_cast<uint32_t *>(read_address);
        uint32_t header_first=*reinterpret_cast<uint32_t *>(atc_header_address);
        uint32_t image_first=*reinterpret_cast<uint32_t *>(atc_image_address);
        uint32_t texture_data_first=*reinterpret_cast<uint32_t *>(atc_texture_data_address);
        uint32_t load_data_first=*reinterpret_cast<uint32_t *>(image_load_data_address);
        uint32_t map_first=*reinterpret_cast<uint32_t *>(texture_map_address);
        if(first==0xe92d4ff8 && open_first==0xe92d4010 && read_first==0xe92d4070 &&
           header_first==0xe92d4ff0 && image_first==0xe92d4ff0 &&
           texture_data_first==0xe92d45f0 && load_data_first==0xe92d4ff0 &&
           map_first==0xe92d41f0) {
            rehook_new(module,&pak_find_file_hook,address,
                reinterpret_cast<uintptr_t>(&trace_pak_find_file));
            rehook_hook(&pak_find_file_hook);
            rehook_new(module,&pak_open_file_hook,open_address,
                reinterpret_cast<uintptr_t>(&trace_pak_open_file));
            rehook_hook(&pak_open_file_hook);
            rehook_new(module,&pak_read_file_hook,read_address,
                reinterpret_cast<uintptr_t>(&trace_pak_read_file));
            rehook_hook(&pak_read_file_hook);
            rehook_new(module,&atc_header_hook,atc_header_address,
                reinterpret_cast<uintptr_t>(&trace_atc_load_texture_header));
            rehook_hook(&atc_header_hook);
            rehook_new(module,&atc_image_hook,atc_image_address,
                reinterpret_cast<uintptr_t>(&trace_atc_load_image));
            rehook_hook(&atc_image_hook);
            rehook_new(module,&atc_texture_data_hook,atc_texture_data_address,
                reinterpret_cast<uintptr_t>(&trace_atc_load_texture_data));
            rehook_hook(&atc_texture_data_hook);
            rehook_new(module,&image_load_data_hook,image_load_data_address,
                reinterpret_cast<uintptr_t>(&trace_image_load_data));
            rehook_hook(&image_load_data_hook);
            rehook_new(module,&texture_map_hook,texture_map_address,
                reinterpret_cast<uintptr_t>(&trace_texture_map));
            rehook_hook(&texture_map_hook);
            fprintf(stderr,"TRACE: archive lookup/open/read/ATC tracing enabled\n");
        } else {
            pak_trace_enabled=false;
            fprintf(stderr,"TRACE: archive lookup/open hook skipped "
                "(unexpected prologues %08x/%08x/%08x/%08x/%08x/%08x/%08x/%08x)\n",
                first,open_first,read_first,header_first,image_first,texture_data_first,
                load_data_first,map_first);
        }
    }
    return 0;
}

int main(int argc,char **argv) {
    if(argc>1 && !strcmp(argv[1],"--version")) {puts("nova2-development-0.1");return 0;}
    if(argc>1 && !strcmp(argv[1],"--sdl-info")) {
        int count=SDL_GetNumVideoDrivers();
        for(int i=0;i<count;i++)
            fprintf(stderr,"sdl: video driver: %s\n",SDL_GetVideoDriver(i));
        return count>0?0:1;
    }
    const char *root=argc>1?argv[1]:"donor";
    char absolute[4096];
    if(!realpath(root,absolute)) {perror("Game data directory");return 1;}
    nova_donor_root=absolute;
    nova_asset_root=nova_donor_root+"/gameloft/games/GloftN2HP";
    if(access(nova_asset_root.c_str(),R_OK))
        nova_asset_root=nova_donor_root+"/Gameloft/games/GloftN2HP";
    if(access(nova_asset_root.c_str(),R_OK)) nova_asset_root=nova_donor_root;
    if(access((nova_asset_root+"/sprites.gla").c_str(),R_OK) ||
       access((nova_asset_root+"/strings.gla").c_str(),R_OK)) {
        fprintf(stderr,"Incomplete N.O.V.A. 2 data: sprites.gla and strings.gla required in %s\n",nova_asset_root.c_str());
        return 1;
    }
    fprintf(stderr,"N.O.V.A. 2: assets=%s\n",nova_asset_root.c_str());
    io_set_game_dir(nova_asset_root.c_str());
    if(chdir(nova_asset_root.c_str())) return 1;
    setenv("NOVA2_GL_SINGLE_DISPATCH","1",0);
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMECONTROLLER)) {
        fprintf(stderr,"SDL_Init: %s\n",SDL_GetError());return 1;
    }
    if(!display_config::detect("NOVA2",nova_width,nova_height,false)) return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    nova_window=SDL_CreateWindow("N.O.V.A. 2",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
        nova_width,nova_height,SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN);
    SDL_GLContext gl=nova_window?SDL_GL_CreateContext(nova_window):nullptr;
    // Optional desktop compatibility GL for the offscreen smoke test.
    if(!gl && getenv("NOVA2_DESKTOP_GL")) {
        if(nova_window) SDL_DestroyWindow(nova_window);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
        nova_window=SDL_CreateWindow("N.O.V.A. 2",0,0,nova_width,nova_height,SDL_WINDOW_OPENGL);
        gl=nova_window?SDL_GL_CreateContext(nova_window):nullptr;
    }
    if(!gl) {fprintf(stderr,"GLES2 context: %s\n",SDL_GetError());return 1;}
    SDL_GL_SetSwapInterval(0);
    int w,h;SDL_GL_GetDrawableSize(nova_window,&w,&h);
    if(!display_config::drawable("NOVA2",w,h,nova_width,nova_height,false)) return 1;
    load_gles1_funcs();load_gles2_funcs();viewport_scale_init(w,h);
    register_services();
    JavaVM *vm=nullptr;
    if(JNI_CreateJavaVM(&vm,&nova_env,nullptr)!=JNI_OK) return 1;
    std::string libdir=nova_donor_root+"/lib/armeabi";
    so_set_options(nullptr,libdir.c_str());
    nova_module=so_load_module("libnova2.so",nullptr,nullptr);
    if(!nova_module) {fprintf(stderr,"Cannot load libnova2.so\n");return 1;}
    using OnLoad=jint (ABI_ATTR *)(JavaVM*,void*);
    auto onload=reinterpret_cast<OnLoad>(so_symbol(nova_module,"JNI_OnLoad"));
    if(onload && onload(vm,nullptr)<0) return 1;
    fprintf(stderr,"N.O.V.A. 2: native library loaded\n");
    native<donor::Device_nativeInit_26>("GLUtils_Device_nativeInit")(nova_env,(jclass)&device_class);
    native<donor::GLResLoader_nativeInit_24>("GLResLoader_nativeInit")(nova_env,(jclass)&resources_class);
    native<donor::GLMediaPlayer_nativeInit_22>("GLMediaPlayer_nativeInit")(nova_env,(jclass)&media_class);
    native<donor::GLGame_nativeInit_8>("GLGame_nativeInit")(nova_env,(jclass)&game_class);
    native<donor::GLGame_nativeKeyboardEnabled_9>("GLGame_nativeKeyboardEnabled")(nova_env,(jclass)&game_class,JNI_TRUE,JNI_TRUE);
    native<donor::GameRenderer_nativeInit_2>("GameRenderer_nativeInit")(nova_env,(jclass)&renderer_class,nova_width,nova_height,0);
    native<donor::GameRenderer_nativeResize_4>("GameRenderer_nativeResize")(nova_env,(jclass)&renderer_class,nova_width,nova_height);
    input_init();
    auto render=native<donor::GameRenderer_nativeRender_3>("GameRenderer_nativeRender");
    const int frame_limit=getenv("NOVA2_TEST_FRAMES")?atoi(getenv("NOVA2_TEST_FRAMES")):0;
    int frames=0;
    while(!android_app_exit_requested()) {
        Uint32 start=SDL_GetTicks();
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type==SDL_QUIT) android_app_request_exit("SDL quit");
            else input_event(event);
        }
        if(android_app_exit_requested()) break;
        input_tick(0.033f);
        render(nova_env,(jclass)&renderer_class);
        android_cursor_draw(w,h);
        if(frame_limit>0) android_fb_probe(frames+1,w,h);
        SDL_GL_SwapWindow(nova_window);
        if(++frames==1) fprintf(stderr,"N.O.V.A. 2: first nativeRender returned\n");
        if(frame_limit>0 && frames>=frame_limit) break;
        // Match GameRenderer.onDrawFrame's original 33 ms interval.
        Uint32 elapsed=SDL_GetTicks()-start;
        if(elapsed<33) SDL_Delay(33-elapsed);
    }
    fprintf(stderr,"N.O.V.A. 2: rendered %d frames\n",frames);
    SDL_Quit();
    return 0;
}
