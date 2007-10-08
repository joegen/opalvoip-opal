/*
 * opalplugins.cxx
 *
 * OPAL codec plugins handler
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
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
 * $Log: opalpluginmgr.cxx,v $
 * Revision 1.2063  2007/10/08 01:45:16  rjongbloed
 * Fixed bad virtual function causing uninitialised variable whcih prevented video from working.
 * Some more clean ups.
 *
 * Revision 2.61  2007/10/05 04:14:47  rjongbloed
 * Quite a large code clean up.
 *
 * Revision 2.60  2007/09/25 10:38:33  rjongbloed
 * Added trace log for when I-Frame sent
 *
 * Revision 2.59  2007/09/25 09:49:54  rjongbloed
 * Fixed videoFastUpdate, is not a count but a simple boolean.
 *
 * Revision 2.58  2007/09/25 06:42:30  rjongbloed
 * Tightened setting of video encoder packet size so allows for RTP header.
 * Also added default size for video decoder output packet (YUV420) frame.
 *
 * Revision 2.57  2007/09/21 04:08:16  rjongbloed
 * Improved logging when setting plug in options
 *
 * Revision 2.56  2007/09/19 10:43:31  csoutheren
 * Exposed G.7231 capability class
 * Added macros to create empty transcoders and capabilities
 *
 * Revision 2.55  2007/09/12 23:02:00  rjongbloed
 * Code clean up, thanks Hannes.
 *
 * Revision 2.54  2007/09/12 19:08:53  ykiryanov
 * Because I have various versions of strcasecmp for WCE based platforms, such as strcasecmp(char*, char*) or strcasecmp(wchar_t*, char), or strcasecmp(PString, char*), which is under UNICODE are all different, I have added cast of some variables to (const char*). Should we NOT use strcasecmp and go with PString comparisons instead?
 *
 * Revision 2.53  2007/09/12 04:19:53  rjongbloed
 * CHanges to avoid creation of long duration OpalMediaFormat instances, eg in
 *   the plug in capabilities, that then do not get updated values from the master
 *   list, or worse from the user modified master list, causing much confusion.
 *
 * Revision 2.52  2007/09/10 03:15:04  rjongbloed
 * Fixed issues in creating and subsequently using correctly unique
 *   payload types in OpalMediaFormat instances and transcoders.
 *
 * Revision 2.51  2007/08/22 05:18:09  rjongbloed
 * Changed size of RTP packet to be slightlysmaller, the true MTU for ethernet.
 * Also some code clean ups.
 *
 * Revision 2.50  2007/08/16 00:47:04  rjongbloed
 * Also bad boy Craig for leaving debugging in the code. :-)
 *
 * Revision 2.49  2007/08/16 00:22:43  csoutheren
 * Fixed cut and paste error. Thanks to Matthias Schneider for noticing the
 * problem, and Robert Jongbloed for providing the fix. Bad boy Craig for making it
 *
 * Revision 2.48  2007/08/13 06:08:08  csoutheren
 * Expose more functions
 *
 * Revision 2.47  2007/08/09 08:21:25  csoutheren
 * Add missing braces
 *
 * Revision 2.46  2007/08/08 17:35:16  csoutheren
 * Final plugin manager changes
 *
 * Revision 2.45  2007/08/08 11:18:47  csoutheren
 * Fixed Linux compile errors
 *
 * Revision 2.44  2007/08/08 08:59:30  csoutheren
 * More plugin manager changes, as the last approach dead-ended :(
 *
 * Revision 2.43  2007/08/08 07:13:01  csoutheren
 * #ifdef out unused code - to be removed later if nobody complains :)
 *
 * Revision 2.42  2007/08/07 09:04:25  csoutheren
 * Export more functions
 *
 * Revision 2.41  2007/08/07 08:25:36  csoutheren
 * Expose plugin media format classes
 *
 * Revision 2.40  2007/08/07 00:13:57  csoutheren
 * Fix compile error on Windows
 *
 * Revision 2.39  2007/08/06 15:05:43  csoutheren
 * Fix problem with media format settings not being applied to plugin
 * video transcoders on startup
 *
 * Revision 2.38  2007/08/06 07:14:23  csoutheren
 * Fix logging
 * Correct matching of H.263 capabilities
 *
 * Revision 2.37  2007/08/03 08:23:22  csoutheren
 * Cleanups
 *
 * Revision 2.36  2007/08/03 08:05:16  csoutheren
 * Add PrintOn for plugin video caps
 *
 * Revision 2.35  2007/08/03 07:20:33  csoutheren
 * Add extra constructor initialisations for plugin capabilities with multiple ancestors
 *
 * Revision 2.34  2007/08/03 03:41:59  csoutheren
 * Fixed usage of tolower in codec control parsing
 * Ensure plugin media options can override existing values
 * Add extra logging
 *
 * Revision 2.33  2007/07/24 12:59:44  rjongbloed
 * Fixed G.723.1 plug in capability matching, should not include Annex A option.
 * Made sure all integer OpalMediaOptions are unsigned so is compatible with H.245 generic capabilities.
 *
 * Revision 2.32  2007/07/20 05:45:45  rjongbloed
 * Fixed incorrect maxBitRate field in Generic Audio Capabilities
 *
 * Revision 2.31  2007/07/02 18:53:47  csoutheren
 * Exposed classes
 *
 * Revision 2.30  2007/06/27 18:19:49  csoutheren
 * Fix compile when video disabled
 *
 * Revision 2.29  2007/06/27 12:29:45  rjongbloed
 * Add new OpalMediaOption for octet strings (simple block of bytes).
 *
 * Revision 2.28  2007/06/25 04:04:38  rjongbloed
 * Fixed compile after change to anonymous structure in header (for GCC).
 *
 * Revision 2.27  2007/06/22 06:29:14  rjongbloed
 * Fixed GCC warnings.
 *
 * Revision 2.26  2007/06/22 05:48:09  rjongbloed
 * Major codec API update:
 *   Automatically map OpalMediaOptions to SIP/SDP FMTP parameters.
 *   Automatically map OpalMediaOptions to H.245 Generic Capability parameters.
 *   Largely removed need to distinguish between SIP and H.323 codecs.
 *   New mechanism for setting OpalMediaOptions from within a plug in.
 * Fixed setting of output RTP payload types on plug in video codecs.
 *
 * Revision 2.25  2007/06/16 21:37:01  dsandras
 * Added H.264 support thanks to Matthias Schneider <ma30002000 yahoo de>.
 * Thanks a lot !
 *
 * Baseline Profile:
 * no B-frames
 * We make use of the baseline profile (which is the designated profile for interactive vide) ,
 * that means:
 * no B-Frames (too much latency in interactive video)
 * CBR (we want to get the max. quality making use of all the bitrate that is available)
 * We allow one exeption: configuring a bitrate of > 786 kbit/s
 *
 * This plugin implements
 * - Single Time Aggregation Packets A
 * - Single NAL units
 * - Fragmentation Units
 * like described in RFC3984
 *
 * It requires x264 and ffmpeg.
 *
 * Revision 2.24  2007/05/12 03:57:34  rjongbloed
 * Fixed transcoder from plug in not being created correctly due to media format not yet being registered.
 *
 * Revision 2.23  2007/05/10 05:34:23  csoutheren
 * Ensure fax transmission works with reasonable size audio blocks
 *
 * Revision 2.22  2007/04/19 06:34:12  csoutheren
 * Applied 1703206 - OpalVideoFastUpdatePicture over SIP
 * Thanks to Josh Mahonin
 *
 * Revision 2.21  2007/04/10 05:15:54  rjongbloed
 * Fixed issue with use of static C string variables in DLL environment,
 *   must use functional interface for correct initialisation.
 *
 * Revision 2.20  2007/04/04 02:12:00  rjongbloed
 * Reviewed and adjusted PTRACE log levels
 *   Now follows 1=error,2=warn,3=info,4+=debug
 *
 * Revision 2.19  2007/04/02 05:51:33  rjongbloed
 * Tidied some trace logs to assure all have a category (bit before a tab character) set.
 *
 * Revision 2.18  2007/03/29 05:20:17  csoutheren
 * Implement T.38 and fax
 *
 * Revision 2.17  2006/12/08 07:33:13  csoutheren
 * Fix problem with wideband audio plugins and sound channel
 *
 * Revision 2.16  2006/11/29 06:28:58  csoutheren
 * Add ability call codec control functions on all transcoders
 *
 * Revision 2.15  2006/10/17 23:55:07  csoutheren
 * Changed to match new enum in opalplugin.h
 *
 * Revision 2.14  2006/10/17 15:36:58  shorne
 * Missed line removal in last commit
 *
 * Revision 2.13  2006/10/17 04:10:12  shorne
 * Corrected h245 generic parameter identifiers
 *
 * Revision 2.12  2006/10/10 07:18:18  csoutheren
 * Allow compilation with and without various options
 *
 * Revision 2.11  2006/10/02 13:30:51  rjongbloed
 * Added LID plug ins
 *
 * Revision 2.10  2006/09/28 07:42:17  csoutheren
 * Merge of useful SRTP implementation
 *
 * Revision 2.9  2006/09/11 04:48:55  csoutheren
 * Fixed problem with cloning plugin media formats
 *
 * Revision 2.8  2006/09/07 09:05:44  csoutheren
 * Fix case significance in IsValidForProtocol
 *
 * Revision 2.7  2006/09/06 22:36:11  csoutheren
 * Fix problem with IsValidForProtocol on video codecs
 *
 * Revision 2.6  2006/08/22 03:08:42  csoutheren
 * Fixed compile error on gcc 4.1
 *
 * Revision 2.5  2006/08/20 03:56:57  csoutheren
 * Add OpalMediaFormat::IsValidForProtocol to allow plugin codecs to be enabled only for certain protocols
 * rather than relying on the presence of the IANA rtp encoding name field
 *
 * Revision 2.4  2006/08/15 23:52:55  csoutheren
 * Ensure codecs with same name but different clock rate get different payload types
 *
 * Revision 2.3  2006/08/11 07:52:01  csoutheren
 * Fix problem with media format factory in VC 2005
 * Fixing problems with Speex codec
 * Remove non-portable usages of PFactory code
 *
 * Revision 2.2  2006/08/10 06:05:32  csoutheren
 * Fix problem with key type in plugin factory
 *
 * Revision 2.1  2006/07/24 14:03:39  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 1.1.2.15  2006/05/16 07:03:09  csoutheren
 * Fixed Linux compile system
 *
 * Revision 1.1.2.14  2006/04/26 08:03:58  csoutheren
 * H.263 encoding and decoding now working from plugin for both SIP and H.323
 *
 * Revision 1.1.2.13  2006/04/26 05:05:59  csoutheren
 * H.263 decoding working via codec plugin
 *
 * Revision 1.1.2.12  2006/04/25 01:14:54  csoutheren
 * Added H.263 capabilities
 *
 * Revision 1.1.2.11  2006/04/24 09:09:13  csoutheren
 * Added H.263 codec definitions
 *
 * Revision 1.1.2.10  2006/04/21 05:42:52  csoutheren
 * Checked in forgotten changes to fix iFrame requests
 *
 * Revision 1.1.2.9  2006/04/19 07:52:30  csoutheren
 * Add ability to have SIP-only and H.323-only codecs, and implement for H.261
 *
 * Revision 1.1.2.8  2006/04/19 04:58:56  csoutheren
 * Debugging and testing of new video plugins
 * H.261 working in both CIF and QCIF modes in H.323
 *
 * Revision 1.1.2.7  2006/04/11 08:40:22  csoutheren
 * Modified H.261 plugin handling to use new media formats
 *
 * Revision 1.1.2.6  2006/04/10 08:53:39  csoutheren
 * Fix problem with audio code parameters
 *
 * Revision 1.1.2.5  2006/04/06 05:48:08  csoutheren
 * Fixed last minute edits to allow compilation
 *
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

#include <opal/buildopts.h>

#include <opal/transcoders.h>
#include <codec/opalpluginmgr.h>
#include <codec/opalplugin.h>
#include <h323/h323caps.h>
#include <asn/h245.h>

#if OPAL_VIDEO
#include <codec/vidcodec.h>
#endif

#if OPAL_T38FAX
#include <t38/t38proto.h>
#endif

// G.711 is *always* available
#include <codec/g711codec.h>
OPAL_REGISTER_G711();


#if OPAL_VIDEO

#define CIF_WIDTH         352
#define CIF_HEIGHT        288

#define CIF4_WIDTH        (CIF_WIDTH*2)
#define CIF4_HEIGHT       (CIF_HEIGHT*2)

#define CIF16_WIDTH       (CIF_WIDTH*4)
#define CIF16_HEIGHT      (CIF_HEIGHT*4)

#define QCIF_WIDTH        (CIF_WIDTH/2)
#define QCIF_HEIGHT       (CIF_HEIGHT/2)

#define SQCIF_WIDTH       128
#define SQCIF_HEIGHT      96

static const char * sqcifMPI_tag                          = "SQCIF MPI";
static const char * qcifMPI_tag                           = "QCIF MPI";
static const char * cifMPI_tag                            = "CIF MPI";
static const char * cif4MPI_tag                           = "CIF4 MPI";
static const char * cif16MPI_tag                          = "CIF16 MPI";

#if OPAL_H323

#define H323CAP_TAG_PREFIX    "h323"

// H.261 only
static const char * h323_stillImageTransmission_tag            = H323CAP_TAG_PREFIX "_stillImageTransmission";

// H.261/H.263 tags
static const char * h323_qcifMPI_tag                           = H323CAP_TAG_PREFIX "_qcifMPI";
static const char * h323_cifMPI_tag                            = H323CAP_TAG_PREFIX "_cifMPI";

// H.263 only
static const char * h323_sqcifMPI_tag                          = H323CAP_TAG_PREFIX "_sqcifMPI";
static const char * h323_cif4MPI_tag                           = H323CAP_TAG_PREFIX "_cif4MPI";
static const char * h323_cif16MPI_tag                          = H323CAP_TAG_PREFIX "_cif16MPI";
static const char * h323_temporalSpatialTradeOffCapability_tag = H323CAP_TAG_PREFIX "_temporalSpatialTradeOffCapability";
static const char * h323_unrestrictedVector_tag                = H323CAP_TAG_PREFIX "_unrestrictedVector";
static const char * h323_arithmeticCoding_tag                  = H323CAP_TAG_PREFIX "_arithmeticCoding";      
static const char * h323_advancedPrediction_tag                = H323CAP_TAG_PREFIX "_advancedPrediction";
static const char * h323_pbFrames_tag                          = H323CAP_TAG_PREFIX "_pbFrames";
static const char * h323_hrdB_tag                              = H323CAP_TAG_PREFIX "_hrdB";
static const char * h323_bppMaxKb_tag                          = H323CAP_TAG_PREFIX "_bppMaxKb";
static const char * h323_errorCompensation_tag                 = H323CAP_TAG_PREFIX "_errorCompensation";

#endif // OPAL_H323
#endif // OPAL_VIDEO



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


static PString CreateCodecName(const PluginCodec_Definition * codec)
{
  return codec->destFormat != NULL ? codec->destFormat : codec->descr;
}


///////////////////////////////////////////////////////////////////////////////

OpalPluginControl::OpalPluginControl(const PluginCodec_Definition * def, const char * name)
  : codecDef(def)
  , fnName(name)
  , controlDef(def->codecControls)
{
  if (controlDef == NULL)
    return;

  while (controlDef->name != NULL) {
    if (strcasecmp(controlDef->name, name) == 0 && controlDef->control != NULL)
      return;
    controlDef++;
  }

  controlDef = NULL;
}


///////////////////////////////////////////////////////////////////////////////

OpalPluginMediaFormat::OpalPluginMediaFormat(const PluginCodec_Definition * defn)
  : codecDef(defn)
  , getOptionsControl(defn, PLUGINCODEC_CONTROL_GET_CODEC_OPTIONS)
  , freeOptionsControl(defn, PLUGINCODEC_CONTROL_FREE_CODEC_OPTIONS)
  , validForProtocolControl(defn, PLUGINCODEC_CONTROL_VALID_FOR_PROTOCOL)
{
}


static void SetOldStyleOption(OpalMediaFormat & format, const PString & _key, const PString & _val, const PString & type)
{
  PCaselessString key(_key);
  const char * val = _val;

#if OPAL_VIDEO
  // Backward compatibility tests
  if (key == h323_qcifMPI_tag)
    key = qcifMPI_tag;
  else if (key == h323_cifMPI_tag)
    key = cifMPI_tag;
  else if (key == h323_sqcifMPI_tag)
    key = sqcifMPI_tag;
  else if (key == h323_cif4MPI_tag)
    key = cif4MPI_tag;
  else if (key == h323_cif16MPI_tag)
    key = cif16MPI_tag;
#endif

  OpalMediaOption::MergeType op = OpalMediaOption::NoMerge;
  if (val[0] != '\0' && val[1] != '\0') {
    switch (val[0]) {
      case '<':
        op = OpalMediaOption::MinMerge;
        ++val;
        break;
      case '>':
        op = OpalMediaOption::MaxMerge;
        ++val;
        break;
      case '=':
        op = OpalMediaOption::EqualMerge;
        ++val;
        break;
      case '!':
        op = OpalMediaOption::NotEqualMerge;
        ++val;
        break;
      case '*':
        op = OpalMediaOption::AlwaysMerge;
        ++val;
        break;
      default:
        break;
    }
  }

  if (type[0] != '\0') {
    PStringArray tokens = PString(val+1).Tokenise(':', FALSE);
    char ** array = tokens.ToCharArray();
    switch (toupper(type[0])) {
      case 'E':
        PTRACE(5, "OpalPlugin\tAdding enum option '" << key << "' " << tokens.GetSize() << " options");
        format.AddOption(new OpalMediaOptionEnum(key, false, array, tokens.GetSize(), op, tokens.GetStringsIndex(val)), TRUE);
        break;
      case 'B':
        PTRACE(5, "OpalPlugin\tAdding boolean option '" << key << "'=" << val);
        format.AddOption(new OpalMediaOptionBoolean(key, false, op, (val[0] == '1') || (toupper(val[0]) == 'T')), TRUE);
        break;
      case 'R':
        PTRACE(5, "OpalPlugin\tAdding real option '" << key << "'=" << val);
        if (tokens.GetSize() < 2)
          format.AddOption(new OpalMediaOptionReal(key, false, op, PString(val).AsReal()));
        else
          format.AddOption(new OpalMediaOptionReal(key, false, op, PString(val).AsReal(), tokens[0].AsReal(), tokens[1].AsReal()), TRUE);
        break;
      case 'I':
        PTRACE(5, "OpalPlugin\tAdding integer option '" << key << "'=" << val);
        if (tokens.GetSize() < 2)
          format.AddOption(new OpalMediaOptionUnsigned(key, false, op, PString(val).AsUnsigned()), TRUE);
        else
          format.AddOption(new OpalMediaOptionUnsigned(key, false, op, PString(val).AsUnsigned(), tokens[0].AsUnsigned(), tokens[1].AsUnsigned()), TRUE);
        break;
      case 'S':
      default:
        PTRACE(5, "OpalPlugin\tAdding string option '" << key << "'=" << val);
        format.AddOption(new OpalMediaOptionString(key, false, val), TRUE);
        break;
    }
    free(array);
  }
}


void OpalPluginMediaFormat::PopulateOptions(OpalMediaFormat & format)
{
  void ** rawOptions = NULL;
  unsigned int optionsLen = sizeof(rawOptions);
  getOptionsControl.Call(&rawOptions, &optionsLen);
  if (rawOptions != NULL) {
    if (codecDef->version < PLUGIN_CODEC_VERSION_OPTIONS) {
      PTRACE(3, "OpalPlugin\tAdding options to OpalMediaFormat " << format << " using old style method");
      // Old scheme
      char const * const * options = (char const * const *)rawOptions;
      while (options[0] != NULL && options[1] != NULL && options[2] != NULL)  {
        SetOldStyleOption(format, options[0], options[1], options[2]);
        options += 3;
      }
    }
    else {
      // New scheme
      struct PluginCodec_Option const * const * options = (struct PluginCodec_Option const * const *)rawOptions;
      PTRACE_IF(5, options != NULL, "OpalPlugin\tAdding options to OpalMediaFormat " << format << " using new style method");
      while (*options != NULL) {
        struct PluginCodec_Option const * option = *options++;
        OpalMediaOption * newOption;
        switch (option->m_type) {
          case PluginCodec_StringOption :
            newOption = new OpalMediaOptionString(option->m_name,
                                                  option->m_readOnly != 0,
                                                  option->m_value);
            break;
          case PluginCodec_BoolOption :
            newOption = new OpalMediaOptionBoolean(option->m_name,
                                                   option->m_readOnly != 0,
                                                   (OpalMediaOption::MergeType)option->m_merge,
                                                   option->m_value != NULL && *option->m_value == 'T');
            break;
          case PluginCodec_IntegerOption :
            newOption = new OpalMediaOptionUnsigned(option->m_name,
                                                    option->m_readOnly != 0,
                                                    (OpalMediaOption::MergeType)option->m_merge,
                                                    PString(option->m_value).AsInteger(),
                                                    PString(option->m_minimum).AsInteger(),
                                                    PString(option->m_maximum).AsInteger());
            break;
          case PluginCodec_RealOption :
            newOption = new OpalMediaOptionReal(option->m_name,
                                                option->m_readOnly != 0,
                                                (OpalMediaOption::MergeType)option->m_merge,
                                                PString(option->m_value).AsReal(),
                                                PString(option->m_minimum).AsReal(),
                                                PString(option->m_maximum).AsReal());
            break;
          case PluginCodec_EnumOption :
            {
              PStringArray valueTokens = PString(option->m_minimum).Tokenise(':');
              char ** enumValues = valueTokens.ToCharArray();
              newOption = new OpalMediaOptionEnum(option->m_name,
                                                  option->m_readOnly != 0,
                                                  enumValues,
                                                  valueTokens.GetSize(),
                                                  (OpalMediaOption::MergeType)option->m_merge,
                                                  valueTokens.GetStringsIndex(option->m_value));
              free(enumValues);
            }
            break;
          case PluginCodec_OctetsOption :
            newOption = new OpalMediaOptionOctets(option->m_name, option->m_readOnly != 0, option->m_minimum != NULL); // Use minimum to indicate Base64
            newOption->FromString(option->m_value);
            break;
          default : // Huh?
            continue;
        }

        newOption->SetFMTPName(option->m_FMTPName);
        newOption->SetFMTPDefault(option->m_FMTPDefault);

        OpalMediaOption::H245GenericInfo genericInfo;
        genericInfo.ordinal = option->m_H245Generic&PluginCodec_H245_OrdinalMask;
        if (option->m_H245Generic&PluginCodec_H245_Collapsing)
          genericInfo.mode = OpalMediaOption::H245GenericInfo::Collapsing;
        else if (option->m_H245Generic&PluginCodec_H245_NonCollapsing)
          genericInfo.mode = OpalMediaOption::H245GenericInfo::NonCollapsing;
        else
          genericInfo.mode = OpalMediaOption::H245GenericInfo::None;
        if (option->m_H245Generic&PluginCodec_H245_Unsigned32)
          genericInfo.integerType = OpalMediaOption::H245GenericInfo::Unsigned32;
        else if (option->m_H245Generic&PluginCodec_H245_BooleanArray)
          genericInfo.integerType = OpalMediaOption::H245GenericInfo::BooleanArray;
        else
          genericInfo.integerType = OpalMediaOption::H245GenericInfo::UnsignedInt;
        genericInfo.excludeTCS = (option->m_H245Generic&PluginCodec_H245_TCS) == 0;
        genericInfo.excludeOLC = (option->m_H245Generic&PluginCodec_H245_OLC) == 0;
        genericInfo.excludeReqMode = (option->m_H245Generic&PluginCodec_H245_ReqMode) == 0;
        newOption->SetH245Generic(genericInfo);

        format.AddOption(newOption, TRUE);
      }
    }
    freeOptionsControl.Call(rawOptions, &optionsLen);
  }

  if (codecDef->h323CapabilityType == PluginCodec_H323Codec_generic && codecDef->h323CapabilityData != NULL) {
    const PluginCodec_H323GenericCodecData * genericData = (const PluginCodec_H323GenericCodecData *)codecDef->h323CapabilityData;
    const PluginCodec_H323GenericParameterDefinition *ptr = genericData->params;
    for (unsigned i = 0; i < genericData->nParameters; i++, ptr++) {
      OpalMediaOption::H245GenericInfo genericInfo;
      genericInfo.ordinal = ptr->id;
      genericInfo.mode = ptr->collapsing ? OpalMediaOption::H245GenericInfo::Collapsing : OpalMediaOption::H245GenericInfo::NonCollapsing;
      genericInfo.excludeTCS = ptr->excludeTCS;
      genericInfo.excludeOLC = ptr->excludeOLC;
      genericInfo.excludeReqMode = ptr->excludeReqMode;
      genericInfo.integerType = OpalMediaOption::H245GenericInfo::UnsignedInt;

      PString name(PString::Printf, "Generic Parameter %u", ptr->id);

      OpalMediaOption * mediaOption;
      switch (ptr->type) {
        case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_logical :
          mediaOption = new OpalMediaOptionBoolean(name, ptr->readOnly, OpalMediaOption::NoMerge, ptr->value.integer != 0);
          break;

        case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_booleanArray :
          genericInfo.integerType = OpalMediaOption::H245GenericInfo::BooleanArray;
          mediaOption = new OpalMediaOptionUnsigned(name, ptr->readOnly, OpalMediaOption::AndMerge, ptr->value.integer, 0, 255);
          break;

        case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_unsigned32Min :
          genericInfo.integerType = OpalMediaOption::H245GenericInfo::Unsigned32;
          // Do next case

        case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_unsignedMin :
          mediaOption = new OpalMediaOptionUnsigned(name, ptr->readOnly, OpalMediaOption::MinMerge, ptr->value.integer);
          break;

        case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_unsigned32Max :
          genericInfo.integerType = OpalMediaOption::H245GenericInfo::Unsigned32;
          // Do next case

        case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_unsignedMax :
          mediaOption = new OpalMediaOptionUnsigned(name, ptr->readOnly, OpalMediaOption::MaxMerge, ptr->value.integer);
          break;

        case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_octetString :
          mediaOption = new OpalMediaOptionString(name, ptr->readOnly, ptr->value.octetstring);
          break;

        default :
          mediaOption = NULL;
      }

      if (mediaOption != NULL) {
        mediaOption->SetH245Generic(genericInfo);
        format.AddOption(mediaOption);
      }
    }
  }

//  PStringStream str; format.PrintOptions(str);
//  PTRACE(5, "OpalPlugin\tOpalMediaFormat " << format << " has options\n" << str);
}


bool OpalPluginMediaFormat::IsValidForProtocol(const PString & _protocol) const
{
  PString protocol(_protocol.ToLower());

  if (validForProtocolControl.Exists())
    return validForProtocolControl.Call((void *)(const char *)protocol, sizeof(const char *));

  if (protocol == "h.323" || protocol == "h323")
    return (codecDef->h323CapabilityType != PluginCodec_H323Codec_undefined) &&
           (codecDef->h323CapabilityType != PluginCodec_H323Codec_NoH323);

  if (protocol == "sip") 
    return codecDef->sdpFormat != NULL;

  return FALSE;
}


///////////////////////////////////////////////////////////////////////////////

#if OPAL_AUDIO

OpalPluginAudioMediaFormat::OpalPluginAudioMediaFormat(const PluginCodec_Definition * _encoderCodec,
                                                       const char * rtpEncodingName, /// rtp encoding name
                                                       unsigned frameTime,           /// Time for frame in RTP units (if applicable)
                                                       unsigned /*timeUnits*/,       /// RTP units for frameTime (if applicable)
                                                       time_t timeStamp              /// timestamp (for versioning)
                                                      )
  : OpalAudioFormat(CreateCodecName(_encoderCodec),
                    (RTP_DataFrame::PayloadTypes)(((_encoderCodec->flags & PluginCodec_RTPTypeMask) == PluginCodec_RTPTypeDynamic) ? RTP_DataFrame::DynamicBase : _encoderCodec->rtpPayload),
                    rtpEncodingName,
                    _encoderCodec->parm.audio.bytesPerFrame,
                    frameTime,
                    _encoderCodec->parm.audio.recommendedFramesPerPacket,
                    _encoderCodec->parm.audio.recommendedFramesPerPacket,
                    _encoderCodec->parm.audio.maxFramesPerPacket,
                    _encoderCodec->sampleRate,
                    timeStamp  
                  )
  , OpalPluginMediaFormat(_encoderCodec)
{
  PopulateOptions(*this);

  if (_encoderCodec->parm.audio.maxFramesPerPacket > 0)
    SetOptionInteger(TxFramesPerPacketOption(), _encoderCodec->parm.audio.maxFramesPerPacket);

  // Override calculated value if we have an explicit bit rate
  if (_encoderCodec->bitsPerSec > 0)
    SetOptionInteger(MaxBitRateOption(), _encoderCodec->bitsPerSec);
}


bool OpalPluginAudioMediaFormat::IsValidForProtocol(const PString & protocol) const
{
  return OpalPluginMediaFormat::IsValidForProtocol(protocol);
}


PObject * OpalPluginAudioMediaFormat::Clone() const
{ return new OpalPluginAudioMediaFormat(*this); }

#if OPAL_H323

static H323Capability * CreateG7231Cap(
  const PluginCodec_Definition * encoderCodec, 
  const PluginCodec_Definition * decoderCodec,
  int subType
);

static H323Capability * CreateGenericAudioCap(
  const PluginCodec_Definition * encoderCodec, 
  const PluginCodec_Definition * decoderCodec,
  int subType
);

static H323Capability * CreateNonStandardAudioCap(
  const PluginCodec_Definition * encoderCodec, 
  const PluginCodec_Definition * decoderCodec,
  int subType
);

static H323Capability * CreateGSMCap(
  const PluginCodec_Definition * encoderCodec, 
  const PluginCodec_Definition * decoderCodec,
  int subType
);

#endif // OPAL_H323

#endif // OPAL_AUDIO

///////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

OpalPluginVideoMediaFormat::OpalPluginVideoMediaFormat(const PluginCodec_Definition * _encoderCodec,
                                                       const char * rtpEncodingName, /// rtp encoding name
                                                       time_t timeStamp              /// timestamp (for versioning)
                                                      )
  : OpalVideoFormat(CreateCodecName(_encoderCodec),
                    (RTP_DataFrame::PayloadTypes)(((_encoderCodec->flags & PluginCodec_RTPTypeMask) == PluginCodec_RTPTypeDynamic) ? RTP_DataFrame::DynamicBase : _encoderCodec->rtpPayload),
                    rtpEncodingName,
                    _encoderCodec->parm.video.maxFrameWidth,
                    _encoderCodec->parm.video.maxFrameHeight,
                    _encoderCodec->parm.video.maxFrameRate,
                    _encoderCodec->bitsPerSec,
                    timeStamp  
                  )
  , OpalPluginMediaFormat(_encoderCodec)
{
  PopulateOptions(*this);
}


PObject * OpalPluginVideoMediaFormat::Clone() const
{ 
  return new OpalPluginVideoMediaFormat(*this); 
}


bool OpalPluginVideoMediaFormat::IsValidForProtocol(const PString & protocol) const
{
  return OpalPluginMediaFormat::IsValidForProtocol(protocol);
}


#if OPAL_H323

static H323Capability * CreateNonStandardVideoCap(
  const PluginCodec_Definition * encoderCodec, 
  const PluginCodec_Definition * decoderCodec,
  int subType
);

static H323Capability * CreateGenericVideoCap(
  const PluginCodec_Definition * encoderCodec, 
  const PluginCodec_Definition * decoderCodec,
  int subType
);

static H323Capability * CreateH261Cap(
  const PluginCodec_Definition * encoderCodec, 
  const PluginCodec_Definition * decoderCodec,
  int subType
);

static H323Capability * CreateH263Cap(
  const PluginCodec_Definition * encoderCodec, 
  const PluginCodec_Definition * decoderCodec,
  int subType
);

#endif // OPAL_H323

#endif // OPAL_VIDEO

///////////////////////////////////////////////////////////////////////////////

#if OPAL_T38FAX

OpalPluginFaxMediaFormat::OpalPluginFaxMediaFormat(const PluginCodec_Definition * _encoderCodec,
                                                   const char * rtpEncodingName, /// rtp encoding name
                                                   unsigned frameTime,
                                                   unsigned /*timeUnits*/,           /// RTP units for frameTime (if applicable)
                                                   time_t timeStamp              /// timestamp (for versioning)
                                                  )
  : OpalMediaFormat(CreateCodecName(_encoderCodec),
                    OpalMediaFormat::DefaultDataSessionID,
                    (RTP_DataFrame::PayloadTypes)(((_encoderCodec->flags & PluginCodec_RTPTypeMask) == PluginCodec_RTPTypeDynamic) ? RTP_DataFrame::DynamicBase : _encoderCodec->rtpPayload),
                    rtpEncodingName,
                    FALSE,                                // need jitter
                    8*_encoderCodec->parm.audio.bytesPerFrame*AudioClockRate/frameTime, // bandwidth
                    _encoderCodec->parm.audio.bytesPerFrame,         // size of frame in bytes
                    frameTime,                            // time for frame
                    _encoderCodec->sampleRate,            // clock rate
                    (unsigned int)timeStamp               // timestamp
                  )
  , OpalPluginMediaFormat(_encoderCodec)
{
  PopulateOptions(*this);
}

PObject * OpalPluginFaxMediaFormat::Clone() const
{
  return new OpalPluginFaxMediaFormat(*this);
}

bool OpalPluginFaxMediaFormat::IsValidForProtocol(const PString & protocol) const
{
  return OpalPluginMediaFormat::IsValidForProtocol(protocol);
}

#endif // OPAL_T38FAX


///////////////////////////////////////////////////////////////////////////////

#if OPAL_H323

class H323CodecPluginCapabilityMapEntry {
  public:
    int pluginCapType;
    int h323SubType;
    H323Capability * (* createFunc)(const PluginCodec_Definition * encoderCodec, const PluginCodec_Definition * decoderCodec, int subType);
};

#if OPAL_AUDIO

static H323CodecPluginCapabilityMapEntry audioMaps[] = {
  { PluginCodec_H323Codec_nonStandard,              H245_AudioCapability::e_nonStandard,         &CreateNonStandardAudioCap },
  { PluginCodec_H323AudioCodec_gsmFullRate,	    H245_AudioCapability::e_gsmFullRate,         &CreateGSMCap },
  { PluginCodec_H323AudioCodec_gsmHalfRate,	    H245_AudioCapability::e_gsmHalfRate,         &CreateGSMCap },
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
  { PluginCodec_H323VideoCodec_h263,                H245_VideoCapability::e_h263VideoCapability,    &CreateH263Cap },
  { PluginCodec_H323Codec_generic,                  H245_VideoCapability::e_genericVideoCapability, &CreateGenericVideoCap },
/*
  PluginCodec_H323VideoCodec_h262,                // not yet implemented
  PluginCodec_H323VideoCodec_is11172,             // not yet implemented
*/

  { -1 }
};

#endif  // OPAL_VIDEO

#endif // OPAL_H323

//////////////////////////////////////////////////////////////////////////////

OpalPluginTranscoder::OpalPluginTranscoder(const PluginCodec_Definition * defn, BOOL isEnc)
  : codecDef(defn)
  , isEncoder(isEnc)
  , context(codecDef->createCodec != NULL ? (*codecDef->createCodec)(codecDef) : NULL)
  , setCodecOptions(defn, PLUGINCODEC_CONTROL_SET_CODEC_OPTIONS)
  , getOutputDataSizeControl(defn, PLUGINCODEC_CONTROL_GET_OUTPUT_DATA_SIZE)
{
}


OpalPluginTranscoder::~OpalPluginTranscoder()
{
  if (codecDef != NULL && codecDef->destroyCodec != NULL)
    (*codecDef->destroyCodec)(codecDef, context);
}

//////////////////////////////////////////////////////////////////////////////
//
// Plugin framed audio codec classes
//

#if OPAL_AUDIO
OpalPluginFramedAudioTranscoder::OpalPluginFramedAudioTranscoder(PluginCodec_Definition * _codec, BOOL _isEncoder, const char * rawFormat)
  : OpalFramedTranscoder( (strcmp(_codec->sourceFormat, "L16") == 0) ? rawFormat : _codec->sourceFormat,
                          (strcmp(_codec->destFormat, "L16") == 0)   ? rawFormat : _codec->destFormat,
                         _isEncoder ? _codec->parm.audio.samplesPerFrame*2 : _codec->parm.audio.bytesPerFrame,
                         _isEncoder ? _codec->parm.audio.bytesPerFrame     : _codec->parm.audio.samplesPerFrame*2)
  , OpalPluginTranscoder(_codec, _isEncoder)
{ 
  inputIsRTP  = (codecDef->flags & PluginCodec_InputTypeMask)  == PluginCodec_InputTypeRTP;
  outputIsRTP = (codecDef->flags & PluginCodec_OutputTypeMask) == PluginCodec_OutputTypeRTP;
}

BOOL OpalPluginFramedAudioTranscoder::ConvertFrame(const BYTE * input,
                                                   PINDEX & consumed,
                                                   BYTE * output,
                                                   PINDEX & created)
{
  unsigned int fromLen = consumed;
  unsigned int toLen   = created;
  unsigned flags = 0;

  BOOL stat = Transcode(input, &fromLen, output, &toLen, &flags);
  consumed = fromLen;
  created  = toLen;

  return stat;
}

BOOL OpalPluginFramedAudioTranscoder::ConvertSilentFrame(BYTE * buffer)
{ 
  if (codecDef == NULL)
    return FALSE;

  unsigned length;

  // for a decoder, this mean that we need to create a silence frame
  // which is easy - ask the decoder, or just create silence
  if (!isEncoder) {
    length = codecDef->parm.audio.samplesPerFrame*2;
    if ((codecDef->flags & PluginCodec_DecodeSilence) == 0) {
      memset(buffer, 0, length); 
      return TRUE;
    }
  }

  // for an encoder, we encode silence but set the flag so it can do something special if need be
  else {
    length = codecDef->parm.audio.bytesPerFrame;
    if ((codecDef->flags & PluginCodec_EncodeSilence) == 0) {
      PShortArray silence(codecDef->parm.audio.samplesPerFrame);
      memset(silence.GetPointer(), 0, codecDef->parm.audio.samplesPerFrame*sizeof(short));
      unsigned silenceLen = codecDef->parm.audio.samplesPerFrame * sizeof(short);
      unsigned flags = 0;
      return Transcode(silence, &silenceLen, buffer, &length, &flags);
    }
  }

  unsigned flags = PluginCodec_CoderSilenceFrame;
  return Transcode(NULL, NULL, buffer, &length, &flags);
}


//////////////////////////////////////////////////////////////////////////////
//
// Plugin streamed audio codec classes
//

OpalPluginStreamedAudioTranscoder::OpalPluginStreamedAudioTranscoder(PluginCodec_Definition * _codec,
                                                                     BOOL _isEncoder,
                                                                     unsigned inputBits,
                                                                     unsigned outputBits,
                                                                     PINDEX optimalBits)
  : OpalStreamedTranscoder((strcmp(_codec->sourceFormat, "L16") == 0) ? OpalPCM16 : _codec->sourceFormat,
                           (strcmp(_codec->destFormat, "L16") == 0) ? OpalPCM16 : _codec->destFormat,
                           inputBits, outputBits, optimalBits)
  , OpalPluginTranscoder(_codec, _isEncoder)
{ 
}

OpalPluginStreamedAudioEncoder::OpalPluginStreamedAudioEncoder(PluginCodec_Definition * _codec, BOOL)
  : OpalPluginStreamedAudioTranscoder(_codec, TRUE,
                                      16,
                                      (_codec->flags & PluginCodec_BitsPerSampleMask) >> PluginCodec_BitsPerSamplePos,
                                      _codec->parm.audio.recommendedFramesPerPacket)
{
}

int OpalPluginStreamedAudioEncoder::ConvertOne(int _sample) const
{
  short sample = (short)_sample;
  unsigned int fromLen = sizeof(sample);
  int to;
  unsigned toLen = sizeof(to);
  unsigned flags = 0;
  return Transcode(&sample, &fromLen, &to, &toLen, &flags) ? to : -1;
}


OpalPluginStreamedAudioDecoder::OpalPluginStreamedAudioDecoder(PluginCodec_Definition * _codec, BOOL)
  : OpalPluginStreamedAudioTranscoder(_codec, FALSE,
                                      (_codec->flags & PluginCodec_BitsPerSampleMask) >> PluginCodec_BitsPerSamplePos,
                                      16,
                                      _codec->parm.audio.recommendedFramesPerPacket)
{
}

int OpalPluginStreamedAudioDecoder::ConvertOne(int codedSample) const
{
  unsigned int fromLen = sizeof(codedSample);
  short to;
  unsigned toLen = sizeof(to);
  unsigned flags = 0;
  return Transcode(&codedSample, &fromLen, &to, &toLen, &flags) ? to : -1;
}

#endif //  OPAL_AUDIO

#if OPAL_VIDEO

/////////////////////////////////////////////////////////////////////////////

OpalPluginVideoTranscoder::OpalPluginVideoTranscoder(const PluginCodec_Definition * _codec, BOOL _isEncoder)
  : OpalVideoTranscoder(_codec->sourceFormat, _codec->destFormat)
  , OpalPluginTranscoder(_codec, _isEncoder)
{ 
  bufferRTP = NULL;
}

OpalPluginVideoTranscoder::~OpalPluginVideoTranscoder()
{ 
  delete bufferRTP;
}


BOOL OpalPluginVideoTranscoder::UpdateOutputMediaFormat(const OpalMediaFormat & fmt)
{
  if (setCodecOptions.Exists()) {
    PStringArray list;
    for (PINDEX i = 0; i < fmt.GetOptionCount(); i++) {
      const OpalMediaOption & option = fmt.GetOption(i);
      list += option.GetName();
      list += option.AsString();
    }

#if PTRACING
    if (PTrace::CanTrace(4)) {
      ostream & strm = PTrace::Begin(4, __FILE__, __LINE__);
      strm << "OpalPlugin\tSetting " << (isEncoder ? "en" : "de") << "coder options:\n";
      for (PINDEX i = 0; i < fmt.GetOptionCount(); i++) {
        const OpalMediaOption & option = fmt.GetOption(i);
        strm << "    " << option.GetName() << " = " << option.AsString() << '\n';
      }
      strm << PTrace::End;
    }
#endif

    char ** options = list.ToCharArray();
    setCodecOptions.Call(options, sizeof(options), context);
    free(options);
  }

  return TRUE;
}


PINDEX OpalPluginVideoTranscoder::GetOptimalDataFrameSize(BOOL /*input*/) const
{
  return (frameWidth * frameHeight * 12) / 8;
}

BOOL OpalPluginVideoTranscoder::ConvertFrames(const RTP_DataFrame & src, RTP_DataFrameList & dstList)
{
  dstList.RemoveAll();

  // get the size of the output buffer
  int outputDataSize;
  if (getOutputDataSizeControl.Call(&outputDataSize, sizeof(outputDataSize), context) <= 0)
    outputDataSize = isEncoder ? PluginCodec_RTP_MaxPacketSize : GetOptimalDataFrameSize(FALSE);

  unsigned flags;

  if (isEncoder) {

    do {

      // create the output buffer, outputDataSize is supposed to include the
      // RTP header size, so take that off as ctor adds it back.
      RTP_DataFrame * dst = new RTP_DataFrame(outputDataSize - PluginCodec_RTP_MinHeaderSize);
      dst->SetPayloadType(GetPayloadType(FALSE));

      // call the codec function
      unsigned int fromLen = src.GetSize();
      unsigned int toLen = dst->GetSize();
      flags = forceIFrame ? PluginCodec_CoderForceIFrame : 0;

      if (!Transcode((const BYTE *)src, &fromLen, dst->GetPointer(), &toLen, &flags)) {
        delete dst;
        return FALSE;
      }

      if ((flags & PluginCodec_ReturnCoderIFrame) != 0) {
        PTRACE(forceIFrame ? 3 : 4, "OpalPlugin\tSending I-Frame" << (forceIFrame ? " in response to videoFastUpdate" : ""));
        forceIFrame = false;
      }

      if (toLen > 0) {
        dst->SetPayloadSize(toLen - dst->GetHeaderSize());
        dstList.Append(dst);
      }

    } while ((flags & PluginCodec_ReturnCoderLastFrame) == 0);
  }

  else {
    // We use the data size indicated by plug in as a payload size, we do not adjust the size
    // downward as many plug ins forget to add the RTP header size in its output data size and
    // it doesn't hurt to make thisbuffer an extra 12 bytes long.

    if (bufferRTP == NULL)
      bufferRTP = new RTP_DataFrame(outputDataSize);
    else
      bufferRTP->SetPayloadSize(outputDataSize);
    bufferRTP->SetPayloadType(GetPayloadType(FALSE));

    // call the codec function
    unsigned int fromLen = src.GetHeaderSize() + src.GetPayloadSize();
    unsigned int toLen = bufferRTP->GetSize();
    flags = 0;

    if (!Transcode((const BYTE *)src, &fromLen, bufferRTP->GetPointer(), &toLen, &flags))
      return FALSE;

    if ((flags & PluginCodec_ReturnCoderRequestIFrame) != 0) {
      if (commandNotifier != PNotifier()) {
        OpalVideoUpdatePicture updatePictureCommand;
        commandNotifier(updatePictureCommand, 0); 
        PTRACE (3, "OpalPlugin\tCould not decode frame, sending VideoUpdatePicture in hope of an I-Frame.");
      }
    }

    if (toLen > (unsigned)bufferRTP->GetHeaderSize() && (flags & PluginCodec_ReturnCoderLastFrame) != 0) {
      bufferRTP->SetPayloadSize(toLen - bufferRTP->GetHeaderSize());
      dstList.Append(bufferRTP);
      bufferRTP = NULL;
    }
  }

  return TRUE;
};


#endif // OPAL_VIDEO



//////////////////////////////////////////////////////////////////////////////
//
// Fax transcoder classes
//

#if OPAL_T38FAX

class OpalFaxAudioTranscoder : public OpalPluginFramedAudioTranscoder
{
  PCLASSINFO(OpalFaxAudioTranscoder, OpalPluginFramedAudioTranscoder);
  public:
    OpalFaxAudioTranscoder(PluginCodec_Definition * _codec, BOOL _isEncoder)
      : OpalPluginFramedAudioTranscoder(_codec, _isEncoder, "PCM-16-Fax") 
    { 
      bufferRTP = NULL;
    }

    ~OpalFaxAudioTranscoder()
    { 
    }

    virtual void SetInstanceID(const BYTE * instance, unsigned instanceLen)
    {
      if (instance != NULL && instanceLen > 0) {
        OpalPluginControl ctl(codecDef, PLUGINCODEC_CONTROL_SET_INSTANCE_ID);
        ctl.Call((void *)instance, instanceLen, context);
      }
    }

    BOOL ConvertFrames(const RTP_DataFrame & src, RTP_DataFrameList & dstList);

  protected:
    RTP_DataFrame * bufferRTP;
};

BOOL OpalFaxAudioTranscoder::ConvertFrames(const RTP_DataFrame & src, RTP_DataFrameList & dstList)
{
  dstList.RemoveAll();

  // get the size of the output buffer
  int outputDataSize = 400;
  //if (!CallCodecControl(GET_OUTPUT_DATA_SIZE_CONTROL, NULL, NULL, outputDataSize))
  // return FALSE;

  unsigned flags = 0;

  if (isEncoder) {

    do {

      // create the output buffer
      RTP_DataFrame * dst = new RTP_DataFrame(outputDataSize);
      dst->SetPayloadType(GetPayloadType(FALSE));

      // call the codec function
      unsigned int fromLen = src.GetSize();
      unsigned int toLen = dst->GetSize();

      BOOL stat = Transcode((const BYTE *)src, &fromLen, dst->GetPointer(), &toLen, &flags);

      if (!stat) {
        delete dst;
        return FALSE;
      }

      if (toLen > 0) {
        dst->SetPayloadSize(toLen - dst->GetHeaderSize());
        dstList.Append(dst);
      }

    } while ((flags & PluginCodec_ReturnCoderLastFrame) == 0);
  }

  else {

    unsigned int fromLen = src.GetHeaderSize() + src.GetPayloadSize();

    do {
      if (bufferRTP == NULL)
        bufferRTP = new RTP_DataFrame(outputDataSize);
      else
        bufferRTP->SetPayloadSize(outputDataSize);
      bufferRTP->SetPayloadType(GetPayloadType(FALSE));

      // call the codec function
      unsigned int toLen = bufferRTP->GetSize();
      flags = 0;
      if (!Transcode((const BYTE *)src, &fromLen, bufferRTP->GetPointer(), &toLen, &flags))
        return FALSE;

      if (toLen > (unsigned)bufferRTP->GetHeaderSize()) {
        bufferRTP->SetPayloadSize(toLen - bufferRTP->GetHeaderSize());
        dstList.Append(bufferRTP);
        bufferRTP = NULL;
      }

      fromLen = 0;
      
    } while ((flags & PluginCodec_ReturnCoderLastFrame) == 0);
  }

  return TRUE;
};

#endif // OPAL_T38FAX

//////////////////////////////////////////////////////////////////////////////

#if OPAL_H323

#if OPAL_AUDIO

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling most audio plugin capabilities
//

H323AudioPluginCapability::H323AudioPluginCapability(const PluginCodec_Definition * _encoderCodec,
                              const PluginCodec_Definition * _decoderCodec,
                              unsigned _pluginSubType)
: H323AudioCapability(), H323PluginCapabilityInfo(_encoderCodec, _decoderCodec),
  pluginSubType(_pluginSubType)
{ 
}

// this constructor is only used when creating a capability without a codec
H323AudioPluginCapability::H323AudioPluginCapability(const PString & _mediaFormat,
                                                     const PString & _baseName,
                                                     unsigned _type)
  : H323AudioCapability(), H323PluginCapabilityInfo(_baseName)
{ 
  for (PINDEX i = 0; audioMaps[i].pluginCapType >= 0; i++) {
    if (audioMaps[i].pluginCapType == (int)_type) { 
      pluginSubType = audioMaps[i].h323SubType;
      break;
    }
  }
  rtpPayloadType = OpalMediaFormat(_mediaFormat).GetPayloadType();
}

PObject * H323AudioPluginCapability::Clone() const
{
  return new H323AudioPluginCapability(*this);
}

PString H323AudioPluginCapability::GetFormatName() const
{
  return H323PluginCapabilityInfo::GetFormatName();
}

unsigned H323AudioPluginCapability::GetSubType() const
{
  return pluginSubType;
}

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling G.723.1 codecs
//

H323PluginG7231Capability::H323PluginG7231Capability(const PluginCodec_Definition * _encoderCodec,
                          const PluginCodec_Definition * _decoderCodec,
                          BOOL _annexA)
  : H323AudioPluginCapability(_encoderCodec, _decoderCodec, H245_AudioCapability::e_g7231),
    annexA(_annexA)
{ }

// this constructor is used for creating empty codecs
H323PluginG7231Capability::H323PluginG7231Capability(const OpalMediaFormat & fmt, BOOL _annexA)
  : H323AudioPluginCapability(fmt, fmt, H245_AudioCapability::e_g7231),
    annexA(_annexA)
{ }

PObject * H323PluginG7231Capability::Clone() const
{ return new H323PluginG7231Capability(*this); }

BOOL H323PluginG7231Capability::OnSendingPDU(H245_AudioCapability & cap, unsigned packetSize) const
{
  cap.SetTag(H245_AudioCapability::e_g7231);
  H245_AudioCapability_g7231 & g7231 = cap;
  g7231.m_maxAl_sduAudioFrames = packetSize;
  g7231.m_silenceSuppression = annexA;
  return TRUE;
}

BOOL H323PluginG7231Capability::OnReceivedPDU(const H245_AudioCapability & cap,  unsigned & packetSize)
{
  if (cap.GetTag() != H245_AudioCapability::e_g7231)
    return FALSE;
  const H245_AudioCapability_g7231 & g7231 = cap;
  packetSize = g7231.m_maxAl_sduAudioFrames;
  annexA = g7231.m_silenceSuppression;
  return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling GSM plugin capabilities
//

class H323GSMPluginCapability : public H323AudioPluginCapability
{
  PCLASSINFO(H323GSMPluginCapability, H323AudioPluginCapability);
  public:
    H323GSMPluginCapability(const PluginCodec_Definition * _encoderCodec,
                            const PluginCodec_Definition * _decoderCodec,
                            int _pluginSubType, int _comfortNoise, int _scrambled)
      : H323AudioPluginCapability(_encoderCodec, _decoderCodec, _pluginSubType),
        comfortNoise(_comfortNoise), scrambled(_scrambled)
    { }

    Comparison Compare(const PObject & obj) const;

    virtual PObject * Clone() const
    { return new H323GSMPluginCapability(*this); }

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

#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////

OpalPluginCodecManager::OpalPluginCodecManager(PPluginManager * _pluginMgr)
  : PPluginModuleManager(PLUGIN_CODEC_GET_CODEC_FN_STR, _pluginMgr)
{
  // instantiate all of the static codecs
  {
    H323StaticPluginCodecFactory::KeyList_T keyList = H323StaticPluginCodecFactory::GetKeyList();
    H323StaticPluginCodecFactory::KeyList_T::const_iterator r;
    for (r = keyList.begin(); r != keyList.end(); ++r) {
      H323StaticPluginCodec * instance = PFactory<H323StaticPluginCodec>::CreateInstance(*r);
      if (instance == NULL) {
        PTRACE(4, "OpalPlugin\tCannot instantiate static codec plugin " << *r);
      } else {
        PTRACE(4, "OpalPlugin\tLoading static codec plugin " << *r);
        RegisterStaticCodec(*r, instance->Get_GetAPIFn(), instance->Get_GetCodecFn());
      }
    }
  }

  // cause the plugin manager to load all dynamic plugins
  pluginMgr->AddNotifier(PCREATE_NOTIFIER(OnLoadModule), TRUE);

#if OPAL_H323
  // register the capabilities
  while (capabilityCreateList.size() > 0) {
    CapabilityCreateListType::iterator r = capabilityCreateList.begin();
    RegisterCapability(r->encoderCodec, r->decoderCodec);
    capabilityCreateList.erase(r);
  }
#endif
}

OpalPluginCodecManager::~OpalPluginCodecManager()
{
}

void OpalPluginCodecManager::OnShutdown()
{
  mediaFormatsOnHeap.RemoveAll();

#if OPAL_H323
  // unregister the plugin capabilities
  H323CapabilityFactory::UnregisterAll();
#endif
}

void OpalPluginCodecManager::OnLoadPlugin(PDynaLink & dll, INT code)
{
  PluginCodec_GetCodecFunction getCodecs;
  if (!dll.GetFunction(PString(signatureFunctionName), (PDynaLink::Function &)getCodecs)) {
    PTRACE(2, "OpalPlugin\tPlugin Codec DLL " << dll.GetName() << " is not a plugin codec");
    return;
  }

  unsigned int count;
  PluginCodec_Definition * codecs = (*getCodecs)(&count, PLUGIN_CODEC_VERSION_OPTIONS);
  if (codecs == NULL || count == 0) {
    PTRACE(1, "OpalPlugin\tPlugin Codec DLL " << dll.GetName() << " contains no codec definitions");
    return;
  } 

  // get handler for this plugin type
  PString name = dll.GetName();
  PFactory<OpalPluginCodecHandler>::KeyList_T keys = PFactory<OpalPluginCodecHandler>::GetKeyList();
  PFactory<OpalPluginCodecHandler>::KeyList_T::const_iterator r;
  OpalPluginCodecHandler * handler = NULL;
  for (r = keys.begin(); r != keys.end(); ++r) {
    if (name.Right(r->length()) *= *r) {
      PTRACE(3, "OpalPlugin\tUsing customer handler for codec " << name);
      handler = PFactory<OpalPluginCodecHandler>::CreateInstance(*r);
      break;
    }
  }

  if (handler == NULL) {
    PTRACE(3, "OpalPlugin\tUsing default handler for plugin codec " << name);
    handler = new OpalPluginCodecHandler;
  }

  switch (code) {

    // plugin loaded
    case 0:
      RegisterCodecPlugins(count, codecs, handler);
      break;

    // plugin unloaded
    case 1:
      UnregisterCodecPlugins(count, codecs, handler);
      break;

    default:
      break;
  }

  delete handler;
}

void OpalPluginCodecManager::RegisterStaticCodec(
      const H323StaticPluginCodecFactory::Key_T & PTRACE_PARAM(name),
      PluginCodec_GetAPIVersionFunction /*getApiVerFn*/,
      PluginCodec_GetCodecFunction getCodecFn)
{
  unsigned int count;
  PluginCodec_Definition * codecs = (*getCodecFn)(&count, PLUGIN_CODEC_VERSION_OPTIONS);
  if (codecs == NULL || count == 0) {
    PTRACE(1, "OpalPlugin\tStatic codec " << name << " contains no codec definitions");
    return;
  } 

  OpalPluginCodecHandler * handler = new OpalPluginCodecHandler;
  RegisterCodecPlugins(count, codecs, handler);
  delete handler;
}

void OpalPluginCodecManager::RegisterCodecPlugins(unsigned int count, PluginCodec_Definition * codecList, OpalPluginCodecHandler * handler)
{
  // make sure all non-timestamped codecs have the same concept of "now"
  static time_t codecNow = ::time(NULL);

  unsigned i, j ;
  for (i = 0; i < count; i++) {

    PluginCodec_Definition & encoder = codecList[i];

    BOOL videoSupported = encoder.version >= PLUGIN_CODEC_VERSION_VIDEO;
    BOOL faxSupported   = encoder.version >= PLUGIN_CODEC_VERSION_FAX;

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
        ) ||
         (
           faxSupported &&
           ((encoder.flags & PluginCodec_MediaTypeMask) == PluginCodec_MediaTypeFax) && 
           strcmp(encoder.sourceFormat, "L16") == 0
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
          RegisterPluginPair(&encoder, &decoder, handler);
          found = TRUE;

          PTRACE(3, "OpalPlugin\tPlugin codec " << encoder.descr << " defined");
          break;
        }
      }
    }
    if (!found && isEncoder) {
      PTRACE(2, "OpalPlugin\tCannot find decoder for plugin encoder " << encoder.descr);
    }
  }
}

void OpalPluginCodecManager::UnregisterCodecPlugins(unsigned int, PluginCodec_Definition *, OpalPluginCodecHandler * )
{
}

void OpalPluginCodecManager::RegisterPluginPair(
       PluginCodec_Definition * encoderCodec,
       PluginCodec_Definition * decoderCodec,
       OpalPluginCodecHandler * handler
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
      frameTime = (8 * encoderCodec->usPerFrame) / 1000;
      clockRate = encoderCodec->sampleRate;
      break;
#endif
#if OPAL_T38FAX
    case PluginCodec_MediaTypeFax:
      defaultSessionID = OpalMediaFormat::DefaultDataSessionID;
      frameTime = (8 * encoderCodec->usPerFrame) / 1000;
      clockRate = encoderCodec->sampleRate;
      break;
#endif
    default:
      break;
  }

  // add the media format
  if (defaultSessionID == 0) {
    PTRACE(1, "OpalPlugin\tCodec DLL provides unknown media format " << (int)(encoderCodec->flags & PluginCodec_MediaTypeMask));
  } else {
    PString fmtName = CreateCodecName(encoderCodec);
    OpalMediaFormat existingFormat(fmtName);
    if (existingFormat.IsValid() && existingFormat.GetCodecBaseTime() >= timeStamp) {
      PTRACE(2, "OpalPlugin\tNewer media format " << fmtName << " already exists");
    } else {
      PTRACE(3, "OpalPlugin\tCreating new media format " << fmtName);

      OpalMediaFormat * mediaFormat = NULL;

      // manually register the new singleton type, as we do not have a concrete type
      switch (encoderCodec->flags & PluginCodec_MediaTypeMask) {
#if OPAL_VIDEO
        case PluginCodec_MediaTypeVideo:
          mediaFormat = handler->OnCreateVideoFormat(*this, encoderCodec, encoderCodec->sdpFormat, timeStamp);
          break;
#endif
#if OPAL_AUDIO
        case PluginCodec_MediaTypeAudio:
        case PluginCodec_MediaTypeAudioStreamed:
          mediaFormat = handler->OnCreateAudioFormat(*this, encoderCodec, encoderCodec->sdpFormat, frameTime, clockRate, timeStamp);
          break;
#endif
#if OPAL_T38FAX
        case PluginCodec_MediaTypeFax:
          mediaFormat = handler->OnCreateFaxFormat(*this, encoderCodec, encoderCodec->sdpFormat, frameTime, clockRate, timeStamp);
          break;
#endif
        default:
          PTRACE(3, "OpalPlugin\tOnknown Media Type " << (encoderCodec->flags & PluginCodec_MediaTypeMask));
          return;
      }

      // Remember format so we can deallocate it on shut down
      mediaFormatsOnHeap.Append(mediaFormat);

      // if the codec has been flagged to use a shared RTP payload type, then find a codec with the same SDP name
      // and clock rate and use that RTP code rather than creating a new one. That prevents codecs (like Speex) from 
      // consuming dozens of dynamic RTP types
      if ((encoderCodec->flags & PluginCodec_RTPTypeShared) != 0 && (encoderCodec->sdpFormat != NULL)) {
        OpalMediaFormatList list = OpalMediaFormat::GetAllRegisteredMediaFormats();
        for (PINDEX i = 0; i < list.GetSize(); i++) {
          OpalMediaFormat * opalFmt = &list[i];
#if OPAL_AUDIO
          {
            OpalPluginAudioMediaFormat * fmt = dynamic_cast<OpalPluginAudioMediaFormat *>(opalFmt);
            if (
                (fmt != NULL) && 
                (encoderCodec->sampleRate == fmt->codecDef->sampleRate) &&
                (fmt->codecDef->sdpFormat != NULL) &&
                (strcasecmp(encoderCodec->sdpFormat, fmt->codecDef->sdpFormat) == 0)
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
                (encoderCodec->sampleRate == fmt->codecDef->sampleRate) &&
                (fmt->codecDef->sdpFormat != NULL) &&
                (strcasecmp(encoderCodec->sdpFormat, fmt->codecDef->sdpFormat) == 0)
                ) {
              mediaFormat->rtpPayloadType = fmt->GetPayloadType();
              break;
            }
          }
#endif
#if OPAL_T38FAX
          {
            OpalPluginFaxMediaFormat * fmt = dynamic_cast<OpalPluginFaxMediaFormat *>(opalFmt);
            if (
                (fmt != NULL) && 
                (encoderCodec->sampleRate == fmt->codecDef->sampleRate) &&
                (fmt->codecDef->sdpFormat != NULL) &&
                (strcasecmp(encoderCodec->sdpFormat, fmt->codecDef->sdpFormat) == 0)
                ) {
              mediaFormat->rtpPayloadType = fmt->GetPayloadType();
              break;
            }
          }
#endif
        }
      }
      OpalMediaFormat::SetRegisteredMediaFormat(*mediaFormat);
    }
  }

  // Create transcoder factories for the codecs
  switch (encoderCodec->flags & PluginCodec_MediaTypeMask) {
#if OPAL_VIDEO
    case PluginCodec_MediaTypeVideo:
      handler->RegisterVideoTranscoder(OpalYUV420P, encoderCodec->destFormat, encoderCodec, TRUE);
      handler->RegisterVideoTranscoder(encoderCodec->destFormat, OpalYUV420P, decoderCodec, FALSE);
      break;
#endif
#if OPAL_AUDIO
    case PluginCodec_MediaTypeAudio:
      if (encoderCodec->sampleRate == 8000) {
        new OpalPluginTranscoderFactory<OpalPluginFramedAudioTranscoder>::Worker(OpalTranscoderKey(OpalPCM16,                encoderCodec->destFormat), encoderCodec, TRUE);
        new OpalPluginTranscoderFactory<OpalPluginFramedAudioTranscoder>::Worker(OpalTranscoderKey(encoderCodec->destFormat, OpalPCM16),                 decoderCodec, FALSE);
      }
      else if (encoderCodec->sampleRate == 16000)
      {
        new OpalPluginTranscoderFactory<OpalPluginFramedAudioTranscoder>::Worker(OpalTranscoderKey(OpalPCM16_16KHZ,          encoderCodec->destFormat), encoderCodec, TRUE);
        new OpalPluginTranscoderFactory<OpalPluginFramedAudioTranscoder>::Worker(OpalTranscoderKey(encoderCodec->destFormat, OpalPCM16_16KHZ),                 decoderCodec, FALSE);
      }
      else
      {
        PTRACE(1, "OpalPlugin\tAudio plugin defines unsupported clock rate " << encoderCodec->sampleRate);
      }
      break;
    case PluginCodec_MediaTypeAudioStreamed:
      if (encoderCodec->sampleRate == 8000) {
        new OpalPluginTranscoderFactory<OpalPluginStreamedAudioEncoder>::Worker(OpalTranscoderKey(OpalPCM16,                encoderCodec->destFormat), encoderCodec, TRUE);
        new OpalPluginTranscoderFactory<OpalPluginStreamedAudioDecoder>::Worker(OpalTranscoderKey(encoderCodec->destFormat, OpalPCM16),                 decoderCodec, FALSE);
      }
      else if (encoderCodec->sampleRate == 16000)
      {
        new OpalPluginTranscoderFactory<OpalPluginStreamedAudioEncoder>::Worker(OpalTranscoderKey(OpalPCM16_16KHZ,          encoderCodec->destFormat), encoderCodec, TRUE);
        new OpalPluginTranscoderFactory<OpalPluginStreamedAudioDecoder>::Worker(OpalTranscoderKey(encoderCodec->destFormat, OpalPCM16_16KHZ),                 decoderCodec, FALSE);
      }
      else
      {
        PTRACE(1, "OpalPlugin\tAudio plugin defines unsupported clock rate " << encoderCodec->sampleRate);
      }
      break;
#endif
#if OPAL_T38FAX
    case PluginCodec_MediaTypeFax:
      new OpalPluginTranscoderFactory<OpalFaxAudioTranscoder>::Worker(OpalTranscoderKey(GetOpalPCM16Fax(),        encoderCodec->destFormat), encoderCodec, TRUE);
      new OpalPluginTranscoderFactory<OpalFaxAudioTranscoder>::Worker(OpalTranscoderKey(encoderCodec->destFormat, GetOpalPCM16Fax()),        decoderCodec, FALSE);
      break;
#endif
    default:
      break;
  }

  OpalPluginControl isValid(encoderCodec, PLUGINCODEC_CONTROL_VALID_FOR_PROTOCOL);
  if (encoderCodec->h323CapabilityType == PluginCodec_H323Codec_NoH323 ||
      (isValid.Exists() && !isValid.Call((void *)"h323", sizeof(const char *)))) {
    PTRACE(2, "OpalPlugin\tNot adding H.323 capability for plugin codec " << encoderCodec->destFormat << " as this has been specifically disabled");
    return;
  }

#if OPAL_H323
  PTRACE(4, "OpalPlugin\tDeferring creation of H.323 capability for plugin codec " << encoderCodec->destFormat);
  capabilityCreateList.push_back(CapabilityListCreateEntry(encoderCodec, decoderCodec));
#endif
}

#if OPAL_H323

void OpalPluginCodecManager::RegisterCapability(PluginCodec_Definition * encoderCodec, PluginCodec_Definition * decoderCodec)
{
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
    PTRACE(1, "OpalPlugin\tCannot create capability for unknown plugin codec media format " << (int)(encoderCodec->flags & PluginCodec_MediaTypeMask));
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
              PTRACE(4, "OpalPlugin\tWarning - no capability creation function for " << CreateCodecName(encoderCodec));
              // all video caps are created using the create functions
              break;
#endif // OPAL_VIDEO

            default:
              break;
          }
        }

        // manually register the new singleton type, as we do not have a concrete type
        if (cap != NULL)
          H323CapabilityFactory::Register(CreateCodecName(encoderCodec), cap);
        break;
      }
    }
  }
}
#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////

OpalPluginCodecHandler::OpalPluginCodecHandler()
{
}

#if OPAL_AUDIO
OpalMediaFormat * OpalPluginCodecHandler::OnCreateAudioFormat(OpalPluginCodecManager & /*mgr*/,
                                                     const PluginCodec_Definition * encoderCodec,
                                                                       const char * rtpEncodingName,
                                                                           unsigned frameTime,
                                                                           unsigned timeUnits,
                                                                             time_t timeStamp)
{
  return new OpalPluginAudioMediaFormat(encoderCodec, rtpEncodingName, frameTime, timeUnits, timeStamp);
}
#endif

#if OPAL_VIDEO
OpalMediaFormat * OpalPluginCodecHandler::OnCreateVideoFormat(OpalPluginCodecManager & /*mgr*/,
                                                     const PluginCodec_Definition * encoderCodec,
                                                                       const char * rtpEncodingName,
                                                                             time_t timeStamp)
{
  return new OpalPluginVideoMediaFormat(encoderCodec, rtpEncodingName, timeStamp);
}

void OpalPluginCodecHandler::RegisterVideoTranscoder(const PString & src, const PString & dst, PluginCodec_Definition * codec, BOOL v)
{
  new OpalPluginTranscoderFactory<OpalPluginVideoTranscoder>::Worker(OpalTranscoderKey(src, dst), codec, v);
}

#endif

#if OPAL_T38FAX
OpalMediaFormat * OpalPluginCodecHandler::OnCreateFaxFormat(OpalPluginCodecManager & /*mgr*/,
                                                   const PluginCodec_Definition * encoderCodec,
                                                                     const char * rtpEncodingName,
                                                                         unsigned frameTime,
                                                                         unsigned timeUnits,
                                                                           time_t timeStamp)
{
  return new OpalPluginFaxMediaFormat(encoderCodec, rtpEncodingName, frameTime, timeUnits, timeStamp);
}
#endif

/////////////////////////////////////////////////////////////////////////////

#if OPAL_H323

#if OPAL_AUDIO

H323Capability * CreateNonStandardAudioCap(const PluginCodec_Definition * encoderCodec,
                                           const PluginCodec_Definition * decoderCodec,
                                           int /*subType*/) 
{
  PluginCodec_H323NonStandardCodecData * pluginData =  (PluginCodec_H323NonStandardCodecData *)encoderCodec->h323CapabilityData;
  if (pluginData == NULL) {
    return new H323CodecPluginNonStandardAudioCapability(encoderCodec,
                                                         decoderCodec,
                                                         (const unsigned char *)encoderCodec->descr, 
                                                         strlen(encoderCodec->descr));
  }

  else if (pluginData->capabilityMatchFunction != NULL) 
    return new H323CodecPluginNonStandardAudioCapability(encoderCodec,
                                                         decoderCodec,
                                                         (H323NonStandardCapabilityInfo::CompareFuncType)pluginData->capabilityMatchFunction,
                                                         pluginData->data,
                                                         pluginData->dataLength);
  else
    return new H323CodecPluginNonStandardAudioCapability(encoderCodec,
                                                         decoderCodec,
                                                         pluginData->data,
                                                         pluginData->dataLength);
}

H323Capability *CreateGenericAudioCap(const PluginCodec_Definition * encoderCodec,
                                      const PluginCodec_Definition * decoderCodec,
                                      int /*subType*/) 
{
  return new H323CodecPluginGenericAudioCapability(encoderCodec, decoderCodec, (PluginCodec_H323GenericCodecData *)encoderCodec->h323CapabilityData);
}

H323Capability * CreateG7231Cap(const PluginCodec_Definition * encoderCodec,
                                const PluginCodec_Definition * decoderCodec,
                                int /*subType*/) 
{
  return new H323PluginG7231Capability(encoderCodec, decoderCodec, decoderCodec->h323CapabilityData != 0);
}


H323Capability * CreateGSMCap(const PluginCodec_Definition * encoderCodec,
                              const PluginCodec_Definition * decoderCodec,
                              int subType) 
{
  PluginCodec_H323AudioGSMData * pluginData =  (PluginCodec_H323AudioGSMData *)encoderCodec->h323CapabilityData;
  return new H323GSMPluginCapability(encoderCodec, decoderCodec, subType, pluginData->comfortNoise, pluginData->scrambled);
}

#endif // OPAL_AUDIO

#if OPAL_VIDEO

H323Capability * CreateNonStandardVideoCap(const PluginCodec_Definition * encoderCodec,
                                           const PluginCodec_Definition * decoderCodec,
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

H323Capability *CreateGenericVideoCap(const PluginCodec_Definition * encoderCodec,  
                                      const PluginCodec_Definition * decoderCodec,
                                      int /*subType*/) 
{
  return new H323CodecPluginGenericVideoCapability(encoderCodec, decoderCodec, (PluginCodec_H323GenericCodecData *)encoderCodec->h323CapabilityData);
}


H323Capability * CreateH261Cap(const PluginCodec_Definition * encoderCodec,
                               const PluginCodec_Definition * decoderCodec,
                               int /*subType*/) 
{
  PTRACE(4, "OpalPlugin\tCreating H.261 plugin capability");
  return new H323H261PluginCapability(encoderCodec, decoderCodec);
}

H323Capability * CreateH263Cap(const PluginCodec_Definition * encoderCodec,
                               const PluginCodec_Definition * decoderCodec,
                               int /*subType*/) 
{
  PTRACE(4, "OpalPlugin\tCreating H.263 plugin capability");
  return new H323H263PluginCapability(encoderCodec, decoderCodec);
}

#endif // OPAL_VIDEO

/////////////////////////////////////////////////////////////////////////////

H323PluginCapabilityInfo::H323PluginCapabilityInfo(const PluginCodec_Definition * _encoderCodec,
                                                   const PluginCodec_Definition * _decoderCodec)
 : encoderCodec(_encoderCodec),
   decoderCodec(_decoderCodec),
   capabilityFormatName(CreateCodecName(_encoderCodec))
{
}

H323PluginCapabilityInfo::H323PluginCapabilityInfo(const PString & _baseName)
 : encoderCodec(NULL),
   decoderCodec(NULL),
   capabilityFormatName(_baseName)
{
}

#if OPAL_AUDIO

/////////////////////////////////////////////////////////////////////////////

H323CodecPluginNonStandardAudioCapability::H323CodecPluginNonStandardAudioCapability(const PluginCodec_Definition * _encoderCodec,
                                                                                     const PluginCodec_Definition * _decoderCodec,
                                                                                     H323NonStandardCapabilityInfo::CompareFuncType compareFunc,
                                                                                     const unsigned char * data, unsigned dataLen)
 : H323NonStandardAudioCapability(compareFunc,data, dataLen), 
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

H323CodecPluginNonStandardAudioCapability::H323CodecPluginNonStandardAudioCapability(const PluginCodec_Definition * _encoderCodec,
                                                                                     const PluginCodec_Definition * _decoderCodec,
                                                                                     const unsigned char * data, unsigned dataLen)
 : H323NonStandardAudioCapability(data, dataLen), 
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

PObject * H323CodecPluginNonStandardAudioCapability::Clone() const
{ return new H323CodecPluginNonStandardAudioCapability(*this); }

PString H323CodecPluginNonStandardAudioCapability::GetFormatName() const
{ return H323PluginCapabilityInfo::GetFormatName();}

/////////////////////////////////////////////////////////////////////////////

H323CodecPluginGenericAudioCapability::H323CodecPluginGenericAudioCapability(const PluginCodec_Definition * _encoderCodec,
                                                                             const PluginCodec_Definition * _decoderCodec,
                                                                             const PluginCodec_H323GenericCodecData *data )
  : H323GenericAudioCapability(data->standardIdentifier, data != NULL ? data->maxBitRate : 0),
    H323PluginCapabilityInfo((PluginCodec_Definition *)_encoderCodec, (PluginCodec_Definition *) _decoderCodec)
{
}

PObject * H323CodecPluginGenericAudioCapability::Clone() const
{ return new H323CodecPluginGenericAudioCapability(*this); }

PString H323CodecPluginGenericAudioCapability::GetFormatName() const
{ return H323PluginCapabilityInfo::GetFormatName();}

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
  gsm.m_audioUnitSize = packetSize * encoderCodec->parm.audio.bytesPerFrame;
  gsm.m_comfortNoise  = comfortNoise;
  gsm.m_scrambled     = scrambled;

  return TRUE;
}


BOOL H323GSMPluginCapability::OnReceivedPDU(const H245_AudioCapability & cap, unsigned & packetSize)
{
  const H245_GSMAudioCapability & gsm = cap;
  packetSize   = gsm.m_audioUnitSize / encoderCodec->parm.audio.bytesPerFrame;
  if (packetSize == 0)
    packetSize = 1;

  scrambled    = gsm.m_scrambled;
  comfortNoise = gsm.m_comfortNoise;

  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

#endif   // OPAL_AUDIO

#if OPAL_VIDEO

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling most videoplugin capabilities
//

H323VideoPluginCapability::H323VideoPluginCapability(const PluginCodec_Definition * _encoderCodec,
                          const PluginCodec_Definition * _decoderCodec,
                          unsigned _pluginSubType)
  : H323VideoCapability(), H323PluginCapabilityInfo(_encoderCodec, _decoderCodec),
    pluginSubType(_pluginSubType)
{ 
}

PString H323VideoPluginCapability::GetFormatName() const
{ return H323PluginCapabilityInfo::GetFormatName();}

unsigned H323VideoPluginCapability::GetSubType() const
{ return pluginSubType; }

BOOL H323VideoPluginCapability::SetCommonOptions(OpalMediaFormat & mediaFormat, int frameWidth, int frameHeight, int frameRate)
{
  if (!mediaFormat.SetOptionInteger(OpalVideoFormat::FrameWidthOption(), frameWidth))
    return FALSE;

  if (!mediaFormat.SetOptionInteger(OpalVideoFormat::FrameHeightOption(), frameHeight))
    return FALSE;

  if (!mediaFormat.SetOptionInteger(OpalMediaFormat::FrameTimeOption(), OpalMediaFormat::VideoClockRate * 100 * frameRate / 2997))
    return FALSE;

  return TRUE;
}

void H323VideoPluginCapability::PrintOn(std::ostream & strm) const
{
  H323VideoCapability::PrintOn(strm);
}

/////////////////////////////////////////////////////////////////////////////

H323H261PluginCapability::H323H261PluginCapability(const PluginCodec_Definition * _encoderCodec,
                                                   const PluginCodec_Definition * _decoderCodec)
  : H323VideoPluginCapability(_encoderCodec, _decoderCodec, H245_VideoCapability::e_h261VideoCapability)
{
}

PObject::Comparison H323H261PluginCapability::Compare(const PObject & obj) const
{
  if (!PIsDescendant(&obj, H323H261PluginCapability))
    return LessThan;

  Comparison result = H323Capability::Compare(obj);
  if (result != EqualTo)
    return result;

  const H323H261PluginCapability & other = (const H323H261PluginCapability &)obj;

  const OpalMediaFormat & mediaFormat = GetMediaFormat();
  int qcifMPI = mediaFormat.GetOptionInteger(qcifMPI_tag);
  int  cifMPI = mediaFormat.GetOptionInteger(cifMPI_tag);

  const OpalMediaFormat & otherFormat = other.GetMediaFormat();
  int other_qcifMPI = otherFormat.GetOptionInteger(qcifMPI_tag);
  int other_cifMPI  = otherFormat.GetOptionInteger(cifMPI_tag);

  if (((qcifMPI > 0) && (other_qcifMPI > 0)) ||
      ((cifMPI  > 0) && (other_cifMPI > 0)))
    return EqualTo;

  if (qcifMPI > 0)
    return LessThan;

  return GreaterThan;
}

PObject * H323H261PluginCapability::Clone() const
{ 
  return new H323H261PluginCapability(*this); 
}

BOOL H323H261PluginCapability::OnSendingPDU(H245_VideoCapability & cap) const
{
  cap.SetTag(H245_VideoCapability::e_h261VideoCapability);

  H245_H261VideoCapability & h261 = cap;

  const OpalMediaFormat & mediaFormat = GetMediaFormat();

  /*
#if PTRACING
  ostream & traceStream = PTrace::Begin(4, __FILE__, __LINE__);
  traceStream << "H.261 extracting options from\n";
  for (PINDEX i = 0; i < mediaFormat.GetOptionCount(); i++) {
    const OpalMediaOption & option = mediaFormat.GetOption(i);
    traceStream << "         " << option.GetName() << " = " << option.AsString() << '\n';
  }
  traceStream << PTrace::End;
#endif
  */

  int qcifMPI = mediaFormat.GetOptionInteger(qcifMPI_tag, 0);
  if (qcifMPI > 0) {
    h261.IncludeOptionalField(H245_H261VideoCapability::e_qcifMPI);
    h261.m_qcifMPI = qcifMPI;
  }
  int cifMPI = mediaFormat.GetOptionInteger(cifMPI_tag);
  if (cifMPI > 0 || qcifMPI == 0) {
    h261.IncludeOptionalField(H245_H261VideoCapability::e_cifMPI);
    h261.m_cifMPI = cifMPI;
  }

  h261.m_temporalSpatialTradeOffCapability = mediaFormat.GetOptionBoolean(h323_temporalSpatialTradeOffCapability_tag, FALSE);
  h261.m_maxBitRate                        = (mediaFormat.GetOptionInteger(OpalMediaFormat::MaxBitRateOption(), 621700)+50)/100;
  h261.m_stillImageTransmission            = mediaFormat.GetOptionBoolean(h323_stillImageTransmission_tag, FALSE);

  return TRUE;
}


BOOL H323H261PluginCapability::OnSendingPDU(H245_VideoMode & pdu) const
{
  pdu.SetTag(H245_VideoMode::e_h261VideoMode);
  H245_H261VideoMode & mode = pdu;

  const OpalMediaFormat & mediaFormat = GetMediaFormat();

  int qcifMPI = mediaFormat.GetOptionInteger(qcifMPI_tag);

  mode.m_resolution.SetTag(qcifMPI > 0 ? H245_H261VideoMode_resolution::e_qcif
                                       : H245_H261VideoMode_resolution::e_cif);

  mode.m_bitRate                = (mediaFormat.GetOptionInteger(OpalMediaFormat::MaxBitRateOption(), 621700) + 50) / 1000;
  mode.m_stillImageTransmission = mediaFormat.GetOptionBoolean(h323_stillImageTransmission_tag, FALSE);

  return TRUE;
}

BOOL H323H261PluginCapability::OnReceivedPDU(const H245_VideoCapability & cap)
{
  if (cap.GetTag() != H245_VideoCapability::e_h261VideoCapability)
    return FALSE;

  OpalMediaFormat & mediaFormat = GetWritableMediaFormat();

  const H245_H261VideoCapability & h261 = cap;
  if (h261.HasOptionalField(H245_H261VideoCapability::e_qcifMPI)) {

    if (!mediaFormat.SetOptionInteger(qcifMPI_tag, h261.m_qcifMPI))
      return FALSE;

    if (!SetCommonOptions(mediaFormat, QCIF_WIDTH, QCIF_HEIGHT, h261.m_qcifMPI))
      return FALSE;
  }

  if (h261.HasOptionalField(H245_H261VideoCapability::e_cifMPI)) {

    if (!mediaFormat.SetOptionInteger(cifMPI_tag, h261.m_cifMPI))
      return FALSE;

    if (!SetCommonOptions(mediaFormat, QCIF_WIDTH, QCIF_HEIGHT, h261.m_cifMPI))
      return FALSE;
  }

  mediaFormat.SetOptionInteger(OpalMediaFormat::MaxBitRateOption(),        h261.m_maxBitRate*100);
  mediaFormat.SetOptionBoolean(h323_temporalSpatialTradeOffCapability_tag, h261.m_temporalSpatialTradeOffCapability);
  mediaFormat.SetOptionBoolean(h323_stillImageTransmission_tag,            h261.m_stillImageTransmission);

  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

H323H263PluginCapability::H323H263PluginCapability(const PluginCodec_Definition * _encoderCodec,
                                                   const PluginCodec_Definition * _decoderCodec)
  : H323VideoPluginCapability(_encoderCodec, _decoderCodec, H245_VideoCapability::e_h263VideoCapability)
{ 
}

PObject::Comparison H323H263PluginCapability::Compare(const PObject & obj) const
{
  if (!PIsDescendant(&obj, H323H263PluginCapability))
    return LessThan;

  Comparison result = H323Capability::Compare(obj);
  if (result != EqualTo)
    return result;

  const H323H263PluginCapability & other = (const H323H263PluginCapability &)obj;

  const OpalMediaFormat & mediaFormat = GetMediaFormat();

  int sqcifMPI = mediaFormat.GetOptionInteger(sqcifMPI_tag);
  int qcifMPI  = mediaFormat.GetOptionInteger(qcifMPI_tag);
  int cifMPI   = mediaFormat.GetOptionInteger(cifMPI_tag);
  int cif4MPI  = mediaFormat.GetOptionInteger(cif4MPI_tag);
  int cif16MPI = mediaFormat.GetOptionInteger(cif16MPI_tag);

  const OpalMediaFormat & otherFormat = other.GetMediaFormat();
  int other_sqcifMPI = otherFormat.GetOptionInteger(sqcifMPI_tag);
  int other_qcifMPI  = otherFormat.GetOptionInteger(qcifMPI_tag);
  int other_cifMPI   = otherFormat.GetOptionInteger(cifMPI_tag);
  int other_cif4MPI  = otherFormat.GetOptionInteger(cif4MPI_tag);
  int other_cif16MPI = otherFormat.GetOptionInteger(cif16MPI_tag);

  if ((sqcifMPI && other_sqcifMPI) ||
      (qcifMPI && other_qcifMPI) ||
      (cifMPI && other_cifMPI) ||
      (cif4MPI && other_cif4MPI) ||
      (cif16MPI && other_cif16MPI)) {
    PTRACE(5, "H.263\t" << *this << " == " << other);
    return EqualTo;
  }

  if ((!cif16MPI && other_cif16MPI) ||
      (!cif4MPI && other_cif4MPI) ||
      (!cifMPI && other_cifMPI) ||
      (!qcifMPI && other_qcifMPI) ||
      (!sqcifMPI && other_sqcifMPI)) {
    PTRACE(5, "H.263\t" << *this << " < " << other);
    return LessThan;
  }

  PTRACE(5, "H.263\t" << *this << " > " << other << " are equal");
  return GreaterThan;
}

PObject * H323H263PluginCapability::Clone() const
{ return new H323H263PluginCapability(*this); }

static void SetTransmittedCap(const OpalMediaFormat & mediaFormat,
                              H245_H263VideoCapability & h263,
                              const char * mpiTag,
                              int mpiEnum,
                              PASN_Integer & mpi,
                              int slowMpiEnum,
                              PASN_Integer & slowMpi)
{
  int mpiVal = mediaFormat.GetOptionInteger(mpiTag);
  if (mpiVal > 0) {
    h263.IncludeOptionalField(mpiEnum);
    mpi = mpiVal;
  }
  else if (mpiVal < 0) {
    h263.IncludeOptionalField(slowMpiEnum);
    slowMpi = -mpiVal;
  }
}


BOOL H323H263PluginCapability::OnSendingPDU(H245_VideoCapability & cap) const
{
  cap.SetTag(H245_VideoCapability::e_h263VideoCapability);
  H245_H263VideoCapability & h263 = cap;

  const OpalMediaFormat & mediaFormat = GetMediaFormat();

  //PStringStream str; mediaFormat.PrintOptions(str);
  //PTRACE(5, "OpalPlugin\tCreating capability for " << mediaFormat << ", options=\n" << str);

  SetTransmittedCap(mediaFormat, cap, sqcifMPI_tag, H245_H263VideoCapability::e_sqcifMPI, h263.m_sqcifMPI, H245_H263VideoCapability::e_slowSqcifMPI, h263.m_slowSqcifMPI);
  SetTransmittedCap(mediaFormat, cap, qcifMPI_tag,  H245_H263VideoCapability::e_qcifMPI,  h263.m_qcifMPI,  H245_H263VideoCapability::e_slowQcifMPI,  h263.m_slowQcifMPI);
  SetTransmittedCap(mediaFormat, cap, cifMPI_tag,   H245_H263VideoCapability::e_cifMPI,   h263.m_cifMPI,   H245_H263VideoCapability::e_slowCifMPI,   h263.m_slowCifMPI);
  SetTransmittedCap(mediaFormat, cap, cif4MPI_tag,  H245_H263VideoCapability::e_cif4MPI,  h263.m_cif4MPI,  H245_H263VideoCapability::e_slowCif4MPI,  h263.m_slowCif4MPI);
  SetTransmittedCap(mediaFormat, cap, cif16MPI_tag, H245_H263VideoCapability::e_cif16MPI, h263.m_cif16MPI, H245_H263VideoCapability::e_slowCif16MPI, h263.m_slowCif16MPI);

  h263.m_maxBitRate                        = (mediaFormat.GetOptionInteger(OpalMediaFormat::MaxBitRateOption(), 327600) + 50) / 100;
  h263.m_temporalSpatialTradeOffCapability = mediaFormat.GetOptionBoolean(h323_temporalSpatialTradeOffCapability_tag, FALSE);
  h263.m_unrestrictedVector	           = mediaFormat.GetOptionBoolean(h323_unrestrictedVector_tag, FALSE);
  h263.m_arithmeticCoding	           = mediaFormat.GetOptionBoolean(h323_arithmeticCoding_tag, FALSE);
  h263.m_advancedPrediction	           = mediaFormat.GetOptionBoolean(h323_advancedPrediction_tag, FALSE);
  h263.m_pbFrames	                   = mediaFormat.GetOptionBoolean(h323_pbFrames_tag, FALSE);
  h263.m_errorCompensation                 = mediaFormat.GetOptionBoolean(h323_errorCompensation_tag, FALSE);

  {
    int hrdB = mediaFormat.GetOptionInteger(h323_hrdB_tag, -1);
    if (hrdB >= 0) {
      h263.IncludeOptionalField(H245_H263VideoCapability::e_hrd_B);
	    h263.m_hrd_B = hrdB;
    }
  }

  {
    int bppMaxKb = mediaFormat.GetOptionInteger(h323_bppMaxKb_tag, -1);
    if (bppMaxKb >= 0) {
      h263.IncludeOptionalField(H245_H263VideoCapability::e_bppMaxKb);
	    h263.m_bppMaxKb = bppMaxKb;
    }
  }

  return TRUE;
}


BOOL H323H263PluginCapability::OnSendingPDU(H245_VideoMode & pdu) const
{
  pdu.SetTag(H245_VideoMode::e_h263VideoMode);
  H245_H263VideoMode & mode = pdu;

  const OpalMediaFormat & mediaFormat = GetMediaFormat();

  int qcifMPI  = mediaFormat.GetOptionInteger(qcifMPI_tag);
  int cifMPI   = mediaFormat.GetOptionInteger(cifMPI_tag);
  int cif4MPI  = mediaFormat.GetOptionInteger(cif4MPI_tag);
  int cif16MPI = mediaFormat.GetOptionInteger(cif16MPI_tag);

  mode.m_resolution.SetTag(cif16MPI ? H245_H263VideoMode_resolution::e_cif16
			  :(cif4MPI ? H245_H263VideoMode_resolution::e_cif4
			   :(cifMPI ? H245_H263VideoMode_resolution::e_cif
			    :(qcifMPI ? H245_H263VideoMode_resolution::e_qcif
            : H245_H263VideoMode_resolution::e_sqcif))));

  mode.m_bitRate              = (mediaFormat.GetOptionInteger(OpalMediaFormat::MaxBitRateOption(), 327600) + 50) / 100;
  mode.m_unrestrictedVector   = mediaFormat.GetOptionBoolean(h323_unrestrictedVector_tag, FALSE);
  mode.m_arithmeticCoding     = mediaFormat.GetOptionBoolean(h323_arithmeticCoding_tag, FALSE);
  mode.m_advancedPrediction   = mediaFormat.GetOptionBoolean(h323_advancedPrediction_tag, FALSE);
  mode.m_pbFrames             = mediaFormat.GetOptionBoolean(h323_pbFrames_tag, FALSE);
  mode.m_errorCompensation    = mediaFormat.GetOptionBoolean(h323_errorCompensation_tag, FALSE);

  return TRUE;
}

static BOOL SetReceivedH263Cap(OpalMediaFormat & mediaFormat, 
                               const H245_H263VideoCapability & h263, 
                               const char * mpiTag,
                               int mpiEnum,
                               const PASN_Integer & mpi,
                               int slowMpiEnum,
                               const PASN_Integer & slowMpi,
                               int frameWidth, int frameHeight,
                               BOOL & formatDefined)
{
  if (h263.HasOptionalField(mpiEnum)) {
    if (!mediaFormat.SetOptionInteger(mpiTag, mpi))
      return FALSE;
    if (mpi != 0) {
      if (!H323VideoPluginCapability::SetCommonOptions(mediaFormat, frameWidth, frameHeight, mpi))
        return FALSE;
      formatDefined = TRUE;
    }
  }
  else if (h263.HasOptionalField(slowMpiEnum)) {
    if (!mediaFormat.SetOptionInteger(mpiTag, -(signed)slowMpi))
      return FALSE;
    if (slowMpi != 0) {
      if (!H323VideoPluginCapability::SetCommonOptions(mediaFormat, frameWidth, frameHeight, -(signed)slowMpi))
        return FALSE;
      formatDefined = TRUE;
    }
  } 
  else
    mediaFormat.SetOptionInteger(mpiTag, 0);

  return TRUE;
}

BOOL H323H263PluginCapability::IsMatch(const PASN_Choice & subTypePDU) const
{
  if (!H323Capability::IsMatch(subTypePDU))
    return FALSE;

  H245_VideoCapability & video    = (H245_VideoCapability &)subTypePDU;
  H245_H263VideoCapability & h263 = (H245_H263VideoCapability &)video;

#define COMPARE_MPI(field, slowField) \
  if ( \
      ((GetMediaFormat().GetOptionInteger(field##_tag) > 0) ? TRUE : FALSE) \
      != \
      (h263.HasOptionalField(H245_H263VideoCapability::e_##field) ? (h263.m_##field != 0) : FALSE) \
      ) \
    return FALSE; \
  if ( \
      ((GetMediaFormat().GetOptionInteger(field##_tag) < 0) ? TRUE : FALSE) \
      != \
      (h263.HasOptionalField(H245_H263VideoCapability::e_##slowField) ? (h263.m_##slowField != 0) : FALSE) \
      ) \
    return FALSE; \
  if ( \
      ((GetMediaFormat().GetOptionInteger(field##_tag) < 0) ? TRUE : FALSE) \
      != \
      (h263.HasOptionalField(H245_H263VideoCapability::e_##slowField) ? (h263.m_##slowField != 0) : FALSE) \
      ) \
    return FALSE; \

  COMPARE_MPI(sqcifMPI,  slowSqcifMPI);
  COMPARE_MPI(qcifMPI,   slowQcifMPI);
  COMPARE_MPI(cifMPI,    slowCifMPI);
  COMPARE_MPI(cif4MPI,   slowCif4MPI);
  COMPARE_MPI(cif16MPI,  slowCif16MPI);


  return TRUE;
}


BOOL H323H263PluginCapability::OnReceivedPDU(const H245_VideoCapability & cap)
{
  if (cap.GetTag() != H245_VideoCapability::e_h263VideoCapability)
    return FALSE;

  OpalMediaFormat & mediaFormat = GetWritableMediaFormat();

  BOOL formatDefined = FALSE;

  const H245_H263VideoCapability & h263 = cap;

  if (!SetReceivedH263Cap(mediaFormat, cap, sqcifMPI_tag, H245_H263VideoCapability::e_sqcifMPI, h263.m_sqcifMPI, H245_H263VideoCapability::e_slowSqcifMPI, h263.m_slowSqcifMPI, SQCIF_WIDTH, SQCIF_HEIGHT, formatDefined))
    return FALSE;

  if (!SetReceivedH263Cap(mediaFormat, cap, qcifMPI_tag,  H245_H263VideoCapability::e_qcifMPI,  h263.m_qcifMPI,  H245_H263VideoCapability::e_slowQcifMPI,  h263.m_slowQcifMPI,  QCIF_WIDTH,  QCIF_HEIGHT,  formatDefined))
    return FALSE;

  if (!SetReceivedH263Cap(mediaFormat, cap, cifMPI_tag,   H245_H263VideoCapability::e_cifMPI,   h263.m_cifMPI,   H245_H263VideoCapability::e_slowCifMPI,   h263.m_slowCifMPI,   CIF_WIDTH,   CIF_HEIGHT,   formatDefined))
    return FALSE;

  if (!SetReceivedH263Cap(mediaFormat, cap, cif4MPI_tag,  H245_H263VideoCapability::e_cif4MPI,  h263.m_cif4MPI,  H245_H263VideoCapability::e_slowCif4MPI,  h263.m_slowCif4MPI,  CIF4_WIDTH,  CIF4_HEIGHT,  formatDefined))
    return FALSE;

  if (!SetReceivedH263Cap(mediaFormat, cap, cif16MPI_tag, H245_H263VideoCapability::e_cif16MPI, h263.m_cif16MPI, H245_H263VideoCapability::e_slowCif16MPI, h263.m_slowCif16MPI, CIF16_WIDTH, CIF16_HEIGHT, formatDefined))
    return FALSE;

  if (!formatDefined)
    return FALSE;

  unsigned maxBitRate = h263.m_maxBitRate*100;
  if (!mediaFormat.SetOptionInteger(OpalMediaFormat::MaxBitRateOption(), maxBitRate))
    return FALSE;
  unsigned targetBitRate = mediaFormat.GetOptionInteger(OpalVideoFormat::TargetBitRateOption());
  if (targetBitRate > maxBitRate)
    mediaFormat.SetOptionInteger(OpalVideoFormat::TargetBitRateOption(),     maxBitRate);

  mediaFormat.SetOptionBoolean(h323_unrestrictedVector_tag,                h263.m_unrestrictedVector);
  mediaFormat.SetOptionBoolean(h323_arithmeticCoding_tag,                  h263.m_arithmeticCoding);
  mediaFormat.SetOptionBoolean(h323_advancedPrediction_tag,                h263.m_advancedPrediction);
  mediaFormat.SetOptionBoolean(h323_pbFrames_tag,                          h263.m_pbFrames);
  mediaFormat.SetOptionBoolean(h323_errorCompensation_tag,                 h263.m_errorCompensation);
  mediaFormat.SetOptionBoolean(h323_temporalSpatialTradeOffCapability_tag, h263.m_temporalSpatialTradeOffCapability);

  if (h263.HasOptionalField(H245_H263VideoCapability::e_hrd_B))
    mediaFormat.SetOptionInteger(h323_hrdB_tag, h263.m_hrd_B);

  if (h263.HasOptionalField(H245_H263VideoCapability::e_bppMaxKb))
    mediaFormat.SetOptionInteger(h323_bppMaxKb_tag, h263.m_bppMaxKb);

//PStringStream str; mediaFormat.PrintOptions(str);
//PTRACE(4, "OpalPlugin\tCreated H.263 cap from incoming PDU with format " << mediaFormat << " and options\n" << str);

  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

H323CodecPluginNonStandardVideoCapability::H323CodecPluginNonStandardVideoCapability(const PluginCodec_Definition * _encoderCodec,
                                                                                     const PluginCodec_Definition * _decoderCodec,
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

H323CodecPluginNonStandardVideoCapability::H323CodecPluginNonStandardVideoCapability(const PluginCodec_Definition * _encoderCodec,
                                                                                     const PluginCodec_Definition * _decoderCodec,
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

PObject * H323CodecPluginNonStandardVideoCapability::Clone() const
{ return new H323CodecPluginNonStandardVideoCapability(*this); }

PString H323CodecPluginNonStandardVideoCapability::GetFormatName() const
{ return H323PluginCapabilityInfo::GetFormatName();}

/////////////////////////////////////////////////////////////////////////////

H323CodecPluginGenericVideoCapability::H323CodecPluginGenericVideoCapability(const PluginCodec_Definition * _encoderCodec,
                                                                             const PluginCodec_Definition * _decoderCodec,
                                                                             const PluginCodec_H323GenericCodecData *data)
  : H323GenericVideoCapability(data->standardIdentifier, data != NULL ? data->maxBitRate : 0),
    H323PluginCapabilityInfo((PluginCodec_Definition *)_encoderCodec, (PluginCodec_Definition *)_decoderCodec)
{
}

PObject * H323CodecPluginGenericVideoCapability::Clone() const
{
  return new H323CodecPluginGenericVideoCapability(*this);
}

PString H323CodecPluginGenericVideoCapability::GetFormatName() const
{
  return H323PluginCapabilityInfo::GetFormatName();
}


/////////////////////////////////////////////////////////////////////////////

#endif  // OPAL_VIDEO

#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////

static PAtomicInteger bootStrapCount = 0;

void OpalPluginCodecManager::Bootstrap()
{
  if (++bootStrapCount != 1)
    return;
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
static H323StaticPluginCodecFactory::Worker<H323StaticPluginCodec_##name > static##name##CodecFactory( #name ); \

#ifdef H323_EMBEDDED_GSM

INCLUDE_STATIC_CODEC(GSM_0610)

#endif

