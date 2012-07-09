/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 
 Author: Jonathan M. Henson
 Date: 07/08/2012
 Contact: jonathan.michael.henson@gmail.com
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;

namespace OpalDotNET
{
  /** This class is a wrapper around the "C" API.
  */
  public class OpalContext
  {
    /// <summary>
    /// Construct an unintialised OPAL context.
    /// </summary>
    public OpalContext()
    {
        m_handle = IntPtr.Zero;
    }

    /// <summary>
    /// Destroy the OPAL context, calls ShutDown().
    /// </summary>
    ~OpalContext()
    {
      ShutDown();
    }   
    
    /// <summary>
    /// Calls OpalIntialise() to initialise the OPAL context.
    /// Returns version of API supported by library, zero if error.    
    /// </summary>
    /// <param name="options">
    /// List of options to pass to OpalIntialise()
    /// </param>
    /// <param name="version">
    /// Version expected by application
    /// </param>    
    public uint Initialise(string options, uint version = Opal_API.OPAL_C_API_VERSION )
    {
      ShutDown();

      m_handle = Opal_API.OpalInitialise(ref version, options);

      return m_handle != IntPtr.Zero ? version : 0;
    }

    /// <summary>
    /// Indicate if the OPAL context has been initialised.
    /// </summary>   
    public bool IsInitialised()  
    { 
      return m_handle != null; 
    }

    /// <summary>
    /// Calls OpalShutDown() to dispose of the OPAL context.
    /// </summary>
    public void ShutDown()
    {
      if (m_handle != IntPtr.Zero)
      {
        Opal_API.OpalShutDown(m_handle);
        m_handle = IntPtr.Zero;
      }
    }

    /// <summary>
    /// Calls OpalGetMessage() to get next message from the OPAL context.
    /// </summary>   
    public bool GetMessage( OpalMessageRef message, uint timeout = 0)
    {
      if (m_handle == IntPtr.Zero)
      {
        message.SetMessageType(Opal_API.OpalMessageType.OpalIndCommandError);
        message.CmdError = "Uninitialised OPAL context.";
        return false;
      }
     
      IntPtr ptrMessage = Opal_API.OpalGetMessage(m_handle, timeout);

      if (ptrMessage == IntPtr.Zero)
      {
        message.SetMessageType(Opal_API.OpalMessageType.OpalIndCommandError);
        message.CmdError = "Timeout getting message.";
        return false;
      }        

      message.m_message = (Opal_API.OpalMessage)Marshal.PtrToStructure(ptrMessage, typeof(Opal_API.OpalMessage));      
      return true;      
    }
    
    /// <summary>
    /// Calls OpalSendMessage() to send a message to the OPAL context.   
    /// </summary>
    /// <param name="message">Message to send to OPAL.</param>
    /// <param name="response"> Response from OPAL.</param>    
    public bool SendMessage(OpalMessageRef message, OpalMessageRef response)
    {
      if (m_handle == IntPtr.Zero)
      {
        response.SetMessageType(Opal_API.OpalMessageType.OpalIndCommandError);
        response.CmdError = "Uninitialised OPAL context.";
        return false;
      }

      IntPtr ptrMessage = Opal_API.OpalSendMessage(m_handle, ref message.m_message);  
      if(ptrMessage == IntPtr.Zero)
      {
        response.SetMessageType(Opal_API.OpalMessageType.OpalIndCommandError);
        response.CmdError = "Invalid message";
        return false;
      }

      response.m_message = (Opal_API.OpalMessage)Marshal.PtrToStructure(ptrMessage, typeof(Opal_API.OpalMessage));
      return response.GetMessageType() != Opal_API.OpalMessageType.OpalIndCommandError;      
    }
    
    /// <summary>
    /// Execute OpalSendMessage() using OpalCmdSetUpCall   
    /// </summary>
    /// <param name="response">Response from OPAL context on initiating call.</param>
    /// <param name="partyB">Destination address, see OpalCmdSetUpCall.</param>
    /// <param name="partyA">Calling sub-system, see OpalCmdSetUpCall.</param>
    /// <param name="alertingType">Alerting type code, see OpalCmdSetUpCall.</param>    
    public bool SetUpCall(OpalMessageRef response, string partyB, string partyA = null, string alertingType = null)
    {
      OpalMessageRef message = new OpalMessageRef(Opal_API.OpalMessageType.OpalCmdSetUpCall);
      OpalParamSetUpCallRef param = message.GetCallSetUp();
      param.PartyA = partyA;
      param.PartyB = partyB;
      param.AlertingType = alertingType;

      return SendMessage(message, response);
    }

    /// <summary>
    /// Answer a call using OpalCmdAnswerCall via OpalSendMessage()   
    /// <param name="callToken">Call token for call being answered.</param>    
    public bool AnswerCall(string callToken)
    {
      OpalMessageRef message = new OpalMessageRef(Opal_API.OpalMessageType.OpalCmdAnswerCall);
      OpalMessageRef response = new OpalMessageRef();

      message.SetCallToken(callToken);

      return SendMessage(message, response);
    }

    /// <summary>
    /// Clear a call using OpalCmdClearCall via OpalSendMessage()    
    /// </summary>
    /// <param name="callToken">
    /// Call token for call being cleared.
    /// </param>
    /// <param name="reason">
    /// Code for the call termination, see OpalCmdClearCall.
    /// </param>    
    public bool ClearCall(string callToken, Opal_API.OpalCallEndReason reason = Opal_API.OpalCallEndReason.OpalCallEndedByLocalUser)
    {
      OpalMessageRef message = new OpalMessageRef(Opal_API.OpalMessageType.OpalCmdClearCall);
      OpalMessageRef response = new OpalMessageRef();

      OpalParamCallClearedRef param = message.GetClearCall();
      param.CallToken = callToken;
      param.Reason = reason;

      return SendMessage(message, response);
    }

    /// <summary>
    /// Send user input using OpalCmdUserInput via OpalSendMessage()    
    /// </summary>
    /// <param name="callToken"> Call token for the call, see OpalCmdUserInput.</param>
    /// <param name="userInput">User input string, e.g. "#", see OpalCmdUserInput.</param>
    /// <param name="duration">Duration in milliseconds for tone, see OpalCmdUserInput.</param>   
    public bool SendUserInput(string callToken, string userInput, uint duration = 0)
    {
      OpalMessageRef message = new OpalMessageRef(Opal_API.OpalMessageType.OpalCmdClearCall);
      OpalMessageRef response = new OpalMessageRef();

      OpalStatusUserInputRef param = message.GetUserInput();
      param.CallToken = callToken;
      param.UserInput = userInput;
      param.Duration = duration;

      return SendMessage(message, response);
    }
      
    protected IntPtr m_handle;
  }

  /// Wrapper around the OpalMessage structure
  public class OpalMessageRef
  {
    internal Opal_API.OpalMessage m_message;
    private OpalParamGeneralRef m_generalParams;    
    private OpalParamProtocolRef m_protocolParams;
    private OpalParamRegistrationRef m_registrationInfo;   ///< Used by OpalCmdRegistration
    private OpalStatusRegistrationRef m_registrationStatus; ///< Used by OpalIndRegistration
    private OpalParamSetUpCallRef m_callSetUp;          ///< Used by OpalCmdSetUpCall/OpalIndProceeding/OpalIndAlerting/OpalIndEstablished
    private OpalStatusIncomingCallRef m_incomingCall;       ///< Used by OpalIndIncomingCall
    private OpalParamAnswerCallRef m_answerCall;         ///< Used by OpalCmdAnswerCall/OpalCmdAlerting
    private OpalStatusUserInputRef m_userInput;          ///< Used by OpalIndUserInput/OpalCmdUserInput
    private OpalStatusMessageWaitingRef m_messageWaiting;     ///< Used by OpalIndMessageWaiting
    private OpalStatusLineAppearanceRef m_lineAppearance;     ///< Used by OpalIndLineAppearance
    private OpalStatusCallClearedRef m_callCleared;        ///< Used by OpalIndCallCleared
    private OpalParamCallClearedRef m_clearCall;          ///< Used by OpalCmdClearCall
    private OpalStatusMediaStreamRef m_mediaStream;        ///< Used by OpalIndMediaStream/OpalCmdMediaStream
    private OpalParamSetUserDataRef m_setUserData;        ///< Used by OpalCmdSetUserData
    private OpalParamRecordingRef m_recording;          ///< Used by OpalCmdStartRecording
    private OpalStatusTransferCallRef m_transferStatus;     ///< Used by OpalIndTransferCall

    private OpalMessageRef(OpalMessageRef objToCopy) { }   

    public OpalMessageRef(Opal_API.OpalMessageType type = Opal_API.OpalMessageType.OpalIndCommandError)
    {       
       SetMessageType(type);
    }
    
    ~OpalMessageRef()
    {
      Opal_API.OpalFreeMessage(ref m_message);
    }

    public Opal_API.OpalMessageType GetMessageType()
    {
      return m_message.m_type;
    }

    public string CmdError
    {
      get { return Marshal.PtrToStringAnsi(m_message.m_param.m_commandError); }
      set { m_message.m_param.m_commandError = Marshal.StringToCoTaskMemAnsi(value); }
    }

    public string CallToken
    {
      get { return Marshal.PtrToStringAnsi(m_message.m_param.m_callToken); }
      set { m_message.m_param.m_callToken = Marshal.StringToCoTaskMemAnsi(value); }
    }

    public void SetMessageType(Opal_API.OpalMessageType type)
    {
      Opal_API.OpalFreeMessage(ref m_message);

      m_message = new Opal_API.OpalMessage();  
      
      switch(type)
      {
        case Opal_API.OpalMessageType.OpalCmdSetGeneralParameters:
          m_generalParams = new OpalParamGeneralRef(m_message.m_param.m_general);
          break;
        case Opal_API.OpalMessageType.OpalCmdSetProtocolParameters:
          m_protocolParams = new OpalParamProtocolRef(m_message.m_param.m_protocol);
          break;
        case Opal_API.OpalMessageType.OpalCmdRegistration:
          m_registrationInfo = new OpalParamRegistrationRef(m_message.m_param.m_registrationInfo);
          break;
        case Opal_API.OpalMessageType.OpalIndRegistration:
          m_registrationStatus = new OpalStatusRegistrationRef(m_message.m_param.m_registrationStatus);
          break;
        case Opal_API.OpalMessageType.OpalCmdSetUpCall:
        case Opal_API.OpalMessageType.OpalIndProceeding:
        case Opal_API.OpalMessageType.OpalIndAlerting:
        case Opal_API.OpalMessageType.OpalIndEstablished:
        case Opal_API.OpalMessageType.OpalIndCompletedIVR:
          m_callSetUp = new OpalParamSetUpCallRef(m_message.m_param.m_callSetUp);
          break;
        case Opal_API.OpalMessageType.OpalIndIncomingCall:
          m_incomingCall = new OpalStatusIncomingCallRef(m_message.m_param.m_incomingCall);
          break;
        case Opal_API.OpalMessageType.OpalCmdAlerting:
        case Opal_API.OpalMessageType.OpalCmdAnswerCall:
          m_answerCall = new OpalParamAnswerCallRef(m_message.m_param.m_answerCall);
          break;
        case Opal_API.OpalMessageType.OpalIndUserInput:
        case Opal_API.OpalMessageType.OpalCmdUserInput:
          m_userInput = new OpalStatusUserInputRef(m_message.m_param.m_userInput);
          break;
        case Opal_API.OpalMessageType.OpalIndMessageWaiting:
          m_messageWaiting = new OpalStatusMessageWaitingRef(m_message.m_param.m_messageWaiting);
          break;
        case Opal_API.OpalMessageType.OpalIndLineAppearance:
          m_lineAppearance = new OpalStatusLineAppearanceRef(m_message.m_param.m_lineAppearance);
          break;
        case Opal_API.OpalMessageType.OpalIndCallCleared:
          m_callCleared = new OpalStatusCallClearedRef(m_message.m_param.m_callCleared);
          break;
        case Opal_API.OpalMessageType.OpalCmdClearCall:
          m_clearCall = new OpalParamCallClearedRef(m_message.m_param.m_clearCall);
          break;
        case Opal_API.OpalMessageType.OpalIndMediaStream:
        case Opal_API.OpalMessageType.OpalCmdMediaStream:
          m_mediaStream = new OpalStatusMediaStreamRef(m_message.m_param.m_mediaStream);
          break;
        case Opal_API.OpalMessageType.OpalCmdSetUserData:
          m_setUserData = new OpalParamSetUserDataRef(m_message.m_param.m_setUserData);
          break;
        case Opal_API.OpalMessageType.OpalCmdStartRecording:
          m_recording = new OpalParamRecordingRef(m_message.m_param.m_recording);
          break;
        case Opal_API.OpalMessageType.OpalIndTransferCall:
          m_transferStatus = new OpalStatusTransferCallRef(m_message.m_param.m_transferStatus);
          break;
        default:
          break;

      }
      m_message.m_type = type;
    }

    /// <summary>
    /// Used by OpalCmdHoldCall/OpalCmdRetrieveCall/OpalCmdStopRecording
    /// </summary>    
    public string GetCallToken()
    {
      switch (m_message.m_type)
      {
        case Opal_API.OpalMessageType.OpalCmdAnswerCall:
        case Opal_API.OpalMessageType.OpalCmdHoldCall:
        case Opal_API.OpalMessageType.OpalCmdRetrieveCall:
        case Opal_API.OpalMessageType.OpalCmdStopRecording:
        case Opal_API.OpalMessageType.OpalCmdAlerting:
          return CallToken;

        case Opal_API.OpalMessageType.OpalCmdSetUpCall:
        case Opal_API.OpalMessageType.OpalIndProceeding:
        case Opal_API.OpalMessageType.OpalIndAlerting:
        case Opal_API.OpalMessageType.OpalIndEstablished:
          return GetCallSetUp().CallToken;

        case Opal_API.OpalMessageType.OpalIndIncomingCall:
          return GetIncomingCall().CallToken;
          
        case Opal_API.OpalMessageType.OpalIndMediaStream:
        case Opal_API.OpalMessageType.OpalCmdMediaStream:
          return GetMediaStream().CallToken;         

        case Opal_API.OpalMessageType.OpalCmdSetUserData:
          return GetSetUserData().CallToken;          

        case Opal_API.OpalMessageType.OpalIndUserInput:
          return GetUserInput().CallToken;          

        case Opal_API.OpalMessageType.OpalCmdStartRecording:
          return GetRecording().CallToken;

        case Opal_API.OpalMessageType.OpalIndCallCleared:
          return GetCallCleared().CallToken;

        case Opal_API.OpalMessageType.OpalCmdClearCall:
          return GetClearCall().CallToken;

        default:
          return null;
      }
    }

    public void SetCallToken(string callToken)
    {
      switch (m_message.m_type)
      {
        case Opal_API.OpalMessageType.OpalCmdAnswerCall:
        case Opal_API.OpalMessageType.OpalCmdHoldCall:
        case Opal_API.OpalMessageType.OpalCmdRetrieveCall:
        case Opal_API.OpalMessageType.OpalCmdStopRecording:
        case Opal_API.OpalMessageType.OpalCmdAlerting:
          CallToken = callToken;
          break;

        case Opal_API.OpalMessageType.OpalCmdSetUpCall:
        case Opal_API.OpalMessageType.OpalIndProceeding:
        case Opal_API.OpalMessageType.OpalIndAlerting:
        case Opal_API.OpalMessageType.OpalIndEstablished:
          GetCallSetUp().CallToken = callToken;
          break;

        case Opal_API.OpalMessageType.OpalIndIncomingCall:
          GetIncomingCall().CallToken = callToken;
          break;

        case Opal_API.OpalMessageType.OpalIndMediaStream:
        case Opal_API.OpalMessageType.OpalCmdMediaStream:
          GetMediaStream().CallToken = callToken;          
          break;

        case Opal_API.OpalMessageType.OpalCmdSetUserData:
          GetSetUserData().CallToken = callToken;          
          break;

        case Opal_API.OpalMessageType.OpalIndUserInput:
          GetUserInput().CallToken = callToken;          
          break;

        case Opal_API.OpalMessageType.OpalCmdStartRecording:
          GetRecording().CallToken = callToken;          
          break;

        case Opal_API.OpalMessageType.OpalIndCallCleared:
          GetCallCleared().CallToken = callToken;          
          break;

        case Opal_API.OpalMessageType.OpalCmdClearCall:
           GetClearCall().CallToken = callToken;
          break;

        default:
          break;
      }
    }

      /// <summary>
      ///  Used by OpalIndCommandError
      /// </summary>    
      public string GetCommandError()
      {
        return m_message.m_type == Opal_API.OpalMessageType.OpalIndCommandError ? CmdError : null;
      }

      /// <summary>
      /// Used by OpalCmdSetGeneralParameters
      /// </summary>      
      public OpalParamGeneralRef GetGeneralParams()
      {
        return m_message.m_type == Opal_API.OpalMessageType.OpalCmdSetGeneralParameters ? m_generalParams : null;
      }

      /// <summary>
      /// Used by OpalCmdSetProtocolParameters
      /// </summary>      
      public OpalParamProtocolRef GetProtocolParams()
      {
        return m_message.m_type == Opal_API.OpalMessageType.OpalCmdSetProtocolParameters ? m_protocolParams : null;
      }
      
      /// <summary>
      /// Used by OpalCmdRegistration
      /// </summary>      
      public OpalParamRegistrationRef GetRegistrationInfo()
      {
        return m_message.m_type == Opal_API.OpalMessageType.OpalCmdRegistration ? m_registrationInfo : null;
      }

      /// <summary>
      /// Used by OpalIndRegistration
      /// </summary>      
      public OpalStatusRegistrationRef GetRegistrationStatus()
      {
        return m_message.m_type == Opal_API.OpalMessageType.OpalIndRegistration ? m_registrationStatus : null;
      }

      /// <summary>
      /// Used by OpalCmdSetUpCall/OpalIndProceeding/OpalIndAlerting/OpalIndEstablished
      /// </summary>      
      public OpalParamSetUpCallRef GetCallSetUp()
      {
        switch (m_message.m_type)
        {
          case Opal_API.OpalMessageType.OpalCmdSetUpCall:
          case Opal_API.OpalMessageType.OpalIndProceeding:
          case Opal_API.OpalMessageType.OpalIndAlerting:
          case Opal_API.OpalMessageType.OpalIndEstablished:
          case Opal_API.OpalMessageType.OpalIndCompletedIVR:
            return m_callSetUp;

          default:
            return null;
        }
      }

      /// <summary>
      /// Used by OpalIndIncomingCall
      /// </summary>     
      public OpalStatusIncomingCallRef GetIncomingCall()
      {
        return m_message.m_type == Opal_API.OpalMessageType.OpalIndIncomingCall ? m_incomingCall : null;
      }

      /// <summary>
      /// Used by OpalCmdAnswerCall/OpalCmdAlerting
      /// </summary>      
      public OpalParamAnswerCallRef GetAnswerCall()
      {
        return m_message.m_type == Opal_API.OpalMessageType.OpalCmdAlerting ||
          m_message.m_type == Opal_API.OpalMessageType.OpalCmdAnswerCall ? m_answerCall : null;
      }

      /// <summary>
      /// Used by OpalIndUserInput/OpalCmdUserInput
      /// </summary>      
      public OpalStatusUserInputRef GetUserInput()
      {
        switch (m_message.m_type)
        {
          case Opal_API.OpalMessageType.OpalIndUserInput:
          case Opal_API.OpalMessageType.OpalCmdUserInput:
            return m_userInput;

          default:
            return null;
        }
      }

      /// <summary>
      /// Used by OpalIndMessageWaiting
      /// </summary>      
      public OpalStatusMessageWaitingRef GetMessageWaiting()
      {
        return m_message.m_type == Opal_API.OpalMessageType.OpalIndMessageWaiting ? m_messageWaiting : null;
      }

      /// <summary>
      /// Used by OpalIndLineAppearance
      /// </summary>      
      public OpalStatusLineAppearanceRef GetLineAppearance()
      {
        return m_message.m_type == Opal_API.OpalMessageType.OpalIndLineAppearance ? m_lineAppearance : null;
      }

      /// <summary>
      /// Used by OpalIndCallCleared
      /// </summary>      
      public OpalStatusCallClearedRef GetCallCleared()
      {
        return m_message.m_type == Opal_API.OpalMessageType.OpalIndCallCleared ? m_callCleared : null;
      }

      /// <summary>
      /// Used by OpalCmdClearCall
      /// </summary>      
      public OpalParamCallClearedRef GetClearCall()
      {
        return m_message.m_type == Opal_API.OpalMessageType.OpalCmdClearCall ? m_clearCall : null;
      }

      /// <summary>
      /// Used by OpalIndMediaStream/OpalCmdMediaStream
      /// </summary>      
      public OpalStatusMediaStreamRef GetMediaStream()
      {
        switch (m_message.m_type)
        {
          case Opal_API.OpalMessageType.OpalIndMediaStream:
          case Opal_API.OpalMessageType.OpalCmdMediaStream:
            return m_mediaStream;

          default:
            return null;
        }
      }

      /// <summary>
      /// Used by OpalCmdSetUserData
      /// </summary>     
      public OpalParamSetUserDataRef GetSetUserData()
      {
        return m_message.m_type == Opal_API.OpalMessageType.OpalCmdSetUserData ? m_setUserData : null;
      } 

      /// <summary>
      /// Used by OpalCmdStartRecording
      /// </summary>      
      public OpalParamRecordingRef GetRecording()
      {
        return m_message.m_type == Opal_API.OpalMessageType.OpalCmdStartRecording ? m_recording : null;
      }

      /// <summary>
      /// Used by OpalIndTransferCall
      /// </summary>      
      public OpalStatusTransferCallRef GetTransferStatus()
      {
        return m_message.m_type == Opal_API.OpalMessageType.OpalIndTransferCall ? m_transferStatus : null;
      }      
      
  }

  public class OpalParamGeneralRef
  {
    private Opal_API.OpalParamGeneral m_opalParamGeneral;

    /// <summary>
    /// Function for reading/writing media data.
    ///Returns size of data actually read or written, or -1 if there is an error
    ///and the media stream should be shut down.
    ///
    ///The "write" function, which is taking data from a remote and providing it
    ///to the "C" application for writing, should not be assumed to have a one to
    ///one correspondence with RTP packets. The OPAL jiter buffer may insert
    ///"silence" data for missing or too late packets. In this case the function
    ///is called with the size parameter equal to zero. It is up to the
    ///application what it does in that circumstance.
    ///
    ///Note that this function will be called in the context of different threads
    ///so the user must take care of any mutex and synchonisation issues.
    /// </summary>       
    public delegate int OpalMediaDataFunction([MarshalAs(UnmanagedType.LPStr)] string token,
        [MarshalAs(UnmanagedType.LPStr)] string stream, [MarshalAs(UnmanagedType.LPStr)] string format, IntPtr userData, IntPtr data, int size);

    /// <summary>
    /// Function called when a message event becomes available.
    /// This function is called before the message is queued for the GetMessage()
    /// function.
    ///
    /// A return value of zero indicates that the message is not to be passed on
    /// to the GetMessage(). A non-zero value will pass the message on.
    ///
    /// Note that this function will be called in the context of different threads
    /// so the user must take care of any mutex and synchonisation issues. If the
    /// user subsequently uses the GetMessage() then the message will have been
    /// serialised so that there are no multi-threading issues.
    ///
    /// A simple use case would be for this function to send a signal or message
    /// to the applications main thread and then return a non-zero value. The
    /// main thread would then wake up and get the message using GetMessage.
    /// </summary>     
    public delegate int OpalMessageAvailableFunction([MarshalAs(UnmanagedType.LPStruct)] Opal_API.OpalMessage message); /**< Message that has become available. */   

    public OpalParamGeneralRef(Opal_API.OpalParamGeneral paramGeneral)
    {
      m_opalParamGeneral = paramGeneral;
    }

    public Opal_API.OpalParamGeneral OpalParamGeneral
    {
      get { return m_opalParamGeneral; }      
    }

    /// <summary>
    /// Audio recording device name
    /// </summary>
    public string AudioRecordDevice
    {
      get { return Marshal.PtrToStringAnsi(m_opalParamGeneral.m_audioRecordDevice); }
      set { m_opalParamGeneral.m_audioRecordDevice = Marshal.StringToCoTaskMemAnsi(value); }
    }        
   
    /// <summary>
    /// Audio playback device name
    /// </summary>    
    public string AudioPlayerDevice
    {
      get { return Marshal.PtrToStringAnsi(m_opalParamGeneral.m_audioPlayerDevice); }
      set { m_opalParamGeneral.m_audioPlayerDevice = Marshal.StringToCoTaskMemAnsi(value); }
    }
   
    /// <summary>
    /// Video input (e.g. camera) device name
    /// </summary>    
    public string VideoInputDevice
    {
      get { return Marshal.PtrToStringAnsi(m_opalParamGeneral.m_videoInputDevice); }
      set { m_opalParamGeneral.m_videoInputDevice = Marshal.StringToCoTaskMemAnsi(value); }
    }    
   
    /// <summary>
    /// Video output (e.g. window) device name
    /// </summary>    
    public string VideoOutputDevice
    {
      get { return Marshal.PtrToStringAnsi(m_opalParamGeneral.m_videoOutputDevice); }
      set { m_opalParamGeneral.m_videoOutputDevice = Marshal.StringToCoTaskMemAnsi(value); }
    }
        
    /// <summary>
    /// Video preview (e.g. window) device name
    /// </summary>   
    public string VideoPreviewDevice
    {
      get { return Marshal.PtrToStringAnsi(m_opalParamGeneral.m_videoPreviewDevice); }
      set { m_opalParamGeneral.m_videoPreviewDevice = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// List of media format names to set the preference order for media.
    /// This list of names (e.g. "G.723.1") is separated by the '\n'/
    /// * character.
    /// </summary>  
    public string MediaOrder
    {
      get { return Marshal.PtrToStringAnsi(m_opalParamGeneral.m_mediaOrder); }
      set { m_opalParamGeneral.m_mediaOrder = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// List of media format names to set media to be excluded.
    /// This list of names (e.g. "G.723.1") is separated by the '\n'
    /// character.
    /// </summary>    
    public string MediaMask
    {
      get { return Marshal.PtrToStringAnsi(m_opalParamGeneral.m_mediaMask); }
      set { m_opalParamGeneral.m_mediaMask = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    ///  List of media types (e.g. audio, video) separated by spaces
    ///  which may automatically be received automatically. If NULL
    ///  no change is made, but if "" then all media is prevented from
    ///  auto-starting.
    /// </summary>    
    public string AutoRxMedia
    {
      get { return Marshal.PtrToStringAnsi(m_opalParamGeneral.m_autoRxMedia); }
      set { m_opalParamGeneral.m_autoRxMedia = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// List of media types (e.g. audio, video) separated by spaces
    /// which may automatically be transmitted automatically. If NULL
    /// no change is made, but if "" then all media is prevented from
    /// auto-starting.
    /// </summary>
    public string AutoTxMedia
    {
      get { return Marshal.PtrToStringAnsi(m_opalParamGeneral.m_autoTxMedia); }
      set { m_opalParamGeneral.m_autoTxMedia = Marshal.StringToCoTaskMemAnsi(value); }
    }
    
    /// <summary>
    /// The host name or IP address of the Network Address Translation
    /// router which may be between the endpoint and the Internet.
    /// </summary>    
    public string NatRouter
    {
      get { return Marshal.PtrToStringAnsi(m_opalParamGeneral.m_natRouter); }
      set { m_opalParamGeneral.m_natRouter = Marshal.StringToCoTaskMemAnsi(value); }
    }
    
    /// <summary>
    /// The host name or IP address of the STUN server which may be
    /// used to determine the NAT router characteristics automatically.
    /// </summary>
    public string StunServer
    {
      get { return Marshal.PtrToStringAnsi(m_opalParamGeneral.m_stunServer); }
      set { m_opalParamGeneral.m_stunServer = Marshal.StringToCoTaskMemAnsi(value); }
    }
   
    /// <summary>
    /// Base of range of ports to use for TCP communications. This may
    /// be required by some firewalls.
    /// </summary>
    public uint TcpPortBase
    {
      get { return m_opalParamGeneral.m_tcpPortBase; }
      set { m_opalParamGeneral.m_tcpPortBase = value; }
    }
   
    /// <summary>
    /// Max of range of ports to use for TCP communications. This may
    /// be required by some firewalls.
    /// </summary>
    public uint TcpPortMax
    {
      get { return m_opalParamGeneral.m_tcpPortMax; }
      set { m_opalParamGeneral.m_tcpPortMax = value; }
    }

    /// <summary>
    /// Base of range of ports to use for UDP communications. This may
    /// be required by some firewalls.
    /// </summary>
    public uint UdpPortBase
    {
      get { return m_opalParamGeneral.m_udpPortBase; }
      set { m_opalParamGeneral.m_udpPortBase = value; }
    }

    /// <summary>
    ///  Max of range of ports to use for UDP communications. This may
    ///  be required by some firewalls.
    /// </summary>
    public uint UdpPortMax
    {
      get { return m_opalParamGeneral.m_udpPortMax; }
      set { m_opalParamGeneral.m_udpPortMax = value; }
    }
    
    /// <summary>
    /// Base of range of ports to use for RTP/UDP communications. This may
    /// be required by some firewalls.
    /// </summary>
    public uint RtpPortBase
    {
      get { return m_opalParamGeneral.m_rtpPortBase; }
      set { m_opalParamGeneral.m_rtpPortBase = value; }
    }

    /// <summary>
    /// Max of range of ports to use for RTP/UDP communications. This may
    /// be required by some firewalls.
    /// </summary>
    public uint RtpPortMax
    {
      get { return m_opalParamGeneral.m_rtpPortMax; }
      set { m_opalParamGeneral.m_rtpPortMax = value; }
    }

    /// <summary>
    /// Value for the Type Of Service byte with UDP/IP packets which may
    /// be used by some routers for simple Quality of Service control.
    /// </summary>
    public uint RtpTypeOfService
    {
      get { return m_opalParamGeneral.m_rtpTypeOfService; }
      set { m_opalParamGeneral.m_rtpTypeOfService = value; }
    }

    /// <summary>
    /// Maximum payload size for RTP packets. This may sometimes need to
    /// be set according to the MTU or the underlying network. 
    /// </summary>
    public uint RtpMaxPayloadSize
    {
      get { return m_opalParamGeneral.m_rtpMaxPayloadSize; }
      set { m_opalParamGeneral.m_rtpMaxPayloadSize = value; }
    }

    /// <summary>
    /// Minimum jitter time in milliseconds. For audio RTP data being
    /// received this sets the minimum time of the adaptive jitter buffer
    /// which smooths out irregularities in the transmission of audio
    /// data over the Internet.
    /// </summary>
    public uint MinAudioJitter
    {
      get { return m_opalParamGeneral.m_minAudioJitter; }
      set { m_opalParamGeneral.m_minAudioJitter = value; }
    }

    /// <summary>
    /// Maximum jitter time in milliseconds. For audio RTP data being
    /// received this sets the maximum time of the adaptive jitter buffer
    /// which smooths out irregularities in the transmission of audio
    /// data over the Internet.
    /// </summary>
    public uint MaxAudioJitter
    {
      get { return m_opalParamGeneral.m_maxAudioJitter; }
      set { m_opalParamGeneral.m_maxAudioJitter = value; }
    }

    /// <summary>
    /// Silence detection mode. This controls the silence
    /// detection algorithm for audio transmission: 0=no change,
    /// 1=disabled, 2=fixed, 3=adaptive.
    /// </summary>
    public Opal_API.OpalSilenceDetectMode SilenceDetectMode
    {
      get { return m_opalParamGeneral.m_silenceDetectMode; }
      set { m_opalParamGeneral.m_silenceDetectMode = value; }
    }

    /// <summary>
    /// Silence detection threshold value. This applies if
    /// m_silenceDetectMode is fixed (2) and is a PCM-16 value.
    /// </summary>
    public uint SilenceThreshold
    {
      get { return m_opalParamGeneral.m_silenceThreshold; }
      set { m_opalParamGeneral.m_silenceThreshold = value; }
    }

    /// <summary>
    /// Time signal is required before audio is transmitted. This is
    /// is RTP timestamp units (8000Hz).
    /// </summary>
    public uint SignalDeadband
    {
      get { return m_opalParamGeneral.m_signalDeadband; }
      set { m_opalParamGeneral.m_signalDeadband = value; }
    }

    /// <summary>
    /// Time silence is required before audio is transmission is stopped.
    /// This is is RTP timestamp units (8000Hz).
    /// </summary>
    public uint SilenceDeadband
    {
      get { return m_opalParamGeneral.m_silenceDeadband; }
      set { m_opalParamGeneral.m_silenceDeadband = value; }
    }

    /// <summary>
    /// Window for adapting the silence threashold. This applies if
    /// m_silenceDetectMode is adaptive (3). This is is RTP timestamp
    /// units (8000Hz).
    /// </summary>
    public uint SilenceAdaptPeriod
    {
      get { return m_opalParamGeneral.m_silenceAdaptPeriod; }
      set { m_opalParamGeneral.m_silenceAdaptPeriod = value; }
    }

    /// <summary>
    /// Accoustic Echo Cancellation control. 0=no change, 1=disabled,
    /// 2=enabled.
    /// </summary>
    public Opal_API.OpalEchoCancelMode EchoCancellation
    {
      get { return m_opalParamGeneral.m_echoCancellation; }
      set { m_opalParamGeneral.m_echoCancellation = value; }
    }

    /// <summary>
    /// Set the number of hardware sound buffers to use.
    /// Note the largest of m_audioBuffers and m_audioBufferTime/frametime
    /// will be used.
    /// </summary>
    public uint AudioBuffers
    {
      get { return m_opalParamGeneral.m_audioBuffers; }
      set { m_opalParamGeneral.m_audioBuffers = value; }
    }

    /// <summary>
    /// Callback function for reading raw media data. See
    /// OpalMediaDataFunction for more information.
    /// </summary>
    public OpalMediaDataFunction MediaReadData
    {
      get { return (OpalMediaDataFunction)Marshal.GetDelegateForFunctionPointer(m_opalParamGeneral.m_mediaReadData, typeof(OpalMediaDataFunction)); }
      set { m_opalParamGeneral.m_mediaReadData = Marshal.GetFunctionPointerForDelegate(value); }
    }

    /// <summary>
    /// Callback function for writing raw media data. See
    /// OpalMediaDataFunction for more information.
    /// </summary>
    public OpalMediaDataFunction MediaWriteData
    {
      get { return (OpalMediaDataFunction)Marshal.GetDelegateForFunctionPointer(m_opalParamGeneral.m_mediaWriteData, typeof(OpalMediaDataFunction)); }
      set { m_opalParamGeneral.m_mediaWriteData = Marshal.GetFunctionPointerForDelegate(value); }
    }

    /// <summary>
    /// Indicate that the media read/write callback function
    /// is passed the full RTP header or just the payload.
    /// 0=no change, 1=payload only, 2=with RTP header.
    /// </summary>
    public uint MediaDataHeader
    {
      get { return m_opalParamGeneral.m_mediaDataHeader; }
      set { m_opalParamGeneral.m_mediaDataHeader = value; }
    }

    /// <summary>
    /// If non-null then this function is called before
    /// the message is queued for return in the
    /// GetMessage(). See the
    /// OpalMessageAvailableFunction for more details.
    /// </summary>
    public OpalMessageAvailableFunction MessageAvailable
    {
      get { return (OpalMessageAvailableFunction)Marshal.GetDelegateForFunctionPointer(m_opalParamGeneral.m_messageAvailable, typeof(OpalMessageAvailableFunction)); }
      set { m_opalParamGeneral.m_messageAvailable = Marshal.GetFunctionPointerForDelegate(value); }
    }

    /// <summary>
    /// List of media format options to be set. This is a '\n' separated
    /// list of entries of the form "codec:option=value". Codec is either
    /// a media type (e.g. "Audio" or "Video") or a specific media format,
    /// for example:
    /// <code>
    /// "G.723.1:Tx Frames Per Packet=2\nH.263:Annex T=0\n"
    /// "Video:Max Rx Frame Width=176\nVideo:Max Rx Frame Height=144"
    /// </code>
    /// </summary>
    public string MediaOptions
    {
      get { return Marshal.PtrToStringAnsi(m_opalParamGeneral.m_mediaOptions); }
      set { m_opalParamGeneral.m_mediaOptions = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Set the hardware sound buffers to use in milliseconds.
    /// Note the largest of m_audioBuffers and m_audioBufferTime/frametime
    /// will be used.
    /// </summary>
    public uint AudioBufferTime
    {
      get { return m_opalParamGeneral.m_audioBufferTime; }
      set { m_opalParamGeneral.m_audioBufferTime = value; }
    }

    /// <summary>
    /// Indicate that an "alerting" message is automatically (value=1)
    /// or manually (value=2) sent to the remote on receipt of an
    /// OpalIndIncomingCall message. If set to manual then it is up
    /// to the application to send a OpalCmdAlerting message to
    /// indicate to the remote system that we are "ringing".
    /// If zero then no change is made.
    /// </summary>
    public uint ManualAlerting
    {
      get { return m_opalParamGeneral.m_manualAlerting; }
      set { m_opalParamGeneral.m_manualAlerting = value; }
    }

    /// <summary>
    /// Indicate how the media read/write callback function
    /// handles the real time aspects of the media flow.
    /// 0=no change, 1=synchronous, 2=asynchronous,
    /// 3=simulate synchronous.
    /// </summary>
    public Opal_API.OpalMediaTiming MediaTiming
    {
      get { return m_opalParamGeneral.m_mediaTiming; }
      set { m_opalParamGeneral.m_mediaTiming = value; }
    }

    /// <summary>
    /// Indicate that the video read callback function
    /// handles the real time aspects of the media flow.
    /// This can override the m_mediaTiming.
    /// </summary>
    public Opal_API.OpalMediaTiming VideoSourceTiming
    {
      get { return m_opalParamGeneral.m_videoSourceTiming; }
      set { m_opalParamGeneral.m_videoSourceTiming = value; }
    }

  }

  /// <summary>
  /// Protocol parameters for the OpalCmdSetProtocolParameters command.
  ///   This is only passed to and returned from the OpalSendMessage() function.
  ///
  ///   Example:
  ///      <code>
  ///      OpalMessage   command;
  ///      OpalMessage * response;
  ///
  ///      memset(&command, 0, sizeof(command));
  ///      command.m_type = OpalCmdSetProtocolParameters;
  ///      command.m_param.m_protocol.m_userName = "robertj";
  ///      command.m_param.m_protocol.m_displayName = "Robert Jongbloed";
  ///      command.m_param.m_protocol.m_interfaceAddresses = "*";
  ///      response = OpalSendMessage(hOPAL, &command);
  ///      </code>
  /// </summary>
  public class OpalParamProtocolRef
  {
    private Opal_API.OpalParamProtocol m_param;

    public OpalParamProtocolRef(Opal_API.OpalParamProtocol paramProtocol)
    {
      m_param = paramProtocol;
    }

    public Opal_API.OpalParamProtocol Param
    {
      get { return m_param; }      
    }    
    
    /// <summary>
    /// Protocol prefix for parameters, e.g. "h323" or "sip". If this is
    /// NULL or empty string, then the parameters are set for all protocols
    /// where they maybe set.
    /// </summary>
    public string Prefix
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_prefix); }
      set { m_param.m_prefix = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// User name to identify the endpoint. This is usually the protocol
    /// specific name and may interact with the OpalCmdRegistration
    /// command. e.g. "robertj" or 61295552148
    /// </summary>
    public string UserName
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_userName); }
      set { m_param.m_userName = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Display name to be used. This is the human readable form of the
    /// users name, e.g. "Robert Jongbloed".
    /// </summary>
    public string DisplayName
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_displayName); }
      set { m_param.m_displayName = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Product description data
    /// </summary>
    public Opal_API.OpalProductDescription Product
    {
      get { return m_param.m_product; }
      set { m_param.m_product = value; }
    }

    /// <summary>
    /// A list of interfaces to start listening for incoming calls.
    /// This list is separated by the '\n' character. If NULL no
    /// listeners are started or stopped. If and empty string ("")
    /// then all listeners are stopped. If a "*" then listeners
    /// are started for all interfaces in the system.
    /// 
    ///  If the prefix is "ivr", then this is the default VXML script
    ///  or URL to execute on incoming calls.
    /// </summary>
    public string InterfaceAddresses
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_interfaceAddresses); }
      set { m_param.m_interfaceAddresses = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// The mode for user input transmission. Note this only applies if an
    /// explicit protocol is indicated in m_prefix. See OpalUserInputModes
    /// for more information.
    /// </summary>
    public Opal_API.OpalUserInputModes UserInputMode
    {
      get { return m_param.m_userInputMode; }
      set { m_param.m_userInputMode = value; }
    }

    /// <summary>
    /// Default options for new calls using the specified protocol. This
    /// string is of the form key=value\nkey=value
    /// </summary>
    public string DefaultOptions
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_defaultOptions); }
      set { m_param.m_defaultOptions = Marshal.StringToCoTaskMemAnsi(value); }
    }
  }

  /// <summary>
  /// Registration parameters for the OpalCmdRegistration command.
  ///   This is only passed to and returned from the OpalSendMessage() function.
  ///
  ///   Example:
  ///      <code>
  ///      OpalMessage   command;
  ///      OpalMessage * response;
  ///
  ///      // H.323 register with gatekeeper
  ///      memset(&command, 0, sizeof(command));
  ///      command.m_type = OpalCmdRegistration;
  ///      command.m_param.m_registrationInfo.m_protocol = "h323";
  ///      command.m_param.m_registrationInfo.m_identifier = "31415";
  ///      command.m_param.m_registrationInfo.m_hostName = gk.voxgratia.org;
  ///      command.m_param.m_registrationInfo.m_password = "secret";
  ///      command.m_param.m_registrationInfo.m_timeToLive = 300;
  ///      response = OpalSendMessage(hOPAL, &command);
  ///      if (response != NULL && response->m_type == OpalCmdRegistration)
  ///        m_AddressOfRecord = response->m_param.m_registrationInfo.m_identifier
  ///
  ///      // SIP register with regstrar/proxy
  ///      memset(&command, 0, sizeof(command));
  ///      command.m_type = OpalCmdRegistration;
  ///      command.m_param.m_registrationInfo.m_protocol = "sip";
  ///      command.m_param.m_registrationInfo.m_identifier = "rjongbloed@ekiga.net";
  ///      command.m_param.m_registrationInfo.m_password = "secret";
  ///      command.m_param.m_registrationInfo.m_timeToLive = 300;
  ///      response = OpalSendMessage(hOPAL, &command);
  ///      if (response != NULL && response->m_type == OpalCmdRegistration)
  ///        m_AddressOfRecord = response->m_param.m_registrationInfo.m_identifier
  ///
  ///      // unREGISTER
  ///      memset(&command, 0, sizeof(command));
  ///      command.m_type = OpalCmdRegistration;
  ///      command.m_param.m_registrationInfo.m_protocol = "sip";
  ///      command.m_param.m_registrationInfo.m_identifier = m_AddressOfRecord;
  ///      command.m_param.m_registrationInfo.m_timeToLive = 0;
  ///      response = OpalSendMessage(hOPAL, &command);
  ///
  ///      // Set event package so do SUBSCRIBE
  ///      memset(&command, 0, sizeof(command));
  ///      command.m_type = OpalCmdRegistration;
  ///      command.m_param.m_registrationInfo.m_protocol = "sip";
  ///      command.m_param.m_registrationInfo.m_identifier = "2012@pbx.local";
  ///      command.m_param.m_registrationInfo.m_hostName = "sa@pbx.local";
  ///      command.m_param.m_registrationInfo.m_eventPackage = OPAL_LINE_APPEARANCE_EVENT_PACKAGE;
  ///      command.m_param.m_registrationInfo.m_timeToLive = 300;
  ///      response = OpalSendMessage(hOPAL, &command);
  ///      if (response != NULL && response->m_type == OpalCmdRegistration)
  ///        m_AddressOfRecord = response->m_param.m_registrationInfo.m_identifier
  ///
  ///      // unSUBSCRIBE
  ///      memset(&command, 0, sizeof(command));
  ///      command.m_type = OpalCmdRegistration;
  ///      command.m_param.m_registrationInfo.m_protocol = "sip";
  ///      command.m_param.m_registrationInfo.m_identifier = m_AddressOfRecord;
  ///      command.m_param.m_registrationInfo.m_eventPackage = OPAL_LINE_APPEARANCE_EVENT_PACKAGE;
  ///      command.m_param.m_registrationInfo.m_timeToLive = 0;
  ///      response = OpalSendMessage(hOPAL, &command);
  ///      </code>
  /// </summary>
  public class OpalParamRegistrationRef
  {
    Opal_API.OpalParamRegistration m_param;

    public OpalParamRegistrationRef(Opal_API.OpalParamRegistration paramRegistration)
    {
       m_param = paramRegistration;
    }

    public Opal_API.OpalParamRegistration Param
    {
      get { return m_param; }      
    }

    /// <summary>
    /// Protocol prefix for registration. Currently must be "h323" or
    /// "sip", cannot be NULL.
    /// </summary>
    public string Protocol
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_protocol); }
      set { m_param.m_protocol = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Identifier for name to be registered at server. If NULL
    /// or empty then the value provided in the OpalParamProtocol::m_userName
    /// field of the OpalCmdSetProtocolParameters command is used. Note
    /// that for SIP the default value will have "@" and the
    /// OpalParamRegistration::m_hostName field apepnded to it to create
    /// and Address-Of_Record.
    /// </summary>
    public string Identifier
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_identifier); }
      set { m_param.m_identifier = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    ///  Host or domain name for server. For SIP this cannot be NULL.
    ///  For H.323 a NULL value indicates that a broadcast discovery is
    ///  be performed. If, for SIP, this contains an "@" and a user part
    ///  then a "third party" registration is performed.
    /// </summary>
    public string HostName
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_hostName); }
      set { m_param.m_hostName = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    ///  User name for authentication. 
    /// </summary>
    public string AuthUserName
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_authUserName); }
      set { m_param.m_authUserName = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Password for authentication with server.
    /// </summary>
    public string Password
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_password); }
      set { m_param.m_password = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Identification of the administrative entity. For H.323 this will
    /// by the gatekeeper identifier. For SIP this is the authentication
    /// realm.
    /// </summary>
    public string AdminEntity
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_adminEntity); }
      set { m_param.m_adminEntity = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Time in seconds between registration updates. If this is zero then
    /// the identifier is unregistered from the server.
    /// </summary>
    public uint TimeToLive
    {
      get { return m_param.m_timeToLive; }
      set { m_param.m_timeToLive = value; }
    }

    /// <summary>
    /// Time in seconds between attempts to restore a registration after
    /// registrar/gatekeeper has gone offline. If zero then a default
    /// value is used. 
    /// </summary>
    public uint RestoreTime
    {
      get { return m_param.m_restoreTime; }
      set { m_param.m_restoreTime = value; }
    }

    /// <summary>
    /// If non-NULL then this indicates that a subscription is made
    /// rather than a registration. The string represents the particular
    /// event package being subscribed too.
    /// A value of OPAL_MWI_EVENT_PACKAGE will cause an
    /// OpalIndMessageWaiting to be sent.
    /// A value of OPAL_LINE_APPEARANCE_EVENT_PACKAGE will cause the
    /// OpalIndLineAppearance to be sent.
    /// Other values are currently not supported.
    /// </summary>
    public string EventPackage
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_eventPackage); }
      set { m_param.m_eventPackage = Marshal.StringToCoTaskMemAnsi(value); }
    }
  }

  /// <summary>
  /// Registration status for the OpalIndRegistration indication.
  ///   This is only returned from the OpalGetMessage() function.
  /// </summary>
  public class OpalStatusRegistrationRef
  {
    private Opal_API.OpalStatusRegistration m_param;

    public OpalStatusRegistrationRef(Opal_API.OpalStatusRegistration statusRegistration)
    {
      m_param = statusRegistration;
    }

    public Opal_API.OpalStatusRegistration Param
    {
      get { return m_param; }      
    }

    /// <summary>
    /// Protocol prefix for registration. Currently must be "h323" or
    /// "sip", is never NULL.
    /// </summary>
    public string Protocol
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_protocol); }
      set { m_param.m_protocol = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Name of the registration server. The exact format is protocol
    /// specific but generally contains the host or domain name, e.g.
    /// "GkId@gatekeeper.voxgratia.org" or "sip.voxgratia.org".
    /// </summary>
    public string ServerName
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_serverName); }
      set { m_param.m_serverName = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Error message for registration. If any error in the initial
    /// registration or any subsequent registration update occurs, then
    /// this contains a string indicating the type of error. If no
    /// error occured then this will be NULL.
    /// </summary>
    public string Error
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_error); }
      set { m_param.m_error = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Status of registration, see enum for details. 
    /// </summary>
    public Opal_API.OpalRegistrationStates Status
    {
      get { return m_param.m_status; }
      set { m_param.m_status = value; }
    }

    /// <summary>
    /// Product description data 
    /// </summary>
    public Opal_API.OpalProductDescription Product
    {
      get { return m_param.m_product; }
      set { m_param.m_product = value; }
    }
  }

  /// <summary>
  /// Set up call parameters for several command and indication messages.
  ///
  ///   When establishing a new call via the OpalCmdSetUpCall command, the m_partyA and
  ///   m_partyB fields indicate the parties to connect.
  ///
  ///   For OpalCmdTransferCall, m_partyA indicates the connection to be transferred and
  ///   m_partyB is the party to be transferred to. If the call transfer is successful then
  ///   a OpalIndCallCleared message will be received clearing the local call.
  ///
  ///   For OpalIndAlerting and OpalIndEstablished indications the three fields are set
  ///   to the data for the call in progress.
  ///
  ///   Example:
  ///   <code>
  ///      OpalMessage   command;
  ///      OpalMessage * response;
  ///
  ///      memset(&command, 0, sizeof(command));
  ///      command.m_type = OpalCmdSetUpCall;
  ///      command.m_param.m_callSetUp.m_partyB = "h323:10.0.1.11";
  ///      response = OpalSendMessage(hOPAL, &command);
  ///
  ///      memset(&command, 0, sizeof(command));
  ///      command.m_type = OpalCmdSetUpCall;
  ///      command.m_param.m_callSetUp.m_partyA = "pots:LINE1";
  ///      command.m_param.m_callSetUp.m_partyB = "sip:10.0.1.11";
  ///      response = OpalSendMessage(hOPAL, &command);
  ///      callToken = strdup(response->m_param.m_callSetUp.m_callToken);
  ///
  ///      memset(&command, 0, sizeof(command));
  ///      command.m_type = OpalCmdTransferCall;
  ///      command.m_param.m_callSetUp.m_callToken = callToken;
  ///      command.m_param.m_callSetUp.m_partyB = "sip:10.0.1.12";
  ///      response = OpalSendMessage(hOPAL, &command);
  ///      </code>
  /// </summary>  
  public class OpalParamSetUpCallRef
  {
    private Opal_API.OpalParamSetUpCall m_param;

    public OpalParamSetUpCallRef(Opal_API.OpalParamSetUpCall paramSetupCall)
    {
      m_param = paramSetupCall;
    }

    public Opal_API.OpalParamSetUpCall Param
    {
      get { return m_param; }      
    }

    /// <summary>
    /// A-Party for call.
    /// For OpalCmdSetUpCall, this indicates what subsystem will
    /// be starting the call, e.g. "pots:Handset One". If NULL
    /// or empty string then "pc:*" is used indication that the
    /// standard PC sound system ans screen is to be used.
    ///
    /// For OpalCmdTransferCall this indicates the party to be
    /// transferred, e.g. "sip:fred@nurk.com". If NULL then
    /// it is assumed that the party to be transferred is of the
    /// same "protocol" as the m_partyB field, e.g. "pc" or "sip".
    /// For OpalIndAlerting and OpalIndEstablished this indicates
    /// the A-party of the call in progress.
    /// </summary>
    public string PartyA
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_partyA); }
      set { m_param.m_partyA = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// B-Party for call. This is typically a remote host URL
    /// address with protocol, e.g. "h323:simple.com" or
    /// "sip:fred@nurk.com".
    /// This must be provided in the OpalCmdSetUpCall and
    /// OpalCmdTransferCall commands, and is set by the system
    /// in the OpalIndAlerting and OpalIndEstablished indications.
    /// If used in the OpalCmdTransferCall command, this may be
    /// a valid call token for another call on hold. The remote
    /// is transferred to the call on hold and both calls are
    /// then cleared.
    /// </summary>
    public string PartyB
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_partyB); }
      set { m_param.m_partyB = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Value of call token for new call. The user would pass NULL
    /// for this string in OpalCmdSetUpCall, a new value is
    /// returned by the OpalSendMessage() function. The user would
    /// provide the call token for the call being transferred when
    /// OpalCmdTransferCall is being called.
    /// </summary>
    public string CallToken
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_callToken); }
      set { m_param.m_callToken = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// The type of "distinctive ringing" for the call. The string
    /// is protocol dependent, so the caller would need to be aware
    /// of the type of call being made. Some protocols may ignore
    /// the field completely.
    ///
    /// For SIP this corresponds to the string contained in the
    /// "Alert-Info" header field of the INVITE. This is typically
    /// a URI for the ring file.
    /// 
    /// For H.323 this must be a string representation of an
    /// integer from 0 to 7 which will be contained in the
    /// Q.931 SIGNAL (0x34) Information Element.
    ///
    /// This is only used in OpalCmdSetUpCall to set the string to
    /// be sent to the remote to change the type of ring the remote
    /// may emit.
    ///
    /// For other indications this field is NULL.
    /// </summary>
    public string AlertingType
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_alertingType); }
      set { m_param.m_alertingType = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    ///  ID assigned by the underlying protocol for the call. 
    ///  Only available in version 18 and above
    /// </summary>
    public string ProtocolCallId
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_protocolCallId); }
      set { m_param.m_protocolCallId = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Overrides for the default parameters for the protocol.
    /// For example, m_userName and m_displayName can be
    /// changed on a call by call basis.
    /// </summary>
    public Opal_API.OpalParamProtocol Overrides
    {
      get { return m_param.m_overrides; }
      set { m_param.m_overrides = value; }
    }
  }

  /// <summary>
  /// Incoming call information for the OpalIndIncomingCall indication.
  ///   This is only returned from the OpalGetMessage() function.
  /// </summary>
  public class OpalStatusIncomingCallRef
  {
    private Opal_API.OpalStatusIncomingCall m_param;
        
    public OpalStatusIncomingCallRef(Opal_API.OpalStatusIncomingCall statusIncomingCall)
    {
      m_param = statusIncomingCall;
    }

    public Opal_API.OpalStatusIncomingCall Param
    {
      get { return m_param; }      
    }

    /// <summary>
    /// Call token for new call.
    /// </summary>
    public string CallToken
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_callToken); }
      set { m_param.m_callToken = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// URL of local interface. e.g. "sip:me@here.com"
    /// </summary>
    public string LocalAddress
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_localAddress); }
      set { m_param.m_localAddress = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// URL of calling party. e.g. "sip:them@there.com"
    /// </summary>
    public string RemoteAddress
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_remoteAddress); }
      set { m_param.m_remoteAddress = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// This is the E.164 number of the caller, if available.
    /// </summary>
    public string RemotePartyNumber
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_remotePartyNumber); }
      set { m_param.m_remotePartyNumber = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    ///  Display name calling party. e.g. "Fred Nurk"
    /// </summary>
    public string RemoteDisplayName
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_remoteDisplayName); }
      set { m_param.m_remoteDisplayName = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// URL of called party the remote is trying to contact.
    /// </summary>
    public string CalledAddress
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_calledAddress); }
      set { m_param.m_calledAddress = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// This is the E.164 number of the called party, if available.
    /// </summary>
    public string CalledPartyNumber
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_calledPartyNumber); }
      set { m_param.m_calledPartyNumber = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Product description data
    /// </summary>
    public Opal_API.OpalProductDescription Product
    {
      get { return m_param.m_product; }
      set { m_param.m_product = value; }
    }

    /// <summary>
    /// The type of "distinctive ringing" for the call. The string
    /// is protocol dependent, so the caller would need to be aware
    /// of the type of call being made. Some protocols may ignore
    /// the field completely.
    ///
    /// For SIP this corresponds to the string contained in the
    /// "Alert-Info" header field of the INVITE. This is typically
    /// a URI for the ring file.
    ///
    /// For H.323 this must be a string representation of an
    /// integer from 0 to 7 which will be contained in the
    /// Q.931 SIGNAL (0x34) Information Element.
    /// </summary>
    public string AlertingType
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_alertingType); }
      set { m_param.m_alertingType = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// ID assigned by the underlying protocol for the call. 
    /// Only available in version 18 and above
    /// </summary>
    public string ProtocolCallId
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_protocolCallId); }
      set { m_param.m_protocolCallId = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// This is the full address of the party doing transfer, if available.
    /// </summary>
    public string ReferredByAddress
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_referredByAddress); }
      set { m_param.m_referredByAddress = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// This is the E.164 number of the party doing transfer, if available.
    /// </summary>
    public string RedirectingNumber
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_redirectingNumber); }
      set { m_param.m_redirectingNumber = Marshal.StringToCoTaskMemAnsi(value); }
    }
  }

  /// <summary>
  /// Incoming call response parameters for OpalCmdAlerting and OpalCmdAnswerCall messages.
  ///
  ///   When a new call is detected via the OpalIndIncomingCall indication, the application
  ///   should respond with OpalCmdClearCall, which does not use this structure, or
  ///   OpalCmdAnswerCall, which does. An optional OpalCmdAlerting may also be sent
  ///   which also uses this structure to allow for the override of default call parameters
  ///   such as suer name or display name on a call by call basis.
  /// </summary>  
  public class OpalParamAnswerCallRef
  {
    Opal_API.OpalParamAnswerCall m_param;
    
    public OpalParamAnswerCallRef(Opal_API.OpalParamAnswerCall paramAnswerCall)
    {
      m_param = paramAnswerCall;
    }

    public Opal_API.OpalParamAnswerCall Param
    {
      get { return m_param; }      
    }


    /// <summary>
    /// Call token for call to be answered.
    /// </summary>
    public string CallToken
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_callToken); }
      set { m_param.m_callToken = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Overrides for the default parameters for the protocol.
    /// For example, m_userName and m_displayName can be
    /// changed on a call by call basis.
    /// </summary>   
    public OpalDotNET.Opal_API.OpalParamProtocol Overrides
    {
      get { return m_param.m_overrides; }
      set { m_param.m_overrides = value; }
    }
  }

  /// <summary>
  /// User input information for the OpalIndUserInput/OpalCmdUserInput indication.
  ///
  /// This is may be returned from the OpalGetMessage() function or
  /// provided to the OpalSendMessage() function.
  /// </summary>
  public class OpalStatusUserInputRef
  {
    private Opal_API.OpalStatusUserInput m_param;    

    public OpalStatusUserInputRef(Opal_API.OpalStatusUserInput statusUserInput)
    {
      m_param = statusUserInput;
    }

    public Opal_API.OpalStatusUserInput Param
    {
      get { return m_param; }      
    }

    /// <summary>
    /// Call token for the call the User Input was received on.
    /// </summary>
    public string CallToken
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_callToken); }
      set { m_param.m_callToken = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// User input string, e.g. "#".
    /// </summary>
    public string UserInput
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_userInput); }
      set { m_param.m_userInput = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Duration in milliseconds for tone. For DTMF style user input
    /// the time the tone was detected may be placed in this field.
    /// Generally zero is passed which means the m_userInput is a
    /// single "string" input. If non-zero then m_userInput must
    /// be a single character.
    /// </summary>
    public uint Duration
    {
      get { return m_param.m_duration; }
      set { m_param.m_duration = value; }
    }
  }

  /// <summary>
  /// Message Waiting information for the OpalIndMessageWaiting indication.
  ///   This is only returned from the OpalGetMessage() function.
  /// </summary>
  public class OpalStatusMessageWaitingRef
  {
    private Opal_API.OpalStatusMessageWaiting m_param;
    
    public OpalStatusMessageWaitingRef(Opal_API.OpalStatusMessageWaiting statusMessageWaiting)
    {
      m_param = statusMessageWaiting;
    }

    public Opal_API.OpalStatusMessageWaiting Param
    {
      get { return m_param; }      
    }

    /// <summary>
    /// Party for which the MWI is directed
    /// </summary>
    public string Party
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_party); }
      set { m_param.m_party = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Type for MWI, "Voice", "Fax", "Pager", "Multimedia", "Text", "None"
    /// </summary>
    public string Type
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_type); }
      set { m_param.m_type = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Extra information for the MWI, e.g. "SUBSCRIBED",
    /// "UNSUBSCRIBED", "2/8 (0/2)
    /// </summary>
    public string ExtraInfo
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_extraInfo); }
      set { m_param.m_extraInfo = Marshal.StringToCoTaskMemAnsi(value); }
    }
  }

  /// <summary>
  /// Line Appearance information for the OpalIndLineAppearance indication.
  /// This is only returned from the OpalGetMessage() function.
  /// </summary>
  public class OpalStatusLineAppearanceRef
  {
    private Opal_API.OpalStatusLineAppearance m_param;
        
    public OpalStatusLineAppearanceRef(Opal_API.OpalStatusLineAppearance statusLineAppearance)
    {
      m_param = statusLineAppearance;
    }

    public Opal_API.OpalStatusLineAppearance Param
    {
      get { return m_param; }
    }

    /// <summary>
    /// URI for the line whose state is changing
    /// </summary>
    public string Line
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_line); }
      set { m_param.m_line = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// State the line has just moved to.
    /// </summary>
    public Opal_API.OpalLineAppearanceStates State
    {
      get { return m_param.m_state; }
      set { m_param.m_state = value; }
    }

    /// <summary>
    /// Appearance code, this is an arbitrary integer
    /// and is defined by the remote servers. If negative
    /// then it is undefined.
    /// </summary>
    public int Appearance
    {
      get { return m_param.m_appearance; }
      set { m_param.m_appearance = value; }
    }

    /// <summary>
    /// If line is "in use" then this gives information
    /// that identifies the call. Note that this will
    /// include the from/to "tags" that can identify
    /// the dialog for REFER/Replace.
    /// </summary>
    public string CallId
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_callId); }
      set { m_param.m_callId = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// A-Party for call.
    /// </summary>
    public string PartyA
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_partyA); }
      set { m_param.m_partyA = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    ///  B-Party for call.
    /// </summary>
    public string PartyB
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_partyB); }
      set { m_param.m_partyB = Marshal.StringToCoTaskMemAnsi(value); }
    }
  }

  /// <summary>
  /// Call clearance information for the OpalIndCallCleared indication.
  ///   This is only returned from the OpalGetMessage() function.
  /// </summary>
  public class OpalStatusCallClearedRef
  {
    private Opal_API.OpalStatusCallCleared m_param;
    
    public OpalStatusCallClearedRef(Opal_API.OpalStatusCallCleared statusCallCleared)
    {
      m_param = statusCallCleared;
    }

    public Opal_API.OpalStatusCallCleared Param
    {
      get { return m_param; }      
    }

    /// <summary>
    /// Call token for call being cleared.
    /// </summary>
    public string CallToken
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_callToken); }
      set { m_param.m_callToken = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// String representing the reason for the call
    /// completing. This string begins with a numeric
    /// code corresponding to values in the
    /// OpalCallEndReason enum, followed by a colon and
    /// an English description.
    /// </summary>
    public string Reason
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_reason); }
      set { m_param.m_reason = Marshal.StringToCoTaskMemAnsi(value); }
    }
  }

  /// <summary>
  /// Call clearance information for the OpalCmdClearCall command.
  /// </summary>
  public class OpalParamCallClearedRef
  {
    private Opal_API.OpalParamCallCleared m_param;   

    public OpalParamCallClearedRef(Opal_API.OpalParamCallCleared paramCallCleared)
    {
      m_param = paramCallCleared;
    }

    public Opal_API.OpalParamCallCleared Param
    {
      get { return m_param; }      
    }

    /// <summary>
    /// Call token for call being cleared.
    /// </summary>
    public string CallToken
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_callToken); }
      set { m_param.m_callToken = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Code for the call termination to be provided to the
    /// remote system.
    /// </summary>          
    public Opal_API.OpalCallEndReason Reason
    {
      get { return m_param.m_reason; }
      set { m_param.m_reason = value; }
    }
  }

  /// <summary>
  /// Media stream information for the OpalIndMediaStream indication and
  ///   OpalCmdMediaStream command.
  ///
  ///   This is may be returned from the OpalGetMessage() function or
  ///   provided to the OpalSendMessage() function.
  /// </summary>
  public class OpalStatusMediaStreamRef
  {
    private Opal_API.OpalStatusMediaStream m_param;    

    public OpalStatusMediaStreamRef(Opal_API.OpalStatusMediaStream statusMediaStream)
    {
      m_param = statusMediaStream;
    }

    public Opal_API.OpalStatusMediaStream Param
    {
      get { return m_param; }      
    }

    /// <summary>
    ///  Call token for the call the media stream is.
    /// </summary>
    public string CallToken
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_callToken); }
      set { m_param.m_callToken = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Unique identifier for the media stream. For OpalCmdMediaStream
    /// this may be left empty and the first stream of the type
    /// indicated by m_mediaType is used.
    /// </summary>
    public string Identifier
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_identifier); }
      set { m_param.m_identifier = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Media type and direction for the stream. This is a keyword such
    /// as "audio" or "video" indicating the type of the stream, a space,
    /// then either "in" or "out" indicating the direction. For
    /// OpalCmdMediaStream this may be left empty if m_identifier is
    /// used.
    /// </summary>
    public string Type
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_type); }
      set { m_param.m_type = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Media format for the stream. For OpalIndMediaStream this
    /// shows the format being used. For OpalCmdMediaStream this
    /// is the format to use. In the latter case, if empty or
    /// NULL, then a default is used. 
    /// </summary>
    public string Format
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_format); }
      set { m_param.m_format = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// For OpalIndMediaStream this indicates the status of the stream.
    /// For OpalCmdMediaStream this indicates the state to move to, see
    /// OpalMediaStates for more information.
    /// </summary>
    public Opal_API.OpalMediaStates State
    {
      get { return m_param.m_state; }
      set { m_param.m_state = value; }
    }


    /// <summary>
    /// Set the volume for the media stream as a percentage. Note this
    /// is dependent on the stream type and may be ignored. Also, a
    /// percentage of zero does not indicate muting, it indicates no
    /// change in volume. Use -1, to mute.
    /// </summary>
    public int Volume
    {
      get { return m_param.m_volume; }
      set { m_param.m_volume = value; }
    }
  }

  /// <summary>
  /// Assign a user data field to a call
  /// </summary>
  public class OpalParamSetUserDataRef
  {
    private Opal_API.OpalParamSetUserData m_param;
    
    public OpalParamSetUserDataRef(Opal_API.OpalParamSetUserData paramSetUserData)
    {
      m_param = paramSetUserData;
    }

    public Opal_API.OpalParamSetUserData Param
    {
      get { return m_param; }
    }

    /// <summary>
    /// Call token for the call the media stream is.
    /// </summary>
    public string CallToken
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_callToken); }
      set { m_param.m_callToken = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// user data value to associate with this call
    /// </summary>
    public IntPtr UserData
    {
      get { return m_param.m_userData; }
      set { m_param.m_userData = value; }
    }
  }

  /// <summary>
  /// Call recording information for the OpalCmdStartRecording command.
  /// </summary>
  public class OpalParamRecordingRef
  {
    private Opal_API.OpalParamRecording m_param;
    
    public OpalParamRecordingRef(Opal_API.OpalParamRecording paramRecording)
    {
      m_param = paramRecording;
    }

    public Opal_API.OpalParamRecording Param
    {
      get { return m_param; }
    }

    /// <summary>
    /// Call token for call being recorded.
    /// </summary>
    public string CallToken
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_callToken); }
      set { m_param.m_callToken = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// File to record into. If NULL then a test is done
    /// for if recording is currently active.
    /// </summary>
    public string File
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_file); }
      set { m_param.m_file = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Number of channels in WAV file, 1 for mono (default) or 2 for
    /// stereo where incoming & outgoing audio are in individual
    /// channels.
    /// </summary>
    public uint Channels
    {
      get { return m_param.m_channels; }
      set { m_param.m_channels = value; }
    }

    /// <summary>
    /// Audio recording format. This is generally an OpalMediaFormat
    /// name which will be used in the recording file. The exact
    /// values possible is dependent on many factors including the
    /// specific file type and what codecs are loaded as plug ins.
    /// </summary>
    public string AudioFormat
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_audioFormat); }
      set { m_param.m_audioFormat = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Video recording format. This is generally an OpalMediaFormat
    /// name which will be used in the recording file. The exact
    /// values possible is dependent on many factors including the
    /// specific file type and what codecs are loaded as plug ins.
    /// </summary>
    public string VideoFormat
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_videoFormat); }
      set { m_param.m_videoFormat = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Width of image for recording video.
    /// </summary>
    public uint VideoWidth
    {
      get { return m_param.m_videoWidth; }
      set { m_param.m_videoWidth = value; }
    }

    /// <summary>
    /// Height of image for recording video.
    /// </summary>
    public uint VideoHeight
    {
      get { return m_param.m_videoHeight; }
      set { m_param.m_videoHeight = value; }
    }

    /// <summary>
    /// Frame rate for recording video.
    /// </summary>
    public uint VideoRate
    {
      get { return m_param.m_videoRate; }
      set { m_param.m_videoRate = value; }
    }

    /// <summary>
    /// How the two images are saved in video recording.
    /// </summary>
    public Opal_API.OpalVideoRecordMixMode VideoMixing
    {
      get { return m_param.m_videoMixing; }
      set { m_param.m_videoMixing = value; }
    }
  }

  /// <summary>
  /// Call transfer information for the OpalIndTransferCall indication.
  /// This is only returned from the OpalGetMessage() function.
  /// </summary>
  public class OpalStatusTransferCallRef
  {
    private Opal_API.OpalStatusTransferCall m_param;    

    public OpalStatusTransferCallRef(Opal_API.OpalStatusTransferCall statusTransferCall)
    {
      m_param = statusTransferCall;
    }

    public Opal_API.OpalStatusTransferCall Param
    {
      get { return m_param; }
    }

    /// <summary>
    /// Call token for call being transferred.
    /// </summary>
    public string CallToken
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_callToken); }
      set { m_param.m_callToken = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// ID assigned by the underlying protocol for the call. 
    /// Only available in version 18 and above
    /// </summary>
    public string ProtocolCallId
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_protocolCallId); }
      set { m_param.m_protocolCallId = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Result of transfer operation. This is one of:
    /// "progress"  transfer of this call is still in progress.
    /// "success"   transfer of this call completed, call will
    ///             be cleared.
    /// "failed"    transfer initiated by this call did not
    ///             complete, call remains active.
    /// "started"   remote system has asked local connection
    ///             to transfer to another target.
    /// "completed" local connection has completed the transfer
    ///             to other target.
    /// "forwarded" remote has forwarded call local system has
    ///             initiated to another address.
    /// "incoming"  this call is the target of an incoming
    ///             transfer, e.g. party C in a consultation
    ///             transfer scenario.
    /// </summary>
    public string Result
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_result); }
      set { m_param.m_result = Marshal.StringToCoTaskMemAnsi(value); }
    }

    /// <summary>
    /// Protocol dependent information in the form:
    /// key=value\n
    /// key=value\n
    /// etc
    /// </summary>
    public string Info
    {
      get { return Marshal.PtrToStringAnsi(m_param.m_info); }
      set { m_param.m_info = Marshal.StringToCoTaskMemAnsi(value); }
    }
  }
}
