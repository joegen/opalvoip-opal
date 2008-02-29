/*
 * Inter Asterisk Exchange 2
 * 
 * The entity which receives all manages weirdo iax2 packets that are 
 * sent outside of a regular call.
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2006 Stephen Cook 
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
 * The Initial Developer of the Original Code is Indranet Technologies Ltd
 *
 * The author of this code is Stephen Cook
 *
 *  $Log: specialprocessor.cxx,v $
 *  Revision 1.4  2007/01/17 03:48:48  dereksmithies
 *  Tidy up comments, remove leaks, improve reporting of packet types.
 *
 *  Revision 1.3  2006/09/13 00:20:12  csoutheren
 *  Fixed warnings under VS.net
 *
 *  Revision 1.2  2006/09/11 03:12:51  dereksmithies
 *  Add logging and MPL license statements.
 *
 *
 */

#include <ptlib.h>
#include <typeinfo>

#ifdef P_USE_PRAGMA
#pragma implementation "specialprocessor.h"
#endif

#include <iax2/causecode.h>
#include <iax2/frame.h>
#include <iax2/iax2con.h>
#include <iax2/iax2ep.h>
#include <iax2/iax2medstrm.h>
#include <iax2/ies.h>
#include <iax2/processor.h>
#include <iax2/callprocessor.h>
#include <iax2/sound.h>
#include <iax2/transmit.h>

#define new PNEW
 
IAX2SpecialProcessor::IAX2SpecialProcessor(IAX2EndPoint & ep)
 : IAX2Processor(ep)
{
}

IAX2SpecialProcessor::~IAX2SpecialProcessor()
{
}
  
void IAX2SpecialProcessor::ProcessLists()
{
  while(ProcessOneIncomingEthernetFrame());
}

void IAX2SpecialProcessor::PrintOn(ostream & strm) const
{
  strm << "Special call processor";
}

void IAX2SpecialProcessor::OnNoResponseTimeout()
{
}
 
void IAX2SpecialProcessor::ProcessFullFrame(IAX2FullFrame & fullFrame)
{
  switch(fullFrame.GetFrameType()) {
  case IAX2FullFrame::iax2ProtocolType:        
    PTRACE(3, "Build matching full frame    fullFrameProtocol");
    ProcessNetworkFrame(new IAX2FullFrameProtocol(fullFrame));
    break;
    
  default:
    PTRACE(3, "Build matching full frame, Type not expected");
  }
}

void IAX2SpecialProcessor::ProcessNetworkFrame(IAX2MiniFrame * src)
{
  PTRACE(1, "unexpected Mini Frame");
  delete src;
}

BOOL IAX2SpecialProcessor::ProcessNetworkFrame(IAX2FullFrameProtocol * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameProtocol * src)");
  src->CopyDataFromIeListTo(ieData);
  
  //check if the common method can process it?
  if (IAX2Processor::ProcessNetworkFrame(src))
      return TRUE;
  
  switch (src->GetSubClass()) {
    case IAX2FullFrameProtocol::cmdPoke:
      ProcessIaxCmdPoke(src);
      break;
    default:
      PTRACE(1, "Process Full Frame Protocol, Type not expected");
      SendUnsupportedFrame(src);
      return FALSE;
  }
  
  return TRUE;
}

void IAX2SpecialProcessor::ProcessIaxCmdPoke(IAX2FullFrameProtocol *src)
{
  PTRACE(3, "ProcessIaxCmdPoke(IAX2FullFrameProtocol * src)");
  
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdPong,
    IAX2FullFrameProtocol::callIrrelevant);    
  TransmitFrameToRemoteEndpoint(f);
  delete src;
}
