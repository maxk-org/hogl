#
LOCAL_PATH:= $(call my-dir)

#
#
include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	src/area.cc \
	src/c-api.cc \
	src/engine.cc \
	src/flush.cc \
	src/format-basic.cc \
	src/format-raw.cc \
	src/internal.cc \
	src/mask.cc \
	src/schedparam.cc \
	src/ostrbuf.cc \
	src/ostrbuf-fd.cc \
	src/ostrbuf-stdio.cc \
	src/output.cc \
	src/output-file.cc \
	src/output-pipe.cc \
	src/output-stderr.cc \
	src/output-stdout.cc \
	src/output-textfile.cc \
	src/platform.cc \
	src/post.cc \
	src/ringbuf.cc \
	src/timesource.cc \
	src/tls.cc \
	src/default-ringopts.cc \
	src/default-timesource.cc \
	src/default-engine.cc

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_CFLAGS   += -Wall -O3
LOCAL_CXXFLAGS += -Wall -O3 -std=c++11
LOCAL_CPP_FEATURES := exceptions
LOCAL_SHARED_LIBRARIES:= libffi
LOCAL_MODULE := libhogl
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	tests/basic_test.cc

LOCAL_CXXFLAGS += -Wall -O3 -std=c++11
LOCAL_CPP_FEATURES := exceptions
LOCAL_SHARED_LIBRARIES:= libhogl
LOCAL_MODULE := hogl-basic-test
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	tests/stress_test.cc

LOCAL_CXXFLAGS += -Wall -O3 -std=c++11
LOCAL_CPP_FEATURES := exceptions
LOCAL_SHARED_LIBRARIES:= libhogl
LOCAL_MODULE := hogl-stress-test
include $(BUILD_EXECUTABLE)

# Import libffi
$(call import-module,libffi)
