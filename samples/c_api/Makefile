#
# Makefile
#
# An example of the "C" interface to Open Phone Abstraction Library
#
# Copyright (c) 2008 Vox Lucida
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
# The Initial Developer of the Original Code is Vox Lucida (Robert Jongbloed)
#
# Contributor(s): ______________________________________.
#
# $Revision$
# $Author$
# $Date$
#

PROG    := c_api
SOURCES := main.c


# Get required flags from library

CPPFLAGS += $(shell pkg-config $(PKG_FLAGS) --cflags opal)
LDFLAGS  += $(shell pkg-config $(PKG_FLAGS) --libs opal) -ldl


OBJDIR := obj

vpath	%.o   $(OBJDIR)

$(OBJDIR)/%.o : %.c
	@mkdir -p $(OBJDIR) >/dev/null 2>&1
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


all both opt optshared debug debugshared optstatic debugstatic : $(OBJDIR)/$(PROG)

OBJECTS = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(patsubst %.cxx,%.o,$(notdir $(SOURCES)))))

$(OBJDIR)/$(PROG): $(OBJECTS)
	 $(CC) $^ $(LDFLAGS) -o $@

optdepend debugdepend bothdepend optlibs debuglibs bothlibs:

clean:
	rm $(OBJECTS) $(OBJDIR)/$(PROG)

# End of Makefile
