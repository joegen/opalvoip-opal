#
# Makefile for G.722 codec plugin
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

BASENAME := g7222

SRCDIR   := $(CURDIR)/AMR-WB
SOURCES  := $(CURDIR)/G7222Codec.cpp \
            $(SRCDIR)/dec_acelp.c \
            $(SRCDIR)/dec_dtx.c \
            $(SRCDIR)/dec_gain.c \
            $(SRCDIR)/dec_if.c \
            $(SRCDIR)/dec_lpc.c \
            $(SRCDIR)/dec_main.c \
            $(SRCDIR)/dec_rom.c \
            $(SRCDIR)/dec_util.c \
            $(SRCDIR)/enc_acelp.c \
            $(SRCDIR)/enc_dtx.c \
            $(SRCDIR)/enc_gain.c \
            $(SRCDIR)/enc_if.c \
            $(SRCDIR)/enc_lpc.c \
            $(SRCDIR)/enc_main.c \
            $(SRCDIR)/enc_rom.c \
            $(SRCDIR)/enc_util.c \
            $(SRCDIR)/if_rom.c

INSTALL_DIR = $(AUD_PLUGIN_DIR)
include $(dir $(lastword $(MAKEFILE_LIST)))../../plugin_inc.mak


###########################################
