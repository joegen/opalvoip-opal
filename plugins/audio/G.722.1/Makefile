#
# Makefile for G.722.1 codec plugin
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

BASENAME := g7221

SOURCES  := $(CURDIR)/G7221Codec.c

LIBG7221_DIR := $(CURDIR)/libg722_1
LIBG7221_FILE := $(LIBG7221_DIR)/src/.libs/libg722_1.a

PLUGIN_CPPFLAGS = -I$(LIBG7221_DIR)/src
PLIGIN_LIBS     = -I$(dir $(LIBG7221_FILE)) -lg722_1

INSTALL_DIR = $(AUD_PLUGIN_DIR)
include $(dir $(lastword $(MAKEFILE_LIST)))../../plugin_inc.mak

$(PLUGIN_PATH): $(LIBG7221_FILE)

$(LIBG7221_FILE):
	touch $(LIBG7221_DIR)/Makefile.in
	touch $(LIBG7221_DIR)/Makefile
	touch $(LIBG7221_DIR)/config-h.in
	$(MAKE) -C $(LIBG7221_DIR)


###########################################
