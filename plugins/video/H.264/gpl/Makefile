#
# Makefile for x264 H.264 Video Codec Helper
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
# $Revision$
# $Author$
# $Date$

BASENAME_win32 := x264plugin_helper.exe
BASENAME := h264_video_pwplugin_helper
SRCDIR   := $(CURDIR)
SOURCES  := h264_helper.cxx

PLUGIN_CPPFLAGS = $(X264_CFLAGS) -I.. -DGPL_HELPER_APP -DPLUGINCODEC_TRACING
PLUGIN_LIBS     = $(X264_LIBS)


INSTALL_DIR = $(VID_PLUGIN_DIR)
include $(dir $(lastword $(MAKEFILE_LIST)))../../../plugin_inc.mak


###########################################
