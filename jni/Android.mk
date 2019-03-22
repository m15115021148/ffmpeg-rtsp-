LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libavcodec-56
LOCAL_SRC_FILES := ffmpeg/lib/libavcodec-56.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libavdevice-56
LOCAL_SRC_FILES := ffmpeg/lib/libavdevice-56.so
include $(PREBUILT_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := libavfilter-5
LOCAL_SRC_FILES := ffmpeg/lib/libavfilter-5.so
include $(PREBUILT_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := libavformat-56
LOCAL_SRC_FILES := ffmpeg/lib/libavformat-56.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libavutil-54
LOCAL_SRC_FILES := ffmpeg/lib/libavutil-54.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libpostproc-53
LOCAL_SRC_FILES := ffmpeg/lib/libpostproc-53.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libswresample-1
LOCAL_SRC_FILES := ffmpeg/lib/libswresample-1.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libswscale-3
LOCAL_SRC_FILES := ffmpeg/lib/libswscale-3.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libyuv
LOCAL_SRC_FILES := ffmpeg/lib/libyuv.so
include $(PREBUILT_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := cubic
LOCAL_SRC_FILES := coreApp.cpp \
			WlListener.cpp


LOCAL_SHARED_LIBRARIES  := libavcodec-56 \
						  libavdevice-56 \
						  libavfilter-5 \
						  libavformat-56 \
						  libavutil-54 \
						  libpostproc-53 \
						  libswresample-1 \
						  libswscale-3 \
						  libyuv \

LOCAL_C_INCLUDES := $(LOCAL_PATH)/ffmpeg/include

LOCAL_LDLIBS := -llog -lz -ldl

include $(BUILD_SHARED_LIBRARY)
