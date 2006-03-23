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
 * Revision 1.1.2.3  2006/03/23 07:55:54  csoutheren
 * Added in options for capability handling
 *
 * Revision 1.1.2.2  2006/03/20 05:04:54  csoutheren
 * Add check to ensure codec can be instantiated
 *
 * Revision 1.1.2.1  2006/03/16 07:06:27  csoutheren
 * Update for new code
 *
 * Revision 1.1  2006/03/13 06:56:37  csoutheren
 * Initial version
 *
 */

#include <ptlib.h>

#include <codec/allcodecs.h>
#include <codec/opalplugin.h>
#include <codec/opalpluginmgr.h>
#include <opal/transcoders.h>

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
  cout << "  Version:             " << defn.version << endl
       << DisplayLicenseInfo(defn.info)
       << "  Flags:               ";
  switch (defn.flags & PluginCodec_MediaTypeMask) {
    case PluginCodec_MediaTypeAudio:
      cout << "Audio";
      break;
    case PluginCodec_MediaTypeVideo:
      cout << "Video";
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
       << "  Ns/frame:            " << defn.nsPerFrame << endl
       << "  Samples/frame:       " << defn.samplesPerFrame << endl
       << "  Bytes/frame:         " << defn.bytesPerFrame << endl
       << "  Frames/packet:       " << defn.recommendedFramesPerPacket << endl
       << "  Frames/packet (max): " << defn.maxFramesPerPacket << endl
       << "  RTP payload:         ";

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
        cout << "not displayed" << endl;
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
    case PluginCodec_H323VideoCodec_h262:
    case PluginCodec_H323VideoCodec_h263:
    case PluginCodec_H323VideoCodec_is11172:
      if (defn.h323CapabilityData == NULL)
        cout << ", no data" << endl;
      else 
        cout << "not displayed" << endl;
      break;

    default:
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
    cout << setw(max_len+2) << fmt << "  ";
    switch (fmt.GetDefaultSessionID()) {
      case OpalMediaFormat::DefaultAudioSessionID:
        cout << "audio, rate=" << fmt.GetBandwidth() << ", RTP=" << fmt.GetPayloadType() << endl;
        break;
      case OpalMediaFormat::DefaultVideoSessionID:
        cout << "video, rate=" << fmt.GetBandwidth() << ", RTP=" << fmt.GetPayloadType() << endl;
        break;
      case OpalMediaFormat::DefaultDataSessionID:
        cout << "data" << endl;;
        break;
      default:
        cout << "unknown type " << hex << mediaList[i].GetDefaultSessionID() << dec << endl;
    }
  }
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
             "t-trace."
             "o-output:"
             "m-mediaformats."
             "T-trancoders."
             "p-pluginlist."
             "c-codecs."
             "i-info:"

             "h-help."

             "a-capabilities:"
             "C-caps:"
             , FALSE);

  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);

  OPALPluginCodecManager & codecMgr = *(OPALPluginCodecManager *)PFactory<PPluginModuleManager>::CreateInstance("OPALPluginCodecManager");
  PPluginModuleManager::PluginListType pluginList = codecMgr.GetPluginList();

  if (args.HasOption('a')) {
    H323CapabilityFactory::KeyList_T keyList = H323CapabilityFactory::GetKeyList();
    cout << "Registered capabilities:" << endl
         << setfill(',') << PStringArray(keyList) << setfill(' ')
         << endl;
    return;
  }

  if (args.HasOption('C')) {
    PString spec = args.GetOptionString('C');
    OpalMediaFormatList stdCodecList = OPALPluginCodecManager::GetMediaFormats();
    H323Capabilities caps;
    caps.AddAllCapabilities(0, 0, spec);
    cout << "Standard capability set:" << caps << endl;
    for (PINDEX i = 0; i < stdCodecList.GetSize(); i++) {
      PString fmt = stdCodecList[i];
      caps.FindCapability(fmt);
    }
    return;
  }

  if (args.HasOption('c')) {
    OpalMediaFormatList stdCodecList = OPALPluginCodecManager::GetMediaFormats();
    cout << "Standard codecs:" << endl;
    DisplayMediaFormats(stdCodecList);
    return;
  }

  if (args.HasOption('m')) {
    OpalMediaFormatList mediaList = OpalMediaFormat::GetAllRegisteredMediaFormats();
    cout << "Registered media formats:" << endl;
    DisplayMediaFormats(mediaList);
    return;
  }

  if (args.HasOption('T')) {
    OpalTranscoderList keys = OpalTranscoderFactory::GetKeyList();
    OpalTranscoderList::const_iterator r;
    for (r = keys.begin(); r != keys.end(); ++r) {
      const OpalMediaFormatPair & transcoder = *r;
      cout << "Name: " << transcoder << "\n"
           << "  Input:  " << transcoder.GetInputFormat() << "\n"
           << "  Output: " << transcoder.GetOutputFormat() << "\n";

      OpalTranscoder * xcoder = OpalTranscoder::Create(transcoder.GetInputFormat(), transcoder.GetOutputFormat());
      if (xcoder == NULL)
        cout << "  CANNOT BE INSTANTIATED\n";
      else
        delete xcoder;
    }
    return;
  }

  if (args.HasOption('p')) {
    cout << "Plugin codecs:" << endl;
    for (int i = 0; i < pluginList.GetSize(); i++) {
      cout << "   " << pluginList.GetKeyAt(i) << endl;
    }
    return;
  }

  if (args.HasOption('i')) {
    PString name = args.GetOptionString('i');
    if (name.IsEmpty()) {
      cout << "error: -i must specify name of plugin" << endl;
      return;
    }
    for (int i = 0; i < pluginList.GetSize(); i++) {
      if (pluginList.GetKeyAt(i) == name) {
        PDynaLink & dll = pluginList.GetDataAt(i);
        PluginCodec_GetCodecFunction getCodecs;
        if (!dll.GetFunction(PLUGIN_CODEC_GET_CODEC_FN_STR, (PDynaLink::Function &)getCodecs)) {
          cout << "error: " << name << " is missing the function " << PLUGIN_CODEC_GET_CODEC_FN_STR << endl;
          return;
        }
        unsigned int count;
        PluginCodec_Definition * codecs = (*getCodecs)(&count, PLUGIN_CODEC_VERSION);
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
    cout << "error: plugin \"" << name << "\" not found" << endl;
    return;
  }

  cout << "available options:" << endl
       << "  -c --codecs         display standard codecs\n"
       << "  -h --help           display this help message\n"
       << "  -i --info name      display info about a plugin\n"
       << "  -m --mediaformats   display media formats\n"
       << "  -p --pluginlist     display codec plugins\n"
       << "  -C --caps spec      display capabilities that match the specification\n"
       << "  -f --factory        display codec conversion factories\n"
  ;
}
