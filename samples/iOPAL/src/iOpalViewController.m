//
//  iOpalViewController.m
//  iOPAL
//
//  Created by Robert Jongbloed on 4/04/13.
//  Copyright (c) 2013 Vox Lucida Pty. Ltd. All rights reserved.
//

#import "iOpalViewController.h"
#import "iOpalAppDelegate.h"

#include <opal.h>


@interface iOpalViewController ()
@property (weak, nonatomic) IBOutlet UITextField *destinationField;
@property (weak, nonatomic) IBOutlet UIButton *callButton;
@property (weak, nonatomic) IBOutlet UIButton *answerButton;
@property (weak, nonatomic) IBOutlet UIButton *hangUpButton;
@property (weak, nonatomic) IBOutlet UITextView *statusField;
@property (weak, nonatomic) IBOutlet UISwitch *registerField;

- (IBAction)makeCall:(id)sender;
- (IBAction)answerCall:(id)sender;
- (IBAction)hangUpCall:(id)sender;
- (IBAction)onLogInOut:(id)sender;
- (IBAction)onDestinationChanged:(id)sender;

@end

@implementation iOpalViewController

NSString * currentCallToken;

- (void)viewDidLoad
{
  [super viewDidLoad];
  // Do any additional setup after loading the view, typically from a nib.
}


- (void)didReceiveMemoryWarning
{
  [super didReceiveMemoryWarning];
  // Dispose of any resources that can be recreated.
}


- (void)outputStatus:(NSString *)str
{
  if ([NSThread isMainThread]) {
    self.statusField.text = [self.statusField.text stringByAppendingFormat:@"\n%@", str];
    self.statusField.selectedRange = NSMakeRange(self.statusField.text.length, 0);
    self.registerField.enabled = true;
  }
  else
    [self performSelectorOnMainThread:@selector(outputStatus:) withObject:str waitUntilDone:NO];
}


-(Boolean)sendOpalMessage:(OpalMessage *)msg withReply:(OpalMessage **)reply
{
  *reply = OpalSendMessage([(iOpalAppDelegate *)[[UIApplication sharedApplication] delegate] getOpalHandle], msg);
  if (*reply == 0) {
    [self outputStatus:@"OPAL not available."];
    return false;
  }
  
  if ((*reply)->m_type != OpalIndCommandError)
    return true;
  
  [self outputStatus:[NSString stringWithFormat:@"OPAL error: %s", (*reply)->m_param.m_commandError]];
  OpalFreeMessage(*reply);
  return false;
}


-(Boolean)sendOpalMessage:(OpalMessage *)msg
{
  OpalMessage * reply;
  if (![self sendOpalMessage:msg withReply:&reply])
    return false;

  OpalFreeMessage(reply);
  return true;
}


- (IBAction)makeCall:(id)sender
{
  if ([self.destinationField.text length] == 0)
    return;
  
  [self outputStatus:[NSString stringWithFormat:@"Calling %@ ...", self.destinationField.text]];

  OpalMessage msg, *reply;
  
  OpalParamSetUpCall * call = OPALMSG_SETUP_CALL(msg);
  call->m_partyB = [self.destinationField.text UTF8String];
  if (![self sendOpalMessage:&msg withReply:&reply])
    return;
  
  self.destinationField.enabled = false;
  self.callButton.enabled = false;
  self.hangUpButton.enabled = true;
  currentCallToken = [NSString stringWithUTF8String:reply->m_param.m_callSetUp.m_callToken];
  OpalFreeMessage(reply);
}


- (IBAction)answerCall:(id)sender
{
  OpalMessage msg;
  
  OpalParamAnswerCall * call = OPALMSG_ANSWER_CALL(msg);
  call->m_callToken = [currentCallToken UTF8String];
  if ([self sendOpalMessage:&msg]) {
    [self outputStatus:@"Answering"];
    self.answerButton.enabled = false;
  }
}


- (IBAction)hangUpCall:(id)sender
{
  OpalMessage msg;
  
  OpalParamCallCleared * call = OPALMSG_CLEAR_CALL(msg);
  call->m_callToken = [currentCallToken UTF8String];
  if ([self sendOpalMessage:&msg])
    [self outputStatus:@"Hanging up"];
}


- (void)logIn
{
  // Do this in background thread as can take a few seconds
  OpalMessage msg;
  
  OpalParamGeneral * param = OPALMSG_GENERAL_PARAM(msg);
  param->m_natMethod = "STUN";
  param->m_natServer = "stun.ekiga.net";
  if (![self sendOpalMessage:&msg])
    return;
  
  OpalParamProtocol * proto = OPALMSG_PROTO_PARAM(msg);
  proto->m_userName = "opal_ios";
  proto->m_displayName = "OPAL on iOS";
  proto->m_interfaceAddresses = "*";
  if (![self sendOpalMessage:&msg])
    return;
  
  OpalParamRegistration * reg = OPALMSG_REGISTRATION(msg);
  reg->m_protocol = "sip";
  reg->m_identifier = "opal_ios";
  reg->m_hostName = "ekiga.net";
  reg->m_password = "ported";
  reg->m_timeToLive = 300;
  [self sendOpalMessage:&msg];
}


- (IBAction)onLogInOut:(id)sender
{
  if (self.registerField.on) {
    [self outputStatus:@"Log in started"];
    self.registerField.enabled = false;
    [NSThread detachNewThreadSelector:@selector(logIn) toTarget:self withObject:nil];
  }
  else {
    OpalMessage msg;
    OpalParamRegistration * reg = OPALMSG_REGISTRATION(msg);
    reg->m_protocol = OPAL_PREFIX_SIP;
    reg->m_identifier = "opal_ios";
    reg->m_hostName = "ekiga.net";
    reg->m_timeToLive = 0; // Zero is un-register
    [self sendOpalMessage:&msg];
        
    [self outputStatus:@"Logging Out"];
    self.destinationField.enabled = false;
    self.callButton.enabled = false;
    self.answerButton.enabled = false;
    self.hangUpButton.enabled = false;
  }
}


- (IBAction)onDestinationChanged:(id)sender
{
  self.callButton.enabled = self.destinationField.text.length > 0;
}


- (BOOL)textFieldShouldReturn:(UITextField *)theTextField
{
  if (theTextField == self.destinationField)
    [theTextField resignFirstResponder];
  return YES;
}


- (void)handleMessage:(NSValue *)msgPtr
{
  OpalMessage * msg = (OpalMessage *)msgPtr.pointerValue;
  
  switch (msg->m_type) {
    case OpalIndRegistration :
      switch (msg->m_param.m_registrationStatus.m_status) {
        case OpalRegisterSuccessful :
          self.destinationField.enabled = true;
          self.callButton.enabled = self.destinationField.text.length > 0;
          [self outputStatus:@"Log in successful"];
          break;
          
        case OpalRegisterFailed :
          [self outputStatus:[NSString stringWithFormat:@"Log in failed: %s",
                                   msg->m_param.m_registrationStatus.m_error]];
          break;
          
        case OpalRegisterRemoved :
          [self outputStatus:@"Log out successful"];
          break;
          
        default :
          break;
      }
      break;
      
    case OpalIndIncomingCall :
      self.callButton.enabled = false;
      self.answerButton.enabled = true;
      self.hangUpButton.enabled = true;
      [self outputStatus:[NSString stringWithFormat:@"Incoming call from %s",
                            msg->m_param.m_incomingCall.m_remoteDisplayName]];
      currentCallToken = [NSString stringWithUTF8String:msg->m_param.m_incomingCall.m_callToken];
      break;
      
    case OpalIndEstablished :
      [self outputStatus:@"Call established"];
      break;
      
    case OpalIndCallCleared :
      [self outputStatus:@"Call cleared"];
      self.callButton.enabled = true;
      self.answerButton.enabled = false;
      self.hangUpButton.enabled = false;
      currentCallToken = nullptr;
      break;
      
    default:
      break;
  }
}


@end
