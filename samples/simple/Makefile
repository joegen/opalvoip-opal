#
# Makefile
#
# Copyright (C) 2008 Post Increment, All Rights Reserved
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
# The Original Code is OPAL library.
#
# The Initial Developer of the Original Code is Post Increment
#
# Contributor(s): Matthias Schneider (ma30002000@yahoo.de)
#

PROG		= simpleopal
SRCS            = main.cxx

ifndef OPALDIR
default_target :
	@echo OPALDIR is not set
endif

ifndef PTLIBDIR
default_target :
	@echo PTLIBDIR is not set
endif

ifeq ($(DEBUG_BUILD), yes)
OPAL_NAME=opal_debug
PTLIBL_NAME=ptlib_debug
else
OPAL_NAME=opal
PTLIB_NAME=ptlib
endif

OPAL_MACHTYPE = $(shell PKG_CONFIG_LIBDIR=$(OPALDIR):$(PTLIBDIR) pkg-config --variable=OPAL_MACHTYPE $(OPAL_NAME))
OPAL_OSTYPE   = $(shell PKG_CONFIG_LIBDIR=$(OPALDIR):$(PTLIBDIR) pkg-config --variable=OPAL_OSTYPE $(OPAL_NAME))
OPAL_CFLAGS   = $(shell PKG_CONFIG_LIBDIR=$(OPALDIR):$(PTLIBDIR) pkg-config --cflags $(OPAL_NAME) --define-variable=prefix=$(OPALDIR) )
OPAL_LIBS     = $(shell PKG_CONFIG_LIBDIR=$(OPALDIR):$(PTLIBDIR) pkg-config --libs $(OPAL_NAME) --define-variable=prefix=$(OPALDIR) --define-variable=libdir=$(OPALDIR)/lib_$(OPAL_OSTYPE)_$(OPAL_MACHTYPE))

PTLIB_MACHTYPE = $(shell PKG_CONFIG_LIBDIR=$(PTLIBDIR) pkg-config --variable=PTLIB_MACHTYPE $(PTLIB_NAME))
PTLIB_OSTYPE   = $(shell PKG_CONFIG_LIBDIR=$(PTLIBDIR) pkg-config --variable=PTLIB_OSTYPE $(PTLIB_NAME))
PTLIB_CFLAGS   = $(shell PKG_CONFIG_LIBDIR=$(PTLIBDIR) pkg-config --cflags $(PTLIB_NAME) --define-variable=prefix=$(PTLIBDIR) )
PTLIB_LIBS     = $(shell PKG_CONFIG_LIBDIR=$(PTLIBDIR) pkg-config --libs $(PTLIB_NAME) --define-variable=prefix=$(PTLIBDIR) --define-variable=libdir=$(PTLIBDIR)/lib_$(PTLIB_OSTYPE)_$(PTLIB_MACHTYPE))

ifeq ($(DEBUG_BUILD), yes)
OBJDIR = obj_$(OPAL_OSTYPE)_$(OPAL_MACHTYPE)_d
else
OBJDIR = obj_$(OPAL_OSTYPE)_$(OPAL_MACHTYPE)
endif

ifeq ($(STATIC_BUILD), yes)
LDSO += -Wl,-static
endif

vpath	%.cxx $(COMMONDIR)
vpath	%.o   $(OBJDIR)

$(OBJDIR)/%.o : %.cxx
	@mkdir -p $(OBJDIR) >/dev/null 2>&1
	$(CXX) $(CFLAGS) $(OPAL_CFLAGS) -c $< -o $@

OBJECTS = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(patsubst %.cxx,%.o,$(notdir $(SRCS)))))

$(PROG): $(OBJECTS)
	 $(CXX) $(LDFLAGS) $(LDSO) $(OPAL_LIBS) $(PTLIB_LIBS) -o $(OBJDIR)/$@ $^

clean:
	rm -rf obj_$(OPAL_OSTYPE)_$(OPAL_MACHTYPE)*
opt:
	$(MAKE) DEBUG_BUILD=no STATIC_BUILD=no
debug:
	$(MAKE) DEBUG_BUILD=yes STATIC_BUILD=no
	
both:
	$(MAKE) DEBUG_BUILD=no STATIC_BUILD=no
	$(MAKE) DEBUG_BUILD=yes STATIC_BUILD=no

optnoshared:
	$(MAKE) DEBUG_BUILD=no STATIC_BUILD=yes
debugnoshared:
	$(MAKE) DEBUG_BUILD=yes STATIC_BUILD=yes
	
bothnoshared:
	$(MAKE) DEBUG_BUILD=no STATIC_BUILD=yes
	$(MAKE) DEBUG_BUILD=yes STATIC_BUILD=yes
		
###########################################
