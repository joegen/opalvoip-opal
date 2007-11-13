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
 * Revision 1.36  2007/08/13 04:24:41  csoutheren
 * Normalise IAX2 answer logic
 *
 * Revision 1.35  2007/08/07 03:10:18  dereksmithies
 * Modify comments. Reduce some of the verbosity of the PTRACE messages.
 *
 * Revision 1.34  2007/08/03 02:31:44  rjongbloed
 * Duelling patches! Removed previous code.
 *
 * Revision 1.33  2007/08/03 01:43:15  rjongbloed
 * Added missing function implementation to IAX2 code so DLL can link.
 *
 * Revision 1.32  2007/08/02 23:25:07  dereksmithies
 * Rework iax2 handling of incoming calls. This should ensure that woomera/simpleopal etc
 * will correctly advise on receiving an incoming call.
 *
 * Revision 1.31  2007/04/22 22:39:42  dereksmithies
 * Lower verbosity of PTRACE statements.
 * Get G711 to work. NOTE - iax2 requires no silence suppression.
 *
 * Revision 1.30  2007/04/03 07:40:24  rjongbloed
 * Fixed CreateCall usage so correct function (with userData) is called on
 *   incoming connections.
 *
 * Revision 1.29  2007/03/29 23:55:46  rjongbloed
 * Tidied some code when a new connection is created by an endpoint. Now
 *   if someone needs to derive a connection class they can create it without
 *   needing to remember to do any more than the new.
 *
 * Revision 1.28  2007/03/13 00:33:10  csoutheren
 * Simple but messy changes to allow compile time removal of protocol
 * options such as H.450 and H.460
 * Fix MakeConnection overrides
 *
 * Revision 1.27  2007/03/01 05:51:04  rjongbloed
 * Fixed backward compatibility of OnIncomingConnection() virtual
 *   functions on various classes. If an old override returned FALSE
 *   then it will now abort the call as it used to.
 *
 * Revision 1.26  2007/01/24 04:00:57  csoutheren
 * Arrrghh. Changing OnIncomingConnection turned out to have a lot of side-effects
 * Added some pure viritual functions to prevent old code from breaking silently
 * New OpalEndpoint and OpalConnection descendants will need to re-implement
 * OnIncomingConnection. Sorry :)
 *
 * Revision 1.25  2007/01/23 02:10:38  dereksmithies
 *  Handle Vnak frames correctly.  Handle iseqno and oseqno correctly.
 *
 * Revision 1.24  2007/01/17 03:48:48  dereksmithies
 * Tidy up comments, remove leaks, improve reporting of packet types.
 *
 * Revision 1.23  2007/01/13 00:04:57  rjongbloed
 * Fixed compilation on DevStudio 2003
 *
 * Revision 1.22  2007/01/12 02:48:11  dereksmithies
 * Make the iax2callprocessor a more permanent variable in the iax2connection.
 *
 * Revision 1.21  2007/01/12 02:39:00  dereksmithies
 * Remove the notion of srcProcessors and dstProcessor lists from the ep.
 * Ensure that the connection looks after the callProcessor.
 *
 * Revision 1.20  2007/01/11 03:02:15  dereksmithies
 * Remove the previous audio buffering code, and switch to using the jitter
 * buffer provided in Opal. Reduce the verbosity of the log mesasges.
 *
 * Revision 1.19  2007/01/09 03:32:23  dereksmithies
 * Tidy up and improve the close down process - make it more robust.
 * Alter level of several PTRACE statements. Add Terminate() method to transmitter and receiver.
 *
 * Revision 1.18  2007/01/08 04:06:58  dereksmithies
 * Modify logging level, and improve the comments.
 *
 * Revision 1.17  2007/01/08 01:26:22  dereksmithies
 * Get it to make an iax2 call correctly.
 * Addjust some PTRACE levels.
 *
 * Revision 1.16  2006/12/18 03:18:42  csoutheren
 * Messy but simple fixes
 *   - Add access to SIP REGISTER timeout
 *   - Ensure OpalConnection options are correctly progagated
 *
 * Revision 1.15  2006/09/28 07:42:17  csoutheren
 * Merge of useful SRTP implementation
 *
 * Revision 1.14  2006/09/11 03:08:50  dereksmithies
 * Add fixes from Stephen Cook (sitiveni@gmail.com) for new patches to
 * improve call handling. Notably, IAX2 call transfer. Many thanks.
 * Thanks also to the Google summer of code for sponsoring this work.
 *
 * Revision 1.13  2006/08/13 02:59:47  rjongbloed
 * Fixed warning on no trace build
 *
 * Revision 1.12  2006/08/10 04:44:03  csoutheren
 * Added new IAX2 files to VC 2005 project
 * Fixed compile warning on Windows
 *
 * Revision 1.11  2006/08/09 03:46:39  dereksmithies
 * Add ability to register to a remote Asterisk box. The iaxProcessor class is split
 * into a callProcessor and a regProcessor class.
 * Big thanks to Stephen Cook, (sitiveni@gmail.com) for this work.
 *
 * Revision 1.9  2006/07/21 00:44:14  csoutheren
 * Applied 1483216 - Opal IAX sends 2 new call messages on outbound call
 * Thanks to Mike T
 *
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
#include <iax2/specialprocessor.h>

#define new PNEW


////////////////////////////////////////////////////////////////////////////////

IAX2EndPoint::IAX2EndPoint(OpalManager & mgr)
  : OpalEndPoint(mgr, "iax2", CanTerminateCall)
{
  
  localUserName = mgr.GetDefaultUserName();
  localNumber   = "1234";
  
  statusQueryCounter = 1;
  specialPacketHandler = new IAX2SpecialProcessor(*this);
  
  transmitter = NULL;
  receiver = NULL;
  sock = NULL;
  callsEstablished.SetValue(0);
  
  //We handle the deletion of regProcessor objects.
  regProcessors.AllowDeleteObjects(FALSE);

  Initialise();
  PTRACE(5, "IAX2\tCreated endpoint.");
}

IAX2EndPoint::~IAX2EndPoint()
{
  PTRACE(5, "Endpoint\tIaxEndPoint destructor. Terminate the  transmitter, receiver, and incoming frame handler.");
  
  //contents of this array are automatically shifted when removed
  //so we only need to loop through the first element until all
  //elements are removed
  while (regProcessors.GetSize()) {
    IAX2RegProcessor *regProcessor = (IAX2RegProcessor*)regProcessors.GetAt(0);
    regProcessor->Unregister();
    regProcessors.RemoveAt(0);
    delete regProcessor;
  }

  PTRACE(6, "Iax2Endpoint\tDestructor - cleaned up the different registration processeors");

  incomingFrameHandler.Terminate();
  incomingFrameHandler.WaitForTermination();
  packetsReadFromEthernet.AllowDeleteObjects();  
  PTRACE(6, "Iax2Endpoint\tDestructor - cleaned up the incoming frame handler");
  
  transmitter->Terminate();
  receiver->Terminate();
  transmitter->WaitForTermination();
  PTRACE(6, "Iax2Endpoint\tDestructor - cleaned up the iax2 transmitter");
  receiver->WaitForTermination();
  PTRACE(6, "Iax2Endpoint\tDestructor - cleaned up the iax2 receiver");

  if (specialPacketHandler != NULL) {
    specialPacketHandler->Terminate();
    specialPacketHandler->WaitForTermination();
    delete specialPacketHandler;
    PTRACE(6, "Iax2Endpoint\tDestructor - cleaned up the iax2 special packet handler");
  }
  specialPacketHandler = NULL;
  
  if (transmitter != NULL)
    delete transmitter;
  if (receiver != NULL)
    delete receiver;

  if (sock != NULL)
    delete sock; 
  
  PTRACE(6, "Endpoint\tDESTRUCTOR of IAX2 endpoint has Finished.");  
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
  PTRACE(3, "IAX2\tWe have received a NEW request from " << f->GetConnectionToken());
  
  if (connectionsActive.Contains(f->GetConnectionToken())) {
    PTRACE(3, "IAX2\thave received  a duplicate new packet from " << f->GetConnectionToken());
    cerr << " Haave received  a duplicate new packet from " << f->GetConnectionToken() << endl;
    delete f;
    return;
  }

/* We need to extract the username from the incoming frame. We have to do this now, before
   establishing the connection. */
  IAX2FullFrameProtocol ffp(*f);
  
  PString userName;
  PString host = f->GetRemoteInfo().RemoteAddress();
  
  {    
    IAX2RegProcessor *regProcessor = NULL;
    
    PWaitAndSignal m(regProcessorsMutex);
    
    PINDEX size = regProcessors.GetSize();
    for (PINDEX i = 0; i < size; i++) {
      regProcessor = (IAX2RegProcessor*)regProcessors.GetAt(i);
      
      if (regProcessor->GetHost() == host) {
        userName = regProcessor->GetUserName();
        break;
      }
    }
  }
  
  /* take the info in the NEW packet and use it to build information about the
     person who is calling us */
  IAX2IeData ieData;
  ffp.CopyDataFromIeListTo(ieData);
  PString url = BuildUrl(host, userName, ieData.callingNumber);

/* We have completed the extraction of information process. Now we can 
   build the matching connection */
  IAX2Connection *connection = CreateConnection(*manager.CreateCall(NULL), 
						f->GetConnectionToken(), NULL, 
						url, ieData.callingName);
  if (!AddConnection(connection)) {
    PTRACE(2, "IAX2\tFailed to create IAX2Connection for NEW request from " 
	   << f->GetConnectionToken());
    delete f;
    delete connection;

    return;
  }

  connection->IncomingEthernetFrame(f);
}


void IAX2EndPoint::OnEstablished(OpalConnection & con)
{
  PTRACE(3, "Endpoint\tOnEstablished for " << con);

  OpalEndPoint::OnEstablished(con);
}

PINDEX IAX2EndPoint::NextSrcCallNumber(IAX2Processor * /*processor*/)
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

  // ReportStoredConnections();

  BOOL res = connectionsActive.Contains(frameToken);
  if (res) {
    return TRUE;
  }

  mutexTokenTable.Wait();
  PString tokenTranslated = tokenTable(frameToken);
  mutexTokenTable.Signal();

  if (tokenTranslated.IsEmpty()) {
    PTRACE(4, "No matching translation table entry token for \"" << frameToken << "\"");
    return FALSE;
  }

  res = connectionsActive.Contains(tokenTranslated);
  if (res) {
    PTRACE(5, "Found \"" << tokenTranslated << "\" in the connectionsActive table");
    return TRUE;
  }

  PTRACE(4, "ERR Could not find matching connection for \"" << tokenTranslated 
	 << "\" or \"" << frameToken << "\"");
  return FALSE;
}

void IAX2EndPoint::ReportStoredConnections()
{
  PStringList cons = GetAllConnections();
  PTRACE(5, " There are " << cons.GetSize() << " stored connections in connectionsActive");
  PINDEX i;
  for(i = 0; i < cons.GetSize(); i++) {
    PTRACE(5, "    #" << (i + 1) << "                     \"" << cons[i] << "\"");
  }

  PWaitAndSignal m(mutexTokenTable);
  PTRACE(5, " There are " << tokenTable.GetSize() << " stored connections in the token translation table.");
  for (i = 0; i < tokenTable.GetSize(); i++) {
    PTRACE(5, " token table at " << i << " is " << tokenTable.GetKeyAt(i) << " " << tokenTable.GetDataAt(i));
  }
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

  PTRACE(4, "Opal\t call protocol          " << res[protoIndex]);
  PTRACE(4, "Opal\t destination user       " << res[userIndex]);
  PTRACE(4, "Opal\t transport to use       " << res[transportIndex]);
  PTRACE(4, "Opal\t destination address    " << res[addressIndex]);
  PTRACE(4, "Opal\t destination extension  " << res[extensionIndex]);
  PTRACE(4, "Opal\t destination context    " << res[contextIndex]);

  return res;
}

PString IAX2EndPoint::BuildUrl(
    const PString & host,
    const PString & userName,
    const PString & extension,
    const PString & context,
    const PString & transport
    )
{
  PString url;
  
  url = host;
  
  if (!extension.IsEmpty())
    url = url + "/" + extension;
    
  if (!context.IsEmpty() && context != "Default")
    url = url + "+" + context;
  
  if (!transport.IsEmpty())
    url = transport + "$" + url;
  
  if (!userName.IsEmpty())
    url = userName + "@" + url;
    
  return url;
}

BOOL IAX2EndPoint::MakeConnection(
				 OpalCall & call,
				 const PString & rParty, 
				 void * userData,
				 unsigned int /*options*/,
         OpalConnection::StringOptions * /*stringOptions*/)
{
  /* This method is called as Step 1 of making an IAX call to some remote destination 
    this method is invoked by the OpalManager, who controls everything, in response by a 
     users desire to talk to someone else */

  PTRACE(3, "IaxEp\tTry to make iax2 call to " << rParty);
  PTRACE(4, "IaxEp\tParty A=\"" << call.GetPartyA() << "\"  and party B=\"" <<  call.GetPartyB() << "\"");

  
  PStringList remoteInfo = DissectRemoteParty(rParty);
  if(remoteInfo[protoIndex] != PString("iax2"))
    return FALSE;

  PString remotePartyName = rParty.Mid(5);    

  PTRACE(5, "OpalMan\tNow do dns resolution of " << remoteInfo[addressIndex] << " for an iax2 call");
  PIPSocket::Address ip;
  if (!PIPSocket::GetHostAddress(remoteInfo[addressIndex], ip)) {
    PTRACE(3, "Could not make a iax2 call to " << remoteInfo[addressIndex] << " as IP resolution failed");
    return FALSE;
  }

  PStringStream callId;
  callId << "iax2:" <<  ip.AsString() << "OutgoingCall" << PString(++callsEstablished);
  IAX2Connection * connection = CreateConnection(call, callId, userData, remotePartyName);
  if (!AddConnection(connection))
    return FALSE;

  //search through the register srcProcessors to see if there is a relevant userName
  //and password we can use for authentication.  If there isn't then the default
  //userName and password of this endpoint will be used instead.
  {
    PWaitAndSignal m(regProcessorsMutex);
    PINDEX size = regProcessors.GetSize();
    
    for (PINDEX i = 0; i < size; i++) {
      IAX2RegProcessor *regProcessor = (IAX2RegProcessor*)regProcessors.GetAt(i);
      
      if (regProcessor->GetHost() == remoteInfo[addressIndex]) {
        PString userName = regProcessor->GetUserName();
        PString password = regProcessor->GetPassword();
        
        connection->SetUserName(userName);
        connection->SetPassword(password);
        break;
      }
    }
  }
  
  // If we are the A-party then need to initiate a call now in this thread. If
  // we are the B-Party then SetUpConnection() gets called in the context of
  // the A-party thread.
  if (call.GetConnection(0) == (OpalConnection*)connection) {
    PTRACE(3, "IAX2Ep\tInitiate a call to the other party");
    connection->SetUpConnection();
  }

  return TRUE;
}

IAX2Connection * IAX2EndPoint::CreateConnection(
      OpalCall & call,
      const PString & token,
      void * userData,
      const PString & remoteParty,
      const PString & remotePartyName)
{
  return new IAX2Connection(call, *this, token, userData, remoteParty, remotePartyName); 
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
  rand.SetSeed((DWORD)(PTime().GetTimeInSeconds() + 1));
  callnumbs = PRandom::Number() % 32000;
  
  sock = new PUDPSocket(ListenPortNumber());
  PTRACE(4, "IAX2EndPoint\tCreate Socket " << sock->GetPort());
  
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
    // Do Not have a FullFrame, so dont add a translation entry.
    return FALSE;
  }
  
  PINDEX destCallNo = frame->GetRemoteInfo().DestCallNumber();  /*Call number at our end */
  /* We do not know if the frame is encrypted, so examination of anything other than the 
     source call number/dest call number is unwise */ 

  PSafePtr<IAX2Connection> connection;
  for (connection = PSafePtrCast<OpalConnection, IAX2Connection>(connectionsActive.GetAt(0)); 
       connection != NULL; 
       ++connection) {
    if (connection->GetRemoteInfo().SourceCallNumber() == destCallNo) {
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
//The frame has not been acknowledged, or replied to.
void IAX2EndPoint::IncomingEthernetFrame(IAX2Frame *frame)
{
  PTRACE(5, "IAXEp\tEthernet Frame received from Receiver " << frame->IdString());
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
    PTRACE(5, "Distribution\tNow try to find a home for " << idString);
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
  for (PINDEX i = 0; i < list.GetSize(); i++) {
    PString format = list[i];
    codecNames += format;
  }

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
    compressedBytes = 160;
    duration = 20;
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
  for (i = 0; i < list.GetSize(); i++) {
    PString format = list[i];
    codecNames += format;
  }

  for(i = 0; i < codecNames.GetSize(); i++) {
    PTRACE(5, "Supported codec in opal is " << codecNames[i]);
  }
    
  PINDEX returnValue = 0;
  for (i = 0; i < codecNames.GetSize(); i++) 
    returnValue += IAX2FullFrameVoice::OpalNameToIax2Value(codecNames[i]);

  PTRACE(5, "Bitmask of codecs we support is 0x" 
	 << ::hex << returnValue << ::dec);
  
  return  returnValue;
}

void IAX2EndPoint::CopyLocalMediaFormats(OpalMediaFormatList & list)
{
  for(PINDEX i = 0; i < localMediaFormats.GetSize(); i++) {
    PStringStream strm;
    strm << localMediaFormats[i];
    PTRACE(5, "copy local format " << strm);
    list += OpalMediaFormat(strm);
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

void IAX2EndPoint::Register(
      const PString & host,
      const PString & username,
      const PString & password,
      PINDEX requestedRefreshTime)
{
  PWaitAndSignal m(regProcessorsMutex);
  
  IAX2RegProcessor *regProcessor = 
          new IAX2RegProcessor(*this, host, username, password, requestedRefreshTime);
  
  regProcessors.Append(regProcessor);
}

void IAX2EndPoint::OnRegistered(
      const PString & /*host*/,
      const PString & /*username*/,
      BOOL /*isFailure*/,
      RegisteredError /*reason*/)
{
  PTRACE(2, "registration event occured");
}

void IAX2EndPoint::OnUnregistered(
      const PString & /*host*/,
      const PString & /*username*/,
      BOOL /*isFailure*/,
      UnregisteredError /*reason*/)
{
  PTRACE(2, "unregistration event occured");
}

void IAX2EndPoint::Unregister(
      const PString & host,
      const PString & username)
{
  IAX2RegProcessor *removeRegProcesser = NULL;
  
  //this section and loop is optimized to remove
  //the time the lock will be held.
  {
    PWaitAndSignal m(regProcessorsMutex);
    PINDEX size = regProcessors.GetSize();
    
    for (PINDEX i = 0; i < size; i++) {
      IAX2RegProcessor *regProcessor = (IAX2RegProcessor*)regProcessors.GetAt(i);
      
      if (regProcessor->GetHost() == host && 
          regProcessor->GetUserName() == username) {
        regProcessors.RemoveAt(i);
        removeRegProcesser = regProcessor; 
        break;
      }
    }
  }
  
  if (removeRegProcesser != NULL) {
    removeRegProcesser->Unregister();
    delete removeRegProcesser;
  }
}

BOOL IAX2EndPoint::IsRegistered(const PString & host, const PString & username)
{
  PWaitAndSignal m(regProcessorsMutex);
  
  PINDEX size = regProcessors.GetSize();
    
  for (PINDEX i = 0; i < size; i++) {
    IAX2RegProcessor *regProcessor = (IAX2RegProcessor*)regProcessors.GetAt(i);
      
    if (regProcessor->GetHost() == host &&
      regProcessor->GetUserName() == username) {
      return TRUE;
    }
  }
  
  return FALSE;
}

PINDEX IAX2EndPoint::GetRegistrationsCount() {
  PWaitAndSignal m(regProcessorsMutex);
  return regProcessors.GetSize();
}

BOOL IAX2EndPoint::OnIncomingCall(IAX2Connection & conn)
{
  return conn.OnIncomingCall(0, NULL);
}


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



