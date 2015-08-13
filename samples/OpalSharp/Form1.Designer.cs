namespace OpalSharp
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.Initialise = new System.Windows.Forms.Button();
            this.Shutdown = new System.Windows.Forms.Button();
            this.Register = new System.Windows.Forms.Button();
            this.Call = new System.Windows.Forms.Button();
            this.Unregister = new System.Windows.Forms.Button();
            this.HangUp = new System.Windows.Forms.Button();
            this.Answer = new System.Windows.Forms.Button();
            this.VideoDisplay = new System.Windows.Forms.PictureBox();
            this.VideoPreview = new System.Windows.Forms.PictureBox();
            this.StunServer = new System.Windows.Forms.TextBox();
            this.LogFile = new System.Windows.Forms.TextBox();
            this.LogLevel = new System.Windows.Forms.NumericUpDown();
            this.urlToCall = new System.Windows.Forms.TextBox();
            this.password = new System.Windows.Forms.TextBox();
            this.user = new System.Windows.Forms.TextBox();
            this.host = new System.Windows.Forms.TextBox();
            this.RegisterStatus = new System.Windows.Forms.Label();
            this.CallStatus = new System.Windows.Forms.Label();
            this.RFC5626 = new System.Windows.Forms.CheckBox();
            ((System.ComponentModel.ISupportInitialize)(this.VideoDisplay)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.VideoPreview)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.LogLevel)).BeginInit();
            this.SuspendLayout();
            // 
            // Initialise
            // 
            this.Initialise.Location = new System.Drawing.Point(30, 12);
            this.Initialise.Name = "Initialise";
            this.Initialise.Size = new System.Drawing.Size(75, 23);
            this.Initialise.TabIndex = 0;
            this.Initialise.Text = "Initialise";
            this.Initialise.UseVisualStyleBackColor = true;
            this.Initialise.Click += new System.EventHandler(this.Initialise_Click);
            // 
            // Shutdown
            // 
            this.Shutdown.Enabled = false;
            this.Shutdown.Location = new System.Drawing.Point(30, 34);
            this.Shutdown.Name = "Shutdown";
            this.Shutdown.Size = new System.Drawing.Size(75, 23);
            this.Shutdown.TabIndex = 1;
            this.Shutdown.Text = "Shutdown";
            this.Shutdown.UseVisualStyleBackColor = true;
            this.Shutdown.Click += new System.EventHandler(this.Shutdown_Click);
            // 
            // Register
            // 
            this.Register.Enabled = false;
            this.Register.Location = new System.Drawing.Point(30, 77);
            this.Register.Name = "Register";
            this.Register.Size = new System.Drawing.Size(75, 23);
            this.Register.TabIndex = 2;
            this.Register.Text = "Register";
            this.Register.UseVisualStyleBackColor = true;
            this.Register.Click += new System.EventHandler(this.Register_Click);
            // 
            // Call
            // 
            this.Call.Enabled = false;
            this.Call.Location = new System.Drawing.Point(30, 143);
            this.Call.Name = "Call";
            this.Call.Size = new System.Drawing.Size(75, 23);
            this.Call.TabIndex = 6;
            this.Call.Text = "Call";
            this.Call.UseVisualStyleBackColor = true;
            this.Call.Click += new System.EventHandler(this.Call_Click);
            // 
            // Unregister
            // 
            this.Unregister.Enabled = false;
            this.Unregister.Location = new System.Drawing.Point(30, 103);
            this.Unregister.Name = "Unregister";
            this.Unregister.Size = new System.Drawing.Size(75, 23);
            this.Unregister.TabIndex = 9;
            this.Unregister.Text = "Unregister";
            this.Unregister.UseVisualStyleBackColor = true;
            this.Unregister.Click += new System.EventHandler(this.Unregister_Click);
            // 
            // HangUp
            // 
            this.HangUp.Enabled = false;
            this.HangUp.Location = new System.Drawing.Point(30, 172);
            this.HangUp.Name = "HangUp";
            this.HangUp.Size = new System.Drawing.Size(75, 23);
            this.HangUp.TabIndex = 10;
            this.HangUp.Text = "Hang Up";
            this.HangUp.UseVisualStyleBackColor = true;
            this.HangUp.Click += new System.EventHandler(this.HangUp_Click);
            // 
            // Answer
            // 
            this.Answer.Enabled = false;
            this.Answer.Location = new System.Drawing.Point(30, 201);
            this.Answer.Name = "Answer";
            this.Answer.Size = new System.Drawing.Size(75, 23);
            this.Answer.TabIndex = 13;
            this.Answer.Text = "Answer";
            this.Answer.UseVisualStyleBackColor = true;
            // 
            // VideoDisplay
            // 
            this.VideoDisplay.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.VideoDisplay.Location = new System.Drawing.Point(53, 243);
            this.VideoDisplay.Name = "VideoDisplay";
            this.VideoDisplay.Size = new System.Drawing.Size(352, 288);
            this.VideoDisplay.TabIndex = 14;
            this.VideoDisplay.TabStop = false;
            // 
            // VideoPreview
            // 
            this.VideoPreview.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.VideoPreview.Location = new System.Drawing.Point(63, 449);
            this.VideoPreview.Name = "VideoPreview";
            this.VideoPreview.Size = new System.Drawing.Size(88, 72);
            this.VideoPreview.TabIndex = 15;
            this.VideoPreview.TabStop = false;
            // 
            // StunServer
            // 
            this.StunServer.DataBindings.Add(new System.Windows.Forms.Binding("Text", global::OpalSharp.Properties.Settings.Default, "StunServer", true, System.Windows.Forms.DataSourceUpdateMode.OnPropertyChanged));
            this.StunServer.Location = new System.Drawing.Point(128, 37);
            this.StunServer.Name = "StunServer";
            this.StunServer.Size = new System.Drawing.Size(131, 20);
            this.StunServer.TabIndex = 12;
            this.StunServer.Text = global::OpalSharp.Properties.Settings.Default.StunServer;
            // 
            // LogFile
            // 
            this.LogFile.AllowDrop = true;
            this.LogFile.DataBindings.Add(new System.Windows.Forms.Binding("Text", global::OpalSharp.Properties.Settings.Default, "LogFile", true, System.Windows.Forms.DataSourceUpdateMode.OnPropertyChanged));
            this.LogFile.Location = new System.Drawing.Point(169, 12);
            this.LogFile.Name = "LogFile";
            this.LogFile.Size = new System.Drawing.Size(286, 20);
            this.LogFile.TabIndex = 11;
            this.LogFile.Text = global::OpalSharp.Properties.Settings.Default.LogFile;
            // 
            // LogLevel
            // 
            this.LogLevel.DataBindings.Add(new System.Windows.Forms.Binding("Value", global::OpalSharp.Properties.Settings.Default, "LogLevel", true, System.Windows.Forms.DataSourceUpdateMode.OnPropertyChanged));
            this.LogLevel.Location = new System.Drawing.Point(128, 12);
            this.LogLevel.Name = "LogLevel";
            this.LogLevel.Size = new System.Drawing.Size(35, 20);
            this.LogLevel.TabIndex = 8;
            this.LogLevel.Value = global::OpalSharp.Properties.Settings.Default.LogLevel;
            // 
            // urlToCall
            // 
            this.urlToCall.DataBindings.Add(new System.Windows.Forms.Binding("Text", global::OpalSharp.Properties.Settings.Default, "URLtoCall", true, System.Windows.Forms.DataSourceUpdateMode.OnPropertyChanged));
            this.urlToCall.Location = new System.Drawing.Point(128, 143);
            this.urlToCall.Name = "urlToCall";
            this.urlToCall.Size = new System.Drawing.Size(327, 20);
            this.urlToCall.TabIndex = 7;
            this.urlToCall.Text = global::OpalSharp.Properties.Settings.Default.URLtoCall;
            // 
            // password
            // 
            this.password.DataBindings.Add(new System.Windows.Forms.Binding("Text", global::OpalSharp.Properties.Settings.Default, "Password", true, System.Windows.Forms.DataSourceUpdateMode.OnPropertyChanged));
            this.password.Location = new System.Drawing.Point(296, 77);
            this.password.Name = "password";
            this.password.Size = new System.Drawing.Size(86, 20);
            this.password.TabIndex = 5;
            this.password.Text = global::OpalSharp.Properties.Settings.Default.Password;
            this.password.UseSystemPasswordChar = true;
            // 
            // user
            // 
            this.user.DataBindings.Add(new System.Windows.Forms.Binding("Text", global::OpalSharp.Properties.Settings.Default, "Username", true, System.Windows.Forms.DataSourceUpdateMode.OnPropertyChanged));
            this.user.Location = new System.Drawing.Point(223, 77);
            this.user.Name = "user";
            this.user.Size = new System.Drawing.Size(67, 20);
            this.user.TabIndex = 4;
            this.user.Text = global::OpalSharp.Properties.Settings.Default.Username;
            // 
            // host
            // 
            this.host.DataBindings.Add(new System.Windows.Forms.Binding("Text", global::OpalSharp.Properties.Settings.Default, "HostDomain", true, System.Windows.Forms.DataSourceUpdateMode.OnPropertyChanged));
            this.host.Location = new System.Drawing.Point(128, 77);
            this.host.Name = "host";
            this.host.Size = new System.Drawing.Size(89, 20);
            this.host.TabIndex = 3;
            this.host.Text = global::OpalSharp.Properties.Settings.Default.HostDomain;
            // 
            // RegisterStatus
            // 
            this.RegisterStatus.Location = new System.Drawing.Point(128, 103);
            this.RegisterStatus.Name = "RegisterStatus";
            this.RegisterStatus.Size = new System.Drawing.Size(327, 37);
            this.RegisterStatus.TabIndex = 16;
            this.RegisterStatus.Text = "Unregistered";
            this.RegisterStatus.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // CallStatus
            // 
            this.CallStatus.Location = new System.Drawing.Point(125, 172);
            this.CallStatus.Name = "CallStatus";
            this.CallStatus.Size = new System.Drawing.Size(330, 52);
            this.CallStatus.TabIndex = 17;
            this.CallStatus.Text = "No call in progress";
            this.CallStatus.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // RFC5626
            // 
            this.RFC5626.AutoSize = true;
            this.RFC5626.Checked = global::OpalSharp.Properties.Settings.Default.RFC5626;
            this.RFC5626.DataBindings.Add(new System.Windows.Forms.Binding("Checked", global::OpalSharp.Properties.Settings.Default, "RFC5626", true, System.Windows.Forms.DataSourceUpdateMode.OnPropertyChanged));
            this.RFC5626.Location = new System.Drawing.Point(389, 77);
            this.RFC5626.Name = "RFC5626";
            this.RFC5626.Size = new System.Drawing.Size(71, 17);
            this.RFC5626.TabIndex = 18;
            this.RFC5626.Text = "RFC5626";
            this.RFC5626.UseVisualStyleBackColor = true;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(467, 543);
            this.Controls.Add(this.RFC5626);
            this.Controls.Add(this.CallStatus);
            this.Controls.Add(this.RegisterStatus);
            this.Controls.Add(this.VideoPreview);
            this.Controls.Add(this.VideoDisplay);
            this.Controls.Add(this.Answer);
            this.Controls.Add(this.StunServer);
            this.Controls.Add(this.LogFile);
            this.Controls.Add(this.HangUp);
            this.Controls.Add(this.Unregister);
            this.Controls.Add(this.LogLevel);
            this.Controls.Add(this.urlToCall);
            this.Controls.Add(this.Call);
            this.Controls.Add(this.password);
            this.Controls.Add(this.user);
            this.Controls.Add(this.host);
            this.Controls.Add(this.Register);
            this.Controls.Add(this.Shutdown);
            this.Controls.Add(this.Initialise);
            this.Name = "Form1";
            this.Text = "OPAL";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.Form1_FormClosed);
            this.Load += new System.EventHandler(this.Form1_Load);
            ((System.ComponentModel.ISupportInitialize)(this.VideoDisplay)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.VideoPreview)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.LogLevel)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button Initialise;
        private System.Windows.Forms.Button Shutdown;
        private System.Windows.Forms.Button Register;
        private System.Windows.Forms.TextBox host;
        private System.Windows.Forms.TextBox user;
        private System.Windows.Forms.TextBox password;
        private System.Windows.Forms.Button Call;
        private System.Windows.Forms.TextBox urlToCall;
        private System.Windows.Forms.NumericUpDown LogLevel;
        private System.Windows.Forms.Button Unregister;
        private System.Windows.Forms.Button HangUp;
        private System.Windows.Forms.TextBox LogFile;
        private System.Windows.Forms.TextBox StunServer;
        private System.Windows.Forms.Button Answer;
        private System.Windows.Forms.PictureBox VideoDisplay;
        private System.Windows.Forms.PictureBox VideoPreview;
        private System.Windows.Forms.Label RegisterStatus;
        private System.Windows.Forms.Label CallStatus;
        private System.Windows.Forms.CheckBox RFC5626;
    }
}

