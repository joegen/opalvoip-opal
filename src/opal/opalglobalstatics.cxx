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
 * $Revision$
 * $Author$
 * $Date$
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
      PWLibStupidLinkerHacks::mediaTypeLoader = 1;
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
