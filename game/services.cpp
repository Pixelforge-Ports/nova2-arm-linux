// Copyright (c) 2026 Pixelforge Ports contributors
#include "nova2.h"
#include "native_bindings.h"
#include "fix_path.h"
#include "app_exit.h"
#include <zip.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>

Class game_class{},renderer_class{},resources_class{},media_class{},device_class{},music_class{};
void register_audio(Class &);
static zip_t *apk=nullptr;
static std::string string(JNIEnv *e,jstring s) {
    if(!s) return {};
    const char *p=e->GetStringUTFChars(s,nullptr);std::string v=p?p:"";
    if(p)e->ReleaseStringUTFChars(s,p);
    return v;
}
std::string data_path(const char *path) {
    std::string p=path?path:"";
    const std::string prefix="/sdcard/";
    if(p.rfind(prefix,0)==0) p.erase(0,prefix.size());
    while(p.rfind("./",0)==0)p.erase(0,2);
    if(p.find("..")!=std::string::npos) return {};
    if(!p.empty() && p[0]=='/') return p;
    if(p.rfind("gameloft/games/GloftN2HP/",0)==0 ||
       p.rfind("Gameloft/games/GloftN2HP/",0)==0)
        return nova_asset_root+"/"+p.substr(strlen("gameloft/games/GloftN2HP/"));
    std::string root=io_game_dir();
    std::string candidate=root+"/"+p;
    if(access(candidate.c_str(),R_OK)==0) return candidate;
    candidate=nova_donor_root+"/apk-assets/"+p;
    return candidate;
}
extern "C" const char *port_fix_path(const char *orig,char *buf,size_t size) {
    if(!orig)return nullptr;
    const char *data=orig;
    if(!strncmp(data,"/sdcard/",8))data+=8;
    const char *prefix="gameloft/games/GloftN2HP";
    const size_t len=strlen(prefix);
    if((!strncmp(data,prefix,len) || !strncmp(data,"Gameloft/games/GloftN2HP",len)) &&
       (data[len]=='/' || data[len]=='\0')) {
        snprintf(buf,size,"%s%s",nova_asset_root.c_str(),data+len);return buf;
    }
    if(!strncmp(orig,"/sdcard/",8)) {
        snprintf(buf,size,"%s/%s",nova_donor_root.c_str(),orig+8);return buf;
    }
    const char *save="/data/data/com.gameloft.android.TBFV.GloftN2HP.ML/";
    if(!strncmp(orig,save,strlen(save))) {
        snprintf(buf,size,"%s/%s",io_writable_dir(),orig+strlen(save));return buf;
    }
    return nullptr;
}
extern "C" const char *port_system_font() {return nullptr;}
static std::vector<jbyte> read_resource(const std::string &name,int offset,int count) {
    if(offset<0 || count < -1)return {};
    auto path=data_path(name.c_str());
    if(FILE *file=fopen(path.c_str(),"rb")) {
        fseek(file,0,SEEK_END);long size=ftell(file);
        long bytes=count<0?size-offset:std::min<long>(count,size-offset);
        if(bytes<0 || bytes>128*1024*1024){fclose(file);return {};}
        fseek(file,offset,SEEK_SET);std::vector<jbyte> result(bytes);
        size_t got=fread(result.data(),1,result.size(),file);fclose(file);
        result.resize(got);return result;
    }
    if(!apk) {int err;apk=zip_open((nova_donor_root+"/original.apk").c_str(),ZIP_RDONLY,&err);}
    if(!apk)return {};
    std::string clean=name;
    while(clean.rfind("./",0)==0)clean.erase(0,2);
    std::string candidates[]={clean,"assets/"+clean,"res/raw/"+clean,"res/drawable/res_"+clean};
    for(auto &entry:candidates) {
        zip_stat_t st{};
        if(zip_stat(apk,entry.c_str(),0,&st))continue;
        if(st.size>128*1024*1024 || (zip_uint64_t)offset>st.size) return {};
        zip_file_t *file=zip_fopen(apk,entry.c_str(),0);if(!file)continue;
        std::vector<jbyte> all(st.size);zip_int64_t got=zip_fread(file,all.data(),all.size());zip_fclose(file);
        if(got<offset)return {};
        size_t end=count<0?got:std::min<size_t>(got,(size_t)offset+count);
        return std::vector<jbyte>(all.begin()+offset,all.begin()+end);
    }
    fprintf(stderr,"Resource not found: %s\n",name.c_str());return {};
}
static jbyteArray bytes(JNIEnv *e,const std::vector<jbyte>&data) {
    auto array=e->NewByteArray(data.size());
    if(array&&!data.empty())e->SetByteArrayRegion(array,0,data.size(),data.data());
    return array;
}
static jbyteArray text(JNIEnv *e,const char *s) {return bytes(e,std::vector<jbyte>(s,s+strlen(s)));}
static jbyteArray full(JNIEnv *e,jclass,jstring s) {return bytes(e,read_resource(string(e,s),0,-1));}
static jbyteArray part(JNIEnv *e,jclass,jstring s,jint offset,jint len) {return bytes(e,read_resource(string(e,s),offset,len));}
static jint length(JNIEnv *e,jclass,jstring s) {
    auto name=string(e,s);auto path=data_path(name.c_str());struct stat info{};
    if(!stat(path.c_str(),&info) && info.st_size<=INT32_MAX)return info.st_size;
    return read_resource(name,0,-1).size();
}
static jbyteArray resource_id(JNIEnv *e,jclass,jint id) {
    // Populated from this APK's resource table by tools/read_resources.py.
    extern const char *resource_name(unsigned);
    auto path=resource_name(id);
    if(!path){fprintf(stderr,"Unknown resource ID: %08x\n",id);return nullptr;}
    return bytes(e,read_resource(path,0,-1));
}
static void quit(JNIEnv*,jclass) {android_app_request_exit("GLGame.Exit");}
static jbyteArray sound_raw(JNIEnv *e,jclass c,jint id) {return resource_id(e,c,0x7f040007+id);}
static jint sound_length(JNIEnv *e,jclass c,jint id) {
    auto data=sound_raw(e,c,id);if(!data)return 0;
    jint size=e->GetArrayLength(data);e->DeleteLocalRef(data);return size;
}
static jint unique_code=0;
static void set_unique_code(JNIEnv*,jclass,jint value) {unique_code=value;}
static jint get_unique_code(JNIEnv*,jclass) {return unique_code;}
static jbyteArray app_version(JNIEnv *e,jclass) {return text(e,"1.0.3");}
static jint zero(JNIEnv*,jclass) {return 0;}
static jboolean false_value(JNIEnv*,jclass) {return JNI_FALSE;}
static jlong memory(JNIEnv*,jclass) {return 128LL*1024*1024;}
static void idle(JNIEnv*,jclass) {}
static void external(JNIEnv*,jclass,jstring) {fprintf(stderr,"External browser unavailable\n");}
static void network(JNIEnv*,jclass,jint) {fprintf(stderr,"Online service unavailable\n");}
static void trophy(JNIEnv*,jclass,jint) {}
static jbyteArray host(JNIEnv *e,jclass) {return text(e,"ARM_Linux");}
static jbyteArray version(JNIEnv *e,jclass) {return bytes(e,read_resource("res/raw/infoversion.txt",0,-1));}
static jbyteArray empty(JNIEnv *e,jclass) {return text(e,"");}
static jbyteArray device_id(JNIEnv *e,jclass) {return text(e,"000000000000000");}
static jbyteArray installation_info(JNIEnv *e,jclass) {return bytes(e,read_resource("res/raw/igli.bin",0,-1));}
static jbyteArray device_info(JNIEnv *e,jclass) {return text(e,"R800i");}
static void video(JNIEnv *e,jclass) {
    fprintf(stderr,"Intro video skipped; notifying game completion\n");
    native<donor::GLGame_nativeSetOnVideoCompletion_12>("GLGame_nativeSetOnVideoCompletion")(e,(jclass)&game_class);
}
static void swap(JNIEnv*,jclass) {SDL_GL_SwapWindow(nova_window);}
static jint keyboard_visible(JNIEnv*,jclass) {return 0;}
static jbyteArray keyboard_text(JNIEnv *e,jclass) {return text(e,"");}
static void keyboard(JNIEnv *e,jclass,jint open,jstring,jint) {
    if(open) native<donor::GameRenderer_onKeyboardFinish_5>("GameRenderer_onKeyboardFinish")(e,(jclass)&renderer_class);
}
static jint no_songs(JNIEnv*,jclass,jint) {return 0;}
static jbyteArray empty_index(JNIEnv *e,jclass,jint){return text(e,"");}
static jbyteArray empty_pair(JNIEnv *e,jclass,jint,jint){return text(e,"");}
static void playlist(JNIEnv*,jclass,jint) {}

static void setup(Class &c,const char *name,const ManagedMethod *methods) {
    c.classpath=name;c.classname=strrchr(name,'/')+1;c.managed_methods=methods;c.instance_size=0;
    ClassRegistry::register_class(c);
}
#define M(c,fn,name,sig) ManagedMethod::RegisterStatic<fn>(c,name,sig)
void register_services() {
    static const ManagedMethod game[]={
        M(game_class,quit,"Exit","()V"),M(game_class,memory,"GetMemoryInfo","()J"),
        M(game_class,zero,"IsWifiEnabled","()I"),M(game_class,zero,"getWifiIP","()I"),
        M(game_class,external,"OpenBrowser","(Ljava/lang/String;)V"),
        M(game_class,network,"OpenGLive","(I)V"),M(game_class,network,"OpenIGP","(I)V"),
        M(game_class,trophy,"NotifyTrophy","(I)V"),M(game_class,host,"getHostName","()[B"),
        M(game_class,version,"getVersion","()[B"),M(game_class,device_id,"da","()[B"),
        M(game_class,installation_info,"db","()[B"),M(game_class,empty,"dc","()[B"),
        M(game_class,false_value,"isDemo","()Z"),M(game_class,video,"PlayGLVideo","()V"),
        M(game_class,idle,"d","()V"),M(game_class,idle,"sendAppToBackground","()V"),M(game_class,idle,"launchGetGames","()V"),{} };
    static const ManagedMethod renderer[]={
        M(renderer_class,swap,"swapEGLBuffers","()V"),M(renderer_class,keyboard_text,"getKeyboardText","()[B"),
        M(renderer_class,keyboard,"setKeyboard","(ILjava/lang/String;I)V"),
        M(renderer_class,keyboard_visible,"isKeyboardVisible","()I"),{} };
    static const ManagedMethod resources[]={
        M(resources_class,full,"getResourceFull","(Ljava/lang/String;)[B"),
        M(resources_class,resource_id,"getResourceFull","(I)[B"),
        M(resources_class,part,"getResourceBytes","(Ljava/lang/String;II)[B"),
        M(resources_class,sound_raw,"getSoundRaw","(I)[B"),
        M(resources_class,sound_length,"getResourceLengthSoundRaw","(I)I"),
        M(resources_class,length,"getResourceLength","(Ljava/lang/String;)I"),{} };
    static const ManagedMethod device[]={
        M(device_class,device_id,"a","()[B"),M(device_class,empty,"b","()[B"),
        M(device_class,device_info,"c","()[B"),M(device_class,app_version,"d","()[B"),
        M(device_class,empty,"f","()[B"),M(device_class,host,"getHostName","()[B"),
        M(device_class,false_value,"IsWifiEnable","()Z"),M(device_class,get_unique_code,"getUniqueCode","()I"),M(device_class,set_unique_code,"e","(I)V"),{} };
    static const ManagedMethod music[]={
        M(music_class,zero,"GetNumPlaylists","()I"),M(music_class,no_songs,"GetNumSongs","(I)I"),
        M(music_class,empty_index,"GetPlayListName","(I)[B"),M(music_class,empty_pair,"GetSongName","(II)[B"),
        M(music_class,idle,"PauseMusicBG","()V"),M(music_class,idle,"PlayBGMusic","()V"),
        M(music_class,idle,"ResumeMusicBG","()V"),M(music_class,idle,"StopMusicBG","()V"),
        M(music_class,playlist,"ChangeMusic","(I)V"),M(music_class,playlist,"SetPlaylist","(I)V"),{} };
    setup(game_class,"com/gameloft/android/TBFV/GloftN2HP/ML/GLGame",game);
    setup(renderer_class,"com/gameloft/android/TBFV/GloftN2HP/ML/GameRenderer",renderer);
    setup(resources_class,"com/gameloft/android/TBFV/GloftN2HP/ML/GLResLoader",resources);
    setup(device_class,"com/gameloft/android/TBFV/GloftN2HP/ML/GLUtils/Device",device);
    setup(music_class,"com/gameloft/android/TBFV/GloftN2HP/ML/Musicplayer",music);
    register_audio(media_class);
}
