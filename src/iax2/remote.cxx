/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Class to manage information information about the remote node.
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2005 Indranet Technologies Ltd.
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
 * The Initial Developer of the Original Code is Indranet Technologies Ltd.
 *
 * The author of this code is Derek J Smithies
 *
 *
 *
 * $Log: remote.cxx,v $
 * Revision 1.2  2005/08/04 08:14:17  rjongbloed
 * Fixed Windows build under DevStudio 2003 of IAX2 code
 *
 * Revision 1.1  2005/07/30 07:01:33  csoutheren
 * Added implementation of IAX2 (Inter Asterisk Exchange 2) protocol
 * Thanks to Derek Smithies of Indranet Technologies Ltd. for
 * writing and contributing this code
 *
 *
 *
 */

#include <ptlib.h>

#ifdef P_USE_PRAGMA
#pragma implementation "remote.h"
#endif

#include <iax2/remote.h>
#include <iax2/frame.h>

#define new PNEW


Remote::Remote()
{
  sourceCallNumber  = callNumberUndefined;
  destCallNumber    = callNumberUndefined;
  
  remotePort        = 0;
}



void Remote::Assign(Remote & source)
{
  destCallNumber   = source.SourceCallNumber();
  sourceCallNumber = source.DestCallNumber();
  remoteAddress    = source.RemoteAddress();
  remotePort       = source.RemotePort();
}

BOOL Remote::operator==(Remote & other)
{
  if (remoteAddress != other.RemoteAddress()) {
    PTRACE(3, "Comparison of two remotes " << endl << other << endl << (*this) );
    PTRACE(4, "comparison of two remotes  Addresses are different");
    return FALSE;
  }
  
  if (remotePort != other.RemotePort()) {
    PTRACE(3, "Comparison of two remotes " << endl << other << endl << (*this) );
    PTRACE(4, "comparison of two remotes  remote ports are different");
    return FALSE;
  }
  
  if (destCallNumber != other.DestCallNumber()) {
    PTRACE(3, "Comparison of two remotes " << endl << other << endl << (*this) );
    PTRACE(4, "comparison of two remotes. Dest call numbers differ");	
    return FALSE;
  }
  
  if (sourceCallNumber != other.SourceCallNumber()) {
    PTRACE(3, "Comparison of two remotes " << endl << other << endl << (*this) );
    PTRACE(4, "comparison of two remotes. Source call numbers differ");	
    return FALSE;
  }
  
  return TRUE;
}

BOOL Remote::operator*=(Remote & other)
{
  PTRACE(6, "Incoming ethernet frame. Compare" << endl << other << endl << (*this) );
  
  if (remoteAddress != other.RemoteAddress()) {
    PTRACE(3, "comparison of two remotes  Addresses are different");
    return FALSE;
  }
  
  if (remotePort != other.RemotePort()) {
    PTRACE(3, "comparison of two remotes  remote ports are different");
    return FALSE;
  }
  
  if ((sourceCallNumber != other.DestCallNumber()) && (other.DestCallNumber() != callNumberUndefined)) {
    PTRACE(3, "comparison of two remotes. Local source number differs to incoming dest call number");
    PTRACE(3, " local sourceCallNumber " << sourceCallNumber 
	   << "        incoming Dest " << other.DestCallNumber());
    return FALSE;
  }
  
  PTRACE(6, "comparison of two remotes  They are the same  ");
  return TRUE;
}


BOOL Remote::operator!=(Remote & other)
{
  return !(*this == other);
}

void Remote::PrintOn(ostream & strm) const
{
  strm << "src call number" << sourceCallNumber 
       << "        Dest call number" << destCallNumber 
       << "        remote address" << remoteAddress 
       << "                Remote port" << remotePort ;
}


////////////////////////////////////////////////////////////////////////////////
 //paranoia here. Use brackets to guarantee the order of calculation.
FrameIdValue::FrameIdValue(PINDEX timeStamp, PINDEX seqVal)
{
  value = (timeStamp << 8) + (seqVal & 0xff);
}

FrameIdValue::FrameIdValue(PINDEX val)
{
  value = val;
}


PINDEX FrameIdValue::GetPlainSequence() const
{
  return (PINDEX)(value & P_MAX_INDEX);
}

PINDEX FrameIdValue::GetTimeStamp() const
{
  return (PINDEX)(value >> 8);            
}

PINDEX FrameIdValue::GetSequenceVal() const 
{
  return (PINDEX)(value & 0xff);         
}

void FrameIdValue::PrintOn(ostream & strm) const 
{
  strm << setw(8) << GetTimeStamp() << " --" << setw(4) << GetSequenceVal(); 
}


PObject::Comparison FrameIdValue::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, FrameIdValue), PInvalidCast);
  const FrameIdValue & other = (const FrameIdValue &)obj;
 
  if ((value > 224) && (other.value < 32))
    return LessThan;   //value has wrapped around 256, other has not.

  if ((value < 32) && (other.value > 224))
    return GreaterThan; //value has wrapped around 256, other has not.

  if (value < other.value)
    return LessThan;
 
  if (value > other.value)
    return GreaterThan;
 
  return EqualTo;
} 

////////////////////////////////////////////////////////////////////////////////
BOOL PacketIdList::Contains(FrameIdValue &src)
{
	PINDEX idex = GetValuesIndex(src);
	return idex != P_MAX_INDEX;
}

PINDEX PacketIdList::GetFirstValue()
{
  if (GetSize() == 0)
    return 255;

  return ((FrameIdValue *)GetAt(0))->GetPlainSequence();
}

void PacketIdList::RemoveOldContiguousValues()
{
  BOOL contiguous = TRUE;
  while((GetSize() > 1) && contiguous)  {
    PINDEX first = ((FrameIdValue *)GetAt(0))->GetPlainSequence();
    PINDEX second = ((FrameIdValue *)GetAt(1))->GetPlainSequence();
    contiguous = ((first + 1) & 0xff) == second;
    if (contiguous)
      RemoveAt(0);
  }
}

void PacketIdList::PrintOn(ostream & strm) const
{
  strm << "Packet Id List Size=" << GetSize() << endl;
  for(PINDEX i = 0; i < GetSize(); i++)
    strm << (*((FrameIdValue *)GetAt(i))) << endl;
}

void PacketIdList::AppendNewFrame(FullFrame &src)
{
  FrameIdValue *f = new FrameIdValue(src.GetSequenceInfo().OutSeqNo());
  PTRACE(3, "AppendNewFrame " << (*f));

  if(GetSize() == 0) {
    PTRACE(3, "SeqNos\tList empty, so add now. " << (*f));
    Append(f);
    return;
  }

  if (Contains(*f)) {
    PTRACE(3, "SeqNos\tJustRead frame is " << (*f));
    PTRACE(3, "SeqNos\tIn queue waiting removal " << (*f));
    delete f;
    return;
  }

  if (((FrameIdValue *)GetAt(0))->Compare(*f) == GreaterThan) {
    PTRACE(3, "SeqNos\tHave already processed " << (*f));
    PTRACE(3, "SeqNos\tFirst frame in que " << (*(FrameIdValue *)GetAt(0)));
    PTRACE(3, "SeqNos\tFrame just read is " << (*f));
    delete f;
    return;
  }

  PTRACE(3, "SeqNos\tList is younger than this value. " << (*f));
  Append(f);
  RemoveOldContiguousValues();

  PTRACE(3, "SeqNos\tzzzz " << endl  << (*this));
}

////////////////////////////////////////////////////////////////////////////////

void SequenceNumbers::ZeroAllValues()
{
  PWaitAndSignal m(mutex);

  inSeqNo  = 0;
  outSeqNo = 0;
  lastSentTimeStamp = 0;
}



BOOL  SequenceNumbers::operator != (SequenceNumbers &other)
{
  PWaitAndSignal m(mutex);
  if (inSeqNo == other.InSeqNo())
    return FALSE;
  
  if (inSeqNo == other.OutSeqNo())
    return FALSE;
  
  if (outSeqNo == other.InSeqNo())
    return FALSE;
  
  if (outSeqNo == other.OutSeqNo())
    return FALSE;
  
  return TRUE;
}

void SequenceNumbers::SetAckSequenceInfo(SequenceNumbers & other)
{
  PWaitAndSignal m(mutex);
  outSeqNo = other.InSeqNo();
}

BOOL SequenceNumbers::operator == (SequenceNumbers &other)
{
  PWaitAndSignal m(mutex);
  if ((inSeqNo == other.InSeqNo()) && (outSeqNo == other.OutSeqNo()))
    return TRUE;
  
  
  if ((inSeqNo == other.OutSeqNo()) && (outSeqNo == other.InSeqNo()))
    return TRUE;
  
  return FALSE;
}


void SequenceNumbers::MassageSequenceForSending(FullFrame &src)
{
  PWaitAndSignal m(mutex);

  inSeqNo = (receivedLog.GetFirstValue() + 1) & 0xff;
  PTRACE(3, "SeqNos\tsentreceivedoseqno is " << inSeqNo);

  if (src.IsAckFrame()) {
    PTRACE(3, "SeqNos\tMassage - SequenceForSending(FullFrame &src) ACK Frame");
    src.ModifyFrameHeaderSequenceNumbers(inSeqNo, src.GetSequenceInfo().OutSeqNo());
    return;
  } 

  PTRACE(3, "SeqNos\tMassage - SequenceForSending(FullFrame &src) ordinary Frame");

  PINDEX timeStamp = src.GetTimeStamp();
  if ((timeStamp < (lastSentTimeStamp + 3)) && (!src.IsNewFrame())) {
    timeStamp = lastSentTimeStamp + 3;
    src.ModifyFrameTimeStamp(timeStamp);
  }

  lastSentTimeStamp = timeStamp;
  src.ModifyFrameHeaderSequenceNumbers(inSeqNo, outSeqNo);
  
  outSeqNo++;
}

BOOL SequenceNumbers::IncomingMessageIsOk(FullFrame &src)
{
  PWaitAndSignal m(mutex);

  receivedLog.AppendNewFrame(src);
  PTRACE(3, "SeqNos\treceivedoseqno is " << src.GetSequenceInfo().OutSeqNo());
  PTRACE(3, "SeqNos\tReceived log of sequence numbers is " << endl << receivedLog);

  return TRUE;
}



void SequenceNumbers::CopyContents(SequenceNumbers &src)
{
  PWaitAndSignal m(mutex);
  inSeqNo = src.InSeqNo();
  outSeqNo = src.OutSeqNo();
}

PString SequenceNumbers::AsString() const
{
  PWaitAndSignal m(mutex);
  PStringStream  res;
  res << "   in" << inSeqNo << "   out" << outSeqNo ;
  
  return res;
}

void SequenceNumbers::PrintOn(ostream & strm) const
{
  strm << AsString();
}

PINDEX SequenceNumbers::InSeqNo() 
{ 
  PWaitAndSignal m(mutex);
  return inSeqNo; 
}
  
PINDEX SequenceNumbers::OutSeqNo() 
{
  PWaitAndSignal m(mutex);
  return outSeqNo; 
}
  
BOOL SequenceNumbers::IsSequenceNosZero() 
{ 
  PWaitAndSignal m(mutex);
  return ((inSeqNo & 0xff) == 0) && ((outSeqNo & 0xff) == 0); 
}

void SequenceNumbers::SetInSeqNo(PINDEX newVal) 
{
  PWaitAndSignal m(mutex);
  inSeqNo = newVal; 
}
  
void SequenceNumbers::SetOutSeqNo(PINDEX newVal) 
{
  PWaitAndSignal m(mutex);
  outSeqNo = newVal;
}




/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 4 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-file-style:linux
 * c-basic-offset:2
 * End:
 */

