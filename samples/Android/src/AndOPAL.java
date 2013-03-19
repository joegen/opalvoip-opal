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
 * $Revision$
 * $Author$
 * $Date$
 */

package org.opalvoip.opal.andsample;

import java.lang.Thread;
import android.app.Activity;
import android.view.View;
import android.widget.*;
import android.text.TextWatcher;
import android.text.Editable;
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
    private Thread      m_opalMessageThread;
    private Thread      m_opalShutDownThread;
    private boolean     m_opalReady = false;
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

    private boolean OpalInitialise()
    {
        OutputStatus("Initialising OPAL ...");

        // Initialise OPAL library
        m_opal = new OpalContext();
        long apiVersion = m_opal.Initialise(OPALConstants.OPAL_PREFIX_ALL +
                                            " --config \"" + getFilesDir().getAbsolutePath() + '"' +
                                            " --plugin \"" + getApplicationInfo().nativeLibraryDir + '"' +
//                                            " -tttttosyslog"
                                            " -ttttto \"" +  getCacheDir().getAbsolutePath() + "/opal.log\""
                                            );
        if (apiVersion == 0) {
            OutputStatus("Could not initialise OPAL");
            return false;
        }

        // Set up general parameters
        OpalMessagePtr message = new OpalMessagePtr(OpalMessageType.OpalCmdSetGeneralParameters);
        OpalParamGeneral general = message.GetGeneralParams();
        general.setM_natMethod(m_natMethod);
        general.setM_natServer(m_natServer);

        if (!SendOpalCommand(message, "General set up"))
            return false;

        // Set up all protocol parameters
        message = new OpalMessagePtr(OpalMessageType.OpalCmdSetProtocolParameters);
        OpalParamProtocol proto = message.GetProtocolParams();
        // The following should come from GUI, but I am too lazy to add.
        proto.setM_userName(m_userName);
        proto.setM_displayName(m_displayName);

        if (!SendOpalCommand(message, "Protocol set up"))
            return false;

        // Set up sip specific parameters
        message = new OpalMessagePtr(OpalMessageType.OpalCmdSetProtocolParameters);
        proto = message.GetProtocolParams();
        proto.setM_prefix("sip");
        proto.setM_interfaceAddresses("*");

        if (!SendOpalCommand(message, "SIP set up"))
            return false;

        // Register withn server
        message = new OpalMessagePtr(OpalMessageType.OpalCmdRegistration);
        OpalParamRegistration reg = message.GetRegistrationParams();
        reg.setM_protocol("sip");
        reg.setM_identifier(m_userName);
        reg.setM_hostName(m_domain);
        reg.setM_password(m_password);
        reg.setM_timeToLive(300);

        if (!SendOpalCommand(message, "Registration"))
            return false;

        OutputStatus("Registering ...");
        return true;
    }

    // Message handler for incoming calls
    private Handler m_opalMessageHandler = new Handler() {
        @Override public void handleMessage(Message msg)
        {
            OpalMessagePtr message = (OpalMessagePtr)msg.obj;
            if (message.GetType() == OpalMessageType.OpalIndIncomingCall) {
                OpalStatusIncomingCall call = message.GetIncomingCall();
                m_callToken = call.getM_callToken();
                m_callButton.setEnabled(false);
                m_answerButton.setEnabled(true);
                m_hangUpButton.setEnabled(true);
                OutputStatus("Incoming call from " + call.getM_remoteAddress());
            }
            else if (message.GetType() == OpalMessageType.OpalIndCallCleared) {
                OpalStatusCallCleared call = message.GetCallCleared();
                m_callToken = call.getM_callToken();
                m_callButton.setEnabled(m_destination.getText().length() > 0);
                m_answerButton.setEnabled(false);
                m_hangUpButton.setEnabled(false);
                String reason = call.getM_reason();
                OutputStatus("Cleared call: " + reason.substring(reason.indexOf(':')+1));
            }
            else if (message.GetType() == OpalMessageType.OpalIndAlerting)
                OutputStatus("Ringing");
            else if (message.GetType() == OpalMessageType.OpalIndEstablished)
                OutputStatus("Established");
            else if (message.GetType() == OpalMessageType.OpalIndRegistration) {
                OpalStatusRegistration reg = message.GetRegistrationStatus();
                if (reg.getM_status() == OpalRegistrationStates.OpalRegisterFailed)
                    OutputStatus("Register failed: " + reg.getM_error());
                else if (reg.getM_status() == OpalRegistrationStates.OpalRegisterRemoved)
                    OutputStatus("Unregistered.");
                else if (reg.getM_status() == OpalRegistrationStates.OpalRegisterSuccessful) {
                    OutputStatus("Registered. Ready!");
                    m_opalReady = true;
                }
            }
        }
    };

    private void MessageLoop()
    {
        if (OpalInitialise()) {
            OpalMessagePtr message = new OpalMessagePtr();
            while (m_opal.GetMessage(message, 1000000000)) {
                m_opalMessageHandler.sendMessage(m_opalMessageHandler.obtainMessage(0, message));
                message = new OpalMessagePtr();
            }
            OutputStatus("OPAL shut down!");
            m_opalReady = false;
        }
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
                    m_callButton.setEnabled(m_opalReady && s.length() > 0);
                }
                public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
                public void onTextChanged(CharSequence s, int start, int before, int count) { }
            }
        );

        m_opalMessageThread = new Thread(new Runnable() {
            public void run()
            {
                MessageLoop();
            }
        });
        m_opalMessageThread.start();
    }

    public void onCallButton(View view)
    {
        String dest = m_destination.getText().toString();

        OpalMessagePtr message = new OpalMessagePtr(OpalMessageType.OpalCmdSetUpCall);
        OpalParamSetUpCall setup = message.GetCallSetUp();
        setup.setM_partyB(dest);

        OpalMessagePtr response = new OpalMessagePtr();
        if (!SendOpalCommand(message, response, "Call"))
            return;

        OutputStatus("Calling \"" + dest + '"');

        setup = response.GetCallSetUp();
        m_callToken = setup.getM_callToken();

        m_callButton.setEnabled(false);
        m_hangUpButton.setEnabled(true);
    }

    public void onAnswerButton(View view)
    {
        OpalMessagePtr message = new OpalMessagePtr(OpalMessageType.OpalCmdAnswerCall);
        OpalParamAnswerCall answer = message.GetAnswerCall();
        answer.setM_callToken(m_callToken);

        if (SendOpalCommand(message, "Answer")) {
            OutputStatus("Answering call");
            m_hangUpButton.setEnabled(true);
        }
    }

    public void onHangUpButton(View view)
    {
        OpalMessagePtr message = new OpalMessagePtr(OpalMessageType.OpalCmdClearCall);
        OpalParamCallCleared clear = message.GetClearCall();
        clear.setM_callToken(m_callToken);

        if (SendOpalCommand(message, "Hang up")) {
            OutputStatus("Hanging up");
            m_callButton.setEnabled(m_destination.getText().length() > 0);
            m_answerButton.setEnabled(false);
            m_hangUpButton.setEnabled(false);
        }
    }

    public void onShutDownButton(View view)
    {
        OutputStatus("Shutting down ...");
        ((Button)findViewById(R.id.shutdown)).setEnabled(false);
        m_opalShutDownThread = new Thread(new Runnable() {
            public void run()
            {
                m_opal.ShutDown();
            }
        });
        m_opalShutDownThread.start();
    }
}
