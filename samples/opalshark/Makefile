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

PROG		:= opalshark
SOURCES		:= main.cxx opalshark.cxx

VERSION_FILE    := version.h

WX_CONFIG := $(firstword $(wildcard $(addsuffix /wx-config,$(subst :, ,$(PATH):/opt/local/Library/Frameworks/wxWidgets.framework/Versions/wxWidgets/3.0/bin))))
ifneq (,$(WX_CONFIG))
  WXRC := $(dir $(WX_CONFIG))/wxrc
else
  WX_CONFIG := $(firstword $(wildcard $(addsuffix /wxgtk2u-2.8-config,$(subst :, ,$(PATH)))))
  ifneq (,$(WX_CONFIG))
    WXRC := $(dir $(WX_CONFIG))/wxrc-gtk2u-2.8
  else
    WX_CONFIG := wx-config-3.0
    ifneq (,$(shell which $(WX_CONFIG) 2>&1))
      WXRC := wxrc-3.0
    else
      WX_CONFIG := wx-config
      ifneq (,$(shell which $(WX_CONFIG) 2>&1))
        WXRC := wxrc
      else
        $(error Could not find wx-config)
      endif
    endif
  endif
endif

ifneq (,$(DEBUG_BUILD))
  ifneq (,$(shell $(WX_CONFIG) --list | grep debug))
    WX_CONFIG += --debug=$(DEBUG_BUILD)
  else
    CPPFLAGS += -DOPAL_WX_DEBUG_HACK
  endif
endif


# Need this before the include of opal.mak
.DEFAULT_GOAL := bundle


ifdef OPALDIR
  include $(OPALDIR)/make/opal.mak
else
  include $(shell pkg-config opal --variable=makedir)/opal.mak
endif


CPPFLAGS += $(shell $(WX_CONFIG) --cppflags)
CXXFLAGS += $(shell $(WX_CONFIG) --cxxflags)
CFLAGS   += $(shell $(WX_CONFIG) --cflags)
LIBS     += $(shell $(WX_CONFIG) --libs)


opalshark.cxx : opalshark.xrc
	$(WXRC) opalshark.xrc -c -o opalshark.cxx


