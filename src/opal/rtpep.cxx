/*
 * rtpep.cxx
 *
 * Open Phone Abstraction Library (OPAL)
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 19146 $
 * $Author: csoutheren $
 * $Date: 2007-12-21 11:57:11 +1100 (Fri, 21 Dec 2007) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "endpoint.h"
#endif

#include <opal/buildopts.h>

#include <opal/rtpep.h>

OpalRTPEndPoint::OpalRTPEndPoint(OpalManager & manager,     ///<  Manager of all endpoints.
                       const PCaselessString & prefix,      ///<  Prefix for URL style address strings
                                      unsigned attributes)  ///<  Bit mask of attributes endpoint has
  : OpalEndPoint(manager, prefix, attributes)
{
  defaultSecurityMode = manager.GetDefaultSecurityMode();
#if OPAL_RTP_AGGREGATE
    ,useRTPAggregation(manager.UseRTPAggregation()),
    rtpAggregationSize(10),
    rtpAggregator(NULL)
#endif
}

OpalRTPEndPoint::~OpalRTPEndPoint()
{
#if OPAL_RTP_AGGREGATE
  // delete aggregators
  {
    PWaitAndSignal m(rtpAggregationMutex);
    if (rtpAggregator != NULL) {
      delete rtpAggregator;
      rtpAggregator = NULL;
    }
  }
#endif
}

PBoolean OpalRTPEndPoint::AdjustInterfaceTable(PIPSocket::Address & /*remoteAddress*/, 
                                        PIPSocket::InterfaceTable & /*interfaceTable*/)
{
  return PTrue;
}


PBoolean OpalRTPEndPoint::IsRTPNATEnabled(OpalConnection & conn, 
                         const PIPSocket::Address & localAddr, 
                         const PIPSocket::Address & peerAddr,
                         const PIPSocket::Address & sigAddr,
                                               PBoolean incoming)
{
  return GetManager().IsRTPNATEnabled(conn, localAddr, peerAddr, sigAddr, incoming);
}

PHandleAggregator * OpalRTPEndPoint::GetRTPAggregator()
{
#if OPAL_RTP_AGGREGATE
  PWaitAndSignal m(rtpAggregationMutex);
  if (rtpAggregationSize == 0)
    return NULL;

  if (rtpAggregator == NULL)
    rtpAggregator = new PHandleAggregator(rtpAggregationSize);

  return rtpAggregator;
#else
  return NULL;
#endif
}

PBoolean OpalRTPEndPoint::UseRTPAggregation() const
{ 
#if OPAL_RTP_AGGREGATE
  return useRTPAggregation; 
#else
  return PFalse;
#endif
}

void OpalRTPEndPoint::SetRTPAggregationSize(PINDEX 
#if OPAL_RTP_AGGREGATE
                             size
#endif
                             )
{ 
#if OPAL_RTP_AGGREGATE
  rtpAggregationSize = size; 
#endif
}

PINDEX OpalRTPEndPoint::GetRTPAggregationSize() const
{ 
#if OPAL_RTP_AGGREGATE
  return rtpAggregationSize; 
#else
  return 0;
#endif
}

OpalMediaFormatList OpalRTPEndPoint::GetMediaFormats() const
{
  return OpalMediaFormat::GetAllRegisteredMediaFormats();
}

