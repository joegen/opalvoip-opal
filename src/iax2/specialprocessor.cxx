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

void IAX2SpecialProcessor::ProcessNetworkFrame(IAX2FullFrameProtocol * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameProtocol * src)");
  src->CopyDataFromIeListTo(ieData);
  
  //check if the common method can process it?
  if (ProcessCommonNetworkFrame(src))
    return;
  
  switch (src->GetSubClass()) {
    case IAX2FullFrameProtocol::cmdPoke:
      ProcessIaxCmdPoke(src);
      break;
    default:
      PTRACE(1, "Process Full Frame Protocol, Type not expected");
      SendUnsupportedFrame(src);
  }
  
  delete src;
}

void IAX2SpecialProcessor::ProcessIaxCmdPoke(IAX2FullFrameProtocol * src)
{
  PTRACE(3, "ProcessIaxCmdPoke(IAX2FullFrameProtocol * src)");
  
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdPong,
    IAX2FullFrameProtocol::callIrrelevant);    
  TransmitFrameToRemoteEndpoint(f);
}
