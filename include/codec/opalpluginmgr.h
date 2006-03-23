/*
 * opalpluginmgr.h
 *
 * OPAL codec plugins handler
 *
 * OPAL Library
 *
 * Copyright (C) 2005 Post Increment
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: opalpluginmgr.h,v $
 * Revision 1.1.2.2  2006/03/23 07:55:18  csoutheren
 * Audio plugin H.323 capability merging completed.
 * GSM, LBC, G.711 working. Speex and LPC-10 are not
 *
 * Revision 1.1.2.1  2006/03/16 07:06:00  csoutheren
 * Initial support for audio plugins
 *
 * Created from OpenH323 h323pluginmgr.h
 * Revision 1.24  2005/11/30 13:05:01  csoutheren
 */

#ifndef __OPALPLUGINMGR_H
#define __OPALPLUGINMGR_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/pluginmgr.h>
#include <ptlib/pfactory.h>
#include <opalplugin.h>
#include <opal/mediafmt.h>

#if OPAL_H323
#include <h323/h323caps.h>
#endif

class H323Capability;

class OPALPluginCodecManager : public PPluginModuleManager
{
  PCLASSINFO(OPALPluginCodecManager, PPluginModuleManager);
  public:
    OPALPluginCodecManager(PPluginManager * pluginMgr = NULL);
    ~OPALPluginCodecManager();

    void RegisterStaticCodec(const char * name,
                             PluginCodec_GetAPIVersionFunction getApiVerFn,
                             PluginCodec_GetCodecFunction getCodecFn);

    void OnLoadPlugin(PDynaLink & dll, INT code);

    static OpalMediaFormatList GetMediaFormats();

    virtual void OnShutdown();

    static void Bootstrap();

#if OPAL_H323
    H323Capability * CreateCapability(
          const PString & _mediaFormat, 
          const PString & _baseName,
                 unsigned maxFramesPerPacket, 
                 unsigned recommendedFramesPerPacket,
                 unsigned _pluginSubType);
#endif

  protected:
    void RegisterPluginPair(
      PluginCodec_Definition * _encoderCodec,
      PluginCodec_Definition * _decoderCodec
    );

    static void AddFormat(const OpalMediaFormat & fmt);
    static OpalMediaFormatList & GetMediaFormatList();
    static PMutex & GetMediaFormatMutex();

    void RegisterCodecPlugins  (unsigned int count, void * codecList);
    void UnregisterCodecPlugins(unsigned int count, void * codecList);
};

class OPALDynaLink : public PDynaLink
{
  PCLASSINFO(OPALDynaLink, PDynaLink)
    
 public:
  OPALDynaLink(const char * basename, const char * reason = NULL);

  virtual void Load();
  virtual BOOL IsLoaded()
  { PWaitAndSignal m(processLock); return isLoadedOK; }
  virtual BOOL LoadPlugin (const PString & fileName);

protected:
  PMutex processLock;
  BOOL isLoadedOK;
  const char * baseName;
  const char * reason;
};

//////////////////////////////////////////////////////
//
//  this is the base class for codecs accesible via the abstract factory functions
//

/**Class for codcs which is accessible via the abstract factory functions.
   The code would be :

      PFactory<OpalFactoryCodec>::CreateInstance(conversion);

  to create an instance, where conversion is (eg) "L16:G.711-uLaw-64k"
*/
class OpalFactoryCodec : public PObject {
  PCLASSINFO(OpalFactoryCodec, PObject)
  public:
  /** Return the PluginCodec_Definition, which describes this codec */
    virtual const struct PluginCodec_Definition * GetDefinition()
    { return NULL; }

  /** Return the sourceFormat field of PluginCodec_Definition for this codec*/
    virtual PString GetInputFormat() const = 0;

  /** Return the destFormat field of PluginCodec_Definition for this codec*/
    virtual PString GetOutputFormat() const = 0;

    /** Take the supplied data and apply the conversion specified by CreateInstance call (above). When this method returns, toLen contains the number of bytes placed in the destination buffer. */
    virtual int Encode(const void * from,      ///< pointer to the source data
                         unsigned * fromLen,   ///< number of bytes in the source data to process
                             void * to,        ///< pointer to the destination buffer, which contains the output of the  conversion
		                 unsigned * toLen,     ///< Number of available bytes in the destination buffer
                     unsigned int * flag       ///< Typically, this is not used.
		       ) = 0;  

    /** Return the  sampleRate field of PluginCodec_Definition for this codec*/
    virtual unsigned int GetSampleRate() const = 0;

    /** Return the  bitsPerSec field of PluginCodec_Definition for this codec*/
    virtual unsigned int GetBitsPerSec() const = 0;

    /** Return the  nmPerFrame field of PluginCodec_Definition for this codec*/
    virtual unsigned int GetFrameTime() const = 0;

    /** Return the samplesPerFrame  field of PluginCodec_Definition for this codec*/
    virtual unsigned int GetSamplesPerFrame() const = 0;

    /** Return the  bytesPerFrame field of PluginCodec_Definition for this codec*/
    virtual unsigned int GetBytesPerFrame() const = 0;

    /** Return the recommendedFramesPerPacket field of PluginCodec_Definition for this codec*/
    virtual unsigned int GetRecommendedFramesPerPacket() const = 0;

    /** Return the maxFramesPerPacket field of PluginCodec_Definition for this codec*/
    virtual unsigned int GetMaxFramesPerPacket() const = 0;

    /** Return the  rtpPayload field of PluginCodec_Definition for this codec*/
    virtual BYTE         GetRTPPayload() const = 0;

    /** Return the  sampleRate field of PluginCodec_Definition for this codec*/
    virtual PString      GetSDPFormat() const = 0;
};

#endif // __OPALPLUGINMGR_H
