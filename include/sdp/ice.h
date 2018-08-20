/*
 * ice.h
 *
 * Interactive Connectivity Establishment
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2010 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 */

#ifndef OPAL_SIP_ICE_H
#define OPAL_SIP_ICE_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#if OPAL_ICE

#include <opal/mediasession.h>
#include <ptclib/pstunsrvr.h>


/**String option key to an integer indicating the time in seconds to
   wait for any ICE/STUN messages. Default 5.
  */
#define OPAL_OPT_ICE_TIMEOUT "ICE-Timeout"

/**Enable ICE-Lite.
   Defaults to true.
   NOTE!!! This is all that is supported at this time, do not set to false
           unless you are developing full ICE support ....
*/
#define OPAL_OPT_ICE_LITE "ICE-Lite"

/**Execute trickle ICE.
   Defaults to false.
   NOTE!!! As only ICE-Lite is supported at this time, this effectively
           means if a endCandidates entry is appended to offers/answers.
*/
#define OPAL_OPT_TRICKLE_ICE "Trickle-ICE"

/**Support network cost in ICE candidate selection.
   Defaults to false.
*/
#define OPAL_OPT_NETWORK_COST_ICE "Network-Cost-ICE"


/** Class for low level transport of media that uses ICE
  */
class OpalICEMediaTransport : public OpalUDPMediaTransport
{
    PCLASSINFO(OpalICEMediaTransport, OpalUDPMediaTransport);
  public:
    OpalICEMediaTransport(const PString & name);
    ~OpalICEMediaTransport();

    virtual bool Open(OpalMediaSession & session, PINDEX count, const PString & localInterface, const OpalTransportAddress & remoteAddress);
    virtual bool IsEstablished() const;
    virtual void SetCandidates(const PString & user, const PString & pass, const PNatCandidateList & candidates);
    virtual bool GetCandidates(PString & user, PString & pass, PNatCandidateList & candidates, bool offering);
#if OPAL_STATISTICS
    virtual void GetStatistics(OpalMediaStatistics & statistics) const;
#endif


  protected:
    class ICEChannel : public PIndirectChannel
    {
        PCLASSINFO(ICEChannel, PIndirectChannel);
      public:
        ICEChannel(OpalICEMediaTransport & owner, SubChannels subchannel, PChannel * channel);
        virtual PBoolean Read(void * buf, PINDEX len);
      protected:
        OpalICEMediaTransport & m_owner;
        SubChannels             m_subchannel;
    };
    bool InternalHandleICE(SubChannels subchannel, const void * buf, PINDEX len);
    virtual void InternalRxData(SubChannels subchannel, const PBYTEArray & data);
    virtual bool InternalOpenPinHole(PUDPSocket & socket);
    virtual PChannel * AddWrapperChannels(SubChannels subchannel, PChannel * channel);

    PString       m_localUsername;    // ICE username sent to remote
    PString       m_localPassword;    // ICE password sent to remote
    PString       m_remoteUsername;   // ICE username expected from remote
    PString       m_remotePassword;   // ICE password expected from remote
    PTimeInterval m_iceTimeout;
    bool          m_lite;
    bool          m_trickle;
    bool          m_useNetworkCost;

    enum CandidateStates
    {
      e_CandidateInProgress,
      e_CandidateWaiting,
      e_CandidateFrozen,
      e_CandidateFailed,
      e_CandidateSucceeded
    };

#if OPAL_STATISTICS
    typedef OpalCandidateStatistics CandidateStateBase;
#else
    typedef PNatCandidate           CandidateStateBase;
#endif
    struct CandidateState : CandidateStateBase
    {
      CandidateStates m_state;
      // Not sure what else might be necessary here. Work in progress!

      CandidateState(const PNatCandidate & cand)
        : CandidateStateBase(cand)
        , m_state(e_CandidateInProgress)
      {
      }
    };
    typedef std::list<CandidateState> CandidateStateList;
    typedef std::vector<CandidateStateList> CandidatesArray;
    friend bool operator!=(const CandidatesArray & left, const CandidatesArray & right);

    CandidatesArray m_localCandidates;
    CandidatesArray m_remoteCandidates;

    enum {
      e_Disabled, // Note values and order are important
      e_Completed,
      e_Offering,
      e_OfferAnswered,
      e_Answering
    } m_state;

    PSTUNServer m_server;
    PSTUNClient m_client;

    PNatCandidate m_selectedCandidate;
};


#endif // OPAL_ICE

#endif // OPAL_SIP_ICE_H
