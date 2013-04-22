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

# autoconf.mak uses this for if we are run as "make -f ../Makefile"
TOP_LEVEL_DIR := $(abspath $(dir $(firstword $(MAKEFILE_LIST))))

CONFIG_FILES := include/opal_config.h \
                make/opal_config.mak \
                opal.pc \
                opal_cfg.dxy \
                plugins/plugin_config.mak \
                plugins/plugin_config.h

PLUGIN_CONFIGURE  := $(TOP_LEVEL_DIR)/plugins/configure
PLUGIN_ACLOCAL_M4 := $(TOP_LEVEL_DIR)/plugins/aclocal.m4


ifdef PTLIBDIR
  include $(PTLIBDIR)/make/autoconf.mak
else
  include $(shell pkg-config ptlib --variable=makedir)/autoconf.mak
endif


ifeq ($(AUTOCONF_AVAILABLE),yes)

  $(CONFIG_STATUS) : $(PLUGIN_CONFIGURE)

  $(PLUGIN_CONFIGURE): $(PLUGIN_CONFIGURE).ac $(M4_FILES) $(PLUGIN_ACLOCAL)
	cd $(dir $@) && $(AUTOCONF)

  $(PLUGIN_ACLOCAL_M4):
	cd $(dir $@) && $(ACLOCAL)

endif


# End of Makefile
