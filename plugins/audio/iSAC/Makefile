#
# Makefile for iSAC Codec Plugin
#
# Copyright (C) 2014 Vox Lucida Pty. Ltd., All Rights Reserved
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
# The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
#
# Contributor(s): ______________________________________.
#

BASENAME := iSAC

SRCDIR   := $(CURDIR)/iSAC

SOURCES  := $(CURDIR)/iSACcodec.c \
            $(SRCDIR)/arith_routines.c \
            $(SRCDIR)/arith_routines_hist.c \
            $(SRCDIR)/arith_routines_logist.c \
            $(SRCDIR)/auto_corr_to_refl_coef.c \
            $(SRCDIR)/bandwidth_estimator.c \
            $(SRCDIR)/crc.c \
            $(SRCDIR)/decode.c \
            $(SRCDIR)/decode_bwe.c \
            $(SRCDIR)/division_operations.c \
            $(SRCDIR)/encode.c \
            $(SRCDIR)/encode_lpc_swb.c \
            $(SRCDIR)/entropy_coding.c \
            $(SRCDIR)/fft.c \
            $(SRCDIR)/filter_functions.c \
            $(SRCDIR)/filterbank_tables.c \
            $(SRCDIR)/filterbanks.c \
            $(SRCDIR)/intialize.c \
            $(SRCDIR)/isac.c \
            $(SRCDIR)/lattice.c \
            $(SRCDIR)/lpc_analysis.c \
            $(SRCDIR)/lpc_gain_swb_tables.c \
            $(SRCDIR)/lpc_shape_swb12_tables.c \
            $(SRCDIR)/lpc_shape_swb16_tables.c \
            $(SRCDIR)/lpc_tables.c \
            $(SRCDIR)/lpc_to_refl_coef.c \
            $(SRCDIR)/pitch_estimator.c \
            $(SRCDIR)/pitch_filter.c \
            $(SRCDIR)/pitch_gain_tables.c \
            $(SRCDIR)/pitch_lag_tables.c \
            $(SRCDIR)/refl_coef_to_lpc.c \
            $(SRCDIR)/resample_fractional.c \
            $(SRCDIR)/spectrum_ar_model_tables.c \
            $(SRCDIR)/splitting_filter.c \
            $(SRCDIR)/transform.c \
            $(SRCDIR)/vector_scaling_operations.c \

PLUGIN_CPPFLAGS = -I$(SRCDIR)


INSTALL_DIR = $(AUD_PLUGIN_DIR)
include $(dir $(lastword $(MAKEFILE_LIST)))../../plugin_inc.mak


###########################################
