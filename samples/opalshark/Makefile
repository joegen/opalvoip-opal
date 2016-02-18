#
# Makefile
#
# Make file for an OPAL analyser/player for RTP.
#
# Copyright (c) 2015 Vox Lucida
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
# The Original Code is Opal Shark.
#
# The Initial Developer of the Original Code is Robert Jongbloed
#
# Contributor(s): ______________________________________.
#
# $Revision$
# $Author$
# $Date$
#

PROG		:= opalshark
SOURCES		:= main.cxx opalshark.cxx
VERSION_FILE    := version.h
USING_WX_WIDGETS:=yes


# Need this before the include of opal.mak
.DEFAULT_GOAL := bundle


ifdef OPALDIR
  include $(OPALDIR)/make/opal.mak
else
  include $(shell pkg-config opal --variable=makedir)/opal.mak
endif


opalshark.cxx : opalshark.xrc
	$(WXRC) opalshark.xrc -c -o opalshark.cxx


