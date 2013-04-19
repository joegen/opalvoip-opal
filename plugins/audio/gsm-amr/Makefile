#
# Makefile for GSM-AMR Codec Plugin
#
# Copyright (C) 2004 MX Telecom Ltd.
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

BASENAME := gsmamrcodec

SRCDIR   := $(CURDIR)/src
SOURCES  := $(CURDIR)/amrcodec.c \
            $(SRCDIR)/interf_enc.c \
            $(SRCDIR)/interf_dec.c \
            $(SRCDIR)/sp_dec.c \
            $(SRCDIR)/sp_enc.c

INSTALL_DIR := $(AUD_PLUGIN_DIR)
include $(dir $(lastword $(MAKEFILE_LIST)))../../plugin_inc.mak


###########################################
