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
# $Log: Makefile,v $
# Revision 1.3  2006/07/31 07:39:21  rjongbloed
# Fixed default target to be PWlib one, not just to build opephone.cxx
# Added OPAL directory default (no OPALDIR env var) to be ../.. before
#   searhcing for $(HOME) and /usr/local/opal
#
# Revision 1.2  2006/07/22 00:24:55  dereksmithies
# Add linux magic so it compiles and runs on linux.
# Add the command wxrc to turn the openphone.xrc file into openphone.cxx
# Compile openphone.cxx, which defines the function InitXmlResource();
# Since openphone.cxx is an auto generated file, get cvs to ignore this file.
#
# Revision 1.1  2006/07/19 10:41:51  dereksmithies
# First steps towards having a unix compiled openphone.
#
#

PROG		:= openphone
SOURCES		:= main.cxx openphone.cxx


ifndef OPALDIR
ifneq (,$(wildcard ../../version.h))
OPALDIR=$(wildcard ../..)
else
ifneq (,$(wildcard $(HOME)/opal))
OPALDIR=$(HOME)/opal
else
ifneq (,$(wildcard /usr/local/opal))
OPALDIR=/usr/local/opal
else
default_target :
	@echo Cannot find OPAL in standard locations, you must set the OPALDIR
	@echo environment variable to build this application.
endif
endif
endif
endif

VERSION_FILE	= $(OPALDIR)/version.h

# Need this before the include of opal_inc.mak
default_target: bundle


ifdef OPALDIR
include $(OPALDIR)/opal_inc.mak
endif


WX_CONFIG=wx-config
ifdef DEBUG
WX_CONFIG+=--debug=yes
else
WX_CONFIG+=--debug=no
endif

CPPFLAGS += $(shell $(WX_CONFIG) --cxxflags)
CFLAGS	 += $(shell $(WX_CONFIG) --cxxflags) -Wno-strict-aliasing -Wno-float-equal
ENDLDLIBS+= $(shell $(WX_CONFIG) --libs)


openphone.cxx : openphone.xrc
	wxrc openphone.xrc -c -o openphone.cxx


ifneq ($(target_os),Darwin)

BUNDLE_DIR	= ./openphone

bundle: $(TARGET)
	@mkdir -p $(BUNDLE_DIR)
	@rm -f $(BUNDLE_DIR)/*
	cp $(TARGET) $(BUNDLE_DIR)
ifeq ($(DEBUG_BUILD),yes)
	cp $(OPAL_LIBDIR)/$(DEBUG_LIB_FILENAME_SHARED) $(BUNDLE_DIR)
	cp $(PTLIB_LIBDIR)/$(PTLIB_DEBUG_FILE) $(BUNDLE_DIR)
else
	cp $(OPAL_LIBDIR)/$(RELEASE_LIB_FILENAME_SHARED) $(BUNDLE_DIR)
	cp $(PTLIB_LIBDIR)/$(PTLIB_FILE) $(BUNDLE_DIR)
endif
	find $(OPAL_LIBDIR)/plugins -name \*_ptplugin.$(SHAREDLIBEXT) -exec cp {} $(BUNDLE_DIR) \;
	find $(PTLIB_LIBDIR)/device -name \*_ptplugin.$(SHAREDLIBEXT) -exec cp {} $(BUNDLE_DIR) \;

else
bundle: $(PROG).app

STRINGSFILE= #InfoPlist.strings

$(PROG).app: $(TARGET) Info.plist version.plist $(PROG).icns $(STRINGSFILE)
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
	cp InfoPlist.strings $@/Contents/Resources/English.lproj/
endif
	echo -n 'APPL????' > $@/Contents/PkgInfo
	cp $(TARGET)         $@/Contents/MacOS/$(PROG)

endif
