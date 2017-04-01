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

ifeq (,$(OPAL_PLATFORM_DIR))
  $(error Plugins can only be built from top level Makefile)
endif

include $(OPAL_PLATFORM_DIR)/plugins/plugin_config.mak


##############################################################################

ifneq (,$(PLUGIN_NAME_$(target_os)))
  PLUGIN_NAME = $(PLUGIN_NAME_$(target_os))
endif

ifneq (,$(BASENAME))
  ifeq (,$(PLUGIN_NAME))
    LIB_SONAME  = $(BASENAME)_ptplugin
    PLUGIN_NAME = $(LIB_SONAME).$(SHAREDLIBEXT)
    CPPFLAGS   += $(SHARED_CPPFLAGS)
  endif
  OBJDIR = $(abspath $(PLUGIN_SRC_DIR)/../lib_$(target)/plugins/$(BASENAME))
  PLUGIN_PATH = $(OBJDIR)/$(PLUGIN_NAME)
endif

CPPFLAGS := -I$(OPAL_PLATFORM_DIR)/plugins $(CPPFLAGS)
CFLAGS   += -fno-common
CXXFLAGS += -fno-common

ifeq ($(DEBUG_BUILD),yes)
  CFLAGS   += -g
  CXXFLAGS += -g
endif

ifeq ($(PLUGIN_SYSTEM),no)
  SOURCES  += $(LOCAL_SOURCES)
  CPPFLAGS += $(LOCAL_CPPFLAGS)
  LIBS     := $(LOCAL_LIBS) $(LIBS)
else
  CPPFLAGS += $(PLUGIN_CPPFLAGS)
  LIBS     := $(PLUGIN_LIBS) $(LIBS)
endif


##############################################################################

ifneq (,$(SUBDIRS))
  export CC CXX LD AR RANLIB CPPFLAGS CFLAGS CXXFLAGS LDFLAGS LIBS ARFLAGS target target_os target_cpu
  all install uninstall clean ::
	$(Q)set -e; $(foreach dir,$(SUBDIRS), \
          if test -d $(dir) ; then \
            $(MAKE) --print-directory -C $(dir) DEBUG_BUILD=$(DEBUG_BUILD) $@; \
          else \
            echo Directory $(dir) does not exist; \
          fi; \
        )
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
  ifeq (,$(LIB_SONAME))
	$(Q_LD)$(CXX) -o $@ $(strip $(LDFLAGS) $^ $(LIBS))
  else
	$(Q_LD)$(CXX) -o $@ $(strip $(SHARED_LDFLAGS) $(LDFLAGS) $^ $(LIBS))
  endif

  install ::
	mkdir -p $(DESTDIR)$(INSTALL_DIR)
	$(INSTALL) $(PLUGIN_PATH) $(DESTDIR)$(INSTALL_DIR)

  uninstall ::
	rm -f $(DESTDIR)$(INSTALL_DIR)/$(PLUGIN_NAME)

endif

clean optclean debugclean distclean ::
ifneq ($(OBJDIR),)
	rm -rf $(OBJDIR)
else
	@true
endif

debug debugshared debugstatic :: DEBUG_BUILD=yes
opt   optshared   optstatic   :: DEBUG_BUILD=no
both opt debug optshared debugshared optstatic debugstatic :: all

optdepend debugdepend bothdepend optlibs debuglibs bothlibs:
	@true

ifeq (,$(SUBDIRS)$(PLUGIN_PATH))
all :
	@echo No plugins to build
endif


# End of file
