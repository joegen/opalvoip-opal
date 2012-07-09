/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 
 Author: Jonathan M. Henson
 Date: 07/08/2012
 Contact: jonathan.michael.henson@gmail.com
 */

#define LOCAL_MEDIA

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using OpalDotNET;
using System.IO;
using System.Runtime.InteropServices;

namespace OpalDotNETMain
{
  class Program
  {    
    static string currentCallToken;
    static string heldCallToken;
    static string playScript;
    static OpalContext context = new OpalContext();
    static FileStream audioInputFile = null;
    static FileStream audioOutputFile = null;

    
    static OpalMessageRef MySendCommand(OpalMessageRef command, string errorMessage)
    {
      OpalMessageRef response = new OpalMessageRef();
      if(!context.SendMessage(command, response))
          return null;     
      
      if (response.GetMessageType() != Opal_API.OpalMessageType.OpalIndCommandError)
        return response;

      if (response.GetCommandError() == null || response.GetCommandError() == "\0")
        Console.WriteLine("{0}.\n", errorMessage);
      else
        Console.WriteLine("{0}: {1}\n", errorMessage, response.GetCommandError()); 

      return null;
    }

#if LOCAL_MEDIA
    static int MyReadMediaData(string token, string id, string format, IntPtr userData, IntPtr data, int size)
    {      
      if (audioInputFile == null) 
      {
        if (format == "PCM-16")
          audioInputFile = File.Open("ogm.wav", FileMode.Open);
        Console.WriteLine("Reading {0} media for stream {1} on call {2}{3}", format, id, token, Environment.NewLine);
      }

      if (audioInputFile != null)
      { 
        byte[] byData = new byte[size];
        int written = audioInputFile.Read(byData, 0, size);
        Marshal.Copy(byData, 0, data, written);
        return written;
      }      
 
      return size;
    }


    static int MyWriteMediaData(string token, string id, string format, IntPtr userData, IntPtr data, int size)
    {      
      if (audioOutputFile == null) 
      {        
        string name;
        name = String.Format("Media-{0}-{1}.{2}", token, id, format);
        audioOutputFile = File.OpenWrite(name);

        if (audioOutputFile == null)
        {
          Console.WriteLine("Could not create media output file \"{0}\"{1}", name, Environment.NewLine);
          return -1;
        }

        Console.WriteLine("Writing {0} media for stream {1} on call {2}{3}", format, id, token, Environment.NewLine);
      }

      byte[] byData = new byte[size];
      Marshal.Copy(data, byData, 0, size);

      audioOutputFile.Write(byData, 0, size);      

      return size; 
    }
#endif

    static int InitialiseOPAL()
    {
      OpalMessageRef command;
      OpalMessageRef response;
      uint version;

      string OPALOptions = Opal_API.OPAL_PREFIX_H323  + " " +
                                    Opal_API.OPAL_PREFIX_SIP + " " +
                                    Opal_API.OPAL_PREFIX_IAX2 + " " +
#if LOCAL_MEDIA
                                    Opal_API.OPAL_PREFIX_LOCAL +
#else
                                    Opal_API.OPAL_PREFIX_PCSS +
#endif
                                    " " +
                                    Opal_API.OPAL_PREFIX_IVR +
                                    " TraceLevel=4 TraceFile=debugstream";

        ///////////////////////////////////////////////
        // Initialisation

        version = Opal_API.OPAL_C_API_VERSION;
  
        if (context.Initialise(OPALOptions, version) == version) 
        {
          Console.WriteLine("Could not initialise OPAL{0}", Environment.NewLine);
          return 0;
        }
  
        #if NULL
        // Test shut down and re-initialisation
        context.ShutDown();

        if (context.Initialise(OPALOptions, version) == version) 
        {
          Console.WriteLine("Could not reinitialise OPAL{0}", Environment.NewLine);
          return 0;
        }  
        #endif

        // General options
        command = new OpalMessageRef(Opal_API.OpalMessageType.OpalCmdSetGeneralParameters);  
        //command.m_param.m_general.m_audioRecordDevice = "Camera Microphone (2- Logitech";
        OpalParamGeneralRef m_general = command.GetGeneralParams();

        m_general.AutoRxMedia = m_general.AutoTxMedia = "audio";
        m_general.StunServer = "stun.voxgratia.org";
        m_general.MediaMask = "RFC4175*";

#if LOCAL_MEDIA
        m_general.MediaReadData = MyReadMediaData;
        m_general.MediaWriteData = MyWriteMediaData;
        m_general.MediaDataHeader = (uint)Opal_API.OpalMediaDataType.OpalMediaDataPayloadOnly;
#endif

        if ((response = MySendCommand(command, "Could not set general options")) == null)
          return 0;     
  
        // Options across all protocols
        command = new OpalMessageRef(Opal_API.OpalMessageType.OpalCmdSetProtocolParameters);
        OpalParamProtocolRef m_protocol = command.GetProtocolParams();

        m_protocol.UserName = "robertj";
        m_protocol.DisplayName = "Robert Jongbloed";
        m_protocol.InterfaceAddresses = "*";

        if ((response = MySendCommand(command, "Could not set protocol options")) == null)
          return 0; 

        command = new OpalMessageRef(Opal_API.OpalMessageType.OpalCmdSetProtocolParameters);
        m_protocol = command.GetProtocolParams();

        m_protocol.Prefix = "sip";
        m_protocol.DefaultOptions = "PRACK-Mode=0\nInitial-Offer=false";

        if ((response = MySendCommand(command, "Could not set SIP options")) == null)
          return 0;  

        return 1;
     }

    static void HandleMessages(uint timeout)
    {
      OpalMessageRef command = new OpalMessageRef();
      OpalMessageRef response;
      OpalMessageRef message = new OpalMessageRef();

      while (context.GetMessage(message, timeout))
      {
        switch (message.GetMessageType())
        {
          case Opal_API.OpalMessageType.OpalIndRegistration:
            OpalStatusRegistrationRef m_param = message.GetRegistrationStatus();

            switch (m_param.Status)
            {
              case Opal_API.OpalRegistrationStates.OpalRegisterRetrying:
                Console.WriteLine("Trying registration to {0}.", m_param.ServerName);
                break;
              case Opal_API.OpalRegistrationStates.OpalRegisterRestored:
                Console.WriteLine("Registration of {0} restored.",m_param.ServerName);
                break;
              case Opal_API.OpalRegistrationStates.OpalRegisterSuccessful:
                Console.WriteLine("Registration of {0} successful.", m_param.ServerName);
                break;
              case Opal_API.OpalRegistrationStates.OpalRegisterRemoved:
                Console.WriteLine("Unregistered {0}.", m_param.ServerName);
                break;
              case Opal_API.OpalRegistrationStates.OpalRegisterFailed:
                if (m_param.Error == null || m_param.Error.Length == 0)
                  Console.WriteLine("Registration of {0} failed.", m_param.ServerName);
                else
                  Console.WriteLine("Registration of {0} error: {1}",m_param.ServerName, m_param.Error);
                break;
            }
            break;

          case Opal_API.OpalMessageType.OpalIndLineAppearance:
            OpalStatusLineAppearanceRef m_lineStatus = message.GetLineAppearance();
            switch (m_lineStatus.State)
            {
              case Opal_API.OpalLineAppearanceStates.OpalLineIdle:
                Console.WriteLine("Line {0} available.", m_lineStatus.Line);
                break;
              case Opal_API.OpalLineAppearanceStates.OpalLineTrying:
                Console.WriteLine("Line {0} in use.", m_lineStatus.Line);
                break;
              case Opal_API.OpalLineAppearanceStates.OpalLineProceeding:
                Console.WriteLine("Line {0} calling.", m_lineStatus.Line);
                break;
              case Opal_API.OpalLineAppearanceStates.OpalLineRinging:
                Console.WriteLine("Line {0} ringing.", m_lineStatus.Line);
                break;
              case Opal_API.OpalLineAppearanceStates.OpalLineConnected:
                Console.WriteLine("Line {0} connected.", m_lineStatus.Line);
                break;
              case Opal_API.OpalLineAppearanceStates.OpalLineSubcribed:
                Console.WriteLine("Line {0} subscription successful.", m_lineStatus.Line);
                break;
              case Opal_API.OpalLineAppearanceStates.OpalLineUnsubcribed:
                Console.WriteLine("Unsubscribed line {0}.", m_lineStatus.Line);
                break;
            }
            break;

          case Opal_API.OpalMessageType.OpalIndIncomingCall:
            OpalStatusIncomingCallRef incomingCall = message.GetIncomingCall();

            Console.WriteLine("Incoming call from \"{0}\", \"{1}\" to \"{2}\", handled by \"{3}\".",
                   incomingCall.RemoteDisplayName,
                   incomingCall.RemoteAddress,
                   incomingCall.CalledAddress,
                   incomingCall.LocalAddress);
            if (currentCallToken == null)
            {
              command = new OpalMessageRef(Opal_API.OpalMessageType.OpalCmdAnswerCall);
              OpalParamAnswerCallRef answerCall = command.GetAnswerCall();
              answerCall.CallToken = incomingCall.CallToken;
              OpalParamProtocolRef overrides = new OpalParamProtocolRef(answerCall.Overrides);              
              overrides.UserName = "test123";
              overrides.DisplayName = "Test Called Party";
              answerCall.Overrides = overrides.Param;

              MySendCommand(command, "Could not answer call");              
            }
            else
            {
              command = new OpalMessageRef(Opal_API.OpalMessageType.OpalCmdClearCall);
              OpalParamCallClearedRef clearCall = command.GetClearCall();
              OpalStatusIncomingCallRef m_incomingCall = message.GetIncomingCall();
              clearCall.CallToken = m_incomingCall.CallToken;
              clearCall.Reason = Opal_API.OpalCallEndReason.OpalCallEndedByLocalBusy;
              MySendCommand(command, "Could not refuse call");                
            }
            break;

          case Opal_API.OpalMessageType.OpalIndProceeding:
            Console.WriteLine("Proceeding.");
            break;

          case Opal_API.OpalMessageType.OpalIndAlerting:
            Console.WriteLine("Ringing.");
            break;

          case Opal_API.OpalMessageType.OpalIndEstablished:
            Console.WriteLine("Established.");

            if (playScript != null)
            {
              Console.WriteLine("Playing {0}", playScript);

              command = new OpalMessageRef(Opal_API.OpalMessageType.OpalCmdTransferCall);
              OpalParamSetUpCallRef m_callSetUp = command.GetCallSetUp();
              m_callSetUp.CallToken = currentCallToken;
              m_callSetUp.PartyA = "pc:";
              m_callSetUp.PartyB = playScript;
              MySendCommand(command, "Could not start playing");               
            }
            break;

          case Opal_API.OpalMessageType.OpalIndMediaStream:
            OpalStatusMediaStreamRef m_mediaStream = message.GetMediaStream();
            Console.WriteLine("Media stream {0} {1} using {2}.", m_mediaStream.Type, 
              m_mediaStream.State == Opal_API.OpalMediaStates.OpalMediaStateOpen ? "opened" : "closed",
              m_mediaStream.Format);
            break;

          case Opal_API.OpalMessageType.OpalIndUserInput:
            OpalStatusUserInputRef m_userInput = message.GetUserInput();
            Console.WriteLine("User Input: {0}.", m_userInput.UserInput);
            break;

          case Opal_API.OpalMessageType.OpalIndCallCleared:
            OpalStatusCallClearedRef m_callCleared = message.GetCallCleared();
            if (m_callCleared.Reason == null)
              Console.WriteLine("Call cleared.");
            else
              Console.WriteLine("Call cleared: {0}", m_callCleared.Reason);
            break;

          default:
            break;
        }        
      }
    }

    static int DoCall(string from, string to)
    {
      // Example cmd line: call 612@ekiga.net
      OpalMessageRef command = new OpalMessageRef();
      OpalMessageRef response = new OpalMessageRef();

      Console.WriteLine("Calling {0}", to);
    
      command.SetMessageType(Opal_API.OpalMessageType.OpalCmdSetUpCall);
      OpalParamSetUpCallRef m_callSetUp = command.GetCallSetUp();
      m_callSetUp.PartyA = from;
      m_callSetUp.PartyB = to;

      OpalParamProtocolRef overrides = new OpalParamProtocolRef(new Opal_API.OpalParamProtocol());
      overrides.DisplayName = "Test Calling Party";
      m_callSetUp.Overrides = overrides.Param;

      if ((response = MySendCommand(command, "Could not make call")) == null)
        return 0;

      m_callSetUp = response.GetCallSetUp();
      currentCallToken = m_callSetUp.CallToken;
  
      return 1;
    }

    static int DoMute(bool on)
    {
      // Example cmd line: mute 612@ekiga.net
      OpalMessageRef command = new OpalMessageRef();
      OpalMessageRef response = new OpalMessageRef();

      Console.WriteLine("Mute {0}", on ? "on" : "off");
      
      command.SetMessageType(Opal_API.OpalMessageType.OpalCmdMediaStream);
      OpalStatusMediaStreamRef m_mediaStream = command.GetMediaStream();
      m_mediaStream.CallToken = currentCallToken;
      m_mediaStream.Type = "audio out";
      m_mediaStream.State = on ? Opal_API.OpalMediaStates.OpalMediaStatePause : Opal_API.OpalMediaStates.OpalMediaStateResume;
      if ((response = MySendCommand(command, "Could not mute call")) == null)      
        return 0;
              
      return 1;
    }

    static int DoHold()
    {
      // Example cmd line: hold 612@ekiga.net
      OpalMessageRef command = new OpalMessageRef();
      OpalMessageRef response;

      Console.WriteLine("Hold");
      
      command.SetMessageType(Opal_API.OpalMessageType.OpalCmdHoldCall);
      command.SetCallToken(currentCallToken);
      
      if ((response = MySendCommand(command, "Could not hold call")) == null)
        return 0;

      heldCallToken = currentCallToken;
      currentCallToken = null;
      
      return 1;
    }

    static int DoTransfer(string to)
    {
      // Example cmd line: transfer fred@10.0.1.11 noris@10.0.1.15
      OpalMessageRef command = new OpalMessageRef();
      OpalMessageRef response;
      
      Console.WriteLine("Transferring to {0}", to);
      
      command.SetMessageType(Opal_API.OpalMessageType.OpalCmdTransferCall);
      OpalParamSetUpCallRef m_callSetup = command.GetCallSetUp();
      m_callSetup.PartyB = to;
      m_callSetup.CallToken = currentCallToken;

      if ((response = MySendCommand(command, "Could not transfer call")) == null)
        return 0;
     
      return 1;
    }

    static int DoRegister(string aor, string pwd)
    {
      // Example cmd line: register robertj@ekiga.net secret
      OpalMessageRef command = new OpalMessageRef();
      OpalMessageRef response;      

      Console.WriteLine("Registering {0}", aor);

      command.SetMessageType(Opal_API.OpalMessageType.OpalCmdRegistration);
      OpalParamRegistrationRef m_registrationInfo = command.GetRegistrationInfo();

      if (!aor.Contains(':')) 
      {        
        m_registrationInfo.Protocol = "h323";
        m_registrationInfo.Identifier = aor;
      }
      else 
      {        
        m_registrationInfo.Protocol = aor;
        m_registrationInfo.Identifier = aor.Substring(aor.IndexOf(':') + 1);
      }

      m_registrationInfo.Password = pwd;
      m_registrationInfo.TimeToLive = 300;

      if ((response = MySendCommand(command, "Could not register endpoint")) == null)
        return 0;
      
      return 1;
    }

    static int DoSubscribe(string package, string aor, string from)
    {
      // Example cmd line: subscribe "dialog;sla;ma" 1501@192.168.1.32 1502@192.168.1.32
      OpalMessageRef command = new OpalMessageRef();
      OpalMessageRef response;

      Console.WriteLine("Susbcribing {0}", aor);

      command.SetMessageType(Opal_API.OpalMessageType.OpalCmdRegistration);
      OpalParamRegistrationRef m_registrationInfo = command.GetRegistrationInfo();

      m_registrationInfo.Protocol = "sip";
      m_registrationInfo.Identifier = aor;
      m_registrationInfo.HostName = from;
      m_registrationInfo.EventPackage = package;
      m_registrationInfo.TimeToLive = 300;

      if ((response = MySendCommand(command, "Could not subscribe")) == null)
        return 0;
              
      return 1;
    }

    static int DoRecord(string to, string file)
    {
      // Example cmd line: call 612@ekiga.net
      OpalMessageRef command = new OpalMessageRef();
      OpalMessageRef response;

      Console.WriteLine("Calling {0}", to);

      command.SetMessageType(Opal_API.OpalMessageType.OpalCmdSetUpCall);
      OpalParamSetUpCallRef m_callSetUp = command.GetCallSetUp();

      m_callSetUp.PartyB = to;
      if ((response = MySendCommand(command, "Could not make call")) == null)
        return 0;

      currentCallToken = response.GetCallSetUp().CallToken;
      
      Console.WriteLine("Recording {0}", file);

      command.SetMessageType(Opal_API.OpalMessageType.OpalCmdStartRecording);
      OpalParamRecordingRef m_recording = command.GetRecording();

      m_recording.CallToken = currentCallToken;
      m_recording.File = file;
      m_recording.Channels = 2;

      if ((response = MySendCommand(command, "Could not start recording")) == null)
        return 0;

      return 1;
    }

    static int DoPlay(string to, string file)
    {
      // Example cmd line: call 612@ekiga.net
      OpalMessageRef command = new OpalMessageRef();
      OpalMessageRef response;

      Console.WriteLine("Playing {0} to {1}", file, to);

      playScript = String.Format("ivr:<?xml version=\"1.0\"?><vxml version=\"1.0\"><form id=\"PlayFile\">" +
                   "<audio src=\"{0}\"/></form></vxml>", file);      

      command.SetMessageType(Opal_API.OpalMessageType.OpalCmdSetUpCall);
      OpalParamSetUpCallRef m_callSetUp = command.GetCallSetUp();
      m_callSetUp.PartyB = to;

      if ((response = MySendCommand(command, "Could not make call")) == null)
        return 0;

      currentCallToken = response.GetCallSetUp().CallToken;
      
      return 1;
    }

    enum Operations
    {
      OpListen,
      OpCall,
      OpMute,
      OpHold,
      OpTransfer,
      OpConsult,
      OpRegister,
      OpSubscribe,
      OpRecord,
      OpPlay,
      NumOperations
    }

    static string[] OperationNames = { "listen", "call", "mute", "hold", "transfer", "consult", "register", "subscribe", "record", "play" };

    static int[] RequiredArgsForOperation = { 1, 2, 2, 2, 3, 3, 2, 2, 2, 2 };
  
    static Operations GetOperation(string name)
    {
      Operations op;

      for (op = Operations.OpListen; op < Operations.NumOperations; op++) {
        if (name == OperationNames[(int)op])
          break;
      }

      return op;
    }

    static void Main(string[] args)
    {
      Operations operation;

      if (args.Length < 1 || (operation = GetOperation(args[0])) == Operations.NumOperations || args.Length < RequiredArgsForOperation[(int)operation])
      {
        Operations op;
        Console.Write("usage: c_api { ");

        for (op = Operations.OpListen; op < Operations.NumOperations; op++)
        {
          if (op > Operations.OpListen)
            Console.Write(" | ");
          Console.Write(OperationNames[(int)op]);
        }

        Console.Write(" } [ A-party [ B-party | file ] ]{0}", Environment.NewLine);
        return;
      }

      Console.WriteLine("Initialising.");

      if (InitialiseOPAL() == 0)
        return;

      switch (operation)
      {
        case Operations.OpListen:
          Console.WriteLine("Listening.");
          HandleMessages(60000);
          break;

        case Operations.OpCall:
          if (args.Length > 3)
          {
            if (DoCall(args[2], args[3]) == 0)
              break;
          }
          else
          {
            if (DoCall(null, args[2]) ==  0)
              break;
          }
          HandleMessages(15000);
          break;

        case Operations.OpMute:
          if (DoCall(null, args[2]) == 0)
            break;
          HandleMessages(15000);
          if (DoMute(true) == 0)
            break;
          HandleMessages(15000);
          if (DoMute(false) == 0)
            break;
          HandleMessages(15000);
          break;

        case Operations.OpHold:
          if (DoCall(null, args[2]) == 0)
            break;
          HandleMessages(15000);
          if (DoHold() == 0)
            break;
          HandleMessages(15000);
          break;

        case Operations.OpTransfer:
          if (DoCall(null, args[2]) == 0)
            break;
          HandleMessages(15000);
          if (DoTransfer(args[3]) == 0)
            break;
          HandleMessages(15000);
          break;

        case Operations.OpConsult:
          if (DoCall(null, args[2]) == 0)
            break;
          HandleMessages(15000);
          if (DoHold() == 0)
            break;
          HandleMessages(15000);
          if (DoCall(null, args[3]) == 0)
            break;
          HandleMessages(15000);
          if (DoTransfer(heldCallToken) == 0)
            break;
          HandleMessages(15000);
          break;

        case Operations.OpRegister:
          if (DoRegister(args[2], args[3]) == 0)
            break;
          HandleMessages(15000);
          break;

        case Operations.OpSubscribe:
          if (DoSubscribe(args[2], args[3], args[4]) == 0)
            break;
          HandleMessages(int.MaxValue); // More or less forever
          break;

        case Operations.OpRecord:
          if (DoRecord(args[2], args[3]) == 0)
            break;
          HandleMessages(int.MaxValue); // More or less forever
          break;

        case Operations.OpPlay:
          if (DoPlay(args[2], args[3]) == 0)
            break;
          HandleMessages(int.MaxValue); // More or less forever
          break;

        default:
          break;
      }

      Console.WriteLine("Exiting.");

      context.ShutDown();      
      return;
    }
  }
}
