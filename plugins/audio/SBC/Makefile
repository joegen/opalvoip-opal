#
# Common included Makefile for plug ins
#
# Copyright (C) 2011 Vox Lucida
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
# The Original Code is OPAL.
#
# The Initial Developer of the Original Code is Robert Jongbloed
#
# Contributor(s): ______________________________________.
#
# $Revision$
# $Author$
# $Date$
#

BASENAME := sbc

SRCDIR   := $(CURDIR)
SOURCES  := $(CURDIR)/sbccodec.cxx \
            $(CURDIR)/sbc.c \
            $(CURDIR)/sbc_primitives.c \
            $(CURDIR)/sbc_primitives_mmx.c \
            $(CURDIR)/sbc_primitives_neon.c

INSTALL_DIR = $(AUD_PLUGIN_DIR)
include $(dir $(lastword $(MAKEFILE_LIST)))../../plugin_inc.mak


###########################################
