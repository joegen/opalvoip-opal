#
# Makefile for GSM 06.10
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

PREFIX=/usr
exec_prefix = ${PREFIX}
LIBDIR=${exec_prefix}/lib

OBJDIR	= ./obj

BASENAME=theora
SONAME	=$(BASENAME)

CC	 	=gcc
CFLAGS	 	=-g -O2
CXX	 	=g++
LDSO	 	=-shared -Wl,-soname,$(SONAME)
PLUGINEXT	=so
STDCCFLAGS	=-fPIC
LDFLAGS		=
THEORA_LIBS     =

CFLAGS        +=

vpath	%.o   $(OBJDIR)

SRCS    += theora_frame.cxx \
           trace.cxx \
           theora_plugin.cxx


$(OBJDIR)/%.o : %.cxx
	@mkdir -p $(OBJDIR) >/dev/null 2>&1
	$(CXX) -I../../../include $(EXTRACCFLAGS) $(STDCCFLAGS) $(OPTCCFLAGS) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o : %.c
	@mkdir -p $(OBJDIR) >/dev/null 2>&1
	$(CC) -I../../../include $(EXTRACCFLAGS) $(STDCCFLAGS) $(OPTCCFLAGS) $(CFLAGS) -c $< -o $@

PLUGIN	= ./$(BASENAME)_video_pwplugin.$(PLUGINEXT)
STATIC	= ./lib$(BASENAME)_video_s.a

OBJECTS = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(patsubst %.cxx,%.o,$(notdir $(SRCS)))))

$(PLUGIN): $(OBJECTS)
	$(CXX) $(THEORA_LIBS) $(LDSO) -o $@ $^


install:
	mkdir -p $(DESTDIR)$(LIBDIR)/pwlib/codecs/video/
	cp $(PLUGIN) $(DESTDIR)$(LIBDIR)/pwlib/codecs/video/

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/pwlib/codecs/video/$(PLUGIN)

clean:
	rm -f $(OBJECTS) $(PLUGIN)

###########################################
