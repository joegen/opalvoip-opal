#include <ptlib.h>
#include <qapplication.h>
#include <qcheckbox.h>
#include <qtextedit.h>
#include <qspinbox.h>
#include <qradiobutton.h>
#include <qcombobox.h>
#include <qmessagebox.h>
#include <qlineedit.h>

#include "main.h"
#include "options.h"
#include "makecall.h"

const char AutoAnswerConfigKey[]              = "Auto Answer";
const char EnableH323ConfigKey[]              = "Enable H323"; 
const char EnableSIPConfigKey[]               = "Enable SIP";
const char AudioJitterMinConfigKey[]          = "Min Audio Jitter";
const char AudioJitterMaxConfigKey[]          = "Max Audio Jitter";
const char SilenceDetectionConfigKey[]        = "Silence detection";
const char AudioDeviceTypeConfigKey[]         = "Audio device";
const char SoundcardRecordDeviceConfigKey[]   = "Sound card record device";
const char SoundcardPlaybackDeviceConfigKey[] = "Sound card playback device";
const char TraceEnabledConfigKey[]            = "Trace enabled"; 
const char TraceLevelConfigKey[]              = "Trace level";
const char TraceFilenameConfigKey[]           = "Trace filename";
const char TraceOptionsConfigKey[]            = "Trace options";

class OptionsDialog : public OptionsDialogBase
{
  public:
    OptionsDialog(MyOptions & _options);
    ~OptionsDialog();
    virtual void OnChangeAudioDevice();

  protected:
    MyOptions & options;
};

////////////////////////////////////////////////////////////

int main( int argc, char ** argv )
{
    OpenPhoneApp a1;
    QApplication a( argc, argv );
    MainWindow *w = new MainWindow;
    w->show();
    a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );
    return a.exec();
}

MainWindow::MainWindow()
{
  options.Load();

#if PTRACING
  myTraceFile = NULL;
  OpenTraceFile(options);
#endif

  opal = new MyManager;
  opal->Initialise(options);
}

MainWindow::~MainWindow()
{
  delete opal;
}

void MainWindow::OnViewOptions()
{
  OptionsDialog dlg(options);

  if (!dlg.exec())
    return;

  opal->Initialise(options);
  OpenTraceFile(options);
}

void MainWindow::OnMakeCall()
{
  MakeCallDialog dlg(this, "new", TRUE );

  if (!dlg.exec())
    return;

  PString destinationAddress = PString(dlg.destinationAddress->currentText());

  if (!opal->MakeCall(destinationAddress, currentCallToken)) {
    PString str = psprintf("Cannot make call to " + destinationAddress);
    QMessageBox::critical(0, "Call failed", QString(str), QMessageBox::Ok, 0);
  }
}

void MainWindow::UpdateCallStatus()
{
  if (currentCallToken.IsEmpty()) {
  } 
  
  else {
  }
}

BOOL MainWindow::OpenTraceFile(const MyOptions & options)
{
  PTrace::SetLevel(options.traceLevel);
  PTrace::ClearOptions(0x1ff);
  PTrace::SetOptions(options.traceOptions);

  PString traceFileName = options.traceFilename;

  // If already have a trace file, see if need to close it
  if (myTraceFile != NULL) {
    // If no change, do nothing more
    if (myTraceFile->GetFilePath() == PFilePath(traceFileName))
      return TRUE;

    PTrace::SetStream(NULL);
    delete myTraceFile;
    myTraceFile = NULL;
  }

  // Have stopped
  if (traceFileName.IsEmpty())
    return TRUE;

  PTextFile * traceFile = new PTextFile;
  if (traceFile->Open(traceFileName, PFile::WriteOnly)) {
    myTraceFile = traceFile;
    PTrace::SetStream(traceFile);
/*
    PProcess & process = PProcess::Current();
    PTRACE(0, process.GetName()
           << " Version " << process.GetVersion(TRUE)
           << " by " << process.GetManufacturer()
           << " on " << process.GetOSClass() << ' ' << process.GetOSName()
           << " (" << process.GetOSVersion() << '-' << process.GetOSHardware() << ')');
*/
    return TRUE;
  }

  QMessageBox::critical(0, "Trace failed", QString((const char *)traceFile->GetName()), QMessageBox::Ok, 0);
  delete traceFile;
  return FALSE;
}

//////////////////////////////////////////////

OptionsDialog::OptionsDialog(MyOptions & _options)
  : OptionsDialogBase(0, 0, TRUE), options(_options)
{
  // general
  autoAnswer->setChecked(options.autoAnswer);
  enableH323->setChecked(options.enableH323);
  enableSIP->setChecked(options.enableSIP);

  // audio device
  PINDEX i;
  PStringArray names = PSoundChannel::GetDeviceNames(PSoundChannel::Recorder);
  for (i = 0;i < names.GetSize(); i++)
    soundcardPlaybackDevice->insertItem(QString(names[i]));
  if (options.soundcardPlaybackDevice.IsEmpty())
    soundcardPlaybackDevice->setCurrentItem(0);
  else
    soundcardPlaybackDevice->setCurrentText(QString(options.soundcardPlaybackDevice));

  names = PSoundChannel::GetDeviceNames(PSoundChannel::Player);
  for (i = 0;i < names.GetSize(); i++)
    soundcardRecordDevice->insertItem(QString(names[i]));
  if (options.soundcardRecordDevice.IsEmpty())
    soundcardRecordDevice->setCurrentItem(0);
  else
    soundcardRecordDevice->setCurrentText(QString(options.soundcardRecordDevice));

  switch (options.audioDevice) {
    case MyOptions::Quicknet:
      quicknetAudioDevice->setChecked(TRUE);
      break;
    case MyOptions::VOIPBlaster:
      voipblasterAudioDevice->setChecked(TRUE);
      break;
    case MyOptions::SoundCard:
    default:
      soundAudioDevice->setChecked(TRUE);

      break;
  }
  OnChangeAudioDevice();

  // audio codec
  minJitter->setValue(options.minJitter);
  maxJitter->setValue(options.maxJitter);
  silenceDetection->setChecked(options.silenceDetection);

  // trace
  traceEnabled    ->setChecked(options.traceEnabled);
  traceLevel      ->setValue(options.traceLevel);
  traceFilename   ->setText(QString((const char *)options.traceFilename));

  traceBlocks     ->setChecked(options.traceOptions & PTrace::Blocks);
  traceDateAndTime->setChecked(options.traceOptions & PTrace::DateAndTime);
  traceThreads    ->setChecked(options.traceOptions & PTrace::Thread);
  traceLevelText  ->setChecked(options.traceOptions & PTrace::TraceLevel);
  traceFileAndLine->setChecked(options.traceOptions & PTrace::FileAndLine);
  traceAddress    ->setChecked(options.traceOptions & PTrace::ThreadAddress);
  traceTimeStamp  ->setChecked(options.traceOptions & PTrace::Timestamp);
}

OptionsDialog::~OptionsDialog()
{
  // general
  options.autoAnswer = autoAnswer->isChecked();
  options.enableH323 = enableH323->isChecked();
  options.enableSIP  = enableSIP->isChecked();

  // audio codec
  options.minJitter        = minJitter->value();
  options.maxJitter        = maxJitter->value();
  options.silenceDetection = silenceDetection->isChecked();

  // trace
  options.traceEnabled  = traceEnabled->isChecked();
  options.traceFilename = traceFilename->text();
  options.traceLevel    = traceLevel->value();
  options.traceOptions  = traceBlocks->isChecked()      ? PTrace::Blocks : 0;
  options.traceOptions |= traceDateAndTime->isChecked() ? PTrace::DateAndTime : 0;
  options.traceOptions |= traceThreads->isChecked()     ? PTrace::Thread : 0;
  options.traceOptions |= traceLevelText->isChecked()   ? PTrace::TraceLevel : 0;
  options.traceOptions |= traceFileAndLine->isChecked() ? PTrace::FileAndLine : 0;
  options.traceOptions |= traceAddress->isChecked()     ? PTrace::ThreadAddress : 0;
  options.traceOptions |= traceTimeStamp->isChecked()   ? PTrace::Timestamp : 0;

  options.Save();
}

void OptionsDialog::OnChangeAudioDevice()
{
}

//////////////////////////////////////////////

MyManager::MyManager()
{
  h323EP = NULL;
  sipEP  = NULL;
  pcssEP = NULL;
}

MyManager::~MyManager()
{
  delete h323EP;
  delete sipEP;
  delete pcssEP;
}

BOOL MyManager::Initialise(const MyOptions & options)
{
  // Clear route table while devices are rearranged
  SetRouteTable(PStringArray());

  // set audio parameters
  SetAudioJitterDelay(options.minJitter, options.maxJitter);

/*
  if (args.HasOption('D'))
    SetMediaFormatMask(args.GetOptionString('D').Lines());
  if (args.HasOption('P'))
    SetMediaFormatOrder(args.GetOptionString('P').Lines());
*/

/*
  if (args.HasOption("tcp-base"))
    SetTCPPorts(args.GetOptionString("tcp-base").AsUnsigned(),
                args.GetOptionString("tcp-max").AsUnsigned());

  if (args.HasOption("udp-base"))
    SetUDPPorts(args.GetOptionString("udp-base").AsUnsigned(),
                args.GetOptionString("udp-max").AsUnsigned());

  if (args.HasOption("rtp-base"))
    SetRtpIpPorts(args.GetOptionString("rtp-base").AsUnsigned(),
                  args.GetOptionString("rtp-max").AsUnsigned());

  if (args.HasOption("rtp-tos")) {
    unsigned tos = args.GetOptionString("rtp-tos").AsUnsigned();
    if (tos > 255) {
      cerr << "IP Type Of Service bits must be 0 to 255." << endl;
      return FALSE;
    }
    SetRtpIpTypeofService(tos);
  }
*/

  ///////////////////////////////////////
  // Create PC Sound System handler

  switch (options.audioDevice) {
    case MyOptions::Quicknet:
      if (pcssEP != NULL) {
        delete pcssEP;
        pcssEP = NULL;
      }
      break;

    case MyOptions::VOIPBlaster:
      if (pcssEP != NULL) {
        delete pcssEP;
        pcssEP = NULL;
      }
      break;

    case MyOptions::SoundCard:
    default:
      if (pcssEP == NULL)
        pcssEP = new MyPCSSEndPoint(*this);

      pcssEP->SetSoundDevice(options.soundcardRecordDevice,   PSoundChannel::Recorder);
      pcssEP->SetSoundDevice(options.soundcardPlaybackDevice, PSoundChannel::Player);
  }

  ///////////////////////////////////////
  // Open the LID if parameter provided, create LID based endpoint

#if 0
#ifdef HAS_IXJ
  if (!args.HasOption('Q')) {
    PStringArray devices = args.GetOptionString('q').Lines();
    if (devices.IsEmpty() || devices[0] == "ALL")
      devices = OpalIxJDevice::GetDeviceNames();
    for (PINDEX d = 0; d < devices.GetSize(); d++) {
      OpalIxJDevice * ixj = new OpalIxJDevice;
      if (ixj->Open(devices[d])) {
        // Create LID protocol handler, automatically adds to manager
        if (potsEP == NULL)
          potsEP = new OpalPOTSEndPoint(*this);
        if (potsEP->AddDevice(ixj))
          cout << "Quicknet device \"" << devices[d] << "\" added." << endl;
      }
      else {
        cerr << "Could not open device \"" << devices[d] << '"' << endl;
        delete ixj;
      }
    }
  }
#endif
#endif


  ///////////////////////////////////////
  // Create H.323 protocol handler

  if (!options.enableH323) {
    if (h323EP != NULL) {
      delete h323EP;
      h323EP = NULL;
    }
  } else {
    if (h323EP == NULL)
      h323EP = new H323EndPoint(*this);

/*
    // Get local username, multiple uses of -u indicates additional aliases
    if (args.HasOption('u')) {
      PStringArray aliases = args.GetOptionString('u').Lines();
      h323EP->SetLocalUserName(aliases[0]);
      for (PINDEX i = 1; i < aliases.GetSize(); i++)
        h323EP->AddAliasName(aliases[i]);
    }

    if (args.HasOption('b')) {
      unsigned initialBandwidth = args.GetOptionString('b').AsUnsigned()*100;
      if (initialBandwidth == 0) {
        cerr << "Illegal bandwidth specified." << endl;
        return FALSE;
      }
      h323EP->SetInitialBandwidth(initialBandwidth);
    }

    h323EP->SetGkAccessTokenOID(args.GetOptionString("gk-token"));

    // Start the listener thread for incoming calls.
    if (args.HasOption("h323-listen")) {
      PStringArray listeners = args.GetOptionString("h323-listen").Lines();
      if (!h323EP->StartListeners(listeners)) {
        cerr <<  "Could not open any H.323 listener from "
             << setfill(',') << listeners << endl;
        return FALSE;
      }
    }
    else 
*/
    {
      if (!h323EP->StartListener("*")) {
        cerr <<  "Could not open H.323 listener on TCP port "
             << h323EP->GetDefaultSignalPort() << endl;
        return FALSE;
      }
    }

/*
    if (args.HasOption('p'))
      h323EP->SetGatekeeperPassword(args.GetOptionString('p'));

    // Establish link with gatekeeper if required.
    if (args.HasOption('g')) {
      PString gkName = args.GetOptionString('g');
      OpalTransportUDP * rasChannel;
      if (args.GetOptionString("h323-listen").IsEmpty())
        rasChannel  = new OpalTransportUDP(*h323EP);
      else {
        PIPSocket::Address interfaceAddress(args.GetOptionString("h323-listen"));
        rasChannel  = new OpalTransportUDP(*h323EP, interfaceAddress);
      }
      if (h323EP->SetGatekeeper(gkName, rasChannel))
        cout << "Gatekeeper set: " << *h323EP->GetGatekeeper() << endl;
      else {
        cerr << "Error registering with gatekeeper at \"" << gkName << '"' << endl;
        return FALSE;
      }
    }
    else if (!args.HasOption('n') || args.HasOption('R')) {
      cout << "Searching for gatekeeper..." << flush;
      if (h323EP->DiscoverGatekeeper(new OpalTransportUDP(*h323EP)))
        cout << "\nGatekeeper found: " << *h323EP->GetGatekeeper() << endl;
      else {
        cerr << "\nNo gatekeeper found." << endl;
        if (args.HasOption("require-gatekeeper")) 
          return FALSE;
      }
    }
*/
  }

  ///////////////////////////////////////
  // Create SIP protocol handler

  if (!options.enableSIP) {
    if (sipEP != NULL) {
      delete sipEP;
      sipEP = NULL;
    }
  } else {
    if (sipEP == NULL)
      sipEP = new SIPEndPoint(*this);

/*
    // set MIME format
    sipEP->SetMIMEForm(args.HasOption("use-long-mime"));

    // Get local username, multiple uses of -u indicates additional aliases
    if (args.HasOption('u')) {
      PStringArray aliases = args.GetOptionString('u').Lines();
      sipEP->SetRegistrationName(aliases[0]);
    }
    if (args.HasOption('p'))
      sipEP->SetRegistrationPassword(args.GetOptionString('p'));

    // Start the listener thread for incoming calls.
    if (args.HasOption("sip-listen")) {
      PStringArray listeners = args.GetOptionString("sip-listen").Lines();
      if (!sipEP->StartListeners(listeners)) {
        cerr <<  "Could not open any SIP listener from "
             << setfill(',') << listeners << endl;
        return FALSE;
      }
    }
    else 
*/
    {
      if (!sipEP->StartListener("udp$*")) {
        cerr <<  "Could not open SIP listener on UDP port "
             << sipEP->GetDefaultSignalPort() << endl;
        return FALSE;
      }
      if (!sipEP->StartListener("tcp$*")) {
        cerr <<  "Could not open SIP listener on TCP port "
             << sipEP->GetDefaultSignalPort() << endl;
        return FALSE;
      }
    }

//    if (args.HasOption('r'))
//      sipEP->Register(args.GetOptionString('r'));
  }




  ///////////////////////////////////////
  // Create IVR protocol handler
/*
  if (!args.HasOption('V')) {
    ivrEP = new OpalIVREndPoint(*this);
    if (args.HasOption('x'))
      ivrEP->SetDefaultVXML(args.GetOptionString('x'));
  }
*/

  ///////////////////////////////////////
  // Set the dial peers
/*
  if (args.HasOption('d')) {
    if (!SetRouteTable(args.GetOptionString('d').Lines())) {
      cerr <<  "No legal entries in dial peer!" << endl;
      return FALSE;
    }
  }

  if (!args.HasOption("no-std-dial-peer")) {
    if (sipEP != NULL) {
      AddRouteEntry("pots:.*\\*.*\\*.* = sip:<dn2ip>");
      AddRouteEntry("pots:.*           = sip:<da>");
      AddRouteEntry("pc:.*             = sip:<da>");
    }
    else if (h323EP != NULL) {
      AddRouteEntry("pots:.*\\*.*\\*.* = h323:<dn2ip>");
      AddRouteEntry("pots:.*           = h323:<da>");
      AddRouteEntry("pc:.*             = h323:<da>");
    }

    if (ivrEP != NULL) {
      AddRouteEntry("h323:# = ivr:");
      AddRouteEntry("sip:#  = ivr:");
    }

    if (potsEP != NULL) {
      AddRouteEntry("h323:.* = pots:<da>");
      AddRouteEntry("sip:.*  = pots:<da>");
    }
    else if (pcssEP != NULL) {
      AddRouteEntry("h323:.* = pc:<da>");
      AddRouteEntry("sip:.*  = pc:<da>");
    }
  }
*/
  if (sipEP != NULL) {
    //AddRouteEntry("pots:.*\\*.*\\*.* = sip:<dn2ip>");
    //AddRouteEntry("pots:.*           = sip:<da>");
    if (pcssEP != NULL)
      AddRouteEntry("pc:.*             = sip:<da>");
  }
  else if (h323EP != NULL) {
    //AddRouteEntry("pots:.*\\*.*\\*.* = h323:<dn2ip>");
    //AddRouteEntry("pots:.*           = h323:<da>");
    if (pcssEP != NULL)
      AddRouteEntry("pc:.*             = h323:<da>");
  }

  if (pcssEP != NULL) {
    if (h323EP != NULL)
      AddRouteEntry("h323:.* = pc:<da>");
    if (sipEP != NULL)
      AddRouteEntry("sip:.*  = pc:<da>");
  }

  return TRUE;
}

BOOL MyManager::MakeCall(const PString & dest, PString & token)
{
  PString src;
  if (pcssEP != NULL)
    src = "pc:*";

  return SetUpCall(src, dest, token);
}


void MyManager::OnEstablishedCall(OpalCall & /*call*/)
{
}

void MyManager::OnClearedCall(OpalCall & /*call*/)
{
}

BOOL MyManager::OnOpenMediaStream(OpalConnection & /*connection*/,  /// Connection that owns the media stream
                                  OpalMediaStream & /*stream*/)    /// New media stream being opened
{
  return FALSE;
}

void MyManager::OnUserInputString(OpalConnection & /*connection*/,  /// Connection input has come from
                                   const PString & /*value*/)       /// String value of indication
{
}


//////////////////////////////////////////////////////////////////////

MyOptions::MyOptions()
{
  // general
  autoAnswer = FALSE;
  enableSIP  = TRUE;
  enableH323 = TRUE;

  // audio device
  audioDevice = (AudioDevice)SoundCard;
  soundcardRecordDevice   = PString::Empty();
  soundcardPlaybackDevice = PString::Empty();

  // audio codec
  minJitter        = 30;
  maxJitter        = 120;
  silenceDetection = TRUE;

  // trace
  traceEnabled   = FALSE;
  traceLevel     = 1;
  traceFilename  = "log.txt";
  traceOptions   = PTrace::Timestamp;
}

BOOL MyOptions::Load()
{
  PConfig config;

  // general
  autoAnswer = config.GetBoolean(AutoAnswerConfigKey, FALSE);
  enableSIP  = config.GetBoolean(EnableH323ConfigKey, TRUE);
  enableH323 = config.GetBoolean(EnableSIPConfigKey,  TRUE);

  // audio device
  audioDevice             = (AudioDevice)config.GetInteger(AudioDeviceTypeConfigKey, SoundCard);
  soundcardRecordDevice   = config.GetString(SoundcardRecordDeviceConfigKey);
  soundcardPlaybackDevice = config.GetString(SoundcardPlaybackDeviceConfigKey);

  // audio codec
  minJitter        = config.GetInteger(AudioJitterMinConfigKey, 30);
  maxJitter        = config.GetInteger(AudioJitterMaxConfigKey, 120);
  silenceDetection = config.GetBoolean(SilenceDetectionConfigKey, TRUE);

  // trace
  traceEnabled     = config.GetBoolean(TraceEnabledConfigKey, FALSE);
  traceLevel       = config.GetInteger(TraceLevelConfigKey, 1);
  traceFilename    = config.GetString(TraceFilenameConfigKey, "log.txt");
  traceOptions     = config.GetInteger(TraceOptionsConfigKey, PTrace::Timestamp);

  return TRUE;
}

BOOL MyOptions::Save()
{
  PConfig config;

  // general
  config.SetBoolean(AutoAnswerConfigKey, autoAnswer);
  config.SetBoolean(EnableH323ConfigKey, enableSIP);
  config.SetBoolean(EnableSIPConfigKey,  enableH323);

  // audio device
  config.SetInteger(AudioDeviceTypeConfigKey,         audioDevice);
  config.SetString (SoundcardRecordDeviceConfigKey,   soundcardRecordDevice);
  config.SetString (SoundcardPlaybackDeviceConfigKey, soundcardPlaybackDevice);

  // audio codec
  config.SetInteger(AudioJitterMinConfigKey, minJitter);
  config.SetInteger(AudioJitterMaxConfigKey, maxJitter);
  config.SetBoolean(SilenceDetectionConfigKey, silenceDetection);

  // trace
  config.SetBoolean(TraceEnabledConfigKey, traceEnabled);
  config.SetInteger(TraceLevelConfigKey,   traceLevel);
  config.SetString(TraceFilenameConfigKey, traceFilename);
  config.SetInteger(TraceOptionsConfigKey, traceOptions);

  return TRUE;
}

//////////////////////////////////////////////////////////////////////

MyPCSSEndPoint::MyPCSSEndPoint(MyManager & mgr)
  : OpalPCSSEndPoint(mgr)
{
}

BOOL MyPCSSEndPoint::SetSoundDevice(const PString & _name, PSoundChannel::Directions dir)
{
  PString name = _name;
  PStringArray names = PSoundChannel::GetDeviceNames(dir);
  if (name.IsEmpty()) {
    if (names.GetSize() == 0)
      return FALSE;
    name = names[0];
  }

  if (names.GetStringsIndex(name) != P_MAX_INDEX)
    return FALSE;

  if (dir == PSoundChannel::Player) {
    if (SetSoundChannelPlayDevice(name))
      return TRUE;
  }
  else {
    if (SetSoundChannelRecordDevice(name))
      return TRUE;
  }

  return FALSE;
}

PString MyPCSSEndPoint::OnGetDestination(const OpalPCSSConnection & /*connection*/)
{
  return PString();
}

void MyPCSSEndPoint::OnShowIncoming(const OpalPCSSConnection & /*connection*/)
{
}

BOOL MyPCSSEndPoint::OnShowOutgoing(const OpalPCSSConnection & /*connection*/)
{
  return TRUE;
}

