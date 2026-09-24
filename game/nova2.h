#pragma once
#include <SDL2/SDL.h>
#include "so_util.h"
#include "jni.h"
#include "jni_internals.h"
#include <string>
extern so_module *nova_module;
extern JNIEnv *nova_env;
extern SDL_Window *nova_window;
extern int nova_width, nova_height;
extern std::string nova_donor_root;
extern std::string nova_asset_root;
extern Class game_class, renderer_class, resources_class, media_class, device_class, music_class;
void register_services();
std::string data_path(const char *path);
void input_init();
void input_event(const SDL_Event &event);
void input_tick(float dt);
template<class T> T native(const char *suffix) {
    std::string name="Java_com_gameloft_android_TBFV_GloftN2HP_ML_";
    name+=suffix;
    auto ptr=so_symbol(nova_module,name.c_str());
    if (!ptr) {
        fprintf(stderr,"Required native entry missing: %s\n",name.c_str());
        exit(1);
    }
    return reinterpret_cast<T>(ptr);
}
