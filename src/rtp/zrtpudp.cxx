#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "rtp/zrtpudp.h"
#endif

#include <zrtp.h>
#include <opal/buildopts.h>
#include "rtp/zrtpudp.h"

#define SRTP_MAX_TAG_LEN 12 
#define SRTP_MAX_TRAILER_LEN SRTP_MAX_TAG_LEN 

#define DECLARE_LIBZRTP_CRYPTO_ALG(name, sas, pk, auth, cipher, hash) \
class OpalZrtpSecurityMode_##name : public LibZrtpSecurityMode_Base \
{ \
  public: \
	OpalZrtpSecurityMode_##name() {\
		Init(sas, pk, auth, cipher, hash); \
	} \
	~OpalZrtpSecurityMode_##name() {\
		if (profile) delete profile; \
	} \
}; \
static PFactory<OpalSecurityMode>::Worker<OpalZrtpSecurityMode_##name> factoryZrtpSecurityMode_##name("ZRTP|" #name);

DECLARE_LIBZRTP_CRYPTO_ALG( DEFAULT, NULL, NULL, NULL, NULL, NULL);
DECLARE_LIBZRTP_CRYPTO_ALG( STRONGHOLD, 
						((int[]){ZRTP_SAS_BASE256, ZRTP_SAS_BASE32, 0}),
				       		((int[]){ZRTP_PKTYPE_PRESH, ZRTP_PKTYPE_DH4096, ZRTP_PKTYPE_DH3072, 0}),
				      	 	((int[]){ZRTP_ATL_HS80, ZRTP_ATL_HS32, 0}),
				       		((int[]){ZRTP_CIPHER_AES256, ZRTP_CIPHER_AES128, 0}),
				       		((int[]){ZRTP_HASH_SHA256, ZRTP_HASH_SHA128, 0}));

LibZrtpSecurityMode_Base::LibZrtpSecurityMode_Base() {
	Init(NULL, NULL, NULL, NULL, NULL);
}
 
zrtp_profile_t* LibZrtpSecurityMode_Base::GetZrtpProfile(){
	return profile;
}

void LibZrtpSecurityMode_Base::Init(int *sas, int *pk, int *auth, int *cipher, int *hash) {
	int i;
	if (!sas || !pk || !auth || !cipher || !hash || !sas[0] || !pk[0] || !auth[0] || !cipher[0] || !hash[0] ) {
		profile = NULL; 
		// use default components values (if we use NULL pointer libzrtp will take default sets of components)
		printf("init zrtp security mode by default\n");
		return;
	}
	
	profile = new zrtp_profile_t;
	memset(profile, 0, sizeof(zrtp_profile_t));
    
	profile->active		= 1;
	profile->autosecure	= 1;
	profile->allowclear	= 0;
	profile->cache_ttl	= ZRTP_CACHE_DEFAULT_TTL;

	for (i = 0; sas[i]; i++)
		profile->sas_schemes[i] = sas[i];

	for (i = 0; pk[i]; i++)
		profile->pk_schemes[i] = pk[i];

	for (i = 0; auth[i]; i++)
		profile->auth_tag_lens[i] = auth[i];

	for (i = 0; cipher[i]; i++)
		profile->cipher_types[i] = cipher[i];

	for (i = 0; hash[i]; i++)
		profile->hash_schemes[i] = hash[i];
	
	printf("init zrtp security mode\n");
}
 
RTP_UDP * LibZrtpSecurityMode_Base::CreateRTPSession(
#if OPAL_RTP_AGGREGATE
                                                      	PHandleAggregator * _aggregator, ///< handle aggregator
#endif
							unsigned id,
							PBoolean remoteIsNAT,            ///< TRUE is remote is behind NAT
                                                        OpalConnection & conn
					            )
{
	OpalZrtp_UDP * session = new OpalZrtp_UDP(
#if OPAL_RTP_AGGREGATE
                                                 _aggregator,
#endif
                                                 id, remoteIsNAT);
	session->SetSecurityMode(this);
	return session;
}
 
PBoolean LibZrtpSecurityMode_Base::Open() {
	return PTrue;
}


//////////////////////////////////////////////////////////////////////

OpalZrtp_UDP::OpalZrtp_UDP(
#if OPAL_RTP_AGGREGATE
                           PHandleAggregator * aggregator,
#endif
                           unsigned id, PBoolean remoteIsNAT)
:SecureRTP_UDP(
#ifdef OPAL_RTP_AGGREGATE
    aggregator,
#endif
    id, remoteIsNAT)
,zrtpStream(NULL)
{
}
 
OpalZrtp_UDP::~OpalZrtp_UDP() {
}

// this method is used for zrtp protocol packets sending in zrtp_send_rtp function, 
//	as we dont want to process them by lib once more time in OnSendData
// (by the way they will have incorrect CRC after setting new packets length in OnSendData)
PBoolean OpalZrtp_UDP::WriteZrtpData(RTP_DataFrame & frame) {
	if (shutdownWrite) {
		shutdownWrite = FALSE;
		return PFalse;
	}

	// Trying to send a PDU before we are set up!
	if (!remoteAddress.IsValid() || remoteDataPort == 0) {
		return PFalse; //libzrtp has to wait
	}

	while (!dataSocket->WriteTo(frame.GetPointer(), 
								frame.GetHeaderSize()+frame.GetPayloadSize(),
								remoteAddress, remoteDataPort))
    {
		switch (dataSocket->GetErrorNumber()) {
			case ECONNRESET :
			case ECONNREFUSED :
    			break;
			default:
				return PFalse;
		}
	}
	return PTrue;
}
 


RTP_UDP::SendReceiveStatus OpalZrtp_UDP::OnSendData(RTP_DataFrame & frame) {
	SendReceiveStatus stat = RTP_UDP::OnSendData(frame);
	if (stat != e_ProcessPacket) {
		return stat;
	}
 
	unsigned len = frame.GetHeaderSize() + frame.GetPayloadSize();
 
 	zrtp_status_t err = ::zrtp_process_rtp(zrtpStream, (char *)frame.GetPointer(), &len);
   
	if (err != zrtp_status_ok) {
		return RTP_Session::e_IgnorePacket;
	}
 
	frame.SetPayloadSize(len - frame.GetHeaderSize());
	return e_ProcessPacket;
}
 
RTP_UDP::SendReceiveStatus OpalZrtp_UDP::OnReceiveData(RTP_DataFrame & frame) {
	unsigned len = frame.GetHeaderSize() + frame.GetPayloadSize();
	zrtp_status_t err = ::zrtp_process_srtp(zrtpStream, (char *)frame.GetPointer(), &len);
 
	if (err != zrtp_status_ok) {
		return RTP_Session::e_IgnorePacket;
	}
	
	frame.SetPayloadSize(len - frame.GetHeaderSize());
 
	return RTP_UDP::OnReceiveData(frame);
}
 
RTP_UDP::SendReceiveStatus OpalZrtp_UDP::OnSendControl(RTP_ControlFrame & frame, PINDEX & transmittedLen) {
	SendReceiveStatus stat = RTP_UDP::OnSendControl(frame, transmittedLen);
	if (stat != e_ProcessPacket) {
		return stat;
	}

	frame.SetMinSize(transmittedLen + SRTP_MAX_TRAILER_LEN);
	 unsigned len = transmittedLen;

	zrtp_status_t err = ::zrtp_process_srtcp(zrtpStream, (char *)frame.GetPointer(), &len);
	if (err != zrtp_status_ok) {
		return RTP_Session::e_IgnorePacket;
	}
	
    transmittedLen = len;
 
	return e_ProcessPacket;
}
 
RTP_UDP::SendReceiveStatus OpalZrtp_UDP::OnReceiveControl(RTP_ControlFrame & frame) {
	unsigned len = frame.GetSize();
	zrtp_status_t err = ::zrtp_process_rtcp(zrtpStream, (char *)frame.GetPointer(), &len);
	if (err != zrtp_status_ok) {
		return RTP_Session::e_IgnorePacket;
	}
	
	frame.SetSize(len);
	return RTP_UDP::OnReceiveControl(frame);
}
 
DWORD OpalZrtp_UDP::GetOutgoingSSRC() {
	return syncSourceOut;
}





