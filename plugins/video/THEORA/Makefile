#
# Makefile for THEORA
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
# Contributor(s): Matthias Schneider (ma30002000@yahoo.de)
#
# $Revision$
# $Author$
# $Date$
#

BASENAME=theora

COMMONDIR := $(abspath ../common)
SRCDIR    := $(CURDIR)
SOURCES   := $(CURDIR)/theora_frame.cxx \
             $(CURDIR)/theora_plugin.cxx

PLUGIN_CPPFLAGS = $(THEORA_CFLAGS) -I$(COMMONDIR)
PLUGIN_LIBS     = $(THEORA_LIBS)

INSTALL_DIR = $(VID_PLUGIN_DIR)
include $(dir $(lastword $(MAKEFILE_LIST)))../../plugin_inc.mak


###########################################
