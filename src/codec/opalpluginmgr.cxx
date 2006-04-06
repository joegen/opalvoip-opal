/*
 * h323plugins.cxx
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
 * $Log: opalpluginmgr.cxx,v $
 * Revision 1.1.2.4  2006/04/06 01:21:18  csoutheren
 * More implementation of video codec plugins
 *
 * Revision 1.1.2.3  2006/03/23 07:55:18  csoutheren
 * Audio plugin H.323 capability merging completed.
 * GSM, LBC, G.711 working. Speex and LPC-10 are not
 *
 * Revision 1.1.2.2  2006/03/20 05:03:23  csoutheren
 * Changes to make audio plugins work with SIP
 *
 * Revision 1.1.2.1  2006/03/16 07:06:00  csoutheren
 * Initial support for audio plugins
 *
 * Created from OpenH323 h323pluginmgr.cxx
 * Revision 1.58  2005/08/05 17:11:03  csoutheren
 *
 */

#ifdef __GNUC__
#pragma implementation "opalpluginmgr.h"
#endif

#include <ptlib.h>

#include <opal/transcoders.h>
#include <codec/opalpluginmgr.h>
#include <codec/opalplugin.h>
#include <h323/h323caps.h>
#include <asn/h245.h>

#if OPAL_VIDEO
#include <codec/vidcodec.h>
#endif

/////////////////////////////////////////////////////////////////////////////

template <typename CodecClass>
class OpalFixedCodecFactory : public PFactory<OpalFactoryCodec>
{
  public:
    class Worker : public PFactory<OpalFactoryCodec>::WorkerBase 
    {
      public:
        Worker(const PString & key)
          : PFactory<OpalFactoryCodec>::WorkerBase()
        { PFactory<OpalFactoryCodec>::Register(key, this); }

      protected:
        virtual OpalFactoryCodec * Create(const PString &) const
        { return new CodecClass(); }
    };
};


static PString CreateCodecName(PluginCodec_Definition * codec, BOOL /*addSW*/)
{
  PString str;
  if (codec->destFormat != NULL)
    str = codec->destFormat;
  else
    str = PString(codec->descr);
  return str;
}

static PString CreateCodecName(const PString & baseName, BOOL /*addSW*/)
{
  return baseName;
}

#if OPAL_AUDIO

class OpalPluginAudioMediaFormat : public OpalAudioFormat
{
  public:
    friend class OpalPluginCodecManager;

    OpalPluginAudioMediaFormat(
      PluginCodec_Definition * _encoderCodec,
      const char * rtpEncodingName, /// rtp encoding name
      unsigned frameTime,           /// Time for frame in RTP units (if applicable)
      unsigned timeUnits,           /// RTP units for frameTime (if applicable)
      time_t timeStamp              /// timestamp (for versioning)
    )
    : OpalAudioFormat(
      CreateCodecName(_encoderCodec, FALSE),
      (RTP_DataFrame::PayloadTypes)(((_encoderCodec->flags & PluginCodec_RTPTypeMask) == PluginCodec_RTPTypeDynamic) ? RTP_DataFrame::DynamicBase : _encoderCodec->rtpPayload),
      rtpEncodingName,
      _encoderCodec->bitsPerSec,
      _encoderCodec->bytesPerFrame,
      frameTime,
      timeUnits,
      timeStamp  
    )
    , encoderCodec(_encoderCodec)
    {
      // manually register the new singleton type, as we do not have a concrete type
      OpalMediaFormatFactory::Register(*this, this);
    }
    ~OpalPluginAudioMediaFormat()
    {
      OpalMediaFormatFactory::Unregister(*this);
    }
    PluginCodec_Definition * encoderCodec;
};

static H323Capability * CreateG7231Cap(
  PluginCodec_Definition * encoderCodec, 
  PluginCodec_Definition * decoderCodec,
  int subType
);

static H323Capability * CreateGenericAudioCap(
  PluginCodec_Definition * encoderCodec, 
  PluginCodec_Definition * decoderCodec,
  int subType
);

static H323Capability * CreateNonStandardAudioCap(
  PluginCodec_Definition * encoderCodec, 
  PluginCodec_Definition * decoderCodec,
  int subType
);

static H323Capability * CreateGSMCap(
  PluginCodec_Definition * encoderCodec, 
  PluginCodec_Definition * decoderCodec,
  int subType
);

#endif // OPAL_AUDIO

#if OPAL_VIDEO

class OpalPluginVideoMediaFormat : public OpalVideoFormat
{
  public:
    friend class OpalPluginCodecManager;

    OpalPluginVideoMediaFormat(
      PluginCodec_Definition * _encoderCodec,
      const char * rtpEncodingName, /// rtp encoding name
      time_t timeStamp              /// timestamp (for versioning)
    )
    : OpalVideoFormat(
      CreateCodecName(_encoderCodec, FALSE),
      (RTP_DataFrame::PayloadTypes)(((_encoderCodec->flags & PluginCodec_RTPTypeMask) == PluginCodec_RTPTypeDynamic) ? RTP_DataFrame::DynamicBase : _encoderCodec->rtpPayload),
      rtpEncodingName,
      _encoderCodec->samplesPerFrame,      // actually frame width
      _encoderCodec->bytesPerFrame,        // actually frame height,
      _encoderCodec->maxFramesPerPacket,   // actually max frameRate
      _encoderCodec->bitsPerSec,
      timeStamp  
    )
    , encoderCodec(_encoderCodec)
    {
      // manually register the new singleton type, as we do not have a concrete type
      OpalMediaFormatFactory::Register(*this, this);
    }
    ~OpalPluginVideoMediaFormat()
    {
      OpalMediaFormatFactory::Unregister(*this);
    }
    PluginCodec_Definition * encoderCodec;
};

static H323Capability * CreateNonStandardVideoCap(
  PluginCodec_Definition * encoderCodec, 
  PluginCodec_Definition * decoderCodec,
  int subType
);

static H323Capability * CreateGenericVideoCap(
  PluginCodec_Definition * encoderCodec, 
  PluginCodec_Definition * decoderCodec,
  int subType
);

static H323Capability * CreateH261Cap(
  PluginCodec_Definition * encoderCodec, 
  PluginCodec_Definition * decoderCodec,
  int subType
);

//static H323Capability * CreateH263Cap(
//  PluginCodec_Definition * encoderCodec, 
//  PluginCodec_Definition * decoderCodec,
//  int subType
//);

#endif // OPAL_VIDEO


/*
//////////////////////////////////////////////////////////////////////////////
//
// Class to auto-register plugin capabilities
//

class H323CodecPluginCapabilityRegistration : public PObject
{
  public:
    H323CodecPluginCapabilityRegistration(
       PluginCodec_Definition * _encoderCodec,
       PluginCodec_Definition * _decoderCodec
    );

    H323Capability * Create(H323EndPoint & ep) const;
  
    static H323Capability * CreateG7231Cap           (H323EndPoint & ep, int subType) const;
    static H323Capability * CreateNonStandardAudioCap(H323EndPoint & ep, int subType) const;
    //H323Capability * CreateNonStandardVideoCap(H323EndPoint & ep, int subType) const;
    static H323Capability * CreateGSMCap             (H323EndPoint & ep, int subType) const;
    static H323Capability * CreateH261Cap            (H323EndPoint & ep, int subType) const;

  protected:
    PluginCodec_Definition * encoderCodec;
    PluginCodec_Definition * decoderCodec;
};

*/

////////////////////////////////////////////////////////////////////

#if OPAL_H323

class H323CodecPluginCapabilityMapEntry {
  public:
    int pluginCapType;
    int h323SubType;
    H323Capability * (* createFunc)(PluginCodec_Definition * encoderCodec, PluginCodec_Definition * decoderCodec, int subType);
};

#if OPAL_AUDIO

static H323CodecPluginCapabilityMapEntry audioMaps[] = {
  { PluginCodec_H323Codec_nonStandard,              H245_AudioCapability::e_nonStandard,         &CreateNonStandardAudioCap },
  { PluginCodec_H323AudioCodec_gsmFullRate,	        H245_AudioCapability::e_gsmFullRate,         &CreateGSMCap },
  { PluginCodec_H323AudioCodec_gsmHalfRate,	        H245_AudioCapability::e_gsmHalfRate,         &CreateGSMCap },
  { PluginCodec_H323AudioCodec_gsmEnhancedFullRate, H245_AudioCapability::e_gsmEnhancedFullRate, &CreateGSMCap },
  { PluginCodec_H323AudioCodec_g711Alaw_64k,        H245_AudioCapability::e_g711Alaw64k },
  { PluginCodec_H323AudioCodec_g711Alaw_56k,        H245_AudioCapability::e_g711Alaw56k },
  { PluginCodec_H323AudioCodec_g711Ulaw_64k,        H245_AudioCapability::e_g711Ulaw64k },
  { PluginCodec_H323AudioCodec_g711Ulaw_56k,        H245_AudioCapability::e_g711Ulaw56k },
  { PluginCodec_H323AudioCodec_g7231,               H245_AudioCapability::e_g7231,               &CreateG7231Cap },
  { PluginCodec_H323AudioCodec_g729,                H245_AudioCapability::e_g729 },
  { PluginCodec_H323AudioCodec_g729AnnexA,          H245_AudioCapability::e_g729AnnexA },
  { PluginCodec_H323AudioCodec_g728,                H245_AudioCapability::e_g728 }, 
  { PluginCodec_H323AudioCodec_g722_64k,            H245_AudioCapability::e_g722_64k },
  { PluginCodec_H323AudioCodec_g722_56k,            H245_AudioCapability::e_g722_56k },
  { PluginCodec_H323AudioCodec_g722_48k,            H245_AudioCapability::e_g722_48k },
  { PluginCodec_H323AudioCodec_g729wAnnexB,         H245_AudioCapability::e_g729wAnnexB }, 
  { PluginCodec_H323AudioCodec_g729AnnexAwAnnexB,   H245_AudioCapability::e_g729AnnexAwAnnexB },
  { PluginCodec_H323Codec_generic,                  H245_AudioCapability::e_genericAudioCapability, &CreateGenericAudioCap },

  // not implemented
  //{ PluginCodec_H323AudioCodec_g729Extensions,      H245_AudioCapability::e_g729Extensions,   0 },
  //{ PluginCodec_H323AudioCodec_g7231AnnexC,         H245_AudioCapability::e_g7231AnnexCMode   0 },
  //{ PluginCodec_H323AudioCodec_is11172,             H245_AudioCapability::e_is11172AudioMode, 0 },
  //{ PluginCodec_H323AudioCodec_is13818Audio,        H245_AudioCapability::e_is13818AudioMode, 0 },

  { -1 }
};

#endif // OPAL_AUDIO

#if OPAL_VIDEO

static H323CodecPluginCapabilityMapEntry videoMaps[] = {
  // video codecs
  { PluginCodec_H323Codec_nonStandard,              H245_VideoCapability::e_nonStandard,            &CreateNonStandardVideoCap },
  { PluginCodec_H323VideoCodec_h261,                H245_VideoCapability::e_h261VideoCapability,    &CreateH261Cap },
  //{ PluginCodec_H323VideoCodec_h263,                H245_VideoCapability::e_h263VideoCapability,    &CreateH263Cap },
  { PluginCodec_H323Codec_generic,                  H245_AudioCapability::e_genericAudioCapability, &CreateGenericVideoCap },
/*
  PluginCodec_H323VideoCodec_h262,                // not yet implemented
  PluginCodec_H323VideoCodec_is11172,             // not yet implemented
*/

  { -1 }
};

#endif  // OPAL_VIDEO

#endif // OPAL_H323

//////////////////////////////////////////////////////////////////////////////

template<class TranscoderClass>
class OpalPluginTranscoderFactory : public OpalTranscoderFactory
{
  public:
    class Worker : public OpalTranscoderFactory::WorkerBase 
    {
      public:
        Worker(const OpalMediaFormatPair & key, PluginCodec_Definition * _codecDefn, BOOL _isEncoder)
          : OpalTranscoderFactory::WorkerBase(TRUE), codecDefn(_codecDefn), isEncoder(_isEncoder)
        { OpalTranscoderFactory::Register(key, this); }

      protected:
        virtual OpalTranscoder * Create(const OpalMediaFormatPair &) const
        { return new TranscoderClass(codecDefn, isEncoder); }

        PluginCodec_Definition * codecDefn;
        BOOL isEncoder;
    };
};

//////////////////////////////////////////////////////////////////////////////
//
// Plugin framed audio codec classes
//

#if OPAL_AUDIO
class OpalPluginFramedAudioTranscoder : public OpalFramedTranscoder
{
  PCLASSINFO(OpalPluginFramedAudioTranscoder, OpalFramedTranscoder);
  public:
    OpalPluginFramedAudioTranscoder(PluginCodec_Definition * _codec, BOOL _isEncoder)
      : OpalFramedTranscoder( (strcmp(_codec->sourceFormat, "L16") == 0) ? "PCM-16" : _codec->sourceFormat,
                              (strcmp(_codec->destFormat, "L16") == 0)   ? "PCM-16" : _codec->destFormat,
                             _isEncoder ? _codec->samplesPerFrame*2 : _codec->bytesPerFrame,
                             _isEncoder ? _codec->bytesPerFrame     : _codec->samplesPerFrame*2),
        codec(_codec), isEncoder(_isEncoder)
    { 
      if (codec != NULL && codec->createCodec != NULL) 
        context = (*codec->createCodec)(codec); 
      else 
        context = NULL; 
    }

    ~OpalPluginFramedAudioTranscoder()
    { 
      if (codec != NULL && codec->destroyCodec != NULL) 
        (*codec->destroyCodec)(codec, context); 
    }

    BOOL ConvertFrame(
      const BYTE * input,   ///<  Input data
      PINDEX & consumed,    ///<  number of input bytes consumed
      BYTE * output,        ///<  Output data
      PINDEX & created      ///<  number of output bytes created  
    )
    {
      if (codec == NULL || codec->codecFunction == NULL)
        return FALSE;

      unsigned int fromLen = consumed;
      unsigned int toLen   = created;
      unsigned flags = 0;

      BOOL stat = (codec->codecFunction)(codec, context, 
                                   (const unsigned char *)input, &fromLen,
                                   output, &toLen,
                                   &flags) != 0;
      consumed = fromLen;
      created  = toLen;

      return stat;
    };

    virtual BOOL ConvertSilentFrame(BYTE * buffer)
    { 
      if (codec == NULL || isEncoder)
        return FALSE;

      unsigned int length = codec->bytesPerFrame;

      if ((codec->flags & PluginCodec_DecodeSilence) == 0)
        memset(buffer, 0, length); 
      else {
        unsigned flags = PluginCodec_CoderSilenceFrame;
        (codec->codecFunction)(codec, context, 
                               NULL, NULL,
                               buffer, &length,
                               &flags);
      }

      return TRUE;
    }

  protected:
    void * context;
    PluginCodec_Definition * codec;
    BOOL isEncoder;
};

//////////////////////////////////////////////////////////////////////////////
//
// Plugin streamed audio codec classes
//

class OpalPluginStreamedAudioTranscoder : public OpalStreamedTranscoder
{
  PCLASSINFO(OpalPluginStreamedAudioTranscoder, OpalStreamedTranscoder);
  public:
    OpalPluginStreamedAudioTranscoder(PluginCodec_Definition * _codec, unsigned inputBits, unsigned outputBits, PINDEX optimalBits)
      : OpalStreamedTranscoder((strcmp(_codec->sourceFormat, "L16") == 0) ? "PCM-16" : _codec->sourceFormat,
                               (strcmp(_codec->destFormat, "L16") == 0) ? "PCM-16" : _codec->destFormat,
                               inputBits, outputBits, optimalBits), 
        codec(_codec)
    { 
      if (codec != NULL && codec->createCodec != NULL) 
        context = (*codec->createCodec)(codec); 
      else 
        context = NULL; 
    }

    ~OpalPluginStreamedAudioTranscoder()
    { 
      if (codec != NULL && codec->destroyCodec != NULL) 
        (*codec->destroyCodec)(codec, context); 
    }

  protected:
    void * context;
    PluginCodec_Definition * codec;
};

class OpalPluginStreamedAudioEncoder : public OpalPluginStreamedAudioTranscoder
{
  PCLASSINFO(OpalPluginStreamedAudioEncoder, OpalPluginStreamedAudioTranscoder);
  public:
    OpalPluginStreamedAudioEncoder(PluginCodec_Definition * _codec, BOOL)
      : OpalPluginStreamedAudioTranscoder(_codec, 16, (_codec->flags & PluginCodec_BitsPerSampleMask) >> PluginCodec_BitsPerSamplePos, _codec->recommendedFramesPerPacket)
    {
    }

    int ConvertOne(int _sample) const
    {
      if (codec == NULL || codec->codecFunction == NULL)
        return 0;

      short sample = (short)_sample;
      unsigned int fromLen = sizeof(sample);
      int to;
      unsigned toLen = sizeof(to);
      unsigned flags = 0;
      (codec->codecFunction)(codec, context, 
                             (const unsigned char *)&sample, &fromLen,
                             (unsigned char *)&to, &toLen,
                             &flags);
      return to;
    }
};

class OpalPluginStreamedAudioDecoder : public OpalPluginStreamedAudioTranscoder
{
  PCLASSINFO(OpalPluginStreamedAudioDecoder, OpalPluginStreamedAudioTranscoder);
  public:
    OpalPluginStreamedAudioDecoder(PluginCodec_Definition * _codec, BOOL)
      : OpalPluginStreamedAudioTranscoder(_codec, (_codec->flags & PluginCodec_BitsPerSampleMask) >> PluginCodec_BitsPerSamplePos, 16, _codec->recommendedFramesPerPacket)
    {
    }

    int ConvertOne(int codedSample) const
    {
      if (codec == NULL || codec->codecFunction == NULL)
        return 0;

      unsigned int fromLen = sizeof(codedSample);
      short to;
      unsigned toLen = sizeof(to);
      unsigned flags = 0;
      (codec->codecFunction)(codec, context, 
                             (const unsigned char *)&codedSample, &fromLen,
                             (unsigned char *)&to, &toLen,
                             &flags);
      return to;
    }
};



#endif //  OPAL_AUDIO

//////////////////////////////////////////////////////////////////////////////
//
// Plugin video codec class
//

static BOOL CallCodecControl(PluginCodec_Definition * codec, 
                                               void * context,
                                         const char * name,
                                               void * parm, 
                                       unsigned int * parmLen
                                                int & retVal)
{
  PluginCodec_ControlDefn * codecControls = codec->codecControls;
  if (codecControls == NULL)
    return FALSE;

  while (codecControls->name != NULL) {
    if (strcmp(codecControls->name, name) == 0) {
      retVal = (*codecControls->control)(codec, context, name, parm, parmLen);
      return TRUE;
    }
    codecControls++;
  }

  return FALSE;
}

#if OPAL_VIDEO

/////////////////////////////////////////////////////////////////////////////

class OpalPluginVideoTranscoder : public OpalVideoTranscoder
{
  PCLASSINFO(OpalPluginVideoTranscoder, OpalVideoTranscoder);
  public:
    OpalPluginVideoTranscoder(PluginCodec_Definition * _codec, BOOL _isEncoder);
    ~OpalPluginVideoTranscoder();
    BOOL HasCodecControl(const char * name);
    BOOL CallCodecControl(const char * name,
                                      void * parm, 
                              unsigned int * parmLen,
                                       int & retVal);
    BOOL ExecuteCommand(const OpalMediaCommand & /*command*/);
    PINDEX GetOptimalDataFrameSize(BOOL input) const;
    BOOL ConvertFrames(const RTP_DataFrame & src, RTP_DataFrameList & dstList);

  protected:
    void * context;
    PluginCodec_Definition * codec;
    BOOL isEncoder;
    RTP_DataFrame * bufferRTP;
};

OpalPluginVideoTranscoder::OpalPluginVideoTranscoder(PluginCodec_Definition * _codec, BOOL _isEncoder)
  : OpalVideoTranscoder(_codec->sourceFormat, _codec->destFormat),
    codec(_codec), isEncoder(_isEncoder)
{ 
  if (codec != NULL && codec->createCodec != NULL) 
    context = (*codec->createCodec)(codec); 
  else 
    context = NULL;

  bufferRTP = NULL;
}

OpalPluginVideoTranscoder::~OpalPluginVideoTranscoder()
{ 
  if (bufferRTP == NULL)
    delete bufferRTP;

  if (codec != NULL && codec->destroyCodec != NULL) 
    (*codec->destroyCodec)(codec, context); 
}

BOOL OpalPluginVideoTranscoder::HasCodecControl(const char * name)
{
  PluginCodec_ControlDefn * codecControls = codec->codecControls;
  if (codecControls == NULL)
    return 0;

  while (codecControls->name != NULL) {
    if (strcmp(codecControls->name, name) == 0)
      return TRUE;
    codecControls++;
  }

  return FALSE;
}

BOOL OpalPluginVideoTranscoder::CallCodecControl(const char * name,
                                  void * parm, 
                          unsigned int * parmLen,
                                    int & retVal)
{
  PluginCodec_ControlDefn * codecControls = codec->codecControls;
  if (codecControls == NULL)
    return FALSE;

  while (codecControls->name != NULL) {
    if (strcmp(codecControls->name, name) == 0) {
      retVal = (*codecControls->control)(codec, context, name, parm, parmLen);
      return TRUE;
    }
    codecControls++;
  }

  return FALSE;
}

BOOL OpalPluginVideoTranscoder::ExecuteCommand(const OpalMediaCommand & /*command*/)
{
  return FALSE;
}

PINDEX OpalPluginVideoTranscoder::GetOptimalDataFrameSize(BOOL /*input*/) const
{
  return (frameWidth * frameHeight * 12) / 8;
  //if (isEncoder) 
  //  return input ? ((frameWidth * frameHeight * 12) / 8) : codec->maxFramesPerPacket;
  //else
  //  return input ? codec->maxFramesPerPacket : ((frameWidth * frameHeight * 12) / 8);
}

BOOL OpalPluginVideoTranscoder::ConvertFrames(const RTP_DataFrame & src, RTP_DataFrameList & dstList)
{
  if (codec == NULL || codec->codecFunction == NULL)
    return FALSE;

  dstList.RemoveAll();

  // get the size of the output buffer
  int outputDataSize;
  if (!CallCodecControl("get_output_data_size", NULL, NULL, outputDataSize))
    return FALSE;

  unsigned flags;

  if (isEncoder) {

    do {

      // create the output buffer
      RTP_DataFrame * dst = new RTP_DataFrame(outputDataSize);

      // call the codec function
      unsigned int fromLen = src.GetSize();
      unsigned int toLen = dst->GetSize();
      flags = 0;
      BOOL stat = (codec->codecFunction)(codec, context, 
                                        (const BYTE *)src, &fromLen,
                                        dst->GetPointer(), &toLen,
                                        &flags) != 0;

      if (!stat) {
        delete dst;
        return FALSE;
      }
      dst->SetPayloadSize(toLen - dst->GetHeaderSize());
      dstList.Append(dst);

    } while ((flags & PluginCodec_ReturnCoderLastFrame) == 0);
  }

  else {

    if (bufferRTP == NULL)
      bufferRTP = new RTP_DataFrame(outputDataSize);
    else
      bufferRTP->SetPayloadSize(outputDataSize);

    // call the codec function
    unsigned int fromLen = src.GetHeaderSize() + src.GetPayloadSize();
    unsigned int toLen = bufferRTP->GetSize();
    flags = 0;
    BOOL stat = (codec->codecFunction)(codec, context, 
                                        (const BYTE *)src, &fromLen,
                                        bufferRTP->GetPointer(), &toLen,
                                        &flags) != 0;
    if (!stat)
      return FALSE;

    if ((flags & PluginCodec_ReturnCoderRequestIFrame) != 0) {
      if (commandNotifier != PNotifier()) {
        OpalVideoUpdatePicture updatePicture;
        commandNotifier(updatePicture, 0); 
        PTRACE (3, "H261\t Could not decode frame, sending VideoUpdatePicture in hope of an I-Frame.");
      }
      return TRUE;
    }

    if ((flags & PluginCodec_ReturnCoderLastFrame) != 0) {
      bufferRTP->SetPayloadSize(toLen - bufferRTP->GetHeaderSize());
      dstList.Append(bufferRTP);
      bufferRTP = NULL;
    }
  }

  return TRUE;
};


#endif // OPAL_VIDEO

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

    H323PluginCapabilityInfo(const PString & _mediaFormat, 
                             const PString & _baseName);

    const PString & GetFormatName() const
    { return capabilityFormatName; }

    //H323Codec * CreateCodec(H323Codec::Direction direction) const;

  protected:
    PluginCodec_Definition * encoderCodec;
    PluginCodec_Definition * decoderCodec;
    PString                  capabilityFormatName;
    OpalMediaFormat          mediaFormat;
};

#if OPAL_AUDIO

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling most audio plugin capabilities
//

class H323AudioPluginCapability : public H323AudioCapability,
                                  public H323PluginCapabilityInfo
{
  PCLASSINFO(H323AudioPluginCapability, H323AudioCapability);
  public:
    H323AudioPluginCapability(PluginCodec_Definition * _encoderCodec,
                         PluginCodec_Definition * _decoderCodec,
                         unsigned _pluginSubType)
      : H323AudioCapability(), 
        H323PluginCapabilityInfo(_encoderCodec, _decoderCodec),
        pluginSubType(_pluginSubType)
      { 
        SetTxFramesInPacket(_decoderCodec->maxFramesPerPacket);
        // _encoderCodec->recommendedFramesPerPacket
      }

    // this constructor is only used when creating a capability without a codec
    H323AudioPluginCapability(const PString & _mediaFormat, const PString & _baseName,
                         unsigned maxFramesPerPacket, unsigned /*recommendedFramesPerPacket*/,
                         unsigned _pluginSubType)
      : H323AudioCapability(), 
        H323PluginCapabilityInfo(_mediaFormat, _baseName),
        pluginSubType(_pluginSubType)
      { 
        for (PINDEX i = 0; audioMaps[i].pluginCapType >= 0; i++) {
          if (audioMaps[i].pluginCapType == (int)_pluginSubType) { 
            h323subType = audioMaps[i].h323SubType;
            break;
          }
        }
        rtpPayloadType = OpalMediaFormat(_mediaFormat).GetPayloadType();
        SetTxFramesInPacket(maxFramesPerPacket);
        // recommendedFramesPerPacket
      }

    virtual PObject * Clone() const
    { return new H323AudioPluginCapability(*this); }

    virtual PString GetFormatName() const
    { return H323PluginCapabilityInfo::GetFormatName();}

    //virtual H323Codec * CreateCodec(H323Codec::Direction direction) const
    //{ return H323PluginCapabilityInfo::CreateCodec(direction); }

    virtual unsigned GetSubType() const
    { return pluginSubType; }

  protected:
    unsigned pluginSubType;
    unsigned h323subType;   // only set if using capability without codec
};

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling non standard audio capabilities
//

class H323CodecPluginNonStandardAudioCapability : public H323NonStandardAudioCapability,
                                                  public H323PluginCapabilityInfo
{
  PCLASSINFO(H323CodecPluginNonStandardAudioCapability, H323NonStandardAudioCapability);
  public:
    H323CodecPluginNonStandardAudioCapability(
                                   PluginCodec_Definition * _encoderCodec,
                                   PluginCodec_Definition * _decoderCodec,
                                   H323NonStandardCapabilityInfo::CompareFuncType compareFunc,
                                   const unsigned char * data, unsigned dataLen);

    H323CodecPluginNonStandardAudioCapability(
                                   PluginCodec_Definition * _encoderCodec,
                                   PluginCodec_Definition * _decoderCodec,
                                   const unsigned char * data, unsigned dataLen);

    virtual PObject * Clone() const
    { return new H323CodecPluginNonStandardAudioCapability(*this); }

    virtual PString GetFormatName() const
    { return H323PluginCapabilityInfo::GetFormatName();}

    //virtual H323Codec * CreateCodec(H323Codec::Direction direction) const
    //{ return H323PluginCapabilityInfo::CreateCodec(direction); }
};

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling generic audio capabilities
//

class H323CodecPluginGenericAudioCapability : public H323GenericAudioCapability,
					                                    public H323PluginCapabilityInfo
{
  PCLASSINFO(H323CodecPluginGenericAudioCapability, H323GenericAudioCapability);
  public:
    H323CodecPluginGenericAudioCapability(
                                   const PluginCodec_Definition * _encoderCodec,
                                   const PluginCodec_Definition * _decoderCodec,
				   const PluginCodec_H323GenericCodecData * data );

    virtual PObject * Clone() const
    {
      return new H323CodecPluginGenericAudioCapability(*this);
    }

    virtual PString GetFormatName() const
    { return H323PluginCapabilityInfo::GetFormatName();}

    //virtual H323Codec * CreateCodec(H323Codec::Direction direction) const
    //{ return H323PluginCapabilityInfo::CreateCodec(direction); }
};

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
                               BOOL _annexA = TRUE)
      : H323AudioPluginCapability(_encoderCodec, _decoderCodec, H245_AudioCapability::e_g7231),
        annexA(_annexA)
      { }

    Comparison Compare(const PObject & obj) const
    {
      if (!PIsDescendant(&obj, H323PluginG7231Capability))
        return LessThan;

      Comparison result = H323AudioCapability::Compare(obj);
      if (result != EqualTo)
        return result;

      PINDEX otherAnnexA = ((const H323PluginG7231Capability &)obj).annexA;
      if (annexA < otherAnnexA)
        return LessThan;
      if (annexA > otherAnnexA)
        return GreaterThan;
      return EqualTo;
    }

    virtual PObject * Clone() const
    { 
      return new H323PluginG7231Capability(*this);
    }

    virtual BOOL OnSendingPDU(H245_AudioCapability & cap, unsigned packetSize) const
    {
      cap.SetTag(H245_AudioCapability::e_g7231);
      H245_AudioCapability_g7231 & g7231 = cap;
      g7231.m_maxAl_sduAudioFrames = packetSize;
      g7231.m_silenceSuppression = annexA;
      return TRUE;
    }

    virtual BOOL OnReceivedPDU(const H245_AudioCapability & cap,  unsigned & packetSize)
    {
      if (cap.GetTag() != H245_AudioCapability::e_g7231)
        return FALSE;
      const H245_AudioCapability_g7231 & g7231 = cap;
      packetSize = g7231.m_maxAl_sduAudioFrames;
      annexA = g7231.m_silenceSuppression;
      return TRUE;
    }

  protected:
    BOOL annexA;
};

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling GSM plugin capabilities
//

class H323GSMPluginCapability : public H323AudioPluginCapability
{
  PCLASSINFO(H323GSMPluginCapability, H323AudioPluginCapability);
  public:
    H323GSMPluginCapability(PluginCodec_Definition * _encoderCodec,
                            PluginCodec_Definition * _decoderCodec,
                            int _pluginSubType, int _comfortNoise, int _scrambled)
      : H323AudioPluginCapability(_encoderCodec, _decoderCodec, _pluginSubType),
        comfortNoise(_comfortNoise), scrambled(_scrambled)
    { }

    Comparison Compare(const PObject & obj) const;

    virtual PObject * Clone() const
    {
      return new H323GSMPluginCapability(*this);
    }

    virtual BOOL OnSendingPDU(
      H245_AudioCapability & pdu,  /// PDU to set information on
      unsigned packetSize          /// Packet size to use in capability
    ) const;

    virtual BOOL OnReceivedPDU(
      const H245_AudioCapability & pdu,  /// PDU to get information from
      unsigned & packetSize              /// Packet size to use in capability
    );
  protected:
    int comfortNoise;
    int scrambled;
};

#endif // OPAL_H323

#if OPAL_VIDEO

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling most audio plugin capabilities
//

class H323VideoPluginCapability : public H323VideoCapability,
                             public H323PluginCapabilityInfo
{
  PCLASSINFO(H323VideoPluginCapability, H323VideoCapability);
  public:
    H323VideoPluginCapability(PluginCodec_Definition * _encoderCodec,
                              PluginCodec_Definition * _decoderCodec,
                              unsigned _pluginSubType)
      : H323VideoCapability(), 
        H323PluginCapabilityInfo(_encoderCodec, _decoderCodec),
        pluginSubType(_pluginSubType)
      { 
      }

#if 0
    // this constructor is only used when creating a capability without a codec
    H323VideoPluginCapability(const PString & _mediaFormat, const PString & _baseName,
                         unsigned maxFramesPerPacket, unsigned /*recommendedFramesPerPacket*/,
                         unsigned _pluginSubType)
      : H323VideoCapability(), 
        H323PluginCapabilityInfo(_mediaFormat, _baseName),
        pluginSubType(_pluginSubType)
      { 
        for (PINDEX i = 0; audioMaps[i].pluginCapType >= 0; i++) {
          if (videoMaps[i].pluginCapType == (int)_pluginSubType) { 
            h323subType = audioMaps[i].h323SubType;
            break;
          }
        }
        rtpPayloadType = OpalMediaFormat(_mediaFormat).GetPayloadType();
      }
#endif

    //virtual PObject * Clone() const
    //{ return new H323VideoPluginCapability(*this); }

    virtual PString GetFormatName() const
    { return H323PluginCapabilityInfo::GetFormatName();}

    //virtual H323Codec * CreateCodec(H323Codec::Direction direction) const
    //{ return H323PluginCapabilityInfo::CreateCodec(direction); }

    virtual unsigned GetSubType() const
    { return pluginSubType; }

  protected:
    unsigned pluginSubType;
    unsigned h323subType;   // only set if using capability without codec
};

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling non standard video capabilities
//

class H323CodecPluginNonStandardVideoCapability : public H323NonStandardVideoCapability,
                                                  public H323PluginCapabilityInfo
{
  PCLASSINFO(H323CodecPluginNonStandardVideoCapability, H323NonStandardVideoCapability);
  public:
    H323CodecPluginNonStandardVideoCapability(
                                   PluginCodec_Definition * _encoderCodec,
                                   PluginCodec_Definition * _decoderCodec,
                                   H323NonStandardCapabilityInfo::CompareFuncType compareFunc,
                                   const unsigned char * data, unsigned dataLen);

    H323CodecPluginNonStandardVideoCapability(
                                   PluginCodec_Definition * _encoderCodec,
                                   PluginCodec_Definition * _decoderCodec,
                                   const unsigned char * data, unsigned dataLen);

    virtual PObject * Clone() const
    { return new H323CodecPluginNonStandardVideoCapability(*this); }

    virtual PString GetFormatName() const
    { return H323PluginCapabilityInfo::GetFormatName();}

    //virtual H323Codec * CreateCodec(H323Codec::Direction direction) const
    //{ return H323PluginCapabilityInfo::CreateCodec(direction); }
};

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling generic video capabilities
//

class H323CodecPluginGenericVideoCapability : public H323GenericVideoCapability,
					                                    public H323PluginCapabilityInfo
{
  PCLASSINFO(H323CodecPluginGenericVideoCapability, H323GenericVideoCapability);
  public:
    H323CodecPluginGenericVideoCapability(
                                   const PluginCodec_Definition * _encoderCodec,
                                   const PluginCodec_Definition * _decoderCodec,
				                 const PluginCodec_H323GenericCodecData * data );

    virtual PObject * Clone() const
    { return new H323CodecPluginGenericVideoCapability(*this); }

    virtual PString GetFormatName() const
    { return H323PluginCapabilityInfo::GetFormatName();}

    //virtual H323Codec * CreateCodec(H323Codec::Direction direction) const
    //{ return H323PluginCapabilityInfo::CreateCodec(direction); }
};

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling H.261 plugin capabilities
//

class H323H261PluginCapability : public H323VideoPluginCapability
{
  PCLASSINFO(H323H261PluginCapability, H323VideoPluginCapability);
  public:
    H323H261PluginCapability(PluginCodec_Definition * _encoderCodec,
                             PluginCodec_Definition * _decoderCodec,
                             PluginCodec_H323VideoH261 * capData)
      : H323VideoPluginCapability(_encoderCodec, _decoderCodec, H245_VideoCapability::e_h261VideoCapability),
        qcifMPI                          (capData->qcifMPI),
        cifMPI                           (capData->cifMPI),
        temporalSpatialTradeOffCapability(capData->temporalSpatialTradeOffCapability),
        maxBitRate                       (capData->maxBitRate),
        stillImageTransmission           (capData->stillImageTransmission)
    { }

    Comparison Compare(const PObject & obj) const;

    virtual PObject * Clone() const
    { 
      return new H323H261PluginCapability(*this); 
    }

    virtual BOOL OnSendingPDU(
      H245_VideoCapability & pdu  /// PDU to set information on
    ) const;

    virtual BOOL OnSendingPDU(
      H245_VideoMode & pdu
    ) const;

    virtual BOOL OnReceivedPDU(
      const H245_VideoCapability & pdu  /// PDU to get information from
    );

    //H323Codec * CreateCodec(H323Codec::Direction direction) const
    //{ return H323PluginCapabilityInfo::CreateCodec(direction); }

  protected:
    unsigned qcifMPI;                   // 1..4 units 1/29.97 Hz
    unsigned cifMPI;                    // 1..4 units 1/29.97 Hz
    BOOL     temporalSpatialTradeOffCapability;
    unsigned maxBitRate;                // units of 100 bit/s
    BOOL     stillImageTransmission;    // Annex D of H.261
};

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling H.263 plugin capabilities
//

/*
class H323H263PluginCapability : public H323PluginCapability
{
  PCLASSINFO(H323H263PluginCapability, H323PluginCapability);
  public:
    H323H263PluginCapability(PluginCodec_Definition * _encoderCodec,
                             PluginCodec_Definition * _decoderCodec,
                             PluginCodec_H323VideoH263 * capData)
      : H323PluginCapability(_encoderCodec, _decoderCodec, H245_VideoCapability::e_h263VideoCapability),
        sqcifMPI(capData->sqcifMPI),
        qcifMPI(capData->qcifMPI),
        cifMPI(capData->cifMPI),
        cif4MPI(capData->cif4MPI),
        cif16MPI(capData->cif16MPI),
        maxBitRate(capData->maxBitRate),
        unrestrictedVector(capData->unrestrictedVector),
        arithmeticCoding(capData->arithmeticCoding),
        advancedPrediction(capData->advancedPrediction),
        pbFrames(capData->pbFrames),
        temporalSpatialTradeOffCapability(capData->temporalSpatialTradeOffCapability),
        hrdB(capData->hrdB),
        bppMaxKb(capData->bppMaxKb),
        slowSqcifMPI(capData->slowSqcifMPI),
        slowQcifMPI(capData->slowQcifMPI),
        slowCifMPI(capData->slowCifMPI),
        slowCif4MPI(capData->slowCif4MPI),
        slowCif16MPI(capData->slowCif16MPI),
        errorCompensation(capData->errorCompensation)
    { }

    Comparison Compare(const PObject & obj) const;

    virtual PObject * Clone() const
    { 
      return new H323H263PluginCapability(*this); 
    }

    virtual BOOL OnSendingPDU(
      H245_VideoCapability & pdu  /// PDU to set information on
    ) const;

    virtual BOOL OnSendingPDU(
      H245_VideoMode & pdu
    ) const;

    virtual BOOL OnReceivedPDU(
      const H245_VideoCapability & pdu  /// PDU to get information from
    );

  protected:
    unsigned char sqcifMPI;                          // INTEGER (1..32) OPTIONAL, -- units 1/29.97 Hz (0 if not present)
    unsigned char qcifMPI;                           // INTEGER (1..32) OPTIONAL, -- units 1/29.97 Hz (0 if not present)
    unsigned char cifMPI;                            // INTEGER (1..32) OPTIONAL, -- units 1/29.97 Hz (0 if not present)
    unsigned char cif4MPI;                           // INTEGER (1..32) OPTIONAL, -- units 1/29.97 Hz (0 if not present)
    unsigned char cif16MPI;                          // INTEGER (1..32) OPTIONAL, -- units 1/29.97 Hz (0 if not present)
    unsigned int maxBitRate;                         // INTEGER (1..192400) units 100 bit/s
    unsigned char unrestrictedVector;                //	BOOLEAN
    unsigned char arithmeticCoding;                  //	BOOLEAN,
    unsigned char advancedPrediction;                //	BOOLEAN,
    unsigned char pbFrames;                          //	BOOLEAN,
    unsigned char temporalSpatialTradeOffCapability; //	BOOLEAN,
    int hrdB;                                        // INTEGER (0..524287) OPTIONAL, -- units 128 bits (-1 if not present)
    int bppMaxKb;                                    // INTEGER (0..65535) OPTIONAL, -- units 1024 bits (-1 if not present)

    unsigned int slowSqcifMPI;                       // INTEGER (1..3600) OPTIONAL, --  units seconds/frame (0 if not present)
    unsigned int slowQcifMPI;                        //	INTEGER (1..3600) OPTIONAL, -- units seconds/frame (0 if not present)
    unsigned int slowCifMPI;                         //	INTEGER (1..3600) OPTIONAL, -- units seconds/frame (0 if not present)
    unsigned int slowCif4MPI;                        //	INTEGER (1..3600) OPTIONAL, -- units seconds/frame (0 if not present)
    unsigned int slowCif16MPI;                       //	INTEGER (1..3600) OPTIONAL, -- units seconds/frame (0 if not present)
    unsigned char errorCompensation;                 // BOOLEAN,
};

*/

/////////////////////////////////////////////////////////////////////////////

#endif //  OPAL_VIDEO

#endif // OPAL_H323


/////////////////////////////////////////////////////////////////////////////

class H323StaticPluginCodec
{
  public:
    virtual ~H323StaticPluginCodec() { }
    virtual PluginCodec_GetAPIVersionFunction Get_GetAPIFn() = 0;
    virtual PluginCodec_GetCodecFunction Get_GetCodecFn() = 0;
};

PMutex & OpalPluginCodecManager::GetMediaFormatMutex()
{
  static PMutex mutex;
  return mutex;
}

OpalPluginCodecManager::OpalPluginCodecManager(PPluginManager * _pluginMgr)
 : PPluginModuleManager(PLUGIN_CODEC_GET_CODEC_FN_STR, _pluginMgr)
{
  // instantiate all of the media formats
  {
    OpalMediaFormatFactory::KeyList_T keyList = OpalMediaFormatFactory::GetKeyList();
    OpalMediaFormatFactory::KeyList_T::const_iterator r;
    for (r = keyList.begin(); r != keyList.end(); ++r) {
      OpalMediaFormat * instance = OpalMediaFormatFactory::CreateInstance(*r);
      if (instance == NULL) {
        PTRACE(4, "H323PLUGIN\tCannot instantiate opal media format " << *r);
      } else {
        PTRACE(4, "H323PLUGIN\tCreating media format " << *r);
      }
    }
  }

  // instantiate all of the static codecs
  {
    PFactory<H323StaticPluginCodec>::KeyList_T keyList = PFactory<H323StaticPluginCodec>::GetKeyList();
    PFactory<H323StaticPluginCodec>::KeyList_T::const_iterator r;
    for (r = keyList.begin(); r != keyList.end(); ++r) {
      H323StaticPluginCodec * instance = PFactory<H323StaticPluginCodec>::CreateInstance(*r);
      if (instance == NULL) {
        PTRACE(4, "H323PLUGIN\tCannot instantiate static codec plugin " << *r);
      } else {
        PTRACE(4, "H323PLUGIN\tLoading static codec plugin " << *r);
        RegisterStaticCodec(*r, instance->Get_GetAPIFn(), instance->Get_GetCodecFn());
      }
    }
  }

  // cause the plugin manager to load all dynamic plugins
  pluginMgr->AddNotifier(PCREATE_NOTIFIER(OnLoadModule), TRUE);
}

OpalPluginCodecManager::~OpalPluginCodecManager()
{
}

void OpalPluginCodecManager::OnShutdown()
{
  // unregister the plugin media formats
  OpalMediaFormatFactory::UnregisterAll();

#if 0 // OPAL_H323
  // unregister the plugin capabilities
  H323CapabilityFactory::UnregisterAll();
#endif
}

void OpalPluginCodecManager::OnLoadPlugin(PDynaLink & dll, INT code)
{
  PluginCodec_GetCodecFunction getCodecs;
  if (!dll.GetFunction(PString(signatureFunctionName), (PDynaLink::Function &)getCodecs)) {
    PTRACE(3, "H323PLUGIN\tPlugin Codec DLL " << dll.GetName() << " is not a plugin codec");
    return;
  }

  unsigned int count;
  PluginCodec_Definition * codecs = (*getCodecs)(&count, PLUGIN_CODEC_VERSION_VIDEO);
  if (codecs == NULL || count == 0) {
    PTRACE(3, "H323PLUGIN\tPlugin Codec DLL " << dll.GetName() << " contains no codec definitions");
    return;
  } 

  PTRACE(3, "H323PLUGIN\tLoading plugin codec " << dll.GetName());

  switch (code) {

    // plugin loaded
    case 0:
      RegisterCodecPlugins(count, codecs);
      break;

    // plugin unloaded
    case 1:
      UnregisterCodecPlugins(count, codecs);
      break;

    default:
      break;
  }
}

void OpalPluginCodecManager::RegisterStaticCodec(
      const char * PTRACE_PARAM(name),
      PluginCodec_GetAPIVersionFunction /*getApiVerFn*/,
      PluginCodec_GetCodecFunction getCodecFn)
{
  unsigned int count;
  PluginCodec_Definition * codecs = (*getCodecFn)(&count, PLUGIN_CODEC_VERSION);
  if (codecs == NULL || count == 0) {
    PTRACE(3, "H323PLUGIN\tStatic codec " << name << " contains no codec definitions");
    return;
  } 

  RegisterCodecPlugins(count, codecs);
}

void OpalPluginCodecManager::RegisterCodecPlugins(unsigned int count, void * _codecList)
{
  // make sure all non-timestamped codecs have the same concept of "now"
  static time_t codecNow = ::time(NULL);

  PluginCodec_Definition * codecList = (PluginCodec_Definition *)_codecList;
  unsigned i, j ;
  for (i = 0; i < count; i++) {

    PluginCodec_Definition & encoder = codecList[i];

    BOOL videoSupported = encoder.version >= PLUGIN_CODEC_VERSION_VIDEO;

    // for every encoder, we need a decoder
    BOOL found = FALSE;
    BOOL isEncoder = FALSE;
    if (encoder.h323CapabilityType != PluginCodec_H323Codec_undefined &&
         (
           ((encoder.flags & PluginCodec_MediaTypeMask) == PluginCodec_MediaTypeAudio) && 
            strcmp(encoder.sourceFormat, "L16") == 0
         ) ||
         (
           ((encoder.flags & PluginCodec_MediaTypeMask) == PluginCodec_MediaTypeAudioStreamed) && 
            strcmp(encoder.sourceFormat, "L16") == 0
         ) ||
         (
           videoSupported &&
           ((encoder.flags & PluginCodec_MediaTypeMask) == PluginCodec_MediaTypeVideo) && 
           strcmp(encoder.sourceFormat, "YUV420P") == 0
        )
       ) {
      isEncoder = TRUE;
      for (j = 0; j < count; j++) {

        PluginCodec_Definition & decoder = codecList[j];
        if (
            (decoder.h323CapabilityType == encoder.h323CapabilityType) &&
            ((decoder.flags & PluginCodec_MediaTypeMask) == (encoder.flags & PluginCodec_MediaTypeMask)) &&
            (strcmp(decoder.sourceFormat, encoder.destFormat) == 0) &&
            (strcmp(decoder.destFormat,   encoder.sourceFormat) == 0)
            )
          { 

          // deal with codec having no info, or timestamp in future
          time_t timeStamp = codecList[i].info == NULL ? codecNow : codecList[i].info->timestamp;
          if (timeStamp > codecNow)
            timeStamp = codecNow;

          // create the media format, transcoder and capability associated with this plugin
          RegisterPluginPair(&encoder, &decoder);
          found = TRUE;

          PTRACE(2, "H323PLUGIN\tPlugin codec " << encoder.descr << " defined");
          break;
        }
      }
    }
    if (!found && isEncoder) {
      PTRACE(2, "H323PLUGIN\tCannot find decoder for plugin encoder " << encoder.descr);
    }
  }
}

void OpalPluginCodecManager::UnregisterCodecPlugins(unsigned int /*count*/, void * /*codec*/)
{
}

OpalMediaFormatList & OpalPluginCodecManager::GetMediaFormatList()
{
  static OpalMediaFormatList mediaFormatList;
  return mediaFormatList;
}

OpalMediaFormatList OpalPluginCodecManager::GetMediaFormats()
{
  PWaitAndSignal m(GetMediaFormatMutex());
  return GetMediaFormatList();
}

void OpalPluginCodecManager::RegisterPluginPair(
       PluginCodec_Definition * encoderCodec,
       PluginCodec_Definition * decoderCodec
) 
{
  // make sure all non-timestamped codecs have the same concept of "now"
  static time_t mediaNow = time(NULL);

  // deal with codec having no info, or timestamp in future
  time_t timeStamp = encoderCodec->info == NULL ? mediaNow : encoderCodec->info->timestamp;
  if (timeStamp > mediaNow)
    timeStamp = mediaNow;

  unsigned defaultSessionID = 0;
  unsigned frameTime = 0;
  unsigned clockRate = 0;
  switch (encoderCodec->flags & PluginCodec_MediaTypeMask) {
#if OPAL_VIDEO
    case PluginCodec_MediaTypeVideo:
      defaultSessionID = OpalMediaFormat::DefaultVideoSessionID;
      break;
#endif
#if OPAL_AUDIO
    case PluginCodec_MediaTypeAudio:
    case PluginCodec_MediaTypeAudioStreamed:
      defaultSessionID = OpalMediaFormat::DefaultAudioSessionID;
      frameTime = (8 * encoderCodec->nsPerFrame) / 1000;
      clockRate = encoderCodec->sampleRate;
      break;
#endif
    default:
      break;
  }

  // add the media format
  if (defaultSessionID == 0) {
    PTRACE(3, "H323PLUGIN\tCodec DLL provides unknown media format " << (int)(encoderCodec->flags & PluginCodec_MediaTypeMask));
  } else {
    PString fmtName = CreateCodecName(encoderCodec, FALSE);
    OpalMediaFormat existingFormat(fmtName);
    if (existingFormat.IsValid()) {
      PTRACE(3, "H323PLUGIN\tMedia format " << fmtName << " already exists");
      AddFormat(existingFormat);
    } else {
      PTRACE(3, "H323PLUGIN\tCreating new media format" << fmtName);

      OpalMediaFormat * mediaFormat = NULL;

      // manually register the new singleton type, as we do not have a concrete type
      switch (encoderCodec->flags & PluginCodec_MediaTypeMask) {
#if OPAL_VIDEO
        case PluginCodec_MediaTypeVideo:
          mediaFormat = new OpalPluginVideoMediaFormat(
                                  encoderCodec, 
                                  encoderCodec->sdpFormat,
                                  timeStamp);
          break;
#endif
#if OPAL_AUDIO
        case PluginCodec_MediaTypeAudio:
        case PluginCodec_MediaTypeAudioStreamed:
          mediaFormat = new OpalPluginAudioMediaFormat(
                                   encoderCodec,
                                   encoderCodec->sdpFormat,
                                   frameTime,
                                   clockRate,
                                   timeStamp);
          break;
#endif
        default:
          break;
      }
      // if the codec has been flagged to use a shared RTP payload type, then find a codec with the same SDP name
      // and use that RTP code rather than creating a new one. That prevents codecs (like Speex) from consuming
      // dozens of dynamic RTP types
      if ((encoderCodec->flags & PluginCodec_RTPTypeShared) != 0 && (encoderCodec->sdpFormat != NULL)) {
        PWaitAndSignal m(OpalPluginCodecManager::GetMediaFormatMutex());
        OpalMediaFormatList & list = OpalPluginCodecManager::GetMediaFormatList();
        for (PINDEX i = 0; i < list.GetSize(); i++) {
          OpalMediaFormat * opalFmt = &list[i];
#if OPAL_AUDIO
          {
            OpalPluginAudioMediaFormat * fmt = dynamic_cast<OpalPluginAudioMediaFormat *>(opalFmt);
            if (
                (fmt != NULL) && 
                (fmt->encoderCodec->sdpFormat != NULL) &&
                (strcmpi(encoderCodec->sdpFormat, fmt->encoderCodec->sdpFormat) == 0)
                ) {
              mediaFormat->rtpPayloadType = fmt->GetPayloadType();
              break;
            }
          }
#endif
#if OPAL_VIDEO
          {
            OpalPluginVideoMediaFormat * fmt = dynamic_cast<OpalPluginVideoMediaFormat *>(opalFmt);
            if (
                (fmt != NULL) && 
                (fmt->encoderCodec->sdpFormat != NULL) &&
                (strcmpi(encoderCodec->sdpFormat, fmt->encoderCodec->sdpFormat) == 0)
                ) {
              mediaFormat->rtpPayloadType = fmt->GetPayloadType();
              break;
            }
          }
#endif
          {
            char fmtpBuffer[1024];
            fmtBuffer[0] - '\0';
            int len = sizeof(fmtpBuffer)-1;
            if (CallCodecControl(encoderCodec, NULL, "get_fmtp", fmtpBuffer, &len, retVal)) {
              if (retVal > 0 && (strlen(fmtpBuffer) > 0)
                mediaFormat->SetOptionString("fmtp", fmtpBuffer);
            }
          }
          mediaFormat->
        }
      }

      // save the format
      AddFormat(*mediaFormat);

      // this looks like a memory leak, but it isn't
      //delete mediaFormat;
    }
  }

  // Create transcoder factories for the codecs
  switch (encoderCodec->flags & PluginCodec_MediaTypeMask) {
#if OPAL_VIDEO
    case PluginCodec_MediaTypeVideo:
      new OpalPluginTranscoderFactory<OpalPluginVideoTranscoder>::Worker(OpalMediaFormatPair(OpalYUV420P,                encoderCodec->destFormat), encoderCodec, TRUE);
      new OpalPluginTranscoderFactory<OpalPluginVideoTranscoder>::Worker(OpalMediaFormatPair(encoderCodec->destFormat, OpalYUV420P),                decoderCodec, FALSE);
      break;
#endif
#if OPAL_AUDIO
    case PluginCodec_MediaTypeAudio:
      new OpalPluginTranscoderFactory<OpalPluginFramedAudioTranscoder>::Worker(OpalMediaFormatPair("PCM-16",                 encoderCodec->destFormat), encoderCodec, TRUE);
      new OpalPluginTranscoderFactory<OpalPluginFramedAudioTranscoder>::Worker(OpalMediaFormatPair(encoderCodec->destFormat, OpalPCM16),                 decoderCodec, FALSE);
      break;
    case PluginCodec_MediaTypeAudioStreamed:
      new OpalPluginTranscoderFactory<OpalPluginStreamedAudioEncoder>::Worker(OpalMediaFormatPair("PCM-16",                 encoderCodec->destFormat), encoderCodec, TRUE);
      new OpalPluginTranscoderFactory<OpalPluginStreamedAudioDecoder>::Worker(OpalMediaFormatPair(encoderCodec->destFormat, OpalPCM16),                 decoderCodec, FALSE);
      break;
#endif
    default:
      break;
  }

#if OPAL_H323

  // add the capability
  H323CodecPluginCapabilityMapEntry * map = NULL;

  switch (encoderCodec->flags & PluginCodec_MediaTypeMask) {
#if OPAL_AUDIO
    case PluginCodec_MediaTypeAudio:
    case PluginCodec_MediaTypeAudioStreamed:
      map = audioMaps;
      break;
#endif // OPAL_AUDIO

#if OPAL_VIDEO
    case PluginCodec_MediaTypeVideo:
      map = videoMaps;
      break;
#endif // OPAL_VIDEO

    default:
      break;
  }

  if (map == NULL) {
    PTRACE(3, "H323PLUGIN\tCannot create capability for unknown plugin codec media format " << (int)(encoderCodec->flags & PluginCodec_MediaTypeMask));
  } 
  else if (encoderCodec->h323CapabilityType != PluginCodec_H323Codec_undefined) {
    for (PINDEX i = 0; map[i].pluginCapType >= 0; i++) {
      if (map[i].pluginCapType == encoderCodec->h323CapabilityType) {
        H323Capability * cap = NULL;
        if (map[i].createFunc != NULL)
          cap = (*map[i].createFunc)(encoderCodec, decoderCodec, map[i].h323SubType);
        else {
          switch (encoderCodec->flags & PluginCodec_MediaTypeMask) {
#if OPAL_AUDIO
            case PluginCodec_MediaTypeAudio:
            case PluginCodec_MediaTypeAudioStreamed:
              cap = new H323AudioPluginCapability(encoderCodec, decoderCodec, map[i].h323SubType);
              break;
#endif // OPAL_AUDIO

#if OPAL_VIDEO
            case PluginCodec_MediaTypeVideo:
              // all video caps are created using the create functions
              break;
#endif // OPAL_VIDEO

            default:
              break;
          }
        }

        // manually register the new singleton type, as we do not have a concrete type
        if (cap != NULL)
          H323CapabilityFactory::Register(CreateCodecName(encoderCodec, TRUE), cap);
        break;
      }
    }
  }
#endif // OPAL_H323
}

void OpalPluginCodecManager::AddFormat(const OpalMediaFormat & fmt)
{
  PWaitAndSignal m(GetMediaFormatMutex());
  GetMediaFormatList() += fmt;
}


/////////////////////////////////////////////////////////////////////////////

#if OPAL_H323

/*
H323Capability * OpalPluginCodecManager::CreateCapability(
          const PString & _mediaFormat, 
          const PString & _baseName,
                 unsigned maxFramesPerPacket, 
                 unsigned recommendedFramesPerPacket,
                 unsigned _pluginSubType
  )
{
  return new H323PluginCapability(_mediaFormat, _baseName,
                                  maxFramesPerPacket, recommendedFramesPerPacket, _pluginSubType);
}
*/

#if OPAL_AUDIO

H323Capability * CreateNonStandardAudioCap(
  PluginCodec_Definition * encoderCodec,  
  PluginCodec_Definition * decoderCodec,
  int /*subType*/) 
{
  PluginCodec_H323NonStandardCodecData * pluginData =  (PluginCodec_H323NonStandardCodecData *)encoderCodec->h323CapabilityData;
  if (pluginData == NULL) {
    return new H323CodecPluginNonStandardAudioCapability(
                             encoderCodec, decoderCodec,
                             (const unsigned char *)encoderCodec->descr, 
                             strlen(encoderCodec->descr));
  }

  else if (pluginData->capabilityMatchFunction != NULL) 
    return new H323CodecPluginNonStandardAudioCapability(encoderCodec, decoderCodec,
                             (H323NonStandardCapabilityInfo::CompareFuncType)pluginData->capabilityMatchFunction,
                             pluginData->data, pluginData->dataLength);
  else
    return new H323CodecPluginNonStandardAudioCapability(
                             encoderCodec, decoderCodec,
                             pluginData->data, pluginData->dataLength);
}

H323Capability *CreateGenericAudioCap(
  PluginCodec_Definition * encoderCodec,  
  PluginCodec_Definition * decoderCodec,
  int /*subType*/) 
{
    PluginCodec_H323GenericCodecData * pluginData = (PluginCodec_H323GenericCodecData *)encoderCodec->h323CapabilityData;

    if(pluginData == NULL ) {
	PTRACE(1, "Generic codec information for codec '"<<encoderCodec->descr<<"' has NULL data field");
	return NULL;
    }
    return new H323CodecPluginGenericAudioCapability(encoderCodec, decoderCodec, pluginData);
}

H323Capability * CreateG7231Cap(
  PluginCodec_Definition * encoderCodec,  
  PluginCodec_Definition * decoderCodec,
  int /*subType*/) 
{
  return new H323PluginG7231Capability(encoderCodec, decoderCodec, decoderCodec->h323CapabilityData != 0);
}


H323Capability * CreateGSMCap(
  PluginCodec_Definition * encoderCodec,  
  PluginCodec_Definition * decoderCodec,
  int subType) 
{
  PluginCodec_H323AudioGSMData * pluginData =  (PluginCodec_H323AudioGSMData *)encoderCodec->h323CapabilityData;
  return new H323GSMPluginCapability(encoderCodec, decoderCodec, subType, pluginData->comfortNoise, pluginData->scrambled);
}

#endif // OPAL_AUDIO

#if OPAL_VIDEO

H323Capability * CreateNonStandardVideoCap(
  PluginCodec_Definition * encoderCodec,  
  PluginCodec_Definition * decoderCodec,
  int /*subType*/) 
{
  PluginCodec_H323NonStandardCodecData * pluginData =  (PluginCodec_H323NonStandardCodecData *)encoderCodec->h323CapabilityData;
  if (pluginData == NULL) {
    return new H323CodecPluginNonStandardVideoCapability(
                             encoderCodec, decoderCodec,
                             (const unsigned char *)encoderCodec->descr, 
                             strlen(encoderCodec->descr));
  }

  else if (pluginData->capabilityMatchFunction != NULL) 
    return new H323CodecPluginNonStandardVideoCapability(encoderCodec, decoderCodec,
                             (H323NonStandardCapabilityInfo::CompareFuncType)pluginData->capabilityMatchFunction,
                             pluginData->data, pluginData->dataLength);
  else
    return new H323CodecPluginNonStandardVideoCapability(
                             encoderCodec, decoderCodec,
                             pluginData->data, pluginData->dataLength);
}

H323Capability *CreateGenericVideoCap(
  PluginCodec_Definition * encoderCodec,  
  PluginCodec_Definition * decoderCodec,
  int /*subType*/) 
{
  PluginCodec_H323GenericCodecData * pluginData = (PluginCodec_H323GenericCodecData *)encoderCodec->h323CapabilityData;

  if (pluginData == NULL ) {
	  PTRACE(1, "Generic codec information for codec '"<<encoderCodec->descr<<"' has NULL data field");
	  return NULL;
  }
  return new H323CodecPluginGenericVideoCapability(encoderCodec, decoderCodec, pluginData);
}


H323Capability * CreateH261Cap(
  PluginCodec_Definition * encoderCodec, 
  PluginCodec_Definition * decoderCodec,
  int /*subType*/) 
{
  PluginCodec_H323VideoH261 * pluginData =  (PluginCodec_H323VideoH261 *)encoderCodec->h323CapabilityData;
  return new H323H261PluginCapability(encoderCodec, decoderCodec, pluginData);
}

#if 0
H323Capability * CreateH263Cap(
  PluginCodec_Definition * encoderCodec, 
  PluginCodec_Definition * decoderCodec,
  int /*subType*/) 
{
  PluginCodec_H323VideoH263 * pluginData =  (PluginCodec_H323VideoH263 *)encoderCodec->h323CapabilityData;
  return new H323H263PluginCapability(encoderCodec, decoderCodec, pluginData);
}
#endif

#endif // OPAL_VIDEO

/////////////////////////////////////////////////////////////////////////////

#if 0
H323Codec * H323PluginCapabilityInfo::CreateCodec(H323Codec::Direction direction) const
{  
  // allow use of this class for external codec capabilities
  if (encoderCodec == NULL || decoderCodec == NULL)
    return NULL;

  PluginCodec_Definition * codec = (direction == H323Codec::Encoder) ? encoderCodec : decoderCodec;

  switch (codec->flags & PluginCodec_MediaTypeMask) {

    case PluginCodec_MediaTypeAudio:
#if OPAL_AUDIO
      PTRACE(3, "H323PLUGIN\tCreating framed audio codec " << mediaFormat << " from plugin");
      return new H323PluginFramedAudioCodec(mediaFormat, direction, codec);
#endif  // OPAL_AUDIO

    case PluginCodec_MediaTypeAudioStreamed:
#if OPAL_AUDIO
      PTRACE(3, "H323PLUGIN\tAudio plugins disabled");
      return NULL;
#else
      {
        PTRACE(3, "H323PLUGIN\tCreating audio codec " << mediaFormat << " from plugin");
        int bitsPerSample = (codec->flags & PluginCodec_BitsPerSampleMask) >> PluginCodec_BitsPerSamplePos;
        if (bitsPerSample == 0)
          bitsPerSample = 16;
        return new H323StreamedPluginAudioCodec(
                                mediaFormat, 
                                direction, 
                                codec->samplesPerFrame,
                                bitsPerSample,
                                codec);
      }
#endif  // OPAL_AUDIO

    case PluginCodec_MediaTypeVideo:
#if OPAL_VIDEO
      PTRACE(3, "H323PLUGIN\tVideo plugins disabled");
      return NULL;
#else
      if (
           (
             (direction == H323Codec::Encoder) &&
             (
               ((codec->flags & PluginCodec_InputTypeMask) != PluginCodec_InputTypeRaw) ||
               ((codec->flags & PluginCodec_OutputTypeMask) != PluginCodec_OutputTypeRTP)
             )
           )
           ||
           (
             (direction != H323Codec::Encoder) &&
             (
               ((codec->flags & PluginCodec_InputTypeMask) != PluginCodec_InputTypeRTP) ||
               ((codec->flags & PluginCodec_OutputTypeMask) != PluginCodec_OutputTypeRaw)
             )
           )
         ) {
          PTRACE(3, "H323PLUGIN\tVideo codec " << mediaFormat << " has incorrect input/output types");
          return NULL;
      }
      PTRACE(3, "H323PLUGIN\tCreating video codec " << mediaFormat << "from plugin");
      return new H323PluginVideoCodec(mediaFormat, direction, codec);
#endif // OPAL_VIDEO
    default:
      break;
  }

  PTRACE(3, "H323PLUGIN\tCannot create codec for unknown plugin codec media format " << (int)(codec->flags & PluginCodec_MediaTypeMask));
  return NULL;
}
#endif

/////////////////////////////////////////////////////////////////////////////

H323PluginCapabilityInfo::H323PluginCapabilityInfo(PluginCodec_Definition * _encoderCodec,
                                                   PluginCodec_Definition * _decoderCodec)
 : encoderCodec(_encoderCodec),
   decoderCodec(_decoderCodec),
   capabilityFormatName(CreateCodecName(_encoderCodec, TRUE)),
   mediaFormat(CreateCodecName(_encoderCodec, FALSE))
{
}

H323PluginCapabilityInfo::H323PluginCapabilityInfo(const PString & _mediaFormat, const PString & _baseName)
 : encoderCodec(NULL),
   decoderCodec(NULL),
   capabilityFormatName(CreateCodecName(_baseName, TRUE)),
   mediaFormat(_mediaFormat)
{
}

#if OPAL_AUDIO

/////////////////////////////////////////////////////////////////////////////

H323CodecPluginNonStandardAudioCapability::H323CodecPluginNonStandardAudioCapability(
    PluginCodec_Definition * _encoderCodec,
    PluginCodec_Definition * _decoderCodec,
    H323NonStandardCapabilityInfo::CompareFuncType compareFunc,
    const unsigned char * data, unsigned dataLen)
 : H323NonStandardAudioCapability(compareFunc,data, dataLen), 
   H323PluginCapabilityInfo(_encoderCodec, _decoderCodec)
{
  SetTxFramesInPacket(_decoderCodec->maxFramesPerPacket);

  PluginCodec_H323NonStandardCodecData * nonStdData = (PluginCodec_H323NonStandardCodecData *)_encoderCodec->h323CapabilityData;
  if (nonStdData->objectId != NULL) {
    oid = PString(nonStdData->objectId);
  } else {
    t35CountryCode   = nonStdData->t35CountryCode;
    t35Extension     = nonStdData->t35Extension;
    manufacturerCode = nonStdData->manufacturerCode;
  }
}

H323CodecPluginNonStandardAudioCapability::H323CodecPluginNonStandardAudioCapability(
    PluginCodec_Definition * _encoderCodec,
    PluginCodec_Definition * _decoderCodec,
    const unsigned char * data, unsigned dataLen)
 : H323NonStandardAudioCapability(data, dataLen), 
   H323PluginCapabilityInfo(_encoderCodec, _decoderCodec)
{
  SetTxFramesInPacket(_decoderCodec->maxFramesPerPacket);
  PluginCodec_H323NonStandardCodecData * nonStdData = (PluginCodec_H323NonStandardCodecData *)_encoderCodec->h323CapabilityData;
  if (nonStdData->objectId != NULL) {
    oid = PString(nonStdData->objectId);
  } else {
    t35CountryCode   = nonStdData->t35CountryCode;
    t35Extension     = nonStdData->t35Extension;
    manufacturerCode = nonStdData->manufacturerCode;
  }
}

/////////////////////////////////////////////////////////////////////////////

H323CodecPluginGenericAudioCapability::H323CodecPluginGenericAudioCapability(
    const PluginCodec_Definition * _encoderCodec,
    const PluginCodec_Definition * _decoderCodec,
    const PluginCodec_H323GenericCodecData *data )
	: H323GenericAudioCapability(data -> standardIdentifier, data -> maxBitRate),
	  H323PluginCapabilityInfo((PluginCodec_Definition *)_encoderCodec, (PluginCodec_Definition *) _decoderCodec)
{
  SetTxFramesInPacket(_decoderCodec->maxFramesPerPacket);
  const PluginCodec_H323GenericParameterDefinition *ptr = data -> params;

  unsigned i;
  for (i = 0; i < data -> nParameters; i++) {
    switch(ptr->type) {
	    case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_ShortMin:
	    case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_ShortMax:
	    case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_LongMin:
	    case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_LongMax:
	      AddIntegerGenericParameter(ptr->collapsing,ptr->id,ptr->type, ptr->value.integer);
	      break;
	
	    case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_Logical:
	    case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_Bitfield:
	    case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_OctetString:
	    case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_GenericParameter:
	    default:
	      PTRACE(1,"Unsupported Generic parameter type "<< ptr->type << " for generic codec " << _encoderCodec->descr );
	      break;
    }
    ptr++;
  }
}

/////////////////////////////////////////////////////////////////////////////

PObject::Comparison H323GSMPluginCapability::Compare(const PObject & obj) const
{
  if (!PIsDescendant(&obj, H323GSMPluginCapability))
    return LessThan;

  Comparison result = H323AudioCapability::Compare(obj);
  if (result != EqualTo)
    return result;

  const H323GSMPluginCapability& other = (const H323GSMPluginCapability&)obj;
  if (scrambled < other.scrambled)
    return LessThan;
  if (comfortNoise < other.comfortNoise)
    return LessThan;
  return EqualTo;
}


BOOL H323GSMPluginCapability::OnSendingPDU(H245_AudioCapability & cap, unsigned packetSize) const
{
  cap.SetTag(pluginSubType);
  H245_GSMAudioCapability & gsm = cap;
  gsm.m_audioUnitSize = packetSize * encoderCodec->bytesPerFrame;
  gsm.m_comfortNoise  = comfortNoise;
  gsm.m_scrambled     = scrambled;

  return TRUE;
}


BOOL H323GSMPluginCapability::OnReceivedPDU(const H245_AudioCapability & cap, unsigned & packetSize)
{
  const H245_GSMAudioCapability & gsm = cap;
  packetSize   = gsm.m_audioUnitSize / encoderCodec->bytesPerFrame;
  if (packetSize == 0)
    packetSize = 1;

  scrambled    = gsm.m_scrambled;
  comfortNoise = gsm.m_comfortNoise;

  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

#endif   // OPAL_AUDIO

#if OPAL_VIDEO

/////////////////////////////////////////////////////////////////////////////

PObject::Comparison H323H261PluginCapability::Compare(const PObject & obj) const
{
  if (!PIsDescendant(&obj, H323H261PluginCapability))
    return LessThan;

  Comparison result = H323Capability::Compare(obj);
  if (result != EqualTo)
    return result;

  const H323H261PluginCapability & other = (const H323H261PluginCapability &)obj;

  if (((qcifMPI > 0) && (other.qcifMPI > 0)) ||
      ((cifMPI  > 0) && (other.cifMPI > 0)))
    return EqualTo;

  if (qcifMPI > 0)
    return LessThan;

  return GreaterThan;
}


BOOL H323H261PluginCapability::OnSendingPDU(H245_VideoCapability & cap) const
{
  cap.SetTag(H245_VideoCapability::e_h261VideoCapability);

  H245_H261VideoCapability & h261 = cap;
  if (qcifMPI > 0) {
    h261.IncludeOptionalField(H245_H261VideoCapability::e_qcifMPI);
    h261.m_qcifMPI = qcifMPI;
  }
  if (cifMPI > 0) {
    h261.IncludeOptionalField(H245_H261VideoCapability::e_cifMPI);
    h261.m_cifMPI = cifMPI;
  }
  h261.m_temporalSpatialTradeOffCapability = temporalSpatialTradeOffCapability;
  h261.m_maxBitRate = maxBitRate;
  h261.m_stillImageTransmission = stillImageTransmission;

  return TRUE;
}


BOOL H323H261PluginCapability::OnSendingPDU(H245_VideoMode & pdu) const
{
  pdu.SetTag(H245_VideoMode::e_h261VideoMode);
  H245_H261VideoMode & mode = pdu;
  mode.m_resolution.SetTag(cifMPI > 0 ? H245_H261VideoMode_resolution::e_cif
                                      : H245_H261VideoMode_resolution::e_qcif);
  mode.m_bitRate = maxBitRate;
  mode.m_stillImageTransmission = stillImageTransmission;
  return TRUE;
}

BOOL H323H261PluginCapability::OnReceivedPDU(const H245_VideoCapability & cap)
{
  if (cap.GetTag() != H245_VideoCapability::e_h261VideoCapability)
    return FALSE;

  OpalMediaFormat & mediaFormat = GetWritableMediaFormat();

  const H245_H261VideoCapability & h261 = cap;
  if (h261.HasOptionalField(H245_H261VideoCapability::e_qcifMPI)) {
    qcifMPI = h261.m_qcifMPI;
    mediaFormat.SetOptionInteger(OpalMediaFormat::FrameTimeOption, OpalMediaFormat::VideoClockRate*100*qcifMPI/2997);
    mediaFormat.SetOptionInteger(OpalVideoFormat::FrameWidthOption, 176);
    mediaFormat.SetOptionInteger(OpalVideoFormat::FrameHeightOption, 144);
  }
  else
    qcifMPI = 0;

  if (h261.HasOptionalField(H245_H261VideoCapability::e_cifMPI)) {
    cifMPI = h261.m_cifMPI;
    mediaFormat.SetOptionInteger(OpalMediaFormat::FrameTimeOption, OpalMediaFormat::VideoClockRate*100*cifMPI/2997);
    mediaFormat.SetOptionInteger(OpalVideoFormat::FrameWidthOption, 352);
    mediaFormat.SetOptionInteger(OpalVideoFormat::FrameHeightOption, 288);
  }
  else
    cifMPI = 0;

  maxBitRate = h261.m_maxBitRate;
  mediaFormat.SetOptionInteger(OpalMediaFormat::MaxBitRateOption, maxBitRate*100);

  temporalSpatialTradeOffCapability = h261.m_temporalSpatialTradeOffCapability;
  stillImageTransmission = h261.m_stillImageTransmission;
  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

H323CodecPluginNonStandardVideoCapability::H323CodecPluginNonStandardVideoCapability(
    PluginCodec_Definition * _encoderCodec,
    PluginCodec_Definition * _decoderCodec,
    H323NonStandardCapabilityInfo::CompareFuncType compareFunc,
    const unsigned char * data, unsigned dataLen)
 : H323NonStandardVideoCapability(compareFunc, data, dataLen), 
   H323PluginCapabilityInfo(_encoderCodec, _decoderCodec)
{
  PluginCodec_H323NonStandardCodecData * nonStdData = (PluginCodec_H323NonStandardCodecData *)_encoderCodec->h323CapabilityData;
  if (nonStdData->objectId != NULL) {
    oid = PString(nonStdData->objectId);
  } else {
    t35CountryCode   = nonStdData->t35CountryCode;
    t35Extension     = nonStdData->t35Extension;
    manufacturerCode = nonStdData->manufacturerCode;
  }
}

H323CodecPluginNonStandardVideoCapability::H323CodecPluginNonStandardVideoCapability(
    PluginCodec_Definition * _encoderCodec,
    PluginCodec_Definition * _decoderCodec,
    const unsigned char * data, unsigned dataLen)
 : H323NonStandardVideoCapability(data, dataLen), 
   H323PluginCapabilityInfo(_encoderCodec, _decoderCodec)
{
  PluginCodec_H323NonStandardCodecData * nonStdData = (PluginCodec_H323NonStandardCodecData *)_encoderCodec->h323CapabilityData;
  if (nonStdData->objectId != NULL) {
    oid = PString(nonStdData->objectId);
  } else {
    t35CountryCode   = nonStdData->t35CountryCode;
    t35Extension     = nonStdData->t35Extension;
    manufacturerCode = nonStdData->manufacturerCode;
  }
}

/////////////////////////////////////////////////////////////////////////////

H323CodecPluginGenericVideoCapability::H323CodecPluginGenericVideoCapability(
    const PluginCodec_Definition * _encoderCodec,
    const PluginCodec_Definition * _decoderCodec,
    const PluginCodec_H323GenericCodecData *data )
	: H323GenericVideoCapability(data -> standardIdentifier, data -> maxBitRate),
	  H323PluginCapabilityInfo((PluginCodec_Definition *)_encoderCodec, (PluginCodec_Definition *) _decoderCodec)
{
  const PluginCodec_H323GenericParameterDefinition *ptr = data -> params;

  unsigned i;
  for (i = 0; i < data -> nParameters; i++) {
    switch(ptr->type) {
	    case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_ShortMin:
	    case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_ShortMax:
	    case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_LongMin:
	    case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_LongMax:
	      AddIntegerGenericParameter(ptr->collapsing,ptr->id,ptr->type, ptr->value.integer);
	      break;
	
	    case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_Logical:
	    case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_Bitfield:
	    case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_OctetString:
	    case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_GenericParameter:
	    default:
	      PTRACE(1,"Unsupported Generic parameter type "<< ptr->type << " for generic codec " << _encoderCodec->descr );
	      break;
    }
    ptr++;
  }
}

/////////////////////////////////////////////////////////////////////////////

#endif  // OPAL_VIDEO

#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////

OPALDynaLink::OPALDynaLink(const char * _baseName, const char * _reason)
  : baseName(_baseName), reason(_reason)
{
  isLoadedOK = FALSE;
}

void OPALDynaLink::Load()
{
  PStringArray dirs = PPluginManager::GetPluginDirs();
  PINDEX i;
  for (i = 0; !PDynaLink::IsLoaded() && i < dirs.GetSize(); i++)
    PLoadPluginDirectory<OPALDynaLink>(*this, dirs[i]);
  
  if (!PDynaLink::IsLoaded()) {
    cerr << "Cannot find " << baseName << " as required for " << ((reason != NULL) ? reason : " a code module") << "." << endl
         << "This function may appear to be installed, but will not operate correctly." << endl
         << "Please put the file " << baseName << PDynaLink::GetExtension() << " into one of the following directories:" << endl
         << "     " << setfill(',') << dirs << setfill(' ') << endl
         << "This list of directories can be set using the PWLIBPLUGINDIR environment variable." << endl;
    return;
  }
}

BOOL OPALDynaLink::LoadPlugin(const PString & filename)
{
  PFilePath fn = filename;
  if (fn.GetTitle() *= "libavcodec")
    return PDynaLink::Open(filename);
  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

static PAtomicInteger bootStrapCount = 0;

void OpalPluginCodecManager::Bootstrap()
{
  if (++bootStrapCount != 1)
    return;

#if defined(OPAL_AUDIO) || defined(OPAL_VIDEO)
//  OpalMediaFormatList & mediaFormatList = OpalPluginCodecManager::GetMediaFormatList();
#endif

#if OPAL_AUDIO

  //mediaFormatList += OpalMediaFormat(OpalG711uLaw);
  //mediaFormatList += OpalMediaFormat(OpalG711ALaw);

  //new OpalFixedCodecFactory<OpalG711ALaw64k_Encoder>::Worker(OpalG711ALaw64k_Encoder::GetFactoryName());
  //new OpalFixedCodecFactory<OpalG711ALaw64k_Decoder>::Worker(OpalG711ALaw64k_Decoder::GetFactoryName());

  //new OpalFixedCodecFactory<OpalG711uLaw64k_Encoder>::Worker(OpalG711uLaw64k_Encoder::GetFactoryName());
  //new OpalFixedCodecFactory<OpalG711uLaw64k_Decoder>::Worker(OpalG711uLaw64k_Decoder::GetFactoryName());

#endif // OPAL_AUDIO

#if 0 //OPAL_VIDEO
  // H.323 require an endpoint to have H.261 if it supports video
  //mediaFormatList += OpalMediaFormat("H.261");

#if RFC2190_AVCODEC
  // only have H.263 if RFC2190 is loaded
  //if (OpenH323_IsRFC2190Loaded())
  //  mediaFormatList += OpalMediaFormat("RFC2190 H.263");
#endif  // RFC2190_AVCODEC
#endif  // OPAL_VIDEO
}

/////////////////////////////////////////////////////////////////////////////

#define INCLUDE_STATIC_CODEC(name) \
extern "C" { \
extern unsigned int Opal_StaticCodec_##name##_GetAPIVersion(); \
extern struct PluginCodec_Definition * Opal_StaticCodec_##name##_GetCodecs(unsigned *,unsigned); \
}; \
class H323StaticPluginCodec_##name : public H323StaticPluginCodec \
{ \
  public: \
    PluginCodec_GetAPIVersionFunction Get_GetAPIFn() \
    { return &Opal_StaticCodec_##name##_GetAPIVersion; } \
    PluginCodec_GetCodecFunction Get_GetCodecFn() \
    { return &Opal_StaticCodec_##name##_GetCodecs; } \
}; \
static PFactory<H323StaticPluginCodec>::Worker<H323StaticPluginCodec_##name > static##name##CodecFactory( #name ); \

#ifdef H323_EMBEDDED_GSM

INCLUDE_STATIC_CODEC(GSM_0610)

#endif

