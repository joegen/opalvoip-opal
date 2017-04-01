#
# Makefile for Opus codec plugin
#
# Copyright (C) 2013 Vox Lucida Pty. Ltd., All Rights Reserved
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
# The Original Code is Open Phone Abstraction Library.
#
# The Initial Developer of the Original Code is Robert Jongbloed
#
# Contributor(s): ______________________________________.
#

BASENAME := opus

SRCDIR   := $(CURDIR)
SOURCES  := $(SRCDIR)/OpusCodec.cpp

PLUGIN_CPPFLAGS = $(OPUS_CFLAGS)
PLUGIN_LIBS     = $(OPUS_LIBS)

PLUGIN_SYSTEM  = $(OPUS_SYSTEM)

LIBOPUS_DIR := $(SRCDIR)/opus
LIBOPUS_LIB := $(LIBOPUS_DIR)/.libs/libopus.a

LOCAL_CPPFLAGS := -I$(LIBOPUS_DIR)/include
LOCAL_LIBS := $(LIBOPUS_LIB)


INSTALL_DIR = $(AUD_PLUGIN_DIR)
include $(dir $(lastword $(MAKEFILE_LIST)))../../plugin_inc.mak


ifeq ($(OPUS_SYSTEM),no)

  $(PLUGIN_PATH): $(LIBOPUS_LIB)

  $(LIBOPUS_LIB):
	$(MAKE) -C $(LIBOPUS_DIR)
endif


###########################################
