#
# Common included Makefile for plug ins
#
# Copyright (C) 2011 Vox Lucida
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
# The Original Code is OPAL.
#
# The Initial Developer of the Original Code is Robert Jongbloed
#
# Contributor(s): ______________________________________.
#
# $Revision$
# $Author$
# $Date$
#

PLUGIN_CONFIG_MAK := plugin_config.mak
ifneq (,$(OPAL_PLATFORM_DIR))
  include $(OPAL_PLATFORM_DIR)/plugins/$(PLUGIN_CONFIG_MAK)
else ifdef OPALDIR
  include $(OPALDIR)/plugins/$(PLUGIN_CONFIG_MAK)
else
  include $(shell pkg-config opal --variable=makedir)/$(PLUGIN_CONFIG_MAK)
endif


##############################################################################

ifneq (,$(BASENAME_$(target_os)))
  BASENAME = $(BASENAME_$(target_os))
endif

ifneq (,$(BASENAME))
  OBJDIR      = $(abspath $(PLUGIN_SRC_DIR)/../lib_$(target)/plugins/$(BASENAME))
  SONAME      = $(BASENAME)_ptplugin
  PLUGIN_NAME = $(SONAME).$(SHAREDLIBEXT)
  PLUGIN_PATH = $(OBJDIR)/$(PLUGIN_NAME)
endif

ifeq ($(DEBUG_BUILD),yes)
  CFLAGS += -g
  CXXFLAGS += -g
endif

ifeq ($(PLUGIN_SYSTEM),no)
  SOURCES  += $(LOCAL_SOURCES)
  CPPFLAGS += $(LOCAL_CPPFLAGS)
  LDFLAGS  := $(LOCAL_LIBS) $(LDFLAGS)
else
  CPPFLAGS += $(PLUGIN_CPPFLAGS)
  LDFLAGS  := $(PLUGIN_LIBS) $(LDFLAGS)
endif


##############################################################################

ifeq ($V$(VERBOSE),)
  Q   := @
  Q_CC = @echo [CC$(Q_SUFFIX)] $(subst $(CURDIR)/,,$<) ; 
  Q_LD = @echo [LD$(Q_SUFFIX)] $(subst $(CURDIR)/,,$@) ; 
endif


vpath	%.o $(OBJDIR)
vpath	%.c $(SRCDIR)
vpath	%.cxx $(SRCDIR)

$(OBJDIR)/%.o : %.c
	@if [ ! -d $(dir $@) ] ; then $(MKDIR_P) $(dir $@) ; fi
	$(Q_CC)$(CC) $(strip $(CPPFLAGS) $(CFLAGS)) -o $@ -c $<

$(OBJDIR)/%.o : %.cxx
	@if [ ! -d $(dir $@) ] ; then $(MKDIR_P) $(dir $@) ; fi
	$(Q_CC)$(CXX) $(strip $(CPPFLAGS) $(CXXFLAGS)) -o $@ -c $<

$(OBJDIR)/%.o : %.cpp
	@if [ ! -d $(dir $@) ] ; then $(MKDIR_P) $(dir $@) ; fi
	$(Q_CC)$(CXX) $(strip $(CPPFLAGS) $(CXXFLAGS)) -o $@ -c $<


##############################################################################

ifneq (,$(PLUGIN_PATH))

  all :: $(PLUGIN_PATH)

  $(PLUGIN_PATH): $(addprefix $(OBJDIR)/,$(patsubst %.cxx,%.o,$(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(notdir $(SOURCES))))))
	@if [ ! -d $(dir $@) ] ; then $(MKDIR_P) $(dir $@) ; fi
	$(Q_LD)$(CXX) -o $@ $* $(SHARED_LDFLAGS:INSERT_SONAME=$(SONAME)) $(strip $(LDFLAGS))

  install ::
	mkdir -p $(DESTDIR)$(libdir)/$(INSTALL_DIR)
	$(INSTALL) $(PLUGIN_PATH) $(DESTDIR)$(libdir)/$(INSTALL_DIR)

  uninstall ::
	rm -f $(DESTDIR)$(libdir)/$(INSTALL_DIR)/$(PLUGIN_NAME)

endif

clean distclean ::
	rm -rf $(OBJDIR)

both opt debug optshared debugshared optstatic debugstatic: all

optdepend debugdepend bothdepend optlibs debuglibs bothlibs:
	@true


##############################################################################

ifneq (,$(SUBDIRS))
  export CC CXX LD AR RANLIB CPPFLAGS CFLAGS CXXFLAGS LDFLAGS ARFLAGS target target_os target_cpu
  all install uninstall clean ::
	$(Q)set -e; $(foreach dir,$(SUBDIRS), \
          if test -d $(dir) ; then \
            $(MAKE) --print-directory -C $(dir) $@; \
          else \
            echo Directory $(dir) does not exist; \
          fi; \
        )
endif


# End of file
