#
# Makefile
#
# Make file for an OPAL analyser/player for RTP.
#
# Copyright (c) 2015 Vox Lucida
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
# The Original Code is Opal Shark.
#
# The Initial Developer of the Original Code is Robert Jongbloed
#
# Contributor(s): ______________________________________.
#
# $Revision$
# $Author$
# $Date$
#

PROG		:= opalshark
SOURCES		:= main.cxx opalshark.cxx

VERSION_FILE    := version.h

# Need this before the include of opal_inc.mak
.DEFAULT_GOAL := bundle

WX_CONFIG := wx-config
WXRC := wxrc
ifeq (,$(shell which $(WX_CONFIG) 2>&1))
  WX_CONFIG := wxgtk2u-2.8-config
  WXRC := wxrc-gtk2u-2.8
  ifeq (,$(shell which $(WX_CONFIG) 2>&1))
    $(error Could not find wx-config)
  endif
endif

ifneq (,$(DEBUG_BUILD))
  ifneq (,$(shell wx-config --list | grep debug))
    WX_CONFIG += --debug=$(DEBUG_BUILD)
  else
    CPPFLAGS += -DOPAL_WX_DEBUG_HACK
  endif
endif

CPPFLAGS += $(shell $(WX_CONFIG) --cppflags)
CXXFLAGS += $(shell $(WX_CONFIG) --cxxflags)
CFLAGS   += $(shell $(WX_CONFIG) --cflags)
LDFLAGS  += $(shell $(WX_CONFIG) --libs)

ifdef OPALDIR
  include $(OPALDIR)/make/opal.mak
else
  include $(shell pkg-config opal --variable=makedir)/opal.mak
endif


opalshark.cxx : opalshark.xrc
	$(WXRC) opalshark.xrc -c -o opalshark.cxx


