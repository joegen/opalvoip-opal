#
# Makefile for the Cisco H.264 Video Codec
#
# Copyright (C) 2014 Vox Lucida Pty. Ltd.
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

BASENAME := openh264

COMMONDIR := $(abspath ../common)
SRCDIR    := $(CURDIR)
SOURCES   := $(CURDIR)/openh264.cxx \
             $(COMMONDIR)/encframe.cxx \
             $(COMMONDIR)/h264frame.cxx \

PLUGIN_LIBS = -lopenh264

vpath   %.cxx $(COMMONDIR)

INSTALL_DIR = $(VID_PLUGIN_DIR)
include $(dir $(lastword $(MAKEFILE_LIST)))../../plugin_inc.mak


###########################################
