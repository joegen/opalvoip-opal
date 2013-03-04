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

import android.app.Activity;
import android.widget.TextView;
import android.os.Bundle;
import android.content.Context;

import org.opalvoip.opal.*;

public class AndOPAL extends Activity
{
    protected OpalContext m_opal;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        /* Create a TextView and set its content.
         * the text is retrieved by calling a native
         * function.
         */
        TextView  tv = new TextView(this);

        // Initialise OPAL library
        m_opal = new OpalContext();
        long apiVersion = m_opal.Initialise(OPALConstants.OPAL_PREFIX_ALL +
                                            " --config \"" + getFilesDir().getAbsolutePath() + '"' +
                                            " --plugin \"" + getApplicationInfo().nativeLibraryDir + '"' +
                                            " -tttto \"" + getCacheDir().getAbsolutePath() + "/opal.log\"");
        if (apiVersion == 0)
            tv.setText("Could not initialise OPAL");
        else {
           // Set up general parameters
           OpalMessagePtr message = new OpalMessagePtr(OpalMessageType.OpalCmdSetGeneralParameters);
           OpalParamGeneral general = message.GetGeneralParams();
           general.setM_natMethod("Fixed");
           general.setM_natServer("110.143.127.148");

           OpalMessagePtr response = new OpalMessagePtr();
           if (!m_opal.SendMessage(message, response))
               tv.setText("General set up failure: " + response.GetCommandError());
           else {
              // Set up general parameters
              message = new OpalMessagePtr(OpalMessageType.OpalCmdSetProtocolParameters);
              OpalParamProtocol proto = message.GetProtocolParams();
              proto.setM_prefix("sip");
              proto.setM_userName("someone");
              proto.setM_displayName("Someone McSomething");

              if (!m_opal.SendMessage(message, response))
                 tv.setText("SIP Protocol set up failure: " + response.GetCommandError());
              else {
                 message = new OpalMessagePtr(OpalMessageType.OpalCmdRegistration);
                 OpalParamRegistration reg = message.GetRegistrationParams();
                 reg.setM_protocol("sip");
                 reg.setM_identifier("android");
                 reg.setM_hostName("10.0.1.16");
                 reg.setM_timeToLive(300);

                 if (!m_opal.SendMessage(message, response))
                    tv.setText("SIP Protocol set up failure: " + response.GetCommandError());
                 else
                    tv.setText("OPAL Successfully initialised");
              }
           }
        }

        setContentView(tv);
    }

    static {
        System.loadLibrary("andopal");
    }
}
