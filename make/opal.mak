#
# opal.mak
#
# Default build environment for OPAL applications
#
# Copyright (c) 2001 Equivalence Pty. Ltd.
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
# the License for the specific language governing rights and limitations
# under the License.
#
# The Original Code is Open Phone Abstraction library.
#
# The Initial Developer of the Original Code is Equivalence Pty. Ltd.
#
# Contributor(s): ______________________________________.
#
#

OPAL_TOP_LEVEL_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))..)

OPAL_CONFIG_MAK := opal_config.mak
ifneq ($(OPAL_PLATFORM_DIR),)
  include $(OPAL_PLATFORM_DIR)/make/$(OPAL_CONFIG_MAK)
  OPAL_INCFLAGS := -I$(OPAL_TOP_LEVEL_DIR)/include
  ifeq ($(OPAL_TOP_LEVEL_DIR),$(OPAL_PLATFORM_DIR))
    OPAL_LIBDIR = $(OPAL_PLATFORM_DIR)/lib_$(target)
  else
    OPAL_LIBDIR = $(OPAL_PLATFORM_DIR)
  endif
else ifndef OPALDIR
  include $(shell pkg-config opal --variable=makedir)/$(OPAL_CONFIG_MAK)
  OPAL_INCFLAGS := $(shell pkg-config opal --cflags-only-I)
  OPAL_LIBDIR := $(shell pkg-config opal --variable=libdir)
else
  ifeq (,$(target))
    ifeq (,$(OS))
      OS := $(shell uname -s)
    endif
    ifeq (,$(CPU))
      CPU := $(shell uname -m)
    endif
    target := $(OS)_$(CPU)
  endif

  OPAL_CONFIG_MAK_PATH := $(OPALDIR)/lib_$(target)/make/$(OPAL_CONFIG_MAK)
  ifeq (,$(wildcard $(OPAL_CONFIG_MAK_PATH)))
    OPAL_CONFIG_MAK_PATH := $(OPALDIR)/make/$(OPAL_CONFIG_MAK)
  endif
  #$(info Including $(OPAL_CONFIG_MAK_PATH))
  include $(OPAL_CONFIG_MAK_PATH)

  OPAL_INCFLAGS := -I$(OPALDIR)/include
  OPAL_LIBDIR = $(OPALDIR)/lib_$(target)
  LIBDIRS := $(OPALDIR) $(LIBDIRS)  # Submodules built with make lib
endif

include $(PTLIB_MAKE_DIR)/pre.mak


###############################################################################
# Determine the library name

OPAL_OBJDIR = $(OPAL_LIBDIR)/obj$(OBJDIR_SUFFIX)
OPAL_DEPDIR = $(OPAL_LIBDIR)/dep$(OBJDIR_SUFFIX)

OPAL_LIB_BASE           := opal
OPAL_LIB_FILE_BASE      := lib$(OPAL_LIB_BASE)
OPAL_OPT_LIB_FILE_BASE   = $(OPAL_LIBDIR)/$(OPAL_LIB_FILE_BASE)
OPAL_OPT_SHARED_LINK     = $(OPAL_OPT_LIB_FILE_BASE).$(SHAREDLIBEXT)
OPAL_OPT_STATIC_FILE     = $(OPAL_OPT_LIB_FILE_BASE)$(STATIC_SUFFIX).$(STATICLIBEXT)
OPAL_DEBUG_LIB_FILE_BASE = $(OPAL_LIBDIR)/$(OPAL_LIB_FILE_BASE)$(DEBUG_SUFFIX)
OPAL_DEBUG_SHARED_LINK   = $(OPAL_DEBUG_LIB_FILE_BASE).$(SHAREDLIBEXT)
OPAL_DEBUG_STATIC_FILE   = $(OPAL_DEBUG_LIB_FILE_BASE)$(STATIC_SUFFIX).$(STATICLIBEXT)

ifeq ($(OPAL_OEM),0)
  OPAL_FILE_VERSION = $(OPAL_MAJOR).$(OPAL_MINOR)$(OPAL_STAGE)$(OPAL_PATCH)
else
  OPAL_FILE_VERSION = $(OPAL_MAJOR).$(OPAL_MINOR)$(OPAL_STAGE)$(OPAL_PATCH)-$(OPAL_OEM)
endif

ifneq (,$(findstring $(target_os),Darwin cygwin mingw))
  OPAL_OPT_SHARED_FILE   = $(OPAL_OPT_LIB_FILE_BASE).$(OPAL_FILE_VERSION).$(SHAREDLIBEXT)
  OPAL_DEBUG_SHARED_FILE = $(OPAL_DEBUG_LIB_FILE_BASE).$(OPAL_FILE_VERSION).$(SHAREDLIBEXT)
else
  OPAL_OPT_SHARED_FILE   = $(OPAL_OPT_LIB_FILE_BASE).$(SHAREDLIBEXT).$(OPAL_FILE_VERSION)
  OPAL_DEBUG_SHARED_FILE = $(OPAL_DEBUG_LIB_FILE_BASE).$(SHAREDLIBEXT).$(OPAL_FILE_VERSION)
endif

ifeq ($(DEBUG_BUILD),yes)
  OPAL_STATIC_LIB_FILE = $(OPAL_DEBUG_STATIC_FILE)
  OPAL_SHARED_LIB_LINK = $(OPAL_DEBUG_SHARED_LINK)
  OPAL_SHARED_LIB_FILE = $(OPAL_DEBUG_SHARED_FILE)
else
  OPAL_STATIC_LIB_FILE = $(OPAL_OPT_STATIC_FILE)
  OPAL_SHARED_LIB_LINK = $(OPAL_OPT_SHARED_LINK)
  OPAL_SHARED_LIB_FILE = $(OPAL_OPT_SHARED_FILE)
endif

OPAL_LIBS = -L$(OPAL_LIBDIR) -l$(OPAL_LIB_BASE)$(LIB_DEBUG_SUFFIX)$(LIB_STATIC_SUFFIX)

BUNDLE_FILES += $(OPAL_SHARED_LIB_FILE) $(OPAL_SHARED_LIB_LINK)
ifneq ($(OPAL_LIBDIR),$(PTLIB_LIBDIR))
  BUNDLE_FILES += `find $(OPAL_LIBDIR) -name \*$(PTLIB_PLUGIN_SUFFIX).$(SHAREDLIBEXT)` `find $(OPAL_LIBDIR) -name h264_video_pwplugin_helper`
endif


###############################################################################
# Add common directory to include path
# Note also have include directory that is always relative to the
# ptlib_config.mak file in OPAL_PLATFORM_INC_DIR

ifeq (,$(findstring $(OPAL_INCFLAGS),$(CPPFLAGS)))
  CPPFLAGS := $(OPAL_INCFLAGS) $(CPPFLAGS)
endif

ifeq (,$(findstring $(OPAL_PLATFORM_INC_DIR),$(CPPFLAGS)))
  CPPFLAGS := -I$(OPAL_PLATFORM_INC_DIR) $(CPPFLAGS)
endif


ifeq ($(OPAL_BUILDING_ITSELF),yes)
  LIBS := $(PTLIB_LIBS) $(LIBS)
else
  LIBS := $(OPAL_LIBS) $(PTLIB_LIBS) $(LIBS)
  include $(PTLIB_MAKE_DIR)/post.mak
endif


###############################################################################
#
# Set up compiler flags and macros for debug/release versions
#

ifeq ($(DEBUG_BUILD),yes)
  CPPFLAGS += $(DEBUG_CPPFLAGS)
  CXXFLAGS += $(DEBUG_CFLAGS)
  CFLAGS   += $(DEBUG_CFLAGS)
  LDFLAGS  := $(DEBUG_CFLAGS) $(LDFLAGS)
else
  CPPFLAGS += $(OPT_CPPFLAGS)
  CXXFLAGS += $(OPT_CFLAGS)
  CFLAGS   += $(OPT_CFLAGS)
  LDFLAGS  := $(OPT_CFLAGS) $(LDFLAGS)
endif


# End of file
