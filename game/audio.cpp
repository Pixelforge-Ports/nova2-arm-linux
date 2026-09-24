// Copyright (c) 2026 Pixelforge Ports contributors
#include "nova2.h"
#include <algorithm>
#include <map>
#include <vector>
#include <cstring>
#include <memory>
#include <unistd.h>
struct Sound {std::string path;float volume=1;};
struct Voice {std::shared_ptr<std::vector<Sint16>> pcm;size_t position=0;int sound=-1;bool loop=false,paused=false;float volume=1;};
static std::map<int,Sound> sounds;
static Voice voices[24];
static SDL_AudioDeviceID output=0;
static float master=1;
static void mix(void*,Uint8 *buffer,int bytes) {
    memset(buffer,0,bytes);auto out=reinterpret_cast<Sint16*>(buffer);
    for(int i=0;i<bytes/2;i++) {
        int sum=0;
        for(auto &v:voices) {
            if(!v.pcm || v.paused || v.pcm->empty())continue;
            if(v.position>=v.pcm->size()){if(v.loop)v.position=0;else continue;}
            sum+=int((*v.pcm)[v.position++]*v.volume*master);
        }
        out[i]=std::clamp(sum,-32768,32767);
    }
}
static bool open_audio() {
    if(output)return true;
    SDL_AudioSpec desired{};desired.freq=44100;desired.format=AUDIO_S16SYS;desired.channels=2;desired.samples=1024;desired.callback=mix;
    output=SDL_OpenAudioDevice(nullptr,0,&desired,nullptr,0);
    if(!output){fprintf(stderr,"NOVA2 audio: %s\n",SDL_GetError());return false;}
    SDL_PauseAudioDevice(output,0);return true;
}
static void register_sound(JNIEnv *e,jclass,jint id,jstring name,jint) {
    const char *text=e->GetStringUTFChars(name,nullptr);if(!text)return;
    std::string base=text;e->ReleaseStringUTFChars(name,text);
    auto slash=base.find_last_of('/');if(slash!=std::string::npos)base.erase(0,slash+1);
    auto suffix=base.find_last_of('.');if(suffix!=std::string::npos)base.resize(suffix);
    sounds[id].path=data_path(("gameloft/games/GloftN2HP/sounds/"+base+".wav").c_str());
}
static jint play(JNIEnv*,jclass,jint id,jfloat volume,jint loops) {
    auto it=sounds.find(id);if(it==sounds.end() || !open_audio())return -1;
    SDL_AudioSpec source{};Uint8 *raw=nullptr;Uint32 bytes=0;
    if(!SDL_LoadWAV(it->second.path.c_str(),&source,&raw,&bytes)) {
        fprintf(stderr,"Cannot decode sound %d: %s (%s)\n",id,it->second.path.c_str(),SDL_GetError());return -1;
    }
    SDL_AudioCVT cvt{};
    if(SDL_BuildAudioCVT(&cvt,source.format,source.channels,source.freq,AUDIO_S16SYS,2,44100)<0 || bytes>32*1024*1024) {SDL_FreeWAV(raw);return -1;}
    std::vector<Uint8> converted(size_t(bytes)*std::max(1,cvt.len_mult));
    memcpy(converted.data(),raw,bytes);SDL_FreeWAV(raw);
    cvt.buf=converted.data();cvt.len=bytes;
    if(SDL_ConvertAudio(&cvt)<0)return -1;
    size_t length=(cvt.needed?cvt.len_cvt:bytes)/2;
    auto samples=std::make_shared<std::vector<Sint16>>(length);memcpy(samples->data(),converted.data(),length*2);
    SDL_LockAudioDevice(output);
    int slot=-1;
    for(int n=0;n<24;n++)if(!voices[n].pcm || voices[n].position>=voices[n].pcm->size()){slot=n;break;}
    if(slot>=0)voices[slot]={samples,0,id,loops<0,false,std::clamp(volume,0.f,1.f)};
    SDL_UnlockAudioDevice(output);return slot;
}
static void each(int id,int command,float volume=1) {
    if(!output)return;SDL_LockAudioDevice(output);
    for(auto &v:voices)if(id<0 || v.sound==id) {
        if(command==0){v.position=v.pcm?v.pcm->size():0;v.loop=false;}
        if(command==1)v.paused=true;if(command==2)v.paused=false;if(command==3)v.volume=std::clamp(volume,0.f,1.f);
    }SDL_UnlockAudioDevice(output);
}
static void stop(JNIEnv*,jclass,jint id){each(id,0);}
static void pause_music(JNIEnv*,jclass,jint id){each(id,1);}
static void resume(JNIEnv*,jclass,jint id,jfloat vol){each(id,2);each(id,3,vol);}
static void stop_all(JNIEnv*,jclass){each(-1,0);}
static void pause_all(JNIEnv*,jclass){each(-1,1);}
static void resume_all(JNIEnv*,jclass,jfloat vol){each(-1,2);each(-1,3,vol);}
static void volume(JNIEnv*,jclass,jfloat vol,jint id){each(id,3,vol);}
static void volume_one(JNIEnv*,jclass,jint id,jfloat vol){each(id,3,vol);}
static void set_master(JNIEnv*,jclass,jfloat vol){if(output)SDL_LockAudioDevice(output);master=std::clamp(vol,0.f,1.f);if(output)SDL_UnlockAudioDevice(output);}
static jfloat get_master(JNIEnv*,jclass){return master;}
static jfloat get_volume(JNIEnv*,jclass,jint id){return sounds[id].volume;}
static jboolean playing(JNIEnv*,jclass,jint id){bool p=false;if(output)SDL_LockAudioDevice(output);for(auto &v:voices)if(v.sound==id&&v.pcm&&v.position<v.pcm->size()&&!v.paused)p=true;if(output)SDL_UnlockAudioDevice(output);return p;}
static jboolean emitter_playing(JNIEnv*e,jclass c,jint id,jint){return playing(e,c,id);}
static jint loaded(JNIEnv*,jclass,jint id){return sounds.count(id)&&access(sounds[id].path.c_str(),R_OK)==0;}
static jint emitter(JNIEnv*,jclass,jint id){return id;}
static void stop_emitter(JNIEnv*,jclass,jint id,jint){each(id,0);}
static void pause_emitter(JNIEnv*,jclass,jint id,jint){each(id,1);}
static void resume_emitter(JNIEnv*,jclass,jint id,jint){each(id,2);}
static void volume_emitter(JNIEnv*,jclass,jint id,jint,jfloat vol){each(id,3,vol);}
static void pitch(JNIEnv*,jclass,jint,jint,jfloat) {}
static void preload(JNIEnv*,jclass,jint) {}
static void group(JNIEnv*,jclass,jint,jboolean) {}
static void idle(JNIEnv*,jclass) {}
static jboolean finished(JNIEnv*,jclass){return JNI_TRUE;}
static void movie(JNIEnv*,jclass,jstring) {}
static void fx(JNIEnv*,jclass,jint,jint,jint) {}
#define M(fn,name,sig) ManagedMethod::RegisterStatic<fn>(c,name,sig)
void register_audio(Class &c) {
    static const ManagedMethod methods[]={
        M(register_sound,"registerSoundFile","(ILjava/lang/String;I)V"),M(play,"playMusic","(IFI)I"),
        M(stop,"stopMusic","(I)V"),M(stop,"unloadMusic","(I)V"),M(stop,"resetMusic","(I)V"),
        M(pause_music,"pauseMusic","(I)V"),M(resume,"resumeMusic","(IF)V"),
        M(stop_all,"stopAllMusic","()V"),M(stop_all,"stopAllSounds","()V"),M(stop_all,"stopVoice","()V"),
        M(pause_all,"pauseAllMusic","()V"),M(resume_all,"resumeAllMusic","(F)V"),
        M(volume,"setVolumeMusic","(FI)V"),M(volume_one,"setVolumeOneMusic","(IF)V"),
        M(set_master,"setMasterVolume","(F)V"),M(get_master,"getMasterVolume","()F"),M(get_volume,"getVolumeMusic","(I)F"),
        M(playing,"isMediaPlaying","(I)Z"),M(loaded,"isMusicLoaded","(I)I"),M(emitter,"getEmitter","(I)I"),
        M(emitter_playing,"isEmitterPlaying","(II)Z"),M(stop_emitter,"stopEmitter","(II)V"),
        M(pause_emitter,"pauseEmitter","(II)V"),M(resume_emitter,"resumeEmitter","(II)V"),
        M(volume_emitter,"setEmitterVolume","(IIF)V"),M(pitch,"setEmitterPitch","(IIF)V"),M(pitch,"setPitch","(IIF)V"),
        M(preload,"loadMusic","(I)V"),M(preload,"loadSoundAsync","(I)V"),M(group,"loadSoundGroup","(IZ)V"),
        M(idle,"update","()V"),M(idle,"recoverAudio","()V"),M(idle,"onRecoverAudio","()V"),
        M(idle,"loadBackground","()V"),M(finished,"isFinishBackground","()Z"),M(movie,"loadMovie","(Ljava/lang/String;)V"),
        M(fx,"setfxMusicRes","(III)V"),M(stop_all,"destroy","()V"),M(idle,"reinit","()V"),{}
    };
    c.classpath="com/gameloft/android/TBFV/GloftN2HP/ML/GLMediaPlayer";c.classname="GLMediaPlayer";
    c.managed_methods=methods;c.instance_size=0;ClassRegistry::register_class(c);
}
