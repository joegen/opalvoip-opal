/*p64encoder.h copyright (c)Indranet Technologies ltd (lara@indranet.co.nz)
 *                       Author Derek J Smithies (derek@indranet.co.nz)
 *
 *This file contains code which is the top level of
 *      a)video grabbing
 *      b)transformation into h261 packets.
 *
 * Questions may be sent to Derek J Smithies.
 *
 ****************************************************************/

/************ Change log
 *
 * $Log: p64encoder.cxx,v $
 * Revision 1.2001  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.10  2000/12/19 22:22:34  dereks
 * Remove connection to grabber-OS.cxx files. grabber-OS.cxx files no longer used.
 * Video data is now read from a video channel, using the pwlib classes.
 *
 * Revision 1.9  2000/10/13 01:47:26  dereks
 * Include command option for setting the number of transmitted video
 * frames per second.   use --videotxfps n
 *
 * Revision 1.8  2000/09/08 06:41:38  craigs
 * Added ability to set video device
 * Added ability to select test input frames
 *
 * Revision 1.7  2000/08/25 03:18:50  dereks
 * Add change log facility (Thanks Robert for the info on implementation)
 *
 *
 *
 ********/


#include "p64encoder.h"
P64Encoder::P64Encoder(int quant_level,int fillLevel)
{
  trans       = new Transmitter();
  h261_edr    = new H261PixelEncoder(trans);
  h261_edr->setq(quant_level);

  vid_frame   = new VideoFrame(WIDTH,HEIGHT);
  pre_vid     = new Pre_Vid_Coder();
  pre_vid->SetBackgroundFill(fillLevel);
}



P64Encoder::~P64Encoder(){
  delete pre_vid;  
  delete vid_frame;
  delete h261_edr; 
  delete trans;    
}


void P64Encoder::SetSize(int width,int height) {
  vid_frame->SetSize(width,height);
}

#if 0
void P64Encoder::GrabOneFrame() {
  /**Ensure we send out frames at a rate < 10 per/sercond.
     There are problems with the timer functions, but this provides
     some measure of control over the frame rate. 
     Suppose the grabber was called three times in a period of 9ms. We cannot
     assume that the system will prevent the grabber from returning the same
     frame twice. 
     Ideally, frame rate is controlled by PDUs received from the remote terminal.*/
     PTimeInterval EndTick= LastTick + frameSepn;
     PTimeInterval delay  = EndTick - PTimer::Tick();

     if(delay>1)
         PThread::Current()->Sleep(delay);
       else
	 EndTick = PTimer::Tick();
     LastTick = EndTick;

     if (useHardwareGrabber)
        video_grab->Grab(vid_frame);   
}
#endif

void P64Encoder::ProcessOneFrame(){
  pre_vid->ProcessFrame(vid_frame);
  h261_edr->consume(vid_frame);
} 


void P64Encoder::ReadOnePacket(
      u_char * buffer,    /// Buffer of encoded data
      unsigned & length /// Actual length of encoded data buffer
      )
{
 u_char * b_ptr;
 u_char * h_ptr;

 unsigned len_head,len_buff;

 trans->GetNextPacket(&h_ptr,&b_ptr, len_head,len_buff);
 length=len_head+len_buff;
 if(length!=0) {                          //Check to see if a packet was read.
    long int h261_hdr=*(long int *)h_ptr;   
    *(long int *)buffer= htonl(h261_hdr);
    memcpy(buffer+len_head,b_ptr,len_buff);
 }
}

u_char* P64Encoder::GetFramePtr()
{
           if(vid_frame)
                return vid_frame->frameptr;
           return NULL; 
}

/////////////////////////////////////////////////////////////////////////////
//VideoFrame

VideoFrame::VideoFrame(u_char *frame, u_char *cr, int newWidth, int newHeight)
{
  crvec = cr;
  frameptr = frame;
  width = newWidth;
  height = newHeight;  
  SetSize(newWidth,newHeight);
}

VideoFrame::VideoFrame(u_char *cr, int newWidth, int newHeight)
{
  crvec = cr;
  frameptr = NULL;
  SetSize(newWidth,newHeight);
}

VideoFrame::VideoFrame(int newWidth, int newHeight)
{
  crvec = NULL;
  frameptr = NULL;
  SetSize(newWidth,newHeight);
}


void VideoFrame::SetSize(int newWidth, int newHeight)
{
  if ((newWidth!=width)||(newHeight!=height)) {
    width = newWidth;
    height = newHeight;
    if (frameptr)
      delete frameptr;
    frameptr= new BYTE[(width*height*3)>>1];
  }
}

VideoFrame::~VideoFrame()
{
  if (frameptr) 
    delete frameptr;
}
/////////////////////////////////////////////////////////////////////////////
