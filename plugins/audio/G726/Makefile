#
# Makefile for G.726 codec plugin
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
#

BASENAME := g726

SRCDIR   := $(CURDIR)/g726
SOURCES  := $(CURDIR)/g726codec.c \
            $(CURDIR)/g711.c \
            $(SRCDIR)/g726_16.c \
            $(SRCDIR)/g726_24.c \
            $(SRCDIR)/g726_32.c \
            $(SRCDIR)/g726_40.c \
            $(SRCDIR)/g72x.c 

INSTALL_DIR = $(AUD_PLUGIN_DIR)
include $(dir $(lastword $(MAKEFILE_LIST)))../../plugin_inc.mak


###########################################
