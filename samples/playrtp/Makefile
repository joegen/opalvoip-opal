#
# Makefile
#
# Makefile for PlayRTP
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
# Revision 1.1  2007/09/28 02:33:26  rjongbloed
# Added application to play back RTP out of a Wireshark capture file.
#


PROG = playrtp
SOURCES := main.cxx
OBJDIR          = obj

EXTRACXXFLAGS=-I$(OPAL_INCDIR)
EXTRALDFLAGS =-L$(OPAL_LIBDIR) $(LIBS) -lopal 

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
	$(CXX) $(LDSO) $(EXTRALDFLAGS) -o $@ $^

clean:
	rm -f $(OBJECTS) $(PROG)