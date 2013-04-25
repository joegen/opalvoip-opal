#					
# Makefile for Silk SDK			
#
# Copyright (c) 2010, Skype Limited
# All rights reserved.
#

#Platform detection and settings

EXESUFFIX = 
LIBPREFIX = lib
LIBSUFFIX = .a
OBJSUFFIX = .o

ifdef target
  OBJDIR=obj_$(target)/
endif

ifndef CC
  CC = $(TOOLCHAIN_PREFIX)gcc$(TOOLCHAIN_SUFFIX)
endif

ifndef AR
  AR = $(TOOLCHAIN_PREFIX)ar
endif

ifndef RANLIB
  RANLIB = $(TOOLCHAIN_PREFIX)ranlib
endif

ifndef CP
  CP = $(TOOLCHAIN_PREFIX)cp
endif

cflags-from-defines    = $(addprefix -D,$(1))
cflags-from-includes   = $(addprefix -I,$(1))
ldflags-from-ldlibdirs = $(addprefix -L,$(1))
ldlibs-from-libs       = $(addprefix -l,$(1))

CFLAGS	+= -Wall -enable-threads -O3 -fPIC

CFLAGS  += $(call cflags-from-defines,$(CDEFINES))
CFLAGS  += $(call cflags-from-defines,$(ADDED_DEFINES))
CFLAGS  += $(call cflags-from-includes,$(CINCLUDES))
LDFLAGS += $(call ldflags-from-ldlibdirs,$(LDLIBDIRS))
LDLIBS  += $(call ldlibs-from-libs,$(LIBS))

COMPILE.c.cmdline   = $(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<
LINK.o.cmdline      = $(LINK.o) $^ -lm $(LDLIBS) -o $@$(EXESUFFIX) 
ARCHIVE.cmdline     = $(AR) $(ARFLAGS) $@ $^ && $(RANLIB) $@

$(OBJDIR)%$(OBJSUFFIX):%.c
ifdef OBJDIR
	mkdir -p $(dir $@)
endif
	$(COMPILE.c.cmdline)

# Directives

CINCLUDES += interface src test

# VPATH e.g. VPATH = src:../headers
VPATH = ./ \
        interface \
        src \
        test \
        $(OBJDIR)

# Variable definitions
LIB_NAME = SKP_SILK_SDK
TARGET = $(OBJDIR)$(LIBPREFIX)$(LIB_NAME)$(LIBSUFFIX)

SRCS_C = $(wildcard src/*.c)

OBJS := $(patsubst src/%,$(OBJDIR)%,$(patsubst %.c,%$(OBJSUFFIX),$(SRCS_C)))

ENCODER_SRCS_C = test/Encoder.c
ENCODER_OBJS := $(patsubst test/%,$(OBJDIR)%,$(patsubst %.c,%$(OBJSUFFIX),$(ENCODER_SRCS_C)))

DECODER_SRCS_C = test/Decoder.c
DECODER_OBJS := $(patsubst test/%,$(OBJDIR)%,$(patsubst %.c,%$(OBJSUFFIX),$(DECODER_SRCS_C)))

SIGNALCMP_SRCS_C = test/signalCompare.c
SIGNALCMP_OBJS := $(patsubst test/%,$(OBJDIR)%,$(patsubst %.c,%$(OBJSUFFIX),$(SIGNALCMP_SRCS_C)))

LIBS = \
	$(LIB_NAME)

LDLIBDIRS = ./

# Rules
default: lib # all

all: $(TARGET) encoder decoder signalcompare

lib: $(TARGET)

$(TARGET): $(OBJS)
	$(ARCHIVE.cmdline)


encoder$(EXESUFFIX): $(TARGET) $(ENCODER_OBJS)	
	$(LINK.o.cmdline)

decoder$(EXESUFFIX): $(TARGET) $(DECODER_OBJS)	
	$(LINK.o.cmdline)

signalcompare$(EXESUFFIX): $(TARGET) $(SIGNALCMP_OBJS)	
	$(LINK.o.cmdline)

clean:
	$(RM) $(TARGET)* $(OBJS) $(ENCODER_OBJS) $(DECODER_OBJS) \
		  $(SIGNALCMP_OBJS) $(TEST_OBJS) \
		  encoder$(EXESUFFIX) decoder$(EXESUFFIX) signalcompare$(EXESUFFIX)
ifdef OBJDIR
	rmdir $(OBJDIR)
endif

install uninstall:
	@true
