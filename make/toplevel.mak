#
# toplevel.mak
#
# Make file for Open Phone Abstraction library
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
# The Original Code is Open Phone Abstraction Library.
#
# The Initial Developer of the Original Code is Equivalence Pty. Ltd.
#
# Contributor(s): ______________________________________.
#
# $Revision$
# $Author$
# $Date$
#

ifndef OPALDIR
  $(error OPALDIR must be defined)
endif

include $(OPALDIR)/make/opal_defs.mak

CPPFLAGS += $(SHARED_CPPFLAGS)

SUBDIRS := 

ifeq ($(OPAL_PLUGINS),yes)
SUBDIRS += plugins 
endif

ifeq ($(OPAL_SAMPLES),yes)

  SUBDIRS += samples/c_api \
             samples/server

  ifeq ($(OPAL_HAS_PCSS),yes)
    SUBDIRS += samples/simple
  endif
  ifeq ($(OPAL_VIDEO),yes)
    SUBDIRS += samples/codectest
  endif
  ifeq ($(OPAL_HAS_PCSS)$(OPAL_IVR)$(OPAL_PTLIB_CLI),yesyesyes)
    SUBDIRS += samples/callgen
  endif
  ifeq ($(OPAL_PTLIB_CLI)$(OPAL_PTLIB_EXPAT)$(OPAL_SIP), yesyesyes)
    SUBDIRS += samples/testpresent
  endif
  ifeq ($(OPAL_PTLIB_CLI)$(OPAL_FAX), yesyes)
    SUBDIRS += samples/faxopal
  endif
  ifeq ($(OPAL_PTLIB_CLI)$(OPAL_IVR),yesyes)
    SUBDIRS += samples/ivropal
  endif
  ifeq ($(OPAL_PTLIB_CLI)$(OPAL_IVR)$(OPAL_HAS_MIXER),yesyesyes)
    SUBDIRS += samples/opalmcu
  endif
  ifeq ($(OPAL_GSTREAMER),yes)
    SUBDIRS += samples/gstreamer
  endif

endif # OPAL_SAMPLES


ifeq ($(V)$(VERBOSE),)
Q=@
Q_CC = @echo [CC] `echo $< | sed s^$(OPALDIR)/^^` ; 
Q_DEP= @echo [DEP] `echo $< | sed s^$(OPALDIR)/^^` ; 
Q_AR = @echo [AR] `echo $@ | sed s^$(OPALDIR)/^^` ;
Q_LD = @echo [LD] `echo $@ | sed s^$(OPALDIR)/^^` ;
endif


OPAL_SRCDIR = $(OPALDIR)/src
OPAL_INCDIR = $(OPALDIR)/include

CPPFLAGS := -I$(OPAL_INCDIR) $(CPPFLAGS)

ifdef DEBUG
  SHARED_LIB_LINK  = $(OPAL_LIBDIR)/$(OPAL_DEBUG_SHARED_LINK)
  SHARED_LIB_FILE  = $(OPAL_LIBDIR)/$(OPAL_DEBUG_SHARED_FILE)
  STATIC_LIB_FILE  = $(OPAL_LIBDIR)/$(OPAL_DEBUG_STATIC_FILE)
  LIB_SONAME       = $(OPAL_DEBUG_SHARED_BASE)
else
  SHARED_LIB_LINK  = $(OPAL_LIBDIR)/$(OPAL_SHARED_LINK)
  SHARED_LIB_FILE  = $(OPAL_LIBDIR)/$(OPAL_SHARED_FILE)
  STATIC_LIB_FILE  = $(OPAL_LIBDIR)/$(OPAL_STATIC_FILE)
  LIB_SONAME       = $(OPAL_SHARED_BASE)
endif

ifeq ($(P_SHAREDLIB),1)
  TARGET=$(SHARED_LIB_LINK)
else
  TARGET=$(STATIC_LIB_FILE)
endif


REVISION_FILE = $(OPALDIR)/revision.h

ASN_SRCDIR := $(OPAL_SRCDIR)/asn
ASN_INCDIR := $(OPAL_INCDIR)/asn

VPATH_CXX := $(OPAL_SRCDIR)/opal \
	     $(OPAL_SRCDIR)/ep \
	     $(OPAL_SRCDIR)/rtp \
	     $(OPAL_SRCDIR)/lids \
	     $(OPAL_SRCDIR)/codec \
             $(OPAL_SRCDIR)/t38 \
             $(OPAL_SRCDIR)/im

ifneq (,$(wildcard src/iax2))
IAX2_AVAIL = yes
VPATH_CXX += $(OPAL_SRCDIR)/iax2 
endif

ifneq (,$(wildcard src/sip))
SIP_AVAIL = yes
VPATH_CXX += $(OPAL_SRCDIR)/sip
endif

ifneq (,$(wildcard src/h323))
H323_AVAIL = yes
VPATH_CXX += $(OPAL_SRCDIR)/h323 \
	     $(OPAL_SRCDIR)/t120 \
	     $(OPAL_SRCDIR)/asn 
endif

ifneq (,$(wildcard src/h224))
H224_AVAIL = yes
VPATH_CXX += $(OPAL_SRCDIR)/h224

endif

ifneq (,$(wildcard src/h460))
H460_AVAIL = yes
VPATH_CXX += $(OPAL_SRCDIR)/h460
endif

VPATH_C := $(OPAL_SRCDIR)/codec

########################################
# Source files for library


SOURCES += $(OPAL_SRCDIR)/opal/manager.cxx \
           $(OPAL_SRCDIR)/opal/endpoint.cxx \
           $(OPAL_SRCDIR)/opal/connection.cxx \
           $(OPAL_SRCDIR)/opal/call.cxx \
           $(OPAL_SRCDIR)/opal/mediafmt.cxx \
           $(OPAL_SRCDIR)/opal/mediatype.cxx \
           $(OPAL_SRCDIR)/opal/mediasession.cxx \
           $(OPAL_SRCDIR)/opal/mediastrm.cxx \
           $(OPAL_SRCDIR)/opal/patch.cxx \
           $(OPAL_SRCDIR)/opal/transcoders.cxx \
           $(OPAL_SRCDIR)/opal/transports.cxx \
           $(OPAL_SRCDIR)/opal/guid.cxx \
           $(OPAL_SRCDIR)/rtp/rtp.cxx \
           $(OPAL_SRCDIR)/rtp/rtp_session.cxx \
           $(OPAL_SRCDIR)/rtp/jitter.cxx \
           $(OPAL_SRCDIR)/rtp/metrics.cxx \
           $(OPAL_SRCDIR)/rtp/pcapfile.cxx \
           $(OPAL_SRCDIR)/rtp/rtpep.cxx \
           $(OPAL_SRCDIR)/rtp/rtpconn.cxx \
           $(OPAL_SRCDIR)/ep/localep.cxx \
           $(OPAL_SRCDIR)/opal/opal_c.cxx \
           $(OPAL_SRCDIR)/opal/pres_ent.cxx

ifeq ($(OPAL_PTLIB_CLI), yes)
  SOURCES	+= $(OPAL_SRCDIR)/opal/console_mgr.cxx 
endif

ifeq ($(OPAL_IVR), yes)
  SOURCES += $(OPAL_SRCDIR)/ep/ivr.cxx \
             $(OPAL_SRCDIR)/ep/opalvxml.cxx 
endif


ifeq ($(OPAL_HAS_MIXER), yes)
  SOURCES += $(OPAL_SRCDIR)/opal/recording.cxx \
             $(OPAL_SRCDIR)/ep/opalmixer.cxx
  ifeq ($(target_os),mingw)
    LDFLAGS += -lvfw32
  endif
endif

ifeq ($(OPAL_HAS_PCSS), yes)
  SOURCES += $(OPAL_SRCDIR)/ep/pcss.cxx
endif

ifeq ($(OPAL_GSTREAMER),yes)
  SOURCES += $(OPAL_SRCDIR)/ep/GstEndPoint.cxx
endif

########################################

# H.323 files

ifeq ($(OPAL_H323),yes)
ifdef H323_AVAIL

SIMPLE_ASN_FILES := x880 mcs gcc

ifeq ($(OPAL_H450),yes)
SIMPLE_ASN_FILES += h4501 h4502 h4503 h4504 h4505 h4506 h4507 h4508 h4509 h45010 h45011
SOURCES		 += $(OPAL_SRCDIR)/h323/h450pdu.cxx 
endif

ASN_H_FILES   := $(addprefix $(ASN_INCDIR)/,$(addsuffix .h,  $(SIMPLE_ASN_FILES)))
ASN_CXX_FILES := $(addprefix $(ASN_SRCDIR)/,$(addsuffix .cxx,$(SIMPLE_ASN_FILES)))

ASN_H_FILES   += $(ASN_INCDIR)/h225.h
ASN_CXX_FILES += $(ASN_SRCDIR)/h225_1.cxx $(ASN_SRCDIR)/h225_2.cxx

ASN_H_FILES   += $(ASN_INCDIR)/h235.h $(ASN_SRCDIR)/h235_t.cxx
ASN_CXX_FILES += $(ASN_SRCDIR)/h235.cxx

ASN_H_FILES   += $(ASN_INCDIR)/h245.h
ASN_CXX_FILES += $(ASN_SRCDIR)/h245_1.cxx $(ASN_SRCDIR)/h245_2.cxx $(ASN_SRCDIR)/h245_3.cxx

ASN_H_FILES   += $(ASN_INCDIR)/h248.h
ASN_CXX_FILES += $(ASN_SRCDIR)/h248.cxx

ifeq ($(OPAL_H501),yes)
ASN_H_FILES   += $(ASN_INCDIR)/h501.h
ASN_CXX_FILES += $(ASN_SRCDIR)/h501.cxx
endif

.PRECIOUS: $(ASN_CXX_FILES) $(ASN_H_FILES)


SOURCES += $(ASN_CXX_FILES) \
           $(OPAL_SRCDIR)/h323/h323ep.cxx \
           $(OPAL_SRCDIR)/h323/h323.cxx \
           $(OPAL_SRCDIR)/h323/h323caps.cxx \
           $(OPAL_SRCDIR)/h323/h323neg.cxx \
           $(OPAL_SRCDIR)/h323/h323pdu.cxx \
           $(OPAL_SRCDIR)/h323/h323rtp.cxx \
           $(OPAL_SRCDIR)/h323/channels.cxx \
           $(OPAL_SRCDIR)/h323/svcctrl.cxx \
           $(OPAL_SRCDIR)/h323/q931.cxx \
           $(OPAL_SRCDIR)/h323/transaddr.cxx \
           $(OPAL_SRCDIR)/h323/gkclient.cxx \
           $(OPAL_SRCDIR)/h323/gkserver.cxx \
           $(OPAL_SRCDIR)/h323/h225ras.cxx \
           $(OPAL_SRCDIR)/h323/h323trans.cxx \
           $(OPAL_SRCDIR)/h323/h235auth.cxx \

ifeq ($(OPAL_H501),yes)
SOURCES += $(OPAL_SRCDIR)/h323/h501pdu.cxx \
           $(OPAL_SRCDIR)/h323/h323annexg.cxx \
           $(OPAL_SRCDIR)/h323/peclient.cxx 
endif

ifeq ($(OPAL_H460), yes)
ifdef H460_AVAIL
SOURCES += $(OPAL_SRCDIR)/h460/h4601.cxx \
	   $(OPAL_SRCDIR)/h460/h46018.cxx \
	   $(OPAL_SRCDIR)/h460/h46018_h225.cxx \
	   $(OPAL_SRCDIR)/h460/h46019.cxx \
	   $(OPAL_SRCDIR)/h460/h4609.cxx \
	   $(OPAL_SRCDIR)/h460/h460p.cxx \
	   $(OPAL_SRCDIR)/h460/h460pres.cxx \
	   $(OPAL_SRCDIR)/h460/h460tm.cxx \
	   $(OPAL_SRCDIR)/h460/h46024b.cxx \
	   $(OPAL_SRCDIR)/h460/h460_std18.cxx \
	   $(OPAL_SRCDIR)/h460/h460_std23.cxx
endif
endif

ifeq ($(OPAL_PTLIB_SSL), yes)
SOURCES += $(OPAL_SRCDIR)/h235auth1.cxx
LDFLAGS += -lcrypto
endif

ifeq ($(OPAL_T120DATA), yes)
SOURCES += $(OPAL_SRCDIR)/t120/t120proto.cxx \
	   $(OPAL_SRCDIR)/t120/h323t120.cxx \
	   $(OPAL_SRCDIR)/t120/x224.cxx 
endif

ifeq ($(OPAL_T38_CAP), yes)
SOURCES += $(OPAL_SRCDIR)/t38/h323t38.cxx
endif

ifeq ($(OPAL_HAS_H224), yes)
ifdef H224_AVAIL
SOURCES += $(OPAL_SRCDIR)/h224/h323h224.cxx
endif
endif

endif
endif

##################
# IAX2 files

ifeq ($(OPAL_IAX2), yes)
ifdef IAX2_AVAIL

SOURCES += \
           $(OPAL_SRCDIR)/iax2/callprocessor.cxx \
           $(OPAL_SRCDIR)/iax2/frame.cxx \
           $(OPAL_SRCDIR)/iax2/iax2con.cxx \
           $(OPAL_SRCDIR)/iax2/iax2ep.cxx \
           $(OPAL_SRCDIR)/iax2/iax2medstrm.cxx \
           $(OPAL_SRCDIR)/iax2/iedata.cxx \
           $(OPAL_SRCDIR)/iax2/ies.cxx \
           $(OPAL_SRCDIR)/iax2/processor.cxx \
           $(OPAL_SRCDIR)/iax2/receiver.cxx \
           $(OPAL_SRCDIR)/iax2/regprocessor.cxx\
           $(OPAL_SRCDIR)/iax2/remote.cxx \
           $(OPAL_SRCDIR)/iax2/safestrings.cxx \
           $(OPAL_SRCDIR)/iax2/sound.cxx \
           $(OPAL_SRCDIR)/iax2/specialprocessor.cxx \
           $(OPAL_SRCDIR)/iax2/transmit.cxx 
endif
endif

################################################################################
# ZRTP files

ifeq ($(OPAL_ZRTP),yes)

SOURCES += $(OPAL_SRCDIR)/rtp/opalzrtp.cxx \
           $(OPAL_SRCDIR)/rtp/zrtpudp.cxx

endif


################################################################################
# SIP files

ifeq ($(OPAL_SIP),yes)
ifdef SIP_AVAIL

SOURCES += $(OPAL_SRCDIR)/sip/sipep.cxx \
           $(OPAL_SRCDIR)/sip/sipcon.cxx \
           $(OPAL_SRCDIR)/sip/sippdu.cxx \
           $(OPAL_SRCDIR)/sip/sdp.cxx \
           $(OPAL_SRCDIR)/sip/handlers.cxx \
           $(OPAL_SRCDIR)/sip/sippres.cxx

ifeq ($(OPAL_T38_CAP), yes)
SOURCES += $(OPAL_SRCDIR)/t38/sipt38.cxx
endif

endif
endif

##################
# H.224 files

ifeq ($(OPAL_HAS_H224), yes)
ifdef H224_AVAIL

SOURCES += $(OPAL_SRCDIR)/h224/q922.cxx \
	   $(OPAL_SRCDIR)/h224/h224.cxx \
	   $(OPAL_SRCDIR)/h224/h281.cxx
endif
endif

##################
# LIDS

ifeq ($(OPAL_LID), yes)

SOURCES += $(OPAL_SRCDIR)/lids/lid.cxx \
           $(OPAL_SRCDIR)/lids/lidep.cxx \
           $(OPAL_SRCDIR)/lids/lidpluginmgr.cxx \

endif

##################
# CAPI

ifeq ($(OPAL_CAPI), yes)

SOURCES += $(OPAL_SRCDIR)/lids/capi_ep.cxx

endif

##################
# DAHDI

ifeq ($(OPAL_DAHDI), yes)

SOURCES += $(OPAL_SRCDIR)/lids/dahdi_ep.cxx

endif

##################
# SRTP

ifeq ($(OPAL_SRTP), yes)

SOURCES += $(OPAL_SRCDIR)/rtp/srtp_session.cxx

endif


ifneq (,$(SWIG))

####################################################
# Java interface

ifeq ($(OPAL_JAVA), yes)

JAVA_SRCDIR      = $(OPAL_SRCDIR)/java
JAVA_WRAPPER     = $(JAVA_SRCDIR)/java_swig_wrapper.c

VPATH_C         += $(JAVA_SRCDIR)
SOURCES         += $(JAVA_WRAPPER)

endif


####################################################
# Ruby interface

ifeq ($(OPAL_RUBY), yes)

RUBY_SRCDIR      = $(OPAL_SRCDIR)/ruby
RUBY_WRAPPER     = $(JAVA_SRCDIR)/ruby_swig_wrapper.c

VPATH_C         += $(RUBY_SRCDIR)
SOURCES         += $(RUBY_WRAPPER)

endif

endif # SWIG

##################
# T.38 Fax

ifeq ($(OPAL_FAX), yes)
SOURCES += $(OPAL_SRCDIR)/t38/t38proto.cxx \
           $(ASN_SRCDIR)/t38.cxx
endif


##################
# IM/MSRP

SOURCES += $(OPAL_SRCDIR)/im/t140.cxx \
	   $(OPAL_SRCDIR)/im/im_mf.cxx \
	   $(OPAL_SRCDIR)/im/rfc4103.cxx 

ifeq ($(OPAL_HAS_MSRP), yes)
SOURCES += $(OPAL_SRCDIR)/im/msrp.cxx 
endif
ifeq ($(OPAL_HAS_SIPIM), yes)
SOURCES += $(OPAL_SRCDIR)/im/sipim.cxx 
endif

##################
# Software codecs

SOURCES += $(OPAL_SRCDIR)/codec/g711codec.cxx \
           $(OPAL_SRCDIR)/codec/g711.c \
           $(OPAL_SRCDIR)/codec/g722mf.cxx \
           $(OPAL_SRCDIR)/codec/g7221mf.cxx \
           $(OPAL_SRCDIR)/codec/g7222mf.cxx \
           $(OPAL_SRCDIR)/codec/g7231mf.cxx \
           $(OPAL_SRCDIR)/codec/g726mf.cxx \
           $(OPAL_SRCDIR)/codec/g728mf.cxx \
           $(OPAL_SRCDIR)/codec/g729mf.cxx \
           $(OPAL_SRCDIR)/codec/gsm0610mf.cxx \
           $(OPAL_SRCDIR)/codec/gsmamrmf.cxx \
           $(OPAL_SRCDIR)/codec/iLBCmf.cxx \
           $(OPAL_SRCDIR)/codec/t38mf.cxx \
           $(OPAL_SRCDIR)/codec/rfc2833.cxx \
           $(OPAL_SRCDIR)/codec/opalwavfile.cxx \
	   $(OPAL_SRCDIR)/codec/silencedetect.cxx \
	   $(OPAL_SRCDIR)/codec/opalpluginmgr.cxx \
	   $(OPAL_SRCDIR)/codec/ratectl.cxx 

ifeq ($(OPAL_VIDEO), yes)
SOURCES += $(OPAL_SRCDIR)/codec/vidcodec.cxx \
           $(OPAL_SRCDIR)/codec/h261mf.cxx \
           $(OPAL_SRCDIR)/codec/h263mf.cxx \
           $(OPAL_SRCDIR)/codec/h264mf.cxx \
           $(OPAL_SRCDIR)/codec/mpeg4mf.cxx
endif

ifeq ($(OPAL_RFC4175), yes)
SOURCES += $(OPAL_SRCDIR)/codec/rfc4175.cxx
endif

ifeq ($(OPAL_RFC2435), yes)
SOURCES += $(OPAL_SRCDIR)/codec/rfc2435.cxx
endif

ifeq ($(OPAL_G711PLC), yes)
SOURCES += $(OPAL_SRCDIR)/codec/g711a1_plc.cxx
endif

ifeq ($(OPAL_AEC), yes)

SOURCES		+= $(OPAL_SRCDIR)/codec/echocancel.cxx

ifeq ($(SPEEXDSP_SYSTEM), no)

SPEEXDSP_SRCDIR    = $(OPAL_SRCDIR)/codec/speex/libspeex

VPATH_C         += $(SPEEXDSP_SRCDIR)
VPATH_CXX       += $(SPEEXDSP_SRCDIR)

SOURCES         += $(SPEEXDSP_SRCDIR)/speex_preprocess.c \
		   $(SPEEXDSP_SRCDIR)/smallft.c \
                   $(SPEEXDSP_SRCDIR)/misc.c \
                   $(SPEEXDSP_SRCDIR)/mdf.c \
                   $(SPEEXDSP_SRCDIR)/math_approx.c \
                   $(SPEEXDSP_SRCDIR)/kiss_fftr.c \
                   $(SPEEXDSP_SRCDIR)/kiss_fft.c \
                   $(SPEEXDSP_SRCDIR)/fftwrap.c
endif # SPEEXDSP_SYSTEM

endif # ifeq ($(OPAL_AEC), yes)


###############################################################################
# Build rules

.SUFFIXES:	.cxx .prc 

vpath %.cxx $(VPATH_CXX)
vpath %.cpp $(VPATH_CXX)
vpath %.c   $(VPATH_C)
vpath %.o   $(OPAL_OBJDIR)
vpath %.dep $(OPAL_DEPDIR)

#  clean whitespace out of source file list
SOURCES         := $(strip $(SOURCES))

#
# create list of object files 
#
SRC_OBJS := $(SOURCES:.c=.o)
SRC_OBJS := $(SRC_OBJS:.cxx=.o)
SRC_OBJS := $(SRC_OBJS:.cpp=.o)
OBJS	 := $(EXTERNALOBJS) $(patsubst %.o, $(OPAL_OBJDIR)/%.o, $(notdir $(SRC_OBJS) $(OBJS)))

#
# define rule for .cxx, .cpp and .c files
#
$(OPAL_OBJDIR)/%.o : %.cxx
	@if [ ! -d $(dir $@) ] ; then $(MKDIR_P) $(dir $@) ; fi
	$(Q_CC)$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OPAL_OBJDIR)/%.o :  %.c 
	@if [ ! -d $(dir $@) ] ; then $(MKDIR_P) $(dir $@) ; fi
	$(Q_CC)$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


#
# create list of dependency files 
#
SRC_DEPS := $(SOURCES:.c=.dep)
SRC_DEPS := $(SRC_DEPS:.cxx=.dep)
SRC_DEPS := $(SRC_DEPS:.cpp=.dep)
DEPS	 := $(patsubst %.dep, $(OPAL_DEPDIR)/%.dep, $(notdir $(SRC_DEPS) $(DEPS)))

#
# define rule for .dep files
#
$(OPAL_DEPDIR)/%.dep : %.cxx 
	@if [ ! -d $(dir $@) ] ; then $(MKDIR_P) $(dir $@) ; fi
	@printf %s $(OPAL_OBJDIR)/ > $@
	$(Q_DEP)$(CXX) $(CPPFLAGS) -M $< >> $@

$(OPAL_DEPDIR)/%.dep : %.c 
	@if [ ! -d $(dir $@) ] ; then $(MKDIR_P) $(dir $@) ; fi
	@printf %s $(OPAL_OBJDIR)/ > $@
	$(Q_DEP)$(CC) $(CPPFLAGS) -M $< >> $@


#####################################################################
# Main targets

# This target was intetionally put here since make dumbly executes the first it finds
# when called without an explicit target.


.PHONY: default_target

default_target : show_target $(TARGET) subdirs


.PHONY: default_depend
default_depend :: depend

.PHONY: show_target
show_target:
	@echo OS=$(target_os), CPU=$(target_cpu)

.PHONY: subdirs $(SUBDIRS)

subdirs:
	@set -e; $(foreach dir,$(SUBDIRS),if test -d ${dir} ; then $(MAKE) -C $(dir); fi ; )

clean:
	rm -rf $(OPAL_LIBDIR)
	@set -e; $(foreach dir,$(SUBDIRS),if test -d ${dir} ; then $(MAKE) -C $(dir) clean; fi ; )

sterile: clean
	rm -f make/toplevel.mak plugins/Makefile

depend: show_target $(DEPS) 
	@set -e; $(foreach dir,$(SUBDIRS),if test -d ${dir} ; then $(MAKE) -C $(dir) depend; fi ; )


ifneq ($(wildcard $(OPAL_DEPDIR)/*.dep),)
include $(OPAL_DEPDIR)/*.dep
endif


#####################################################################
# Targets for the libraries

$(STATIC_LIB_FILE): $(OBJS) 
	@if [ ! -d $(dir $@) ] ; then $(MKDIR_P) $(dir $@) ; fi
	$(Q_AR)$(AR) $(ARFLAGS) $@ $(OBJS)
ifeq ($(HAVE_RANLIB),yes)
	$Q $(RANLIB) $@
endif

ifeq ($(OPAL_SHARED_LIB),1)

  $(SHARED_LIB_LINK): $(SHARED_LIB_FILE)
	$Q cd $(dir $@) ; rm -f $@ ; $(LN_S) $(notdir $<) $(notdir $@)

  $(SHARED_LIB_FILE): $(STATIC_LIB_FILE)
	@if [ ! -d $(dir $@) ] ; then $(MKDIR_P) $(dir $@) ; fi
	$(Q_LD)$(LD) -o $@ $(SHARED_LDFLAGS:INSERT_SONAME=$(LIB_SONAME)) $(OBJS) $(LDFLAGS)

endif # OPAL_SHARED_LIB


####################################################

ifneq (,$(SWIG))

ifeq ($(OPAL_JAVA), yes)
$(JAVA_WRAPPER): $(JAVA_SRCDIR)/opal.i $(OPAL_INCDIR)/opal.h
	$(SWIG) -java -package org.opalvoip -w451 -I$(OPAL_INCDIR) -o $@ $<
endif

ifeq ($(OPAL_RUBY), yes)
$(RUBY_WRAPPER): $(RUBY_SRCDIR)/opal.i $(OPAL_INCDIR)/opal.h
	$(SWIG) -ruby -w451 -I$(OPAL_INCDIR) -o $@ $<
endif

endif


####################################################

$(OPAL_SRCDIR)/opal/manager.cxx: $(REVISION_FILE)

$(REVISION_FILE) : $(REVISION_FILE).in
ifeq ($(SVN),)
	$(Q)sed -e "s/.WCREV./`sed -n -e 's/.*Revision: \([0-9]*\).*/\1/p' $<`/" $< > $@
else
	$(Q)sed "s/SVN_REVISION.*/SVN_REVISION `LC_ALL=C $(SVN) info | sed -n 's/Revision: //p'`/" $< > $@.tmp
	$(Q)if diff -q $@ $@.tmp >/dev/null 2>&1; then \
	  rm $@.tmp; \
	else \
	  mv -f $@.tmp $@; \
	  echo "Revision file updated to `sed -n 's/.*SVN_REVISION\(.*\)/\1/p' $@`" ; \
	fi

.PHONY: $(REVISION_FILE)
endif


#####################################################################
# Install targets

install:
	$(MKDIR_P) $(DESTDIR)$(libdir); chmod 755 $(DESTDIR)$(libdir)
	( if test -e $(OPAL_LIBDIR)/$(OPAL_STATIC_FILE) ; then \
	  $(INSTALL) -m 755 $(OPAL_LIBDIR)/$(OPAL_STATIC_FILE) $(DESTDIR)$(libdir) ; \
	fi )
	( if test -e $(OPAL_LIBDIR)/$(OPAL_DEBUG_STATIC_FILE) ; then \
	  $(INSTALL) -m 755 $(OPAL_LIBDIR)/$(OPAL_DEBUG_STATIC_FILE) $(DESTDIR)$(libdir) ; \
	fi )
	( if test -e $(OPAL_LIBDIR)/$(OPAL_SHARED_FILE) ; then \
	  $(INSTALL) -m 755 $(OPAL_LIBDIR)/$(OPAL_SHARED_FILE) $(DESTDIR)$(libdir) ; \
	  cd $(OPAL_LIBDIR) \
          $(LN_S) -nf $(OPAL_SHARED_FILE) $(OPAL_SHARED_LINK) ; \
	fi )
	( if test -e $(OPAL_LIBDIR)/$(OPAL_DEBUG_SHARED_FILE) ; then \
	  $(INSTALL) -m 755 $(OPAL_LIBDIR)/$(OPAL_DEBUG_SHARED_FILE) $(DESTDIR)$(libdir) ; \
	  cd $(OPAL_LIBDIR) \
	  $(LN_S) -nf $(OPAL_DEBUG_SHARED_FILE) $(OPAL_DEBUG_SHARED_LINK) ; \
	fi )
	$(MKDIR_P) $(DESTDIR)$(libdir)/pkgconfig ; chmod 755 $(DESTDIR)$(libdir)/pkgconfig
	$(INSTALL) -m 644 opal.pc $(DESTDIR)$(libdir)/pkgconfig
	$(MKDIR_P) $(DESTDIR)$(datarootdir)/opal/make ; chmod 755 $(DESTDIR)$(datarootdir)/opal/make
	$(INSTALL) -m 644 make/opal_defs.mak $(DESTDIR)$(datarootdir)/opal/make
	$(INSTALL) -m 644 opal_inc.mak $(DESTDIR)$(datarootdir)/opal/make
	$(MKDIR_P) $(DESTDIR)$(includedir); chmod 755 $(DESTDIR)$(includedir)
	$(INSTALL) -m 644 include/opal.h $(DESTDIR)$(includedir)
	( for dir in asn codec ep h323 h460 iax2 im lids opal rtp sip t120 t38; \
		do $(MKDIR_P) $(DESTDIR)$(includedir)/$$dir ; chmod 755 $(DESTDIR)$(includedir)/$$dir ; \
		( for fn in include/$$dir/*.h ; do \
			$(INSTALL) -m 644 $$fn $(DESTDIR)$(includedir)/$$dir ; \
		done); \
	done)

ifeq ($(OPAL_PLUGINS),yes)
	$(MAKE) -C plugins install
endif

uninstall:
	rm -rf $(DESTDIR)$(includedir) \
               $(DESTDIR)$(datarootdir)/opal \
	       $(DESTDIR)$(libdir)/$(OPAL_SHARED_LINK) \
               $(DESTDIR)$(libdir)/$(OPAL_SHARED_FILE) \
               $(DESTDIR)$(libdir)/$(OPAL_STATIC_FILE) \
               $(DESTDIR)$(libdir)/$(OPAL_DEBUG_SHARED_LINK) \
               $(DESTDIR)$(libdir)/$(OPAL_DEBUG_SHARED_FILE) \
               $(DESTDIR)$(libdir)/$(OPAL_DEBUG_STATIC_FILE) \
	       $(DESTDIR)$(libdir)/pkgconfig/opal.pc
ifeq ($(OPAL_PLUGINS),yes)
	$(MAKE) -C plugins uninstall
endif


###############################################################################
# A collection or random useful targets

docs: FORCE
	rm -rf html
	doxygen opal_cfg.dxy > doxygen.out 2>&1

graphdocs: FORCE
	rm -rf html
	sed "s/HAVE_DOT.*=.*/HAVE_DOT=YES/" opal_cfg.dxy > /tmp/docs.dxy
	doxygen /tmp/docs.dxy > doxygen.out 2>&1
	rm /tmp/docs.dxy

FORCE:

opt:
	$(MAKE) DEBUG_BUILD=no OPAL_SHARED_LIB=1

optnoshared:
	$(MAKE) DEBUG_BUILD=no OPAL_SHARED_LIB=0

optdepend:
	$(MAKE) DEBUG_BUILD=no depend

debug:
	$(MAKE) DEBUG_BUILD=yes OPAL_SHARED_LIB=1

debugnoshared:
	$(MAKE) DEBUG_BUILD=yes OPAL_SHARED_LIB=0

debugdepend:
	$(MAKE) DEBUG_BUILD=yes depend

both: opt debug

bothnoshared:
	$(MAKE) DEBUG_BUILD=yes P_SHAREDLIB=0
	$(MAKE) DEBUG_BUILD=no P_SHAREDLIB=0

all: optdepend opt debugdepend debug

.PHONY: update
ifeq ($(SVN),)
update:
	@echo SVN not available
	@false
else
update:
	$(SVN) update
	$(MAKE) all
endif

distclean: clean
	rm -rf config.log config.err autom4te.cache config.status a.out aclocal.m4
	rm -rf lib*

# End of file #################################################################
