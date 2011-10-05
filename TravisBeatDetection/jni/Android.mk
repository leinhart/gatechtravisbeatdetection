LOCAL_PATH := $(call my-dir)

#---------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := pdbeatdetection
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../pdlib-joneill2s-libpd/pure-data/src
LOCAL_CFLAGS := -DPD
LOCAL_SRC_FILES := pdbeatdetection.c
LOCAL_LDLIBS := -L$(LOCAL_PATH)/../../../../pdlib-joneill2s-pd-for-android/PdCore/libs/armeabi -lpdnative
include $(BUILD_SHARED_LIBRARY)