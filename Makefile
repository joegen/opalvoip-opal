#
# Makefile
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
# $Log: Makefile,v $
# Revision 1.2002  2001/07/30 03:41:20  robertj
# Added build of subdirectories for samples.
# Hid the asnparser.version file.
# Changed default OPALDIR variable to be current directory.
#
# Revision 2.0  2001/07/27 15:48:24  robertj
# Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
#


SUBDIRS := samples/simple


OPAL_LIBRARY_MAKEFILE=1

ifndef OPALDIR
OPALDIR=$(CURDIR)
export OPALDIR
endif

include $(OPALDIR)/opal_inc.mak


OPAL_OBJDIR = $(OPAL_LIBDIR)/$(PT_OBJBASE)
ifdef NOTRACE
OPAL_OBJDIR += n
endif


OBJDIR	=	$(OPAL_OBJDIR)
LIBDIR	=	$(OPAL_LIBDIR)
TARGET	=	$(LIBDIR)/$(OPAL_FILE)
VERSION_FILE = samples/simple/version.h


ifdef NOTRACE
STDCCFLAGS += -DPASN_NOPRINTON
else
STDCCFLAGS += -DPTRACING
endif

VPATH_CXX := $(OPAL_SRCDIR)/opal \
	     $(OPAL_SRCDIR)/rtp \
	     $(OPAL_SRCDIR)/h323 \
	     $(OPAL_SRCDIR)/lids \
	     $(OPAL_SRCDIR)/codec \
	     $(OPAL_SRCDIR)/t120 \
	     $(OPAL_SRCDIR)/t38 \

VPATH_C := $(OPAL_SRCDIR)/codec


########################################

# ASN files

ASN_SRCDIR := $(OPAL_SRCDIR)/asn
ASN_INCDIR := $(OPAL_INCDIR)/asn

H450_ASN_FILES := h4501 h4502 h4503 h4504 h4505 h4506 h4507 h4508 h4509 h45010 h45011
SIMPLE_ASN_FILES := x880 $(H450_ASN_FILES) mcs gcc t38 ldap

ASN_H_FILES   := $(addprefix $(ASN_INCDIR)/,$(addsuffix .h,  $(SIMPLE_ASN_FILES)))
ASN_CXX_FILES := $(addprefix $(ASN_SRCDIR)/,$(addsuffix .cxx,$(SIMPLE_ASN_FILES)))

ASN_H_FILES   += $(ASN_INCDIR)/h225.h
ASN_CXX_FILES += $(ASN_SRCDIR)/h225_1.cxx $(ASN_SRCDIR)/h225_2.cxx

ASN_H_FILES   += $(ASN_INCDIR)/h235.h $(ASN_SRCDIR)/h235_t.cxx
ASN_CXX_FILES += $(ASN_SRCDIR)/h235.cxx

ASN_H_FILES   += $(ASN_INCDIR)/h245.h
ASN_CXX_FILES += $(ASN_SRCDIR)/h245_1.cxx $(ASN_SRCDIR)/h245_2.cxx $(ASN_SRCDIR)/h245_3.cxx

.PRECIOUS: $(ASN_CXX_FILES) $(ASN_H_FILES)


# Source files for library

SOURCES := $(ASN_CXX_FILES) \
           $(OPAL_SRCDIR)/opal/manager.cxx \
           $(OPAL_SRCDIR)/opal/endpoint.cxx \
           $(OPAL_SRCDIR)/opal/connection.cxx \
           $(OPAL_SRCDIR)/opal/call.cxx \
           $(OPAL_SRCDIR)/opal/mediafmt.cxx \
           $(OPAL_SRCDIR)/opal/mediastrm.cxx \
           $(OPAL_SRCDIR)/opal/patch.cxx \
           $(OPAL_SRCDIR)/opal/transcoders.cxx \
           $(OPAL_SRCDIR)/opal/transports.cxx \
           $(OPAL_SRCDIR)/opal/guid.cxx \
           $(OPAL_SRCDIR)/opal/pcss.cxx \
           $(OPAL_SRCDIR)/rtp/rtp.cxx \
           $(OPAL_SRCDIR)/rtp/jitter.cxx \
           $(OPAL_SRCDIR)/h323/h323ep.cxx \
           $(OPAL_SRCDIR)/h323/h323.cxx \
           $(OPAL_SRCDIR)/h323/h323caps.cxx \
           $(OPAL_SRCDIR)/h323/h323neg.cxx \
           $(OPAL_SRCDIR)/h323/h323pdu.cxx \
           $(OPAL_SRCDIR)/h323/h323rtp.cxx \
           $(OPAL_SRCDIR)/h323/channels.cxx \
           $(OPAL_SRCDIR)/h323/h450pdu.cxx \
           $(OPAL_SRCDIR)/h323/q931.cxx \
           $(OPAL_SRCDIR)/h323/transaddr.cxx \
           $(OPAL_SRCDIR)/h323/gkclient.cxx \
           $(OPAL_SRCDIR)/h323/gkserver.cxx \
           $(OPAL_SRCDIR)/h323/h225ras.cxx \
           $(OPAL_SRCDIR)/h323/h235security.cxx \

ifdef HAS_OPENSSL
SOURCES += $(OPAL_SRCDIR)/h235ras.cxx
endif

SOURCES += $(OPAL_SRCDIR)/lids/lid.cxx \
           $(OPAL_SRCDIR)/lids/lidep.cxx \
           $(OPAL_SRCDIR)/lids/h323lid.cxx \

ifdef HAS_IXJ
SOURCES += $(OPAL_SRCDIR)/lids/ixjunix.cxx
endif

ifdef HAS_VPB
SOURCES += $(OPAL_SRCDIR)/lids/vpblid.cxx
endif


# Software codecs

SOURCES += $(OPAL_SRCDIR)/codec/g711.c

GSM_DIR 	= $(OPAL_SRCDIR)/codec/gsm
GSM_SRCDIR	= $(GSM_DIR)/src
GSM_INCDIR	= $(GSM_DIR)/inc

SOURCES += $(OPAL_SRCDIR)/codec/gsmcodec.c \
           $(GSM_SRCDIR)/gsm_create.c \
           $(GSM_SRCDIR)/gsm_destroy.c \
           $(GSM_SRCDIR)/gsm_decode.c \
           $(GSM_SRCDIR)/gsm_encode.c \
           $(GSM_SRCDIR)/gsm_option.c \
           $(GSM_SRCDIR)/code.c \
           $(GSM_SRCDIR)/decode.c \
           $(GSM_SRCDIR)/add.c \
           $(GSM_SRCDIR)/lpc.c \
           $(GSM_SRCDIR)/rpe.c \
           $(GSM_SRCDIR)/preprocess.c \
           $(GSM_SRCDIR)/long_term.c \
           $(GSM_SRCDIR)/short_term.c \
           $(GSM_SRCDIR)/table.c

LPC10_DIR 	= $(OPAL_SRCDIR)/codec/lpc10
LPC10_INCDIR	= $(LPC10_DIR)
LPC10_SRCDIR	= $(LPC10_DIR)/src

SOURCES += $(OPAL_SRCDIR)/codec/lpc10codec.c \
           $(LPC10_SRCDIR)/f2clib.c \
           $(LPC10_SRCDIR)/analys.c \
           $(LPC10_SRCDIR)/bsynz.c \
           $(LPC10_SRCDIR)/chanwr.c \
           $(LPC10_SRCDIR)/dcbias.c \
           $(LPC10_SRCDIR)/decode_.c \
           $(LPC10_SRCDIR)/deemp.c \
           $(LPC10_SRCDIR)/difmag.c \
           $(LPC10_SRCDIR)/dyptrk.c \
           $(LPC10_SRCDIR)/encode_.c \
           $(LPC10_SRCDIR)/energy.c \
           $(LPC10_SRCDIR)/ham84.c \
           $(LPC10_SRCDIR)/hp100.c \
           $(LPC10_SRCDIR)/invert.c \
           $(LPC10_SRCDIR)/irc2pc.c \
           $(LPC10_SRCDIR)/ivfilt.c \
           $(LPC10_SRCDIR)/lpcdec.c \
           $(LPC10_SRCDIR)/lpcenc.c \
           $(LPC10_SRCDIR)/lpcini.c \
           $(LPC10_SRCDIR)/lpfilt.c \
           $(LPC10_SRCDIR)/median.c \
           $(LPC10_SRCDIR)/mload.c \
           $(LPC10_SRCDIR)/onset.c \
           $(LPC10_SRCDIR)/pitsyn.c \
           $(LPC10_SRCDIR)/placea.c \
           $(LPC10_SRCDIR)/placev.c \
           $(LPC10_SRCDIR)/preemp.c \
           $(LPC10_SRCDIR)/prepro.c \
           $(LPC10_SRCDIR)/random.c \
           $(LPC10_SRCDIR)/rcchk.c \
           $(LPC10_SRCDIR)/synths.c \
           $(LPC10_SRCDIR)/tbdm.c \
           $(LPC10_SRCDIR)/voicin.c \
           $(LPC10_SRCDIR)/vparms.c \

VIC_DIR = $(OPAL_SRCDIR)/codec/vic

SOURCES += $(OPAL_SRCDIR)/codec/h261codec.c \
           $(VIC_DIR)/dct.cxx \
           $(VIC_DIR)/p64.cxx \
           $(VIC_DIR)/huffcode.c \
           $(VIC_DIR)/bv.c \
           $(VIC_DIR)/encoder-h261.cxx \
           $(VIC_DIR)/p64encoder.cxx \
           $(VIC_DIR)/transmitter.cxx \
           $(VIC_DIR)/vid_coder.cxx 

# More protocols

SOURCES += $(OPAL_SRCDIR)/t120/t120proto.cxx \
           $(OPAL_SRCDIR)/t120/h323t120.cxx \
           $(OPAL_SRCDIR)/t120/x224.cxx \
           $(OPAL_SRCDIR)/t38/t38proto.cxx \
           $(OPAL_SRCDIR)/t38/h323t38.cxx \


#
# Files to be cleande during make clean
#

CLEAN_FILES = $(OPAL_LIB) $(ASN_CXX_FILES) $(ASN_H_FILES)


####################################################

include $(PWLIBDIR)/make/common.mak

####################################################

ifeq ($(OSTYPE),beos)
# When linking the shared version of the OPAL library under BeOS,
# pwlib must be in the list of external libraries
SYSLIBS        += -L$(PW_LIBDIR) -l$(PTLIB_BASE)$(LIB_TYPE)
endif

LIB_BASENAME	=	$(OPAL_FILE)

include $(PWLIBDIR)/make/lib.mak

####################################################


notrace::
	$(MAKE) NOTRACE=1 opt


# Build rules for VIC codecs

$(OPAL_OBJDIR)/%.o : $(VIC_DIR)/%.cxx
	@if [ ! -d $(OPAL_OBJDIR) ] ; then mkdir -p $(OPAL_OBJDIR) ; fi
	$(CPLUS) -I$(VIC_DIR) $(STDCCFLAGS) $(CFLAGS) -c $< -o $@

$(OPAL_OBJDIR)/%.o : $(VIC_DIR)/%.c
	@if [ ! -d $(OPAL_OBJDIR) ] ; then mkdir -p $(OPAL_OBJDIR) ; fi
	$(CC) -I$(VIC_DIR) $(STDCCFLAGS) $(CFLAGS) -c $< -o $@

$(DEPDIR)/%.dep : $(VIC_DIR)/%.c
	@if [ ! -d $(DEPDIR) ] ; then mkdir -p $(DEPDIR) ; fi
	@printf %s $(OPAL_OBJDIR)/ > $@
	$(CPLUS) -I$(VIC_DIR) $(STDCCFLAGS) $(CFLAGS) -M $< >> $@

$(DEPDIR)/%.dep : $(VIC_DIR)/%.cxx
	@if [ ! -d $(DEPDIR) ] ; then mkdir -p $(DEPDIR) ; fi
	@printf %s $(OPAL_OBJDIR)/ > $@
	$(CPLUS) -I$(VIC_DIR) $(STDCCFLAGS) $(CFLAGS) -M $< >> $@


# Build rules for the GSM codec

$(OPAL_OBJDIR)/%.o : $(GSM_SRCDIR)/%.c
	@if [ ! -d $(OPAL_OBJDIR) ] ; then mkdir -p $(OPAL_OBJDIR) ; fi
	$(CC) -ansi -I$(GSM_INCDIR) -DWAV49 -DNeedFunctionPrototypes=1 $(OPTCCFLAGS) $(CFLAGS) -c $< -o $@

$(DEPDIR)/%.dep : $(GSM_SRCDIR)/%.c
	@if [ ! -d $(DEPDIR) ] ; then mkdir -p $(DEPDIR) ; fi
	@printf %s $(OPAL_OBJDIR)/ > $@
	$(CC) -ansi -I$(GSM_INCDIR) -DWAV49 -DNeedFunctionPrototypes=1 $(CFLAGS) -M $< >> $@


# Build rules for the LPC10 codec

$(OPAL_OBJDIR)/%.o : $(LPC10_SRCDIR)/%.c
	@if [ ! -d $(OPAL_OBJDIR) ] ; then mkdir -p $(OPAL_OBJDIR) ; fi
	$(CC) -I$(LPC10_INCDIR) $(OPTCCFLAGS) $(CFLAGS) -c $< -o $@

$(DEPDIR)/%.dep : $(LPC10_SRCDIR)/%.c
	@if [ ! -d $(DEPDIR) ] ; then mkdir -p $(DEPDIR) ; fi
	@printf %s $(OPAL_OBJDIR)/ > $@
	$(CC) -I$(LPC10_INCDIR) $(CFLAGS) -M $< >> $@


# Build rules for ASN files

ASNPARSER = $(ASNPARSE_DIR)/obj_$(PLATFORM_TYPE)_r/asnparser 

ifdef ASN_EXCLUDE
ASNPARSE_DIR = $(PWLIBDIR)/tools/asnparser.new
ASNPARSER += -x $(ASN_EXCLUDE)
else
ASNPARSE_DIR = $(PWLIBDIR)/tools/asnparser
endif

.asnparser.version: $(ASNPARSER)
	$(MAKE) -C $(ASNPARSE_DIR) opt
	$(ASNPARSER) --version | awk '{print $$1,$$2,$$3}' > .asnparser.version.new
	if test -e .asnparser.version && diff -q .asnparser.version.new .asnparser.version ; then rm .asnparser.version.new ; else mv .asnparser.version.new .asnparser.version ; fi

$(ASNPARSER):


$(ASN_INCDIR)/%.h : $(ASN_SRCDIR)/%.asn .asnparser.version
	$(ASNPARSER) -m $(shell echo $(notdir $(basename $<)) | tr a-z A-Z) -c $<
	mv $(basename $<).h $@

$(ASN_SRCDIR)/%.cxx : $(ASN_INCDIR)/%.h
	@true

$(OBJDIR)/%.o : $(ASN_SRCDIR)/%.cxx
	@if [ ! -d $(OBJDIR) ] ; then mkdir -p $(OBJDIR) ; fi
	$(CPLUS) $(STDCCFLAGS) $(OPTCCFLAGS) $(CFLAGS) -I$(ASN_INCDIR) -c $< -o $@

$(DEPDIR)/%.dep : $(ASN_SRCDIR)/%.cxx
	@if [ ! -d $(DEPDIR) ] ; then mkdir -p $(DEPDIR) ; fi
	@printf %s $(OBJDIR)/ > $@
	$(CPLUS) $(STDCCFLAGS) -I$(ASN_INCDIR) -M $< >> $@



#### h225

$(ASN_SRCDIR)/h225_1.cxx \
$(ASN_SRCDIR)/h225_2.cxx : $(ASN_INCDIR)/h225.h $(ASN_SRCDIR)/h235_t.cxx

$(ASN_INCDIR)/h225.h: $(ASN_SRCDIR)/h225.asn $(ASN_INCDIR)/h235.h $(ASN_INCDIR)/h245.h .asnparser.version
	$(ASNPARSER) -s2 -m H225 -r MULTIMEDIA-SYSTEM-CONTROL=H245 -c $<
	mv $(basename $<).h $@


#### h245

$(ASN_SRCDIR)/h245_1.cxx \
$(ASN_SRCDIR)/h245_2.cxx \
$(ASN_SRCDIR)/h245_3.cxx : $(ASN_INCDIR)/h245.h


$(ASN_INCDIR)/h245.h: $(ASN_SRCDIR)/h245.asn .asnparser.version
	$(ASNPARSER) -s3 -m H245 -c $<
	mv $(basename $<).h $@


#### Various dependencies

$(ASN_SRCDIR)/h235.cxx $(ASN_SRCDIR)/h235_t.cxx : $(ASN_INCDIR)/h235.h

$(addprefix $(ASN_SRCDIR)/,$(addsuffix .cxx,$(H450_ASN_FILES))) : \
                                $(ASN_INCDIR)/x880.h $(ASN_INCDIR)/h225.h



###############################################################################
#### Subdirectories

$(STANDARD_TARGETS) ::
	set -e; $(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) $@;)



# End of file #################################################################

