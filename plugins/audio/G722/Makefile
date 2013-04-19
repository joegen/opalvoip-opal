#
# Makefile for G.722 codec plugin
#
# Copyright (C) 2008 Vox Lucida Pty. Ltd., All Rights Reserved
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
# The Original Code is Open Phone Abstraction Library.
#
# The Initial Developer of the Original Code is Robert Jongbloed
#
# Contributor(s): ______________________________________.
#
# $Revision$
# $Author$
# $Date$
#

BASENAME := g722

SRCDIR   := $(CURDIR)/VoIPCodecs
SOURCES  := $(CURDIR)/g722codec.c \
            $(SRCDIR)/g722_encode.c \
            $(SRCDIR)/g722_decode.c \
            $(SRCDIR)/bitstream.c 

INSTALL_DIR = $(AUD_PLUGIN_DIR)
include $(dir $(lastword $(MAKEFILE_LIST)))../../plugin_inc.mak


###########################################
