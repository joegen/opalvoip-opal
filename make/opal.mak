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
# $Revision: 20959 $
# $Author: ms30002000 $
# $Date: 2008-09-15 03:57:09 +1000 (Mon, 15 Sep 2008) $
#

OPAL_TOP_LEVEL_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))..)

OPAL_CONFIG_MAK := opal_config.mak
ifneq ($(OPAL_PLATFORM_DIR),)
  include $(OPAL_PLATFORM_DIR)/make/$(OPAL_CONFIG_MAK)
  OPAL_INCFLAGS := -I$(OPAL_PLATFORM_DIR)/include
  OPAL_LIBDIR = $(OPAL_PLATFORM_DIR)/lib_$(target)
else ifndef OPALDIR
  include $(shell pkg-config opal --variable=makedir)/$(OPAL_CONFIG_MAK)
  OPAL_INCFLAGS := $(shell pkg-config opal --cflags-only-I)
  OPAL_LIBDIR := $(shell pkg-config opal --variable=libdir)
else
  ifneq (,$(wildcard $(OPALDIR)/lib_$(target)/make/$(OPAL_CONFIG_MAK)))
    include $(OPALDIR)/lib_$(target)/make/$(OPAL_CONFIG_MAK)
  else
    include $(OPALDIR)/make/$(OPAL_CONFIG_MAK)
  endif
  OPAL_INCFLAGS := -I$(OPALDIR)/include
  OPAL_LIBDIR = $(OPALDIR)/lib_$(target)
  LIBDIRS += $(OPALDIR)  # Submodules built with make lib
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

OPAL_FILE_VERSION = $(OPAL_MAJOR).$(OPAL_MINOR)$(OPAL_STAGE)$(OPAL_BUILD)

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


ifneq ($(OPAL_BUILDING_ITSELF),yes)

  LDFLAGS := -L$(OPAL_LIBDIR) -l$(OPAL_LIB_BASE)$(LIB_DEBUG_SUFFIX)$(LIB_STATIC_SUFFIX) $(LDFLAGS)

  include $(PTLIB_MAKE_DIR)/ptlib.mak

endif


# End of file
