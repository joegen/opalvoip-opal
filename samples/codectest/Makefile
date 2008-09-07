#
# Makefile
#
# Makefile for codectest
#
# Copyright (c) 2007 Equivalence Pty. Ltd.
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
# The Original Code is Portable Windows Library.
#
# The Initial Developer of the Original Code is Equivalence Pty. Ltd.
#
# Contributor(s): ______________________________________.
#
# $Log: Makefile,v $
# Revision 1.1  2007/08/20 06:28:15  rjongbloed
# Added new application to test audio and video codecs by taking real media
#   (camera/YUV file/microphone/WAV file etc) encoding it, decoding it and playing
#   back on real output device (screen/speaker etc)
#


PROG = codectest
SOURCES := main.cxx
OBJDIR          = obj

include ../opal_samples.mak
ifeq ($(SAMPLES_DEBUG),yes)
EXTRACXXFLAGS=-I$(OPAL_INCDIR) $(DEBUG_CFLAGS)
else
EXTRACXXFLAGS=-I$(OPAL_INCDIR) $(RELEASE_CFLAGS)
endif

EXTRALDFLAGS =-L$(OPAL_LIBDIR) $(LIBS) 

ifndef OPALDIR
ifneq (,$(wildcard ../../../opal/opal_inc.mak))
OPALDIR=../..
endif
endif

ifndef OPALDIR
default_target :
	@echo Cannot find opal_inc.mak in standard locations, you must set the OPALDIR
	@echo environment variable manually to build this application.
else
include $(OPALDIR)/opal_inc.mak
endif

$(OBJDIR)/%.o : %.cxx
	@mkdir -p $(OBJDIR) >/dev/null 2>&1
	$(CXX) $(EXTRACXXFLAGS) $(CXXFLAGS) -c $< -o $@

OBJECTS = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(patsubst %.cxx,%.o,$(notdir $(SOURCES)))))

$(PROG): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDSO) $(SAMPLES_LIBS) $(EXTRALDFLAGS) 

clean:
	rm -f $(OBJECTS) $(PROG)