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
# Revision 1.2029  2004/04/25 02:53:28  rjongbloed
# Fixed GNU 3.4 warnings
#
# Revision 2.27  2004/03/11 06:54:25  csoutheren
# Added ability to disable SIP or H.323 stacks
#
# Revision 2.26  2004/02/23 01:28:49  rjongbloed
# Fixed unix build for recent upgrade to OpenH323 latest code.
#
# Revision 2.25  2004/02/16 09:15:19  csoutheren
# Fixed problems with codecs on Unix systems
#
# Revision 2.24  2003/04/08 11:46:35  robertj
# Better portability for tr command when doing ASN parse.
#
# Revision 2.23  2003/04/08 06:09:19  robertj
# Fixed ASN compilation so do not need -I on compile line for asn includes.
#
# Revision 2.22  2003/04/02 06:52:04  robertj
# Added dependencies for H450 ASN files
#
# Revision 2.21  2003/03/26 02:49:00  robertj
# Added service/daemon sample application.
#
# Revision 2.20  2003/03/19 04:45:29  robertj
# Added opalvxml to build
#
# Revision 2.19  2003/03/18 23:09:37  robertj
# Fixed LD_LIBRARY_PATH issue with Solaris
#
# Revision 2.18  2003/03/17 23:08:41  robertj
# Added IVR endpoint
#
# Revision 2.17  2003/03/17 22:36:38  robertj
# Added video support.
#
# Revision 2.16  2003/01/15 00:08:18  robertj
# Updated to OpenH323 v1.10.3
#
# Revision 2.15  2002/11/12 12:06:34  robertj
# Fixed Solaris compatibility
#
# Revision 2.14  2002/11/11 07:43:32  robertj
# Added speex codec files
#
# Revision 2.13  2002/09/11 05:56:16  robertj
# Fixed double inclusion of common.mak
# Added opalwavfile.cxx module
#
# Revision 2.12  2002/03/15 10:51:53  robertj
# Fixed problem with recursive inclusion on make files.
#
# Revision 2.11  2002/03/05 06:27:34  robertj
# Added ASN parser build and version check code.
#
# Revision 2.10  2002/02/22 04:16:25  robertj
# Added G.726 codec and fixed the lpc10 and GSM codecs.
#
# Revision 2.9  2002/02/11 09:38:28  robertj
# Moved version to root directory
#
# Revision 2.8  2002/02/06 11:52:53  rogerh
# Move -I$(ASN_INCDIR) so Opal's ldap.h is found instead of the OS's ldap.h
#
# Revision 2.7  2002/02/01 10:29:35  rogerh
# Use the right version.h file. (the other one had comments which confused
# pwlib's lib.mak)
#
# Revision 2.6  2002/02/01 04:58:23  craigs
# Added sip directory to VPATH
#
# Revision 2.5  2002/02/01 04:53:01  robertj
# Added (very primitive!) SIP support.
#
# Revision 2.4  2002/02/01 00:19:20  robertj
# Updated to latest pwlilb.
# Added rfc2833 module
#
# Revision 2.3  2001/08/17 05:24:22  robertj
# Updates from OpenH323 v1.6.0 release.
#
# Revision 2.2  2001/08/01 06:22:55  robertj
# Major changes to H.323 capabilities, uses OpalMediaFormat for base name.
# Added G.711 transcoder.
#
# Revision 2.1  2001/07/30 03:41:20  robertj
# Added build of subdirectories for samples.
# Hid the asnparser.version file.
# Changed default OPALDIR variable to be current directory.
#
# Revision 2.0  2001/07/27 15:48:24  robertj
# Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
#

SUBDIRS := samples/simple samples/opalgw

ifndef OPALDIR
OPALDIR=$(CURDIR)
export OPALDIR
endif

LIBRARY_MAKEFILE:=1

include $(OPALDIR)/opal_inc.mak


OPAL_OBJDIR = $(OPAL_LIBDIR)/$(PT_OBJBASE)
ifdef NOTRACE
OPAL_OBJDIR += n
endif


OBJDIR	=	$(OPAL_OBJDIR)
LIBDIR	=	$(OPAL_LIBDIR)
TARGET	=	$(LIBDIR)/$(OPAL_FILE)

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
	     $(OPAL_SRCDIR)/sip \

VPATH_C := $(OPAL_SRCDIR)/codec

########################################
# Source files for library

SOURCES := $(OPAL_SRCDIR)/opal/manager.cxx \
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
           $(OPAL_SRCDIR)/opal/ivr.cxx \
           $(OPAL_SRCDIR)/opal/opalvxml.cxx \
           $(OPAL_SRCDIR)/rtp/rtp.cxx \
           $(OPAL_SRCDIR)/rtp/jitter.cxx \

########################################
# H.323 files

ifeq ($(OPAL_H323),1)

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

ASN_H_FILES   += $(ASN_INCDIR)/h248.h
ASN_CXX_FILES += $(ASN_SRCDIR)/h248.cxx

ASN_H_FILES   += $(ASN_INCDIR)/h501.h
ASN_CXX_FILES += $(ASN_SRCDIR)/h501.cxx

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
           $(OPAL_SRCDIR)/h323/h450pdu.cxx \
           $(OPAL_SRCDIR)/h323/q931.cxx \
           $(OPAL_SRCDIR)/h323/transaddr.cxx \
           $(OPAL_SRCDIR)/h323/gkclient.cxx \
           $(OPAL_SRCDIR)/h323/gkserver.cxx \
           $(OPAL_SRCDIR)/h323/h225ras.cxx \
           $(OPAL_SRCDIR)/h323/h323trans.cxx \
           $(OPAL_SRCDIR)/h323/h235auth.cxx \
           $(OPAL_SRCDIR)/h323/h501pdu.cxx \
           $(OPAL_SRCDIR)/h323/h323annexg.cxx \
           $(OPAL_SRCDIR)/h323/peclient.cxx \

ifdef HAS_OPENSSL
SOURCES += $(OPAL_SRCDIR)/h235auth1.cxx
endif

SOURCES += $(OPAL_SRCDIR)/t120/t120proto.cxx \
           $(OPAL_SRCDIR)/t120/h323t120.cxx \
           $(OPAL_SRCDIR)/t120/x224.cxx \
           $(OPAL_SRCDIR)/t38/t38proto.cxx \
           $(OPAL_SRCDIR)/t38/h323t38.cxx \


endif

##################
# SIP files

ifeq ($(OPAL_SIP),1)

SOURCES += $(OPAL_SRCDIR)/sip/sipep.cxx \
           $(OPAL_SRCDIR)/sip/sipcon.cxx \
           $(OPAL_SRCDIR)/sip/sippdu.cxx \
           $(OPAL_SRCDIR)/sip/sdp.cxx \

endif

##################
# LIDS


SOURCES += $(OPAL_SRCDIR)/lids/lid.cxx \
           $(OPAL_SRCDIR)/lids/lidep.cxx \

ifdef HAS_IXJ
SOURCES += $(OPAL_SRCDIR)/lids/ixjunix.cxx
endif

ifdef HAS_VPB
SOURCES += $(OPAL_SRCDIR)/lids/vpblid.cxx
endif


##################
# Software codecs

SOURCES += $(OPAL_SRCDIR)/codec/g711codec.cxx \
           $(OPAL_SRCDIR)/codec/g711.c \
           $(OPAL_SRCDIR)/codec/rfc2833.cxx \
           $(OPAL_SRCDIR)/codec/vidcodec.cxx \
           $(OPAL_SRCDIR)/codec/opalwavfile.cxx


# G.726

G726_DIR = g726

SOURCES  += $(OPAL_SRCDIR)/codec/g726codec.cxx \
            $(G726_DIR)/g72x.c \
            $(G726_DIR)/g726_16.c \
            $(G726_DIR)/g726_24.c \
            $(G726_DIR)/g726_32.c \
            $(G726_DIR)/g726_40.c


# GSM06.10

GSM_DIR 	= $(OPAL_SRCDIR)/codec/gsm
GSM_SRCDIR	= $(GSM_DIR)/src
GSM_INCDIR	= $(GSM_DIR)/inc

SOURCES += $(OPAL_SRCDIR)/codec/gsmcodec.cxx \
           $(GSM_SRCDIR)/gsm_create.c \
           $(GSM_SRCDIR)/gsm_destroy.c \
           $(GSM_SRCDIR)/gsm_decode.c \
           $(GSM_SRCDIR)/gsm_encode.c \
           $(GSM_SRCDIR)/gsm_option.c \
           $(GSM_SRCDIR)/code.c \
           $(GSM_SRCDIR)/decode.c \
           $(GSM_SRCDIR)/add.c \
           $(GSM_SRCDIR)/gsm_lpc.c \
           $(GSM_SRCDIR)/rpe.c \
           $(GSM_SRCDIR)/preprocess.c \
           $(GSM_SRCDIR)/long_term.c \
           $(GSM_SRCDIR)/short_term.c \
           $(GSM_SRCDIR)/table.c


SOURCES += $(OPAL_SRCDIR)/codec/mscodecs.cxx


# LPC-10

LPC10_DIR 	= $(OPAL_SRCDIR)/codec/lpc10
LPC10_INCDIR	= $(LPC10_DIR)
LPC10_SRCDIR	= $(LPC10_DIR)/src

SOURCES += $(OPAL_SRCDIR)/codec/lpc10codec.cxx \
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


# Speex

SPEEX_DIR       = $(OPAL_SRCDIR)/codec/speex
SPEEX_INCDIR    = $(SPEEX_DIR)/libspeex
SPEEX_SRCDIR    = $(SPEEX_DIR)/libspeex

VPATH_C		+= $(SPEEX_SRCDIR)
VPATH_CXX	+= $(SPEEX_SRCDIR)

HEADER_FILES    += $(OPAL_INCDIR)/speexcodec.h \
		   $(SPEEX_INCDIR)/speex.h \
                   $(SPEEX_INCDIR)/speex_bits.h \
                   $(SPEEX_INCDIR)/speex_header.h \
                   $(SPEEX_INCDIR)/speex_callbacks.h

SOURCES 	+= $(OPAL_SRCDIR)/codec/speexcodec.cxx \
		   $(SPEEX_SRCDIR)/nb_celp.c \
                   $(SPEEX_SRCDIR)/sb_celp.c \
                   $(SPEEX_SRCDIR)/lpc.c \
                   $(SPEEX_SRCDIR)/ltp.c \
                   $(SPEEX_SRCDIR)/lsp.c \
                   $(SPEEX_SRCDIR)/quant_lsp.c \
                   $(SPEEX_SRCDIR)/lsp_tables_nb.c \
                   $(SPEEX_SRCDIR)/gain_table.c \
                   $(SPEEX_SRCDIR)/gain_table_lbr.c \
                   $(SPEEX_SRCDIR)/cb_search.c \
                   $(SPEEX_SRCDIR)/filters.c \
                   $(SPEEX_SRCDIR)/bits.c \
                   $(SPEEX_SRCDIR)/modes.c \
                   $(SPEEX_SRCDIR)/vq.c \
                   $(SPEEX_SRCDIR)/high_lsp_tables.c \
                   $(SPEEX_SRCDIR)/vbr.c \
                   $(SPEEX_SRCDIR)/hexc_table.c \
                   $(SPEEX_SRCDIR)/exc_5_256_table.c \
                   $(SPEEX_SRCDIR)/exc_5_64_table.c \
                   $(SPEEX_SRCDIR)/exc_8_128_table.c \
                   $(SPEEX_SRCDIR)/exc_10_32_table.c \
                   $(SPEEX_SRCDIR)/exc_10_16_table.c \
                   $(SPEEX_SRCDIR)/exc_20_32_table.c \
                   $(SPEEX_SRCDIR)/hexc_10_32_table.c \
                   $(SPEEX_SRCDIR)/misc.c \
                   $(SPEEX_SRCDIR)/speex_header.c \
                   $(SPEEX_SRCDIR)/speex_callbacks.c


# iLBC

ILBC_DIR 	= $(OPAL_SRCDIR)/codec/iLBC

VPATH_C		+= $(ILBC_DIR)
VPATH_CXX	+= $(ILBC_DIR)

HEADER_FILES	+= $(OPAL_INCDIR)/codec/ilbccodec.h
SOURCES	+= $(OPAL_SRCDIR)/codec/ilbccodec.cxx

HEADER_FILES	+= $(ILBC_DIR)/iLBC_define.h
HEADER_FILES	+= $(ILBC_DIR)/iLBC_decode.h
SOURCES		+= $(ILBC_DIR)/iLBC_decode.c
HEADER_FILES	+= $(ILBC_DIR)/iLBC_encode.h
SOURCES		+= $(ILBC_DIR)/iLBC_encode.c
HEADER_FILES	+= $(ILBC_DIR)/FrameClassify.h
SOURCES		+= $(ILBC_DIR)/FrameClassify.c
HEADER_FILES	+= $(ILBC_DIR)/LPCdecode.h
SOURCES		+= $(ILBC_DIR)/LPCdecode.c
HEADER_FILES	+= $(ILBC_DIR)/LPCencode.h
SOURCES		+= $(ILBC_DIR)/LPCencode.c
HEADER_FILES	+= $(ILBC_DIR)/StateConstructW.h
SOURCES		+= $(ILBC_DIR)/StateConstructW.c
HEADER_FILES	+= $(ILBC_DIR)/StateSearchW.h
SOURCES		+= $(ILBC_DIR)/StateSearchW.c
HEADER_FILES	+= $(ILBC_DIR)/anaFilter.h
SOURCES		+= $(ILBC_DIR)/anaFilter.c
HEADER_FILES	+= $(ILBC_DIR)/constants.h
SOURCES		+= $(ILBC_DIR)/constants.c
HEADER_FILES	+= $(ILBC_DIR)/createCB.h
SOURCES		+= $(ILBC_DIR)/createCB.c
HEADER_FILES	+= $(ILBC_DIR)/doCPLC.h
SOURCES		+= $(ILBC_DIR)/doCPLC.c
HEADER_FILES	+= $(ILBC_DIR)/enhancer.h
SOURCES		+= $(ILBC_DIR)/enhancer.c
HEADER_FILES	+= $(ILBC_DIR)/filter.h
SOURCES		+= $(ILBC_DIR)/filter.c
HEADER_FILES	+= $(ILBC_DIR)/gainquant.h
SOURCES		+= $(ILBC_DIR)/gainquant.c
HEADER_FILES	+= $(ILBC_DIR)/getCBvec.h
SOURCES		+= $(ILBC_DIR)/getCBvec.c
HEADER_FILES	+= $(ILBC_DIR)/helpfun.h
SOURCES		+= $(ILBC_DIR)/helpfun.c
HEADER_FILES	+= $(ILBC_DIR)/hpInput.h
SOURCES		+= $(ILBC_DIR)/hpInput.c
HEADER_FILES	+= $(ILBC_DIR)/hpOutput.h
SOURCES		+= $(ILBC_DIR)/hpOutput.c
HEADER_FILES	+= $(ILBC_DIR)/iCBConstruct.h
SOURCES		+= $(ILBC_DIR)/iCBConstruct.c
HEADER_FILES	+= $(ILBC_DIR)/iCBSearch.h
SOURCES		+= $(ILBC_DIR)/iCBSearch.c
HEADER_FILES	+= $(ILBC_DIR)/lsf.h
SOURCES		+= $(ILBC_DIR)/lsf.c
HEADER_FILES	+= $(ILBC_DIR)/packing.h
SOURCES		+= $(ILBC_DIR)/packing.c
HEADER_FILES	+= $(ILBC_DIR)/syntFilter.h
SOURCES		+= $(ILBC_DIR)/syntFilter.c


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

LIB_BASENAME=$(OPAL_BASE)
LIB_FILENAME=$(OPAL_FILE)


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
	$(CPLUS) -I$(VIC_DIR) $(STDCCFLAGS:-g=) $(CFLAGS) -M $< >> $@

$(DEPDIR)/%.dep : $(VIC_DIR)/%.cxx
	@if [ ! -d $(DEPDIR) ] ; then mkdir -p $(DEPDIR) ; fi
	@printf %s $(OPAL_OBJDIR)/ > $@
	$(CPLUS) -I$(VIC_DIR) $(STDCCFLAGS:-g=) $(CFLAGS) -M $< >> $@


# Build rules for the GSM codec

$(OPAL_OBJDIR)/%.o : $(GSM_SRCDIR)/%.c
	@if [ ! -d $(OPAL_OBJDIR) ] ; then mkdir -p $(OPAL_OBJDIR) ; fi
	$(CC) -ansi -I$(GSM_INCDIR) -DWAV49 -DNeedFunctionPrototypes=1 $(OPTCCFLAGS) $(CFLAGS) -c $< -o $@

$(DEPDIR)/%.dep : $(GSM_SRCDIR)/%.c
	@if [ ! -d $(DEPDIR) ] ; then mkdir -p $(DEPDIR) ; fi
	@printf %s $(OPAL_OBJDIR)/ > $@
	$(CC) -ansi -I$(GSM_INCDIR) -DWAV49 -DNeedFunctionPrototypes=1 $(CFLAGS) -M $< >> $@


# Build rules for the G.726 codec

$(OPAL_OBJDIR)/%.o : $(G726_DIR)/%.c
	@if [ ! -d $(OPAL_OBJDIR) ] ; then mkdir -p $(OPAL_OBJDIR) ; fi
	$(CC) $(CFLAGS) -c $< -o $@

$(DEPDIR)/%.dep : $(G726_DIR)/%.c
	@if [ ! -d $(DEPDIR) ] ; then mkdir -p $(DEPDIR) ; fi
	@printf %s $(OPAL_OBJDIR)/ > $@
	$(CC) $(CFLAGS) -M $< >> $@


# Build rules for the LPC10 codec

$(OPAL_OBJDIR)/%.o : $(LPC10_SRCDIR)/%.c
	@if [ ! -d $(OPAL_OBJDIR) ] ; then mkdir -p $(OPAL_OBJDIR) ; fi
	$(CC) -I$(LPC10_INCDIR) $(OPTCCFLAGS) $(CFLAGS) -c $< -o $@

$(DEPDIR)/%.dep : $(LPC10_SRCDIR)/%.c
	@if [ ! -d $(DEPDIR) ] ; then mkdir -p $(DEPDIR) ; fi
	@printf %s $(OPAL_OBJDIR)/ > $@
	$(CC) -I$(LPC10_INCDIR) $(CFLAGS) -M $< >> $@


ifeq ($(OPAL_H323),1)

# Make sure the asnparser is built and if new version force recompiles

# Use a different variable here to support cross compiling
ifndef HOSTPWLIBDIR
HOSTPWLIBDIR=$(PWLIBDIR)
endif

ifndef HOST_PLATFORM_TYPE
HOST_PLATFORM_TYPE=$(PLATFORM_TYPE)
endif


# Set library path so asnparser will run

ifdef LD_LIBRARY_PATH
export LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(HOSTPWLIBDIR)/lib
else
export LD_LIBRARY_PATH:=$(HOSTPWLIBDIR)/lib
endif


# If we're cross compiling, we want the host's asnparser
# otherwise use the one for the current platform
ASNPARSE_DIR = $(HOSTPWLIBDIR)/tools/asnparser
ASNPARSER = $(ASNPARSE_DIR)/obj_$(HOST_PLATFORM_TYPE)_r/asnparser


# If not cross compiling then make sure asnparser is built
ifeq ($(PLATFORM_TYPE),$(HOST_PLATFORM_TYPE))
$(ASNPARSER):
	$(MAKE) -C $(ASNPARSE_DIR) opt
endif


.asnparser.version: $(ASNPARSER)
	$(MAKE) -C $(ASNPARSE_DIR) opt
	$(ASNPARSER) --version | awk '{print $$1,$$2,$$3}' > .asnparser.version.new
	if test -f .asnparser.version && diff .asnparser.version.new .asnparser.version >/dev/null 2>&1 ; \
                then rm .asnparser.version.new ; \
                else mv .asnparser.version.new .asnparser.version ; \
        fi



# Build rules for ASN files

$(ASN_INCDIR)/%.h : $(ASN_SRCDIR)/%.asn .asnparser.version
	$(ASNPARSER) -h asn/ -m $(shell echo $(notdir $(basename $<)) | tr '[a-z]' '[A-Z]') -c $<
	mv $(basename $<).h $@

$(ASN_SRCDIR)/%.cxx : $(ASN_INCDIR)/%.h
	@true

$(OBJDIR)/%.o : $(ASN_SRCDIR)/%.cxx
	@if [ ! -d $(OBJDIR) ] ; then mkdir -p $(OBJDIR) ; fi
	$(CPLUS) $(STDCCFLAGS) $(OPTCCFLAGS) $(CFLAGS) -c $< -o $@

$(DEPDIR)/%.dep : $(ASN_SRCDIR)/%.cxx
	@if [ ! -d $(DEPDIR) ] ; then mkdir -p $(DEPDIR) ; fi
	@printf %s $(OBJDIR)/ > $@
	$(CPLUS) $(STDCCFLAGS) -M $< >> $@



#### h225

$(ASN_SRCDIR)/h225_1.cxx \
$(ASN_SRCDIR)/h225_1.dep \
$(ASN_SRCDIR)/h225_2.cxx \
$(ASN_SRCDIR)/h225_2.dep : $(ASN_INCDIR)/h225.h $(ASN_SRCDIR)/h235_t.cxx

$(ASN_INCDIR)/h225.h: $(ASN_SRCDIR)/h225.asn $(ASN_INCDIR)/h235.h $(ASN_INCDIR)/h245.h .asnparser.version
	$(ASNPARSER) -s2 -h asn/ -m H225 -r MULTIMEDIA-SYSTEM-CONTROL=H245 -c $<
	mv $(basename $<).h $@


#### h245

$(ASN_SRCDIR)/h245_1.cxx \
$(ASN_SRCDIR)/h245_1.dep \
$(ASN_SRCDIR)/h245_2.cxx \
$(ASN_SRCDIR)/h245_2.dep \
$(ASN_SRCDIR)/h245_3.cxx \
$(ASN_SRCDIR)/h245_3.dep : $(ASN_INCDIR)/h245.h


$(ASN_INCDIR)/h245.h: $(ASN_SRCDIR)/h245.asn .asnparser.version
	$(ASNPARSER) -s3 -h asn/ -m H245 --classheader "H245_AudioCapability=#ifndef PASN_NOPRINTON\nvoid PrintOn(ostream & strm) const;\n#endif" -c $<
	mv $(basename $<).h $@


#### Various dependencies

$(ASN_SRCDIR)/h235.cxx $(ASN_SRCDIR)/h235_t.cxx : $(ASN_INCDIR)/h235.h

$(addprefix $(ASN_SRCDIR)/,$(addsuffix .cxx,$(H450_ASN_FILES))) : \
                                $(ASN_INCDIR)/x880.h \
                                $(ASN_INCDIR)/h4501.h \
                                $(ASN_INCDIR)/h225.h

$(ASN_SRCDIR)/h4506.cxx : $(ASN_INCDIR)/h4504.h
$(ASN_SRCDIR)/h4507.cxx : $(ASN_INCDIR)/h4504.h
$(ASN_SRCDIR)/h4508.cxx : $(ASN_INCDIR)/h4505.h
$(ASN_SRCDIR)/h4509.cxx : $(ASN_INCDIR)/h4504.h $(ASN_INCDIR)/h4507.h
$(ASN_SRCDIR)/h45010.cxx: $(ASN_INCDIR)/h4506.h
$(ASN_SRCDIR)/h45011.cxx: $(ASN_INCDIR)/h4504.h $(ASN_INCDIR)/h4506.h $(ASN_INCDIR)/h45010.h

endif  # OPAL_H323



###############################################################################
#### Subdirectories

$(STANDARD_TARGETS) ::
	set -e; $(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) $@;)



# End of file #################################################################

