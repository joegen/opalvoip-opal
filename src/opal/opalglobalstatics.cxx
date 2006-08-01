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
 * Revision 1.2003  2006/08/01 12:46:32  rjongbloed
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

#include <opal/mediafmt.h>
#include <codec/opalwavfile.h>

#include <codec/opalpluginmgr.h>

#if defined(P_HAS_PLUGINS)
class PluginLoader : public PProcessStartup
{
  PCLASSINFO(PluginLoader, PProcessStartup);
  public:
    void OnStartup()
    { 
      OpalPluginCodecManager::Bootstrap(); 
      PWLibStupidLinkerHacks::opalwavfileLoader =1;
#ifndef NO_OPAL_VIDEO
#ifdef RFC2190_AVCODEC
      //PWLibStupidLinkerHacks::rfc2190h263Loader =1;
#endif
#endif
    }
};

static PFactory<PPluginModuleManager>::Worker<OpalPluginCodecManager> opalPluginCodecManagerFactory("OpalPluginCodecManager", true);
static PFactory<PProcessStartup>::Worker<PluginLoader> opalpluginStartupFactory("OpalPluginLoader", true);

#endif // P_HAS_PLUGINS

namespace PWLibStupidLinkerHacks {

int opalLoader;

#ifndef NO_H323_VIDEO
extern int h261Loader;
extern int rfc2190h263Loader;
#endif

#ifdef P_WAVFILE
extern int opalwavfileLoader;
#endif

}; // namespace PWLibStupidLinkerHacks

//////////////////////////////////
