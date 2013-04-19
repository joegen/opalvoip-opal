#
# Makefile for the Web-M VP8 Video Codec
#
# Copyright (C) 2012 Vox Lucida Pty. Ltd., All Rights Reserved
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
# The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
#
# Contributor(s): ______________________________________.
#
# $Revision$
# $Author$
# $Date$
#

BASENAME := vp8_webm

SRCDIR   := $(CURDIR)
SOURCES  := $(CURDIR)/vp8_webm.cxx

PLUGIN_CPPFLAGS = $(VP8_CFLAGS) -DLIB_DIR='"$(libdir)"' -DVID_PLUGIN_DIR='"$(VID_PLUGIN_DIR)"'
PLUGIN_LIBS     = $(VP8_LIBS)

INSTALL_DIR = $(VID_PLUGIN_DIR)
include $(dir $(lastword $(MAKEFILE_LIST)))../../plugin_inc.mak


###########################################
