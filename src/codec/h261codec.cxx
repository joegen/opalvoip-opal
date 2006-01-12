/*
 * h261codec.cxx
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): Michele Piccini (michele@piccini.com)
 *                 Derek Smithies (derek@indranet.co.nz)
 *
 * $Log: h261codec.cxx,v $
 * Revision 1.2029  2006/01/12 17:54:01  dsandras
 * Added check for commandNotifier before calling it, prevents assert.
 *
 * Revision 2.27  2006/01/02 16:34:24  dsandras
 * Fixed compilation warning.
 *
 * Revision 2.26  2005/10/22 10:29:04  dsandras
 * Removed FIXME.
 *
 * Revision 2.25  2005/09/15 20:01:23  dsandras
 * Allow dynamic quality adjustments.
 *
 * Revision 2.24  2005/09/15 19:24:15  dsandras
 * Backported adaptative packet delay algorithm from OpenH323.
 *
 * Revision 2.23  2005/09/06 12:44:49  rjongbloed
 * Many fixes to finalise the video processing: merging remote media
 *
 * Revision 2.22  2005/09/04 06:23:39  rjongbloed
 * Added OpalMediaCommand mechanism (via PNotifier) for media streams
 *   and media transcoders to send commands back to remote.
 *
 * Revision 2.21  2005/08/31 13:19:25  rjongbloed
 * Added mechanism for controlling media (especially codecs) including
 *   changing the OpalMediaFormat option list (eg bit rate) and a completely
 *   new OpalMediaCommand abstraction for things like video fast update.
 *
 * Revision 2.20  2005/08/28 07:59:17  rjongbloed
 * Converted OpalTranscoder to use factory, requiring sme changes in making sure
 *   OpalMediaFormat instances are initialised before use.
 *
 * Revision 2.19  2005/08/24 10:15:10  rjongbloed
 * Fixed missing marker bit on codec output, get more than just the first frame!
 *
 * Revision 2.18  2005/08/23 12:46:27  rjongbloed
 * Fix some decoder issues, now get inital frame displayed!
 *
 * Revision 2.17  2005/08/20 09:01:56  rjongbloed
 * H.261 port to OPAL model
 *
 * Revision 2.16  2005/02/24 19:49:44  dsandras
 * Applied patch from Hannes Friederich to fix quality problem in some circumstances.
 *
 * Revision 2.15  2005/02/21 12:19:53  rjongbloed
 * Added new "options list" to the OpalMediaFormat class.
 *
 * Revision 2.14  2004/04/07 08:21:00  rjongbloed
 * Changes for new RTTI system.
 *
 * Revision 2.13  2004/03/11 06:54:28  csoutheren
 * Added ability to disable SIP or H.323 stacks
 *
 * Revision 2.12  2004/02/19 10:47:02  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.11  2004/01/18 15:35:20  rjongbloed
 * More work on video support
 *
 * Revision 2.10  2003/03/17 10:26:59  robertj
 * Added video support.
 *
 * Revision 2.9  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.8  2002/11/10 11:33:18  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.7  2002/09/04 06:01:47  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.6  2002/07/01 04:56:31  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.5  2002/01/22 05:18:59  robertj
 * Added RTP encoding name string to media format database.
 * Changed time units to clock rate in Hz.
 *
 * Revision 2.4  2002/01/14 06:35:57  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.3  2001/11/02 10:45:19  robertj
 * Updated to OpenH323 v1.7.3
 *
 * Revision 2.2  2001/10/05 00:22:13  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.1  2001/08/01 05:04:28  robertj
 * Changes to allow control of linking software transcoders, use macros
 *   to force linking.
 * Allowed codecs to be used without H.,323 being linked by using the
 *   new NO_H323 define.
 * Major changes to H.323 capabilities, uses OpalMediaFormat for base name.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.59  2004/01/02 00:31:42  dereksmithies
 * Fix test on presence/absence of video.
 *
 * Revision 1.58  2003/12/14 10:42:29  rjongbloed
 * Changes for compilability without video support.
 *
 * Revision 1.57  2003/10/02 23:37:56  dereksmithies
 * Add fix for fast update problem in h261 codec. Thanks to Martin André for doing the initial legwork.
 *
 * Revision 1.56  2003/04/03 23:54:15  robertj
 * Added fast update to H.261 codec, thanks Gustavo García Bernardo
 *
 * Revision 1.55  2003/03/20 23:45:46  dereks
 * Make formatting more consistant with the openh323.org standard.
 *
 * Revision 1.54  2003/03/17 08:05:02  robertj
 * Removed videoio classes in openh323 as now has versions in pwlib.
 *
 * Revision 1.53  2003/03/11 22:05:00  dereks
 * Video receive will not terminate in abnormal situations
 * Thanks to Damien Sandras.
 *
 * Revision 1.52  2002/12/29 22:35:34  dereks
 * Fix so video with Windows XP works. Thanks Damien Sandras.
 *
 * Revision 1.51  2002/12/24 07:38:49  robertj
 * Patches to fix divide by zero error, thanks Damien Sandras
 *
 * Revision 1.50  2002/12/16 09:11:19  robertj
 * Added new video bit rate control, thanks Walter H. Whitlock
 *
 * Revision 1.49  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.48  2002/05/19 22:03:45  dereks
 * Add fix from Sean Miceli, so correct P64Decoder is picked in calls with Netmeeting.
 * Many thanks, good work!
 *
 * Revision 1.47  2002/04/26 04:58:34  dereks
 * Add Walter Whitlock's fixes, based on Victor Ivashim's suggestions to improve
 * the quality with Netmeeting. Thanks guys!!!
 *
 * Revision 1.46  2002/04/05 00:53:19  dereks
 * Modify video frame encoding so that frame is encoded on an incremental basis.
 * Thanks to Walter Whitlock - good work.
 *
 * Revision 1.45  2002/01/09 06:07:13  robertj
 * Fixed setting of RTP timestamp values on video transmission.
 *
 * Revision 1.44  2002/01/09 00:21:39  robertj
 * Changes to support outgoing H.245 RequstModeChange.
 *
 * Revision 1.43  2002/01/08 01:30:41  robertj
 * Tidied up some PTRACE debug output.
 *
 * Revision 1.42  2002/01/03 23:05:50  dereks
 * Add methods to count number of H261 packets waiting to be sent.
 *
 * Revision 1.41  2001/12/20 01:19:43  dereks
 * modify ptrace statments.
 *
 * Revision 1.40  2001/12/13 03:01:23  dereks
 * Modify trace statement.
 *
 * Revision 1.39  2001/12/04 05:13:12  robertj
 * Added videa bandwidth limiting code for H.261, thanks Jose Luis Urien.
 *
 * Revision 1.38  2001/12/04 04:26:06  robertj
 * Added code to allow change of video quality in H.261, thanks Damian Sandras
 *
 * Revision 1.37  2001/10/23 02:17:16  dereks
 * Initial release of cu30 video codec.
 *
 * Revision 1.36  2001/09/26 01:59:31  robertj
 * Fixed MSVC warning.
 *
 * Revision 1.35  2001/09/25 03:14:47  dereks
 * Add constant bitrate control for the h261 video codec.
 * Thanks Tiziano Morganti for the code to set bit rate. Good work!
 *
 * Revision 1.34  2001/09/21 02:51:29  robertj
 * Added default session ID to media format description.
 *
 * Revision 1.33  2001/08/22 01:28:20  robertj
 * Resolved confusion with YUV411P and YUV420P video formats, thanks Mark Cooke.
 *
 * Revision 1.32  2001/07/09 07:19:40  rogerh
 * Make the encoder render a frames (for local video) only when there is a
 * renderer attached
 *
 * Revision 1.31  2001/06/19 02:01:42  dereks
 * The video encoder thread ends if the renderframe fails.
 *
 * Revision 1.30  2001/06/13 21:46:37  dereks
 * Add 5 msec separator between consecutive packets generated from the same
 * frame. Prevents lost packets, and improves reliability.
 *
 * Revision 1.29  2001/05/25 01:10:26  dereks
 * Remove unnecessary packet receive variable.
 * Alter the position of the check for change in frame size.
 *
 * Revision 1.28  2001/02/09 05:13:55  craigs
 * Added pragma implementation to (hopefully) reduce the executable image size
 * under Linux
 *
 * Revision 1.27  2001/01/25 07:27:16  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 * Revision 1.26  2000/12/19 22:33:44  dereks
 * Adjust so that the video channel is used for reading/writing raw video
 * data, which better modularizes the video codec.
 *
 * Revision 1.25  2000/10/13 02:20:32  robertj
 * Fixed capability clone so gets all fields including those in ancestor.
 *
 * Revision 1.24  2000/10/13 01:47:26  dereks
 * Include command option for setting the number of transmitted video
 * frames per second.   use --videotxfps n
 *
 * Revision 1.23  2000/09/08 06:41:38  craigs
 * Added ability to set video device
 * Added ability to select test input frames
 *
 * Revision 1.22  2000/08/28 23:47:41  dereks
 * Fix bug in resizing image of received video
 *
 * Revision 1.21  2000/08/21 04:45:06  dereks
 * Fix dangling pointer that caused segfaults for windows&unix users.
 * Improved the test image which is used when video grabber won't open.
 * Added code to handle setting of video Tx Quality.
 * Added code to set the number of background blocks sent with every frame.
 *
 * Revision 1.20  2000/07/13 12:31:31  robertj
 * Fixed format name output for in band switching H.261
 *
 * Revision 1.19  2000/07/04 13:00:36  craigs
 * Fixed problem with selecting large and small video sizes
 *
 * Revision 1.18  2000/06/10 09:21:36  rogerh
 * Make GetFormatName return H.261 QCIF or H.261 CIF
 *
 * Revision 1.17  2000/05/10 04:05:34  robertj
 * Changed capabilities so has a function to get name of codec, instead of relying on PrintOn.
 *
 * Revision 1.16  2000/05/02 04:32:26  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.15  2000/04/29 03:01:48  robertj
 * Fixed bug in receive of H261 capability, setting cif & qcif variables correctly.
 *
 * Revision 1.14  2000/03/24 01:23:49  robertj
 * Directory reorganisation.
 *
 * Revision 1.13  2000/03/21 03:06:49  robertj
 * Changes to make RTP TX of exact numbers of frames in some codecs.
 *
 * Revision 1.12  2000/02/10 03:08:02  craigs
 * Added ability to specify NTSC or PAL video format
 *
 * Revision 1.11  2000/02/04 05:11:19  craigs
 * Updated for new Makefiles and for new video transmission code
 *
 * Revision 1.10  2000/01/13 04:03:45  robertj
 * Added video transmission
 *
 * Revision 1.9  1999/12/23 23:02:35  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 * Revision 1.8  1999/11/29 08:20:47  craigs
 * Updated for new codec interface
 *
 * Revision 1.7  1999/11/01 00:52:00  robertj
 * Fixed various problems in video, especially ability to pass error return value.
 *
 * Revision 1.6  1999/10/08 09:59:03  robertj
 * Rewrite of capability for sending multiple audio frames
 *
 * Revision 1.5  1999/10/08 04:58:38  robertj
 * Added capability for sending multiple audio frames in single RTP packet
 *
 * Revision 1.4  1999/09/21 14:13:53  robertj
 * Windows MSVC compatibility.
 *
 * Revision 1.3  1999/09/21 08:10:03  craigs
 * Added support for video devices and H261 codec
 *
 * Revision 1.2  1999/09/18 13:24:38  craigs
 * Added ability to disable jitter buffer
 * Added ability to access entire RTP packet in codec Write
 *
 * Revision 1.1  1999/09/08 04:05:49  robertj
 * Added support for video capabilities & codec, still needs the actual codec itself!
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h261codec.h"
#endif

#include <codec/h261codec.h>

#ifndef NO_OPAL_VIDEO

#include <asn/h245.h>
#include "vic/p64.h"
#include "vic/p64encoder.h"


#define new PNEW


const OpalVideoFormat & GetOpalH261_QCIF()
{
  static const OpalVideoFormat H261_QCIF(OPAL_H261_QCIF, RTP_DataFrame::H261, "H261", 176, 144, 10, 128000);
  return H261_QCIF;
}

const OpalVideoFormat & GetOpalH261_CIF()
{
  static const OpalVideoFormat H261_CIF(OPAL_H261_CIF,  RTP_DataFrame::H261, "H261", 352, 288, 10, 128000);
  return H261_CIF;
}


///////////////////////////////////////////////////////////////////////////////

#ifndef NO_H323

H323_H261Capability::H323_H261Capability(unsigned _qcifMPI,
                                         unsigned _cifMPI,
                                         BOOL _temporalSpatialTradeOffCapability,
                                         BOOL _stillImageTransmission,
                                         unsigned _maxBitRate)
{
  qcifMPI = _qcifMPI;
  cifMPI = _cifMPI;
  temporalSpatialTradeOffCapability = _temporalSpatialTradeOffCapability;
  maxBitRate = _maxBitRate;
  stillImageTransmission = _stillImageTransmission;
}


PObject * H323_H261Capability::Clone() const
{
  return new H323_H261Capability(*this);
}


PObject::Comparison H323_H261Capability::Compare(const PObject & obj) const
{
  Comparison result = H323Capability::Compare(obj);
  if (result != EqualTo)
    return result;

  PAssert(PIsDescendant(&obj, H323_H261Capability), PInvalidCast);
  const H323_H261Capability & other = (const H323_H261Capability &)obj;

  if (((qcifMPI > 0) && (other.qcifMPI > 0)) ||
      ((cifMPI  > 0) && (other.cifMPI > 0)))
    return EqualTo;

  if (qcifMPI > 0)
    return LessThan;

  return GreaterThan;
}


unsigned H323_H261Capability::GetSubType() const
{
  return H245_VideoCapability::e_h261VideoCapability;
}


PString H323_H261Capability::GetFormatName() const
{
  return qcifMPI > 0 ? OpalH261_QCIF : OpalH261_CIF;
}


BOOL H323_H261Capability::OnSendingPDU(H245_VideoCapability & cap) const
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


BOOL H323_H261Capability::OnSendingPDU(H245_VideoMode & pdu) const
{
  pdu.SetTag(H245_VideoMode::e_h261VideoMode);
  H245_H261VideoMode & mode = pdu;
  mode.m_resolution.SetTag(cifMPI > 0 ? H245_H261VideoMode_resolution::e_cif
                                      : H245_H261VideoMode_resolution::e_qcif);
  mode.m_bitRate = maxBitRate;
  mode.m_stillImageTransmission = stillImageTransmission;
  return TRUE;
}


BOOL H323_H261Capability::OnReceivedPDU(const H245_VideoCapability & cap)
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

#endif


//////////////////////////////////////////////////////////////////////////////

Opal_H261_YUV420P::Opal_H261_YUV420P()
  : OpalVideoTranscoder(OpalH261_QCIF, OpalYUV420P)
{
  expectedSequenceNumber = 0;
  nblk = ndblk = 0;
  rvts = NULL;
  now = 1;
  packetReceived = FALSE;

  // Create the actual decoder
  videoDecoder = new FullP64Decoder();
  videoDecoder->marks(rvts);

  PTRACE(3, "Codec\tH261 decoder created");
}


Opal_H261_YUV420P::~Opal_H261_YUV420P()
{
  if (rvts)
    delete rvts;
  delete videoDecoder;
}


PINDEX Opal_H261_YUV420P::GetOptimalDataFrameSize(BOOL input) const
{
  return input ? RTP_DataFrame::MaxEthernetPayloadSize : ((frameWidth * frameHeight * 12) / 8);
}


BOOL Opal_H261_YUV420P::ConvertFrames(const RTP_DataFrame & src, RTP_DataFrameList & dst)
{
  PWaitAndSignal mutex(updateMutex);

  dst.RemoveAll();

  // Check for lost packets to help decoder
  BOOL lostPreviousPacket = FALSE;
  if (expectedSequenceNumber != 0 && expectedSequenceNumber != src.GetSequenceNumber()) {
    lostPreviousPacket = TRUE;
    PTRACE(3,"H261\tDetected loss of one video packet. "
          << expectedSequenceNumber << " != "
          << src.GetSequenceNumber() << " Will recover.");
  }
  expectedSequenceNumber = (WORD)(src.GetSequenceNumber()+1);

  videoDecoder->mark(now);
  if (!videoDecoder->decode(src.GetPayloadPtr(), src.GetPayloadSize(), lostPreviousPacket) && commandNotifier != PNotifier()) {
    OpalVideoUpdatePicture updatePicture;
    commandNotifier(updatePicture, 0); 
    PTRACE (3, "H261\t Could not decode frame, sending VideoUpdatePicture in hope of an I-Frame.");
    return TRUE;
  }

  //Check for a resize - can change at any time!
  if (frameWidth  != (unsigned)videoDecoder->width() ||
      frameHeight != (unsigned)videoDecoder->height()) {
    frameWidth = videoDecoder->width();
    frameHeight = videoDecoder->height();

    nblk = (frameWidth * frameHeight) / 64;
    delete [] rvts;
    rvts = new BYTE[nblk];
    memset(rvts, 0, nblk);
    videoDecoder->marks(rvts);
  }

  if (!src.GetMarker()) // Have not built an entire frame yet
    return TRUE;

  videoDecoder->sync();
  ndblk = videoDecoder->ndblk();

  int wraptime = now ^ 0x80;
  BYTE * ts = rvts;
  int k;
  for (k = nblk; --k >= 0; ++ts) {
    if (*ts == wraptime)
      *ts = (BYTE)now;
  }

  now = (now + 1) & 0xff;

  PINDEX frameBytes = (frameWidth * frameHeight * 12) / 8;
  RTP_DataFrame * pkt = new RTP_DataFrame(sizeof(FrameHeader)+frameBytes);
  pkt->SetMarker(true);
  FrameHeader * frameHeader = (FrameHeader *)pkt->GetPayloadPtr();
  frameHeader->x = frameHeader->y = 0;
  frameHeader->width = frameWidth;
  frameHeader->height = frameHeight;
  memcpy(frameHeader->data, videoDecoder->GetFramePtr(), frameBytes);

  dst.Append(pkt);

  videoDecoder->resetndblk();

  return TRUE;
}


//////////////////////////////////////////////////////////////////////////////

Opal_YUV420P_H261::Opal_YUV420P_H261()
  : OpalVideoTranscoder(OpalYUV420P, OpalH261_QCIF)
{
  maxOutputSize = (frameWidth * frameHeight * 12) / 8;

  videoEncoder = new P64Encoder(videoQuality, fillLevel);
  videoEncoder->SetSize(frameWidth, frameHeight);

  PTRACE(3, "Codec\tH261 encoder created");
}


Opal_YUV420P_H261::~Opal_YUV420P_H261()
{
  delete videoEncoder;
}


PINDEX Opal_YUV420P_H261::GetOptimalDataFrameSize(BOOL input) const
{
  return input ? ((frameWidth * frameHeight * 12) / 8) : maxOutputSize;
}


BOOL Opal_YUV420P_H261::ConvertFrames(const RTP_DataFrame & src, RTP_DataFrameList & dst)
{
  unsigned totalLength = 0;
  PWaitAndSignal mutex(updateMutex);

  dst.RemoveAll();

  if ((unsigned) src.GetPayloadSize() < sizeof(FrameHeader)) {
    PTRACE(1,"H261\t Video grab too small, Close down video transmission thread.");
    return FALSE;
  } 
 
  FrameHeader * header = (FrameHeader *)src.GetPayloadPtr();

  if (header->x != 0 && header->y != 0) {
    PTRACE(1,"H261\tVideo grab of partial frame unsupported, Close down video transmission thread.");
    return FALSE;
  }

  if (frameWidth != header->width || frameHeight != header->height) {
    frameWidth = header->width;
    frameHeight = header->height;
    videoEncoder->SetSize(frameWidth, frameHeight);
  }


  // "grab" the frame
  memcpy(videoEncoder->GetFramePtr(), header->data, frameWidth*frameHeight*12/8);

  if (updatePicture) {
    videoEncoder->FastUpdatePicture();
    updatePicture = false;
  }

  videoEncoder->PreProcessOneFrame();

  while (videoEncoder->MoreToIncEncode()) {
    unsigned length = 0;
    RTP_DataFrame * pkt = new RTP_DataFrame(2048);
    videoEncoder->IncEncodeAndGetPacket(pkt->GetPayloadPtr(), length); //get next packet on list
    pkt->SetPayloadSize(length);
    pkt->SetTimestamp(src.GetTimestamp());
    pkt->SetPayloadType(RTP_DataFrame::H261);
    dst.Append(pkt);
    totalLength += length;
  }

  dst[dst.GetSize()-1].SetMarker(TRUE);
  
  if (adaptivePacketDelay) {
    
    PTimeInterval waitBeforeSending;
    PTimeInterval currentTime;

    if (newTime != 0) { // calculate delay and wait
      currentTime = PTimer::Tick();
      waitBeforeSending = newTime - currentTime;
      if (waitBeforeSending > 0) 
	PThread::Current()->Sleep(waitBeforeSending);
      currentTime = PTimer::Tick(); //re-acquire current time after wait
    }
    currentTime = PTimer::Tick(); 
    if (targetBitRate/1000)
      newTime = currentTime + totalLength*8/(targetBitRate/1000);
    else
      newTime = currentTime + totalLength*8;
  }

  if (videoEncoder)
    videoEncoder->SetQualityLevel(videoQuality);
  
  return TRUE;
}


#endif // NO_OPAL_VIDEO


/////////////////////////////////////////////////////////////////////////////
