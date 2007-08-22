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

#include "shared/pipes.h"
#include "enc-ctx.h"
#include <sys/stat.h>
#include <fstream>
#include "trace.h"

#define MAX_FRAME_SIZE 608286

std::ifstream dlStream;
std::ofstream ulStream;

unsigned msg;
int val;

unsigned srcLen;
unsigned dstLen;
unsigned headerLen;
unsigned char src [MAX_FRAME_SIZE];
unsigned char dst [MAX_FRAME_SIZE];
unsigned flags;
int ret;

X264EncoderContext* x264;

void closeAndExit()
{
  dlStream.close();
  if (dlStream.fail()) { TRACE (1, "H264\tIPC\tCP: Error when closing DL named pipe"); }
  ulStream.close();
  if (ulStream.fail()) { TRACE (1, "H264\tIPC\tCP: Error when closing UL named pipe"); }
  exit(1);
}

void writeStream (std::ofstream & stream, const char* data, unsigned bytes)
{
  stream.write(data, bytes);
  if (stream.bad())  { TRACE (1, "H264\tIPC\tCP: Bad flag set on writing - terminating"); closeAndExit(); }
}

void readStream (std::ifstream & stream, char* data, unsigned bytes)
{
  stream.read(data, bytes);
  if (stream.fail()) { TRACE (1, "H264\tIPC\tCP: Failure on reading - terminating"); closeAndExit();      }
  if (stream.bad())  { TRACE (1, "H264\tIPC\tCP: Bad flag set on reading - terminating"); closeAndExit(); }
  if (stream.eof())  { TRACE (1, "H264\tIPC\tCP: Received EOF - terminating"); closeAndExit();            }
}

void flushStream (std::ofstream & stream)
{
  stream.flush();
  if (stream.bad())  { TRACE (1, "H264\tIPC\tCP: Bad flag set on flushing - terminating"); closeAndExit(); }
}


int main(int argc, char *argv[])
{
  if (argc != 2) { fprintf(stderr, "Not to be executed directly - exiting\n"); exit (1); }

  char * debug_level = getenv ("PWLIB_TRACE_CODECS");
  if (debug_level!=NULL) {
      Trace::SetLevel(atoi(debug_level));
  } 
  else {
    Trace::SetLevel(0);
  }
		      
  x264 = NULL;
  dstLen=1400;
  dlStream.open(argv[0], std::ios::binary);
  if (dlStream.fail()) { TRACE (1, "H264\tIPC\tCP: Error when opening DL named pipe"); exit (1); }
  ulStream.open(argv[1],std::ios::binary);
  if (ulStream.fail()) { TRACE (1, "H264\tIPC\tCP: Error when opening UL named pipe"); exit (1); }

  while (1) {
    readStream(dlStream, (char*)&msg, sizeof(msg));

  switch (msg) {
    case H264ENCODERCONTEXT_CREATE:
        x264 = new X264EncoderContext();
        writeStream(ulStream,(char*)&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    case H264ENCODERCONTEXT_DELETE:;
        delete x264;
        writeStream(ulStream,(char*)&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    case APPLY_OPTIONS:;
        x264->ApplyOptions ();
        writeStream(ulStream,(char*)&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    case SET_TARGET_BITRATE:
        readStream(dlStream, (char*)&val, sizeof(val));
        x264->SetTargetBitRate (val);
        writeStream(ulStream,(char*)&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    case SET_FRAME_RATE:
        readStream(dlStream, (char*)&val, sizeof(val));
        x264->SetFrameRate (val);
        writeStream(ulStream,(char*)&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    case SET_FRAME_WIDTH:
        readStream(dlStream, (char*)&val, sizeof(val));
        x264->SetFrameWidth (val);
        writeStream(ulStream,(char*)&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    case SET_FRAME_HEIGHT:
        readStream(dlStream, (char*)&val, sizeof(val));
        x264->SetFrameHeight (val);
        writeStream(ulStream,(char*)&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    case ENCODE_FRAMES:
        readStream(dlStream, (char*)&srcLen, sizeof(srcLen));
        readStream(dlStream, (char*)&src, srcLen);
        readStream(dlStream, (char*)&headerLen, sizeof(headerLen));
        readStream(dlStream, (char*)&dst, headerLen);

    case ENCODE_FRAMES_BUFFERED:
        ret = (x264->EncodeFrames( src,  srcLen, dst, dstLen, flags));
        writeStream(ulStream,(char*)&msg, sizeof(msg));
        writeStream(ulStream,(char*)&dstLen, sizeof(dstLen));
        writeStream(ulStream,(char*)&dst, dstLen);
        writeStream(ulStream,(char*)&flags, sizeof(flags));
        writeStream(ulStream,(char*)&ret, sizeof(ret));
        flushStream(ulStream);
      break;
    case SET_MAX_FRAME_SIZE:
        readStream(dlStream, (char*)&val, sizeof(val));
        x264->SetMaxRTPFrameSize (val);
        writeStream(ulStream,(char*)&msg, sizeof(msg)); 
        flushStream(ulStream);
      break;
    default:
      break;
    }
  }
  return 0;
}
