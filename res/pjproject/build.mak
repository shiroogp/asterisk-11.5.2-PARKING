export PJDIR := /usr/src/vitel4/asterisk-11.5.1-PARKING/res/pjproject
include $(PJDIR)/version.mak
export PJ_DIR := $(PJDIR)

# build.mak.  Generated from build.mak.in by configure.
export MACHINE_NAME := auto
export OS_NAME := auto
export HOST_NAME := unix
export CC_NAME := gcc
export TARGET_NAME := x86_64-unknown-linux-gnu
export CROSS_COMPILE := 
export LINUX_POLL := select 
export SHLIB_SUFFIX := so

export ac_prefix := /usr/local

LIB_SUFFIX = $(TARGET_NAME).a

# Determine which party libraries to use
export APP_THIRD_PARTY_LIBS := -lmilenage-$(TARGET_NAME) -lsrtp-$(TARGET_NAME)
export APP_THIRD_PARTY_EXT :=
export APP_THIRD_PARTY_LIB_FILES := $(PJ_DIR)/third_party/lib/libmilenage-$(LIB_SUFFIX) $(PJ_DIR)/third_party/lib/libsrtp-$(LIB_SUFFIX)

ifeq (,1)
export PJ_RESAMPLE_DLL := 1
export APP_THIRD_PARTY_LIBS := -lresample $(APP_THIRD_PARTY_LIBS)
export APP_THIRD_PARTY_LIB_FILES := $(PJ_DIR)/third_party/lib/libresample.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/third_party/lib/libresample.$(SHLIB_SUFFIX) $(APP_THIRD_PARTY_LIB_FILES)
else
export APP_THIRD_PARTY_LIBS := -lresample-$(TARGET_NAME) $(APP_THIRD_PARTY_LIBS)
export APP_THIRD_PARTY_LIB_FILES := $(PJ_DIR)/third_party/lib/libresample-$(LIB_SUFFIX) $(APP_THIRD_PARTY_LIB_FILES)
endif

ifneq (1,1)
ifeq (0,1)
# External GSM library
APP_THIRD_PARTY_EXT += -lgsm
else
APP_THIRD_PARTY_LIBS += -lgsmcodec-$(TARGET_NAME)
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libgsmcodec-$(LIB_SUFFIX)
endif
endif

ifneq (1,1)
ifeq (0,1)
APP_THIRD_PARTY_EXT += -lspeex -lspeexdsp
else
APP_THIRD_PARTY_LIBS += -lspeex-$(TARGET_NAME)
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libspeex-$(LIB_SUFFIX)
endif
endif

ifneq (1,1)
APP_THIRD_PARTY_LIBS += -lilbccodec-$(TARGET_NAME)
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libilbccodec-$(LIB_SUFFIX)
endif

ifneq (1,1)
APP_THIRD_PARTY_LIBS += -lg7221codec-$(TARGET_NAME)
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libg7221codec-$(LIB_SUFFIX)
endif

ifneq ($(findstring pa,null),)
ifeq (0,1)
# External PA
APP_THIRD_PARTY_EXT += -lportaudio
else
APP_THIRD_PARTY_LIBS += -lportaudio-$(TARGET_NAME)
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libportaudio-$(LIB_SUFFIX)
endif
endif

# Additional flags


#
# Video
# Note: there are duplicated macros in pjmedia/os-auto.mak.in (and that's not
#       good!

# SDL flags
SDL_CFLAGS = 
SDL_LDFLAGS = 

# FFMPEG dlags
FFMPEG_CFLAGS =   
FFMPEG_LDFLAGS =   

# Video4Linux2
V4L2_CFLAGS = 
V4L2_LDFLAGS = 

# QT
AC_PJMEDIA_VIDEO_HAS_QT = 
QT_CFLAGS = 

# iOS
IOS_CFLAGS = 

# PJMEDIA features exclusion
PJ_VIDEO_CFLAGS += $(SDL_CFLAGS) $(FFMPEG_CFLAGS) $(V4L2_CFLAGS) $(QT_CFLAGS) \
		   $(IOS_CFLAGS)
PJ_VIDEO_LDFLAGS += $(SDL_LDFLAGS) $(FFMPEG_LDFLAGS) $(V4L2_LDFLAGS)


# CFLAGS, LDFLAGS, and LIBS to be used by applications
export APP_CC := gcc
export APP_CXX := g++
export APP_CFLAGS := -DPJ_AUTOCONF=1\
	-fPIC -DPJ_IS_BIG_ENDIAN=0 -DPJ_IS_LITTLE_ENDIAN=1\
	$(PJ_VIDEO_CFLAGS) \
	-I$(PJDIR)/pjlib/include\
	-I$(PJDIR)/pjlib-util/include\
	-I$(PJDIR)/pjnath/include\
	-I$(PJDIR)/pjmedia/include\
	-I$(PJDIR)/pjsip/include
export APP_CXXFLAGS := $(APP_CFLAGS)
export APP_LDFLAGS := -L$(PJDIR)/pjlib/lib\
	-L$(PJDIR)/pjlib-util/lib\
	-L$(PJDIR)/pjnath/lib\
	-L$(PJDIR)/pjmedia/lib\
	-L$(PJDIR)/pjsip/lib\
	-L$(PJDIR)/third_party/lib\
	$(PJ_VIDEO_LDFLAGS) \
	
export APP_LDLIBS := -lpjnath-$(TARGET_NAME)\
	-lpjlib-util-$(TARGET_NAME)\
	-lpj-$(TARGET_NAME)\
	-luuid -lm -lnsl -lrt -lpthread    -lcrypto -lssl
export APP_LIB_FILES = $(PJ_DIR)/pjnath/lib/libpjnath-$(LIB_SUFFIX) \
	$(PJ_DIR)/pjlib-util/lib/libpjlib-util-$(LIB_SUFFIX) \
	$(PJ_DIR)/pjlib/lib/libpj-$(LIB_SUFFIX)

# Here are the variabels to use if application is using the library
# from within the source distribution
export PJ_CC := $(APP_CC)
export PJ_CXX := $(APP_CXX)
export PJ_CFLAGS := $(APP_CFLAGS)
export PJ_CXXFLAGS := $(APP_CXXFLAGS)
export PJ_LDFLAGS := $(APP_LDFLAGS)
export PJ_LDLIBS := $(APP_LDLIBS)
export PJ_LIB_FILES := $(APP_LIB_FILES)

# And here are the variables to use if application is using the
# library from the install location (i.e. --prefix)
export PJ_INSTALL_DIR := /usr/local
export PJ_INSTALL_INC_DIR := $(PJ_INSTALL_DIR)/include
export PJ_INSTALL_LIB_DIR := $(PJ_INSTALL_DIR)/lib
export PJ_INSTALL_CFLAGS := -I$(PJ_INSTALL_INC_DIR) -DPJ_AUTOCONF=1	-fPIC -DPJ_IS_BIG_ENDIAN=0 -DPJ_IS_LITTLE_ENDIAN=1
export PJ_INSTALL_CXXFLAGS := $(PJ_INSTALL_CFLAGS)
export PJ_INSTALL_LDFLAGS := -L$(PJ_INSTALL_LIB_DIR) $(APP_LDLIBS)
