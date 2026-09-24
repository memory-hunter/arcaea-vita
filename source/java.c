#include <falso_jni/FalsoJNI.h>
#include <falso_jni/FalsoJNI_Impl.h>
#include <falso_jni/FalsoJNI_Logger.h>

#include "SharedPreferences.h"
#include "vitasdk.h"

/*
 * Hill Climb Racing methods
 */

void setAnimationInterval(jmethodID id, va_list args)
{
	jdouble interval = va_arg(args, jdouble);
	fjni_logv_dbg("[FalsoJNI] setAnimationInterval(%f) called", (float)interval);
}

jstring getCocos2dxPackageName(jmethodID id, va_list args)
{
	fjni_logv_dbg("%s", "[FalsoJNI] getCocos2dxPackageName() called");
	return jni->NewStringUTF(&jni, "arcaea");
}

jint getMarketVariation(jmethodID id, va_list args)
{
	fjni_logv_dbg("%s", "[FalsoJNI] getMarketVariation() called");
	return 0;
}
jstring getAndroidVersion(jmethodID id, va_list args)
{
	fjni_logv_dbg("%s", "[FalsoJNI] getAndroidVersion() called");
	return jni->NewStringUTF(&jni, "4.4");
}

jint getSettingInt(jmethodID id, va_list args)
{
	jstring key = va_arg(args, jstring);
	jint defaultValue = va_arg(args, jint);
	fjni_logv_dbg("[FalsoJNI] getSettingInt(%s, %d) called", key, defaultValue);
	return defaultValue;
}

jboolean hasInstallReward(jmethodID id, va_list args)
{
	fjni_logv_dbg("%s", "[FalsoJNI] flush() called");
	return JNI_FALSE;
}

jint getIAPCoins(jmethodID id, va_list args)
{
	fjni_logv_dbg("%s", "[FalsoJNI] getIAPCoins() called");
	return 0;
}

jint getIAPAdFree(jmethodID id, va_list args)
{
	fjni_logv_dbg("%s", "[FalsoJNI] getIAPAdFree() called");
	return 0;
}

jboolean hasValue(jmethodID id, va_list args)
{
	jstring key = va_arg(args, jstring);
	fjni_logv_dbg("[FalsoJNI] hasValue(%s) called", key);
	return JNI_FALSE;
}

void startAdView(jmethodID id, va_list args)
{
	fjni_logv_dbg("%s", "[FalsoJNI] startAdView() called");
}

void trackPage(jmethodID id, va_list args)
{
	jstring arg1 = va_arg(args, jstring);
	fjni_logv_dbg("[FalsoJNI] trackPage(%s) called", arg1);
}

void stopAdView(jmethodID id, va_list args)
{
	fjni_logv_dbg("%s", "[FalsoJNI] stopAdView() called");
}

jint getApiLevel(jmethodID id, va_list args)
{
	fjni_logv_dbg("%s", "[FalsoJNI] getApiLevel() called");
	return 19;
}

jobject dummyObject(jmethodID id, va_list args)
{
	return (jobject)0x69426767;
}

jobject getClassLoader(jmethodID id, va_list args)
{
	fjni_logv_dbg("%s", "[FalsoJNI] getClassLoader() called");
	return (jobject)0x42424242;
}

jstring getCurrentLanguage(jmethodID id, va_list args)
{
	int lang = 0; // default to eng
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &lang);

	const char *code;
	switch (lang)
	{
	case SCE_SYSTEM_PARAM_LANG_FRENCH:
		code = "fr";
		break;
	case SCE_SYSTEM_PARAM_LANG_SPANISH:
		code = "es";
		break;
	case SCE_SYSTEM_PARAM_LANG_ITALIAN:
		code = "it";
		break;
	case SCE_SYSTEM_PARAM_LANG_GERMAN:
		code = "de";
		break;
	case SCE_SYSTEM_PARAM_LANG_RUSSIAN:
		code = "ru";
		break;
	case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_BR:
		code = "pt";
		break;
	case SCE_SYSTEM_PARAM_LANG_CHINESE_S:
	case SCE_SYSTEM_PARAM_LANG_CHINESE_T:
		code = "zh";
		break;
	case SCE_SYSTEM_PARAM_LANG_ENGLISH_US:
	case SCE_SYSTEM_PARAM_LANG_ENGLISH_GB:
	default:
		code = "en";
		break;
	}

	fjni_logv_dbg("[FalsoJNI] getCurrentLanguage() called -> %s", code);
	return jni->NewStringUTF(&jni, code);
}

jobject getCocos2dxWritablePath(jmethodID id, va_list args)
{
	fjni_logv_dbg("%s", "[FalsoJNI] getCocos2dxWritablePath() called");
	return jni->NewStringUTF(&jni, "data/arcaea");
}

jstring generateGuid(jmethodID id, va_list args)
{
	fjni_logv_dbg("%s", "[FalsoJNI] generateGuid() called");
	return jni->NewStringUTF(&jni, "00000000-0000-0000-0000-000000000000");
}

void dummyVoid(jmethodID id, va_list args)
{
}

void logCrash(jmethodID id, va_list args)
{
	jstring msg = va_arg(args, jstring);
	fjni_logv_dbg("[FalsoJNI] logCrashylitcs logs: %s", msg);
}

/*
 * JNI Methods
 */

// start functions from id 100
NameToMethodID nameToMethodId[] = {
	{100, "setAnimationInterval", METHOD_TYPE_VOID},
	{101, "getIntegerForKey", METHOD_TYPE_INT},
	{102, "getCocos2dxPackageName", METHOD_TYPE_OBJECT},
	{103, "getMarketVariation", METHOD_TYPE_INT},
	{104, "getCurrentLanguage", METHOD_TYPE_OBJECT},
	{105, "getAndroidVersion", METHOD_TYPE_OBJECT},
	{109, "getSettingInt", METHOD_TYPE_INT},
	{110, "setStringForKey", METHOD_TYPE_VOID},
	{111, "flush", METHOD_TYPE_VOID},
	{112, "getStringForKey", METHOD_TYPE_OBJECT},
	{113, "setIntegerForKey", METHOD_TYPE_VOID},
	{114, "hasInstallReward", METHOD_TYPE_BOOLEAN},
	{115, "getIAPCoins", METHOD_TYPE_INT},
	{116, "getIAPAdFree", METHOD_TYPE_INT},
	{117, "hasValue", METHOD_TYPE_BOOLEAN},
	{118, "startAdView", METHOD_TYPE_VOID},
	{119, "stopAdView", METHOD_TYPE_VOID},
	{120, "trackPage", METHOD_TYPE_VOID},
	{121, "getApiLevel", METHOD_TYPE_INT},
	{210, "getBoolForKey", METHOD_TYPE_BOOLEAN},
	{211, "getFloatForKey", METHOD_TYPE_FLOAT},
	{212, "setFloatForKey", METHOD_TYPE_VOID},
	{213, "setBoolForKey", METHOD_TYPE_VOID},
	{215, "generateGuid", METHOD_TYPE_OBJECT},
	{301, "loadClass", METHOD_TYPE_OBJECT},
	{302, "getClassLoader", METHOD_TYPE_OBJECT},
	{303, "getCocos2dxWritablePath", METHOD_TYPE_OBJECT},
	{304, "logCrashlytics", METHOD_TYPE_VOID},
};

MethodsBoolean methodsBoolean[] = {
	{114, hasInstallReward},
	{117, hasValue},
	{210, getBoolForKey},
};
MethodsByte methodsByte[] = {};
MethodsChar methodsChar[] = {};
MethodsDouble methodsDouble[] = {};
MethodsFloat methodsFloat[] = {
	{211, getFloatForKey},
};
MethodsInt methodsInt[] = {
	{101, getIntegerForKey},
	{103, getMarketVariation},
	{109, getSettingInt},
	{115, getIAPCoins},
	{116, getIAPAdFree},
	{121, getApiLevel},
};

MethodsLong methodsLong[] = {};
MethodsObject methodsObject[] = {
	{102, getCocos2dxPackageName},
	{104, getCurrentLanguage},
	{105, getAndroidVersion},
	{112, getStringForKey},
	{301, dummyObject},
	{302, getClassLoader},
	{303, getCocos2dxWritablePath},
	{215, generateGuid},
};
MethodsShort methodsShort[] = {};
MethodsVoid methodsVoid[] = {
	{100, setAnimationInterval},
	{110, setStringForKey},
	{111, flush},
	{113, setIntegerForKey},
	{118, startAdView},
	{119, stopAdView},
	{120, trackPage},
	{212, setFloatForKey},
	{213, setBoolForKey},
	{304, logCrash},
	{305, dummyVoid},
};

/*
 * JNI Fields
 */

// System-wide constant that applications sometimes request
// https://developer.android.com/reference/android/content/Context.html#WINDOW_SERVICE
char WINDOW_SERVICE[] = "window";

// System-wide constant that's often used to determine Android version
// https://developer.android.com/reference/android/os/Build.VERSION.html#SDK_INT
// Possible values: https://developer.android.com/reference/android/os/Build.VERSION_CODES
const int SDK_INT = 19; // Android 4.4 / KitKat

NameToFieldID nameToFieldId[] = {
	{0, "WINDOW_SERVICE", FIELD_TYPE_OBJECT},
	{1, "SDK_INT", FIELD_TYPE_INT},
};

FieldsBoolean fieldsBoolean[] = {};
FieldsByte fieldsByte[] = {};
FieldsChar fieldsChar[] = {};
FieldsDouble fieldsDouble[] = {};
FieldsFloat fieldsFloat[] = {};
FieldsInt fieldsInt[] = {
	{1, SDK_INT},
};
FieldsObject fieldsObject[] = {
	{0, WINDOW_SERVICE},
};
FieldsLong fieldsLong[] = {};
FieldsShort fieldsShort[] = {};

__FALSOJNI_IMPL_CONTAINER_SIZES