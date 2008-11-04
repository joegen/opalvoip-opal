/*
 * RFC 2190 packetiser and unpacketiser 
 *
 * Copyright (C) 2008 Post Increment
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
 * The Original Code is Opal
 *
 * Contributor(s): Craig Southeren <craigs@postincrement.com>
 *
 */

#ifndef _RFC2190_H_
#define _RFC2190_H_

#include <vector>
#include <list>

#include "../common/rtpframe.h"

class RFC2190Depacketizer {
  public:
    RFC2190Depacketizer();
    void NewFrame();
    int SetPacket(const RTPFrame & outputFrame, bool & requestIFrame, bool & isIFrame);

    std::vector<unsigned char> frame;

  protected:
    unsigned lastSequence;
    int LostSync(bool & requestIFrame, const char * reason);
    bool first;
    bool skipUntilEndOfFrame;
    unsigned lastEbit;
};

class RFC2190Packetizer
{
  public:
    RFC2190Packetizer();
    int Open(unsigned long timeStamp);
    int GetPacket(RTPFrame & outputFrame, unsigned int & flags);

    std::vector<unsigned char> buffer;
    unsigned int TR;
    unsigned int frameSize;
    int iFrame;
    int annexD, annexE, annexF, annexG, pQuant, cpm;
    int macroblocksPerGOB;

    unsigned long timestamp;
    std::list<int> fragments;
    std::list<int>::iterator currFrag;
    unsigned char * fragPtr;
};

#endif // _RFC2190_H_

