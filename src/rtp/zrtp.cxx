/*
 * zrtp.cxx
 *
 * ZRTP protocol handler
 *
 * OPAL Library
 *
 * Copyright (C) 2007 Post Increment
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
 * The Original Code is OPAL Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "zrtp.h"
#endif

#include <ptlib//pprocess.h>
#include <opal/buildopts.h>

#if defined(OPAL_ZRTP)

#include <rtp/zrtp.h>

#ifdef _WIN32
#pragma warning(disable:4244)
#pragma warning(disable:4505)
#endif

extern "C" {
#include <ZRTP/zrtp.h>
#include <ZRTP/zrtp_log.h>
};

namespace PWLibStupidLinkerHacks {
  int libZRTPLoader;
};

#if _WIN32
#pragma comment(lib, LIBZRTP_LIBRARY)
#endif

class LibZRTPSecurityMode_Base : public OpalZRTPSecurityMode
{
  PCLASSINFO(LibZRTPSecurityMode_Base, OpalZRTPSecurityMode);
  public:
    LibZRTPSecurityMode_Base();

    RTP_UDP * CreateRTPSession(PHandleAggregator * _aggregator,   ///< handle aggregator
                                            unsigned id,          ///<  Session ID for RTP channel
                                            PBoolean remoteIsNAT      ///<  PTrue is remote is behind NAT
    );

    PBoolean Open();

    zrtp_zid_t zid;
    zrtp_conn_ctx_t zrtpConn;
    zrtp_stream_ctx_t zrtpStream;

  protected:
    void Init();

  private:
    static PMutex initMutex;
    static zrtp_global_ctx * zrtpContext;
};

PMutex LibZRTPSecurityMode_Base::initMutex;
zrtp_global_ctx_t * LibZRTPSecurityMode_Base::zrtpContext = NULL;

#define DECLARE_LIBZRTP_CRYPTO_ALG(name) \
class OpalZRTPSecurityMode_##name : public LibZRTPSecurityMode_Base \
{ \
  public: \
    OpalZRTPSecurityMode_##name() \
    { \
      Init(); \
    } \
}; \
static PFactory<OpalSecurityMode>::Worker<OpalZRTPSecurityMode_##name> factoryZRTPSecurityMode_##name("ZRTP|" #name); \

DECLARE_LIBZRTP_CRYPTO_ALG(AES_128_DH_4096_AUTH_80);
//DECLARE_LIBZRTP_CRYPTO_ALG(AES_CM_128_HMAC_SHA1_32);
//DECLARE_LIBZRTP_CRYPTO_ALG(AES_CM_128_NULL_AUTH);
//DECLARE_LIBZRTP_CRYPTO_ALG(NULL_CIPHER_HMAC_SHA1_80);

DECLARE_LIBZRTP_CRYPTO_ALG(STRONGHOLD);


///////////////////////////////////////////////////////

extern "C" {

void zrtp_print_log(log_level_t level, const char* format, ...)
{
  va_list arg;
  va_start(arg, format);
  PTRACE(level+1, "libZRTP\t" + psprintf(format, arg));
  va_end( arg );
}

int zrtp_send_rtp(const zrtp_stream_ctx_t* stream_ctx, char* packet, unsigned int length)
{
  OpalZRTP_UDP * rtpSession = (OpalZRTP_UDP *)stream_ctx->stream_usr_data;
  if (rtpSession == NULL)
    return zrtp_status_write_fail;

  RTP_DataFrame frame((BYTE *)packet, length);
  return rtpSession->WriteData(frame);
}

zrtp_status_t zrtp_packet_callback(zrtp_packet_event_t /*evnt*/, zrtp_stream_ctx_t * /*ctx*/, zrtp_rtp_info_t * /*packet*/)
{
  return zrtp_status_ok;
}

void zrtp_event_callback(zrtp_event_t evnt, zrtp_stream_ctx_t * /*ctx*/)
{
  //zrtp_test_stream_t* stream = (zrtp_test_stream_t*) ctx->stream_usr_data;	
    
	zrtp_print_log(LOG_DEBUG, ">>>>>>>>>>>>>>>>>>>>>>>\n");
  switch (evnt){
	  case ZRTP_EVENT_GOTHELLO:
	    zrtp_print_log(LOG_DEBUG, "libzrtp_test: ZRTP_EVENT_GOTHELLO.\n");
	    break;
    case ZRTP_EVENT_GOTHELLOACK:
	    zrtp_print_log(LOG_DEBUG, "libzrtp_test: stream ZRTP_EVENT_GOTHELLOACK.\n");
	    break;
	  case ZRTP_EVENT_IS_CLEAR:
	    zrtp_print_log(LOG_DEBUG, "libzrtp_test: stream ZRTP_EVENT_IS_CLEAR.\n");
	    break;
	  case ZRTP_EVENT_IS_INITIATINGSECURE:
	    zrtp_print_log(LOG_DEBUG, "libzrtp_test: stream ZRTP_EVENT_IS_INITIATINGSECURE.\n");
	    break;
	  case ZRTP_EVENT_IS_PENDINGSECURE:
	    zrtp_print_log(LOG_DEBUG, "libzrtp_test: stream ZRTP_EVENT_IS_PENDINGSECURE.\n");
	    break;
	  case ZRTP_EVENT_IS_INITIATINGCLEAR:
	    zrtp_print_log(LOG_DEBUG, "libzrtp_test: stream ZRTP_EVENT_IS_INITIATINGCLEAR.\n");		
	    break;
	  case ZRTP_EVENT_IS_PENDINGCLEAR:
	    zrtp_print_log(LOG_DEBUG, "libzrtp_test: stream ZRTP_EVENT_IS_PENDINGCLEAR.\n");
	    break;
	  case ZRTP_EVENT_IS_SECURE:
	    zrtp_print_log(LOG_DEBUG, "libzrtp_test: stream ZRTP_EVENT_IS_SECURE.\n");
	    //zrtp_print_log(LOG_DEBUG, "libzrtp_test: stream atributes:\n");
	    //zrtp_print_log(LOG_DEBUG, "libzrtp_test: CIPHER    =%.4s\n", ctx->_session_ctx->_blockcipher->type);
	    //zrtp_print_log(LOG_DEBUG, "libzrtp_test: ATL       =%.4s\n", ctx->_session_ctx->_authtaglength->type);
	    //zrtp_print_log(LOG_DEBUG, "libzrtp_test: SAS       =%.4s\n", ctx->_session_ctx->_sasscheme->type);
	    //zrtp_print_log(LOG_DEBUG, "libzrtp_test: HASH      =%.4s\n", ctx->_session_ctx->_hash->type);
	    //zrtp_print_log(LOG_DEBUG, "libzrtp_test: PKType    =%.4s\n", ctx->_session_ctx->_pubkeyscheme->type);
	    //zrtp_print_log(LOG_DEBUG, "libzrtp_test: \n");
	    //zrtp_print_log(LOG_DEBUG, "libzrtp_test: Staysecure=%s\n", ctx->_session_ctx->_staysecure ? "ON" : "OFF");
	    //zrtp_print_log(LOG_DEBUG, "libzrtp_test: Autosecure=%s\n", ctx->_session_ctx->_profile._autosecure ? "ON" : "OFF");
	    //zrtp_print_log(LOG_DEBUG, "libzrtp_test: \n");
	    //zrtp_print_log(LOG_DEBUG, "libzrtp_test: SAS value: <<<%.4s>>>\n", ctx->_session_ctx->sas_values.str1);		
  		break;
	  case ZRTP_EVENT_ERROR:
	    zrtp_print_log(LOG_DEBUG, "libzrtp_test: stream ZRTP_EVENT_ERROR.\n");		
		  //stream->conn->state = TEST_ERROR_STATE;
	    break;
	  default:
      break;
  }	
	zrtp_print_log(LOG_DEBUG, "<<<<<<<<<<<<<<<<<<<<<<<\n");	
}

void zrtp_play_alert(zrtp_stream_ctx_t * /*stream_ctx*/)
{
	//stream_ctx->_need_play_alert = zrtp_play_no;
}

};
///////////////////////////////////////////////////////

LibZRTPSecurityMode_Base::LibZRTPSecurityMode_Base()
{
}

void LibZRTPSecurityMode_Base::Init()
{
  {
    PWaitAndSignal m(initMutex);
    if (zrtpContext == NULL) {
      zrtpContext = new zrtp_global_ctx_t;
      zrtp_init(zrtpContext, (const char *)PProcess::Current().GetName());
    }
  }
}

RTP_UDP * LibZRTPSecurityMode_Base::CreateRTPSession(
  PHandleAggregator * _aggregator,   ///< handle aggregator
  unsigned id,                       ///<  Session ID for RTP channel
  PBoolean remoteIsNAT                   ///<  PTrue is remote is behind NAT
)
{
  OpalZRTP_UDP * session = new OpalZRTP_UDP(_aggregator, id, remoteIsNAT);
  session->SetSecurityMode(this);
  return session;
}

PBoolean LibZRTPSecurityMode_Base::Open()
{
  return PTrue;
}

///////////////////////////////////////////////////////

OpalZRTP_UDP::OpalZRTP_UDP(PHandleAggregator * _aggregator,   ///<  RTP aggregator
                                      unsigned id,            ///<  Session ID for RTP channel
                                          PBoolean remoteIsNAT)   ///<  PTrue is remote is behind NAT
  : SecureRTP_UDP(_aggregator, id, remoteIsNAT)
{
}

OpalZRTP_UDP::~OpalZRTP_UDP()
{
}

RTP_UDP::SendReceiveStatus OpalZRTP_UDP::OnSendData(RTP_DataFrame & frame)
{
  return e_IgnorePacket;
  SendReceiveStatus stat = RTP_UDP::OnSendData(frame);
  if (stat != e_ProcessPacket)
    return stat;

  LibZRTPSecurityMode_Base * zrtp = (LibZRTPSecurityMode_Base *)securityParms;

  unsigned len = frame.GetHeaderSize() + frame.GetPayloadSize();
  frame.SetPayloadSize(len + SRTP_MAX_TRAILER_LEN);

  zrtp_status_t err = ::zrtp_process_rtp(&zrtp->zrtpStream, (char *)frame.GetPointer(), &len);
  
  if (err != zrtp_status_ok)
    return RTP_Session::e_IgnorePacket;

  frame.SetPayloadSize(len - frame.GetHeaderSize());
  return e_ProcessPacket;
}

RTP_UDP::SendReceiveStatus OpalZRTP_UDP::OnReceiveData(RTP_DataFrame & frame)
{
  LibZRTPSecurityMode_Base * zrtp = (LibZRTPSecurityMode_Base *)securityParms;

  unsigned len = frame.GetHeaderSize() + frame.GetPayloadSize();

  zrtp_status_t err = ::zrtp_process_srtp(&zrtp->zrtpStream, (char *)frame.GetPointer(), &len);

  if (err != zrtp_status_ok)
    return RTP_Session::e_IgnorePacket;

  frame.SetPayloadSize(len - frame.GetHeaderSize());

  return RTP_UDP::OnReceiveData(frame);
}

RTP_UDP::SendReceiveStatus OpalZRTP_UDP::OnSendControl(RTP_ControlFrame & frame, PINDEX & transmittedLen)
{
  SendReceiveStatus stat = RTP_UDP::OnSendControl(frame, transmittedLen);
  if (stat != e_ProcessPacket)
    return stat;

  frame.SetMinSize(transmittedLen + SRTP_MAX_TRAILER_LEN);
  unsigned len = transmittedLen;

  LibZRTPSecurityMode_Base * zrtp = (LibZRTPSecurityMode_Base *)securityParms;

  zrtp_status_t err = ::zrtp_process_srtcp(&zrtp->zrtpStream, (char *)frame.GetPointer(), &len);
  if (err != zrtp_status_ok)
    return RTP_Session::e_IgnorePacket;
  transmittedLen = len;

  return e_ProcessPacket;
}

RTP_UDP::SendReceiveStatus OpalZRTP_UDP::OnReceiveControl(RTP_ControlFrame & frame)
{
  LibZRTPSecurityMode_Base * zrtp = (LibZRTPSecurityMode_Base *)securityParms;

  unsigned len = frame.GetSize();
  zrtp_status_t err = ::zrtp_process_rtcp(&zrtp->zrtpStream, (char *)frame.GetPointer(), &len);
  if (err != zrtp_status_ok)
    return RTP_Session::e_IgnorePacket;
  frame.SetSize(len);

  return RTP_UDP::OnReceiveControl(frame);
}

#endif
