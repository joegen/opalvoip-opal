#
# opal_inc.mak
#
# Make symbols include file for Open Phone Abstraction library
#
# Copyright (c) 2001 Equivalence Pty. Ltd.
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
# The Original Code is Open Phone Abstraction library.
#
# The Initial Developer of the Original Code is Equivalence Pty. Ltd.
#
# Contributor(s): ______________________________________.
#
# $Log: opal_inc.mak,v $
# Revision 1.2005  2002/09/11 05:55:40  robertj
# Fixed double inclusion of common.mak
# Added more directories to search to find pwlib
#
# Revision 2.3  2002/04/19 01:24:30  robertj
# Changed /usr/include to SYSINCDIR helps with X-compiling, thanks Bob Lindell
#
# Revision 2.2  2002/03/15 10:51:53  robertj
# Fixed problem with recursive inclusion on make files.
#
# Revision 2.1  2002/02/06 09:39:37  rogerh
# Look for telephony.h in the place where the FreeBSD port puts it
#
# Revision 2.0  2001/07/27 15:48:24  robertj
# Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
#


ifndef PWLIBDIR
ifneq (,$(wildcard ../pwlib))
PWLIBDIR=$(CURDIR)/../pwlib
else
ifneq (,$(wildcard $(HOME)/pwlib))
PWLIBDIR=$(HOME)/pwlib
else
PWLIBDIR=/usr/local/pwlib
endif
endif
endif


LIBDIRS += $(OPALDIR)


ifdef LIBRARY_MAKEFILE
include $(PWLIBDIR)/make/unix.mak
else
ifdef NOTRACE
OBJDIR_SUFFIX := n
endif
include $(PWLIBDIR)/make/ptlib.mak
endif



OPAL_SRCDIR = $(OPALDIR)/src
OPAL_INCDIR = $(OPALDIR)/include
OPAL_LIBDIR = $(OPALDIR)/lib


ifdef NOTRACE
STDCCFLAGS += -DPASN_NOPRINTON -DPASN_LEANANDMEAN
OPAL_SUFFIX = n
else
STDCCFLAGS += -DPTRACING
RCFLAGS	   += -DPTRACING
OPAL_SUFFIX = $(OBJ_SUFFIX)
endif


OPAL_BASE  = opal_$(PLATFORM_TYPE)_$(OPAL_SUFFIX)
OPAL_FILE  = lib$(OPAL_BASE)$(LIB_TYPE).$(LIB_SUFFIX)

LDFLAGS	    += -L$(OPAL_LIBDIR)
LDLIBS	    := -l$(OPAL_BASE)$(LIB_TYPE) $(LDLIBS)

STDCCFLAGS  += -I$(OPAL_INCDIR)

ifneq (,$(wildcard $(SYSINCDIR)/linux/telephony.h))
HAS_IXJ	    = 1
STDCCFLAGS += -DHAS_IXJ
endif

ifneq (,$(wildcard $(SYSINCDIR)/sys/telephony.h))
HAS_IXJ	    = 1
STDCCFLAGS += -DHAS_IXJ
endif

ifneq (,$(wildcard /usr/local/include/sys/telephony.h))
HAS_IXJ	    = 1
STDCCFLAGS += -DHAS_IXJ -I/usr/local/include
endif

ifneq (,$(wildcard $(SYSINCDIR)/linux/soundcard.h))
HAS_OSS	    = 1
STDCCFLAGS += -DHAS_OSS
endif

ifeq ($(OSTYPE),linux)

#ifneq (,$(wildcard /usr/local/lib/libvpb.a))
#HAS_VPB	   = 1
#STDCCFLAGS += -DHAS_VPB
#LDLIBS	   += /usr/local/lib/libvpb.a
#endif

# linux
endif

$(TARGET) :	$(OPAL_LIBDIR)/$(OPAL_FILE)

ifndef LIBRARY_MAKEFILE

ifdef DEBUG
$(OPAL_LIBDIR)/$(OPAL_FILE):
	$(MAKE) -C $(OPALDIR) debug
else
$(OPAL_LIBDIR)/$(OPAL_FILE):
	$(MAKE) -C $(OPALDIR) opt
endif

libs :: $(OPAL_LIBDIR)/$(OPAL_FILE)

endif


# End of file

