/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
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
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

%ModuleHeaderCode
template <class C>
void PLoadPluginDirectory(C & obj, const PDirectory & directory, const char * suffix);
%End

//////////////////////////////////////////////////////
//
//  Ancestor Service descriptor for plugins
//

class PPluginServiceDescriptor 
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib\plugin.h>
%End
  public:
    PPluginServiceDescriptor();
    virtual ~PPluginServiceDescriptor();

    virtual unsigned GetPluginAPIVersion() const;
};


class PDevicePluginServiceDescriptor : PPluginServiceDescriptor /Abstract/
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib\plugin.h>
%End
  public:
    static const char SeparatorChar;

    virtual PObject *    CreateInstance(int userData) const = 0;
    virtual PStringArray GetDeviceNames(int userData) const = 0;
    virtual bool         ValidateDeviceName(const PString & deviceName, int userData) const;
    virtual bool         GetDeviceCapabilities(const PString & deviceName, void * capabilities) const;
};


//////////////////////////////////////////////////////
//
// Define a service provided by a plugin, which consists of the following:
//
//    serviceType - the base class name of the service which is used to identify
//                  the service type, such as PSoundChannel, 
//
//    serviceName - the name of the service provided by the plugin. This will usually
//                  be related to the class implementing the service, such as:
//                       service name = OSS, class name = PSoundChannelOSS
//
//    descriptor  - a pointer to a class containing pointers to any static functions
//                  for this class
//   
//

class PPluginService: PObject
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib\pluginmgr.h>
%End
  public:
    PPluginService(const PString & name,
                   const PString & type,
                   PPluginServiceDescriptor * desc);

    PString serviceName;
    PString serviceType;
    PPluginServiceDescriptor * descriptor;
};


//////////////////////////////////////////////////////
//
//  Manager for plugins
//

class PPluginManager : PObject
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib\pluginmgr.h>
%End
  public:
    // functions to load/unload a dynamic plugin 
    PBoolean LoadPlugin (const PString & fileName);
    void LoadPluginDirectory (const PDirectory & dir);
  
    // functions to access the plugins' services 
    PStringArray GetPluginTypes() const;
    PStringArray GetPluginsProviding(const PString & serviceType) const;
    PPluginServiceDescriptor * GetServiceDescriptor(const PString & serviceName, const PString & serviceType) const;
    PObject * CreatePluginsDevice(const PString & serviceName, const PString & serviceType, int userData = 0) const;
    PObject * CreatePluginsDeviceByName(const PString & deviceName, const PString & serviceType, int userData = 0, const PString & serviceName = PString::Empty()) const;
    PStringArray GetPluginsDeviceNames(const PString & serviceName, const PString & serviceType, int userData = 0) const;
    PBoolean GetPluginsDeviceCapabilities(const PString & serviceType,const PString & serviceName,const PString & deviceName,void * capabilities) const;

    // function to register a service (used by the plugins themselves)
    PBoolean RegisterService (const PString & serviceName, const PString & serviceType, PPluginServiceDescriptor * descriptor);

    // Add a directory to the list of plugin directories (used by OPAL)
    static void AddPluginDirs(PString dirs);

    // Get the list of plugin directories
    static PStringArray GetPluginDirs();

    // static functions for accessing global instances of plugin managers
    static PPluginManager & GetPluginManager();
};
