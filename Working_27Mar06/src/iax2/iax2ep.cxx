/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Implementation of the IAX2 extensions to the OpalEndpoint class.
 * There is one instance of this class in the Opal environemnt.
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
 *
 *
 * $Log: iax2ep.cxx,v $
 * Revision 1.8  2005/11/15 19:45:20  dereksmithies
 * Add fix from  Vyacheslav Frolov, to resolve a private variable access problem.
 *
 * Revision 1.7  2005/09/05 01:19:43  dereksmithies
 * add patches from Adrian Sietsma to avoid multiple hangup packets at call end,
 * and stop the sending of ping/lagrq packets at call end. Many thanks.
 *
 * Revision 1.6  2005/08/26 03:07:38  dereksmithies
 * Change naming convention, so all class names contain the string "IAX2"
 *
 * Revision 1.5  2005/08/25 00:46:08  dereksmithies
 * Thanks to Adrian Sietsma for his code to better dissect the remote party name
 * Add  PTRACE statements, and more P_SSL_AES tests
 *
 * Revision 1.4  2005/08/24 01:38:38  dereksmithies
 * Add encryption, iax2 style. Numerous tidy ups. Use the label iax2, not iax
 *
 * Revision 1.3  2005/08/13 07:19:18  rjongbloed
 * Fixed MSVC6 compiler issues
 *
 * Revision 1.2  2005/08/04 08:14:17  rjongbloed
 * Fixed Windows build under DevStudio 2003 of IAX2 code
 *
 * Revision 1.1  2005/07/30 07:01:33  csoutheren
 * Added implementation of IAX2 (Inter Asterisk Exchange 2) protocol
 * Thanks to Derek Smithies of Indranet Technologies Ltd. for
 * writing and contributing this code
 *
 *
 *
 */
#include <ptlib.h>
#include <ptclib/random.h>

#ifdef P_USE_PRAGMA
#pragma implementation "iax2ep.h"
#endif

#include <iax2/iax2ep.h>
#include <iax2/receiver.h>
#include <iax2/transmit.h>


#define new PNEW


////////////////////////////////////////////////////////////////////////////////

IAX2EndPoint::IAX2EndPoint(OpalManager & mgr)
  : OpalEndPoint(mgr, "iax2", CanTerminateCall)
{
  
  localUserName = mgr.GetDefaultUserName();
  localNumber   = "1234";
  
  statusQueryCounter = 1;
  specialPacketHandler = new IAX2Processor(*this);
  specialPacketHandler->SetSpecialPackets(TRUE);
  specialPacketHandler->SetCallToken("Special packet handler");

  transmitter = NULL;
  receiver = NULL;
  sock = NULL;
  callsEstablished.SetValue(0);

  Initialise();
  PTRACE(5, "IAX2\tCreated endpoint.");
}

IAX2EndPoint::~IAX2EndPoint()
{
  PTRACE(3, "Endpoint\tIaxEndPoint destructor. Terminate the  transmitter, receiver, and incoming frame handler.");

  incomingFrameHandler.Terminate();
  incomingFrameHandler.WaitForTermination();
  packetsReadFromEthernet.AllowDeleteObjects();
  
  if (transmitter != NULL)
    delete transmitter;
  if (receiver != NULL)
    delete receiver;

  if (sock != NULL)
    delete sock;
  
  if (specialPacketHandler != NULL) {
    specialPacketHandler->Resume();
    specialPacketHandler->Terminate();
    specialPacketHandler->WaitForTermination();
    delete specialPacketHandler;
  }
  specialPacketHandler = NULL;

  PTRACE(3, "Endpoint\tDESTRUCTOR of IAX2 endpoint has Finished.");  
}

void IAX2EndPoint::ReportTransmitterLists()
{
  transmitter->ReportLists(); 
}


BOOL IAX2EndPoint::NewIncomingConnection(OpalTransport * /*transport*/)
{
  return TRUE;
}


void IAX2EndPoint::NewIncomingConnection(IAX2Frame *f)
{
  PTRACE(2, "IAX2\tWe have received a  NEW request from " << f->GetConnectionToken());
  // ask the endpoint for a connection

  if (connectionsActive.Contains(f->GetConnectionToken())) {
    PTRACE(3, "IAX2\thave received  a duplicate new packet from " << f->GetConnectionToken());
    cerr << " Haave received  a duplicate new packet from " << f->GetConnectionToken() << endl;
    delete f;
    return;
  }

  IAX2Connection *connection =
    CreateConnection(*GetManager().CreateCall(), f->GetConnectionToken(),
		     NULL, f->GetConnectionToken());
  if (connection == NULL) {
    PTRACE(2, "IAX2\tFailed to create IAX2Connection for NEW request from " << f->GetConnectionToken());
    delete f;
    return;
  }
  
  // add the connection to the endpoint list
  connectionsActive.SetAt(connection->GetToken(), connection);
  connection->OnIncomingConnection();

  connection->IncomingEthernetFrame(f);
}

void IAX2EndPoint::OnEstablished(OpalConnection & con)
{
  PTRACE(3, "Endpoint\tOnEstablished for " << con);

  OpalEndPoint::OnEstablished(con);
}

int IAX2EndPoint::NextSrcCallNumber()
{
  PWaitAndSignal m(callNumbLock);
  PINDEX callno = callnumbs++;
  if (callnumbs > 32766)
    callnumbs = 1;
  
  return callno;
}


BOOL IAX2EndPoint::ConnectionForFrameIsAlive(IAX2Frame *f)
{
  PString frameToken = f->GetConnectionToken();

  ReportStoredConnections();

  BOOL res = connectionsActive.Contains(frameToken);
  if (res) {
    return TRUE;
  }

  mutexTokenTable.Wait();
  PString tokenTranslated = tokenTable(frameToken);
  mutexTokenTable.Signal();

  if (tokenTranslated.IsEmpty()) {
    PTRACE(3, "No matching translation table entry token for \"" << frameToken << "\"");
    return FALSE;
  }

  res = connectionsActive.Contains(tokenTranslated);
  if (res) {
    PTRACE(5, "Found \"" << tokenTranslated << "\" in the connectionsActive table");
    return TRUE;
  }

  PTRACE(3, "ERR Could not find matching connection for \"" << tokenTranslated 
	 << "\" or \"" << frameToken << "\"");
  return FALSE;
}

void IAX2EndPoint::ReportStoredConnections()
{
  PStringList cons = GetAllConnections();
  PTRACE(3, " There are " << cons.GetSize() << " stored connections in connectionsActive");
  PINDEX i;
  for(i = 0; i < cons.GetSize(); i++)
    PTRACE(3, "    #" << (i + 1) << "                     \"" << cons[i] << "\"");

  PWaitAndSignal m(mutexTokenTable);
  PTRACE(3, " There are " << tokenTable.GetSize() << " stored connections in the token translation table.");
  for (i = 0; i < tokenTable.GetSize(); i++)
    PTRACE(3, " token table at " << i << " is " << tokenTable.GetKeyAt(i) << " " << tokenTable.GetDataAt(i));
}


PStringList IAX2EndPoint::DissectRemoteParty(const PString & other)
{
  PStringList res;
  for(int i = 0; i < maximumIndex; i++)
    res.AppendString(PString());

  res[protoIndex] = PString("iax2");
  res[transportIndex] = PString("UDP");

  PString working;                 
  if (other.Find("iax2:") != P_MAX_INDEX)  //Remove iax2:  from "other"
    working = other.Mid(5);
  else 
    working = other;

  PStringList halfs = working.Tokenise("@");
  if (halfs.GetSize() == 2) {
    res[userIndex] = halfs[0];
    working = halfs[1];
  } else
    working = halfs[0];

  if (working.IsEmpty())
    goto finishedDissection;

  halfs = working.Tokenise("$");
  if (halfs.GetSize() == 2) {
    res[transportIndex] = halfs[0];
    working = halfs[1];
  } else
    working = halfs[0];

  if (working.IsEmpty())
    goto finishedDissection;

  halfs = working.Tokenise("/");
  if (halfs.GetSize() == 2) {
    res[addressIndex] = halfs[0];
    working = halfs[1];
  } else {
    res[addressIndex] = halfs[0];
    goto finishedDissection;
  }


  halfs = working.Tokenise("+");
  if (halfs.GetSize() == 2) {
    res[extensionIndex] = halfs[0];
    res[contextIndex]   = halfs[1];
  } else
    res[extensionIndex] = halfs[0];

 finishedDissection:

  PTRACE(3, "Opal\t call protocol          " << res[protoIndex]);
  PTRACE(3, "Opal\t destination user       " << res[userIndex]);
  PTRACE(3, "Opal\t transport to use       " << res[transportIndex]);
  PTRACE(3, "Opal\t destination address    " << res[addressIndex]);
  PTRACE(3, "Opal\t destination extension  " << res[extensionIndex]);
  PTRACE(3, "Opal\t destination context    " << res[contextIndex]);

  return res;
}


BOOL IAX2EndPoint::MakeConnection(
				 OpalCall & call,
				 const PString & rParty, 
				 void * userData
				 )
{

  PTRACE(3, "IaxEp\tTry to make iax2 call to " << rParty);
  PTRACE(3, "IaxEp\tParty A=\"" << call.GetPartyA() << "\"  and party B=\"" <<  call.GetPartyB() << "\"");

  if (!call.GetPartyA().IsEmpty()) {
     PTRACE(3, "IaxEp\tWe are receving a call");
     return TRUE;
  }
  
  PStringList remoteInfo = DissectRemoteParty(rParty);
  if(remoteInfo[protoIndex] != PString("iax2"))
    return FALSE;

  PString remotePartyName = rParty.Mid(5);    

  PTRACE(3, "OpalMan\tNow do dns resolution of " << remoteInfo[addressIndex] << " for an iax2 call");
  PIPSocket::Address ip;
  if (!PIPSocket::GetHostAddress(remoteInfo[addressIndex], ip)) {
    PTRACE(3, "Could not make a iax2 call to " << remoteInfo[addressIndex] << " as IP resolution failed");
    return FALSE;
  }

  PStringStream callID;
  callID << "iax2:" <<  ip.AsString() << "OutgoingCall" << PString(++callsEstablished);
  IAX2Connection * connection = CreateConnection(call, callID, userData, remotePartyName);
  if (connection == NULL)
    return FALSE;
  connectionsActive.SetAt(connection->GetToken(), connection);

  connection->SetUpConnection();

  return TRUE;
}


IAX2Connection * IAX2EndPoint::CreateConnection(
      OpalCall & call,
      const PString & token,
      void * userData,
      const PString &remoteParty)
{
  return new IAX2Connection(call, *this, token, userData, remoteParty); 
}


OpalMediaFormatList IAX2EndPoint::GetMediaFormats() const
{
  return localMediaFormats;
}


BOOL IAX2EndPoint::Initialise()
{
  transmitter = NULL;
  receiver    = NULL;
  
  localMediaFormats = OpalMediaFormat::GetAllRegisteredMediaFormats();
  for (PINDEX i = localMediaFormats.GetSize(); i > 0; i--) {
    PStringStream strm;
    strm << localMediaFormats[i - 1];
    const PString formatName = strm;
    if (IAX2FullFrameVoice::OpalNameToIax2Value(formatName) == 0)
      localMediaFormats.RemoveAt(i - 1);
  }

  incomingFrameHandler.Assign(this);
  packetsReadFromEthernet.Initialise();

  PTRACE(6, "IAX2EndPoint\tInitialise()");
  PRandom rand;
  rand.SetSeed(PTime().GetTimeInSeconds() + 1);
  callnumbs = PRandom::Number() % 32000;
  
  sock = new PUDPSocket(ListenPortNumber());
  PTRACE(3, "IAX2EndPoint\tCreate Socket " << sock->GetPort());
  
  if (!sock->Listen(INADDR_ANY, 0, ListenPortNumber())) {
    PTRACE(3, "Receiver\tFailed to listen for incoming connections on " << ListenPortNumber());
    PTRACE(3, "Receiver\tFailed because the socket:::" << sock->GetErrorText());
    return FALSE;
  }
  
  PTRACE(6, "Receiver\tYES.. Ready for incoming connections on " << ListenPortNumber());
  
  transmitter = new IAX2Transmit(*this, *sock);
  receiver    = new IAX2Receiver(*this, *sock);
  
  return TRUE;
}


PINDEX IAX2EndPoint::GetOutSequenceNumberForStatusQuery()
{
  PWaitAndSignal m(statusQueryMutex);
  
  if (statusQueryCounter > 240)
    statusQueryCounter = 1;
  
  return statusQueryCounter++;
}


BOOL IAX2EndPoint::AddNewTranslationEntry(IAX2Frame *frame)
{   
  if (!frame->IsFullFrame()) {
    PTRACE(3, frame->GetConnectionToken() << " is Not a FullFrame, so dont add a translation entry(return now) ");
    return FALSE;
  }
  
  PINDEX destCallNo = frame->GetRemoteInfo().DestCallNumber();  /*Call number at our end */
  /* We do not know if the frame is encrypted, so examination of anything other than the 
     source call number/dest call number is unwise */ 

  PSafePtr<IAX2Connection> connection;
  for (connection = PSafePtrCast<OpalConnection, IAX2Connection>(connectionsActive.GetAt(0)); connection != NULL; ++connection) {
    PTRACE(3, "Compare " << connection->GetRemoteInfo().SourceCallNumber() << " and " <<  destCallNo);
    if (connection->GetRemoteInfo().SourceCallNumber() == destCallNo) {
      PTRACE(3, "Need to add translation for " << connection->GetCallToken() 
	     << " (" << frame->GetConnectionToken() 
	     << PString(") into token translation table"));
      PWaitAndSignal m(mutexTokenTable);
      tokenTable.SetAt(frame->GetConnectionToken(), connection->GetCallToken());
      return TRUE;
    }
  }

  return FALSE;
}

BOOL IAX2EndPoint::ProcessInMatchingConnection(IAX2Frame *f)
{
  ReportStoredConnections();

  PString tokenTranslated;
  mutexTokenTable.Wait();
  tokenTranslated = tokenTable(f->GetConnectionToken());
  mutexTokenTable.Signal();

  if (tokenTranslated.IsEmpty()) 
    tokenTranslated = f->GetConnectionToken();

  IAX2Connection *connection;
  connection = PSafePtrCast<OpalConnection, IAX2Connection>(connectionsActive.FindWithLock(tokenTranslated));
  if (connection != NULL) {
    connection->IncomingEthernetFrame(f);
    return TRUE;
  }
  
  PTRACE(3, "ERR Could not find matching connection for \"" << tokenTranslated 
	 << "\" or \"" << f->GetConnectionToken() << "\"");
  return FALSE;
}

//The receiving thread has finished reading a frame, and has droppped it here.
//At this stage, we do not know the frame type. We just know if it is full or mini.
void IAX2EndPoint::IncomingEthernetFrame(IAX2Frame *frame)
{
  PTRACE(3, "IAXEp\tEthernet Frame received from Receiver " << frame->IdString());   
  packetsReadFromEthernet.AddNewFrame(frame);
  incomingFrameHandler.ProcessList();
}

void IAX2EndPoint::ProcessReceivedEthernetFrames() 
{ 
  IAX2Frame *f;
  do {
    f = packetsReadFromEthernet.GetLastFrame();
    if (f == NULL) {
      continue;
    }

    PString idString = f->IdString();
    PTRACE(3, "Distrution\tNow try to find a home for " << idString);
    if (ProcessInMatchingConnection(f)) {
      continue;
    }

    if (AddNewTranslationEntry(f)) {
      if (ProcessInMatchingConnection(f)) {
	continue;
      }
    }

    /**These packets cannot be encrypted, as they are not going to a phone call */
    IAX2Frame *af = f->BuildAppropriateFrameType();
    if (af == NULL) 
      continue;
    delete f;
    f = af;

    if (specialPacketHandler->IsStatusQueryEthernetFrame(f)) {
      PTRACE(3, "Distribution\tthis frame is a  Status Query with no destination call" << idString);
      specialPacketHandler->IncomingEthernetFrame(f);
      continue;
    }
    
    if (!PIsDescendant(f, IAX2FullFrame)) {
      PTRACE(3, "Distribution\tNO matching connection for incoming ethernet frame Sorry" << idString);
      delete af;
      continue;
    }	  

    IAX2FullFrame *ff = (IAX2FullFrame *)f;
     if (ff->IsAckFrame()) {// snuck in here after termination. may be an ack for hangup ?
       PTRACE(3, "Distribution\t***** it's an ACK " << idString);
       /* purge will check for remote, call id, etc */
       transmitter->PurgeMatchingFullFrames(ff);
       delete ff;
       continue;
     }

    if (ff->GetFrameType() != IAX2FullFrame::iax2ProtocolType) {
      PTRACE(3, "Distribution\tNO matching connection for incoming ethernet frame Sorry" << idString);
      delete ff;
      continue;
    }	      
    
    if (ff->GetSubClass() != IAX2FullFrameProtocol::cmdNew) {
      PTRACE(3, "Distribution\tNO matching connection for incoming ethernet frame Sorry" << idString);
      delete ff;
      continue;
    }	      
        
    NewIncomingConnection(f);
    
  } while (f != NULL);  
}     


PINDEX IAX2EndPoint::GetPreferredCodec(OpalMediaFormatList & list)
{
  PTRACE(3, "preferred codecs are " << list);

  PStringArray codecNames;
  for (PINDEX i = 0; i < list.GetSize(); i++)
    codecNames += PString(list[i]);

  unsigned short val = 0;
  PINDEX index = 0;
  
  while ((index < codecNames.GetSize()) && (val == 0)) {
    val = IAX2FullFrameVoice::OpalNameToIax2Value(codecNames[index]);
    index++;
  }
  
  if (val == 0) {
    PTRACE(3, "Preferred codec is empty");
  } else {
    PTRACE(3, "EndPoint\tPreferred codec is  " << codecNames[index - 1]);
  }
  
  return val;
}

void IAX2EndPoint::GetCodecLengths(PINDEX codec, PINDEX &compressedBytes, PINDEX &duration)
{
  switch (codec) {
  case IAX2FullFrameVoice::g7231:     
    compressedBytes = 24;
    duration = 30;
    return;
  case IAX2FullFrameVoice::gsm:  
    compressedBytes = 33;
    duration = 20;
    return;
  case IAX2FullFrameVoice::g711ulaw: 
  case IAX2FullFrameVoice::g711alaw: 
    compressedBytes = 8;
    duration = 1;
    return;
  case IAX2FullFrameVoice::pcm:
    compressedBytes = 16;
    duration =  1;
  case IAX2FullFrameVoice::mp3: 
  case IAX2FullFrameVoice::adpcm:
  case IAX2FullFrameVoice::lpc10:
  case IAX2FullFrameVoice::g729: 
  case IAX2FullFrameVoice::speex:
  case IAX2FullFrameVoice::ilbc: 

  default: ;

  }

  PTRACE(1, "ERROR - could not find format " << IAX2FullFrameVoice::GetOpalNameOfCodec(codec) << " so use 20ms");
  duration = 20;
  compressedBytes = 33;
}  

PINDEX IAX2EndPoint::GetSupportedCodecs(OpalMediaFormatList & list)
{
  PTRACE(3, "Supported codecs are " << list);

  PINDEX i;
  PStringArray codecNames;
  for (i = 0; i < list.GetSize(); i++)
    codecNames += PString(list[i]);

  for(i = 0; i < codecNames.GetSize(); i++) {
    PTRACE(3, "Supported codec in opal is " << codecNames[i]);
  }
    
  PINDEX returnValue = 0;
  for (i = 0; i < codecNames.GetSize(); i++) 
    returnValue += IAX2FullFrameVoice::OpalNameToIax2Value(codecNames[i]);

  PTRACE(3, "Bitmask of codecs we support is 0x" 
	 << ::hex << returnValue << ::dec);
  
  return  returnValue;
}

void IAX2EndPoint::CopyLocalMediaFormats(OpalMediaFormatList & list)
{
  for(PINDEX i = 0; i < localMediaFormats.GetSize(); i++) {
    PStringStream strm;
    strm << localMediaFormats[i];
    PTRACE(3, "copy local format " << strm);
    list += *(new OpalMediaFormat(strm));
  }
}

void IAX2EndPoint::SetPassword(PString newValue)
{
  password = newValue; 
}

void IAX2EndPoint::SetLocalUserName(PString newValue)
{ 
  localUserName = newValue; 
}

void IAX2EndPoint::SetLocalNumber(PString newValue)
{ 
  localNumber = newValue; 
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
IAX2IncomingEthernetFrames::IAX2IncomingEthernetFrames() 
  : PThread(1000, NoAutoDeleteThread)
{
  keepGoing = TRUE;
}


void IAX2IncomingEthernetFrames::Assign(IAX2EndPoint *ep)
{
  endpoint = ep;
  Resume();
}

void IAX2IncomingEthernetFrames::Terminate()
{
  PTRACE(3, "Distribute\tEnd of thread - have received a terminate signal");
  keepGoing = FALSE;
  ProcessList();
}

void IAX2IncomingEthernetFrames::Main()
{
  SetThreadName("Distribute to Cons");
  while (keepGoing) {
    activate.Wait();
    PTRACE(3, "Distribute\tNow look for frames to send to connections");
    endpoint->ProcessReceivedEthernetFrames();
  }

  PTRACE(3, "Distribute\tEnd of thread - Do no more work now");
  return;
}

void IAX2IncomingEthernetFrames::ProcessList()
{
  activate.Signal(); 
}

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 2 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-file-style:linux
 * c-basic-offset:2
 * End:
 */



