LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	generate-emulation.c \
	orcarm.c \
	orc.c \
	orccodemem.c \
	orccompiler.c \
	orccpu-arm.c \
	orcdebug.c \
	orcemulateopcodes.c \
	orcexecutor.c \
	orcfunctions.c \
	orcfunctions.orc \
	orconce.c \
	orcopcodes.c \
	orcparse.c \
	orcpowerpc.c \
	orcprogram-arm.c \
	orcprogram.c \
	orcprogram-c.c \
	orcrule.c \
	orcrules-arm.c \
	orccode.c \
	orcutils.c

LOCAL_SHARED_LIBRARIES := libm

LOCAL_MODULE:= liborc

LOCAL_C_INCLUDES := 			\
	$(LIBORC_TOP)			\
	$(LOCAL_PATH)

LOCAL_CFLAGS := \
    -DHAVE_CONFIG_H		    \
    -D_BSD_SOURCE           \
    -D_GNU_SOURCE	        \
    -DORC_ENABLE_UNSTABLE_API

LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)
