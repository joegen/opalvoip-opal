#
# opal_inc.mak
#
# Default build environment for OPAL applications
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
# $Revision: 20959 $
# $Author: ms30002000 $
# $Date: 2008-09-15 03:57:09 +1000 (Mon, 15 Sep 2008) $
#


# This file is for backward compatibility with old OPAL
# applications that are not pkg-config aware, or ones
# that are just too lazy to write their own. :-)

  opt ::
	@true


ifdef DEBUG
  DEBUG_BUILD:=yes
endif
 
ifeq ($(P_SHARELIB),0)
  OPAL_SHARED_LIB:=no
endif


ifdef OPALDIR
  include $(OPALDIR)/make/opal_defs.mak
  LIBDIRS  += $(OPALDIR)
else
  include $(shell pkg-config opal --variable=makedir)/opal_defs.mak
endif


ifdef PTLIBDIR
  include $(PTLIBDIR)/make/ptlib.mak
else
  include $(shell pkg-config ptlib --variable=makedir)/ptlib.mak
endif


LDLIBS := -l$(LIB_NAME)$(LIBTYPE) $(LDLIBS)

$(TARGET) : $(OPAL_LIBDIR)/$(OPAL_FILE)


# End of file
