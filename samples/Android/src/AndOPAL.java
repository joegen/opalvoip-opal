/*
 * AndOPAL.java
 *
 * Copyright (c) 2013 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida (Robert Jongbloed)
 *
 * Contributor(s): ______________________________________.
 *
 */

package org.opalvoip.opal.andsample;

import java.lang.Thread;
import android.app.Activity;
import android.view.View;
import android.view.*;
import android.widget.*;
import android.text.TextWatcher;
import android.text.Editable;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.content.Context;

import org.opalvoip.opal.*;

public class AndOPAL extends Activity
{
    static {
        System.loadLibrary("andopal");
    }


    // Parameters
    String m_traceOutput = "opal.log";
//    String m_traceOutput = "syslog";
    String m_traceOptions = "--trace-level 5 --trace-option +ar";
    int    m_audioBufferTime = Build.MANUFACTURER.equals("unknown") ? 250 : 120; // Milliseconds
    String m_natMethod = "STUN";
    String m_natServer = "stun.ekiga.net";
    String m_userName = "opal_android";
    String m_displayName = "OPAL on Android User";
    String m_domain = "ekiga.net";
    String m_password = "ported!";

    // GUI elements
    private TextView m_statusText;
    private EditText m_destination;
    private Button   m_callButton;
    private Button   m_answerButton;
    private Button   m_hangUpButton;

    // OPAL elements
    private OpalContext m_opal;
    private Thread      m_opalShutDownThread;
    private boolean     m_loggedIn = false;
    private String      m_callToken;


    // Message handler for getting text from background thread to GUI
    private Handler m_statusHandler = new Handler() {
        @Override public void handleMessage(Message msg)
        {
            m_statusText.append((String)msg.obj + '\n');
        }
    };


    private void OutputStatus(String str)
    {
        m_statusHandler.sendMessage(m_statusHandler.obtainMessage(0, str));
    }


    private boolean SendOpalCommand(OpalMessagePtr message, OpalMessagePtr response, String title)
    {
        if (m_opal.SendMessage(message, response))
            return true;

        OutputStatus(title + " failure: " + response.GetCommandError());
        return false;
    }


    private boolean SendOpalCommand(OpalMessagePtr message, String title)
    {
        OpalMessagePtr response = new OpalMessagePtr();
        return SendOpalCommand(message, response, title);
    }


    // Message handler for incoming calls
    private Handler m_opalMessageHandler = new Handler() {
        @Override public void handleMessage(Message msg)
        {
            OpalMessagePtr message = (OpalMessagePtr)msg.obj;
            if (message.GetType() == OpalMessageType.OpalIndIncomingCall) {
                OpalStatusIncomingCall call = message.GetIncomingCall();
                m_callToken = call.getCallToken();
                m_callButton.setEnabled(false);
                m_answerButton.setEnabled(true);
                m_hangUpButton.setEnabled(true);
                OutputStatus("Incoming call from " + call.getRemoteAddress());
            }
            else if (message.GetType() == OpalMessageType.OpalIndCallCleared) {
                OpalStatusCallCleared call = message.GetCallCleared();
                m_callToken = call.getCallToken();
                m_callButton.setEnabled(m_destination.getText().length() > 0);
                m_answerButton.setEnabled(false);
                m_hangUpButton.setEnabled(false);
                String reason = call.getReason();
                OutputStatus("Cleared call: " + reason.substring(reason.indexOf(':')+1));
            }
            else if (message.GetType() == OpalMessageType.OpalIndAlerting)
                OutputStatus("Ringing");
            else if (message.GetType() == OpalMessageType.OpalIndEstablished)
                OutputStatus("Established");
            else if (message.GetType() == OpalMessageType.OpalIndRegistration) {
                OpalStatusRegistration reg = message.GetRegistrationStatus();
                if (reg.getStatus() == OpalRegistrationStates.OpalRegisterFailed)
                    OutputStatus("Register failed: " + reg.getError());
                else if (reg.getStatus() == OpalRegistrationStates.OpalRegisterRemoved)
                    OutputStatus("Unregistered.");
                else if (reg.getStatus() == OpalRegistrationStates.OpalRegisterSuccessful) {
                    m_loggedIn = true;
                    m_destination.setEnabled(true);
                    invalidateOptionsMenu();
                    OutputStatus("Registered. Ready!");
                }
            }
        }
    };


    private void MessageLoop()
    {
        OutputStatus("OPAL ready.");

        OpalMessagePtr message = new OpalMessagePtr();
        while (m_opal.GetMessage(message, 1000000000)) {
            m_opalMessageHandler.sendMessage(m_opalMessageHandler.obtainMessage(0, message));
            message = new OpalMessagePtr();
        }

        OutputStatus("OPAL shut down!");
    }


    public void MakeCall(String dest)
    {
        OpalMessagePtr message = new OpalMessagePtr(OpalMessageType.OpalCmdSetUpCall);
        OpalParamSetUpCall setup = message.GetCallSetUp();
        setup.setPartyB(dest);

        OpalMessagePtr response = new OpalMessagePtr();
        if (!SendOpalCommand(message, response, "Call"))
            return;

        OutputStatus("Calling \"" + dest + '"');

        setup = response.GetCallSetUp();
        m_callToken = setup.getCallToken();

        m_callButton.setEnabled(false);
        m_hangUpButton.setEnabled(true);
    }


    public void onCallButton(View view)
    {
        MakeCall(m_destination.getText().toString());
    }


    public void onAnswerButton(View view)
    {
        OpalMessagePtr message = new OpalMessagePtr(OpalMessageType.OpalCmdAnswerCall);
        OpalParamAnswerCall answer = message.GetAnswerCall();
        answer.setCallToken(m_callToken);

        if (SendOpalCommand(message, "Answer")) {
            OutputStatus("Answering call");
            m_hangUpButton.setEnabled(true);
        }
    }


    public void onHangUpButton(View view)
    {
        OpalMessagePtr message = new OpalMessagePtr(OpalMessageType.OpalCmdClearCall);
        OpalParamCallCleared clear = message.GetClearCall();
        clear.setCallToken(m_callToken);

        if (SendOpalCommand(message, "Hang up")) {
            OutputStatus("Hanging up");
            m_callButton.setEnabled(m_destination.getText().length() > 0);
            m_answerButton.setEnabled(false);
            m_hangUpButton.setEnabled(false);
        }
    }


    private void OpalLogIn()
    {
        // Set up general parameters
        OpalMessagePtr message = new OpalMessagePtr(OpalMessageType.OpalCmdSetGeneralParameters);
        OpalParamGeneral general = message.GetGeneralParams();
        general.setAudioRecordDevice("microphone");
        general.setAudioPlayerDevice("voice");
        general.setAudioBufferTime(m_audioBufferTime);
        general.setNatMethod(m_natMethod);
        general.setNatServer(m_natServer);

        if (!SendOpalCommand(message, "General set up"))
            return;

        // Set up all protocol parameters
        message = new OpalMessagePtr(OpalMessageType.OpalCmdSetProtocolParameters);
        OpalParamProtocol proto = message.GetProtocolParams();
        // The following should come from GUI, but I am too lazy to add.
        proto.setUserName(m_userName);
        proto.setDisplayName(m_displayName);

        if (!SendOpalCommand(message, "Protocol set up"))
            return;

        // Set up sip specific parameters
        message = new OpalMessagePtr(OpalMessageType.OpalCmdSetProtocolParameters);
        proto = message.GetProtocolParams();
        proto.setPrefix("sip");
        proto.setInterfaceAddresses("*");

        if (!SendOpalCommand(message, "SIP set up"))
            return;

        // Register withn server
        message = new OpalMessagePtr(OpalMessageType.OpalCmdRegistration);
        OpalParamRegistration reg = message.GetRegistrationParams();
        reg.setProtocol("sip");
        reg.setIdentifier(m_userName);
        reg.setHostName(m_domain);
        reg.setPassword(m_password);
        reg.setTimeToLive(300);
        reg.setAttributes("compatibility=ALG");

        if (!SendOpalCommand(message, "Registration"))
            return;

        OutputStatus("Registering ...");
    }


    public void onLogIn(MenuItem item)
    {
        OutputStatus("Logging in ...");
        item.setEnabled(false);

        (new Thread(new Runnable() {
            public void run()
            {
                OpalLogIn();
            }
        })).start();
    }


    public void onEchoTest(MenuItem item)
    {
        MakeCall("sip:500@ekiga.net");
    }


    public void onLogOut(MenuItem item)
    {
        m_loggedIn = false;
        m_destination.setEnabled(false);
        m_callButton.setEnabled(false);
        m_answerButton.setEnabled(false);
        m_hangUpButton.setEnabled(false);
        invalidateOptionsMenu();

        OutputStatus("Shutting down ...");
    }


    public static native String TestPlayer(int bufferTime);
    public static native String TestRecorder(int bufferTime);

    public void onTestAudio(MenuItem item)
    {
        (new Thread(new Runnable() {
            public void run()
            {
                OutputStatus("Testing audio player ... ");
                OutputStatus(TestPlayer(m_audioBufferTime));
                OutputStatus("Testing audio recorder ... ");
                OutputStatus(TestRecorder(m_audioBufferTime));
            }
        })).start();
    }


    public void onQuit(MenuItem item)
    {

        (new Thread(new Runnable() {
            public void run()
            {
                m_opal.ShutDown();
                finish();
            }
        })).start();
    }


    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        m_statusText = (TextView)findViewById(R.id.status);

        m_callButton = (Button)findViewById(R.id.call);
        m_callButton.setEnabled(false);

        m_answerButton = (Button)findViewById(R.id.answer);
        m_answerButton.setEnabled(false);

        m_hangUpButton = (Button)findViewById(R.id.hangup);
        m_hangUpButton.setEnabled(false);

        m_destination = (EditText)findViewById(R.id.destination);
        m_destination.addTextChangedListener(
            new TextWatcher() {
                public void afterTextChanged(Editable s) {
                    m_callButton.setEnabled(m_loggedIn && s.length() > 0);
                }
                public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
                public void onTextChanged(CharSequence s, int start, int before, int count) { }
            }
        );

        // Initialise OPAL library
        m_opal = new OpalContext();
        String options = OPALConstants.OPAL_PREFIX_ALL +
                         " --config \"" + getFilesDir().getAbsolutePath() + '"' +
                         " --plugin \"" + getApplicationInfo().nativeLibraryDir + "\" " +
                         m_traceOptions;
        if (m_traceOutput != "") {
          options += " --output \"";
          if (m_traceOutput != "syslog")
            options += getCacheDir().getAbsolutePath() + "/";
          options += m_traceOutput + "\"";
        }
        if (m_opal.Initialise(options) == 0)
            OutputStatus("Could not initialise OPAL");
        else {
            (new Thread(new Runnable() {
                public void run()
                {
                    MessageLoop();
                }
            })).start();
        }
    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.popup, menu);
        return true;
    }


    @Override
    public boolean onPrepareOptionsMenu(Menu menu)
    {
        menu.findItem(R.id.testaudio).setEnabled(m_opal.IsInitialised());
        menu.findItem(R.id.login).setEnabled(!m_loggedIn);
        menu.findItem(R.id.echoTest).setEnabled(m_loggedIn);
        menu.findItem(R.id.logout).setEnabled(m_loggedIn);
        return true;
    }
}
