#
# Makefile for ffmpeg H.263 (1998 aka H.263+)
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
# Contributor(s): Matthias Schneider.
#
# $Revision$
# $Author$
# $Date$
#

BASENAME := h263_ffmpeg

COMMONDIR := $(abspath ../common)
SRCDIR    := $(CURDIR)
SOURCES   := $(CURDIR)/h263-1998.cxx \
             $(CURDIR)/rfc2429.cxx \
             $(CURDIR)/rfc2190.cxx \
             $(COMMONDIR)/ffmpeg.cxx \
             $(COMMONDIR)/dyna.cxx

PLUGIN_CPPFLAGS = $(LIBAVCODEC_CFLAGS) -I$(COMMONDIR) -DLIBAVCODEC_RTP_MODE=$(subst yes,1,$(HAVE_LIBAVCODEC_RTP_MODE))
PLUGIN_LIBS     = $(DLFCN_LIBS)

vpath	%.cxx $(COMMONDIR)

INSTALL_DIR = $(VID_PLUGIN_DIR)
include $(dir $(lastword $(MAKEFILE_LIST)))../../plugin_inc.mak


###########################################
