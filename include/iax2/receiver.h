/*
 *
 *
 * Inter Asterisk Exchange 2
 * 
 * The entity which receives all packets for all calls.
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2005 Indranet Technologies Ltd.
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
 * The Initial Developer of the Original Code is Indranet Technologies Ltd.
 *
 * The author of this code is Derek J Smithies
 *
 *  $Log: receiver.h,v $
 *  Revision 1.1  2005/07/30 07:01:32  csoutheren
 *  Added implementation of IAX2 (Inter Asterisk Exchange 2) protocol
 *  Thanks to Derek Smithies of Indranet Technologies Ltd. for
 *  writing and contributing this code
 *
 *
 *
 *
 *
 */

#ifndef RECEIVER_H
#define RECEIVER_H

#include <ptlib.h>
#include <ptlib/sockets.h>

#ifdef P_USE_PRAGMA
#pragma interface
#endif

class IAX2EndPoint;
class Frame;
class FrameList;
class PacketIdList;

#include <iax2/frame.h>

/**Manage the reception of etherenet packets on the specified port.
   All received packets are handed to the appropriate connection.
   A separate thread is used to wait on the ethernet port*/
class Receiver : public PThread
{ 
  PCLASSINFO(Receiver, PThread);
 public:
  /**@name Constructors/destructors*/
  //@{
  /**Construct a receiver, given references to the endpoint and socket*/
  Receiver(IAX2EndPoint & _newEndpoint, PUDPSocket & _newSocket);
  
  /**Destroy the receiver*/
  ~Receiver();
  //@}
  
  /**@name general worker methods*/
  //@{
  /*The method which the receiver thread invokes*/
  virtual void Main();
  
  /**Sit in here, waiting for data on the socket*/
  BOOL ReadNetworkSocket();
  
  /**We have just read a frame from the network. This is a good
     MiniFrame, or good FullFrame. Put it on the queue of frames to be
     processed by the IAX2EndPoint.  The IAX2EndPoint will give this frame
     to the appropriate IAXConnection */
  void AddNewReceivedFrame(Frame *newFrame);
  //@}
 protected:
  /**Global variable which holds the application specific data */
  IAX2EndPoint &endpoint;
  
  /**Socket that is used to receive all network data from */
  PUDPSocket & sock;
  
  /**The act of processing a header will (inevitably) create  additional
     frames as trunked frames are split up */
  FrameList      fromNetworkFrames;
  
  /**Flag to indicate if this receiver thread should keep listening for network data */
  BOOL           keepGoing;
};

#endif // RECEIVER_H
/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 4 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-file-style:linux
 * c-basic-offset:2
 * End:
 */

