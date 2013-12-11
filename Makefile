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

ENV_OPALDIR := $(OPALDIR)
ifndef OPALDIR
  export OPALDIR:=$(CURDIR)
  $(info Setting default OPALDIR to $(OPALDIR))
endif

TOP_LEVEL_MAKE := $(OPALDIR)/make/toplevel.mak
CONFIGURE      := $(OPALDIR)/configure
CONFIG_FILES   := $(OPALDIR)/opal.pc \
                  $(OPALDIR)/opal_cfg.dxy \
                  $(TOP_LEVEL_MAKE) \
                  $(OPALDIR)/make/opal_defs.mak \
                  $(OPALDIR)/include/opal/buildopts.h \
		  $(OPALDIR)/plugins/plugin-inc.mak

PLUGIN_CONFIG  := $(OPALDIR)/plugins/configure
PLUGIN_ACLOCAL := $(OPALDIR)/plugins/aclocal.m4

AUTOCONF       := autoconf
ACLOCAL        := aclocal

CONFIG_IN_FILES := $(addsuffix .in, $(CONFIG_FILES))
ALLBUTFIRST = $(filter-out $(firstword $(CONFIG_FILES)), $(CONFIG_FILES))
ALLBUTLAST = $(filter-out $(lastword $(CONFIG_FILES)), $(CONFIG_FILES))
PAIRS = $(join $(ALLBUTLAST),$(addprefix :,$(ALLBUTFIRST)))

ifndef CFG_ARGS
  ifneq (,$(shell which ./config.status))
    CFG_ARGS=$(shell ./config.status --config)
  endif
endif

#
# The following goals do not generate a call to configure
NO_CONFIG_GOALS += clean distclean config

ifneq (,$(MAKECMDGOALS))
  CONFIG_GOALS:=$(filter-out $(NO_CONFIG_GOALS),$(MAKECMDGOALS))
  ifneq (,$(CONFIG_GOALS))
    $(CONFIG_GOALS): default
  endif
endif

default: $(CONFIG_FILES)
	@$(MAKE) -f $(TOP_LEVEL_MAKE) $(filter-out config,$(MAKECMDGOALS))

.PHONY:config
config:	$(CONFIG_FILES)
	$(CONFIGURE)

.PHONY:clean
clean:
	if test -e $(TOP_LEVEL_MAKE) ; then \
	  $(MAKE) -f $(TOP_LEVEL_MAKE) clean ; \
	else \
	  rm -f $(CONFIG_FILES) ; \
	fi

.PHONY:default_clean
default_clean: clean
	if test -e $(TOP_LEVEL_MAKE) ; then \
	  $(MAKE) -f $(TOP_LEVEL_MAKE) clean ; \
	fi

.PHONY:distclean
distclean: clean
	if test -e $(TOP_LEVEL_MAKE) ; then \
	  $(MAKE) -f $(TOP_LEVEL_MAKE) clean ; \
	fi

.PHONY:sterile
sterile: distclean
	if test -e $(TOP_LEVEL_MAKE) ; then \
	  $(MAKE) -f $(TOP_LEVEL_MAKE) sterile ; \
	else \
	  rm -f $(CONFIGURE) config.log config.status ; \
	fi

$(lastword $(CONFIG_FILES)) : $(CONFIGURE) $(CONFIG_IN_FILES)
	OPALDIR=$(ENV_OPALDIR) $(CONFIGURE) $(CFG_ARGS)
	touch $(CONFIG_FILES)

ifeq ($(shell which $(AUTOCONF) > /dev/null && \
              which $(ACLOCAL) > /dev/null && \
              test `autoconf --version | sed -n "s/autoconf.*2.\\([0-9]*\\)/\\1/p"` -ge 68 \
              ; echo $$?),0)

$(CONFIGURE): $(CONFIGURE).ac $(OPALDIR)/make/*.m4 $(ACLOCAL).m4
	$(AUTOCONF)

$(ACLOCAL).m4:
	$(ACLOCAL)

$(PLUGIN_CONFIG): $(PLUGIN_CONFIG).ac $(PLUGIN_ACLOCAL) $(OPALDIR)/make/*.m4 
	( cd $(dir $@) ; $(AUTOCONF) )

$(PLUGIN_ACLOCAL):
	( cd $(dir $@) ; $(ACLOCAL) )

else # autoconf

$(CONFIGURE): $(CONFIGURE).ac
	@echo ---------------------------------------------------------------------
	@echo The configure script requires updating but autoconf not is installed.
	@echo Either install autoconf or execute the command:
	@echo touch $@
	@echo ---------------------------------------------------------------------

endif # autoconf good

$(foreach pair,$(PAIRS),$(eval $(pair)))

# End of Makefile.in