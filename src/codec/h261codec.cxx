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
 * Revision 1.2001  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
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

#include <asn/h245.h>
#include "vic/p64.h"
#include "vic/p64encoder.h"


#define new PNEW


#define H261_BASE_NAME  "H.261"

static OpalMediaFormat const H261_MediaFormat(H261_BASE_NAME,
                                              RTP_Session::DefaultVideoSessionID,
                                              RTP_DataFrame::H261,
                                              FALSE,  // No jitter for video
                                              240000, // bits/sec
                                              2000,   // Not sure of this value!
                                              0,      // No intrinsic time per frame
                                              OpalMediaFormat::VideoTimeUnits);


OPAL_REGISTER_TRANSCODER(Opal_H261_YUV411P, H261_BASE_NAME, "YUV411P");
OPAL_REGISTER_TRANSCODER(Opal_YUV411P_H261, "YUV411P", H261_BASE_NAME);


///////////////////////////////////////////////////////////////////////////////

H323_REGISTER_CAPABILITY_FUNCTION(H323_H261_QCIF_CIF, "H.261-(Q)CIF", H323_NO_EP_VAR)
{
  return new H323_H261Capability(2, 4, FALSE, FALSE, 6217);
}


H323_REGISTER_CAPABILITY_FUNCTION(H323_H261_QCIF, "H.261-QCIF", H323_NO_EP_VAR)
{
  return new H323_H261Capability(2, 0, FALSE, FALSE, 6217);
}


H323_REGISTER_CAPABILITY_FUNCTION(H323_H261_CIF, "H.261-CIF", H323_NO_EP_VAR)
{
  return new H323_H261Capability(0, 4, FALSE, FALSE, 6217);
}


///////////////////////////////////////////////////////////////////////////////

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

  PAssert(obj.IsDescendant(H323_H261Capability::Class()), PInvalidCast);
  const H323_H261Capability & other = (const H323_H261Capability &)obj;

  if (((qcifMPI > 0) && (other.qcifMPI > 0)) ||
      ((cifMPI  > 0) && (other.cifMPI > 0)))
    return EqualTo;

  if (qcifMPI > 0)
    return LessThan;

  return GreaterThan;
}


PString H323_H261Capability::GetFormatName() const
{
  if (qcifMPI > 0 && cifMPI > 0)
    return "H.261-(Q)CIF";

  if (qcifMPI > 0)
    return "H.261-QCIF";

  if (cifMPI > 0)
    return "H.261-CIF";

  return "H.261";
}


unsigned H323_H261Capability::GetSubType() const
{
  return H245_VideoCapability::e_h261VideoCapability;
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


BOOL H323_H261Capability::OnReceivedPDU(const H245_VideoCapability & cap)
{
  if (cap.GetTag() != H245_VideoCapability::e_h261VideoCapability)
    return FALSE;

  const H245_H261VideoCapability & h261 = cap;
  if (h261.HasOptionalField(H245_H261VideoCapability::e_qcifMPI))
    qcifMPI = h261.m_qcifMPI;
  else
    qcifMPI = 0;
  if (h261.HasOptionalField(H245_H261VideoCapability::e_cifMPI))
    cifMPI = h261.m_cifMPI;
  else
    cifMPI = 0;
  temporalSpatialTradeOffCapability = h261.m_temporalSpatialTradeOffCapability;
  maxBitRate = h261.m_maxBitRate;
  stillImageTransmission = h261.m_stillImageTransmission;
  return TRUE;
}


//////////////////////////////////////////////////////////////////////////////

Opal_H261_YUV411P::Opal_H261_YUV411P(const OpalTranscoderRegistration & registration)
  : OpalVideoTranscoder(registration)
{
  videoDecoder = NULL;
  nblk = ndblk = 0;
  rvts = NULL;
  now = 1;
  packetReceived = FALSE;

  PTRACE(3, "Codec\tH261 decoder created");
}


Opal_H261_YUV411P::~Opal_H261_YUV411P()
{
  if (rvts)
    delete rvts;
  delete videoDecoder;
}


BOOL Opal_H261_YUV411P::Convert(const RTP_DataFrame & src, RTP_DataFrame & dst)
{
  // assume buffer is start of H.261 header
  const BYTE * payload = src.GetPayloadPtr();
  PINDEX length = src.GetPayloadSize();

  const BYTE * header = payload;
  // see if any contributing source
  PINDEX cnt = src.GetContribSrcCount();
  if (cnt > 0) {
    header += cnt * 4;
    length -= cnt * 4;
  }

  // rh = RTP header -> header
  // bp = payload
  // cc = payload length

  // determine video codec type
  if (videoDecoder == NULL) {
    if ( (*(payload+1)) & 2)
      videoDecoder = new IntraP64Decoder();
    else
      videoDecoder = new FullP64Decoder();
    videoDecoder->marks(rvts);
  }

  // decode values from the RTP H261 header
  u_int v = ntohl(*(u_int*)header);
  int sbit  = (v >> 29) & 0x07;
  int ebit  = (v >> 26) & 0x07;
  int gob   = (v >> 20) & 0xf;
  int mba   = (v >> 15) & 0x1f;
  int quant = (v >> 10) & 0x1f;
  int mvdh  = (v >>  5) & 0x1f;
  int mvdv  =  v & 0x1f;


  //if (gob > 12) {
  //  h261_rtp_bug_ = 1;
  //  mba = (v >> 19) & 0x1f;
  //  gob = (v >> 15) & 0xf;
 // }

  if (gob > 12) {
    //count(STAT_BAD_HEADER);
    return FALSE;
  }

  // leave 4 bytes for H.261 header
  const BYTE * h261data = header + 4;
  length = length - 4;

  videoDecoder->mark(now);
  (void)videoDecoder->decode(h261data, length, sbit, ebit, mba, gob, quant, mvdh, mvdv);

  BOOL ok = TRUE;

  // If the incoming video stream changes size, resize the rendering device.
  if ((unsigned)videoDecoder->width() != frameWidth) {
    packetReceived = TRUE;
//    ok = Resize(videoDecoder->width(), videoDecoder->height());
    videoDecoder->marks(rvts);
  }

  if (ok && src.GetMarker()) {
    videoDecoder->sync();
    ndblk = videoDecoder->ndblk();
    if (!packetReceived) {
      packetReceived = TRUE;
//      ok = Resize(videoDecoder->width(), videoDecoder->height());
      videoDecoder->marks(rvts);
    } else {
      int wraptime = now ^ 0x80;
      BYTE * ts = rvts;
      int k;
      for (k = nblk; --k >= 0; ++ts) {
        if (*ts == wraptime)
          *ts = (BYTE)now;
      }

      ok = TRUE;
      if (packetReceived) {
        DWORD * yuvHeader = (DWORD *)dst.GetPayloadPtr();
        yuvHeader[0] = videoDecoder->width();
        yuvHeader[1] = videoDecoder->height();

        memcpy(&yuvHeader[2], videoDecoder->GetFramePtr(), 1);
      }

      now = (now + 1) & 0xff;
    }

    videoDecoder->resetndblk();
  }

  return ok;
}


//////////////////////////////////////////////////////////////////////////////

Opal_YUV411P_H261::Opal_YUV411P_H261(const OpalTranscoderRegistration & registration)
  : OpalVideoTranscoder(registration)
{
  videoEncoder = NULL;
  PTRACE(3, "Codec\tH261 encoder created");
}


Opal_YUV411P_H261::~Opal_YUV411P_H261()
{
  delete videoEncoder;
}


BOOL Opal_YUV411P_H261::Convert(const RTP_DataFrame & src, RTP_DataFrame & dst)
{
  DWORD * yuvHeader = (DWORD *)src.GetPayloadPtr();
  int frameWidth  = yuvHeader[0];
  int frameHeight = yuvHeader[1];

  if( frameWidth == 0) {
    PTRACE(3,"H261\t Video grab width is 0 x 0, Close down video transmission thread.");
    return FALSE;
  } 
 
  videoEncoder->SetSize(frameWidth, frameHeight); 

  PINDEX bytesInFrame = 0;
  if(!videoEncoder->PacketsOutStanding()) {
    //NO data is waiting to be read. Go and get some with the read call.
    memcpy(videoEncoder->GetFramePtr(), &yuvHeader[2], bytesInFrame);
    videoEncoder->ProcessOneFrame();        //Generate H261 packets.
  }
  
  unsigned length;
  videoEncoder->ReadOnePacket(dst.GetPayloadPtr(), length); //get next packet on list
  if (length != 0)                         
    dst.SetMarker(!videoEncoder->PacketsOutStanding()); //packet read, so process it.
  return TRUE;
}













#if 0






/**This class is a H.261 video codec.
 */
class H323_H261Codec : public H323VideoCodec
{
  PCLASSINFO(H323_H261Codec, H323VideoCodec)

  public:
    /**Create a new H261 video codec
     */
    H323_H261Codec(
      Direction direction,        /// Direction in which this instance runs
      BOOL isqCIF
    );
    ~H323_H261Codec();

    /**Encode the data from the appropriate device.
       This will encode a frame of data for transmission. The exact size and
       description of the data placed in the buffer is codec dependent but
       should be less than H323Capability::GetTxFramesInPacket() *
       OpalMediaFormat::GetFrameSize()  in length.

       The length parameter is filled with the actual length of the encoded
       data, often this will be the same as the size parameter.

       This function is called every GetFrameRate() timestamp units, so MUST
       take less than (or equal to) that amount of time to complete!

       Note that a returned length of zero indicates that time has passed but
       there is no data encoded. This is typically used for silence detection
       in an audio codec.
     */
    virtual BOOL Read(
      BYTE * buffer,            /// Buffer of encoded data
      unsigned & length,        /// Actual length of encoded data buffer
      RTP_DataFrame & rtpFrame  /// RTP data frame
    );

    /**Decode the data and output it to appropriate device.
       This will decode a single frame of received data. The exact size and
       description of the data required in the buffer is codec dependent but
       should be less than H323Capability::GetRxFramesInPacket() *
       OpalMediaFormat::GetFrameSize()  in length.

       It is expected this function anunciates the data. That is, for example
       with audio data, the sound is output on a speaker.

       This function is called every GetFrameRate() timestamp units, so MUST
       take less than that amount of time to complete!
     */
    virtual BOOL Write(
      const BYTE * buffer,        /// Buffer of encoded data
      unsigned length,            /// Length of encoded data buffer
      const RTP_DataFrame & rtp,  /// RTP data frame
      unsigned & written          /// Number of bytes used from data buffer
    );

    //There is a problem with the H261codec. It needs to be able to 
    //carry out two tasks. 1)Grab data from the camera.
    //2)Render data from an array.
    //Thus, we either: two PVideoChannels, or one PVideoChannel to both
    //grab and render.
    //We use one PVideoChannel, which is not consistant with elsewhere,
    //but enables us to (later) have a grab and display process irrespective
    //of there being a H323 connection.

  protected:
    BOOL Resize(int width, int height);
    BOOL Redraw();
    BOOL RenderFrame();

    P64Decoder       * videoDecoder;
    P64Encoder       * videoEncoder;
    int now;    
    BYTE * rvts;
    int ndblk, nblk;
    BOOL packetReceived;
};



H323_H261Codec::H323_H261Codec(Direction dir, BOOL isqCIF)
  : H323VideoCodec("H.261", dir)
{
  PTRACE(3, "Codec\tH261 " << (dir == Encoder ? "en" : "de") << "coder created");
  
  // no video decoder until we receive a packet
  videoDecoder = NULL;

  // no video encoder until we receive a packet
  videoEncoder = NULL;


  // other stuff
  now = 1;
  rvts = NULL;
  nblk = ndblk = 0;

  // initial size of the window is CIF
  if (dir == Encoder) {
    frameWidth  = isqCIF ? QCIF_WIDTH  : CIF_WIDTH;
    frameHeight = isqCIF ? QCIF_HEIGHT : CIF_HEIGHT;
  } else {
    frameWidth=0;
    frameHeight=0;
  }

}


H323_H261Codec::~H323_H261Codec()
{
  PWaitAndSignal mutex1(videoHandlerActive);

  PString label1 = ( (videoEncoder==NULL)? "" : "active encoder");
  PString label2 = ( (videoDecoder==NULL)? "" : "active DEcoder");

  if (videoDecoder)
  {
    delete videoDecoder;
    videoDecoder = NULL;    
  }
  
  if (videoEncoder){
    delete videoEncoder;
    videoEncoder = NULL;    
  }

  if (rvts){
    delete rvts;
  }

}


//This function grabs, displays, and compresses a video frame into
//into H261 packets.
//Get another frame if all packets of previous frame have been sent.
//Get next packet on list and send that one.
//Render the current frame if all of its packets have been sent.
BOOL H323_H261Codec::Read(BYTE * buffer,
                          unsigned & length,
                          RTP_DataFrame & frame)
{
  PWaitAndSignal mutex1(videoHandlerActive);  
  PTRACE(6,"Read\t Acquire next packet from h261 encoder.\n");

  if (videoEncoder == NULL)
      videoEncoder = new P64Encoder(qualityLevel, fillLevel);

  if( rawDataChannel == NULL ) {
    length = 0;
     PTRACE(3,"H261\t No channel to connect to video grabber.");
     PTRACE(3,"H261\t Close down video transmission thread.");
    return FALSE;
  }
  
  if( !rawDataChannel->IsOpen()) {
     PTRACE(3,"H261\t Video grabber is not initialised.");
     PTRACE(3,"H261\t Close down video transmission thread.");
     length = 0;
     return FALSE;
  }

  frameWidth  = ((PVideoChannel *)rawDataChannel)->GetGrabWidth();
  frameHeight = ((PVideoChannel *)rawDataChannel)->GetGrabHeight();

    if( frameWidth == 0) {
    PTRACE(3,"H261\t Video grab width is 0 x 0.");
    PTRACE(3,"H261\t Close down video transmission thread.");
    length=0;
    return FALSE;
  } 
 
  videoEncoder->SetSize(frameWidth, frameHeight); 

  PINDEX bytesInFrame = 0;
  BOOL ok=TRUE;
  if(!videoEncoder->PacketsOutStanding()) {
  

  //NO data is waiting to be read. Go and get some with the read call.
    if(rawDataChannel->Read(videoEncoder->GetFramePtr(),bytesInFrame)) {
      
      // If there is a Renderer attached, display the grabbed video.
      if(((PVideoChannel *)rawDataChannel)->IsRenderOpen()) {
        ok = RenderFrame();                   //use data from grab process.
      }

      videoEncoder->ProcessOneFrame();        //Generate H261 packets.
    } else {
      PTRACE(3,"H261\t Failed to read data from video grabber..");
      PTRACE(3,"H261\t Close down video transmission thread.");      
      return FALSE;   //Read failed, return false.
    }
  } else
    PThread::Current()->Sleep(5);  // Place a 5 ms interval betwen 
                                   // packets of the same frame.

 
  videoEncoder->ReadOnePacket(buffer,length); //get next packet on list
  if( length != 0 ) {                         
      if(videoEncoder->PacketsOutStanding()) //packet read, so process it.
         frame.SetMarker(FALSE);                
       else
	 frame.SetMarker(TRUE);
  } 
  return ok;
}


BOOL H323_H261Codec::Write(const BYTE * buffer,
                           unsigned length,
                           const RTP_DataFrame & frame,
                           unsigned & written)
{
  PWaitAndSignal mutex1(videoHandlerActive);  

  if( rawDataChannel == NULL ) {//Some other task has killed our videohandler. Exit.
    return FALSE;
  }

  // always indicate we have written the entire packet
  written = length;

  // assume buffer is start of H.261 header
  const BYTE * header  = buffer;

  // see if any contributing source
  PINDEX cnt = frame.GetContribSrcCount();
  if (cnt > 0) {
    header += cnt * 4;
    length -= cnt * 4;
  }

  // rh = RTP header -> header
  // bp = payload
  // cc = payload length

  // determine video codec type
  if (videoDecoder == NULL) {
    if ( (*(buffer+1)) & 2)
      videoDecoder = new IntraP64Decoder();
    else
      videoDecoder = new FullP64Decoder();
    videoDecoder->marks(rvts);
  }

  // decode values from the RTP H261 header
  u_int v = ntohl(*(u_int*)header);
  int sbit  = (v >> 29) & 0x07;
  int ebit  = (v >> 26) & 0x07;
  int gob   = (v >> 20) & 0xf;
  int mba   = (v >> 15) & 0x1f;
  int quant = (v >> 10) & 0x1f;
  int mvdh  = (v >>  5) & 0x1f;
  int mvdv  =  v & 0x1f;


  //if (gob > 12) {
  //  h261_rtp_bug_ = 1;
  //  mba = (v >> 19) & 0x1f;
  //  gob = (v >> 15) & 0xf;
 // }

  if (gob > 12) {
    //count(STAT_BAD_HEADER);
    return FALSE;
  }

  // leave 4 bytes for H.261 header
  const BYTE * payload = header + 4;
  length = length - 4;

  videoDecoder->mark(now);
  (void)videoDecoder->decode(payload, length, sbit, ebit, mba, gob, quant, mvdh, mvdv);

  BOOL ok = TRUE;

  // If the incoming video stream changes size, resize the rendering device.
  ok = Resize(videoDecoder->width(), videoDecoder->height());

  if (ok && frame.GetMarker()) {
    videoDecoder->sync();
    ndblk = videoDecoder->ndblk();
    ok = RenderFrame();       
    videoDecoder->resetndblk();
  }

  return ok;
}

/* Resize is relevant to the decoder only, as the encoder does not
   change size mid transmission.
*/
BOOL H323_H261Codec::Resize(int _width, int _height)
{
  //Check for a resize is carried out one two level -.
  // a) size change in the receive video stream.

  if( (frameWidth!=_width) || (frameHeight!=_height) ) {
      frameWidth  = _width;
      frameHeight = _height;

      nblk = (frameWidth * frameHeight) / 64;
      delete rvts;
      rvts = new BYTE[nblk];
      memset(rvts, 0, nblk);
      if(videoDecoder!=NULL)
	videoDecoder->marks(rvts);
  }
 
  return TRUE;
}


BOOL H323_H261Codec::Redraw()
{
  now = 1;
  memset(rvts, 1, nblk);

  return RenderFrame();
}

/* RenderFrame does three things.
   a) Set internal variables
   b) Set size of the display frame. This call happens with every frame.
         A very small overhead.
   c) Display a frame.
*/
BOOL H323_H261Codec::RenderFrame()
{
  int wraptime = now ^ 0x80;
  BYTE * ts = rvts;
  int k;
  for (k = nblk; --k >= 0; ++ts) {
    if (*ts == wraptime)
      *ts = (BYTE)now;
  }

  BOOL ok = TRUE;
  if (rawDataChannel != NULL) {

    //Now display local image.
    ((PVideoChannel *)rawDataChannel)->SetRenderFrameSize(frameWidth, frameHeight);
    PTRACE(6, "Size of video rendering frame set to " << frameWidth << "x" << frameHeight);

    if (direction == Encoder)
        ok = rawDataChannel->Write((const void *)videoEncoder->GetFramePtr(),0);
      else 
        ok = rawDataChannel->Write((const void *)videoDecoder->GetFramePtr(),0);    
  }

  now = (now + 1) & 0xff;

  return ok;
}
#endif

/////////////////////////////////////////////////////////////////////////////
