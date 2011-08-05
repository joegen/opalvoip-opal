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

#include "plugin-config.h"

#include "shared/pipes.h"
#include "enc-ctx.h"
#include <sys/stat.h>
#include <fstream>
#include <stdlib.h> 


std::ifstream dlStream;
std::ofstream ulStream;

unsigned msg;
unsigned val;

unsigned srcLen;
unsigned dstLen;
unsigned headerLen;
size_t bufSize;
size_t rtpSize;
unsigned char * buffer;
unsigned flags;
int ret;

X264EncoderContext* x264;


void closeAndExit()
{
  dlStream.close();
  ulStream.close();
  exit(1);
}

void writeStream (std::ofstream & stream, const void* data, unsigned bytes)
{
  stream.write((const char *)data, bytes);
  if (stream.bad())  { PTRACE(1, "x264", "IPC CP: Bad flag set on writing - terminating"); closeAndExit(); }
}

void readStream (std::ifstream & stream, void* data, unsigned bytes)
{
  stream.read((char *)data, bytes);
  if (stream.fail()) { PTRACE(1, "x264", "IPC CP: Terminating");                           closeAndExit(); }
  if (stream.bad())  { PTRACE(1, "x264", "IPC CP: Bad flag set on reading - terminating"); closeAndExit(); }
  if (stream.eof())  { PTRACE(1, "x264", "IPC CP: Received EOF - terminating");            closeAndExit(); }
}

void flushStream (std::ofstream & stream)
{
  stream.flush();
  if (stream.bad())  { PTRACE(1, "x264", "IPC CP: Bad flag set on flushing - terminating"); closeAndExit(); }
}


void resizeBuffer()
{
  size_t newBufSize = x264->GetFrameWidth()*x264->GetFrameWidth()*3/2 + dstLen + 1024;
  if (newBufSize != bufSize) {
    bufSize = newBufSize;
    buffer = (unsigned char *)realloc(buffer, newBufSize);
  }
}


int main(int argc, char *argv[])
{
  if (argc != 3) {
    fprintf(stderr, "Not to be executed directly - exiting\n");
    exit (1);
  }

  x264 = NULL;
  dstLen=1400;
  rtpSize = 8192;

  dlStream.open(argv[1], std::ios::binary);
  if (dlStream.fail()) { PTRACE(1, "x264", "IPC CP: Error when opening DL named pipe"); exit (1); }
  ulStream.open(argv[2],std::ios::binary);
  if (ulStream.fail()) { PTRACE(1, "x264", "IPC CP: Error when opening UL named pipe"); exit (1); }

  unsigned status = 1;
  readStream(dlStream, &msg, sizeof(msg));
  writeStream(ulStream,&msg, sizeof(msg)); 
  writeStream(ulStream,&status, sizeof(status)); 
  flushStream(ulStream);

  if (status == 0) {
    PTRACE(1, "x264", "IPC CP: Failed to load dynamic library - exiting"); 
    closeAndExit();
  }

  while (1) {
    readStream(dlStream, &msg, sizeof(msg));

  switch (msg) {
    case H264ENCODERCONTEXT_CREATE:
        x264 = new X264EncoderContext();
        writeStream(ulStream,&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    case H264ENCODERCONTEXT_DELETE:;
        delete x264;
        writeStream(ulStream,&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    case APPLY_OPTIONS:;
        x264->ApplyOptions();
        writeStream(ulStream,&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    case SET_TARGET_BITRATE:
        readStream(dlStream, &val, sizeof(val));
        x264->SetTargetBitrate (val);
        writeStream(ulStream,&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    case SET_FRAME_RATE:
        readStream(dlStream, &val, sizeof(val));
        x264->SetFrameRate (val);
        writeStream(ulStream,&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    case SET_FRAME_WIDTH:
        readStream(dlStream, &val, sizeof(val));
        x264->SetFrameWidth (val);
        writeStream(ulStream,&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    case SET_FRAME_HEIGHT:
        readStream(dlStream, &val, sizeof(val));
        x264->SetFrameHeight (val);
        writeStream(ulStream,&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    case SET_MAX_KEY_FRAME_PERIOD:
        readStream(dlStream, &val, sizeof(val));
        x264->SetMaxKeyFramePeriod (val);
        writeStream(ulStream,&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    case SET_TSTO:
        readStream(dlStream, &val, sizeof(val));
        x264->SetTSTO (val);
        writeStream(ulStream,&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    case SET_PROFILE_LEVEL:
        readStream(dlStream, &val, sizeof(val));
        x264->SetProfileLevel (val);
        writeStream(ulStream,&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    case ENCODE_FRAMES:
        resizeBuffer();
        readStream(dlStream, &srcLen, sizeof(srcLen));
        readStream(dlStream, buffer+rtpSize, srcLen);
        readStream(dlStream, &headerLen, sizeof(headerLen));
        readStream(dlStream, buffer, headerLen);
        readStream(dlStream, &flags, sizeof(flags));
    case ENCODE_FRAMES_BUFFERED:
        resizeBuffer();
        ret = x264->EncodeFrames(buffer+rtpSize, srcLen, buffer, dstLen, flags);
        writeStream(ulStream,&msg, sizeof(msg));
        writeStream(ulStream,&dstLen, sizeof(dstLen));
        writeStream(ulStream,buffer, dstLen);
        writeStream(ulStream,&flags, sizeof(flags));
        writeStream(ulStream,&ret, sizeof(ret));
        flushStream(ulStream);
      break;
    case SET_MAX_FRAME_SIZE:
        readStream(dlStream, &val, sizeof(val));
        x264->SetMaxRTPFrameSize (val);
        rtpSize = val;
        writeStream(ulStream,&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    default:
      break;
    }
  }
  return 0;
}
