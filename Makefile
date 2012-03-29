#
# Makefile
#
# Make file for OPAL library
#
# Copyright (c) 1993-2012 Equivalence Pty. Ltd.
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
# $Revision: 27217 $
# $Author: rjongbloed $
# $Date: 2012-03-17 21:28:55 +1100 (Sat, 17 Mar 2012) $
#

ifdef OPALDIR
  CFG_ARGS:=--prefix=$(OPALDIR) $(CFG_ARGS)
else
  export OPALDIR=$(CURDIR)
endif

TOP_LEVEL_MAKE := $(OPALDIR)/make/toplevel.mak
CONFIG_FILES   := $(OPALDIR)/make/opal_defs.mak $(TOP_LEVEL_MAKE)
CONFIGURE      := $(OPALDIR)/configure
PLUGIN_CONFIG  := $(OPALDIR)/plugins/configure
PLUGIN_ACLOCAL := $(OPALDIR)/plugins/aclocal.m4

AUTOCONF       := autoconf
ACLOCAL        := aclocal


ifneq (,$(MAKECMDGOALS))
  $(MAKECMDGOALS): default
endif

default: $(CONFIG_FILES)
	@$(MAKE) -f $(TOP_LEVEL_MAKE) $(MAKECMDGOALS)

$(CONFIG_FILES): $(CONFIGURE) $(PLUGIN_CONFIG)
	$(CONFIGURE) $(CFG_ARGS)


ifneq (,$(AUTOCONF))
ifneq (,$(shell which $(AUTOCONF)))

$(CONFIGURE): $(CONFIGURE).ac $(ACLOCAL).m4 $(OPALDIR)/make/*.m4 
	$(AUTOCONF)
 
$(PLUGIN_CONFIG): $(PLUGIN_CONFIG).ac $(PLUGIN_ACLOCAL) $(OPALDIR)/make/*.m4 
	( cd $(dir $@) ; $(AUTOCONF) )
 
$(ACLOCAL).m4:
	$(ACLOCAL)

$(PLUGIN_ACLOCAL):
	( cd $(dir $@) ; $(ACLOCAL) )

$(CONFIG_FILES): $(addsuffix .in, $(CONFIG_FILES))

else # autoconf installed

$(CONFIGURE): $(CONFIGURE).ac
	@echo ---------------------------------------------------------------------
	@echo The configure script requires updating but autoconf not is installed.
	@echo Either install autoconf or execute the command:
	@echo touch $@
	@echo ---------------------------------------------------------------------

endif # autoconf installed
endif # autoconf enabled


# End of Makefile.in
