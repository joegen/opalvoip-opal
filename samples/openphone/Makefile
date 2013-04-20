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

# Need this before the include of opal_inc.mak
.DEFAULT_GOAL := bundle

WX_CONFIG := wx-config
ifneq (,$(DEBUG_BUILD))
  WX_CONFIG += --debug=$(DEBUG_BUILD)
endif

CPPFLAGS += $(shell $(WX_CONFIG) --cxxflags)
LDFLAGS  += $(shell $(WX_CONFIG) --libs)

include $(shell PKG_CONFIG_PATH="$(OPALDIR)" pkg-config opal --variable=makedir)/opal.mak


openphone.cxx : openphone.xrc
	wxrc openphone.xrc -c -o openphone.cxx


ifeq ($(DEBUG_BUILD),yes)

  bundle: $(TARGET)
	@echo Not creating bundle with debug version.

else # DEBUG_BUILD

  BUNDLE_DIR := ./$(PROG).app
  PROG_AND_LIBS =\
      $(TARGET) \
      $(PTLIB_LIBDIR)/$(PTLIB_SHARED_FILE) \
      `find $(OPAL_LIBDIR) -name \*$(PTLIB_PLUGIN_SUFFIX).$(SHAREDLIBEXT)` \
      $(OPAL_LIBDIR)/$(OPAL_SHARED_FILE) \
      `find $(PTLIB_LIBDIR) -name \*$(PTLIB_PLUGIN_SUFFIX).$(SHAREDLIBEXT)`

  bundle: $(BUNDLE_DIR)

  ifneq ($(target_os),Darwin)

    $(BUNDLE_DIR): $(TARGET)
	@-rm -f $@/*
	@-mkdir -p $@
	cp $(PROG_AND_LIBS) $@

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
	cp Info.plist        $@/Contents/
	cp version.plist     $@/Contents/
	cp $(PROG).icns      $@/Contents/Resources/
    ifneq ($(STRINGSFILE),)	
	cp $(STRINGSFILE)    $@/Contents/Resources/English.lproj/
    endif
	echo -n 'APPL????' > $@/Contents/PkgInfo
	cp $(PROG_AND_LIBS)  $@/Contents/MacOS

  endif # Darwin
endif # DEBUG_BUILD
