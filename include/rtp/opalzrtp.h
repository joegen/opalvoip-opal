#ifndef OPAL_ZRTP_OPALZRTP_H
#define OPAL_ZRTP_OPALZRTP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifdef OPAL_ZRTP

#include <zrtp.h>
#include <opal_config.h>
#include <zrtp/zrtpeventproc.h>

namespace PWLibStupidLinkerHacks {
  extern int libZRTPLoader;
};

class OpalZrtp {
  public:
	static bool Init(char *name, char *zidFile);
	static bool Init(OpalZrtp *opalZrtp);
	static bool DeInit();

	static zrtp_global_ctx *GetZrtpContext();
	static unsigned char *GetZID();
	static void SetEventProcessor(ZrtpEventProcessor *eventProcessor);
	static ZrtpEventProcessor * GetEventProcessor();

	virtual ~OpalZrtp();

  protected:
	virtual unsigned char *DoGetZID();
	virtual zrtp_global_ctx *DoGetZrtpContext();
	virtual bool DoInit(char *name, char *zidFile);
	
  private:
	static OpalZrtp *instance;
	static int		isDefault;
	static ZrtpEventProcessor *eventProcessor;
};


class OpalZRTPStreamInfo {
  public:
    virtual bool Open() = 0;
    virtual OpalMediaSession * CreateSession(
      OpalConnection & connection,    ///< Connection that owns the sesion
      unsigned sessionId,             ///< Unique (in connection) session ID for session
      const OpalMediaType & mediaType ///< Media type for session
    ) = 0;
};

class OpalZRTPConnectionInfo {
  public:
    virtual bool Open() = 0;
    virtual OpalMediaSession * CreateSession(
      OpalConnection & connection,    ///< Connection that owns the sesion
      unsigned sessionId,             ///< Unique (in connection) session ID for session
      const OpalMediaType & mediaType ///< Media type for session
    ) = 0;

    PMutex mutex;
};

#endif // OPAL_ZRTP

#endif // OPAL_ZRTP_OPALZRTP_H
