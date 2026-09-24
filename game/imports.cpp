// Copyright (c) 2026 Pixelforge Ports contributors
#include "so_util.h"
#include "thunk_gen.h"
#include <cmath>
#include <cstdarg>
#include <cwchar>
#include <new>
#include <unwind.h>
#include <SDL2/SDL.h>
#include <string>

// The donor shader contains a stray bracket after this preprocessor directive.
static void guest_glShaderSource(unsigned shader,int count,const char *const *strings,const int *lengths) {
    using Fn=void (*)(unsigned,int,const char *const *,const int *);
    static auto real=reinterpret_cast<Fn>(SDL_GL_GetProcAddress("glShaderSource"));
    if(!real)return;
    if(count<=0 || !strings){real(shader,count,strings,lengths);return;}
    std::string source;
    for(int i=0;i<count;++i) {
        if(!strings[i]){real(shader,count,strings,lengths);return;}
        source.append(strings[i],lengths && lengths[i]>=0 ? size_t(lengths[i]) : std::char_traits<char>::length(strings[i]));
    }
    size_t at=0;
    while((at=source.find("#endif]",at))!=std::string::npos) {
        const size_t end=at+7;
        if((at==0 || source[at-1]=='\n') &&
           (end==source.size() || source[end]=='\r' || source[end]=='\n'))source.erase(at+6,1);
        at+=6;
    }
    const char *text=source.data();
    int size=static_cast<int>(source.size());
    real(shader,1,&text,&size);
}
// ARM divmod returns quotient/remainder in r0/r1, not a C struct via sret.
extern "C" void __aeabi_uidivmod();
extern "C" void __aeabi_idivmod();
extern "C" _Unwind_Reason_Code __aeabi_unwind_cpp_pr0(_Unwind_State, _Unwind_Control_Block *, _Unwind_Context *);
extern "C" _Unwind_Reason_Code __aeabi_unwind_cpp_pr1(_Unwind_State, _Unwind_Control_Block *, _Unwind_Context *);
static float host_frexpf(float value,int *exponent){return std::frexp(value,exponent);}
static ABI_ATTR long long guest_d2lz(double value){return static_cast<long long>(value);}
static void *new_array_nothrow(size_t bytes,const void*){return ::operator new[](bytes,std::nothrow);}
static ABI_ATTR int guest_wprintf(const wchar_t *format,...) {
    va_list args;va_start(args,format);int result=vwprintf(format,args);va_end(args);return result;
}
DynLibFunction symtable_nova2[]={
    NO_THUNK("glShaderSource",(uintptr_t)&guest_glShaderSource),
    NO_THUNK("__aeabi_uidivmod",(uintptr_t)&__aeabi_uidivmod),
    NO_THUNK("__aeabi_idivmod",(uintptr_t)&__aeabi_idivmod),
    THUNK_SPECIFIC("frexpf",host_frexpf),
    NO_THUNK("__aeabi_d2lz",(uintptr_t)&guest_d2lz),
    NO_THUNK("wprintf",(uintptr_t)&guest_wprintf),
    NO_THUNK("_ZSt7nothrow",(uintptr_t)&std::nothrow),
    NO_THUNK("_ZnajRKSt9nothrow_t",(uintptr_t)&new_array_nothrow),
    NO_THUNK("__aeabi_unwind_cpp_pr0",(uintptr_t)&__aeabi_unwind_cpp_pr0),
    NO_THUNK("__aeabi_unwind_cpp_pr1",(uintptr_t)&__aeabi_unwind_cpp_pr1),
    {nullptr,0}
};
