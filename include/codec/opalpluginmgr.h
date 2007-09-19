/*
 * opalpluginmgr.h
 *
 * OPAL codec plugins handler
 *
 * OPAL Library
 *
 * Copyright (C) 2005-2006 Post Increment
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: opalpluginmgr.h,v $
 * Revision 1.2005.4.2  2007/09/19 12:05:18  csoutheren
 * Exposed G.7231 capability class
 * Added macros to create empty transcoders and capabilities
 * (backport from head)
 *
 * Revision 2.4.4.1  2007/08/15 06:21:39  csoutheren
 * Fix link problem on old compilers caused by PWLib change
 *
 * Revision 2.4  2006/10/02 13:30:50  rjongbloed
 * Added LID plug ins
 *
 * Revision 2.3  2006/09/28 07:42:14  csoutheren
 * Merge of useful SRTP implementation
 *
 * Revision 2.2  2006/08/11 07:52:01  csoutheren
 * Fix problem with media format factory in VC 2005
 * Fixing problems with Speex codec
 * Remove non-portable usages of PFactory code
 *
 * Revision 2.1  2006/07/24 14:03:38  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 1.1.2.3  2006/04/06 01:21:16  csoutheren
 * More implementation of video codec plugins
 *
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
#include <codec/opalplugin.h>
#include <opal/mediafmt.h>

#if OPAL_H323
#include <h323/h323caps.h>
#endif

class H323Capability;

class H323StaticPluginCodec
{
  public:
    virtual ~H323StaticPluginCodec() { }
    virtual PluginCodec_GetAPIVersionFunction Get_GetAPIFn() = 0;
    virtual PluginCodec_GetCodecFunction Get_GetCodecFn() = 0;
};

typedef PFactory<H323StaticPluginCodec> H323StaticPluginCodecFactory;

class OpalPluginCodecManager : public PPluginModuleManager
{
  PCLASSINFO(OpalPluginCodecManager, PPluginModuleManager);
  public:
    OpalPluginCodecManager(PPluginManager * pluginMgr = NULL);
    ~OpalPluginCodecManager();

    void RegisterStaticCodec(const H323StaticPluginCodecFactory::Key_T & name,
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

#if OPAL_H323
    void RegisterCapability(PluginCodec_Definition * encoderCodec, PluginCodec_Definition * decoderCodec);
    struct CapabilityListCreateEntry {
      CapabilityListCreateEntry(PluginCodec_Definition * e, PluginCodec_Definition * d)
        : encoderCodec(e), decoderCodec(d) { }
      PluginCodec_Definition * encoderCodec;
      PluginCodec_Definition * decoderCodec;
    };
    typedef vector<CapabilityListCreateEntry> CapabilityCreateListType;
    CapabilityCreateListType capabilityCreateList;
#endif
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

#if OPAL_H323

//////////////////////////////////////////////////////////////////////////////
//
// Helper class for handling plugin capabilities
//

class H323PluginCapabilityInfo
{
  public:
    H323PluginCapabilityInfo(PluginCodec_Definition * _encoderCodec,
                             PluginCodec_Definition * _decoderCodec);

    H323PluginCapabilityInfo(const PString & _baseName);

    const PString & GetFormatName() const
    { return capabilityFormatName; }

  protected:
    PluginCodec_Definition * encoderCodec;
    PluginCodec_Definition * decoderCodec;
    PString                  capabilityFormatName;
};

class H323AudioPluginCapability : public H323AudioCapability,
                                  public H323PluginCapabilityInfo
{
  PCLASSINFO(H323AudioPluginCapability, H323AudioCapability);
  public:
    H323AudioPluginCapability(PluginCodec_Definition * _encoderCodec,
                         PluginCodec_Definition * _decoderCodec,
                         unsigned _pluginSubType);

    // this constructor is only used when creating a capability without a codec
    H323AudioPluginCapability(const PString & _mediaFormat, const PString & _baseName, unsigned _type);

    virtual PObject * Clone() const;
    virtual PString GetFormatName() const;
    virtual unsigned GetSubType() const;

  protected:
    unsigned pluginSubType;
    //unsigned h323subType;   // only set if using capability without codec
};

#define OPAL_DECLARE_EMPTY_AUDIO_CAPABILITY(fmt, type) \
class fmt##_CapabilityRegisterer { \
  public: \
    fmt##_CapabilityRegisterer() \
    { H323CapabilityFactory::Register(fmt, new H323AudioPluginCapability(fmt, fmt, type)); } \
}; \

#define OPAL_DEFINE_EMPTY_AUDIO_CAPABILITY(fmt) \
static fmt##_CapabilityRegisterer fmt##_CapabilityRegisterer_instance; \

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling G.723.1 codecs
//

class H323PluginG7231Capability : public H323AudioPluginCapability
{
  PCLASSINFO(H323PluginG7231Capability, H323AudioPluginCapability);
  public:
    H323PluginG7231Capability(PluginCodec_Definition * _encoderCodec,
                               PluginCodec_Definition * _decoderCodec,
                               BOOL _annexA = TRUE);

    H323PluginG7231Capability(const OpalMediaFormat & fmt, BOOL _annexA = TRUE);

    Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
    virtual BOOL OnSendingPDU(H245_AudioCapability & cap, unsigned packetSize) const;
    virtual BOOL OnReceivedPDU(const H245_AudioCapability & cap,  unsigned & packetSize);

  protected:
    BOOL annexA;
};

#define OPAL_DECLARE_EMPTY_G7231_CAPABILITY(fmt, annex) \
class fmt##_CapabilityRegisterer { \
  public: \
    fmt##_CapabilityRegisterer() \
    { H323CapabilityFactory::Register(fmt, new H323PluginG7231Capability(fmt, annex)); } \
}; \

#define OPAL_DEFINE_EMPTY_G7231_CAPABILITY(fmt) \
static fmt##_CapabilityRegisterer fmt##_CapabilityRegisterer_instance; \


#endif // OPAL_H323

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
