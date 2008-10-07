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

