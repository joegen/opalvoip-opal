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
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_SIP_ICE_H
#define OPAL_SIP_ICE_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#if OPAL_ICE

#include <opal/mediasession.h>


class PSTUNClient;


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
    virtual void InternalRxData(SubChannels subchannel, const PBYTEArray & data);
    virtual void SetCandidates(const PString & user, const PString & pass, const PNatCandidateList & candidates);
    virtual bool GetCandidates(PString & user, PString & pass, PNatCandidateList & candidates, bool offering);

    const PTimeInterval & GetICESetUpTime() const { return m_maxICESetUpTime; }
    void SetICESetUpTime(const PTimeInterval & t) { m_maxICESetUpTime = t; }

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

    PString       m_localUsername;    // ICE username sent to remote
    PString       m_localPassword;    // ICE password sent to remote
    PString       m_remoteUsername;   // ICE username expected from remote
    PString       m_remotePassword;   // ICE password expected from remote
    PTimeInterval m_maxICESetUpTime;

    enum CandidateStates
    {
      e_CandidateInProgress,
      e_CandidateWaiting,
      e_CandidateFrozen,
      e_CandidateFailed,
      e_CandidateSucceeded
    };

    struct CandidateState : PNatCandidate {
      CandidateStates m_state;
      // Not sure what else might be necessary here. Work in progress!

      CandidateState(const PNatCandidate & cand)
        : PNatCandidate(cand)
        , m_state(e_CandidateInProgress)
      {
      }
    };
    typedef PList<CandidateState> CandidateStateList;
    typedef PArray<CandidateStateList> CandidatesArray;
    CandidatesArray m_localCandidates;
    CandidatesArray m_remoteCandidates;

    enum {
      e_Disabled, // Note values and order are important
      e_Completed,
      e_Offering,
      e_OfferAnswered,
      e_Answering
    } m_state;

    class Server;
    Server      * m_server;
    PSTUNClient * m_client;
};


#endif // OPAL_ICE

#endif // OPAL_SIP_ICE_H
