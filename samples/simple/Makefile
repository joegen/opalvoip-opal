#
# Makefile
#
# Make file for simple sample application for the Open Phone Abstraction library.
#
# Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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
# Revision 1.2003  2002/02/11 09:38:28  robertj
# Moved version to root directory
#
# Revision 2.1  2001/07/30 03:07:15  robertj
# Improved reporting of incorrect OPALDIR
#
# Revision 2.0  2001/07/27 15:48:24  robertj
# Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
#
# Revision 1.2  2000/10/30 00:24:49  robertj
# No longer need to include ptlib explicitly
#
# Revision 1.1  2000/05/11 04:05:57  robertj
# Simple sample program.
#

PROG		= simpleopal
SOURCES		:= main.cxx

VERSION_FILE =  ../../version.h

ifndef OPALDIR
ifneq (,$(wildcard $(HOME)/opal))
OPALDIR=$(HOME)/opal
else
ifneq (,$(wildcard /usr/local/opal))
OPALDIR=/usr/local/opal
else
default_target :
	@echo Cannot find OPAL in standard locations, you must set the OPALDIR
	@echo environment variable to build this application.
endif
endif
endif

ifdef OPALDIR
include $(OPALDIR)/opal_inc.mak
endif

