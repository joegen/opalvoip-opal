#
# Makefile
#
# Make file for opalgw
#
# Open Phone Abstraction Lebrary Project
#
# Copyright (c) 2003 Equivalence Pty. Ltd.
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
# $Log: Makefile,v $
# Revision 1.2  2003/03/26 03:03:52  robertj
# Set correct project name
#
# Revision 1.1  2003/03/26 02:49:00  robertj
# Added service/daemon sample application.
#

PROG	= opalgw
SOURCES	:= main.cxx custom.cxx

ifndef OPALDIR
ifneq (,$(wildcard ../opal))
OPALDIR=../opal
else
OPALDIR=$(HOME)/opal
endif
endif

include $(OPALDIR)/opal_inc.mak
