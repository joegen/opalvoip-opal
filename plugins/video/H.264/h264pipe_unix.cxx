/*
 * H.264 Plugin codec for OpenH323/OPAL
 *
 * Copyright (C) 2007 Matthias Schneider, All Rights Reserved
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
 * The Original Code is Open H323 Library.
 *
 * Contributor(s): Matthias Schneider (ma30002000@yahoo.de)
 *
 */

/*
  Notes
  -----

 */

#include <stdlib.h>
#include <sys/stat.h>
#include "shared/trace.h"
#include "shared/rtpframe.h"
#include "h264pipe_unix.h"
#include "shared/trace.h"

#define HAS_MKFIFO 1
#define GPL_PROCESS_FILENAME "pwlib/codecs/video/h264_video_pwplugin_helper"
#define P_DEFAULT_PLUGIN_DIR "/usr/lib/pwlib"
#define DIR_SEPERATOR "/"
#define DIR_TOKENISER ":"

H264EncCtx::H264EncCtx()
{
  width  = 0;
  height = 0;
  size = 0;
  startNewFrame = true;
  loaded = false;  
}

H264EncCtx::~H264EncCtx()
{
  closeAndRemovePipes();
}

bool 
H264EncCtx::Load()
{
  sprintf (dlName,"/tmp/x264-dl-%d", getpid());
  sprintf (ulName,"/tmp/x264-ul-%d", getpid());
  if (!createPipes()) {
  
    closeAndRemovePipes(); 
    return false;
  }
  
  if (!findGplProcess()) { 

    TRACE(1, "H264\tIPC\tPP: Couldn't find GPL process executable: " << GPL_PROCESS_FILENAME)
    closeAndRemovePipes(); 
    return false;
  }
    // Check if file is executable!!!!
  int pid = fork();

  if(pid == 0) 

    execGplProcess();

  else if(pid < 0) {

    TRACE(1, "H264\tIPC\tPP: Error when trying to for");
    closeAndRemovePipes(); 
    return false;
  }

  dlStream.open(dlName, std::ios::binary);
  if (dlStream.fail()) { 

    TRACE(1, "H264\tIPC\tPP: Error when opening DL named pipe");
    closeAndRemovePipes(); 
    return false;
  }
  ulStream.open(ulName, std::ios::binary);
  if (ulStream.fail()) { 

    TRACE(1, "H264\tIPC\tPP: Error when opening UL named pipe")
    closeAndRemovePipes(); 
    return false;
  }

  TRACE(1, "H264\tIPC\tPP: Successfully forked child process "<<  pid << " and established communication")
  loaded = true;  
  return true;
}

void H264EncCtx::call(unsigned msg)
{
  if (msg == H264ENCODERCONTEXT_CREATE) 
    startNewFrame = true;
  writeStream((char*) &msg, sizeof(msg));
  flushStream();
  readStream((char*) &msg, sizeof(msg));
}

void H264EncCtx::call(unsigned msg, int value)
{
  switch (msg) {
    case SET_FRAME_WIDTH:  width  = value; size = (int) (width * height * 1.5) + sizeof(frameHeader) + 40; break;
    case SET_FRAME_HEIGHT: height = value; size = (int) (width * height * 1.5) + sizeof(frameHeader) + 40; break;
   }
  
  writeStream((char*) &msg, sizeof(msg));
  writeStream((char*) &value, sizeof(value));
  flushStream();
  readStream((char*) &msg, sizeof(msg));
}
     
void H264EncCtx::call(unsigned msg , const u_char * src, unsigned & srcLen, u_char * dst, unsigned & dstLen, unsigned & headerLen, unsigned int & flags, int & ret)
{
  if (startNewFrame) {

    writeStream((char*) &msg, sizeof(msg));
    if (size) {
      writeStream((char*) &size, sizeof(size));
      writeStream((char*) src, size);
      writeStream((char*) &headerLen, sizeof(headerLen));
      writeStream((char*) dst, headerLen);

    }
    else {
      writeStream((char*) &srcLen, sizeof(srcLen));
      writeStream((char*) src, srcLen);
      writeStream((char*) &headerLen, sizeof(headerLen));
      writeStream((char*) dst, headerLen);
    }
  }
  else {
  
    msg = ENCODE_FRAMES_BUFFERED;
    writeStream((char*) &msg, sizeof(msg));
  }
  
  flushStream();
  
  readStream((char*) &msg, sizeof(msg));
  readStream((char*) &dstLen, sizeof(dstLen));
  readStream((char*) dst, dstLen);
  readStream((char*) &flags, sizeof(flags));
  readStream((char*) &ret, sizeof(ret));

  if (flags & 1) 
    startNewFrame = true;
   else
    startNewFrame = false;
}

bool H264EncCtx::createPipes()
{
  umask(0);
#ifdef HAS_MKFIFO
  if (mkfifo((const char*) &dlName, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) {

    TRACE(1, "H264\tIPC\tPP: Error when trying to create DL named pipe");
    return false;
  }
  if (mkfifo((const char*) &ulName, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) {

    TRACE(1, "H264\tIPC\tPP: Error when trying to create UL named pipe");
    return false;
  }
#else
  if (mknod((const char*) &dlName, S_IFIFO | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, 0)) {

    TRACE(1, "H264\tIPC\tPP: Error when trying to create named pipe");
    return false;
  }
  if (mknod((const char*) &ulName, S_IFIFO | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, 0)) {

    TRACE(1, "H264\tIPC\tPP: Error when trying to create named pipe");
    return false;
  }
#endif
  return true;
}

void H264EncCtx::closeAndRemovePipes()
{
  dlStream.close();
  if (dlStream.fail()) { TRACE(1, "H264\tIPC\tPP: Error when closing DL named pipe\n"); }
  ulStream.close();
  if (ulStream.fail()) { TRACE(1, "H264\tIPC\tPP: Error when closing UL named pipe\n"); }
  if (std::remove((const char*) &ulName) == -1) printf ("Error when trying to remove named pipe\n");
  if (std::remove((const char*) &dlName) == -1) printf ("Error when trying to remove named pipe\n");
}

void H264EncCtx::readStream (char* data, unsigned bytes)
{
  ulStream.read(data, bytes);
  if (ulStream.fail()) { TRACE(1, "H264\tIPC\tPP: Failure on reading - terminating\n"); closeAndRemovePipes();      }
  if (ulStream.bad())  { TRACE(1, "H264\tIPC\tPP: Bad flag set on reading - terminating\n"); closeAndRemovePipes(); }
  if (ulStream.eof())  { TRACE(1, "H264\tIPC\tPP: Received EOF - terminating\n"); closeAndRemovePipes();            }
}

void H264EncCtx::writeStream (const char* data, unsigned bytes)
{
  dlStream.write(data, bytes);
  if (dlStream.bad())  { TRACE(1, "H264\tIPC\tPP: Bad flag set on writing - terminating\n"); closeAndRemovePipes(); }
}

void H264EncCtx::flushStream ()
{
  dlStream.flush();
  if (dlStream.bad())  { TRACE(1, "H264\tIPC\tPP: Bad flag set on flushing - terminating\n"); closeAndRemovePipes(); }
}

bool H264EncCtx::findGplProcess()
{
  char * env = ::getenv("PWLIBPLUGINDIR");
  if (env != NULL) {
    const char * token = strtok(env, DIR_TOKENISER);
    while (token != NULL) {

      if (checkGplProcessExists(token)) 
        return true;

      token = strtok(NULL, DIR_TOKENISER);
    }
  }
  return checkGplProcessExists(".");
}

bool H264EncCtx::checkGplProcessExists (const char * dir)
{
  struct stat buffer;
  memset(gplProcess, 0, sizeof(gplProcess));
  strncpy(gplProcess, dir, sizeof(gplProcess));
  if (gplProcess[strlen(gplProcess)-1] != DIR_SEPERATOR[0]) 
    strcat(gplProcess, DIR_SEPERATOR);
  strcat(gplProcess, GPL_PROCESS_FILENAME);

  if (stat(gplProcess, &buffer ) ) { 

    TRACE(1, "H264\tIPC\tPP: Couldn't find GPL process executable in " << gplProcess);
    return false;
  }

  TRACE(1, "H264\tIPC\tPP: Found GPL process executable in  " << gplProcess);

  return true;
}

void H264EncCtx::execGplProcess() 
{
  if (execl(gplProcess,dlName,ulName, NULL) == -1) {
    TRACE(1, "H264\tIPC\tPP: Error when trying to execute GPL process  " << gplProcess);
    exit(1);
  }
}
