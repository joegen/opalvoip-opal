/*
 * h263codec.h
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2005 Salyens
 * Copyright (c) 2001 March Networks Corporation
 * Copyright (c) 1999-2000 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): Guilhem Tardy (gtardy@salyens.com)
 *
 * $Log: h263codec.h,v $
 * Revision 1.2003.2.1  2006/03/23 07:55:18  csoutheren
 * Audio plugin H.323 capability merging completed.
 * GSM, LBC, G.711 working. Speex and LPC-10 are not
 *
 * Revision 2.2  2006/02/13 03:46:16  csoutheren
 * Added initialisation stuff to make sure that everything works OK
 *
 * Revision 2.1  2006/01/01 19:19:33  dsandras
 * Added RFC2190 H.263 codec thanks to Salyens. Many thanks!
 *
 * Revision 1.1  2005/12/30 12:30:03  guilhem
 * *** empty log message ***
 *
 * Revision 1.1  2005/08/19 22:32:46  gtardy
 * Initial release of h263 codec.
 *
 */

#ifndef __OPAL_H263CODEC_H
#define __OPAL_H263CODEC_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#ifndef NO_OPAL_VIDEO

#ifdef RFC2190_AVCODEC

#ifndef NO_H323
#include <h323/h323caps.h>
#endif

#include <codec/vidcodec.h>
#include <rtp/rtp.h>

#define OPAL_H263 "H.263"

#define OPAL_H263_SQCIF "H.263(SQCIF)"
#define OPAL_H263_QCIF "H.263(QCIF)"
#define OPAL_H263_CIF  "H.263(CIF)"

extern const OpalVideoFormat & GetOpalH263();

#define OpalH263 GetOpalH263()

struct AVCodec;
struct AVCodecContext;
struct AVFrame;

namespace PWLibStupidLinkerHacks {
  extern int rfc2190h263Loader;
};
///////////////////////////////////////////////////////////////////////////////

#ifndef NO_H323

/** This class is a H.263 video capability.
 */
class H323_H263Capability : public H323VideoCapability
{
  PCLASSINFO(H323_H263Capability, H323VideoCapability)

  public:
  /**@name Construction */
  //@{
    /**Create a new H263 Capability
     */
    H323_H263Capability(
      unsigned sqcifMPI = 1,	// {1..3600 units seconds/frame, 1..32 units 1/29.97 Hz}
      unsigned qcifMPI = 2,
      unsigned cifMPI = 4,
      unsigned cif4MPI = 8,
      unsigned cif16MPI = 32,
      unsigned maxBitRate = 400,
      BOOL unrestrictedVector = FALSE,
      BOOL arithmeticCoding = FALSE, // not supported
      BOOL advancedPrediction = FALSE,
      BOOL pbFrames = FALSE,
      BOOL temporalSpatialTradeOff = FALSE, // not supported
      unsigned hrd_B = 0, // not supported
      unsigned bppMaxKb = 0, // not supported
      unsigned slowSqcifMPI = 0,
      unsigned slowQcifMPI = 0,
      unsigned slowCifMPI = 0,
      unsigned slowCif4MPI = 0,
      unsigned slowCif16MPI = 0,
      BOOL errorCompensation = FALSE // not supported
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Create a copy of the object.
      */
    virtual PObject * Clone() const;
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare object
      */
    Comparison Compare(const PObject & obj) const;
   //@}

    /**@name Identification functions */
  //@{
    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns one of the four possible combinations of mode and speed
       using the enum values of the protocol ASN H245_AudioCapability class.
     */
    virtual unsigned GetSubType() const;

    /**Get the name of the media data format this class represents.
     */
    virtual PString GetFormatName() const;
  //@}

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour sets the data rate field in the PDU.
     */
    virtual BOOL OnSendingPDU(
      H245_VideoCapability & pdu  /// PDU to set information on
    ) const;

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour sets the resolution and bit rate.
     */
    virtual BOOL OnSendingPDU(
      H245_VideoMode & pdu  /// PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour gets the data rate field from the PDU.
     */
    virtual BOOL OnReceivedPDU(
      const H245_VideoCapability & pdu  /// PDU to set information on
    );

    /** Get sqcifMPI
     */
    unsigned GetSQCIFMPI() const
      { return sqcifMPI; }

    /** Get qcifMPI
     */
    unsigned GetQCIFMPI() const
      { return qcifMPI; }

    /** Get cifMPI
     */
    unsigned GetCIFMPI() const
      { return cifMPI; }

    /** Get cif4MPI
     */
    unsigned GetCIF4MPI() const
      { return cif4MPI; }

    /** Get cif16MPI
     */
    unsigned GetCIF16MPI() const
      { return cif16MPI; }

    /** Get maximum bit rate
     */
    unsigned GetMaxBitRate() const
      { return maxBitRate; }

    /** Get unrestrictedVector capabilty
     */
    BOOL GetUnrestrictedVectorCapability() const
      { return unrestrictedVector; }

    /** Get arithmeticCoding capabilty
     */
    BOOL GetArithmeticCodingCapability() const
      { return arithmeticCoding; }

    /** Get advancedPrediction capabilty
     */
    BOOL GetAdvancedPredictionCapability() const
      { return advancedPrediction; }

    /** Get  pbFrames capabilty
     */
    BOOL GetPbFramesCapability() const
      { return pbFrames; }

    /** Get temporal/spatial tradeoff capabilty
     */
    BOOL GetTemporalSpatialTradeOffCapability() const
      { return temporalSpatialTradeOff; }

    /** Get hrd_B
     */
    BOOL GetHrd_B() const
      { return hrd_B; }

    /** Get bppMaxKb
     */
    BOOL GetBppMaxKb() const
      { return bppMaxKb; }

    /** Get slowSqcifMPI
     */
    unsigned GetSlowSQCIFMPI() const
      { return (sqcifMPI<0?-sqcifMPI:0); }

    /** Get slowQcifMPI
     */
    unsigned GetSlowQCIFMPI() const
      { return (qcifMPI<0?-qcifMPI:0); }

    /** Get slowCifMPI
     */
    unsigned GetSlowCIFMPI() const
      { return (cifMPI<0?-cifMPI:0); }

    /** Get slowCif4MPI
     */
    unsigned GetSlowCIF4MPI() const
      { return (cif4MPI<0?-cif4MPI:0); }

    /** Get slowCif16MPI
     */
    unsigned GetSlowCIF16MPI() const
      { return (cif16MPI<0?-cif16MPI:0); }

    /** Get errorCompensation capabilty
     */
    BOOL GetErrorCompensationCapability() const
      { return errorCompensation; }
  //@}

protected:

    signed sqcifMPI;		// {1..3600 units seconds/frame, 1..32 units 1/29.97 Hz}
    signed qcifMPI;
    signed cifMPI;
    signed cif4MPI;
    signed cif16MPI;

    unsigned maxBitRate;	// units of bit/s

    BOOL     unrestrictedVector;
    BOOL     arithmeticCoding;
    BOOL     advancedPrediction;
    BOOL     pbFrames;
    BOOL     temporalSpatialTradeOff;

    long unsigned hrd_B;	// units of 128 bits
    unsigned bppMaxKb;		// units of 1024 bits

    BOOL     errorCompensation;
};

/** Capability registration functions (no need for an Endpoint to get NonStandardInfo from).
 */

#define DECLARE_H263_CAP(cls, fmt, val1, val2, val3) \
  H323_DECLARE_CAPABILITY_CLASS(cls, H323_H263Capability) \
    : H323_H263Capability(val1, val2, val3, 0, 0, 6217) { } }; \
  H323_REGISTER_CAPABILITY(cls, fmt) \

#define OPAL_REGISTER_H263_H323 \
  DECLARE_H263_CAP(H323_H263_SQCIF_QCIF_CIF_Capability, OPAL_H263,       1, 1, 2) \
  DECLARE_H263_CAP(H323_H263_SQCIF_Capability,          OPAL_H263_SQCIF, 1, 0, 0) \
  DECLARE_H263_CAP(H323_H263_QCIF_Capability,           OPAL_H263_QCIF,  0, 1, 0) \
  DECLARE_H263_CAP(H323_H263_CIF_Capability,            OPAL_H263_CIF,   0, 0, 2) \

#else // ifndef NO_H323

#define OPAL_REGISTER_H263_H323

#endif // ifndef NO_H323

////////////////////////////////////////////////////////////////

class H263Packet : public PObject
{
  PCLASSINFO(H263Packet, PObject)

  public:

    H263Packet() { data_size = hdr_size = 0; hdr = data = NULL; };
    ~H263Packet() {};

    void Store(void *data, int data_size, void *hdr, int hdr_size);
    BOOL Read(unsigned & length, RTP_DataFrame & frame);

  private:

    void *data;
    int data_size;
    void *hdr;
    int hdr_size;
};

PDECLARE_LIST(H263PacketList, H263Packet)
#if 0
{
#endif
};

///////////////////////////////////////////////////////////////////////////////

class Opal_H263_YUV420P : public OpalVideoTranscoder {
  public:
    Opal_H263_YUV420P();
    ~Opal_H263_YUV420P();
    virtual PINDEX GetOptimalDataFrameSize(BOOL input) const;
    virtual BOOL ConvertFrames(const RTP_DataFrame & src, RTP_DataFrameList & dst);

  protected:
    BOOL OpenCodec();
    void CloseCodec();

    PBYTEArray encFrameBuffer;

    AVCodec        *codec;
    AVCodecContext *context;
    AVFrame        *picture;

    int frameNum;
};

class Opal_YUV420P_H263 : public OpalVideoTranscoder {
  public:
    Opal_YUV420P_H263();
    ~Opal_YUV420P_H263();
    virtual PINDEX GetOptimalDataFrameSize(BOOL input) const;
    virtual BOOL ConvertFrames(const RTP_DataFrame & src, RTP_DataFrameList & dst);

    static void RtpCallback(void *data, int data_size,
                            void *hdr, int hdr_size, void *priv_data);

  protected:
    BOOL OpenCodec();
    void CloseCodec();

    H263PacketList encodedPackets;
    H263PacketList unusedPackets;

    PBYTEArray encFrameBuffer;
    PBYTEArray rawFrameBuffer;

    PINDEX encFrameLen;
    PINDEX rawFrameLen;

    AVCodec        *codec;
    AVCodecContext *context;
    AVFrame        *picture;

    int videoQMax, videoQMin; // dynamic video quality min/max limits, 1..31
    int videoQuality; // current video encode quality setting, 1..31
    int frameNum;

    enum StdSize {UnknownStdSize, SQCIF = 1, QCIF, CIF, CIF4, CIF16, NumStdSizes};
    static int GetStdSize(int width, int height);
};

///////////////////////////////////////////////////////////////////////////////

#define OPAL_REGISTER_H263() \
          OPAL_REGISTER_H263_H323 \
          OpalTranscoderFactory::Worker<Opal_H263_YUV420P> Opal_H263_YUV420P(OpalCreateMediaFormatPair(OpalH263, OpalYUV420P)); \
          OpalTranscoderFactory::Worker<Opal_YUV420P_H263> Opal_YUV420P_H263(OpalCreateMediaFormatPair(OpalYUV420P, OpalH263))

/////////////////////////////////////////////////////////////////////////////

#endif // RFC2190_AVCODEC

#endif // NO_OPAL_VIDEO

#endif // __OPAL_H263CODEC_H
