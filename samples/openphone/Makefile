#
# Makefile
#
# Make file for simple sample application for the Open Phone Abstraction library.
#
# Copyright (c) 2006 Derek J Smithies
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
# The Initial Developer of the Original Code is Equivalence Pty. Ltd.
#
# Contributor(s): ______________________________________.
#
# $Revision$
# $Author$
# $Date$
#

PROG		:= openphone
SOURCES		:= main.cxx openphone.cxx

VERSION_FILE    := version.h

WX_CONFIG := $(firstword $(wildcard $(addsuffix /wx-config,$(subst :, ,$(PATH):/opt/local/Library/Frameworks/wxWidgets.framework/Versions/wxWidgets/3.0/bin))))
ifneq (,$(WX_CONFIG))
  WXRC := $(dir $(WX_CONFIG))/wxrc
else
  WX_CONFIG := $(firstword $(wildcard $(addsuffix /wxgtk2u-2.8-config,$(subst :, ,$(PATH)))))
  ifeq (,$(shell which $(WX_CONFIG) 2>&1))
    $(error Could not find wx-config)
  endif
  WXRC := $(dir $(WX_CONFIG))/wxrc-gtk2u-2.8
endif

ifneq (,$(DEBUG_BUILD))
  ifneq (,$(shell $(WX_CONFIG) --list | grep debug))
    WX_CONFIG += --debug=$(DEBUG_BUILD)
  else
    CPPFLAGS += -DOPAL_WX_DEBUG_HACK
  endif
endif

CPPFLAGS += $(shell $(WX_CONFIG) --cppflags)
CXXFLAGS += $(shell $(WX_CONFIG) --cxxflags)
CFLAGS   += $(shell $(WX_CONFIG) --cflags)
LDFLAGS  += $(shell $(WX_CONFIG) --libs)


# Need this before the include of opal.mak
.DEFAULT_GOAL := bundle


ifdef OPALDIR
  include $(OPALDIR)/make/opal.mak
else
  include $(shell pkg-config opal --variable=makedir)/opal.mak
endif


openphone.cxx : openphone.xrc
	$(WXRC) openphone.xrc -c -o openphone.cxx


ifeq ($(DEBUG_BUILD),yes)

  bundle: $(TARGET)
	@echo Not creating bundle with debug version.

else # DEBUG_BUILD

  BUNDLE_DIR := ./$(PROG).app
  PROG_AND_LIBS :=\
      $(TARGET) \
      $(PTLIB_SHARED_LIB_FILE) \
      $(PTLIB_SHARED_LIB_LINK) \
      `find $(OPAL_LIBDIR) -name \*$(PTLIB_PLUGIN_SUFFIX).$(SHAREDLIBEXT)` \
      $(OPAL_SHARED_LIB_FILE) \
      $(OPAL_SHARED_LIB_LINK)

  ifneq ($(OPAL_LIBDIR),$(PTLIB_LIBDIR))
    PROG_AND_LIBS += `find $(PTLIB_LIBDIR) -name \*$(PTLIB_PLUGIN_SUFFIX).$(SHAREDLIBEXT)`
  endif

  bundle: $(BUNDLE_DIR)

  ifneq ($(target_os),Darwin)

    $(BUNDLE_DIR): $(TARGET)
	@-rm -f $@/*
	@-mkdir -p $@
	cp -pR $(PROG_AND_LIBS) $@

  else # Darwin

    STRINGSFILE= #InfoPlist.strings

    $(BUNDLE_DIR): $(TARGET) Info.plist version.plist $(PROG).icns $(STRINGSFILE)
	@for dir in $@ \
	           $@/Contents \
	           $@/Contents/MacOS \
	           $@/Contents/Resources \
	           $@/Contents/Resources/English.lproj; do \
	  if test ! -d $$dir; then \
	    echo Creating $$dir; \
	    mkdir $$dir; \
	  fi \
	done
	cp Info.plist version.plist $@/Contents/
	cp $(PROG).icns $@/Contents/Resources/
    ifneq ($(STRINGSFILE),)	
	cp $(STRINGSFILE) $@/Contents/Resources/English.lproj/
    endif
	echo -n 'APPL????'    > $@/Contents/PkgInfo
	cp -pR $(PROG_AND_LIBS) $@/Contents/MacOS

  endif # Darwin
endif # DEBUG_BUILD
