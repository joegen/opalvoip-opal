/*
 * main.c
 *
 * An example of the "C" interface to OPAL
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 2008 Vox Lucida
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

#define _CRT_NONSTDC_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <opal.h>


#ifdef _DEBUG
#define OPAL_DLL "OPALd.DLL"
#else
#define OPAL_DLL "OPAL.DLL"
#endif

HANDLE                  hDLL;
OpalInitialiseFunction  InitialiseFunction;
OpalShutDownFunction    ShutDownFunction;
OpalGetMessageFunction  GetMessageFunction;
OpalSendMessageFunction SendMessageFunction;
OpalFreeMessageFunction FreeMessageFunction;
OpalHandle              hOPAL;

char * CurrentCallToken;
char * HeldCallToken;



OpalMessage * MySendCommand(OpalMessage * command, const char * errorMessage)
{
  OpalMessage * response;
  if ((response = SendMessageFunction(hOPAL, command)) == NULL)
    return NULL;
  if (response->m_type != OpalIndCommandError)
    return response;

  if (response->m_param.m_commandError == NULL || *response->m_param.m_commandError == '\0')
    printf("%s.\n", errorMessage);
  else
    printf("%s: %s\n", errorMessage, response->m_param.m_commandError);

  FreeMessageFunction(response);

  return NULL;
}


int InitialiseOPAL()
{
  OpalMessage   command;
  OpalMessage * response;

  if ((hDLL = LoadLibrary(OPAL_DLL)) == NULL) {
    fprintf(stderr, "Could not file %s\n", OPAL_DLL);
    return 0;
  }

  InitialiseFunction  = (OpalInitialiseFunction )GetProcAddress(hDLL, OPAL_INITIALISE_FUNCTION  );
  ShutDownFunction    = (OpalShutDownFunction   )GetProcAddress(hDLL, OPAL_SHUTDOWN_FUNCTION    );
  GetMessageFunction  = (OpalGetMessageFunction )GetProcAddress(hDLL, OPAL_GET_MESSAGE_FUNCTION );
  SendMessageFunction = (OpalSendMessageFunction)GetProcAddress(hDLL, OPAL_SEND_MESSAGE_FUNCTION);
  FreeMessageFunction = (OpalFreeMessageFunction)GetProcAddress(hDLL, OPAL_FREE_MESSAGE_FUNCTION);

  if (InitialiseFunction  == NULL ||
      ShutDownFunction    == NULL ||
      GetMessageFunction  == NULL ||
      SendMessageFunction == NULL ||
      FreeMessageFunction == NULL) {
    fputs("OPAL.DLL is invalid\n", stderr);
    return 0;
  }


  ///////////////////////////////////////////////
  // Initialisation

  if ((hOPAL = InitialiseFunction("pc h323 sip TraceLevel=4")) == NULL) {
    fputs("Could not initialise OPAL\n", stderr);
    return 0;
  }


  // General options
  memset(&command, 0, sizeof(command));
  command.m_type = OpalCmdSetGeneralParameters;

  if ((response = MySendCommand(&command, "Could not set general options")) == NULL)
    return 0;

  FreeMessageFunction(response);

  // Options across all protocols
  memset(&command, 0, sizeof(command));
  command.m_type = OpalCmdSetProtocolParameters;

  command.m_param.m_protocol.m_name = "robertj";
  command.m_param.m_protocol.m_displayName = "Robert Jongbloed";
  command.m_param.m_protocol.m_interfaceAddresses = "*";

  if ((response = MySendCommand(&command, "Could not set protocol options")) == NULL)
    return 0;

  FreeMessageFunction(response);

  return 1;
}


static void HandleMessages(unsigned timeout)
{
  OpalMessage command;
  OpalMessage * response;
  OpalMessage * message;
    

  while ((message = GetMessageFunction(hOPAL, timeout)) != NULL) {
    switch (message->m_type) {
      case OpalIndRegistration :
        if (message->m_param.m_registrationStatus.m_error == NULL ||
            message->m_param.m_registrationStatus.m_error[0] == '\0')
          puts("Registered.\n");
        else
          printf("Registration error: %s\n", message->m_param.m_registrationStatus.m_error);
        break;

      case OpalIndIncomingCall :
        if (CurrentCallToken == NULL) {
          memset(&command, 0, sizeof(command));
          command.m_type = OpalCmdClearCall;
          command.m_param.m_callToken = message->m_param.m_incomingCall.m_callToken;
          if ((response = MySendCommand(&command, "Could not answer call")) != NULL)
            FreeMessageFunction(response);
        }
        break;

      case OpalIndAlerting :
        puts("Ringing.\n");
        break;

      case OpalIndEstablished :
        puts("Established.\n");
        break;

      case OpalIndUserInput :
        if (message->m_param.m_userInput.m_userInput != NULL)
          printf("User Input: %s.\n", message->m_param.m_userInput.m_userInput);
        break;

      case OpalIndCallCleared :
        if (message->m_param.m_callCleared.m_reason == NULL)
          puts("Call cleared.\n");
        else
          printf("Call cleared: %s\n", message->m_param.m_callCleared.m_reason);
    }

    FreeMessageFunction(message);
  }
}


int DoCall(const char * to)
{
  OpalMessage command;
  OpalMessage * response;


  printf("Calling %s\n", to);

  memset(&command, 0, sizeof(command));
  command.m_type = OpalCmdSetUpCall;
  command.m_param.m_callSetUp.m_partyB = to;
  if ((response = MySendCommand(&command, "Could not make call")) == NULL)
    return 0;

  CurrentCallToken = strdup(response->m_param.m_callSetUp.m_callToken);
  FreeMessageFunction(response);
  return 1;
}


int DoHold()
{
  OpalMessage command;
  OpalMessage * response;


  printf("Hold\n");

  memset(&command, 0, sizeof(command));
  command.m_type = OpalCmdHoldCall;
  command.m_param.m_callToken = CurrentCallToken;
  if ((response = MySendCommand(&command, "Could not hold call")) == NULL)
    return 0;

  HeldCallToken = CurrentCallToken;
  CurrentCallToken = NULL;

  FreeMessageFunction(response);
  return 1;
}


int DoTransfer(const char * to)
{
  OpalMessage command;
  OpalMessage * response;


  printf("Transferring to %s\n", to);

  memset(&command, 0, sizeof(command));
  command.m_type = OpalCmdTransferCall;
  command.m_param.m_callSetUp.m_partyB = to;
  command.m_param.m_callSetUp.m_callToken = CurrentCallToken;
  if ((response = MySendCommand(&command, "Could not transfer call")) == NULL)
    return 0;

  FreeMessageFunction(response);
  return 1;
}


typedef enum
{
  OpListen,
  OpCall,
  OpHold,
  OpTransfer,
  OpConsult,
  NumOperations
} Operations;

static const char * const OperationNames[NumOperations] =
  { "listen", "call", "hold", "transfer", "consult" };

static int const RequiredArgsForOperation[NumOperations] =
  { 2, 3, 3, 4, 4 };


static Operations GetOperation(const char * name)
{
  Operations op;

  for (op = OpListen; op < NumOperations; op++) {
    if (strcmp(name, OperationNames[op]) == 0)
      break;
  }

  return op;
}


int main(int argc, char * argv[])
{
  Operations operation;

  
  if (argc < 2 || (operation = GetOperation(argv[1])) == NumOperations || argc < RequiredArgsForOperation[operation]) {
    fputs("usage: c_api { listen | call | transfer } [ A-party [ B-party ] ]\n", stderr);
    return 1;
  }

  puts("Initialising.\n");

  if (!InitialiseOPAL())
    return 1;

  switch (operation) {
    case OpListen :
      puts("Listening.\n");
      HandleMessages(60000);
      break;

    case OpCall :
      if (!DoCall(argv[2]))
        break;
      HandleMessages(15000);
      break;

    case OpHold :
      if (!DoCall(argv[2]))
        break;
      HandleMessages(15000);
      if (!DoHold())
        break;
      HandleMessages(15000);
      break;

    case OpTransfer :
      if (!DoCall(argv[2]))
        break;
      HandleMessages(15000);
      if (!DoTransfer(argv[3]))
        break;
      HandleMessages(15000);
      break;

    case OpConsult :
      if (!DoCall(argv[2]))
        break;
      HandleMessages(15000);
      if (!DoHold())
        break;
      HandleMessages(15000);
      if (!DoCall(argv[3]))
        break;
      HandleMessages(15000);
      if (!DoTransfer(HeldCallToken))
        break;
      HandleMessages(15000);
      break;
  }

  puts("Exiting.\n");

  ShutDownFunction(hOPAL);
  return 0;
}



// End of File ///////////////////////////////////////////////////////////////
