/*
 * H.264 Plugin codec for OpenH323/OPAL
 *
 * Copyright (C) 2007 Matthias Schneider, All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef _MSC_VER
#include "../../common/platform.h"
#endif

#include <iostream>
#include <fstream>
#include <stdlib.h> 
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


#ifndef PLUGINCODEC_TRACING
#define PLUGINCODEC_TRACING 0
#endif

#if PLUGINCODEC_TRACING
  unsigned TraceLevel = 1;
  #define PTRACE_CHECK(level) (level <= TraceLevel)
  #define PTRACE(level,module,args) if (level > TraceLevel) ; else std::cerr << module << '\t' << args << std::endl
#else
  #define PTRACE_CHECK(level)
  #define PTRACE(level,module,args)
#endif


#include "../shared/x264wrap.cxx"
#include "../shared/h264frame.cxx"
#include "../../common/ffmpeg.cxx"
#include "../../common/dyna.cxx"


static const char Version[] = "$Revision$";


#ifdef WIN32

HANDLE hNamedPipe;

void OpenPipe(const char * name, const char *)
{
  if (!WaitNamedPipe(name, NMPWAIT_USE_DEFAULT_WAIT)) { 
    PTRACE(1, HelperTraceName, "Error when waiting for Pipe (" << GetLastError() << ')');
    exit(1);
  } 

  hNamedPipe = CreateFile(name,                         // pipe name 
                          GENERIC_READ | GENERIC_WRITE, // read and write access 
                          0,                            // no sharing 
                          NULL,                         // default security attributes
                          OPEN_EXISTING,                // opens existing pipe 
                          0,                            // default attributes 
                          NULL);                        // no template file 
  if (hNamedPipe == INVALID_HANDLE_VALUE)  {
    PTRACE(1, HelperTraceName, "Could not open Pipe (" << GetLastError() << ')');
    exit(1);
  }

  // The pipe connected; change to message-read mode. 
  DWORD dwMode = PIPE_READMODE_MESSAGE; 
  if (!SetNamedPipeHandleState(hNamedPipe,   // pipe handle 
                               &dwMode,  // new pipe mode 
                               NULL,     // don't set maximum bytes 
                               NULL))    // don't set maximum time 
  {
    PTRACE(1, HelperTraceName, "Failure on activating message mode (" << GetLastError() << ')');
    exit(1);
  }
}


void ReadPipe(LPVOID data, unsigned bytes)
{
  DWORD bytesRead;
  if (ReadFile(hNamedPipe, data, bytes, &bytesRead, NULL) && bytes == bytesRead)
    return;

  PTRACE(1, HelperTraceName, "IPC\tCP: Failure on reading - terminating (Read " << bytesRead << " bytes, expected " << bytes);
  exit(1);
}


void WritePipe(LPCVOID data, unsigned bytes)
{
  DWORD bytesWritten;
  if (WriteFile(hNamedPipe, data, bytes, &bytesWritten, NULL) && bytes == bytesWritten)
    return;

  PTRACE(1, HelperTraceName, "IPC\tCP: Failure on writing - terminating (Written " << bytesWritten << " bytes, intended " << bytes);
  exit(1);
}


#else // WIN32

#include <errno.h>
#include <fcntl.h>

int downLink;
int upLink;

void OpenPipe(const char * dl, const char * ul)
{
  if ((downLink = open(dl, O_RDONLY)) < 0) {
    PTRACE(1, HelperTraceName, "Error when opening DL named pipe \"" << dl << "\" - " << strerror(errno));
    exit(1);
  }

  if ((upLink = open(ul, O_WRONLY)) < 0) {
    PTRACE(1, HelperTraceName, "Error when opening UL named pipe \"" << ul << "\" - " << strerror(errno));
    exit(1);
  }
}


void ReadPipe(void * data, unsigned bytes)
{
  if (read(downLink, data, bytes) == bytes)
    return;

  if (errno != 0) {
    PTRACE(1, HelperTraceName, "Error reading down link: " << strerror(errno));
  }

  exit(1);
}


void WritePipe(const void * data, unsigned bytes)
{
  if (write(upLink, data, bytes) == bytes)
    return;

  PTRACE(1, HelperTraceName, "Error writing up link: " << strerror(errno));
  exit(1);
}


#endif // WIN32


unsigned val;

unsigned srcLen;
unsigned headerLen;
size_t bufSize;
size_t rtpSize;
unsigned char * buffer;
unsigned flags;

H264Encoder x264;


void ResizeBuffer()
{
  size_t newBufSize = x264.GetWidth()*x264.GetHeight()*3/2 + rtpSize + 1024;
  if (newBufSize > bufSize) {
    buffer = (unsigned char *)realloc(buffer, newBufSize);
    if (buffer == NULL) {
      PTRACE(3, HelperTraceName, "Could not resize frame buffer from " << bufSize << " to " << newBufSize);
      exit(1);
    }
    PTRACE(3, HelperTraceName, "Resizing frame buffer from " << bufSize << " to " << newBufSize);
    bufSize = newBufSize;
  }
}


int main(int argc, char *argv[])
{
  if (argc < 2) {
    std::cerr << "Not to be executed directly - exiting\n";
    exit(1);
  }

#if PLUGINCODEC_TRACING
  const char * traceLevelStr = getenv("PTLIB_TRACE_LEVEL");
  if (traceLevelStr != NULL)
    TraceLevel = atoi(traceLevelStr);
#endif

  OpenPipe(argv[1], argv[2]);

  PTRACE(5, HelperTraceName, "GPL executable ready");

  rtpSize = 1500;

  for (;;) {
    unsigned msg;
    ReadPipe(&msg, sizeof(msg));

    switch (msg) {
      case H264ENCODERCONTEXT_CREATE:
          msg = atoi(&Version[11]);
          WritePipe(&msg, sizeof(msg)); 
        break;
      case H264ENCODERCONTEXT_DELETE:
          WritePipe(&msg, sizeof(msg)); 
        break;
      case APPLY_OPTIONS:
          x264.ApplyOptions();
          WritePipe(&msg, sizeof(msg)); 
        break;
      case SET_TARGET_BITRATE:
          ReadPipe(&val, sizeof(val));
          x264.SetTargetBitrate(val);
          WritePipe(&msg, sizeof(msg)); 
        break;
      case SET_RATE_CONTROL_PERIOD:
          ReadPipe(&val, sizeof(val));
          x264.SetRateControlPeriod(val);
          WritePipe(&msg, sizeof(msg)); 
        break;
      case SET_FRAME_RATE:
          ReadPipe(&val, sizeof(val));
          x264.SetFrameRate (val);
          WritePipe(&msg, sizeof(msg)); 
        break;
      case SET_FRAME_WIDTH:
          ReadPipe(&val, sizeof(val));
          x264.SetFrameWidth (val);
          WritePipe(&msg, sizeof(msg)); 
        break;
      case SET_FRAME_HEIGHT:
          ReadPipe(&val, sizeof(val));
          x264.SetFrameHeight (val);
          WritePipe(&msg, sizeof(msg)); 
        break;
      case SET_MAX_KEY_FRAME_PERIOD:
          ReadPipe(&val, sizeof(val));
          x264.SetMaxKeyFramePeriod (val);
          WritePipe(&msg, sizeof(msg)); 
        break;
      case SET_TSTO:
          ReadPipe(&val, sizeof(val));
          x264.SetTSTO (val);
          WritePipe(&msg, sizeof(msg)); 
        break;
      case SET_PROFILE_LEVEL:
          ReadPipe(&val, sizeof(val));
          x264.SetProfileLevel((val>>16)&0xff, val&0xff, (val>>8)&0xff);
          WritePipe(&msg, sizeof(msg)); 
        break;
      case ENCODE_FRAMES:
          ResizeBuffer();
          ReadPipe(&srcLen, sizeof(srcLen));
          ReadPipe(buffer+rtpSize, srcLen);
          ReadPipe(&headerLen, sizeof(headerLen));
          ReadPipe(buffer, headerLen);
          ReadPipe(&flags, sizeof(flags));
      case ENCODE_FRAMES_BUFFERED:
        {
          ResizeBuffer();
          unsigned dstLen = rtpSize;
          unsigned ret = x264.EncodeFrames(buffer+rtpSize, srcLen, buffer, dstLen, headerLen, flags);
          WritePipe(&msg, sizeof(msg));
          WritePipe(&dstLen, sizeof(dstLen));
          WritePipe(buffer, dstLen);
          WritePipe(&flags, sizeof(flags));
          WritePipe(&ret, sizeof(ret));
        }
        break;
      case SET_MAX_PAYLOAD_SIZE:
          ReadPipe(&val, sizeof(val));
          x264.SetMaxRTPPayloadSize(val);
          rtpSize = val+100; // Allow for standard 12 byte RTP header and a LOT of header extensions
          WritePipe(&msg, sizeof(msg)); 
        break;
      case SET_MAX_NALU_SIZE:
          ReadPipe(&val, sizeof(val));
          x264.SetMaxNALUSize(val);
          WritePipe(&msg, sizeof(msg)); 
        break;
      default:
        break;
    }
  }
}
