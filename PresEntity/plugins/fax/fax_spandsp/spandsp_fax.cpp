/*  
 * Fax plugin codec for OPAL using SpanDSP
 *
 * Copyright (C) 2008 Post Increment, All Rights Reserved
 *
 * Contributor(s): Craig Southeren (craigs@postincrement.com)
 *                 Robert Jongbloed (robertj@voxlucida.com.au)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Revision$
 * $Author$
 * $Date$
 */


#define LOGGING 1

extern "C" {
#include "spandsp.h"
};

#include <codec/opalplugin.h>


#include <stdlib.h>

#if defined(_WIN32) || defined(_WIN32_WCE)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <io.h>
  #define strcasecmp _stricmp
  #define access _access
  #define W_OK 2
  #define R_OK 4
  #define DIR_SEPERATORS "/\\:"
#else
  #include <unistd.h>
  #include <pthread.h>
  #define DIR_SEPERATORS "/"
#endif

#include <sstream>
#include <vector>
#include <queue>
#include <map>


#define   BITS_PER_SECOND           14400
#define   MICROSECONDS_PER_FRAME    20000
#define   SAMPLES_PER_FRAME         160
#define   BYTES_PER_FRAME           100
#define   PREF_FRAMES_PER_PACKET    1
#define   MAX_FRAMES_PER_PACKET     1


#if LOGGING

static PluginCodec_LogFunction LogFunction;

#define PTRACE(level, args) \
    if (LogFunction != NULL && LogFunction(level, NULL, 0, NULL, NULL)) { \
    std::ostringstream strm; strm << args; \
      LogFunction(level, __FILE__, __LINE__, "SpanDSP", strm.str().c_str()); \
    } else (void)0

static void SpanDSP_Message(int level, const char *text)
{
  if (*text != '\0' && LogFunction != NULL && LogFunction(level, NULL, 0, NULL, NULL)) {
    int last = strlen(text)-1;
    if (text[last] == '\n')
      ((char *)text)[last] = '\0';
    LogFunction(level, __FILE__, __LINE__, "SpanDSP", text);
  }
}

static void InitLogging(logging_state_t * logging)
{
  span_log_set_message_handler(logging, SpanDSP_Message);
  span_log_set_level(logging, SPAN_LOG_SHOW_SEVERITY | SPAN_LOG_SHOW_PROTOCOL | SPAN_LOG_DEBUG);
}

#else // LOGGING

#define PTRACE(level, expr)
#define InitLogging(logging)

#endif // LOGGING

/////////////////////////////////////////////////////////////////////////////

static const char L16Format[] = "L16";
static const char T38Format[] = "T.38";
static const char TIFFFormat[] = "TIFF-File";
static const char T38sdp[]    = "t38";


static struct PluginCodec_Option const ReceivingOption =
{
  PluginCodec_BoolOption,     // PluginCodec_OptionTypes
  "Receiving",                // Generic (human readable) option name
  1,                          // Read Only flag
  PluginCodec_AlwaysMerge,    // Merge mode
  "0",                        // Initial value
  NULL,                       // SIP/SDP FMTP name
  NULL,                       // SIP/SDP FMTP default value (option not included in FMTP if have this value)
  0                           // H.245 Generic Capability number and scope bits
};

static struct PluginCodec_Option const TiffFileNameOption =
{
  PluginCodec_StringOption,   // PluginCodec_OptionTypes
  "TIFF-File-Name",           // Generic (human readable) option name
  1,                          // Read Only flag
  PluginCodec_AlwaysMerge,    // Merge mode
  "",                         // Initial value
  NULL,                       // SIP/SDP FMTP name
  NULL,                       // SIP/SDP FMTP default value (option not included in FMTP if have this value)
  0                           // H.245 Generic Capability number and scope bits
};

static struct PluginCodec_Option const T38FaxVersion =
{
  PluginCodec_IntegerOption,  // PluginCodec_OptionTypes
  "T38FaxVersion",            // Generic (human readable) option name
  0,                          // Read Only flag
  PluginCodec_MinMerge,       // Merge mode
  "0",                        // Initial value
  NULL,                       // SIP/SDP FMTP name
  NULL,                       // SIP/SDP FMTP default value (option not included in FMTP if have this value)
  0,                          // H.245 Generic Capability number and scope bits
  "0",                        // Minimum value
  "1"                         // Maximum value
};

static struct PluginCodec_Option const T38FaxRateManagement =
{
  PluginCodec_EnumOption,     // PluginCodec_OptionTypes
  "T38FaxRateManagement",     // Generic (human readable) option name
  0,                          // Read Only flag
  PluginCodec_MinMerge,       // Merge mode
  "transferredTCF",           // Initial value
  NULL,                       // SIP/SDP FMTP name
  NULL,                       // SIP/SDP FMTP default value (option not included in FMTP if have this value)
  0,                          // H.245 Generic Capability number and scope bits
  "localTCF:transferredTCF"   // enum values
};

static struct PluginCodec_Option const T38MaxBitRate =
{
  PluginCodec_IntegerOption,  // PluginCodec_OptionTypes
  "T38MaxBitRate",            // Generic (human readable) option name
  0,                          // Read Only flag
  PluginCodec_MinMerge,       // Merge mode
  "14400",                    // Initial value
  NULL,                       // SIP/SDP FMTP name
  NULL,                       // SIP/SDP FMTP default value (option not included in FMTP if have this value)
  0,                          // H.245 Generic Capability number and scope bits
  "300",                      // Minimum value
  "56000"                     // Maximum value
};

static struct PluginCodec_Option const T38FaxMaxBuffer =
{
  PluginCodec_IntegerOption,  // PluginCodec_OptionTypes
  "T38FaxMaxBuffer",          // Generic (human readable) option name
  0,                          // Read Only flag
  PluginCodec_MinMerge,       // Merge mode
  "2000",                     // Initial value
  NULL,                       // SIP/SDP FMTP name
  NULL,                       // SIP/SDP FMTP default value (option not included in FMTP if have this value)
  0,                          // H.245 Generic Capability number and scope bits
  "100",                       // Minimum value
  "9999"                      // Maximum value
};

static struct PluginCodec_Option const T38FaxMaxDatagram =
{
  PluginCodec_IntegerOption,  // PluginCodec_OptionTypes
  "T38FaxMaxDatagram",        // Generic (human readable) option name
  0,                          // Read Only flag
  PluginCodec_MinMerge,       // Merge mode
  "528",                      // Initial value
  NULL,                       // SIP/SDP FMTP name
  NULL,                       // SIP/SDP FMTP default value (option not included in FMTP if have this value)
  0,                          // H.245 Generic Capability number and scope bits
  "10",                       // Minimum value
  "1500"                      // Maximum value
};

static struct PluginCodec_Option const T38FaxUdpEC =
{
  PluginCodec_EnumOption,     // PluginCodec_OptionTypes
  "T38FaxUdpEC",              // Generic (human readable) option name
  0,                          // Read Only flag
  PluginCodec_MinMerge,       // Merge mode
  "t38UDPRedundancy",         // Initial value
  NULL,                       // SIP/SDP FMTP name
  NULL,                       // SIP/SDP FMTP default value (option not included in FMTP if have this value)
  0,                          // H.245 Generic Capability number and scope bits
  "t38UDPRedundancy:t38UDPFEC"// enum values
};

static struct PluginCodec_Option const T38FaxFillBitRemoval =
{
  PluginCodec_BoolOption,     // PluginCodec_OptionTypes
  "T38FaxFillBitRemoval",     // Generic (human readable) option name
  0,                          // Read Only flag
  PluginCodec_MinMerge,       // Merge mode
  "0",                        // Initial value
  NULL,                       // SIP/SDP FMTP name
  NULL,                       // SIP/SDP FMTP default value (option not included in FMTP if have this value)
  0                           // H.245 Generic Capability number and scope bits
};

static struct PluginCodec_Option const T38FaxTranscodingMMR =
{
  PluginCodec_BoolOption,     // PluginCodec_OptionTypes
  "T38FaxTranscodingMMR",     // Generic (human readable) option name
  0,                          // Read Only flag
  PluginCodec_MinMerge,       // Merge mode
  "0",                        // Initial value
  NULL,                       // SIP/SDP FMTP name
  NULL,                       // SIP/SDP FMTP default value (option not included in FMTP if have this value)
  0                           // H.245 Generic Capability number and scope bits
};

static struct PluginCodec_Option const T38FaxTranscodingJBIG =
{
  PluginCodec_BoolOption,     // PluginCodec_OptionTypes
  "T38FaxTranscodingJBIG",    // Generic (human readable) option name
  0,                          // Read Only flag
  PluginCodec_MinMerge,       // Merge mode
  "0",                        // Initial value
  NULL,                       // SIP/SDP FMTP name
  NULL,                       // SIP/SDP FMTP default value (option not included in FMTP if have this value)
  0                           // H.245 Generic Capability number and scope bits
};

static struct PluginCodec_Option const * const OptionTableTIFF[] = {
  &ReceivingOption,
  &TiffFileNameOption,
  NULL
};

static struct PluginCodec_Option const * const OptionTableT38[] = {
  &ReceivingOption,
  &TiffFileNameOption,
  &T38FaxVersion,
  &T38FaxRateManagement,
  &T38MaxBitRate,
  &T38FaxMaxBuffer,
  &T38FaxMaxDatagram,
  &T38FaxUdpEC,
  &T38FaxFillBitRemoval,
  &T38FaxTranscodingMMR,
  &T38FaxTranscodingJBIG,
  NULL
};


/////////////////////////////////////////////////////////////////
//
// define a class to implement a critical section mutex
// based on PCriticalSection from PWLib

#ifdef _WIN32
class CriticalSection
{
private:
  CRITICAL_SECTION m_CriticalSection;
public:
  inline CriticalSection()  { InitializeCriticalSection(&m_CriticalSection); }
  inline ~CriticalSection() { DeleteCriticalSection(&m_CriticalSection); }
  inline void Wait()        { EnterCriticalSection(&m_CriticalSection); }
  inline void Signal()      { LeaveCriticalSection(&m_CriticalSection); }
};
#else
class CriticalSection
{
private:
  pthread_mutex_t m_Mutex;
public:
  inline CriticalSection()  { pthread_mutex_init(&m_Mutex, NULL); }
  inline ~CriticalSection() { pthread_mutex_destroy(&m_Mutex); }
  inline void Wait()        { pthread_mutex_lock(&m_Mutex); }
  inline void Signal()      { pthread_mutex_unlock(&m_Mutex); }
};
#endif
    
class WaitAndSignal
{
  private:
    CriticalSection & m_criticalSection;

    void operator=(const WaitAndSignal &) { }
    WaitAndSignal(const WaitAndSignal & other) : m_criticalSection(other.m_criticalSection) { }
  public:
    inline WaitAndSignal(const CriticalSection & cs) 
      : m_criticalSection((CriticalSection &)cs) { m_criticalSection.Wait(); }
    inline ~WaitAndSignal()                      { m_criticalSection.Signal(); }
};


/////////////////////////////////////////////////////////////////

typedef std::vector<unsigned char> InstanceKey;

class FaxSpanDSP
{
  protected:
    CriticalSection m_mutex;
    unsigned        m_referenceCount;

    bool            m_receiving;
    bool            m_usingT38;
    std::string     m_tiffFileName;
    std::string     m_stationIdentifer;
    unsigned        m_protoVersion;
    bool            m_useECM;

    t38_terminal_state_t * m_t38State;
    fax_state_t          * m_faxState;
    t30_state_t          * m_t30state;
    int                    m_t30result;

    struct T38Packet : public std::vector<unsigned char>
    {
      int m_sequence;
    };
    std::queue<T38Packet> m_t38Queue;

  public:
    FaxSpanDSP(bool usingT38)
      : m_referenceCount(1)
      , m_receiving(false)
      , m_usingT38(usingT38)
      , m_stationIdentifer("-")
      , m_protoVersion(0)
      , m_useECM(false)
      , m_t38State(NULL)
      , m_faxState(NULL)
      , m_t30state(NULL)
      , m_t30result(-1)
    {
    }


    ~FaxSpanDSP()
    {
      if (m_t38State != NULL) {
        t38_terminal_release(m_t38State);
        t38_terminal_free(m_t38State);
      }
      if (m_faxState != NULL) {
        fax_release(m_faxState);
        fax_free(m_faxState);
      }
      PTRACE(4, "Deleted SpanDSP instance.");
    }


    void AddReference()
    {
      m_mutex.Wait();
      ++m_referenceCount;
      m_mutex.Signal();
    }


    bool Dereference()
    {
      WaitAndSignal mutex(m_mutex);
      return --m_referenceCount == 0;
    }


    bool SetOptions(const char * const * options)
    {
      if (options == NULL)
        return false;

      while (options[0] != NULL && options[1] != NULL) {
        if (!SetOption(options[0], options[1]))
          return false;
        options += 2;
      }

      return true;
    }


    bool SetOption(const char * option, const char * value)
    {
      if (strcasecmp(option, TiffFileNameOption.m_name) == 0) {
        m_tiffFileName = value;
        return true;
      }

      if (strcasecmp(option, ReceivingOption.m_name) == 0) {
        m_receiving = atoi(value) != 0;
        return true;
      }

      if (strcasecmp(option, T38FaxVersion.m_name) == 0) {
        m_protoVersion = atoi(value);
        return true;
      }

      return true;
    }


    void PhaseE(t30_state_t * t30state, int result)
    {
      m_t30result = result;
#if LOGGING
      if (LogFunction != NULL && LogFunction(3, NULL, 0, NULL, NULL)) { \
        t30_stats_t stats;
        t30_get_transfer_statistics(t30state, &stats);

        std::ostringstream strm;
        strm << "SpanDSP entered Phase E: " << t30_completion_code_to_str(result) << "\n"
                "Encoding: " << stats.encoding << "\n"
                "Bit rate: " << stats.bit_rate << "\n"
                "Bytes = " << stats.image_size << "\n"
                "Pages: tx=" << stats.pages_tx << " rx=" << stats.pages_rx << " total=" << stats.pages_in_file << "\n"
                "Resolution: x=" << stats.x_resolution << " y=" << stats.y_resolution << "\n"
                "Pixel Width: " << stats.width << "\n"
                "Pixel Rows: " << stats.length << " (bad=" << stats.bad_rows << ")\n"
                "Error Correction Mode: " << stats.error_correcting_mode << " (retries=" << stats.error_correcting_mode_retries << ")\n"
                "Status = " << stats.current_status;
        LogFunction(3, __FILE__, __LINE__, "SpanDSP", strm.str().c_str());
    }
#endif
    }
    static void PhaseE(t30_state_t * t30state, void * user_data, int result)
    {
      if (user_data != NULL)
        ((FaxSpanDSP *)user_data)->PhaseE(t30state, result);
    }


    void T38Packets(const uint8_t * buf, int len, int sequence)
    {
      m_mutex.Wait();

      m_t38Queue.push(T38Packet());
      T38Packet & packet = m_t38Queue.back();
      packet.resize(len);
      memcpy(&packet[0], buf, len);
      packet.m_sequence = sequence;

      m_mutex.Signal();
    }
    static int T38Packets(t38_core_state_t *, void * user_data, const uint8_t * buf, int len, int sequence)
    {
      if (user_data != NULL)
        ((FaxSpanDSP *)user_data)->T38Packets(buf, len, sequence);
      return 0;
    }


    bool Open()
    {
      WaitAndSignal mutex(m_mutex);

      if (m_t30result >= 0)
        return false; // Finished, exit codec loops

      if (m_t30state != NULL)
        return true;

      if (m_usingT38) {
        PTRACE(3, "Opening SpanDSP for T.38");
        if ((m_t38State = t38_terminal_init(NULL, !m_receiving, T38Packets, this)) == NULL)
          return false;

        InitLogging(t38_terminal_get_logging_state(m_t38State));
        t38_terminal_set_config(m_t38State, false);

        t38_core_state_t * t38core = t38_terminal_get_t38_core_state(m_t38State);
        InitLogging(t38_core_get_logging_state(t38core));
        t38_set_t38_version(t38core, m_protoVersion);

        m_t30state = t38_terminal_get_t30_state(m_t38State);
      }
      else {
        PTRACE(3, "Opening SpanDSP for PCM");
        if ((m_faxState = fax_init(NULL, !m_receiving)) == NULL)
          return false;

        InitLogging(fax_get_logging_state(m_faxState));

        m_t30state = fax_get_t30_state(m_faxState);
      }

      if (!m_tiffFileName.empty()) {
        if (m_receiving) {
          std::string dir;
          std::string::size_type pos = m_tiffFileName.find_last_of(DIR_SEPERATORS);
          if (pos == std::string::npos)
            dir = ".";
          else
            dir.assign(m_tiffFileName, 0, pos+1);

          if (access(dir.c_str(), W_OK) != 0) {
            PTRACE(1, "Cannot set receive TIFF file to \"" << m_tiffFileName << '"');
            return false;
          }

          t30_set_rx_file(m_t30state, m_tiffFileName.c_str(), -1);
          PTRACE(3, "Set receive TIFF file to \"" << m_tiffFileName << '"');
        }
        else {
          if (access(m_tiffFileName.c_str(), R_OK) != 0) {
            PTRACE(1, "Cannot set transmit TIFF file to \"" << m_tiffFileName << '"');
            return false;
          }

          t30_set_tx_file(m_t30state, m_tiffFileName.c_str(), -1, -1);
          PTRACE(3, "Set transmit TIFF file to \"" << m_tiffFileName << '"');
        }
      }

      t30_set_phase_e_handler(m_t30state, PhaseE, this);
      t30_set_tx_ident(m_t30state, m_stationIdentifer.c_str());
      t30_set_ecm_capability(m_t30state, m_useECM);

      return true;
    }


    bool WritePCM(const void * fromPtr, unsigned & fromLen)
    {
      int samplesLeft = fax_rx(m_faxState, (int16_t *)fromPtr, fromLen/2);
      if (samplesLeft < 0)
        return false;

      fromLen -= samplesLeft*2;
      return true;
    }


    bool ReadPCM(void * toPtr, unsigned & toLen, unsigned & flags)
    {
      int samplesGenerated = fax_tx(m_faxState, (int16_t *)PluginCodec_RTP_GetPayloadPtr(toPtr), toLen/2);
      if (samplesGenerated < 0)
        return false;

      toLen = samplesGenerated*2;
      flags = PluginCodec_ReturnCoderLastFrame;
      return true;
    }


    bool WriteT38(const void * fromPtr, unsigned & fromLen)
    {
      int payloadSize = fromLen - PluginCodec_RTP_GetHeaderLength(fromPtr);
      if (payloadSize < 0)
        return false;

      if (payloadSize == 0)
        return true;

      return t38_core_rx_ifp_packet(t38_terminal_get_t38_core_state(m_t38State),
                                    PluginCodec_RTP_GetPayloadPtr(fromPtr),
                                    payloadSize,
                                    PluginCodec_RTP_GetSequenceNumber(fromPtr)) != -1;
    }


    bool ReadT38(void * toPtr, unsigned  & toLen, unsigned & flags)
    {
      WaitAndSignal mutex(m_mutex);

      if (m_t38Queue.empty()) {
        if (t38_terminal_send_timeout(m_t38State, SAMPLES_PER_FRAME) || m_t38Queue.empty()) {
          toLen = 0;
          flags = PluginCodec_ReturnCoderLastFrame;
          return true;
        }
      }

      T38Packet & packet = m_t38Queue.front();
      size_t size = packet.size() + PluginCodec_RTP_MinHeaderSize;
      if (toLen < size)
        return false;

      toLen = size;
      memcpy(PluginCodec_RTP_GetPayloadPtr(toPtr), &packet[0], packet.size());

      m_t38Queue.pop();
      if (m_t38Queue.empty())
        flags = PluginCodec_ReturnCoderLastFrame;

      return true;
    }


    virtual bool Encode(const void * fromPtr, unsigned & fromLen, void * toPtr, unsigned & toLen, unsigned & flags) = 0;
    virtual bool Decode(const void * fromPtr, unsigned & fromLen, void * toPtr, unsigned & toLen, unsigned & flags) = 0;
};



typedef std::map<InstanceKey, FaxSpanDSP *> InstanceMapType;
static InstanceMapType InstanceMap;
static CriticalSection InstanceMapMutex;


/////////////////////////////////////////////////////////////////

class T38_PCM : public FaxSpanDSP
{
  public:
    T38_PCM()
      : FaxSpanDSP(true)
    {
      PTRACE(4, "Created T38_PCM");
    }

    virtual bool Encode(const void * fromPtr, unsigned & fromLen, void * toPtr, unsigned & toLen, unsigned & flags)
    {
      return Open() && WritePCM(fromPtr, fromLen) && ReadT38(toPtr, toLen, flags);
    }

    virtual bool Decode(const void * fromPtr, unsigned & fromLen, void * toPtr, unsigned & toLen, unsigned & flags)
    {
      return Open() && WriteT38(fromPtr, fromLen) && ReadPCM(toPtr, toLen, flags);
    }
};


/////////////////////////////////////////////////////////////////

class TIFF_T38 : public FaxSpanDSP
{
  public:
    TIFF_T38()
      : FaxSpanDSP(true)
    {
      PTRACE(4, "Created TIFF_T38");
    }

    virtual bool Encode(const void * /*fromPtr*/, unsigned & /*fromLen*/, void * toPtr, unsigned & toLen, unsigned & flags)
    {
      return Open() && ReadT38(toPtr, toLen, flags);
    }

    virtual bool Decode(const void * fromPtr, unsigned & fromLen, void * /*toPtr*/, unsigned & toLen, unsigned & flags)
    {
      toLen = 0;
      flags = PluginCodec_ReturnCoderLastFrame;
      return Open() && WriteT38(fromPtr, fromLen);
    }
};


/////////////////////////////////////////////////////////////////

class TIFF_PCM : public FaxSpanDSP
{
  public:
    TIFF_PCM()
      : FaxSpanDSP(false)
    {
      PTRACE(4, "Created TIFF_PCM");
    }

    virtual bool Encode(const void * /*fromPtr*/, unsigned & /*fromLen*/, void * toPtr, unsigned & toLen, unsigned & flags)
    {
      return Open() && ReadPCM(toPtr, toLen, flags);
    }

    virtual bool Decode(const void * fromPtr, unsigned & fromLen, void * /*toPtr*/, unsigned & toLen, unsigned & flags)
    {
      toLen = 0;
      flags = PluginCodec_ReturnCoderLastFrame;
      return Open() && WritePCM(fromPtr, fromLen);
    }
};


/////////////////////////////////////////////////////////////////

class FaxCodecContext
{
  private:
    const PluginCodec_Definition * m_definition;
    InstanceKey                    m_key;
    FaxSpanDSP                   * m_instance;

  public:
    FaxCodecContext(const PluginCodec_Definition * defn)
      : m_definition(defn)
      , m_instance(NULL)
    {
    }


    ~FaxCodecContext()
    {
      if (m_instance == NULL)
        return;

      WaitAndSignal mutex(InstanceMapMutex);

      InstanceMapType::iterator iter = InstanceMap.find(m_key);
      if (iter != InstanceMap.end() && iter->second->Dereference()) {
        delete iter->second;
        InstanceMap.erase(iter);
      }
    }


    bool SetContextId(void * parm, unsigned * parmLen)
    {
      if (parm == NULL || parmLen == NULL || *parmLen == 0)
        return false;

      m_key.resize(*parmLen);
      memcpy(&m_key[0], parm, *parmLen);

      WaitAndSignal mutex(InstanceMapMutex);

      InstanceMapType::iterator iter = InstanceMap.find(m_key);
      if (iter != InstanceMap.end()) {
        m_instance = iter->second;
        m_instance->AddReference();
      }
      else {
        if (m_definition->sourceFormat == TIFFFormat) {
          if (m_definition->destFormat == T38Format)
            m_instance = new TIFF_T38();
          else
            m_instance = new TIFF_PCM();
        }
        else if (m_definition->sourceFormat == T38Format) {
          if (m_definition->destFormat == TIFFFormat)
            m_instance = new TIFF_T38();
          else
            m_instance = new T38_PCM();
        }
        else {
          if (m_definition->destFormat == TIFFFormat)
            m_instance = new TIFF_PCM();
          else
            m_instance = new T38_PCM();
        }
        InstanceMap[m_key] = m_instance;
      }

      return true;
    }


    bool SetOptions(const char * const * options)
    {
      return m_instance != NULL && m_instance->SetOptions(options);
    }


    bool Encode(const void * fromPtr, unsigned & fromLen, void * toPtr, unsigned & toLen, unsigned & flags)
    {
      return m_instance != NULL && m_instance->Encode(fromPtr, fromLen, toPtr, toLen, flags);
    }


    bool Decode(const void * fromPtr, unsigned & fromLen, void * toPtr, unsigned & toLen, unsigned & flags)
    {
      return m_instance != NULL && m_instance->Decode(fromPtr, fromLen, toPtr, toLen, flags);
    }
};


/////////////////////////////////////////////////////////////////////////////

static int get_codec_options(const PluginCodec_Definition * ,
                                                     void * context,
                                               const char * ,
                                                     void * parm,
                                                 unsigned * parmLen)
{
  if (parm == NULL || parmLen == NULL || *parmLen != sizeof(struct PluginCodec_Option **))
    return false;

  *(struct PluginCodec_Option const * const * *)parm =
          context != NULL && strcasecmp((char *)context, T38Format) == 0 ? OptionTableT38 : OptionTableTIFF;
  return true;
}


static int set_codec_options(const PluginCodec_Definition * ,
                                                     void * context,
                                               const char * , 
                                                     void * parm, 
                                                 unsigned * parmLen)
{
  return context != NULL &&
         parm != NULL &&
         parmLen != NULL &&
         *parmLen == sizeof(const char **) &&
         ((FaxCodecContext *)context)->SetOptions((const char * const *)parm);
}


static int set_instance_id(const PluginCodec_Definition * ,
                                                   void * context,
                                             const char * ,
                                                   void * parm,
                                               unsigned * parmLen)
{
  return context != NULL && ((FaxCodecContext *)context)->SetContextId(parm, parmLen);
}


#if LOGGING
static int set_log_function(const PluginCodec_Definition * ,
                                                   void * ,
                                             const char * ,
                                                   void * parm,
                                               unsigned * parmLen)
{
  if (parmLen == NULL || *parmLen != sizeof(PluginCodec_LogFunction))
    return false;

  LogFunction = (PluginCodec_LogFunction)parm;
  return true;
}
#endif


static struct PluginCodec_ControlDefn Controls[] = {
  { PLUGINCODEC_CONTROL_GET_CODEC_OPTIONS, get_codec_options },
  { PLUGINCODEC_CONTROL_SET_CODEC_OPTIONS, set_codec_options },
  { PLUGINCODEC_CONTROL_SET_INSTANCE_ID,   set_instance_id },
#if LOGGING
  { PLUGINCODEC_CONTROL_SET_LOG_FUNCTION,  set_log_function },
#endif
  { NULL }
};


/////////////////////////////////////////////////////////////////////////////

static void * Create(const PluginCodec_Definition * codec)
{
  return new FaxCodecContext(codec);
}


static void Destroy(const PluginCodec_Definition * /*codec*/, void * context)
{
  delete (FaxCodecContext *)context;
}


static int Encode(const PluginCodec_Definition * /*codec*/,
                                          void * context,
                                    const void * fromPtr,
                                      unsigned * fromLen,
                                          void * toPtr,
                                      unsigned * toLen,
                                      unsigned * flags)
{
  return context != NULL && ((FaxCodecContext *)context)->Encode(fromPtr, *fromLen, toPtr, *toLen, *flags);
}



static int Decode(const PluginCodec_Definition * /*codec*/,
                                          void * context,
                                    const void * fromPtr,
                                      unsigned * fromLen,
                                          void * toPtr,
                                      unsigned * toLen,
                                      unsigned * flags)
{
  return context != NULL && ((FaxCodecContext *)context)->Decode(fromPtr, *fromLen, toPtr, *toLen, *flags);
}


/////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_information LicenseInfo = {
  1081086550,                              // timestamp = Sun 04 Apr 2004 01:49:10 PM UTC = 

  "Craig Southeren, Post Increment",                           // source code author
  "1.0",                                                       // source code version
  "craigs@postincrement.com",                                  // source code email
  "http://www.postincrement.com",                              // source code URL
  "Copyright (C) 2007 by Post Increment, All Rights Reserved", // source code copyright
  "MPL 1.0",                                                   // source code license
  PluginCodec_License_MPL,                                     // source code license
  
  "T.38 Fax Codec",                                            // codec description
  "Craig Southeren",                                           // codec author
  "Version 1",                                                 // codec version
  "craigs@postincrement.com",                                  // codec email
  "",                                                          // codec URL
  "",                                                          // codec copyright information
  NULL,                                                        // codec license
  PluginCodec_License_MPL                                      // codec license code
};

#define MY_API_VERSION PLUGIN_CODEC_VERSION_OPTIONS

static PluginCodec_Definition faxCodecDefn[] = {

  { 
    // encoder
    MY_API_VERSION,                     // codec API version
    &LicenseInfo,                       // license information

    PluginCodec_MediaTypeFax |          // audio codec
    PluginCodec_InputTypeRaw |          // raw input data
    PluginCodec_OutputTypeRTP |         // RTP output data
    PluginCodec_RTPTypeDynamic,         // dynamic RTP type

    "PCM to T.38 Codec",                // text decription
    L16Format,                          // source format
    T38Format,                          // destination format

    NULL,                               // user data

    8000,                               // samples per second
    BITS_PER_SECOND,                    // raw bits per second
    MICROSECONDS_PER_FRAME,             // microseconds per frame
    SAMPLES_PER_FRAME,                  // samples per frame
    BYTES_PER_FRAME,                    // bytes per frame
    PREF_FRAMES_PER_PACKET,             // recommended number of frames per packet
    MAX_FRAMES_PER_PACKET,              // maximum number of frames per packe
    0,                                  // dynamic payload
    T38sdp,                             // RTP payload name

    Create,                             // create codec function
    Destroy,                            // destroy codec
    Encode,                             // encode/decode
    Controls,                           // codec controls

    PluginCodec_H323T38Codec,           // h323CapabilityType 
    NULL                                // h323CapabilityData
  },

  { 
    // decoder
    MY_API_VERSION,                     // codec API version
    &LicenseInfo,                       // license information

    PluginCodec_MediaTypeFax |          // audio codec
    PluginCodec_InputTypeRTP |          // raw input RTP
    PluginCodec_OutputTypeRaw |         // raw output data
    PluginCodec_RTPTypeDynamic,         // dynamic RTP type

    "T.38 to PCM Codec",                // text decription
    T38Format,                          // source format
    L16Format,                          // destination format

    NULL,                               // user data

    8000,                               // samples per second
    BITS_PER_SECOND,                    // raw bits per second
    MICROSECONDS_PER_FRAME,             // microseconds per frame
    SAMPLES_PER_FRAME,                  // samples per frame
    BYTES_PER_FRAME,                    // bytes per frame
    PREF_FRAMES_PER_PACKET,             // recommended number of frames per packet
    MAX_FRAMES_PER_PACKET,              // maximum number of frames per packe
    0,                                  // dynamic payload
    T38sdp,                             // RTP payload name

    Create,                             // create codec function
    Destroy,                            // destroy codec
    Decode,                             // encode/decode
    Controls,                           // codec controls

    PluginCodec_H323T38Codec,           // h323CapabilityType 
    NULL                                // h323CapabilityData
  },

  { 
    // encoder
    MY_API_VERSION,                     // codec API version
    &LicenseInfo,                       // license information

    PluginCodec_MediaTypeFax |          // audio codec
    PluginCodec_InputTypeRaw |          // raw input data
    PluginCodec_OutputTypeRTP |         // RTP output data
    PluginCodec_RTPTypeDynamic,         // dynamic RTP type

    "TIFF to T.38 Codec",               // text decription
    TIFFFormat,                         // source format
    T38Format,                          // destination format

    NULL,                               // user data

    8000,                               // samples per second
    BITS_PER_SECOND,                    // raw bits per second
    MICROSECONDS_PER_FRAME,             // microseconds per frame
    SAMPLES_PER_FRAME,                  // samples per frame
    BYTES_PER_FRAME,                    // bytes per frame
    PREF_FRAMES_PER_PACKET,             // recommended number of frames per packet
    MAX_FRAMES_PER_PACKET,              // maximum number of frames per packe
    0,                                  // dynamic payload
    NULL,                               // RTP payload name

    Create,                             // create codec function
    Destroy,                            // destroy codec
    Encode,                             // encode/decode
    Controls,                           // codec controls

    PluginCodec_H323T38Codec,           // h323CapabilityType 
    NULL                                // h323CapabilityData
  },

  { 
    // decoder
    MY_API_VERSION,                     // codec API version
    &LicenseInfo,                       // license information

    PluginCodec_MediaTypeFax |          // audio codec
    PluginCodec_InputTypeRTP |          // raw input RTP
    PluginCodec_OutputTypeRaw |         // raw output data
    PluginCodec_RTPTypeDynamic,         // dynamic RTP type

    "T.38 to TIFF Codec",               // text decription
    T38Format,                          // source format
    TIFFFormat,                         // destination format

    NULL,                               // user data

    8000,                               // samples per second
    BITS_PER_SECOND,                    // raw bits per second
    MICROSECONDS_PER_FRAME,             // microseconds per frame
    SAMPLES_PER_FRAME,                  // samples per frame
    BYTES_PER_FRAME,                    // bytes per frame
    PREF_FRAMES_PER_PACKET,             // recommended number of frames per packet
    MAX_FRAMES_PER_PACKET,              // maximum number of frames per packe
    0,                                  // dynamic payload
    NULL,                               // RTP payload name

    Create,                             // create codec function
    Destroy,                            // destroy codec
    Decode,                             // encode/decode
    Controls,                           // codec controls

    PluginCodec_H323T38Codec,           // h323CapabilityType 
    NULL                                // h323CapabilityData
  },

  { 
    // encoder
    MY_API_VERSION,                     // codec API version
    &LicenseInfo,                       // license information

    PluginCodec_MediaTypeFax |          // audio codec
    PluginCodec_InputTypeRaw |          // raw input data
    PluginCodec_OutputTypeRaw |         // RTP output data
    PluginCodec_RTPTypeDynamic,         // dynamic RTP type

    "PCM to TIFF Codec",                // text decription
    L16Format,                          // source format
    TIFFFormat,                         // destination format

    NULL,                               // user data

    8000,                               // samples per second
    BITS_PER_SECOND,                    // raw bits per second
    MICROSECONDS_PER_FRAME,             // microseconds per frame
    SAMPLES_PER_FRAME,                  // samples per frame
    BYTES_PER_FRAME,                    // bytes per frame
    PREF_FRAMES_PER_PACKET,             // recommended number of frames per packet
    MAX_FRAMES_PER_PACKET,              // maximum number of frames per packe
    0,                                  // dynamic payload
    NULL,                               // RTP payload name

    Create,                             // create codec function
    Destroy,                            // destroy codec
    Encode,                             // encode/decode
    Controls,                           // codec controls

    0,                                  // h323CapabilityType 
    NULL                                // h323CapabilityData
  },

  { 
    // decoder
    MY_API_VERSION,                     // codec API version
    &LicenseInfo,                       // license information

    PluginCodec_MediaTypeFax |          // audio codec
    PluginCodec_InputTypeRaw |          // raw input RTP
    PluginCodec_OutputTypeRaw |         // raw output data
    PluginCodec_RTPTypeDynamic,         // dynamic RTP type

    "TIFF to PCM Codec",                // text decription
    TIFFFormat,                         // source format
    L16Format,                          // destination format

    NULL,                               // user data

    8000,                               // samples per second
    BITS_PER_SECOND,                    // raw bits per second
    MICROSECONDS_PER_FRAME,             // microseconds per frame
    SAMPLES_PER_FRAME,                  // samples per frame
    BYTES_PER_FRAME,                    // bytes per frame
    PREF_FRAMES_PER_PACKET,             // recommended number of frames per packet
    MAX_FRAMES_PER_PACKET,              // maximum number of frames per packe
    0,                                  // dynamic payload
    NULL,                               // RTP payload name

    Create,                             // create codec function
    Destroy,                            // destroy codec
    Decode,                             // encode/decode
    Controls,                           // codec controls

    0,                                  // h323CapabilityType 
    NULL                                // h323CapabilityData
  },

};

/////////////////////////////////////////////////////////////////////////////

extern "C" {

PLUGIN_CODEC_IMPLEMENT_ALL(SpanDSP, faxCodecDefn, MY_API_VERSION)

};
