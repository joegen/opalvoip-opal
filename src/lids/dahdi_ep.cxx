/*
 * dahdi.cxx
 *
 * DAHDI Line Interface Device
 *
 * Open Phone Abstraction Library
 *
 * Copyright (C) 2011 Post Increment
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
 * The Original Code is Opal Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "dahdi_ep.cxx"
#endif

#include <opal/buildopts.h>

#include <lids/dahdi_ep.h>

#define MAX_DTMF_QUEUE_LEN  16

#define DEFAULT_BLOCK_SIZE  160

#define new PNEW

const char * DahdiLineInterfaceDevice::DeviceName = "/dev/dahdi/ctl";

DahdiLineInterfaceDevice::DahdiLineInterfaceDevice()
  : m_samplesPerBlock(DEFAULT_BLOCK_SIZE)
  , m_thread(NULL)
  , m_running(false)
  , m_pollListDirty(true)
{
}

bool DahdiLineInterfaceDevice::Open(const PString &)
{
  PWaitAndSignal m(m_mutex);

  // close if already open
  Close();

  int fd = open(DeviceName, O_RDWR);

  // open the device
  if (fd < 0) {
    PTRACE(2, "DAHDI\tUnable to open " << DeviceName << " : " << strerror(errno));
    return false;
  }

  // scan through all spans looking for available channels
  int s;
  int baseChannel = 1;
  for (s = 1; s < DAHDI_MAX_SPANS; s++) {

    dahdi_spaninfo spanInfo;
    memset(&spanInfo, 0, sizeof(spanInfo));
    spanInfo.spanno = s;

    // if this span is not active, move to next
    if (ioctl(fd, DAHDI_SPANSTAT, &spanInfo) != 0)
      continue;

    if (IsDigitalSpan(spanInfo)) {
      PTRACE(4, "DAHDI\tSkipping digital span " << s);
    }
    else {
      int active = 0;
      for (int i = 0; i < spanInfo.totalchans; i++) {

        int c = baseChannel + i;

        dahdi_params chanInfo;
        memset(&chanInfo, 0, sizeof(chanInfo));
        chanInfo.channo = c;

        // if channel is not active, move to next
        if (ioctl(fd, DAHDI_GET_PARAMS, &chanInfo))
          continue;

        bool ok = true;

        ChannelInfo * channelInfo = NULL;

        switch (chanInfo.sigcap & (__DAHDI_SIG_FXO | __DAHDI_SIG_FXS)) {
          case __DAHDI_SIG_FXS:
            PTRACE(4, "DAHDI\tSkipping FXO channel " << (i+1) << " in span " << s);
	          break;
	        case __DAHDI_SIG_FXO:
            PTRACE(2, "DAHDI\tCreating FXS channel " << (i+1) << " in span " << s);
            channelInfo = new FXSChannelInfo(chanInfo);
            active++;
	          break;
	        default:
            PTRACE(4, "DAHDI\tSkipping unknown channel " << (i+1) << " in span " << s);
            break;
        }

        // open the channel
        if (channelInfo != NULL) {
          if (channelInfo->Open(m_samplesPerBlock)) {
            PTRACE(3, "DAHDI\tOpened channel " << (i+1) << " in span " << s);
          }
          else {
            PTRACE(2, "DAHDI\tFailed to open channel " << (i+1) << " in span " << s);
            channelInfo->Close();
            delete channelInfo;
            channelInfo = NULL;
          }
        }

        if (channelInfo != NULL)
          m_channelInfoList.push_back(channelInfo);

      }
      PTRACE(3, "DAHDI\tAnalog span " << s << " has " << active << " active channels from " << spanInfo.totalchans << " total");
    }

    // and increment the base channel number
    baseChannel += spanInfo.totalchans;
  }

  PTRACE(3, "DAHDI\t" << m_channelInfoList.size() << " channels defined");

  // build the list of FDs to poll for events
  BuildPollFDs();

  // start the monitoring thread
  m_running = true;
  m_pollListDirty = true;

  m_thread = new PThreadObj<DahdiLineInterfaceDevice>(*this, &DahdiLineInterfaceDevice::ThreadMain);

  os_handle = 1;

  return true;
}


bool DahdiLineInterfaceDevice::Close()
{
  PWaitAndSignal m(m_mutex);

  if (m_thread != NULL) {
    m_running = false;
    m_thread->WaitForTermination();
    delete m_thread;
    m_thread = NULL;
  }

  while (m_channelInfoList.size() != 0) {
    m_channelInfoList[0]->Close();
    delete m_channelInfoList[0];
    m_channelInfoList.erase(m_channelInfoList.begin());
  }

  os_handle = -1;

  return true;
}

void DahdiLineInterfaceDevice::BuildPollFDs()
{
  PWaitAndSignal m(m_pollListMutex);
  m_pollListDirty = true;

  // interrupt the monitoring thread
  // as we have the mutex, it will not start polling until we
  // have finished building the list
#ifdef P_LINUX
  if (m_thread != NULL)
    pthread_kill(m_thread->GetThreadId(), SIGUSR1);
#endif

  PTRACE(3, "DAHDI\tpoll list reset");
}

void DahdiLineInterfaceDevice::ThreadMain()
{
  PTRACE(3, "DAHDI\tthread started");
  
  std::vector<BYTE> buffer;
  buffer.resize(m_samplesPerBlock * 2);

  while (m_running) {

    {
      PWaitAndSignal m(m_pollListMutex);
      if (m_pollListDirty) {
        m_pollFds.clear();
        for (size_t i = 0; i < m_channelInfoList.size(); ++i) {
          if (!m_channelInfoList[i]->IsMediaRunning()) {
            PTRACE(3, "DAHDI\tAdding channel at offset " << i << " to poll list");
            pollfd pollFd;
            pollFd.fd      = m_channelInfoList[i]->m_fd;
            pollFd.events  = POLLIN;
            pollFd.revents = 0;
            m_pollFds.push_back(pollFd);
          }
        }
        m_pollListDirty = false;
        continue;
      }
    }

    int err = poll(&m_pollFds[0], m_pollFds.size(), -1);

    if (m_pollListDirty ||(err == 0))
      continue;

    if (err < 0) {
      if (errno != EINTR) {
        PTRACE(1, "DAHDI\tEvent poll failed - " << strerror(errno));
        break;
      }
      continue;
    }

    size_t j = 0;
    for (size_t i = 0; i < m_pollFds.size(); ++i) {
      if ((m_pollFds[i].revents & POLLIN) != 0) {
        while ((j < m_channelInfoList.size()) && 
               (m_channelInfoList[j]->m_fd != m_pollFds[i].fd)) 
          ++j;
        if (j >= m_channelInfoList.size())
          PTRACE(2, "DAHDI\tUnable to find matching channel for fd");
        else {
          ChannelInfo & channel = *m_channelInfoList[j];
          if (!channel.IsMediaRunning()) {
            if (channel.InternalReadFrame(&buffer[0]))
              channel.DetectTones(&buffer[0], buffer.size());
          }
        }
      }
    }
  }
}

bool DahdiLineInterfaceDevice::SetReadFormat(unsigned line, const OpalMediaFormat & mediaFormat)      
{ 
  if (!IsValidLine(line)) 
    return false; 

  ChannelInfo & channel = *m_channelInfoList[line];
  if (channel.StartMedia())
    BuildPollFDs();

  return channel.SetReadFormat(mediaFormat); 
}

bool DahdiLineInterfaceDevice::SetWriteFormat(unsigned line, const OpalMediaFormat & mediaFormat)
{
  if (!IsValidLine(line)) 
    return false; 

  ChannelInfo & channel = *m_channelInfoList[line];
  if (channel.StartMedia())
    BuildPollFDs();

  return channel.SetWriteFormat(mediaFormat); 
}


bool DahdiLineInterfaceDevice::StopReading(unsigned line)
{  
  if (!IsValidLine(line)) 
    return false; 

  ChannelInfo & channel = *m_channelInfoList[line];
  if (channel.StopMedia())
    BuildPollFDs();

  OpalLineInterfaceDevice::StopReading(line);
  return channel.StopReading(); 
}

bool DahdiLineInterfaceDevice::StopWriting(unsigned line)
{  
  if (!IsValidLine(line)) 
    return false; 

  ChannelInfo & channel = *m_channelInfoList[line];
  if (channel.StopMedia())
    BuildPollFDs();

  OpalLineInterfaceDevice::StopWriting(line);
  return channel.StopWriting(); 
}

////////////////////////////////////////////////////////

DahdiLineInterfaceDevice::ChannelInfo::ChannelInfo(dahdi_params & parms)
  : m_spanNumber(parms.spanno)
  , m_channelNumber(parms.channo)
  , m_chanPos(parms.chanpos)
  , m_fd(-1)
  , m_samplesPerBlock(DEFAULT_BLOCK_SIZE)
  , m_audioEnable(false)
  , m_mediaStarted(false)
  , m_toneBuffer(NULL)
  , m_toneBufferLen(0)
  , m_isALaw(parms.curlaw == DAHDI_LAW_ALAW)
  , m_writeVol(100)
  , m_readVol(100)
{ 
}


DahdiLineInterfaceDevice::ChannelInfo::~ChannelInfo()
{
  Close();
}


bool DahdiLineInterfaceDevice::ChannelInfo::Close()
{ 
  PWaitAndSignal m(m_mutex);

  if (m_fd != -1) {
    ::close(m_fd);
    m_fd = -1;
  }

  if (m_toneBuffer != NULL) { 
    delete [] m_toneBuffer;
    m_toneBuffer = NULL;
  }

  m_audioEnable = false;
  m_mediaStarted = false;

  return true;
}


bool DahdiLineInterfaceDevice::ChannelInfo::Open(int blockSize)
{ 
  PStringStream devName;
  devName << "/dev/dahdi/" << (int)m_channelNumber;

  PWaitAndSignal m(m_mutex);

  Close();

  m_fd = open((const char *)devName, O_RDWR, 0600);
  if (m_fd < 0) {
    PTRACE(2, "DAHDI\tChan " << m_channelNumber << " : Cannot open channel device " << devName << " - " << strerror(errno));
    return false;
  }

  // get parameters (not sure why - all examples do this)
  dahdi_params tp;
  if (ioctl(m_fd, DAHDI_GET_PARAMS, &tp)) {
    PTRACE(2, "DAHDI\tChan " << m_channelNumber << " : unable to get channel parameters - " << strerror(errno));
    return false;
  }

  // waste first event ((not sure why - all examples do this)
  ioctl(m_fd, DAHDI_GETEVENT);

  // flush data
  Flush();

  // set block size
  if (ioctl(m_fd, DAHDI_SET_BLOCKSIZE, &m_samplesPerBlock) < 0) {
    PTRACE(2, "DAHDI\tChan " << m_channelNumber << " cannot set block size");
    return false;
  }

  m_hasHardwareToneDetection = false;

#if 0
  // enable tone detection
  {
    int v = 1;
    if (ioctl(m_fd, DAHDI_TONEDETECT, &v) >= 0)
      m_hasHardwareToneDetection = true;
    else {
      PTRACE(2, "DAHDI\tChan " << m_channelNumber << " has no hardware tone detection");
      m_hasHardwareToneDetection = false;
    }
  }

  // turn off echo cancellation
  {
    int v = 0;
    if (ioctl(m_fd, DAHDI_ECHOCANCEL, &v) < 0) {
      PTRACE(2, "DAHDI\tChan " << m_channelNumber << " cannot disable echo cancellation");
    }
  }
#endif

#if 0
  // set tx gain
  {  
    dahdi_hwgain gain;
    gain.newgain = 0;
    gain.tx      = 0;
    if (ioctl(m_fd, DAHDI_SET_HWGAIN, &gain) < 0)
      PTRACE(2, "DAHDI\tChan " << m_channelNumber << " cannot set tx gain");
  }

  // set rx gain
  {
    dahdi_hwgain gain;
    gain.newgain = -35;
    gain.tx      = 1;
    int v = 0;
    if (ioctl(m_fd, DAHDI_SET_HWGAIN, &v) < 0)
      PTRACE(2, "DAHDI\tChan " << m_channelNumber << " cannot set rx gain");
  }
#endif

#if 0
#define NUM_BUFS 32
	struct dahdi_bufferinfo bi;
	bi.txbufpolicy = DAHDI_POLICY_IMMEDIATE;
	bi.rxbufpolicy = DAHDI_POLICY_IMMEDIATE;
	bi.numbufs = NUM_BUFS;
	bi.bufsize = 512;
	if (ioctl(fd, DAHDI_SET_BUFINFO, &bi)) {
		close(fd);
		return;
	}
#endif

  m_readBuffer.resize(m_samplesPerBlock);
  m_writeBuffer.resize(m_samplesPerBlock);

  return true;
}


void DahdiLineInterfaceDevice::ChannelInfo::Flush()
{
  int i = DAHDI_FLUSH_ALL;
  if (ioctl(m_fd,DAHDI_FLUSH,&i) == -1) {
    PTRACE(2, "DAHDI\tChan " << m_channelNumber << " : unable to flush events - " << strerror(errno));
  }
}


bool DahdiLineInterfaceDevice::ChannelInfo::EnableAudio(bool enable)
{
  PWaitAndSignal m(m_mutex);

  PTRACE(3, "DAHDI\tChan " << m_channelNumber << " " << (enable ? "en" : "dis") << "abling audio");

  m_audioEnable = enable;

  if (!enable)
    m_writeBuffer.resize(0);

  return true;
}


bool DahdiLineInterfaceDevice::ChannelInfo::SetReadFormat(const OpalMediaFormat & mediaFormat)
{
  return true;
}

bool DahdiLineInterfaceDevice::ChannelInfo::SetWriteFormat(const OpalMediaFormat & mediaFormat)
{
  return true;
}

bool DahdiLineInterfaceDevice::ChannelInfo::StopReading()
{
  return true;
}

bool DahdiLineInterfaceDevice::ChannelInfo::StopWriting()
{
  return true;
}

bool DahdiLineInterfaceDevice::ChannelInfo::StartMedia()
{
  PWaitAndSignal m(m_mutex);
  if (m_mediaStarted)
    return false;

  m_readBuffer.resize(m_samplesPerBlock);
  m_writeBuffer.resize(m_samplesPerBlock);

  PTRACE(3, "DAHDI\tChan " << m_channelNumber << " starting media");
  
  m_mediaStarted = true;
  return true;
}

bool DahdiLineInterfaceDevice::ChannelInfo::StopMedia()
{
  PWaitAndSignal m(m_mutex);
  if (!m_mediaStarted)
    return false;

  PTRACE(3, "DAHDI\tChan " << m_channelNumber << " stopping media");
  
  m_mediaStarted = false;
  return true;
}


bool DahdiLineInterfaceDevice::ChannelInfo::ReadFrame(void * buf, PINDEX & count)
{ 
  if (!m_audioEnable || m_fd < 0) 
    return false;

  // make sure buffer size is OK
  if (count > m_samplesPerBlock * 2) 
    return false;

  count = m_samplesPerBlock * 2;

  return InternalReadFrame(buf);
}

bool DahdiLineInterfaceDevice::ChannelInfo::InternalReadFrame(void * buf)
{
  PWaitAndSignal m(m_mutex);

  // read the data
  int res;
  for (;;) {
    res = read(m_fd, &m_readBuffer[0], m_samplesPerBlock);
    if (res > 0)
      break;
    if (errno == ELAST)
      LookForEvent();
    else {
      PTRACE(3, "DAHDI\tChan " << m_channelNumber << ": read of " << m_samplesPerBlock << " failed - " << strerror(errno));
      return false;
    }
  }

  if (res != m_samplesPerBlock) {
    PTRACE(3, "DAHDI\tChan " << m_channelNumber << ": read of " << m_samplesPerBlock << " returned " << res << " instead");
    return false;
  }

  // convert and scale
  {
    const BYTE * src = (const BYTE *)&m_readBuffer[0];
    short * dst = (short *)buf;
    for (int i = 0; i < m_samplesPerBlock; ++i)
      *dst++ = DecodeSample((int)(*src++)); // * m_readVol / 100);
  }

  return true;
}

bool DahdiLineInterfaceDevice::ChannelInfo::WriteFrame(const void * buf, PINDEX count, PINDEX & written)
{  
  //PWaitAndSignal m(m_mutex);

  if (!m_audioEnable || m_fd < 0) 
    return false;

  // make sure buffer size is OK
  if (count > (PINDEX)m_writeBuffer.size()*2) {
PTRACE(2, "write called with " << count << " which is > " << m_writeBuffer.size()*2);
    return false;
  }

  // insert tone if playing, else encode and scale the data
#if 0
  if (m_toneBufferLen > 0) {
    int toSend = 0;
    while (toSend < count) {
      int toCopy = PMIN(count - toSend, m_toneBufferLen - m_toneBufferPos);
      memcpy(&m_writeBuffer[0], m_toneBuffer + m_toneBufferPos, toCopy);
      toSend += toCopy;
      m_toneBufferPos = (m_toneBufferPos + toCopy) % m_toneBufferLen;
    }
    buf = m_toneBuffer;
  }
  else
#endif
  {
    const short * src = (const short *)buf;
    BYTE * dst = (BYTE *)&m_writeBuffer[0];
    for (int i = 0; i < m_samplesPerBlock; ++i)
      *dst++ = EncodeSample((int)(*src++)); // * m_writeVol / 100);
  }

  // write data
  for (;;) {
    int res = write(m_fd, &m_writeBuffer[0], m_samplesPerBlock);
    if (res < 0)
      return false;
    if (res < m_samplesPerBlock)
      LookForEvent();
    else {
      written = count;
      return true;
    }
  }

  return false; // should never happen
}

bool DahdiLineInterfaceDevice::ChannelInfo::DetectTones(void * buffer, int len)
{
  PWaitAndSignal m(m_mutex);

  if (m_hasHardwareToneDetection)
    return true;

  PString tones = m_dtmfDecoder.Decode((const short *)buffer, len/2);

  if (tones.GetLength() > 0) {
    PWaitAndSignal m(m_dtmfMutex);
    for (PINDEX i = 0; i < tones.GetLength() && (m_dtmfQueue.size() < MAX_DTMF_QUEUE_LEN); ++i) {
      PTRACE(3, "DAHDI\tChan " << m_channelNumber << " : decoded tone '" << tones[i] << "'");
      m_dtmfQueue.push(tones[i]);
    }
  }

  return true;
}


bool DahdiLineInterfaceDevice::ChannelInfo::LookForEvent()
{
  int e;
  int r;
  {
    PWaitAndSignal m(m_mutex);
    r = ioctl(m_fd, DAHDI_GETEVENT, &e);
  }
  if (r == -1) {
    PTRACE(2, "DAHDI\tChan " << m_channelNumber << " : cannot get event");
    return false;
  }

  switch (e) {
    case -1:
      PTRACE(2, "DAHDI\tChan " << m_channelNumber << " : unknown event");
      break;

    case DAHDI_EVENT_NONE:
      break;

    case DAHDI_EVENT_ONHOOK:
      PTRACE(2, "DAHDI\tChan " << m_channelNumber << " : onhook");
      {
        PWaitAndSignal m(m_mutex);
        OnHook();
      }
      break;

    case DAHDI_EVENT_RINGOFFHOOK:
      PTRACE(2, "DAHDI\tChan " << m_channelNumber << " : offhook/ring");
      {
        PWaitAndSignal m(m_mutex);
        OffHook();
      }
      break;

    default :
      PTRACE(2, "DAHDI\tChan " << m_channelNumber << " : unknown event " << e);
      break;
  }

  return true;
}

bool DahdiLineInterfaceDevice::ChannelInfo::PlayTone(CallProgressTones tone)
{
  // stop any existing tone
  {
    PWaitAndSignal m(m_mutex);

    if (!m_audioEnable || m_fd < 0) 
      return false;

    if (IsTonePlaying())
      StopTone();
  }

  // create PCM-16 version of tone
  PDTMFEncoder toneEncoder;
  switch (tone) {
    case OpalLineInterfaceDevice::DialTone:
      toneEncoder.GenerateDialTone();
      break;
    case OpalLineInterfaceDevice::BusyTone:
    case OpalLineInterfaceDevice::CongestionTone:
      toneEncoder.GenerateBusyTone();
      break;
    case OpalLineInterfaceDevice::RingTone:
      toneEncoder.GenerateRingBackTone();
      break;
    case OpalLineInterfaceDevice::CNGTone:
      toneEncoder.AddTone('X', 500);
      toneEncoder.Generate(' ', 0, 0, 3500);
      break;
    case OpalLineInterfaceDevice::CEDTone:
      toneEncoder.AddTone('Y', 500);
      toneEncoder.Generate(' ', 0, 0, 3500);
      break;
    case OpalLineInterfaceDevice::ClearTone:
    case OpalLineInterfaceDevice::MwiTone:
    case OpalLineInterfaceDevice::RoutingTone:
    default:
      return false;
  }

  // convert to G.711
  BYTE * buffer = new BYTE [toneEncoder.GetSize()];
  BYTE * dst = buffer;
  short * src = toneEncoder.GetPointer();
  for (PINDEX i = 0; i < toneEncoder.GetSize(); ++i)
    *dst++ = EncodeSample(*src++);

  // switch buffers
  PWaitAndSignal m(m_mutex);
  delete [] m_toneBuffer;
  m_toneBuffer = buffer;
  m_toneBufferLen = toneEncoder.GetSize();

  PTRACE(3, "DAHDI\tChan " << m_channelNumber << " : tone " << tone << " started");

  return true;
}

bool DahdiLineInterfaceDevice::ChannelInfo::IsTonePlaying()
{
  PWaitAndSignal m(m_mutex);
  return m_toneBufferLen > 0;
}

bool DahdiLineInterfaceDevice::ChannelInfo::StopTone()
{
  PTRACE(3, "DAHDI\tChan " << m_channelNumber << " : tone stopped");
  PWaitAndSignal m(m_mutex);
  m_toneBufferLen = 0;
  return true;
}

char DahdiLineInterfaceDevice::ChannelInfo::ReadDTMF()
{
  PWaitAndSignal m(m_dtmfMutex);
  if (m_dtmfQueue.size() == 0)
    return '\0';
  char ch = m_dtmfQueue.front();
  m_dtmfQueue.pop();
  return ch;
}

short DahdiLineInterfaceDevice::ChannelInfo::DecodeSample(BYTE sample)
{
  return m_isALaw ? Opal_G711_ALaw_PCM::ConvertSample(sample) : Opal_G711_uLaw_PCM::ConvertSample(sample);
}

BYTE DahdiLineInterfaceDevice::ChannelInfo::EncodeSample(short sample)
{
  return m_isALaw ? Opal_PCM_G711_ALaw::ConvertSample(sample) : Opal_PCM_G711_uLaw::ConvertSample(sample);
}

////////////////////////////////////////////////////////

DahdiLineInterfaceDevice::FXSChannelInfo::FXSChannelInfo(dahdi_params & parms)
  : ChannelInfo(parms)
  , m_hookState(eOnHook)
{
}


DahdiLineInterfaceDevice::FXSChannelInfo::~FXSChannelInfo()
{
  Close();
}


void DahdiLineInterfaceDevice::FXSChannelInfo::OnHook()
{
  m_hookState = eOnHook;
}

void DahdiLineInterfaceDevice::FXSChannelInfo::OffHook()
{
  // set hook state
  m_hookState = eOffHook;

  // make sure audio is up to date
  Flush();
}


