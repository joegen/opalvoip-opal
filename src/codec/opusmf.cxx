/*
 * opusmf.cxx
 *
 * Opus Media Format descriptions
 *
 * Open Phone Abstraction Library
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2008 Vox Lucida
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
 * The Original Code is Open Phone Abstraction Library
 *
 * The Initial Developer of the Original Code is Vox Lucida
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#include "opusmf_inc.cxx"

#include <opal/mediafmt.h>
#include <codec/opalplugin.h>
//#include <h323/h323caps.h>


class OpalOpusFormatInternal : public OpalAudioFormatInternal
{
  public:
    OpalOpusFormatInternal(const char * formatName, unsigned sampleRate, unsigned channels)
      : OpalAudioFormatInternal(formatName,
                                RTP_DataFrame::DynamicBase,
                                OpusEncodingName,
                                MAX_BIT_RATE*OPUS_FRAME_MS/1000/8,
                                OPUS_FRAME_MS,
                                1, 1, 1,
                                sampleRate, 0, channels)
    {
#if OPAL_SDP
      OpalMediaOption * option;

      option = new OpalMediaOptionBoolean(UseInBandFEC_OptionName, true, OpalMediaOption::AndMerge, DEFAULT_USE_DTX);
      option->SetFMTP(UseInBandFEC_FMTPName, NULL);
      AddOption(option);

      option = new OpalMediaOptionBoolean(UseDTX_OptionName, true, OpalMediaOption::AndMerge, DEFAULT_USE_DTX);
      option->SetFMTP(UseDTX_FMTPName, "0");
      AddOption(option);

      option = new OpalMediaOptionBoolean(ConstantBitRate_OptionName, true, OpalMediaOption::AndMerge, DEFAULT_CBR);
      option->SetFMTP(ConstantBitRate_FMTPName, "0");
      AddOption(option);

      option = new OpalMediaOptionInteger(MaxPlaybackRate_OptionName, true, OpalMediaOption::MinMerge, OPUS_SAMPLE_RATE, MIN_SAMPLE_RATE, MAX_SAMPLE_RATE);
      option->SetFMTP(MaxPlaybackRate_FMTPName, STRINGIZE(MAX_SAMPLE_RATE));
      AddOption(option);

      option = new OpalMediaOptionInteger(MaxCaptureRate_OptionName, true, OpalMediaOption::MinMerge, OPUS_SAMPLE_RATE, MIN_SAMPLE_RATE, MAX_SAMPLE_RATE);
      option->SetFMTP(MaxCaptureRate_FMTPName, STRINGIZE(MAX_SAMPLE_RATE));
      AddOption(option);

      option = new OpalMediaOptionBoolean(PlaybackStereo_OptionName, true, OpalMediaOption::AndMerge, DEFAULT_STEREO);
      option->SetFMTP(PlaybackStereo_FMTPName, NULL);
      AddOption(option);

      option = new OpalMediaOptionBoolean(CaptureStereo_OptionName, true, OpalMediaOption::AndMerge, DEFAULT_STEREO);
      option->SetFMTP(CaptureStereo_FMTPName, NULL);
      AddOption(option);

      option = FindOption(OpalMediaFormat::MaxBitRateOption());
      option->SetFMTP(MaxAverageBitRate_FMTPName, NULL);
#endif
    }


    virtual PObject * Clone() const
    {
      return new OpalOpusFormatInternal(*this);
    }
};


#define CHANNEL  1
#define CHANNELS 2
#define DEF_MEDIA_FORMAT(rate,stereo) \
  const OpalAudioFormat & GetOpalOpus##rate##stereo() \
  { \
    static OpalMediaFormatStatic<OpalAudioFormat> format(new OpalOpusFormatInternal(OPAL_OPUS##rate##stereo, rate*1000, CHANNEL##stereo)); \
    return format; \
  }

DEF_MEDIA_FORMAT( 8, );
DEF_MEDIA_FORMAT( 8,S);
DEF_MEDIA_FORMAT(12, );
DEF_MEDIA_FORMAT(12,S);
DEF_MEDIA_FORMAT(16, );
DEF_MEDIA_FORMAT(16,S);
DEF_MEDIA_FORMAT(24, );
DEF_MEDIA_FORMAT(24,S);
DEF_MEDIA_FORMAT(48, );
DEF_MEDIA_FORMAT(48,S);


struct OpalAudioFrameDetectorOpus : OpalAudioFormat::FrameDetector
{
  static const BYTE * ParseFramePtr(const BYTE * & payloadPtr, PINDEX & payloadLen)
  {
    if (payloadLen < 1)
      return NULL;

    const BYTE * framePtr;
    PINDEX frameLen;
    if (payloadPtr[0] < 252) {
      frameLen = payloadPtr[0];
      framePtr = ++payloadPtr;
      --payloadLen;
    }
    else {
      if (payloadLen < 2)
        return NULL;

      frameLen = 4 * payloadPtr[1] + payloadPtr[0];
      framePtr = payloadPtr += 2;
      payloadLen -= 2;
    }

    if (frameLen == 0 || frameLen > payloadLen)
      return NULL;

    payloadPtr += frameLen;
    payloadLen -= frameLen;
    return framePtr;
  }

  virtual OpalAudioFormat::FrameType GetFrameType(const BYTE * payloadPtr, PINDEX payloadLen, unsigned sampleRate)
  {
    if (payloadLen < 2)
      return OpalAudioFormat::e_UnknownFrameType;

    unsigned toc = *payloadPtr++;
    --payloadLen;

    // In CELT_ONLY mode, packets does not have VAD.
    if (toc & 0x80)
      return OpalAudioFormat::e_NormalFrame;

    unsigned channels = (toc & 0x4) ? 2 : 1;

    unsigned subFrames;
    if ((toc & 0x60) == 0x60)
      subFrames = 1;
    else {
      subFrames = ((toc >> 3) & 0x3);
      if (subFrames != 3)
        subFrames = ((sampleRate << subFrames) * 10 / sampleRate + 19) / 20;
    }


    PINDEX frameCount;
    static PINDEX const MaxFrames = 6;
    const BYTE * framePtr[MaxFrames];
    switch (toc & 0x3) {
      case 0:
        frameCount = 1;
        framePtr[0] = payloadPtr;
        break;

      case 1: // Two CBR frames
        frameCount = 2;
        if (payloadLen & 1)
          return OpalAudioFormat::e_UnknownFrameType;
        framePtr[0] = payloadPtr;
        framePtr[1] = payloadPtr + payloadLen / 2;
        break;

      case 2: // Two VBR frames
        frameCount = 2;
        if ((framePtr[0] = ParseFramePtr(payloadPtr, payloadLen)) == NULL)
            return OpalAudioFormat::e_UnknownFrameType;
        framePtr[1] = payloadPtr;
        break;

      default: // Multiple CBR/VBR frames (from 0 to 120 ms)
        if (--payloadLen == 0)
          return OpalAudioFormat::e_UnknownFrameType;
        unsigned toc2 = *payloadPtr++;

        frameCount = toc2 & 0x3F;  // Number of frames encoded in bits 0 to 5
        if (frameCount <= 0 || frameCount > MaxFrames)
          return OpalAudioFormat::e_UnknownFrameType;

        if (toc2 & 0x40) { // Padding flag is bit 6
          PINDEX totalPad = 0;
          for (;;) {
            if (--payloadLen == 0)
              return OpalAudioFormat::e_UnknownFrameType;
            unsigned padByte = *payloadPtr++;
            if (padByte != 255) {
              totalPad += padByte;
              break;
            }
            totalPad += 254;
          }

          if (payloadLen < totalPad)
            return OpalAudioFormat::e_UnknownFrameType;
          payloadLen -= totalPad;
        }

        if (toc2 & 0x80) {
          // VBR case
          for (PINDEX i = 0; i < frameCount; i++) {
            if ((framePtr[i] = ParseFramePtr(payloadPtr, payloadLen)) == NULL)
              return OpalAudioFormat::e_UnknownFrameType;
          }
        }
        else {
          // CBR case
          PINDEX frameSize = payloadLen/frameCount;
          if (frameSize*frameCount != frameCount)
            return OpalAudioFormat::e_UnknownFrameType;
          for (PINDEX i = 0; i < frameCount; i++)
            framePtr[i] = payloadPtr+frameSize*i;
        }
    }

    bool vad = false;
    OpalAudioFormat::FrameType frameType = OpalAudioFormat::e_NormalFrame;
    for (PINDEX frame = 0; frame < frameCount; ++frame) {
      for (unsigned chan = 0; chan < channels; chan++) {
        // Highest "subFrames" bits are VAD, next bit is LBRR flag, repeated for each channel
        unsigned offset = (subFrames+1)*chan;
        if (framePtr[frame][0] & (0x80 >> (offset + subFrames)))
          frameType |= OpalAudioFormat::e_FECFrame;
        if (framePtr[frame][0] & (((0xFF << (8-subFrames))&0xFF) >> offset))
          vad = true;
      }
    }

    if (!vad)
      frameType |= OpalAudioFormat::e_SilenceFrame;
    return frameType;
  }
};

PFACTORY_CREATE(OpalAudioFormat::FrameDetectFactory, OpalAudioFrameDetectorOpus, "OPUS");


// End of File ///////////////////////////////////////////////////////////////
