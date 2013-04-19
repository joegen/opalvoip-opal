#
# Makefile for GSM 06.10
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

BASENAME := gsm0610

SRCDIR   := $(CURDIR)/src
SOURCES  := $(CURDIR)/gsm06_10_codec.c

GSM_SYSTEM=@GSM_SYSTEM@
ifeq ($(GSM_SYSTEM),no)
  SOURCES := $(SRCDIR)/gsm_create.c \
             $(SRCDIR)/gsm_destroy.c \
             $(SRCDIR)/gsm_decode.c \
             $(SRCDIR)/gsm_encode.c \
             $(SRCDIR)/gsm_option.c \
             $(SRCDIR)/code.c \
             $(SRCDIR)/decode.c \
             $(SRCDIR)/add.c \
             $(SRCDIR)/gsm_lpc.c \
             $(SRCDIR)/rpe.c \
             $(SRCDIR)/preprocess.c \
             $(SRCDIR)/long_term.c \
             $(SRCDIR)/short_term.c \
             $(SRCDIR)/table.c
  PLUGIN_CPPFLAGS = -I$(CURDIR)/inc -DWAV49 -DNeedFunctionPrototypes=1
else
  PLUGIN_CPPFLAGS = $(GSM_CFLAGS)
  PLUGIN_LIBS     = $(GSM_LIBS)
endif

INSTALL_DIR = $(AUD_PLUGIN_DIR)
include $(dir $(lastword $(MAKEFILE_LIST)))../../plugin_inc.mak


###########################################
