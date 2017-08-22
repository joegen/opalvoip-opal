#
# Makefile for the x264 H.264 Video Codec
#
# Copyright (C) 2004 Post Increment, All Rights Reserved
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
# The Original Code is Open H323 library.
#
# The Initial Developer of the Original Code is Post Increment
#
# Contributor(s): ______________________________________.
#

BASENAME := h264_x264

COMMONDIR := $(abspath ../common)
SRCDIR    := $(CURDIR)
SOURCES   := $(CURDIR)/h264-x264.cxx \
             $(CURDIR)/x264wrap.cxx \
             $(COMMONDIR)/h264frame.cxx \
             $(COMMONDIR)/encframe.cxx \
             $(COMMONDIR)/ffmpeg.cxx

PLUGIN_CPPFLAGS = $(X264_CFLAGS) $(LIBAVCODEC_CFLAGS) -I$(COMMONDIR) -DVID_PLUGIN_DIR='"$(VID_PLUGIN_DIR)"'
PLUGIN_LIBS     = $(X264_LIBS) $(LIBAVCODEC_LIBS) $(DLFCN_LIBS)

SUBDIRS = $(X264_HELPER)

vpath	%.cxx $(COMMONDIR)

INSTALL_DIR = $(VID_PLUGIN_DIR)
include $(dir $(lastword $(MAKEFILE_LIST)))../../plugin_inc.mak


###########################################
