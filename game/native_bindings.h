#pragma once
#include "jni.h"
#include <cstdint>
// Copyright (c) 2026 Pixelforge Ports contributors
// Exact declarations read from the supplied APK. No Android code included.
#if defined(__arm__)
#define GUEST_ABI __attribute__((pcs("aapcs")))
#else
#define GUEST_ABI
#endif
namespace donor {
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GameGLSurfaceView; nativePause()V
using GameGLSurfaceView_nativePause_0 = void (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *GameGLSurfaceView_nativePause_0_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GameGLSurfaceView_nativePause";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GameGLSurfaceView; nativeResume()V
using GameGLSurfaceView_nativeResume_1 = void (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *GameGLSurfaceView_nativeResume_1_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GameGLSurfaceView_nativeResume";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GameRenderer; nativeInit(III)V
using GameRenderer_nativeInit_2 = void (GUEST_ABI *)(JNIEnv *, jclass ,jint, jint, jint);
inline constexpr const char *GameRenderer_nativeInit_2_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GameRenderer_nativeInit";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GameRenderer; nativeRender()V
using GameRenderer_nativeRender_3 = void (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *GameRenderer_nativeRender_3_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GameRenderer_nativeRender";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GameRenderer; nativeResize(II)V
using GameRenderer_nativeResize_4 = void (GUEST_ABI *)(JNIEnv *, jclass ,jint, jint);
inline constexpr const char *GameRenderer_nativeResize_4_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GameRenderer_nativeResize";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GameRenderer; onKeyboardFinish()V
using GameRenderer_onKeyboardFinish_5 = void (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *GameRenderer_onKeyboardFinish_5_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GameRenderer_onKeyboardFinish";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLGame; nativeAccelerometer(FFF)V
using GLGame_nativeAccelerometer_6 = void (GUEST_ABI *)(JNIEnv *, jobject ,jfloat, jfloat, jfloat);
inline constexpr const char *GLGame_nativeAccelerometer_6_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLGame_nativeAccelerometer";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLGame; nativeCanInterrupt()I
using GLGame_nativeCanInterrupt_7 = jint (GUEST_ABI *)(JNIEnv *, jobject);
inline constexpr const char *GLGame_nativeCanInterrupt_7_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLGame_nativeCanInterrupt";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLGame; nativeInit()V
using GLGame_nativeInit_8 = void (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *GLGame_nativeInit_8_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLGame_nativeInit";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLGame; nativeKeyboardEnabled(ZZ)V
using GLGame_nativeKeyboardEnabled_9 = void (GUEST_ABI *)(JNIEnv *, jclass ,jboolean, jboolean);
inline constexpr const char *GLGame_nativeKeyboardEnabled_9_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLGame_nativeKeyboardEnabled";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLGame; nativeSetOnKeyDown(I)V
using GLGame_nativeSetOnKeyDown_10 = void (GUEST_ABI *)(JNIEnv *, jclass ,jint);
inline constexpr const char *GLGame_nativeSetOnKeyDown_10_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLGame_nativeSetOnKeyDown";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLGame; nativeSetOnKeyUp(I)V
using GLGame_nativeSetOnKeyUp_11 = void (GUEST_ABI *)(JNIEnv *, jclass ,jint);
inline constexpr const char *GLGame_nativeSetOnKeyUp_11_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLGame_nativeSetOnKeyUp";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLGame; nativeSetOnVideoCompletion()V
using GLGame_nativeSetOnVideoCompletion_12 = void (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *GLGame_nativeSetOnVideoCompletion_12_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLGame_nativeSetOnVideoCompletion";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLGame; nativeTouchMoved(III)V
using GLGame_nativeTouchMoved_13 = void (GUEST_ABI *)(JNIEnv *, jclass ,jint, jint, jint);
inline constexpr const char *GLGame_nativeTouchMoved_13_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLGame_nativeTouchMoved";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLGame; nativeTouchPadMoved(III)V
using GLGame_nativeTouchPadMoved_14 = void (GUEST_ABI *)(JNIEnv *, jclass ,jint, jint, jint);
inline constexpr const char *GLGame_nativeTouchPadMoved_14_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLGame_nativeTouchPadMoved";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLGame; nativeTouchPadPressed(III)V
using GLGame_nativeTouchPadPressed_15 = void (GUEST_ABI *)(JNIEnv *, jclass ,jint, jint, jint);
inline constexpr const char *GLGame_nativeTouchPadPressed_15_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLGame_nativeTouchPadPressed";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLGame; nativeTouchPadReleased(III)V
using GLGame_nativeTouchPadReleased_16 = void (GUEST_ABI *)(JNIEnv *, jclass ,jint, jint, jint);
inline constexpr const char *GLGame_nativeTouchPadReleased_16_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLGame_nativeTouchPadReleased";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLGame; nativeTouchPressed(III)V
using GLGame_nativeTouchPressed_17 = void (GUEST_ABI *)(JNIEnv *, jclass ,jint, jint, jint);
inline constexpr const char *GLGame_nativeTouchPressed_17_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLGame_nativeTouchPressed";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLGame; nativeTouchReleased(III)V
using GLGame_nativeTouchReleased_18 = void (GUEST_ABI *)(JNIEnv *, jclass ,jint, jint, jint);
inline constexpr const char *GLGame_nativeTouchReleased_18_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLGame_nativeTouchReleased";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLGame; processTouchpadAsPointer(ILandroid/view/ViewParent;Z)Z
using GLGame_processTouchpadAsPointer_19 = jboolean (GUEST_ABI *)(JNIEnv *, jclass ,jint, jobject, jboolean);
inline constexpr const char *GLGame_processTouchpadAsPointer_19_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLGame_processTouchpadAsPointer";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLMediaPlayer; nativeGetTotalSounds()I
using GLMediaPlayer_nativeGetTotalSounds_20 = jint (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *GLMediaPlayer_nativeGetTotalSounds_20_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLMediaPlayer_nativeGetTotalSounds";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLMediaPlayer; nativeGetTotalSoundsOfSameInstance()I
using GLMediaPlayer_nativeGetTotalSoundsOfSameInstance_21 = jint (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *GLMediaPlayer_nativeGetTotalSoundsOfSameInstance_21_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLMediaPlayer_nativeGetTotalSoundsOfSameInstance";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLMediaPlayer; nativeInit()V
using GLMediaPlayer_nativeInit_22 = void (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *GLMediaPlayer_nativeInit_22_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLMediaPlayer_nativeInit";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLMediaPlayer; nativeSetStopOnMusic(I)V
using GLMediaPlayer_nativeSetStopOnMusic_23 = void (GUEST_ABI *)(JNIEnv *, jclass ,jint);
inline constexpr const char *GLMediaPlayer_nativeSetStopOnMusic_23_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLMediaPlayer_nativeSetStopOnMusic";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLResLoader; nativeInit()V
using GLResLoader_nativeInit_24 = void (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *GLResLoader_nativeInit_24_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLResLoader_nativeInit";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/Musicplayer; nativeInitplayer()V
using Musicplayer_nativeInitplayer_25 = void (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *Musicplayer_nativeInitplayer_25_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_Musicplayer_nativeInitplayer";
// Lcom/gameloft/android/TBFV/GloftN2HP/ML/GLUtils/Device; nativeInit()V
using Device_nativeInit_26 = void (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *Device_nativeInit_26_symbol = "Java_com_gameloft_android_TBFV_GloftN2HP_ML_GLUtils_Device_nativeInit";
}
#undef GUEST_ABI
