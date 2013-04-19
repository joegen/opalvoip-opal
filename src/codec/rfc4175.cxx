/*
 * rfc4175.cxx
 *
 * RFC4175 transport for uncompressed video
 *
 * Open Phone Abstraction Library
 *
 * Copyright (C) 2007 Post Increment
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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "rfc4175.h"
#endif

#include <opal_config.h>

#if OPAL_RFC4175

#include <ptclib/random.h>
#include <opal/mediafmt.h>
#include <codec/rfc4175.h>
#include <codec/opalplugin.h>


#define   FRAME_WIDTH   1920
#define   FRAME_HEIGHT  1080
#define   FRAME_RATE    60

#define   REASONABLE_UDP_PACKET_SIZE  800


class RFC4175VideoFormatInternal : public OpalVideoFormatInternal
{
  PCLASSINFO(RFC4175VideoFormatInternal, OpalVideoFormatInternal);
  public:
    RFC4175VideoFormatInternal(
      const char * fullName,    ///<  Full name of media format
      const char * samplingName,
      unsigned int bandwidth
    );

    virtual PObject * Clone() const
    {
      PWaitAndSignal m(media_format_mutex);
      return new RFC4175VideoFormatInternal(*this);
    }

    virtual bool ToNormalisedOptions()
    { 
      int v;
      if ((v = GetOptionInteger(OpalVideoFormat::FrameWidthOption(), -1)) > 0) {
        SetOptionInteger(OpalVideoFormat::MinRxFrameWidthOption(), v);
        SetOptionInteger(OpalVideoFormat::MaxRxFrameWidthOption(), v);
      }
      if ((v = GetOptionInteger(OpalVideoFormat::FrameHeightOption(), -1)) > 0) {
        SetOptionInteger(OpalVideoFormat::MinRxFrameHeightOption(), v);
        SetOptionInteger(OpalVideoFormat::MaxRxFrameHeightOption(), v);
      }
      return true;
    }
};

const OpalVideoFormat & GetOpalRFC4175_YCbCr420()
{
  static OpalVideoFormat RFC4175_YCbCr420(new RFC4175VideoFormatInternal(OPAL_RFC4175_YCbCr420, "YCbCr-4:2:0", (FRAME_WIDTH*FRAME_HEIGHT*3/2)*FRAME_RATE));
  return RFC4175_YCbCr420;
}


const OpalVideoFormat & GetOpalRFC4175_RGB()
{
  static OpalVideoFormat RFC4175_RGB(new RFC4175VideoFormatInternal(OPAL_RFC4175_RGB, "RGB", FRAME_WIDTH*FRAME_HEIGHT*3*FRAME_RATE));
  return RFC4175_RGB;
}


/////////////////////////////////////////////////////////////////////////////

RFC4175VideoFormatInternal::RFC4175VideoFormatInternal(
      const char * fullName,    ///<  Full name of media format
      const char * samplingName,
      unsigned int bandwidth)
 : OpalVideoFormatInternal(fullName, 
                   RTP_DataFrame::DynamicBase,
                   "raw",
                   FRAME_WIDTH, FRAME_HEIGHT,
                   FRAME_RATE,
                   bandwidth, 0)
{
  OpalMediaOption * option;

#if OPAL_SIP
  // add mandatory fields
  option = FindOption(OpalVideoFormat::ClockRateOption());
  if (option != NULL)
    option->SetFMTPName("rate");

  option = FindOption(OpalVideoFormat::FrameWidthOption());
  if (option != NULL)
    option->SetFMTPName("width");

  option = FindOption(OpalVideoFormat::FrameHeightOption());
  if (option != NULL)
    option->SetFMTPName("height");
#endif // OPAL_SIP

  option = new OpalMediaOptionString("rfc4175_sampling", true, samplingName);
#if OPAL_SIP
  option->SetFMTPName("sampling");
#endif // OPAL_SIP
  AddOption(option, true);

  option = new OpalMediaOptionInteger("rfc4175_depth", true, OpalMediaOption::NoMerge, 8);
#if OPAL_SIP
  option->SetFMTPName("depth");
#endif // OPAL_SIP
  AddOption(option, true);

  option = new OpalMediaOptionString("rfc4175_colorimetry", true, "BT601-5");
#if OPAL_SIP
  option->SetFMTPName("colorimetry");
#endif // OPAL_SIP
  AddOption(option, true);
}

/////////////////////////////////////////////////////////////////////////////

OpalRFC4175Transcoder::OpalRFC4175Transcoder(      
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat  ///<  Output media format
)
 : OpalVideoTranscoder(inputMediaFormat, outputMediaFormat)
{
}

PINDEX OpalRFC4175Transcoder::RFC4175HeaderSize(PINDEX lines)
{ return 2 + lines*6; }

/////////////////////////////////////////////////////////////////////////////

OpalRFC4175Encoder::OpalRFC4175Encoder(      
  const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
  const OpalMediaFormat & outputMediaFormat  ///<  Output media format
) : OpalRFC4175Transcoder(inputMediaFormat, outputMediaFormat)
{
#ifdef _DEBUG
  m_extendedSequenceNumber = 0;
#else
  m_extendedSequenceNumber = PRandom::Number();
#endif

  m_maximumPacketSize      = REASONABLE_UDP_PACKET_SIZE;
}

void OpalRFC4175Encoder::StartEncoding(const RTP_DataFrame &)
{
}

bool OpalRFC4175Encoder::ConvertFrames(const RTP_DataFrame & input, RTP_DataFrameList & outputFrames)
{
  outputFrames.RemoveAll();

  PAssert(sizeof(ScanLineHeader) == 6, "ScanLineHeader is not packed");

  // make sure the incoming frame is big enough for a frame header
  if (input.GetPayloadSize() < (int)(sizeof(PluginCodec_Video_FrameHeader))) {
    PTRACE(1,"RFC4175\tPayload of grabbed frame too small for frame header");
    return false;
  }

  PluginCodec_Video_FrameHeader * header = (PluginCodec_Video_FrameHeader *)input.GetPayloadPtr();
  if (header->x != 0 && header->y != 0) {
    PTRACE(1,"RFC4175\tVideo grab of partial frame unsupported");
    return false;
  }

  // get information from frame header
  m_frameHeight       = header->height;
  m_frameWidth        = header->width;

  // make sure the incoming frame is big enough for the specified frame size
  if (input.GetPayloadSize() < (int)(sizeof(PluginCodec_Video_FrameHeader) + PixelsToBytes(m_frameWidth*m_frameHeight))) {
    PTRACE(1,"RFC4175\tPayload of grabbed frame too small for full frame");
    return false;
  }

  m_srcTimestamp = input.GetTimestamp();

  StartEncoding(input);

  // save pointer to output data
  m_dstFrames = &outputFrames;
  m_dstScanlineCounts.resize(0);

  // encode the full frame
  EncodeFullFrame();

  // grab the actual data
  EndEncoding();

  PTRACE(6,"RFC4175\tFrame encoded to " << outputFrames.GetSize() << " packets from seq = " << outputFrames[0].GetSequenceNumber());

  return true;
}

void OpalRFC4175Encoder::EncodeFullFrame()
{
  // encode the scan lines
  unsigned y;
  for (y = 0; y < m_frameHeight; y += GetRowsPerPgroup())
    EncodeScanLineSegment(y, 0, m_frameWidth);
}

void OpalRFC4175Encoder::EncodeScanLineSegment(PINDEX y, PINDEX offs, PINDEX width)
{
  // add new packets until scan line segment is finished
  PINDEX endX = offs + width;
  PINDEX x = offs;
  while (x < endX) {

    PINDEX roomLeft = m_maximumPacketSize - m_dstPacketSize;

    // if current frame cannot hold at least one pgroup, then add a new frame
    if ((m_dstFrames->GetSize() == 0) || (roomLeft < (PINDEX)(sizeof(ScanLineHeader) + GetPgroupSize()))) {
      AddNewDstFrame();
      continue;
    }

    PINDEX maxPGroupsInPacket = (roomLeft - (PINDEX)sizeof(ScanLineHeader)) / GetPgroupSize();
    PINDEX pGroupsInScanLine = (endX - x) / GetColsPerPgroup();

    // calculate how many pixels we can add
    PINDEX pixelsToAdd, octetsToAdd;
    if (maxPGroupsInPacket  <= pGroupsInScanLine) {
      octetsToAdd = maxPGroupsInPacket * GetPgroupSize(); 
      pixelsToAdd = maxPGroupsInPacket * GetColsPerPgroup();
    } else {
      octetsToAdd = pGroupsInScanLine * GetPgroupSize();
      pixelsToAdd = endX - x;
    }

    // populate the scan line table
    m_dstScanLineTable->m_length = (WORD)octetsToAdd;
    m_dstScanLineTable->m_y      = (WORD)y;
    m_dstScanLineTable->m_offset = (WORD)x | 0x8000;

    // adjust pointer to scan line table and number of scan lines
    ++m_dstScanLineTable;
    ++m_dstScanLineCount;

    // adjust packet size
    m_dstPacketSize += sizeof(ScanLineHeader) + octetsToAdd;

    // adjust X offset
    x += pixelsToAdd;
  }
}

void OpalRFC4175Encoder::AddNewDstFrame()
{
  // complete the previous output frame (if any)
  FinishOutputFrame();

  // allocate a new output frame
  RTP_DataFrame * frame = new RTP_DataFrame(m_maximumPacketSize - RTP_DataFrame::MinHeaderSize);
  m_dstFrames->Append(frame);

  // initialise payload size for maximum size
  frame->SetPayloadType(outputMediaFormat.GetPayloadType());
  // initialise current output scanline count;
  m_dstScanLineCount = 0;
  m_dstPacketSize    = frame->GetHeaderSize() + 2;
  m_dstScanLineTable = (ScanLineHeader *)(frame->GetPayloadPtr() + 2);
}

void OpalRFC4175Encoder::FinishOutputFrame()
{
  if ((m_dstFrames->GetSize() > 0) && (m_dstScanLineCount > 0)) {

    // populate the frame fields
    RTP_DataFrame & dst = m_dstFrames->back();

    // set the end of scan line table bit
    --m_dstScanLineTable;
    m_dstScanLineTable->m_offset = ((WORD)m_dstScanLineTable->m_offset) & 0x7FFF;

    // set the timestamp and payload type
    dst.SetTimestamp(m_srcTimestamp);
    dst.SetPayloadType(outputMediaFormat.GetPayloadType());

    // set and increment the sequence number
    dst.SetSequenceNumber((WORD)(m_extendedSequenceNumber & 0xffff));
    *(PUInt16b *)dst.GetPayloadPtr() = (WORD)((m_extendedSequenceNumber >> 16) & 0xffff);
    ++m_extendedSequenceNumber;

    // set actual payload size
    dst.SetPayloadSize(m_dstPacketSize - dst.GetHeaderSize());

    // save scanline count
    m_dstScanlineCounts.push_back(m_dstScanLineCount);
  }
}

/////////////////////////////////////////////////////////////////////////////

OpalRFC4175Decoder::OpalRFC4175Decoder(      
  const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
  const OpalMediaFormat & outputMediaFormat  ///<  Output media format
) : OpalRFC4175Transcoder(inputMediaFormat, outputMediaFormat)
{
  m_inputFrames.AllowDeleteObjects();
  m_first            = true;
  m_missingPackets   = false;
  m_maxWidth         = 0;
  m_maxHeight        = 0;
  m_frameWidth  = 0;
  m_frameHeight = 0;
}


OpalRFC4175Decoder::~OpalRFC4175Decoder()
{
}


bool OpalRFC4175Decoder::ConvertFrames(const RTP_DataFrame & input, RTP_DataFrameList & output)
{
  output.RemoveAll();

  PAssert(sizeof(ScanLineHeader) == 6, "ScanLineHeader is not packed");

  // do quick sanity check on packet
  if (input.GetPayloadSize() < 2) {
    PTRACE(1,"RFC4175\tInput frame too small for header");
    return false;
  }

  // get extended sequence number
  DWORD receivedSeqNo = input.GetSequenceNumber() | ((*(PUInt16b *)input.GetPayloadPtr()) << 16);
  DWORD  timestamp    = input.GetTimestamp();

  // special handling for first packet ever received
  if (m_first) {
    m_firstSequenceOfFrame = receivedSeqNo;
    m_timeStampOfFrame     = timestamp;
    m_first                = false;
    m_missingPackets       = false;
  } 
  else {
    // if timestamp changed, then we missed the marker bit
    // don't allow timestamp to go backwards
    // flush output and change to the new timestamp
    // and reset the sequence number to ignore packets for previous frames
    if (timestamp != m_timeStampOfFrame) {
      if (m_inputFrames.GetSize() > 0) {
        if ((m_timeStampOfFrame > timestamp) && ((m_timeStampOfFrame - timestamp) < 1024)) {
          PTRACE(2, "RFC4175\tIgnoring packet with earlier timestamp");
          return true;
        }
        PTRACE(2, "RFC4175\tDetected lost marker bit");
        m_missingPackets = true;
        DecodeFramesAndSetFrameSize(output);
      }
      m_firstSequenceOfFrame = receivedSeqNo;
      m_timeStampOfFrame     = timestamp;
    }

    else if (receivedSeqNo < m_nextSequenceNumber) {
      m_missingPackets = true;
      PTRACE(2, "RFC4175\tOut of order packet (got " << receivedSeqNo << " expecting " << m_nextSequenceNumber << ")");
    }

    else if (receivedSeqNo > m_nextSequenceNumber) {
      m_missingPackets = true;
      PTRACE(2, "RFC4175\tMissing " << (receivedSeqNo - m_nextSequenceNumber) << " packets");
    }
  }

  m_nextSequenceNumber = receivedSeqNo + 1;

  // make a pass through the scan line table and update the overall frame width and height
  PINDEX lineCount = 0;
  ScanLineHeader * scanLinePtr = (ScanLineHeader *)(input.GetPayloadPtr() + 2);

  bool lastLine = false;
  while (!lastLine && RFC4175HeaderSize(lineCount+1) < input.GetPayloadSize()) {

    // scan line length (in pgroups units)
    PINDEX lineLength = scanLinePtr->m_length / GetPgroupSize();

    // line number 
    WORD lineNumber = scanLinePtr->m_y & 0x7fff; 

    // pixel offset of scanline start
    WORD offset = scanLinePtr->m_offset;

    // detect if last scanline in table
    if (offset & 0x8000)
      offset &= 0x7fff;
    else
      lastLine = true;

    // update frame width and height seen so far
    PINDEX right = offset + lineLength * GetColsPerPgroup();
    if (right > m_maxWidth)
      m_maxWidth = right;
    PINDEX bottom = lineNumber + GetRowsPerPgroup();
    if (bottom > m_maxHeight)
      m_maxHeight = bottom;

    // count lines
    ++lineCount;

    // update scan line pointer
    ++scanLinePtr;
  }

  // add the frame to the input frame list, if OK
  m_inputFrames.Append(input.Clone());
  m_scanlineCounts.push_back(lineCount);

  // if marker set, decode the frames
  if (input.GetMarker()) 
    DecodeFramesAndSetFrameSize(output);

  return true;
}


void OpalRFC4175Decoder::DecodeFramesAndSetFrameSize(RTP_DataFrameList & output)
{
  // update frame width and height if the frame was complete or if it is the first frame we have seen
  if (!m_missingPackets || ((m_frameWidth == 0) && (m_frameHeight == 0))) {
    PTRACE(4, "RFC4175\tChanged received frame size from "
           << m_frameWidth << 'x' << m_frameHeight << " to " << m_maxWidth << 'x' << m_maxHeight);
    m_frameWidth  = m_maxWidth;
    m_frameHeight = m_maxHeight;
  }

  DecodeFrames(output);

  m_missingPackets   = false;
  m_maxWidth         = 0;
  m_maxHeight        = 0;
  m_inputFrames.RemoveAll();
  m_scanlineCounts.resize(0);
}

/////////////////////////////////////////////////////////////////////////////

void Opal_YUV420P_to_RFC4175YCbCr420::StartEncoding(const RTP_DataFrame & input)
{
  // save pointers to input data
  m_srcYPlane    = input.GetPayloadPtr() + sizeof(PluginCodec_Video_FrameHeader);
  m_srcCbPlane   = m_srcYPlane  + (m_frameWidth * m_frameHeight);
  m_srcCrPlane   = m_srcCbPlane + (m_frameWidth * m_frameHeight / 4);
}

void Opal_YUV420P_to_RFC4175YCbCr420::EndEncoding()
{
  FinishOutputFrame();

  PTRACE(6, "RFC4175\tEncoded YUV420P input frame to " << m_dstFrames->GetSize() << " RFC4175 output frames in YCbCr420 format");

  PINDEX f= 0;
  for (RTP_DataFrameList::iterator output = m_dstFrames->begin(); output != m_dstFrames->end(); ++output,++f) {
    ScanLineHeader * hdrs = (ScanLineHeader *)(output->GetPayloadPtr() + 2);
    register BYTE * scanLineDataPtr = output->GetPayloadPtr() + 2 + m_dstScanlineCounts[f] * sizeof(ScanLineHeader);
    for (PINDEX i = 0; i < m_dstScanlineCounts[f]; ++i) {
      ScanLineHeader & hdr = hdrs[i];

      PINDEX x     = hdr.m_offset & 0x7fff;
      PINDEX y     = hdr.m_y & 0x7fff;
      unsigned len = (hdr.m_length / GetPgroupSize()) * GetColsPerPgroup();

      register BYTE * yPlane0  = m_srcYPlane  + (m_frameWidth * y + x);
      register BYTE * yPlane1  = yPlane0    + m_frameWidth;
      register BYTE * cbPlane  = m_srcCbPlane + (m_frameWidth * y / 4) + x / 2;
      register BYTE * crPlane  = m_srcCrPlane + (m_frameWidth * y / 4) + x / 2;

      unsigned p;
      for (p = 0; p < len; p += 2) {
        *scanLineDataPtr++ = *yPlane0++;
        *scanLineDataPtr++ = *yPlane0++;
        *scanLineDataPtr++ = *yPlane1++;
        *scanLineDataPtr++ = *yPlane1++;
        *scanLineDataPtr++ = *cbPlane++;
        *scanLineDataPtr++ = *crPlane++;
      }
    }
  } 

  // set marker bit on last frame
  if (m_dstFrames->GetSize() != 0) {
    RTP_DataFrame & dst = m_dstFrames->back();
    dst.SetMarker(true);
  }
}

/////////////////////////////////////////////////////////////////////////////

bool Opal_RFC4175YCbCr420_to_YUV420P::DecodeFrames(RTP_DataFrameList & output)
{
  if (m_inputFrames.GetSize() == 0) {
    PTRACE(2, "RFC4175\tNo input frames to decode");
    return false;
  }

  PTRACE(6, "RFC4175\tDecoding output from " << m_inputFrames.GetSize() << " input frames");

  // allocate destination frame
  output.Append(new RTP_DataFrame(sizeof(PluginCodec_Video_FrameHeader) + PixelsToBytes(m_frameWidth*m_frameHeight)));
  RTP_DataFrame & outputFrame = output.back();
  outputFrame.SetMarker(true);
  outputFrame.SetPayloadType(outputMediaFormat.GetPayloadType());

  // get pointer to header and payload
  PluginCodec_Video_FrameHeader * hdr = (PluginCodec_Video_FrameHeader *)outputFrame.GetPayloadPtr();
  hdr->x = 0;
  hdr->y = 0;
  hdr->width  = m_frameWidth;
  hdr->height = m_frameHeight;

  BYTE * payload    = OPAL_VIDEO_FRAME_DATA_PTR(hdr);
  BYTE * dstYPlane  = payload;
  BYTE * dstCbPlane = dstYPlane  + (m_frameWidth * m_frameHeight);
  BYTE * dstCrPlane = dstCbPlane + (m_frameWidth * m_frameHeight / 4);

  // pass through all of the input frames, and extract information
  PINDEX f = 0;
  for (RTP_DataFrameList::iterator source = m_inputFrames.begin(); source != m_inputFrames.end(); ++source,++f) {
    // scan through table
    PINDEX l;
    ScanLineHeader * tablePtr = (ScanLineHeader *)(source->GetPayloadPtr() + 2);

    BYTE * yuvData = source->GetPayloadPtr() + 2 + m_scanlineCounts[f] * sizeof(ScanLineHeader);

    for (l = 0; l < m_scanlineCounts[f]; ++l) {

      // scan line length (in pixels units)
      PINDEX width = (tablePtr->m_length / GetPgroupSize()) * GetColsPerPgroup();

      // line number 
      WORD y = tablePtr->m_y & 0x7fff; 

      // pixel offset of scanline start
      WORD x = tablePtr->m_offset & 0x7fff;

      ++tablePtr;

      // only convert lines on even boundaries
      if (
          ((y & 1) == 0) 
          ) {

        BYTE * yPlane0 = dstYPlane  + y * m_frameWidth + x;
        BYTE * yPlane1 = yPlane0    + m_frameWidth;
        BYTE * cbPlane = dstCbPlane + (y * m_frameWidth / 4) + x / 2;
        BYTE * crPlane = dstCrPlane + (y * m_frameWidth / 4) + x / 2;

        PINDEX i;
        for (i = 0; i < width; i += 2) {
          *yPlane0++ = *yuvData++;
          *yPlane0++ = *yuvData++;
          *yPlane1++ = *yuvData++;
          *yPlane1++ = *yuvData++;
          *cbPlane++ = *yuvData++;
          *crPlane++ = *yuvData++;
        }
      }
    }
  }

  return true;
}

/////////////////////////////////////////////////////////////////////////////

void Opal_RGB24_to_RFC4175RGB::StartEncoding(const RTP_DataFrame & input)
{
  // save pointer to input data
  m_rgbBase = input.GetPayloadPtr() + sizeof(PluginCodec_Video_FrameHeader);
}

void Opal_RGB24_to_RFC4175RGB::EndEncoding()
{
 FinishOutputFrame();

  PTRACE(6, "RFC4175\tEncoded RGB24 input frame to " << m_dstFrames->GetSize() << " RFC4175 output frames in RGB format");

  BYTE* inPtr = m_rgbBase;

  PINDEX f = 0;
  for (RTP_DataFrameList::iterator output = m_dstFrames->begin(); output != m_dstFrames->end(); ++output,++f) {
    ScanLineHeader * hdrs = (ScanLineHeader *)(output->GetPayloadPtr() + 2);
    BYTE * scanLineDataPtr = output->GetPayloadPtr() + 2 + m_dstScanlineCounts[f] * sizeof (ScanLineHeader);
	
    for (PINDEX i = 0; i < m_dstScanlineCounts[f]; ++i) {
      ScanLineHeader & hdr = hdrs[i];

      memcpy(scanLineDataPtr, inPtr, (int)hdr.m_length); 
      scanLineDataPtr+= hdr.m_length;
      inPtr += hdr.m_length; 
    }
  } 

  // set marker bit on last frame
  if (m_dstFrames->GetSize() != 0) {
    RTP_DataFrame & dst = m_dstFrames->back();
    dst.SetMarker(TRUE);
  }
}

/////////////////////////////////////////////////////////////////////////////

bool Opal_RFC4175RGB_to_RGB24::DecodeFrames(RTP_DataFrameList & output)
{
  if (m_inputFrames.GetSize() == 0) {
    PTRACE(2, "RFC4175\tNo input frames to decode");
    return false;
  }

  PTRACE(6, "RFC4175\tDecoding output from " << m_inputFrames.GetSize() << " input frames");

  // allocate destination frame
  output.Append(new RTP_DataFrame(sizeof(PluginCodec_Video_FrameHeader) + PixelsToBytes(m_frameWidth*m_frameHeight)));
  RTP_DataFrame & outputFrame = output.back();
  outputFrame.SetMarker(true);

  // get pointer to header and payload
  PluginCodec_Video_FrameHeader * hdr = (PluginCodec_Video_FrameHeader *)outputFrame.GetPayloadPtr();
  hdr->x = 0;
  hdr->y = 0;
  hdr->width  = m_frameWidth;
  hdr->height = m_frameHeight;

  BYTE * rgbDest = OPAL_VIDEO_FRAME_DATA_PTR(hdr);

  // pass through all of the input frames, and extract information
  PINDEX f = 0;
  for (RTP_DataFrameList::iterator source = m_inputFrames.begin(); source != m_inputFrames.end(); ++source,++f) {
    // scan through table
    PINDEX l;
    ScanLineHeader * tablePtr = (ScanLineHeader *)(source->GetPayloadPtr() + 2);

    BYTE * rgbSource = source->GetPayloadPtr() + 2 + m_scanlineCounts[f] * sizeof(ScanLineHeader);

    for (l = 0; l < m_scanlineCounts[f]; ++l) {

      // scan line length
      PINDEX width = (tablePtr->m_length / GetPgroupSize()) * GetColsPerPgroup();

      // line number 
      WORD y = tablePtr->m_y & 0x7fff; 

      // pixel offset of scanline start
      WORD x = tablePtr->m_offset & 0x7fff;

      ++tablePtr;

      memcpy(rgbDest + (y * m_frameWidth + x) * 3, rgbSource, width * 3);

      rgbSource += width*3;
    }
  }

  return true;
}

#endif // OPAL_RFC4175


