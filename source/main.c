#include "utils/init.h"
#include "utils/glutil.h"
#include "utils/logger.h"
#include "utils/dialog.h"
#include "utils/utils.h"
#include "reimpl/asset_manager.h"
#include "SharedPreferences.h"

#include <psp2/kernel/threadmgr.h>
#include <vitasdk.h>

#include <falso_jni/FalsoJNI.h>
#include <so_util/so_util.h>

#include <stdlib.h>

#ifndef NDK_PORT
#include "reimpl/controls.h"
#else
#include <falso_ndk/FalsoNDK.h>
#endif

int _newlib_heap_size_user = 256 * 1024 * 1024;

#ifdef USE_SCELIBC_IO
int sceLibcHeapSize = 12 * 1024 * 1024;
#endif

so_module so_mod;
so_module fmod_mod;
so_module provider_mod;

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 544
#define MAIN_JNI_FUNC_2_DECL(name, ...) \
    int (*name)(void *env, void *obj, ##__VA_ARGS__)
#define MAIN_JNI_FUNC_1_DECL(name, ...) \
    int (*name)(void *env)
#define MAIN_JNI_FUNC_0_DECL(name, ...) \
    int (*name)(void)
#define MAIN_JNI_FUNC_2(name, ...) \
    int (*name)(void *env, void *obj, ##__VA_ARGS__) = (void *)so_symbol(&so_mod, #name)
#define MAIN_JNI_FUNC_1(name, ...) \
    int (*name)(void *env) = (void *)so_symbol(&so_mod, #name)
#define MAIN_JNI_FUNC_0(name, ...) \
    int (*name)() = (void *)so_symbol(&so_mod, #name)
#define PROVIDER_JNI_FUNC_2(name, ...) \
    int (*name)(void *env, void *obj, ##__VA_ARGS__) = (void *)so_symbol(&provider_mod, #name)
#define PROVIDER_JNI_FUNC_1(name, ...) \
    int (*name)(void *env) = (void *)so_symbol(&provider_mod, #name)
#define PROVIDER_JNI_FUNC_0(name, ...) \
    int (*name)() = (void *)so_symbol(&provider_mod, #name)
#define JNI_STR(str) jni->NewStringUTF(&jni, str)

MAIN_JNI_FUNC_2_DECL(Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesBegin, jint id, jfloat x, jfloat y);
MAIN_JNI_FUNC_2_DECL(Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesEnd, jint id, jfloat x, jfloat y);
MAIN_JNI_FUNC_2_DECL(Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesMove, jintArray ids, jfloatArray xs, jfloatArray ys);
MAIN_JNI_FUNC_2_DECL(Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeKeyEvent, jint keycode, jboolean isPressed);

int main()
{
    soloader_init_all();

    sceSysmoduleLoadModule(SCE_SYSMODULE_AVPLAYER);
    l_debug("[main] Loading FMOD Studio...");
    if (!file_exists("ur0:/data/libfmodstudio.suprx"))
        fatal_error("Error libfmodstudio.suprx is not installed.");
    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    int ret = sceNetShowNetstat();
    SceNetInitParam initparam;
    if (ret == SCE_NET_ERROR_ENOTINIT)
    {
        initparam.memory = malloc(141 * 1024);
        initparam.size = 141 * 1024;
        initparam.flags = 0;
        sceNetInit(&initparam);
    }
    uintptr_t a1 = sceKernelLoadStartModule("vs0:sys/external/libfios2.suprx", 0, NULL, 0, NULL, NULL);
    uintptr_t a2 = sceKernelLoadStartModule("vs0:sys/external/libc.suprx", 0, NULL, 0, NULL, NULL);
    uintptr_t a3 = sceKernelLoadStartModule("ur0:data/libfmodstudio.suprx", 0, NULL, 0, NULL, NULL);
    l_debug("[main] sceKernelLoadStartModule %x", a1);
    l_debug("[main] sceKernelLoadStartModule %x", a2);
    l_debug("[main] sceKernelLoadStartModule %x", a3);

    int (*JNI_OnLoad)(void *jvm) = (void *)so_symbol(&so_mod, "JNI_OnLoad");
    MAIN_JNI_FUNC_2(Java_com_sdkbox_plugin_SDKBox_nativeInit, jobject context, jobject classloader);
    MAIN_JNI_FUNC_2(Java_org_cocos2dx_lib_Cocos2dxHelper_nativeSetApkPath, jstring apkPath);
    MAIN_JNI_FUNC_2(Java_org_cocos2dx_lib_Cocos2dxHelper_nativeSetContext, jobject context, jobject aam);
    MAIN_JNI_FUNC_0(Java_com_sdkbox_plugin_SDKBox_nOnStart);
    MAIN_JNI_FUNC_0(Java_com_sdkbox_plugin_SDKBox_nOnResume);
    MAIN_JNI_FUNC_2(Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInit, jint width, jint height);
    MAIN_JNI_FUNC_2(Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeOnSurfaceChanged, jint width, jint height);
    MAIN_JNI_FUNC_0(Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeRender);
    MAIN_JNI_FUNC_0(Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeOnResume);
    MAIN_JNI_FUNC_0(Java_org_cocos2dx_cpp_AppActivity_nativeResume);
    MAIN_JNI_FUNC_2(Java_org_cocos2dx_cpp_AppActivity_setObbPath, jstring path);
    MAIN_JNI_FUNC_1(Java_org_cocos2dx_cpp_AppActivity_initJVMPlatformUtils);
    MAIN_JNI_FUNC_1(Java_org_cocos2dx_cpp_AppActivity_initJVMAnalytics);
    PROVIDER_JNI_FUNC_0(Java_org_cocos2dx_cpp_AppActivity_setStateStart);
    PROVIDER_JNI_FUNC_2(Java_org_cocos2dx_cpp_AppActivity_setAndroidAudioProperties, jint samplerate, jint buffersize, jboolean bluetoothprobably);
    MAIN_JNI_FUNC_0(Java_org_cocos2dx_cpp_AppActivity_notifyExpansionCheckComplete);
    Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesBegin = (void *)so_symbol(&so_mod, "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesBegin");
    Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesEnd = (void *)so_symbol(&so_mod, "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesEnd");
    Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesMove = (void *)so_symbol(&so_mod, "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesMove");
    Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeKeyEvent = (void *)so_symbol(&so_mod, "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeKeyEvent");

    gl_init();
    controls_init();
    prefs_init();

    JNI_OnLoad(&jvm);
    l_debug("[main] JNI_OnLoad");

    Java_com_sdkbox_plugin_SDKBox_nativeInit(&jni, NULL, NULL, (jobject)0x67676767);
    l_debug("[main] sdkboxinit");
    Java_org_cocos2dx_lib_Cocos2dxHelper_nativeSetApkPath(&jni, NULL, JNI_STR(DATA_PATH "base.apk"));
    l_debug("[main] apkpath");
    AAssetManager *aam = AAssetManager_create();
    Java_org_cocos2dx_lib_Cocos2dxHelper_nativeSetContext(&jni, NULL, (jobject)0x69696969, aam);
    l_debug("[main] aam");
    Java_org_cocos2dx_cpp_AppActivity_setObbPath(&jni, NULL, JNI_STR(DATA_PATH"assets"));
    Java_org_cocos2dx_cpp_AppActivity_notifyExpansionCheckComplete();
    l_debug("[main] obb");
    Java_org_cocos2dx_cpp_AppActivity_initJVMAnalytics(&jni);
    Java_org_cocos2dx_cpp_AppActivity_initJVMPlatformUtils(&jni);
    l_debug("[main] jvm shi");

    Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInit(&jni, NULL, SCREEN_WIDTH, SCREEN_HEIGHT);
    l_debug("[main] renderer");

    Java_org_cocos2dx_cpp_AppActivity_setStateStart();
    Java_org_cocos2dx_cpp_AppActivity_setAndroidAudioProperties(&jni, NULL, 44100, 512, JNI_FALSE);
    l_debug("[main] fmod shit");

    Java_com_sdkbox_plugin_SDKBox_nOnStart();
    l_debug("[main] startsdkbox");

    Java_org_cocos2dx_cpp_AppActivity_nativeResume();
    Java_com_sdkbox_plugin_SDKBox_nOnResume();
    Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeOnResume();

#ifndef NDK_PORT
    // ... do some initialization

    while (1)
    {
        // ... render call
        controls_poll();
        Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeOnSurfaceChanged(&jni, NULL, SCREEN_WIDTH, SCREEN_HEIGHT);
        Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeRender();
        gl_swap();
    }
#else
    // Build a fake ANativeActivity that the game's onCreate will receive
    ANativeActivity *activity = malloc(sizeof(ANativeActivity));
    activity->callbacks = malloc(sizeof(ANativeActivityCallbacks));
    activity->env = &jni; // from FalsoJNI
    activity->vm = &jvm;  // from FalsoJNI
    activity->clazz = (jclass)0x42424242;
    activity->internalDataPath = DATA_PATH "assets/";
    activity->externalDataPath = DATA_PATH "assets/";
    activity->sdkVersion = 14;
    activity->instance = NULL;

    // Drive the activity lifecycle
    int (*ANativeActivity_onCreate)(ANativeActivity *, void *, size_t) =
        (void *)so_symbol(&so_mod, "ANativeActivity_onCreate");
    ANativeActivity_onCreate(activity, NULL, 0);

    activity->callbacks->onStart(activity);
    activity->callbacks->onResume(activity);

    // Wire up input and the native window
    AInputQueue *aInputQueue = AInputQueue_create();
    activity->callbacks->onInputQueueCreated(activity, aInputQueue);

    ANativeWindow *aNativeWindow = ANativeWindow_create();
    activity->callbacks->onNativeWindowCreated(activity, aNativeWindow);

    activity->callbacks->onWindowFocusChanged(activity, 1);
#endif

    prefs_destroy();

    sceKernelExitDeleteThread(0);
}

#ifndef NDK_PORT

void controls_handler_key(int32_t keycode, ControlsAction action)
{
    Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeKeyEvent(&jni, NULL, keycode, action == CONTROLS_ACTION_DOWN);
}

void controls_handler_touch(int32_t id, float x, float y, ControlsAction action)
{
    switch (action)
    {
    case CONTROLS_ACTION_DOWN:
        Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesBegin(&jni, NULL, id, x, y);
        break;
    case CONTROLS_ACTION_UP:
        Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesEnd(&jni, NULL, id, x, y);
        break;
    case CONTROLS_ACTION_MOVE:
    {
        jint ids[1] = {id};
        jfloat xs[1] = {x};
        jfloat ys[1] = {y};

        jintArray jids = jni->NewIntArray(&jni, 1);
        jni->SetIntArrayRegion(&jni, jids, 0, 1, ids);

        jfloatArray jxs = jni->NewFloatArray(&jni, 1);
        jni->SetFloatArrayRegion(&jni, jxs, 0, 1, xs);

        jfloatArray jys = jni->NewFloatArray(&jni, 1);
        jni->SetFloatArrayRegion(&jni, jys, 0, 1, ys);

        Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesMove(&jni, NULL, jids, jxs, jys);
        break;
    }
    default:
        break;
    }
}

void controls_handler_analog(ControlsStickId which, float x, float y, ControlsAction action)
{
    // Call into the .so here
}
#endif
