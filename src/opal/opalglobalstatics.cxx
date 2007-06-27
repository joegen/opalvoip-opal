/*
 * opalglobalstatics.cxx
 *
 * Various global statics that need to be instantiated upon startup
 *
 * Portable Windows Library
 *
 * Copyright (C) 2004 Post Increment
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: opalglobalstatics.cxx,v $
 * Revision 1.2010  2007/06/27 18:19:49  csoutheren
 * Fix compile when video disabled
 *
 * Revision 2.8  2007/05/31 14:11:45  csoutheren
 * Add initial support for RFC 4175 uncompressed video encoding
 *
 * Revision 2.7  2007/03/29 05:20:17  csoutheren
 * Implement T.38 and fax
 *
 * Revision 2.6  2006/11/20 03:56:37  csoutheren
 * Fix usage of define
 *
 * Revision 2.5  2006/10/05 07:11:49  csoutheren
 * Add --disable-lid option
 *
 * Revision 2.4  2006/10/02 13:30:52  rjongbloed
 * Added LID plug ins
 *
 * Revision 2.3  2006/09/28 07:42:18  csoutheren
 * Merge of useful SRTP implementation
 *
 * Revision 2.2  2006/08/01 12:46:32  rjongbloed
 * Added build solution for plug ins
 * Removed now redundent code due to plug ins addition
 *
 * Revision 2.1  2006/07/24 14:03:40  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 1.1.2.1  2006/04/08 04:35:15  csoutheren
 * Initial version
 *
 *
 */

#include <ptlib.h>

#include <opal/buildopts.h>

#include <opal/mediafmt.h>
#include <codec/opalwavfile.h>

#include <codec/opalpluginmgr.h>
#include <lids/lidpluginmgr.h>
#include <rtp/srtp.h>
#include <t38/t38proto.h>

#if OPAL_RFC4175
#include <codec/rfc4175.h>
#endif

#if defined(P_HAS_PLUGINS)
class PluginLoader : public PProcessStartup
{
  PCLASSINFO(PluginLoader, PProcessStartup);
  public:
    void OnStartup()
    { 
      OpalPluginCodecManager::Bootstrap(); 
      PWLibStupidLinkerHacks::opalwavfileLoader =1;
#if HAS_LIBSRTP
      PWLibStupidLinkerHacks::libSRTPLoader = 1;
#endif
#if OPAL_T38FAX
      PWLibStupidLinkerHacks::t38Loader = 1;
#endif
#if OPAL_RFC4175
      PWLibStupidLinkerHacks::rfc4175Loader = 1;
#endif
    }
};

static PFactory<PPluginModuleManager>::Worker<OpalPluginCodecManager> opalPluginCodecManagerFactory("OpalPluginCodecManager", true);
#if OPAL_LID
static PFactory<PPluginModuleManager>::Worker<OpalPluginLIDManager> opalPluginLIDManagerFactory("OpalPluginLIDManager", true);
#endif
static PFactory<PProcessStartup>::Worker<PluginLoader> opalpluginStartupFactory("OpalPluginLoader", true);

#endif // P_HAS_PLUGINS

namespace PWLibStupidLinkerHacks {

int opalLoader;

#ifdef P_WAVFILE
extern int opalwavfileLoader;
#endif

}; // namespace PWLibStupidLinkerHacks

//////////////////////////////////
