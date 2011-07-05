/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
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
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

%Module(name=pyvoip.opal, keyword_arguments="All")

%Import ptlib/ptlibmod.sip

typedef PList<OpalListener> OpalListenerList;

%Include RTP.sip
%Include OpalMediaType.sip
%Include OpalConnection.sip
%Include OpalRTPConnection.sip
%Include OpalPCSSConnection.sip
%Include OpalSilenceDetector.sip
%Include OpalEchoCanceler.sip
%Include OpalMediaFormat.sip
%Include OpalMediaCommand.sip
%Include OpalMediaPatch.sip
%Include OpalMediaStream.sip
%Include OpalTranscoder.sip
%Include OpalVideoRateController.sip
%Include OpalCall.sip
%Include OpalManager.sip
%Include OpalEndPoint.sip
%Include OpalLocalEndPoint.sip
%Include OpalRTPEndPoint.sip
%Include OpalTransport.sip
%Include OpalPresenceInfo.sip
%Include OpalRecordManager.sip
%Include OpalAudioMixer.sip
%Include OpalJitterBuffer.sip
%Include H323Connection.sip
%Include H323TransportAddress.sip
%Include H323Gatekeeper.sip
%Include H323EndPoint.sip
%Include H323Channel.sip
%Include H323Capability.sip
%Include T140String.sip
%Include SDP.sip
%Include SIPPDU.sip
%Include SIPHandler.sip
%Include RFC4103Context.sip
%Include SIPConnection.sip
%Include SIPEndPoint.sip
%Include OpalFaxEndPoint.sip
