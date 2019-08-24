#
# Makefile
#
# Make file for OPAL Server
#
# Copyright (c) 2003 Equivalence Pty. Ltd.
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
# Contributor(s): Vox Lucida Pty. Ltd.
#

PROG	= opalsrv
SOURCES	:= main.cxx custom.cxx h323serv.cxx sipserv.cxx status.cxx cdr.cxx

OPAL_MAKE_DIR := $(if $(OPALDIR),$(OPALDIR)/make,$(shell pkg-config opal --variable=makedir))
ifeq ($(OPAL_MAKE_DIR),)
  $(error Cannot build without OPAL installed or OPALDIR set)
endif
include $(OPAL_MAKE_DIR)/opal.mak

ifndef DESTDIR
  DESTDIR=/opt/opalsrv
endif

BINDIR=$(DESTDIR)/bin
ETCDIR=$(DESTDIR)/etc
INIFILE=$(ETCDIR)/opalsrv.ini

install: $(TARGET) | $(BINDIR) $(ETCDIR)
	install $(TARGET) $(BINDIR)
	install start.sh $(BINDIR)
	install webrtc_test.html $(BINDIR)
	install main_menu.vxml $(ETCDIR)
	if [ ! -e "$(INIFILE)" ]; then install default.ini "$(INIFILE)"; fi

$(BINDIR) $(ETCDIR):
	$(MKDIR_P) $@


# End of Makefile
