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
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include "shared/rtpframe.h"
#include "h264pipe_unix.h"
#include <string.h>
#include <codec/opalplugin.hpp>

#define HAVE_MKFIFO 1
#define GPL_PROCESS_FILENAME "h264_video_pwplugin_helper"
#define DIR_TOKENISER ":"


H264EncCtx::H264EncCtx()
{
  startNewFrame = true;
  loaded = false;  
  pipesCreated = false;
  m_pipeToProcess = m_pipeFromProcess = -1;
  m_pid = -1;
}

H264EncCtx::~H264EncCtx()
{
  closeAndRemovePipes();
}

bool H264EncCtx::Load(void * instance)
{
  if (loaded)
    return true;

  snprintf ( dlName, sizeof(dlName), "/tmp/x264-dl-%d-%p", getpid(), instance);
  snprintf ( ulName, sizeof(ulName), "/tmp/x264-ul-%d-%p", getpid(), instance);
  if (!createPipes()) {
    closeAndRemovePipes(); 
    return false;
  }
  pipesCreated = true;  

  if (!findGplProcess()) { 
    PTRACE(1, "x264", "Couldn't find GPL process executable: " << GPL_PROCESS_FILENAME);
    closeAndRemovePipes(); 
    return false;
  }
    // Check if file is executable!!!!
  m_pid = fork();
  if (m_pid < 0) {
    PTRACE(1, "x264", "Error when trying to fork");
    closeAndRemovePipes(); 
    return false;
  }

  if (m_pid == 0) {
    execGplProcess(); // Doesn't return
    return false;
  }

  m_pipeToProcess = open(dlName, O_WRONLY);
  if (m_pipeToProcess < 0) { 
    PTRACE(1, "x264", "Error when opening DL named pipe - " << strerror(errno));
    closeAndRemovePipes(); 
    return false;
  }

  m_pipeFromProcess = open(ulName, O_RDONLY);
  if (m_pipeFromProcess < 0) { 
    PTRACE(1, "x264", "Error when opening UL named pipe - " << strerror(errno));
    closeAndRemovePipes(); 
    return false;
  }
  
  unsigned msg = INIT;
  unsigned status = 0;
  if (writePipe(&msg, sizeof(msg)) &&
      readPipe(&msg, sizeof(msg)) &&
      readPipe(&status, sizeof(status)) &&
      status != 0) {
    PTRACE(1, "x264", "Successfully forked child process "<< m_pid << " and established communication");
    loaded = true;  
    return true;
  }

  PTRACE(1, "x264", "GPL Process returned failure on initialization - plugin disabled");
  closeAndRemovePipes(); 
  return false;
}

bool H264EncCtx::call(unsigned msg)
{
  if (msg == H264ENCODERCONTEXT_CREATE) 
    startNewFrame = true;
  return writePipe(&msg, sizeof(msg)) && readPipe(&msg, sizeof(msg));
}

bool H264EncCtx::call(unsigned msg, unsigned value)
{
  return writePipe(&msg, sizeof(msg)) && writePipe(&value, sizeof(value)) && readPipe(&msg, sizeof(msg));
}
     
bool H264EncCtx::call(unsigned msg , const u_char * src, unsigned & srcLen, u_char * dst, unsigned & dstLen, unsigned & headerLen, unsigned int & flags, int & ret)
{
  if (startNewFrame) {
    if (!writePipe(&msg, sizeof(msg)) ||
        !writePipe(&srcLen, sizeof(srcLen)) ||
        !writePipe(src, srcLen) ||
        !writePipe(&headerLen, sizeof(headerLen)) ||
        !writePipe(dst, headerLen) ||
        !writePipe(&flags, sizeof(flags)))
      return false;
  }
  else {
    msg = ENCODE_FRAMES_BUFFERED;
    if (!writePipe(&msg, sizeof(msg)))
      return false;
  }
  
  if (!readPipe(&msg, sizeof(msg)) ||
      !readPipe(&dstLen, sizeof(dstLen)) ||
      !readPipe(dst, dstLen) ||
      !readPipe(&flags, sizeof(flags)) ||
      !readPipe(&ret, sizeof(ret)))
    return false;

  startNewFrame = (flags & 1) != 0;
  return true;
}

bool H264EncCtx::createPipes()
{
  umask(0);
#ifdef HAVE_MKFIFO
  if (mkfifo((const char*) &dlName, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) {
    PTRACE(1, "x264", "Error when trying to create DL named pipe");
    return false;
  }
  if (mkfifo((const char*) &ulName, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) {
    PTRACE(1, "x264", "Error when trying to create UL named pipe");
    return false;
  }
#else
  if (mknod((const char*) &dlName, S_IFIFO | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, 0)) {
    PTRACE(1, "x264", "Error when trying to create named pipe");
    return false;
  }
  if (mknod((const char*) &ulName, S_IFIFO | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, 0)) {
    PTRACE(1, "x264", "Error when trying to create named pipe");
    return false;
  }
#endif /* HAVE_MKFIFO */
  return true;
}

void H264EncCtx::closeAndRemovePipes()
{
  if (m_pipeToProcess >= 0) {
    close(m_pipeToProcess);
    m_pipeToProcess = -1;
  }

  if (m_pipeFromProcess >= 0) {
    close(m_pipeFromProcess);
    m_pipeFromProcess = -1;
  }

  if (pipesCreated) {
    if (std::remove((const char*) &ulName) == -1)
      PTRACE(1, "x264", "Error when trying to remove UL named pipe - " << strerror(errno));
    if (std::remove((const char*) &dlName) == -1)
      PTRACE(1, "x264", "Error when trying to remove DL named pipe - " << strerror(errno));
    pipesCreated = false;
  }
}

bool H264EncCtx::readPipe(void* data, unsigned bytes)
{
  int result = read(m_pipeFromProcess, data, bytes);
  if (result == bytes)
    return true;

  PTRACE(1, "x264", "Error reading pipe (" << result << ") - " << strerror(errno));
  if (kill(m_pid, 0) < 0)
    PTRACE(1, "x264", "Sub-process no longer running!");
  return false;
}

bool H264EncCtx::writePipe(const void * data, unsigned bytes)
{
  int result = write(m_pipeToProcess, data, bytes);
  if (result == bytes)
    return true;

  PTRACE(1, "x264", "Error writing pipe (" << result << ") - " << strerror(errno));
  if (kill(m_pid, 0) < 0)
    PTRACE(1, "x264", "Sub-process no longer running!");
  return false;
}

bool H264EncCtx::findGplProcess()
{
  char * env = ::getenv("PWLIBPLUGINDIR");
  if (env == NULL)
    env = ::getenv("PTLIBPLUGINDIR");
  if (env != NULL) {
    const char * token = strtok(env, DIR_TOKENISER);
    while (token != NULL) {

      if (checkGplProcessExists(token)) 
        return true;

      token = strtok(NULL, DIR_TOKENISER);
    }
  }

  return checkGplProcessExists(".") ||
#ifdef LIB_DIR
         checkGplProcessExists(LIB_DIR) ||
#endif
         checkGplProcessExists("/usr/lib") ||
         checkGplProcessExists("/usr/local/lib");
}

bool H264EncCtx::checkGplProcessExists (const char * dir)
{
  memset(gplProcess, 0, sizeof(gplProcess));

  size_t dirlen = strlen(dir);
  if (dirlen > sizeof(gplProcess)-strlen(VC_PLUGIN_DIR)-strlen(GPL_PROCESS_FILENAME)) {
    PTRACE(1, "x264", "Directory too long");
    return false;
  }

  strcpy(gplProcess, dir);
  if (gplProcess[dirlen-1] != '/')
    gplProcess[dirlen++] = '/';
  strcat(gplProcess, GPL_PROCESS_FILENAME);

  if (access(gplProcess, R_OK|X_OK) < 0) {
    strcpy(&gplProcess[dirlen], VC_PLUGIN_DIR);
    strcat(gplProcess, "/");
    strcat(gplProcess, GPL_PROCESS_FILENAME);
    if (access(gplProcess, R_OK|X_OK) ) { 
      PTRACE(4, "x264", "Couldn't find GPL process executable in " << gplProcess);
      return false;
    }
  }

  PTRACE(4, "x264", "Found GPL process executable in  " << gplProcess);
  return true;
}

void H264EncCtx::execGplProcess() 
{
  unsigned msg;
  unsigned status = 0;
  if (execl(gplProcess,"h264_video_pwplugin_helper", dlName,ulName, NULL) == -1) {

    PTRACE(1, "x264", "Error when trying to execute GPL process  " << gplProcess << " - " << strerror(errno));
    cpDLStream.open(dlName, std::ios::binary);
    if (cpDLStream.fail()) { PTRACE(1, "x264", "IPC\tCP: Error when opening DL named pipe"); exit (1); }
    cpULStream.open(ulName,std::ios::binary);
    if (cpULStream.fail()) { PTRACE(1, "x264", "IPC\tCP: Error when opening UL named pipe"); exit (1); }

    cpDLStream.read((char*)&msg, sizeof(msg));
    if (cpDLStream.fail()) { PTRACE(1, "x264", "IPC\tCP: Failure on reading - terminating");       cpCloseAndExit(); }
    if (cpDLStream.bad())  { PTRACE(1, "x264", "IPC\tCP: Bad flag set on reading - terminating");  cpCloseAndExit(); }
    if (cpDLStream.eof())  { PTRACE(1, "x264", "IPC\tCP: Received EOF - terminating"); exit (1);   cpCloseAndExit(); }

    cpULStream.write((char*)&msg, sizeof(msg));
    if (cpULStream.bad())  { PTRACE(1, "x264", "IPC\tCP: Bad flag set on writing - terminating");  cpCloseAndExit(); }

    cpULStream.write((char*)&status, sizeof(status));
    if (cpULStream.bad())  { PTRACE(1, "x264", "IPC\tCP: Bad flag set on writing - terminating");  cpCloseAndExit(); }

    cpULStream.flush();
    if (cpULStream.bad())  { PTRACE(1, "x264", "IPC\tCP: Bad flag set on flushing - terminating"); }
    cpCloseAndExit();
  }
}

void H264EncCtx::cpCloseAndExit()
{
  cpDLStream.close();
  if (cpDLStream.fail()) { PTRACE(1, "x264", "IPC\tCP: Error when closing DL named pipe"); }
  cpULStream.close();
  if (cpULStream.fail()) { PTRACE(1, "x264", "IPC\tCP: Error when closing UL named pipe"); }
  exit(1);
}
