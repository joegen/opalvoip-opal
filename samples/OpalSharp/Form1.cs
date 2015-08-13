using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace OpalSharp
{
    public partial class Form1 : Form
    {
        OpalContext m_opalContext;
        System.Threading.Thread m_opalThread;
        string m_registrationIdentifier;
        string m_opalCallToken;

        delegate void EnableCallback(Control ctrl, bool enable);

        private void EnableSafely(Control ctrl, bool enable)
        {
            if (ctrl.InvokeRequired)
            {
                EnableCallback d = new EnableCallback(EnableSafely);
                this.Invoke(d, new object[] { ctrl, enable  });
            }
            else
            {
                ctrl.Enabled = enable;
            }
        }

        delegate void SetTextCallback(Control ctrl, string text);

        private void SetTextSafely(Control ctrl, string text)
        {
            if (ctrl.InvokeRequired)
            {
                SetTextCallback d = new SetTextCallback(SetTextSafely);
                this.Invoke(d, new object[] { ctrl, text });
            }
            else
            {
                ctrl.Text = text;
            }
        }

        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
        }


        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
            Properties.Settings.Default.Save();
        }

        private void HandleOpalMessages()
        {
            OpalMessagePtr msg = new OpalMessagePtr();
            while (m_opalContext.GetMessage(msg, System.UInt32.MaxValue))
            {
                // Handle the message
                switch (msg.GetType())
                {
                    case OpalMessageType.OpalIndRegistration:
                        OpalStatusRegistration reg = msg.GetRegistrationStatus();
                        switch (reg.status)
                        {
                            case OpalRegistrationStates.OpalRegisterSuccessful:
                                SetTextSafely(RegisterStatus, "Registration complete to " + reg.serverName);
                                EnableSafely(Unregister, true);
                                break;
                            case OpalRegistrationStates.OpalRegisterRemoved :
                                SetTextSafely(RegisterStatus, "Registration removed from " + reg.serverName);
                                EnableSafely(Unregister, false);
                                EnableSafely(Register, true);
                                break;
                            case OpalRegistrationStates.OpalRegisterFailed:
                                SetTextSafely(RegisterStatus, "Registration failed: " + reg.error);
                                EnableSafely(Register, true);
                                break;
                        }
                        break;

                    case OpalMessageType.OpalIndIncomingCall :
                        EnableSafely(Call, false);
                        EnableSafely(Answer, true);
                        EnableSafely(HangUp, true);
                        SetTextSafely(CallStatus, "Incoming call ..." + msg.GetIncomingCall().remoteDisplayName);
                        break;

                    case OpalMessageType.OpalIndEstablished :
                        SetTextSafely(CallStatus, "Call established");
                        break;

                    case OpalMessageType.OpalIndCallCleared:
                        EnableSafely(Call, true);
                        EnableSafely(Answer, false);
                        EnableSafely(HangUp, false);
                        SetTextSafely(CallStatus, "Call ended: " + msg.GetCallCleared().reason);
                        break;
                }
            }
        }

        private void Initialise_Click(object sender, EventArgs e)
        {
            if (m_opalContext == null)
            {
                Initialise.Enabled = false;
                LogLevel.Enabled = false;
                LogFile.Enabled = false;
                StunServer.Enabled = false;

                m_opalContext = new OpalContext();
                if (m_opalContext.Initialise(OPAL.OPAL_PREFIX_SIP + " " + OPAL.OPAL_PREFIX_PCSS +
                                             " --trace-level " + LogLevel.Value + " --output \"" + LogFile.Text + "\"") == 0)
                    MessageBox.Show("Failed to initialise", "OPAL", MessageBoxButtons.OK);
                else
                {
                    m_opalThread = new System.Threading.Thread(new System.Threading.ThreadStart(HandleOpalMessages));
                    m_opalThread.Start();

                    OpalMessagePtr msg = new OpalMessagePtr(OpalMessageType.OpalCmdSetGeneralParameters);
                    OpalParamGeneral gen = msg.GetGeneralParams();
                    gen.natMethod = "STUN";
                    gen.natServer = StunServer.Text;
                    gen.autoRxMedia = gen.autoTxMedia = "audio video";
                    gen.mediaMask = "!*uLaw*\n!*264-0\nH.263*"; // Kind of backwards, it's a mask with negative entries
                    gen.mediaOptions = "video:Frame Width=640\nvideo:Frame Height=480\nvideo:Target Bit Rate=256000";
                    gen.audioPlayerDevice = "Default communications";
                    gen.audioRecordDevice = "Default communications";
                    gen.videoOutputDevice = "MSWIN STYLE=0x50000000 PARENT=" + VideoDisplay.Handle + " X=0 Y=0 WIDTH=" + VideoDisplay.Width + " HEIGHT=" + VideoDisplay.Height;
                    gen.videoPreviewDevice = "MSWIN STYLE=0x50000000 PARENT=" + VideoPreview.Handle + " X=0 Y=0 WIDTH=" + VideoPreview.Width + " HEIGHT=" + VideoPreview.Height;
                    gen.noMediaTimeout = 15000;
                    OpalMessagePtr result = new OpalMessagePtr();
                    if (!m_opalContext.SendMessage(msg, result))
                        MessageBox.Show("Could not set general parameters: " + result.GetCommandError(), "OPAL", MessageBoxButtons.OK);
                    else
                    {
                        msg = new OpalMessagePtr(OpalMessageType.OpalCmdSetProtocolParameters);
                        OpalParamProtocol proto = msg.GetProtocolParams();
                        proto.prefix = OPAL.OPAL_PREFIX_SIP;
                        proto.interfaceAddresses = "0.0.0.0"; // All interfaces
                        proto.displayName = "OPAL C# Sample";
                        if (!m_opalContext.SendMessage(msg))
                            MessageBox.Show("Could not set general parameters", "OPAL", MessageBoxButtons.OK);
                        else
                        {
                            Shutdown.Enabled = true;
                            Register.Enabled = true;
                            Call.Enabled = true;
                            return;
                        }
                    }
                }

                Initialise.Enabled = true;
                LogLevel.Enabled = true;
                LogFile.Enabled = true;
                StunServer.Enabled = true;
            }
        }

        private void Shutdown_Click(object sender, EventArgs e)
        {
            if (m_opalContext != null)
            {
                Shutdown.Enabled = false;
                Register.Enabled = false;
                Unregister.Enabled = false;
                Call.Enabled = false;
                HangUp.Enabled = false;

                m_opalContext.ShutDown(); // This will make GetMessage() return false
                m_opalThread.Join();
                m_opalThread = null;
                m_opalContext = null;

                Initialise.Enabled = true;
                LogLevel.Enabled = true;
                LogFile.Enabled = true;
                StunServer.Enabled = true;
            }
        }

        private void Register_Click(object sender, EventArgs e)
        {
            if (m_opalContext != null)
            {
                Register.Enabled = false;
                host.Enabled = false;
                user.Enabled = false;
                password.Enabled = false;

                OpalMessagePtr msg = new OpalMessagePtr(OpalMessageType.OpalCmdRegistration);
                OpalParamRegistration reg = msg.GetRegistrationParams();
                reg.protocol = "sip";
                reg.hostName = host.Text;
                reg.identifier = user.Text;
                reg.password = password.Text;
                reg.timeToLive = 300;
                if (RFC5626.Checked)
                    reg.attributes = "compatibility=RFC5626";
                OpalMessagePtr result = new OpalMessagePtr();
                if (m_opalContext.SendMessage(msg, result))
                {
                    m_registrationIdentifier = result.GetRegistrationParams().identifier;
                    return;
                }

                MessageBox.Show("Could not start registration: " + result.GetCommandError(), "OPAL", MessageBoxButtons.OK);
                Register.Enabled = true;
                host.Enabled = true;
                user.Enabled = true;
                password.Enabled = true;
            }
        }

        private void Unregister_Click(object sender, EventArgs e)
        {
            if (m_opalContext != null)
            {
                OpalMessagePtr msg = new OpalMessagePtr(OpalMessageType.OpalCmdRegistration);
                OpalParamRegistration reg = msg.GetRegistrationParams();
                reg.protocol = "sip";
                reg.identifier = m_registrationIdentifier;
                reg.timeToLive = 0; // Zero unregisters
                OpalMessagePtr result = new OpalMessagePtr();
                if (m_opalContext.SendMessage(msg, result))
                    Register.Enabled = false;
                else
                    MessageBox.Show("Could not set start registration: " + result.GetCommandError(), "OPAL", MessageBoxButtons.OK);
            }
        }

        private void Call_Click(object sender, EventArgs e)
        {
            if (m_opalContext != null)
            {
                OpalMessagePtr msg = new OpalMessagePtr(OpalMessageType.OpalCmdSetUpCall);
                OpalParamSetUpCall call = msg.GetCallSetUp();
                call.partyB = urlToCall.Text;
                OpalMessagePtr result = new OpalMessagePtr();
                if (m_opalContext.SendMessage(msg, result))
                {
                    m_opalCallToken = result.GetCallSetUp().callToken;
                    Call.Enabled = false;
                    HangUp.Enabled = true;
                    CallStatus.Text = "Calling " + urlToCall.Text;
                }
                else
                    MessageBox.Show("Could not start call: " + result.GetCommandError(), "OPAL", MessageBoxButtons.OK);
            }
        }

        private void HangUp_Click(object sender, EventArgs e)
        {
            if (m_opalContext != null && m_opalCallToken.Length > 0)
            {
                HangUp.Enabled = false;

                OpalMessagePtr msg = new OpalMessagePtr(OpalMessageType.OpalCmdClearCall);
                OpalParamCallCleared call = msg.GetClearCall();
                call.callToken = m_opalCallToken;
                OpalMessagePtr result = new OpalMessagePtr();
                if (m_opalContext.SendMessage(msg, result))
                    HangUp.Enabled = false;
                else
                    MessageBox.Show("Could not hang up call: " + result.GetCommandError(), "OPAL", MessageBoxButtons.OK);
            }
        }
    }
}
