/*
 * main.cxx
 *
 * OPAL codec info program
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
 * The Original Code is Open H323
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: main.cxx,v $
 * Revision 1.12  2007/09/20 00:34:37  rjongbloed
 * Moved samples binary to common directory to make it easier to clean.
 * Fixed "No Trace" builds.
 *
 * Revision 1.11  2007/09/12 04:19:53  rjongbloed
 * CHanges to avoid creation of long duration OpalMediaFormat instances, eg in
 *   the plug in capabilities, that then do not get updated values from the master
 *   list, or worse from the user modified master list, causing much confusion.
 *
 * Revision 1.10  2007/09/10 03:15:04  rjongbloed
 * Fixed issues in creating and subsequently using correctly unique
 *   payload types in OpalMediaFormat instances and transcoders.
 *
 * Revision 1.9  2007/06/25 05:09:09  rjongbloed
 * Fixed compile after change to anonymous structure in header (for GCC).
 *
 * Revision 1.8  2007/06/22 05:49:44  rjongbloed
 * Major codec API update:
 *   Automatically map OpalMediaOptions to SIP/SDP FMTP parameters.
 *   Automatically map OpalMediaOptions to H.245 Generic Capability parameters.
 *   Largely removed need to distinguish between SIP and H.323 codecs.
 *   New mechanism for setting OpalMediaOptions from within a plug in.
 * Added test for video codec, feed simple raw frames and output the
 *   encoded RTP packets, then feed them back to decoder.
 * Added display of media format information, epecially option list.
 *
 * Revision 1.7  2007/05/20 05:56:26  rjongbloed
 * Added ability to debug audio plug in codecs encode/decode function.
 * General clean up.
 *
 * Revision 1.6  2006/10/02 22:04:19  rjongbloed
 * Added LID plug ins
 *
 * Revision 1.5  2006/09/06 22:36:11  csoutheren
 * Fix problem with IsValidForProtocol on video codecs
 *
 * Revision 1.4  2006/09/05 22:50:57  csoutheren
 * Show protocol valid for status
 *
 * Revision 1.3  2006/07/24 14:03:39  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 1.1.2.6  2006/04/26 05:09:57  csoutheren
 * Cleanup bit rate settings
 *
 * Revision 1.1.2.5  2006/04/19 07:52:30  csoutheren
 * Add ability to have SIP-only and H.323-only codecs, and implement for H.261
 *
 * Revision 1.1.2.4  2006/04/06 01:21:18  csoutheren
 * More implementation of video codec plugins
 *
 * Revision 1.1.2.3  2006/03/23 07:55:54  csoutheren
 * Added in options for capability handling
 *
 * Revision 1.1.2.2  2006/03/20 05:04:54  csoutheren
 * Add check to ensure codec can be instantiated
 *
 * Revision 1.1.2.1  2006/03/16 07:06:27  csoutheren
 * Update for new code
 *
 * Revision 1.2  2006/06/29 08:47:20  csoutheren
 * Removed compiler warning
 *
 * Revision 1.1  2006/03/13 06:56:37  csoutheren
 * Initial version
 *
 */

#include <ptlib.h>

#include <opal/transcoders.h>
#include <codec/opalplugin.h>
#include <codec/opalpluginmgr.h>
#include <codec/vidcodec.h>

#include <ptclib/dtmf.h>
#include <ptlib/videoio.h>


class OpalCodecInfo : public PProcess
{
  PCLASSINFO(OpalCodecInfo, PProcess)

  public:
    OpalCodecInfo();

    void Main();
};

#define new PNEW

PCREATE_PROCESS(OpalCodecInfo)


///////////////////////////////////////////////////////////////

OpalCodecInfo::OpalCodecInfo()
  : PProcess("Post Increment", "OpalCodecInfo")
{
}


PString DisplayLicenseType(int type)
{
  PString str;
  switch (type) {
    case PluginCodec_Licence_None:
      str = "No license";
      break;
    case PluginCodec_License_GPL:
      str = "GPL";
      break;
    case PluginCodec_License_MPL:
      str = "MPL";
      break;
    case PluginCodec_License_Freeware:
      str = "Freeware";
      break;
    case PluginCodec_License_ResearchAndDevelopmentUseOnly:
      str = "Research and development use only";
      break;
    case PluginCodec_License_BSD:
      str = "BSD";
      break;
    case PluginCodec_License_LGPL:
      str = "LGPL";
      break;
    default:
      if (type <= PluginCodec_License_NoRoyalties)
        str = "No royalty license";
      else
        str = "Requires royalties";
      break;
  }
  return str;
}


PString DisplayableString(const char * str)
{
  if (str == NULL)
    return PString("(none)");
  return PString(str);
}


PString DisplayLicenseInfo(PluginCodec_information * info)
{
  PStringStream str;
  if (info == NULL) 
    str << "    None" << endl;
  else {
    str << "  License" << endl
        << "    Timestamp: " << PTime().AsString(PTime::RFC1123) << endl
        << "    Source" << endl
        << "      Author:       " << DisplayableString(info->sourceAuthor) << endl
        << "      Version:      " << DisplayableString(info->sourceVersion) << endl
        << "      Email:        " << DisplayableString(info->sourceEmail) << endl
        << "      URL:          " << DisplayableString(info->sourceURL) << endl
        << "      Copyright:    " << DisplayableString(info->sourceCopyright) << endl
        << "      License:      " << DisplayableString(info->sourceLicense) << endl
        << "      License Type: " << DisplayLicenseType(info->sourceLicenseCode) << endl
        << "    Codec" << endl
        << "      Description:  " << DisplayableString(info->codecDescription) << endl
        << "      Author:       " << DisplayableString(info->codecAuthor) << endl
        << "      Version:      " << DisplayableString(info->codecVersion) << endl
        << "      Email:        " << DisplayableString(info->codecEmail) << endl
        << "      URL:          " << DisplayableString(info->codecURL) << endl
        << "      Copyright:    " << DisplayableString(info->codecCopyright) << endl
        << "      License:      " << DisplayableString(info->codecLicense) << endl
        << "      License Type: " << DisplayLicenseType(info->codecLicenseCode) << endl;
  }
  return str;
}


void DisplayCodecDefn(PluginCodec_Definition & defn)
{
  BOOL isVideo = FALSE;

  cout << "  Version:             " << defn.version << endl
       << DisplayLicenseInfo(defn.info)
       << "  Flags:               ";
  switch (defn.flags & PluginCodec_MediaTypeMask) {
    case PluginCodec_MediaTypeAudio:
      cout << "Audio";
      break;
    case PluginCodec_MediaTypeVideo:
      cout << "Video";
      isVideo = TRUE;
      break;
    case PluginCodec_MediaTypeAudioStreamed:
      cout << "Streamed audio (" << ((defn.flags & PluginCodec_BitsPerSampleMask) >> PluginCodec_BitsPerSamplePos) << " bits per sample)";
      break;
    default:
      cout << "unknown type " << (defn.flags & PluginCodec_MediaTypeMask) << endl;
      break;
  }
  cout << ", " << ((defn.flags & PluginCodec_InputTypeRTP) ? "RTP input" : "raw input");
  cout << ", " << ((defn.flags & PluginCodec_OutputTypeRTP) ? "RTP output" : "raw output");
  cout << ", " << ((defn.flags & PluginCodec_RTPTypeExplicit) ? "explicit" : "dynamic") << " RTP payload";
  cout << endl;

  cout << "  Description:         " << defn.descr << endl
       << "  Source format:       " << defn.sourceFormat << endl
       << "  Dest format:         " << defn.destFormat << endl
       << "  Userdata:            " << (void *)defn.userData << endl
       << "  Sample rate:         " << defn.sampleRate << endl
       << "  Bits/sec:            " << defn.bitsPerSec << endl
       << "  Frame time (us):     " << defn.usPerFrame << endl;
  if (!isVideo) {
    cout << "  Samples/frame:       " << defn.parm.audio.samplesPerFrame << endl
         << "  Bytes/frame:         " << defn.parm.audio.bytesPerFrame << endl
         << "  Frames/packet:       " << defn.parm.audio.recommendedFramesPerPacket << endl
         << "  Frames/packet (max): " << defn.parm.audio.maxFramesPerPacket << endl;
  }
  else {
    cout << "  Frame width:         " << defn.parm.video.maxFrameWidth << endl
         << "  Frame height:        " << defn.parm.video.maxFrameHeight << endl
         << "  Frame rate:          " << defn.parm.video.recommendedFrameRate << endl
         << "  Frame rate (max):    " << defn.parm.video.maxFrameRate << endl;
  }
  cout << "  RTP payload:         ";

  if (defn.flags & PluginCodec_RTPTypeExplicit)
    cout << (int)defn.rtpPayload << endl;
  else
    cout << "(not used)";

  cout << endl
       << "  SDP format:          " << DisplayableString(defn.sdpFormat) << endl;

  cout << "  Create function:     " << (void *)defn.createCodec << endl
       << "  Destroy function:    " << (void *)defn.destroyCodec << endl
       << "  Encode function:     " << (void *)defn.codecFunction << endl;

  cout << "  Codec controls:      ";
  if (defn.codecControls == NULL)
    cout << "(none)" << endl;
  else {
    cout << endl;
    PluginCodec_ControlDefn * ctls = defn.codecControls;
    while (ctls->name != NULL) {
      cout << "    " << ctls->name << "(" << (void *)ctls->control << ")" << endl;
      ++ctls;
    }
  }

  static char * capabilityNames[] = {
    "not defined",
    "programmed",
    "nonStandard",
    "generic",

    // audio codecs
    "G.711 Alaw 64k",
    "G.711 Alaw 56k",
    "G.711 Ulaw 64k",
    "G.711 Ulaw 56k",
    "G.722 64k",
    "G.722 56k",
    "G.722 48k",
    "G.723.1",
    "G.728",
    "G.729",
    "G.729 Annex A",
    "is11172",
    "is13818 Audio",
    "G.729 Annex B",
    "G.729 Annex A and Annex B",
    "G.723.1 Annex C",
    "GSM Full Rate",
    "GSM Half Rate",
    "GSM Enhanced Full Rate",
    "G.729 Extensions",

    // video codecs
    "H.261",
    "H.262",
    "H.263",
    "is11172"
  };

  cout << "  Capability type:     ";
  if (defn.h323CapabilityType < (sizeof(capabilityNames) / sizeof(capabilityNames[0])))
    cout << capabilityNames[defn.h323CapabilityType];
  else
    cout << "Unknown capability code " << defn.h323CapabilityType;

  switch (defn.h323CapabilityType) {
    case PluginCodec_H323Codec_undefined:
      break;

    case PluginCodec_H323Codec_nonStandard:
      if (defn.h323CapabilityData == NULL)
        cout << ", no data" << endl;
      else {
        struct PluginCodec_H323NonStandardCodecData * data =
            (struct PluginCodec_H323NonStandardCodecData *)defn.h323CapabilityData;
        if (data->objectId != NULL) 
          cout << ", objectID=" << data->objectId;
        else
          cout << ", t35=" << (int)data->t35CountryCode << "," << (int)data->t35Extension << "," << (int)data->manufacturerCode;
        if ((data->data == NULL) || (data->dataLength == 0))
          cout << ", empty data";
        else {
          BOOL printable = TRUE;
          for (unsigned i = 0; i < data->dataLength; ++i) 
            printable = printable && (0x20 <= data->data[i] && data->data[i] < 0x7e);
          if (printable)
            cout << ", data=" << PString((const char *)data->data, data->dataLength);
          else
            cout << ", " << (int)data->dataLength << " bytes";
        }
        if (data->capabilityMatchFunction != NULL) 
          cout << ", match function provided";
        cout << endl;
      }
      break;

    case PluginCodec_H323Codec_programmed:
    case PluginCodec_H323Codec_generic:
    case PluginCodec_H323AudioCodec_g711Alaw_64k:
    case PluginCodec_H323AudioCodec_g711Alaw_56k:
    case PluginCodec_H323AudioCodec_g711Ulaw_64k:
    case PluginCodec_H323AudioCodec_g711Ulaw_56k:
    case PluginCodec_H323AudioCodec_g722_64k:
    case PluginCodec_H323AudioCodec_g722_56k:
    case PluginCodec_H323AudioCodec_g722_48k:
    case PluginCodec_H323AudioCodec_g728:
    case PluginCodec_H323AudioCodec_g729:
    case PluginCodec_H323AudioCodec_g729AnnexA:
    case PluginCodec_H323AudioCodec_g729wAnnexB:
    case PluginCodec_H323AudioCodec_g729AnnexAwAnnexB:
      if (defn.h323CapabilityData != NULL)
        cout << (void *)defn.h323CapabilityData << endl;
      break;

    case PluginCodec_H323AudioCodec_g7231:
      cout << "silence supression " << ((defn.h323CapabilityData > 0) ? "on" : "off") << endl;
      break;

    case PluginCodec_H323AudioCodec_is11172:
    case PluginCodec_H323AudioCodec_is13818Audio:
    case PluginCodec_H323AudioCodec_g729Extensions:
      cout << "ignored";
      break;

    case PluginCodec_H323AudioCodec_g7231AnnexC:
      if (defn.h323CapabilityData == NULL)
        cout << ", no data" << endl;
      else 
        cout << ", not supported" << endl;
      break;

    case PluginCodec_H323AudioCodec_gsmFullRate:
    case PluginCodec_H323AudioCodec_gsmHalfRate:
    case PluginCodec_H323AudioCodec_gsmEnhancedFullRate:
      if (defn.h323CapabilityData == NULL)
        cout << ", no data -> default comfortNoise=0, scrambled=0" << endl;
      else {
        struct PluginCodec_H323AudioGSMData * data =
            (struct PluginCodec_H323AudioGSMData *)defn.h323CapabilityData;
        cout << ", comfort noise=" << data->comfortNoise << ", scrambled=" << data->scrambled;
        cout << endl;
      }
      break;

    case PluginCodec_H323VideoCodec_h261:
    case PluginCodec_H323VideoCodec_h263:
      if (defn.h323CapabilityData != NULL)
        cout << ", warning-non-NULL h323capData";
      cout << ", TODO: adding in codec control here" << endl;
      break;

    case PluginCodec_H323VideoCodec_h262:
    case PluginCodec_H323VideoCodec_is11172:
      if (defn.h323CapabilityData != NULL)
        cout << ", warning-non-NULL h323capData" << endl;
      break;

    case PluginCodec_H323Codec_NoH323:
      cout << "H323 capability not created" << endl;
      break;

    default:
      cout << " unknown code (" << (int)defn.h323CapabilityType << endl;
      break;
  }
}


void DisplayMediaFormats(const OpalMediaFormatList & mediaList)
{
  PINDEX i;
  PINDEX max_len = 0;
  for (i = 0; i < mediaList.GetSize(); i++) {
    PINDEX len = mediaList[i].GetLength();
    if (len > max_len)
      max_len = len;
  }

  for (i = 0; i < mediaList.GetSize(); i++) {
    OpalMediaFormat & fmt = mediaList[i];
    PString str = ", h323=";
    str += fmt.IsValidForProtocol("h.323") ? "yes" : "no";
    str += ", sip=";
    str += fmt.IsValidForProtocol("sip") ? "yes" : "no";
    cout << setw(max_len+2) << fmt << "  ";
    switch (fmt.GetDefaultSessionID()) {
      case OpalMediaFormat::DefaultAudioSessionID:
        cout << "audio, bandwidth=" << fmt.GetBandwidth() << ", RTP=" << fmt.GetPayloadType() << str << endl;
        break;
      case OpalMediaFormat::DefaultVideoSessionID:
        cout << "video, bandwidth=" << fmt.GetBandwidth() << ", RTP=" << fmt.GetPayloadType() << str << endl;
        break;
      case OpalMediaFormat::DefaultDataSessionID:
        cout << "data" << endl;;
        break;
      default:
        cout << "unknown type " << hex << mediaList[i].GetDefaultSessionID() << dec << endl;
    }
  }

  cout << "\n\n";
}


void DisplayPlugInInfo(const PString & name, const PPluginModuleManager::PluginListType & pluginList)
{
  for (int i = 0; i < pluginList.GetSize(); i++) {
    if (pluginList.GetKeyAt(i) == name) {
      PDynaLink & dll = pluginList.GetDataAt(i);
      PluginCodec_GetCodecFunction getCodecs;
      if (!dll.GetFunction(PLUGIN_CODEC_GET_CODEC_FN_STR, (PDynaLink::Function &)getCodecs)) {
        cout << "error: " << name << " is missing the function " << PLUGIN_CODEC_GET_CODEC_FN_STR << endl;
        return;
      }
      unsigned int count;
      PluginCodec_Definition * codecs = (*getCodecs)(&count, PLUGIN_CODEC_VERSION_VIDEO);
      if (codecs == NULL || count == 0) {
        cout << "error: " << name << " does not define any codecs for this version of the plugin API" << endl;
        return;
      } 
      cout << name << " contains " << count << " coders:" << endl;
      for (unsigned j = 0; j < count; j++) {
        cout << "---------------------------------------" << endl
            << "Coder " << j+1 << endl;
        DisplayCodecDefn(codecs[j]);
      }
      return;
    }
  }

  cout << "error: plugin \"" << name << "\" not found, specify one of :\n"
       << setfill('\n') << pluginList << setfill(' ') << endl;
}


void DisplayMediaFormat(const PString & fmtName)
{
  OpalMediaFormat mediaFormat = fmtName;
  if (mediaFormat.IsEmpty()) {
    cout << "error: cannot get info on format name \"" << fmtName << '"' << endl;
    return;
  }

  cout << "        Media Format Name       = " << mediaFormat << "\n"
          "       Default Session ID (R/O) = " << mediaFormat.GetDefaultSessionID() << "\n"
          "             Payload type (R/O) = " << (unsigned)mediaFormat.GetPayloadType() << "\n"
          "            Encoding name (R/O) = " << mediaFormat.GetEncodingName() << '\n';
  for (PINDEX i = 0; i < mediaFormat.GetOptionCount(); i++) {
    const OpalMediaOption & option = mediaFormat.GetOption(i);
    cout << right << setw(25) << option.GetName() << " (R/" << (option.IsReadOnly() ? 'O' : 'W')
         << ") = " << left << setw(10) << option.AsString();
    if (!option.GetFMTPName().IsEmpty())
      cout << "  FMTP name: " << option.GetFMTPName() << " (" << option.GetFMTPDefault() << ')';
    const OpalMediaOption::H245GenericInfo & genericInfo = option.GetH245Generic();
    if (genericInfo.mode != OpalMediaOption::H245GenericInfo::None) {
      cout << "  H.245 Ordinal: " << genericInfo.ordinal
           << ' ' << (genericInfo.mode == OpalMediaOption::H245GenericInfo::Collapsing ? "Collapsing" : "Non-Collapsing");
      if (!genericInfo.excludeTCS)
        cout << " TCS";
      if (!genericInfo.excludeOLC)
        cout << " OLC";
      if (!genericInfo.excludeReqMode)
        cout << " RM";
    }
    cout << endl;
  }
  cout << endl;
}


void AudioTest(const PString & fmtName)
{
  OpalMediaFormat mediaFormat = fmtName;
  if (mediaFormat.IsEmpty() || mediaFormat.GetDefaultSessionID() != OpalMediaFormat::DefaultAudioSessionID) {
    cout << "error: cannot use format name \"" << fmtName << '"' << endl;
    return;
  }

  std::auto_ptr<OpalTranscoder> encoder(OpalTranscoder::Create(OpalPCM16, mediaFormat));
  if (encoder.get() == NULL) {
    cout << "error: Encoder cannot be instantiated!" << endl;
    return;
  }

  std::auto_ptr<OpalTranscoder> decoder(OpalTranscoder::Create(mediaFormat, OpalPCM16));
  if (decoder.get() == NULL) {
    cout << "error: Decoder cannot be instantiated!" << endl;
    return;
  }

  PINDEX decBlkSize = encoder->GetOptimalDataFrameSize(TRUE);
  if (decBlkSize != decoder->GetOptimalDataFrameSize(FALSE)) {
    cout << "error: Encoder and decoder have different frame sizes, mode not supported." << endl;
    return;
  }

  PTones tones("110:0.4-0.1/220:0.4-0.1/440:0.4-0.1/880:0.4-0.1/1760:0.4-0.1");

  RTP_DataFrame source, output, encoded;
  source.SetPayloadSize(decBlkSize);
  output.SetPayloadSize(decBlkSize);
  encoded.SetPayloadSize(PMAX(encoder->GetOptimalDataFrameSize(FALSE), decoder->GetOptimalDataFrameSize(TRUE)));
  encoded.SetPayloadType(mediaFormat.GetPayloadType());

  int count = 0;
  cout << "Encoded frames: " << endl;
  for (PINDEX offset = 0; offset < tones.GetSize(); offset += decBlkSize/2) {
    memcpy(source.GetPayloadPtr(), &tones[offset], decBlkSize);

    if (!encoder->Convert(source, encoded)) {
      cout << "error: Encoder conversion failed!" << endl;
      return;
    }

    cout << "Frame " << ++count << '\n' << encoded << endl;

    if (!decoder->Convert(encoded, output)) {
      cout << "error: Decoder conversion failed!" << endl;
      return;
    }

    cout << "   Done" << endl;
  }

  cout << "Audio test completed." << endl;
}


void VideoTest(const PString & fmtName)
{
  OpalMediaFormat mediaFormat = fmtName;
  if (mediaFormat.IsEmpty() || mediaFormat.GetDefaultSessionID() != OpalMediaFormat::DefaultVideoSessionID) {
    cout << "error: cannot use format name \"" << fmtName << '"' << endl;
    return;
  }

  std::auto_ptr<OpalTranscoder> encoder(OpalTranscoder::Create(OpalYUV420P, mediaFormat));
  if (encoder.get() == NULL) {
    cout << "error: Encoder cannot be instantiated!" << endl;
    return;
  }

  std::auto_ptr<OpalTranscoder> decoder(OpalTranscoder::Create(mediaFormat, OpalYUV420P));
  if (decoder.get() == NULL) {
    cout << "error: Decoder cannot be instantiated!" << endl;
    return;
  }

  std::auto_ptr<PVideoInputDevice> fakeVideo(PVideoInputDevice::CreateDevice("FakeVideo"));
  fakeVideo->SetChannel(0); // Moving blocks
  fakeVideo->SetColourFormat(OpalYUV420P);
  fakeVideo->Open("");

  RTP_DataFrame source;
  source.SetPayloadSize(fakeVideo->GetMaxFrameBytes()+sizeof(OpalVideoTranscoder::FrameHeader));

  RTP_DataFrameList encoded, output;

  cout << "Encoded frames: " << endl;
  for (unsigned count = 1; count <= 20; count++) {
    cout << "\nFrame " << count << ' ' << flush;
    OpalVideoTranscoder::FrameHeader * frame = (OpalVideoTranscoder::FrameHeader *)source.GetPayloadPtr();
    frame->x = frame->y = 0;
    fakeVideo->GetFrameSize(frame->width, frame->height);
    fakeVideo->GetFrameData((BYTE *)(frame+1)); 

    if (!encoder->ConvertFrames(source, encoded)) {
      cout << "error: Encoder conversion failed!" << endl;
      return;
    }

    cout << encoded.GetSize() << " packets." << endl;

    for (PINDEX i = 0; i < encoded.GetSize(); i++) {
      cout << "Packet " << (i+1) << ": " << encoded[i] << endl;

      if (!decoder->ConvertFrames(encoded[i], output)) {
        cout << "error: Decoder conversion failed!" << endl;
        return;
      }
      if (output.GetSize() > 0) {
        frame = (OpalVideoTranscoder::FrameHeader *)output[0].GetPayloadPtr();
        cout << "Decoded frame: " << frame->width << 'x' << frame->height << endl;
      }
    }
  }

  cout << "\n\nVideo test completed." << endl;
}


void OpalCodecInfo::Main()
{
  cout << GetName()
       << " Version " << GetVersion(TRUE)
       << " by " << GetManufacturer()
       << " on " << GetOSClass() << ' ' << GetOSName()
       << " (" << GetOSVersion() << '-' << GetOSHardware() << ")\n\n";

  // Get and parse all of the command line arguments.
  PArgList & args = GetArguments();
  args.Parse(
             "A-audio-test:"
             "a-capabilities."
             "C-caps:"
             "f-format:"
             "h-help."
             "i-info:"
             "m-mediaformats."
             "p-pluginlist."
             "T-trancoders."
#if PTRACING
             "t-trace."
             "o-output:"
#endif
             "V-video-test:"
             , FALSE);

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);
#endif

  bool needHelp = true;

  if (args.HasOption('m')) {
    OpalMediaFormatList mediaList = OpalMediaFormat::GetAllRegisteredMediaFormats();
    cout << "Registered media formats:" << endl;
    DisplayMediaFormats(mediaList);
    needHelp = false;
  }

  if (args.HasOption('f')) {
    PStringArray formats = args.GetOptionString('f').Lines();
    for (PINDEX i = 0; i < formats.GetSize(); i++)
      DisplayMediaFormat(formats[i]);
    needHelp = false;
  }

  OpalPluginCodecManager & codecMgr = *(OpalPluginCodecManager *)PFactory<PPluginModuleManager>::CreateInstance("OpalPluginCodecManager");
  PPluginModuleManager::PluginListType pluginList = codecMgr.GetPluginList();

  if (args.HasOption('p')) {
    cout << "Plugin codecs:" << endl;
    for (int i = 0; i < pluginList.GetSize(); i++)
      cout << "   " << pluginList.GetKeyAt(i) << endl;
    cout << "\n\n";
    needHelp = false;
  }

  if (args.HasOption('i')) {
    PStringArray plugins = args.GetOptionString('i').Lines();
    for (PINDEX i = 0; i < plugins.GetSize(); i++)
      DisplayPlugInInfo(plugins[i], pluginList);
    needHelp = false;
  }

  if (args.HasOption('a')) {
    H323CapabilityFactory::KeyList_T keyList = H323CapabilityFactory::GetKeyList();
    cout << "Registered capabilities:" << endl
         << setfill('\n') << PStringArray(keyList) << setfill(' ')
         << "\n\n";
    needHelp = false;
  }

  if (args.HasOption('C')) {
    PStringArray specs = args.GetOptionString('C').Lines();
    for (PINDEX s = 0; s < specs.GetSize(); s++) {
      OpalMediaFormatList stdCodecList = OpalMediaFormat::GetAllRegisteredMediaFormats();
      H323Capabilities caps;
      caps.AddAllCapabilities(0, 0, specs[s]);
      cout << "Capability set using \"" << specs[s] << "\" :\n" << caps << endl;
      for (PINDEX i = 0; i < stdCodecList.GetSize(); i++) {
        PString fmt = stdCodecList[i];
        caps.FindCapability(fmt);
      }
    }
    needHelp = false;
  }

  if (args.HasOption('T')) {
    cout << "Available trancoders:" << endl;
    OpalTranscoderList keys = OpalTranscoderFactory::GetKeyList();
    OpalTranscoderList::const_iterator transcoder;
    for (transcoder = keys.begin(); transcoder != keys.end(); ++transcoder) {
      cout << "   " << transcoder->first << "->" << transcoder->second << flush;
      OpalTranscoder * xcoder = OpalTranscoder::Create(transcoder->first, transcoder->second);
      if (xcoder == NULL)
        cout << "  CANNOT BE INSTANTIATED";
      else
        delete xcoder;
      cout << endl;
    }
    needHelp = false;
  }

  if (args.HasOption('A')) {
    AudioTest(args.GetOptionString('A'));
    needHelp = false;
  }

  if (args.HasOption('V')) {
    VideoTest(args.GetOptionString('V'));
    needHelp = false;
  }

  if (needHelp) {
    cout << "available options:" << endl
        << "  -m --mediaformats   display media formats\n"
        << "  -f --format fmt     display info about a media format\n"
        << "  -c --codecs         display standard codecs\n"
        << "  -p --pluginlist     display codec plugins\n"
        << "  -i --info name      display info about a plugin\n"
        << "  -a --capabilities   display all registered capabilities that match the specification\n"
        << "  -C --caps spec      display capabilities that match the specification\n"
        << "  -T --transcoders    display available transcoders\n"
        << "  -A --audio-test fmt Test audio transcoder: PCM-16->fmt then fmt->PCM-16\n"
        << "  -V --video-test fmt Test video transcoder: YUV420P->fmt then fmt->YUV420P\n"
        << "  -t --trace          Increment trace level\n"
        << "  -o --output         Trace output file\n"
        << "  -h --help           display this help message\n";
  }
}
