/*
 * capi_ep.cxx
 *
 * ISDN via CAPI EndPoint
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 2010 Vox Lucida Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <lids/capi_ep.h>

#if OPAL_CAPI

  #ifdef _MSC_VER
    #pragma message("CAPI ISDN support enabled")
  #endif

#ifdef _WIN32
#include "capi_win32.h"
#else
#include "capi_linux.h"
#endif


PURL_LEGACY_SCHEME(isdn, true, false, true, true, false, false, true, false, false, false, 0)

enum {
    MaxLineCount = 30,
    MaxBlockCount = 2,
    MaxBlockSize = 160
};


// For portability reasons we have to define the message structures ourselves!

#pragma pack(1)

struct OpalCapiMessage
{
  struct Header {
    WORD  m_Length;
    WORD  m_ApplId;
    BYTE  m_Command;
    BYTE  m_Subcommand;
    WORD  m_Number;
  } header;

  union Params {
    struct ListenReq {
      DWORD m_Controller;
      DWORD m_InfoMask;
      DWORD m_CIPMask;
      DWORD m_CIPMask2;
    } listen_req;

    struct ListenConf {
      DWORD m_Controller;
      WORD  m_Info;
    } listen_conf;

    struct ConnectReq {
      DWORD m_Controller;
      WORD  m_CIPValue;
    } connect_req;

    struct ConnectConf {
      DWORD m_PLCI;
      WORD  m_Info;
    } connect_conf;

    struct ConnectInd {
      DWORD m_PLCI;
      WORD  m_CIPValue;
    } connect_ind;

    struct ConnectResp {
      DWORD m_PLCI;
      WORD  m_Reject;
    } connect_resp;

    struct AlertReq {
      DWORD m_PLCI;
    } alert_req;

    struct ConnectActiveInd {
      DWORD m_PLCI;
    } connect_active_ind;

    struct ConnectActiveResp {
      DWORD m_PLCI;
    } connect_active_resp;

    struct InfoInd {
      DWORD m_PLCI;
      WORD  m_Number;
    } info_ind;

    struct InfoResp {
      DWORD m_PLCI;
    } info_resp;

    struct ConnectB3Req {
      DWORD m_PLCI;
    } connect_b3_req;

    struct ConnectB3Conf {
      DWORD m_NCCI;
      WORD  m_Info;
    } connect_b3_conf;

    struct ConnectB3Ind {
      DWORD m_NCCI;
    } connect_b3_ind;

    struct ConnectB3Resp {
      DWORD m_NCCI;
      WORD  m_Reject;
    } connect_b3_resp;

    struct ConnectB3ActiveInd {
      DWORD m_NCCI;
    } connect_b3_active_ind;

    struct ConnectB3ActiveResp {
      DWORD m_NCCI;
    } connect_b3_active_resp;

    struct DataB3Req {
      DWORD   m_NCCI;
      DWORD   m_Data;
      WORD    m_Length;
      WORD    m_Handle;
      WORD    m_Flags;
      PUInt64 m_Data64;
#if P_64BIT
      void SetPtr(const void * ptr) { m_Data64 = (PUInt64)ptr; }
#else
      void SetPtr(const void * ptr) { m_Data = (DWORD)ptr; }
#endif
    } data_b3_req;

    struct DataB3Conf {
      DWORD   m_NCCI;
      WORD    m_Handle;
      WORD    m_Info;
    } data_b3_conf;

    struct DataB3Ind {
      DWORD   m_NCCI;
      DWORD   m_Data;
      WORD    m_Length;
      WORD    m_Handle;
      WORD    m_Flags;
      PUInt64 m_Data64;
#if P_64BIT
      void * GetPtr() const { return (void *)m_Data64; }
#else
      void * GetPtr() const { return (void *)m_Data; }
#endif
    } data_b3_ind;

    struct DataB3Resp {
      DWORD   m_NCCI;
      WORD    m_Handle;
    } data_b3_resp;

    struct DisconnectB3Req {
      DWORD m_NCCI;
    } disconnect_b3_req;

    struct DisconnectB3Conf {
      DWORD m_NCCI;
    } disconnect_b3_conf;

    struct DisconnectB3Ind {
      DWORD m_NCCI;
    } disconnect_b3_ind;

    struct DisconnectB3Resp {
      DWORD m_NCCI;
    } disconnect_b3_resp;

    struct DisconnectReq {
      DWORD m_PLCI;
    } disconnect_req;

    struct DisconnectInd {
      DWORD m_PLCI;
      WORD  m_Reason;
    } disconnect_ind;

    struct DisconnectResp {
      DWORD m_PLCI;
    } disconnect_resp;

    DWORD m_Controller;
    DWORD m_PLCI;
    DWORD m_NCCI;

    BYTE m_Params[200]; // Space for command specific parameters
  } param;

  OpalCapiMessage(unsigned command, unsigned subcommand, size_t fixedParamSize)
  {
    header.m_Length = (WORD)(sizeof(Header)+fixedParamSize);
    header.m_ApplId = 0;
    header.m_Command = (BYTE)command;
    header.m_Subcommand = (BYTE)subcommand;
    header.m_Number = 0;
    memset(&param, 0, sizeof(param));
  }

  void Add(const char * value)
  {
    Add((const BYTE *)value, strlen(value));
  }

  void Add(unsigned prefix, const char * value)
  {
    size_t length = strlen(value);
    BYTE * param = ((BYTE *)this) + header.m_Length;
    *param++ = (BYTE)(length+1);
    *param++ = (BYTE)prefix;
    if (length > 0)
      memcpy(param, value, length);
    header.m_Length = (WORD)(header.m_Length + length + 2);
  }

  void Add(unsigned prefix1, unsigned prefix2, const char * value)
  {
    size_t length = strlen(value);
    BYTE * param = ((BYTE *)this) + header.m_Length;
    *param++ = (BYTE)(length+2);
    *param++ = (BYTE)prefix1;
    *param++ = (BYTE)prefix2;
    if (length > 0)
      memcpy(param, value, length);
    header.m_Length = (WORD)(header.m_Length + length + 3);
  }

  void Add(const void * value, size_t length)
  {
    BYTE * param = ((BYTE *)this) + header.m_Length;
    *param++ = (char)length;
    if (length > 0)
      memcpy(param, value, length);
    header.m_Length = (WORD)(header.m_Length + length + 1);
  }

  void AddEmpty()
  {
    ++header.m_Length;
  }

  bool Get(PINDEX & pos, BYTE * value, size_t size, PString & str) const
  {
    if (pos+size >= header.m_Length)
      return false;

    const BYTE * param = ((const BYTE *)this) + pos;
    size_t length = *param++;
    if (length < size) {
      pos += length + 1;
      return false;
    }

    if (value != NULL)
      memcpy(value, param, size);

    str = PString((const char *)param + size, length - size);
    pos += length + 1;
    return true;
  }

  void Skip(PINDEX & pos) const
  {
    pos += (*(const BYTE *)this) + 1;
  }
};


struct OpalCapiProfile {
  WORD  m_NumControllers;     // # of installed Controllers
  WORD  m_NumBChannels;       // # of supported B-channels
  DWORD m_GlobalOptions;      // Global options
  DWORD m_B1ProtocolOptions;  // B1 protocol support
  DWORD m_B2ProtocolOptions;  // B2 protocol support
  DWORD m_B3ProtocolOptions;  // B3 protocol support
  BYTE  m_Reserved[64];       // Make sure struct is big enough
};

#pragma pack()

#if PTRACING
  #define MAKE_MSG_NAME(cmd,sub) { CAPICMD(CAPI_##cmd, CAPI_##sub), #cmd "_" #sub }

  ostream & operator<<(ostream & strm, const OpalCapiMessage & message)
  {
    static POrdinalToString::Initialiser const MsgNameInit[] = {
      MAKE_MSG_NAME(ALERT,REQ),
      MAKE_MSG_NAME(ALERT,CONF),
      MAKE_MSG_NAME(CONNECT,REQ),
      MAKE_MSG_NAME(CONNECT,CONF),
      MAKE_MSG_NAME(CONNECT,IND),
      MAKE_MSG_NAME(CONNECT,RESP),
      MAKE_MSG_NAME(CONNECT_ACTIVE,IND),
      MAKE_MSG_NAME(CONNECT_ACTIVE,RESP),
      MAKE_MSG_NAME(CONNECT_B3_ACTIVE,IND),
      MAKE_MSG_NAME(CONNECT_B3_ACTIVE,RESP),
      MAKE_MSG_NAME(CONNECT_B3,REQ),
      MAKE_MSG_NAME(CONNECT_B3,CONF),
      MAKE_MSG_NAME(CONNECT_B3,IND),
      MAKE_MSG_NAME(CONNECT_B3,RESP),
      MAKE_MSG_NAME(CONNECT_B3_T90_ACTIVE,IND),
      MAKE_MSG_NAME(CONNECT_B3_T90_ACTIVE,RESP),
      MAKE_MSG_NAME(DATA_B3,REQ),
      MAKE_MSG_NAME(DATA_B3,CONF),
      MAKE_MSG_NAME(DATA_B3,IND),
      MAKE_MSG_NAME(DATA_B3,RESP),
      MAKE_MSG_NAME(DISCONNECT_B3,REQ),
      MAKE_MSG_NAME(DISCONNECT_B3,CONF),
      MAKE_MSG_NAME(DISCONNECT_B3,IND),
      MAKE_MSG_NAME(DISCONNECT_B3,RESP),
      MAKE_MSG_NAME(DISCONNECT,REQ),
      MAKE_MSG_NAME(DISCONNECT,CONF),
      MAKE_MSG_NAME(DISCONNECT,IND),
      MAKE_MSG_NAME(DISCONNECT,RESP),
      MAKE_MSG_NAME(FACILITY,REQ),
      MAKE_MSG_NAME(FACILITY,CONF),
      MAKE_MSG_NAME(FACILITY,IND),
      MAKE_MSG_NAME(FACILITY,RESP),
      MAKE_MSG_NAME(INFO,REQ),
      MAKE_MSG_NAME(INFO,CONF),
      MAKE_MSG_NAME(INFO,IND),
      MAKE_MSG_NAME(INFO,RESP),
      MAKE_MSG_NAME(LISTEN,REQ),
      MAKE_MSG_NAME(LISTEN,CONF),
      MAKE_MSG_NAME(MANUFACTURER,REQ),
      MAKE_MSG_NAME(MANUFACTURER,CONF),
      MAKE_MSG_NAME(MANUFACTURER,IND),
      MAKE_MSG_NAME(MANUFACTURER,RESP),
      MAKE_MSG_NAME(RESET_B3,REQ),
      MAKE_MSG_NAME(RESET_B3,CONF),
      MAKE_MSG_NAME(RESET_B3,IND),
      MAKE_MSG_NAME(RESET_B3,RESP),
      MAKE_MSG_NAME(SELECT_B_PROTOCOL,REQ),
      MAKE_MSG_NAME(SELECT_B_PROTOCOL,CONF)
    };

    static POrdinalToString MsgName(PARRAYSIZE(MsgNameInit), MsgNameInit);

    #undef MAKE_MSG_NAME

    PINDEX cmd = CAPICMD(message.header.m_Command, message.header.m_Subcommand);
    PString name = MsgName(cmd);
    if (name.IsEmpty())
      strm << '[' << hex << message.header.m_Command << ',' << message.header.m_Subcommand;
    else
      strm << name;

    switch (cmd) {
      case CAPICMD(CAPI_LISTEN,CAPI_REQ) :
      case CAPICMD(CAPI_LISTEN,CAPI_CONF) :
      case CAPICMD(CAPI_CONNECT,CAPI_REQ) :
        strm << " Controler=" << message.param.m_Controller;
        break;

      case CAPICMD(CAPI_CONNECT,CAPI_CONF) :
      case CAPICMD(CAPI_CONNECT,CAPI_IND) :
      case CAPICMD(CAPI_CONNECT,CAPI_RESP) :
      case CAPICMD(CAPI_ALERT,CAPI_REQ) :
      case CAPICMD(CAPI_CONNECT_ACTIVE,CAPI_IND) :
      case CAPICMD(CAPI_CONNECT_ACTIVE,CAPI_RESP) :
      case CAPICMD(CAPI_INFO,CAPI_IND) :
      case CAPICMD(CAPI_INFO,CAPI_RESP) :
      case CAPICMD(CAPI_CONNECT_B3,CAPI_REQ) :
      case CAPICMD(CAPI_DISCONNECT,CAPI_IND) :
      case CAPICMD(CAPI_DISCONNECT,CAPI_RESP) :
        strm << " PLCI=0x" << hex << message.param.m_PLCI;
        break;

      case CAPICMD(CAPI_CONNECT_B3,CAPI_CONF) :
      case CAPICMD(CAPI_CONNECT_B3,CAPI_IND) :
      case CAPICMD(CAPI_CONNECT_B3,CAPI_RESP) :
      case CAPICMD(CAPI_CONNECT_B3_ACTIVE,CAPI_IND) :
      case CAPICMD(CAPI_CONNECT_B3_ACTIVE,CAPI_RESP) :
      case CAPICMD(CAPI_DATA_B3,CAPI_REQ) :
      case CAPICMD(CAPI_DATA_B3,CAPI_CONF) :
      case CAPICMD(CAPI_DATA_B3,CAPI_IND) :
      case CAPICMD(CAPI_DATA_B3,CAPI_RESP) :
      case CAPICMD(CAPI_DISCONNECT_B3,CAPI_REQ) :
      case CAPICMD(CAPI_DISCONNECT_B3,CAPI_CONF) :
      case CAPICMD(CAPI_DISCONNECT_B3,CAPI_IND) :
      case CAPICMD(CAPI_DISCONNECT_B3,CAPI_RESP) :
        strm << " NCCI=0x" << hex << message.param.m_NCCI;
        break;
    }

    switch (cmd) {
      case CAPICMD(CAPI_LISTEN,CAPI_REQ) :
        strm << " InfoMask=0x" << hex << message.param.listen_req.m_InfoMask
             << " CIPMask=0x" << hex << message.param.listen_req.m_CIPMask;
        break;

      case CAPICMD(CAPI_LISTEN,CAPI_CONF) :
      case CAPICMD(CAPI_CONNECT,CAPI_CONF) :
      case CAPICMD(CAPI_CONNECT_B3,CAPI_CONF) :
        strm << " Info=0x" << hex << message.param.listen_conf.m_Info;
        break;

      case CAPICMD(CAPI_CONNECT,CAPI_REQ) :
      case CAPICMD(CAPI_CONNECT,CAPI_IND) :
        strm << " CIP=0x" << hex << message.param.connect_req.m_CIPValue;
        break;

      case CAPICMD(CAPI_CONNECT,CAPI_RESP) :
      case CAPICMD(CAPI_CONNECT_B3,CAPI_RESP) :
        strm << " Reject=0x" << hex << message.param.connect_resp.m_Reject;
        break;

      case CAPICMD(CAPI_INFO,CAPI_IND) :
        strm << " Number=0x" << hex << message.param.info_ind.m_Number;
        break;

      case CAPICMD(CAPI_DATA_B3,CAPI_REQ) :
        strm << " Handle=0x" << hex << message.param.data_b3_req.m_Handle
             << " Length=" << dec << message.param.data_b3_req.m_Length
             << " Flags=0x" << hex << message.param.data_b3_req.m_Flags;
        break;

      case CAPICMD(CAPI_DATA_B3,CAPI_CONF) :
        strm << " Handle=0x" << hex << message.param.data_b3_conf.m_Handle
             << " Info=0x" << hex << message.param.data_b3_conf.m_Info;
        break;

      case CAPICMD(CAPI_DATA_B3,CAPI_IND) :
        strm << " Handle=0x" << hex << message.param.data_b3_ind.m_Handle
             << " Length=" << dec << message.param.data_b3_ind.m_Length
             << " Flags=0x" << hex << message.param.data_b3_ind.m_Flags;
        break;

      case CAPICMD(CAPI_DATA_B3,CAPI_RESP) :
        strm << " Handle=0x" << hex << message.param.data_b3_conf.m_Handle;
        break;

      case CAPICMD(CAPI_DISCONNECT,CAPI_IND) :
        strm << " Reason=0x" << hex << message.param.disconnect_ind.m_Reason;
        break;
    }

    return strm;
  }
#endif


PString MakeToken(unsigned controller, unsigned bearer)
{
  return psprintf("CAPI:%u:%u", controller, bearer);
}


static BYTE const ReverseBits[256] = {
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};


static struct {
  WORD m_B1protocol;
  WORD m_B2protocol;
  WORD m_B3protocol;
  BYTE m_B1config;      // Zero length place holder
  BYTE m_B2config;      // Zero length place holder
  BYTE m_B3config;      // Zero length place holder
  BYTE m_GlobalConfig;  // Zero length place holder
} const DefaultBprotocol = {
  1, // Physical layer: 64 kbit/s bit-transparent operation with byte framing from the network
  1, // Data link layer: Transparent
  0, // Network layer: Transparent
  0, // Zero length place holder
  0, // Zero length place holder
  0, // Zero length place holder
  0, // Zero length place holder
};

///////////////////////////////////////////////////////////////////////

OpalCapiEndPoint::OpalCapiEndPoint(OpalManager & manager)
  : OpalEndPoint(manager, "isdn", CanTerminateCall|SupportsE164)
  , m_capi(new OpalCapiFunctions)
  , m_thread(NULL)
  , m_applicationId(0)
{
  PTRACE(4, "CAPI\tOpalCapiEndPoint created");

  OpalCapiFunctions::ApplID id = 0;
  DWORD err = m_capi->REGISTER(MaxLineCount*MaxBlockCount*MaxBlockSize+1024,
                               MaxLineCount, MaxBlockCount, MaxBlockSize, &id);
  switch (err) {
    case CapiNoError :
      break;

    case 0x10000 :
      PTRACE(1, "CAPI\tDLL not installed, or invalid.");
      return;

    case 0x1009 :
      PTRACE(1, "CAPI\tDriver not installed.");
      return;

    default :
      PTRACE(1, "CAPI\tRegistration failed, error=0x" << hex << err);
      return;
  }

  m_applicationId = id;
  m_thread = PThread::Create(PCREATE_NOTIFIER(ProcessMessages), "CAPI");

  OpenControllers();
}


OpalCapiEndPoint::~OpalCapiEndPoint()
{
  if (m_thread != NULL) {
    PTRACE(4, "LID EP\tAwaiting monitor thread termination " << GetPrefixName());
    OpalCapiFunctions::ApplID oldId = m_applicationId;
    m_applicationId = 0;
    m_capi->RELEASE(oldId);
    m_thread->WaitForTermination();
    delete m_thread;
    m_thread = NULL;
  }

  delete m_capi;

  PTRACE(4, "CAPI\tOpalCapiEndPoint destroyed");
}


PSafePtr<OpalConnection> OpalCapiEndPoint::MakeConnection(OpalCall & call,
                                                     const PString & party,
                                                              void * userData,
                                                      unsigned int   options,
                                     OpalConnection::StringOptions * stringOptions)
{
  PURL uri;
  if (!uri.Parse(party, "isdn")) {
    PTRACE(1, "CAPI\tNo available bearers for outgoing call.");
    return NULL;
  }
  
  OpalConnection::StringOptions localStringOptions;
  if (stringOptions == NULL)
    stringOptions = &localStringOptions;

  stringOptions->ExtractFromURL(uri);

  unsigned controller = uri.GetHostName().AsUnsigned();
  unsigned bearer = uri.GetPort();
  if (GetFreeLine(controller, bearer)) {
    OpalCapiConnection * connection = CreateConnection(call, userData, options, stringOptions, controller, bearer);
    if (AddConnection(connection) != NULL) {
      m_cbciToConnection[(controller<<8)|bearer] = connection;
      connection->m_calledPartyNumber = uri.GetUserName(); // Number part
      return connection;
    }
  }

  PTRACE(1, "CAPI\tNo available bearers for outgoing call.");
  return NULL;
}


OpalMediaFormatList OpalCapiEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList formats;
  formats += OpalG711_ALAW_64K;;
  //formats += OpalG711_ULAW_64K;  // Need to figure out how to detect/select this
  return formats;
}


OpalCapiConnection * OpalCapiEndPoint::CreateConnection(OpalCall & call,
                                                            void * /*userData*/,
                                                        unsigned   options,
                                   OpalConnection::StringOptions * stringOptions,
                                                        unsigned   controller,
                                                        unsigned   bearer)
{
  return new OpalCapiConnection(call, *this, options, stringOptions, controller, bearer);
}


PString OpalCapiEndPoint::GetDriverInfo() const
{
  if (m_capi == NULL)
    return PString::Empty();

  OpalCapiProfile profile;
  DWORD capiversion_min;
  DWORD capiversion_maj;
  DWORD manufacturerversion_min;
  DWORD manufacturerversion_maj;
  DWORD InstalledControllers;
  char szBuffer[64] = "";
  PStringStream str;

  m_capi->GET_MANUFACTURER(szBuffer);
  str << "CAPI_manufacturer=" << szBuffer << '\n';

  m_capi->GET_VERSION(&capiversion_maj, &capiversion_min, &manufacturerversion_maj, &manufacturerversion_min);
  str << "CAPI_version=" << capiversion_maj << '.' << capiversion_min << '(' << manufacturerversion_maj << '.' << manufacturerversion_min << ")\n";

  m_capi->GET_PROFILE(&profile, 0);
  InstalledControllers = profile.m_NumControllers;
  if (InstalledControllers > 30)
    InstalledControllers = 1;
  str << "ISDN_controllers=" << InstalledControllers << '\n';
  for (DWORD controller = 1; controller <= InstalledControllers; ++controller) {
    m_capi->GET_PROFILE(&profile, controller);
    str << "Controller#_" << controller << "_NumB=" << profile.m_NumBChannels << '\n';
  }
  return str;
}


unsigned OpalCapiEndPoint::OpenControllers()
{
  {
    PWaitAndSignal mutex(m_controllerMutex);

    if (m_thread == NULL || m_applicationId == 0) {
      PTRACE(1, "CAPI\tNot registered with drivers, or not installed.");
      return 0;
    }

    if (m_controllers.empty()) {
      OpalCapiProfile profile;
      if (m_capi->GET_PROFILE(&profile, 0) != CapiNoError) {
        PTRACE(1, "CAPI\tCould not determine number of controllers.");
        return 0;
      }

      if (profile.m_NumControllers == 0) {
        PTRACE(1, "CAPI\tNo controllers available.");
        return 0;
      }

      m_controllers.resize(profile.m_NumControllers+1);
    }
  }

  unsigned activeCount = 0;

  for (size_t controller = 1; controller < m_controllers.size(); ++controller) {
    if (!m_controllers[controller].m_active) {
      OpalCapiProfile profile;
      if (m_capi->GET_PROFILE(&profile, controller) != CapiNoError)
        continue;

      PTRACE(3, "CAPI\tListening to controller " << controller);
      m_controllers[controller].m_bearerInUse.resize(profile.m_NumBChannels+1);

      // Start listening for incoming calls
      OpalCapiMessage message(CAPI_LISTEN, CAPI_REQ, sizeof(OpalCapiMessage::Params::ListenReq));
      message.param.listen_req.m_Controller = controller;
      message.param.listen_req.m_InfoMask = 0x414; // redirecting numbers, call progress, display info
      message.param.listen_req.m_CIPMask = 0x00010012; //Audio only
      message.param.listen_req.m_CIPMask2 = 0;
      message.AddEmpty(); // Calling party number
      message.AddEmpty(); // Calling party subaddress

      if (PutMessage(message))
        m_listenCompleted.Wait(5000); // Wait for response
    }

    if (m_controllers[controller].m_active)
      ++activeCount;
  }

  return activeCount;
}


bool OpalCapiEndPoint::GetFreeLine(unsigned & controller, unsigned & bearer)
{
  PWaitAndSignal mutex(m_controllerMutex);

  if (controller != 0)
    return controller < m_controllers.size() && m_controllers[controller].GetFreeLine(bearer);

  for (controller = 1; controller < m_controllers.size(); ++controller) {
    if (m_controllers[controller].GetFreeLine(bearer))
      return true;
  }

  controller = 0;
  return false;
}


bool OpalCapiEndPoint::Controller::GetFreeLine(unsigned & bearer)
{
  if (!m_active)
    return false;

  if (bearer != 0) {
    if (bearer >= m_bearerInUse.size() || m_bearerInUse[bearer])
      return false;
    m_bearerInUse[bearer] = true;
    return true;
  }

  for (bearer = 1; bearer < m_bearerInUse.size(); ++bearer) {
    if (!m_bearerInUse[bearer]) {
      m_bearerInUse[bearer] = true;
      return true;
    }
  }

  bearer = 0;
  return false;
}


void OpalCapiEndPoint::ProcessMessages(PThread &, INT)
{
  PTRACE(4, "CAPI\tStarted message thread.");

  while (m_applicationId != 0) {
    OpalCapiMessage * pMessage = NULL;

    unsigned result = m_capi->WAIT_FOR_SIGNAL(m_applicationId);
    if (result == CapiNoError)
      result = m_capi->GET_MESSAGE(m_applicationId, (void **)&pMessage);

    switch (result) {
      case 0x1101 : // Illegal application number
        // Probably closing down as another thread set m_applicationId to zero, but just in case
        m_applicationId = 0;
        break;

      case 0x1104 : // Queue is empty
        // Really should not have happened if WAIT_FOR_SIGNAL returned!!
        break;

      case CapiNoError :
        PTRACE(pMessage->header.m_Command == CAPI_DATA_B3 ? 6 : 4, "CAPI\tGot message " << *pMessage);
        ProcessMessage(*pMessage);
        break;

      default:
        PTRACE(2, "CAPI\tGet message error 0x" << hex << result);
    }
  }

  PTRACE(4, "CAPI\tFinished message thread.");
}


bool OpalCapiEndPoint::IdToConnMap::Forward(const OpalCapiMessage & message, DWORD id)
{
  PSafePtr<OpalCapiConnection> connection;

  m_mutex.Wait();

  iterator iter = find(id);
  if (iter != end())
    connection = iter->second;

  m_mutex.Signal();

  if (connection == NULL)
    return false;

  connection->ProcessMessage(message);
  return true;
}


void OpalCapiEndPoint::ProcessMessage(const OpalCapiMessage & message)
{
  if (message.header.m_Subcommand == CAPI_CONF) {
    if (message.header.m_Command != CAPI_LISTEN)
      m_cbciToConnection.Forward(message, message.header.m_Number);
    else {
      m_controllers[message.param.listen_conf.m_Controller].m_active = (message.param.listen_conf.m_Info == CapiNoError);
      m_listenCompleted.Signal();
    }
    return;
  }

  // Various indications.
  switch (message.header.m_Command) {
    case CAPI_CONNECT :
      ProcessConnectInd(message);
      break;

    case CAPI_INFO :
    case CAPI_CONNECT_ACTIVE :
      m_plciToConnection.Forward(message, message.param.m_PLCI);
      break;

    case CAPI_DISCONNECT :
      if (!m_plciToConnection.Forward(message, message.param.m_PLCI) && message.header.m_Subcommand == CAPI_IND) {
        // Is a DISCONNECT_IND and it was not forwarded to connection (already gone), then have to handle it here
        OpalCapiMessage resp(CAPI_DISCONNECT, CAPI_RESP, sizeof(OpalCapiMessage::Params::DisconnectResp));
        resp.param.disconnect_resp.m_PLCI = message.param.disconnect_ind.m_PLCI;
        PutMessage(resp);
      }
      break;

    case CAPI_CONNECT_B3 :
    case CAPI_CONNECT_B3_ACTIVE :
    case CAPI_DATA_B3 :
    case CAPI_DISCONNECT_B3 :
      m_plciToConnection.Forward(message, message.param.m_NCCI&0xffff);
      break;
  }
}


void OpalCapiEndPoint::ProcessConnectInd(const OpalCapiMessage & message)
{
  unsigned controller = (message.param.connect_ind.m_PLCI&0xff);
  unsigned bearer = 0;
  if (GetFreeLine(controller, bearer)) {
    // Get new instance of a call, abort if none created
    OpalCall * call = manager.InternalCreateCall();
    if (call != NULL) {
      OpalCapiConnection * connection = CreateConnection(*call, NULL, 0, NULL, controller, bearer);
      if (AddConnection(connection) != NULL) {
        m_cbciToConnection[(controller<<8)|bearer] = connection;
        connection->ProcessMessage(message);
        return;
      }
    }
  }

  // Reject, circuit/channel not available
  OpalCapiMessage resp(CAPI_CONNECT, CAPI_RESP, sizeof(OpalCapiMessage::Params::ConnectResp));
  resp.param.connect_resp.m_PLCI = message.param.connect_ind.m_PLCI;
  resp.param.connect_resp.m_Reject = 4;
  resp.AddEmpty(); // B protocol
  resp.Add(GetDefaultLocalPartyName()); // Connected party number
  resp.AddEmpty(); // Connected party subaddress
  resp.AddEmpty(); // Low Layer Compatibility
  resp.AddEmpty(); // Additional Info
  PutMessage(resp);
}


bool OpalCapiEndPoint::PutMessage(OpalCapiMessage & message)
{
  PTRACE(message.header.m_Command == CAPI_DATA_B3 ? 6 : 4, "CAPI\tSending message " << message);
  message.header.m_ApplId = (WORD)m_applicationId;
  DWORD err = m_capi->PUT_MESSAGE(m_applicationId, &message);
  PTRACE_IF(2, err != CapiNoError, "CAPI\tPUT_MESSAGE error 0x" << hex << err);
  return err == CapiNoError;
}


///////////////////////////////////////////////////////////////////////////////

OpalCapiConnection::OpalCapiConnection(OpalCall & call,
                               OpalCapiEndPoint & endpoint,
                                       unsigned   options,
                  OpalConnection::StringOptions * stringOptions,
                                       unsigned   controller,
                                       unsigned   bearer)
  : OpalConnection(call, endpoint, MakeToken(controller, bearer), options, stringOptions)
  , m_endpoint(endpoint)
  , m_controller(controller)
  , m_bearer(bearer)
  , m_PLCI(0)
  , m_NCCI(0)
  , m_Bprotocol((const BYTE *)&DefaultBprotocol, sizeof(DefaultBprotocol))
{
}


bool OpalCapiConnection::IsNetworkConnection() const
{
  return true;
}


void OpalCapiConnection::OnApplyStringOptions()
{
  OpalConnection::OnApplyStringOptions();

  PStringStream bProto(m_stringOptions(OPAL_OPT_CAPI_B_PROTO));
  if (!bProto.IsEmpty())
    bProto >> m_Bprotocol;
}


PBoolean OpalCapiConnection::SetUpConnection()
{
  InternalSetAsOriginating();

  SetPhase(SetUpPhase);

  OnApplyStringOptions();

  OpalCapiMessage message(CAPI_CONNECT, CAPI_REQ, sizeof(OpalCapiMessage::Params::ConnectReq));
  message.param.connect_req.m_Controller = m_controller;
  message.param.connect_req.m_CIPValue = 16; // Telephony
  message.Add(0x80, m_calledPartyNumber); // Called party number
  PString number(m_stringOptions(OPAL_OPT_CALLING_PARTY_NUMBER, m_stringOptions(OPAL_OPT_CALLING_PARTY_NAME, localPartyName)));
  message.Add(0x00, 0x80, number.Left(number.FindSpan("0123456789#*"))); // Calling party number
  message.AddEmpty(); // Called party subaddress
  message.AddEmpty(); // Calling party subaddress

  message.Add(m_Bprotocol, m_Bprotocol.GetSize());

  message.AddEmpty(); // Bearer Capabilities
  message.AddEmpty(); // Low Layer Compatibility
  message.AddEmpty(); // High Layer Compatibility
  message.AddEmpty(); // Additional Info
  return PutMessage(message);
}


PBoolean OpalCapiConnection::SetAlerting(const PString & /*calleeName*/, PBoolean /*withMedia*/)
{
  SetPhase(AlertingPhase);

  OpalCapiMessage message(CAPI_ALERT, CAPI_REQ, sizeof(OpalCapiMessage::Params::AlertReq));
  message.param.alert_req.m_PLCI = m_PLCI;

  message.Add(m_Bprotocol, m_Bprotocol.GetSize());

  message.AddEmpty(); // Keypad facility
  message.AddEmpty(); // User-user data
  message.AddEmpty(); // Facility data
  message.AddEmpty(); // Sending complete
  return PutMessage(message);
}


PBoolean OpalCapiConnection::SetConnected()
{
  SetPhase(ConnectedPhase);

  OpalCapiMessage message(CAPI_CONNECT, CAPI_RESP, sizeof(OpalCapiMessage::Params::ConnectResp));
  message.param.connect_resp.m_PLCI = m_PLCI;
  message.param.connect_resp.m_Reject = 0;
  message.Add(m_Bprotocol, m_Bprotocol.GetSize());
  message.Add(localPartyName); // Connected party number
  message.AddEmpty(); // Connected party subaddress
  message.AddEmpty(); // Low Layer Compatibility
  message.AddEmpty(); // Additional Info
  return PutMessage(message);
}


void OpalCapiConnection::OnReleased()
{
  if (m_NCCI != 0) {
    OpalCapiMessage message(CAPI_DISCONNECT_B3, CAPI_REQ, sizeof(OpalCapiMessage::Params::DisconnectB3Req));
    message.param.disconnect_b3_req.m_NCCI = m_NCCI;
    message.AddEmpty(); // Network Control Protocol Information
    PutMessage(message);
  }

  if (m_PLCI != 0) {
    m_endpoint.m_plciToConnection.erase(m_PLCI);

    OpalCapiMessage message(CAPI_DISCONNECT, CAPI_REQ, sizeof(OpalCapiMessage::Params::DisconnectReq));
    message.param.disconnect_req.m_PLCI = m_PLCI;
    message.AddEmpty(); // Additional info
    PutMessage(message);
  }

  m_endpoint.m_cbciToConnection.erase((m_controller<<8)|m_bearer);
  m_endpoint.m_controllers[m_controller].m_bearerInUse[m_bearer] = false;

  OpalConnection::OnReleased();
}


PString OpalCapiConnection::GetDestinationAddress()
{
  return m_calledPartyNumber;
}


OpalMediaFormatList OpalCapiConnection::GetMediaFormats() const
{
  return m_endpoint.GetMediaFormats();
}


OpalMediaStream * OpalCapiConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                        unsigned sessionID,
                                                        PBoolean isSource)
{
  return new OpalCapiMediaStream(*this, mediaFormat, sessionID, isSource);
}


PBoolean OpalCapiConnection::SendUserInputTone(char tone, int duration)
{
  return OpalConnection::SendUserInputTone(tone, duration);
}


void OpalCapiConnection::ProcessMessage(const OpalCapiMessage & message)
{
  switch (CAPICMD(message.header.m_Command, message.header.m_Subcommand)) {
    case CAPICMD(CAPI_CONNECT,CAPI_CONF) :
      // Outgoing call
      switch (message.param.connect_conf.m_Info) {
        case 0 :
          m_PLCI = message.param.connect_ind.m_PLCI;
          m_endpoint.m_plciToConnection[m_PLCI] = this;
          break;

        case 0x2003 :
          Release(EndedByLocalCongestion);
          break;

        default :
          Release(EndedByOutOfService);
      }
      break;

    case CAPICMD(CAPI_CONNECT,CAPI_IND) :
    {
      // Incoming call
      m_PLCI = message.param.connect_ind.m_PLCI;
      m_endpoint.m_plciToConnection[m_PLCI] = this;
      PINDEX pos = sizeof(OpalCapiMessage::Header) + sizeof(message.param.connect_ind);
      message.Get(pos, NULL, 1, m_calledPartyNumber); // Called party address
      message.Get(pos, NULL, 2, remotePartyNumber); // Calling party address
      remotePartyAddress = remotePartyNumber;

      OnApplyStringOptions();
      if (OnIncomingConnection(0, NULL))
        ownerCall.OnSetUp(*this);
      break;
    }

    case CAPICMD(CAPI_INFO,CAPI_IND) :
    {
      OpalCapiMessage resp(CAPI_INFO, CAPI_RESP, sizeof(OpalCapiMessage::Params::InfoResp));
      resp.param.info_resp.m_PLCI = m_PLCI;
      PutMessage(resp);
      switch (message.param.info_ind.m_Number) {
        case 0x8001 :
          OnAlerting();
          break;

        case 0x8002 :
          OnProceeding();
          break;
      }
      break;
    }

    case CAPICMD(CAPI_CONNECT_ACTIVE,CAPI_IND) :
    {
      OpalCapiMessage resp(CAPI_CONNECT_ACTIVE, CAPI_RESP, sizeof(OpalCapiMessage::Params::ConnectActiveResp));
      resp.param.connect_active_resp.m_PLCI = m_PLCI;
      PutMessage(resp);

      if (IsOriginating()) {
        OpalCapiMessage req(CAPI_CONNECT_B3, CAPI_REQ, sizeof(OpalCapiMessage::Params::ConnectB3Req));
        req.param.connect_b3_req.m_PLCI = m_PLCI;
        req.AddEmpty(); // Network Control Protocol Information
        PutMessage(req);
      }
      break;
    }

    case CAPICMD(CAPI_CONNECT_B3,CAPI_CONF) :
      // Outgoing call
      switch (message.param.connect_b3_conf.m_Info) {
        case 0 :
          m_NCCI = message.param.connect_b3_conf.m_NCCI;
          AutoStartMediaStreams();
          OnConnectedInternal();
          break;

        case 0x2004 :
          Release(EndedByLocalCongestion);
          break;

        default :
          Release(EndedByOutOfService);
      }
      break;

    case CAPICMD(CAPI_CONNECT_B3,CAPI_IND) :
    {
      m_NCCI = message.param.connect_b3_ind.m_NCCI;

      OpalCapiMessage resp(CAPI_CONNECT_B3, CAPI_RESP, sizeof(OpalCapiMessage::Params::ConnectB3Resp));
      resp.param.connect_b3_resp.m_NCCI = m_NCCI;
      resp.param.connect_b3_resp.m_Reject = 0; // Accept call
      resp.AddEmpty(); // Network Control Protocol Information
      PutMessage(resp);

      AutoStartMediaStreams();
      break;
    }

    case CAPICMD(CAPI_CONNECT_B3_ACTIVE,CAPI_IND) :
    {
      OpalCapiMessage resp(CAPI_CONNECT_B3_ACTIVE, CAPI_RESP, sizeof(OpalCapiMessage::Params::ConnectB3ActiveResp));
      resp.param.connect_b3_active_resp.m_NCCI = m_NCCI;
      PutMessage(resp);
      break;
    }

    case CAPICMD(CAPI_DATA_B3,CAPI_IND) :
    {
      PSafePtr<OpalCapiMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalCapiMediaStream>(GetMediaStream(OpalMediaType::Audio(), true));
      if (stream != NULL)
        stream->m_queue.Write(message.param.data_b3_ind.GetPtr(), message.param.data_b3_ind.m_Length);

      OpalCapiMessage resp(CAPI_DATA_B3, CAPI_RESP, sizeof(OpalCapiMessage::Params::DataB3Resp));
      resp.param.data_b3_resp.m_NCCI = m_NCCI;
      resp.param.data_b3_resp.m_Handle = message.param.data_b3_ind.m_Handle;
      PutMessage(resp);
      break;
    }

    case CAPICMD(CAPI_DATA_B3,CAPI_CONF) :
    {
      PSafePtr<OpalCapiMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalCapiMediaStream>(GetMediaStream(OpalMediaType::Audio(), false));
      if (stream != NULL)
        stream->m_written.Signal();
      break;
    }

    case CAPICMD(CAPI_DISCONNECT_B3,CAPI_IND) :
    {
      OpalCapiMessage resp(CAPI_DISCONNECT_B3, CAPI_RESP, sizeof(OpalCapiMessage::Params::DisconnectB3Resp));
      resp.param.disconnect_b3_resp.m_NCCI = m_NCCI;
      PutMessage(resp);

      m_NCCI = 0;
      break;
    }

    case CAPICMD(CAPI_DISCONNECT,CAPI_IND) :
    {
      OpalCapiMessage resp(CAPI_DISCONNECT, CAPI_RESP, sizeof(OpalCapiMessage::Params::DisconnectResp));
      resp.param.disconnect_resp.m_PLCI = message.param.disconnect_ind.m_PLCI;
      PutMessage(resp);

      if (m_PLCI != 0) {
        m_endpoint.m_plciToConnection.erase(m_PLCI);
        m_PLCI = 0;
      }

      if ((message.param.disconnect_ind.m_Reason&0xff00) != 0x3400) {
        PTRACE(3, "CAPI\tDisconnected by stack, reason=0x" << hex << message.param.disconnect_ind.m_Reason);
        switch (message.param.disconnect_ind.m_Reason) {
          case 0x3301 :
            Release(EndedByTransportFail);
          case 0x3302 :
          case 0x3303 :
            Release(EndedByConnectFail);
          default :
            Release(EndedByOutOfService);
        }
      }
      else {
        unsigned cause = message.param.disconnect_ind.m_Reason & 0x7f;
        PTRACE(3, "CAPI\tDisconnected by network, Q.850=" << cause << " (0x" << hex << cause << ')');
        switch (cause) {
          case 16 :
            Release(EndedByRemoteUser);
            break;
          case 17 :
            Release(EndedByRemoteBusy);
            break;
          case 18 :
          case 19 :
            Release(EndedByNoAnswer);
            break;
          case 42 :
          case 34 :
          case 44 :
          case 47 :
            Release(EndedByRemoteCongestion);
            break;
          case 21 :
            Release(EndedByRefusal);
            break;
          case 2 :
          case 6 :
            Release(EndedByUnreachable);
            break;
          case 1 :
          case 3 :
          case 20 :
            Release(EndedByNoUser);
            break;
          case 23 :
            Release(EndedByCallForwarded);
            break;
          case 27 :
            Release(EndedByConnectFail);
            break;
          case 41 :
            Release(EndedByTemporaryFailure);
            break;
          default :
            Release(CallEndReason(EndedByQ931Cause, cause));
        }
      }
      break;
    }

    case CAPICMD(CAPI_DISCONNECT,CAPI_CONF) :
    case CAPICMD(CAPI_DISCONNECT_B3,CAPI_CONF) :
      m_disconnected.Signal();
      break;
  }
}


bool OpalCapiConnection::PutMessage(OpalCapiMessage & message)
{
  message.header.m_Number = (WORD)((m_controller<<8)|m_bearer);
  if (m_endpoint.PutMessage(message))
    return true;

  Release(EndedByOutOfService);
  return false;
}


///////////////////////////////////////////////////////////////////////////////

OpalCapiMediaStream::OpalCapiMediaStream(OpalCapiConnection & conn,
                                      const OpalMediaFormat & mediaFormat,
                                                   unsigned   sessionID,
                                                   PBoolean   isSource)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
  , m_connection(conn)
  , m_queue(8000) // One seconds worth of audio
{
}


void OpalCapiMediaStream::InternalClose()
{
  m_queue.Close();
  m_written.Signal();
}


PBoolean OpalCapiMediaStream::ReadData(BYTE * data, PINDEX size, PINDEX & length)
{
  if (IsSink() || !m_queue.Read(data, size))
    return false;

  length = m_queue.GetLastReadCount();

  for (PINDEX i = 0; i < length; ++i)
    data[i] = ReverseBits[data[i]];

  return true;
}


PBoolean OpalCapiMediaStream::WriteData(const BYTE * data, PINDEX length, PINDEX & written)
{
  if (IsSource() || !IsOpen() || !PAssert(length < 65535, PInvalidParameter))
    return false;

  written = length;

  // Have not got the logical channel yet
  if (m_connection.m_NCCI == 0) {
    m_delay.Delay(length/8); // Always 8kHz G.711 for ISDN
    return true;
  }

  for (PINDEX i = 0; i < length; ++i)
    ((BYTE *)data)[i] = ReverseBits[data[i]];

  OpalCapiMessage message(CAPI_DATA_B3, CAPI_REQ, sizeof(OpalCapiMessage::Params::DataB3Req));
  message.param.data_b3_req.m_NCCI = m_connection.m_NCCI;
  message.param.data_b3_req.SetPtr(data);
  message.param.data_b3_req.m_Length = (WORD)length;

  if (!m_connection.PutMessage(message))
    return false;

  m_written.Wait();
  return IsOpen();
}


PBoolean OpalCapiMediaStream::IsSynchronous() const
{
  return true;
}


#else

  #ifdef _MSC_VER
    #pragma message("CAPI ISDN support DISABLED")
  #endif

#endif // OPAL_CAPI


// End of File ///////////////////////////////////////////////////////////////
