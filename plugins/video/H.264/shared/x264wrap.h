/*****************************************************************************/
/* The contents of this file are subject to the Mozilla Public License       */
/* Version 1.0 (the "License"); you may not use this file except in          */
/* compliance with the License.  You may obtain a copy of the License at     */
/* http://www.mozilla.org/MPL/                                               */
/*                                                                           */
/* Software distributed under the License is distributed on an "AS IS"       */
/* basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the  */
/* License for the specific language governing rights and limitations under  */
/* the License.                                                              */
/*                                                                           */
/* The Original Code is the Open H323 Library.                               */
/*                                                                           */
/* The Initial Developer of the Original Code is Matthias Schneider          */
/* Copyright (C) 2007 Matthias Schneider, All Rights Reserved.               */
/*                                                                           */
/* Contributor(s): Matthias Schneider (ma30002000@yahoo.de)                  */
/*                                                                           */
/* Alternatively, the contents of this file may be used under the terms of   */
/* the GNU General Public License Version 2 or later (the "GPL"), in which   */
/* case the provisions of the GPL are applicable instead of those above.  If */
/* you wish to allow use of your version of this file only under the terms   */
/* of the GPL and not to allow others to use your version of this file under */
/* the MPL, indicate your decision by deleting the provisions above and      */
/* replace them with the notice and other provisions required by the GPL.    */
/* If you do not delete the provisions above, a recipient may use your       */
/* version of this file under either the MPL or the GPL.                     */
/*                                                                           */
/* The Original Code was written by Matthias Schneider <ma30002000@yahoo.de> */
/*****************************************************************************/

#ifndef OPAL_H264ENC_H
#define OPAL_H264ENC_H 1

#include "../../common/platform.h"


#if X264_LICENSED || GPL_HELPER_APP
#define _INTTYPES_H_ // ../common/platform.h is equivalent to this
extern "C" {
#include <x264.h>
};
#include "h264frame.h"
#endif


#ifdef WIN32
#include <windows.h>
#define EXECUTABLE_NAME "x264plugin_helper.exe"
#else
#define EXECUTABLE_NAME "h264_video_pwplugin_helper"
#endif


#define INIT                      0
#define H264ENCODERCONTEXT_CREATE 1
#define H264ENCODERCONTEXT_DELETE 2
#define APPLY_OPTIONS             3
#define SET_TARGET_BITRATE        4
#define SET_FRAME_RATE            5
#define SET_FRAME_WIDTH           6
#define SET_FRAME_HEIGHT          7
#define ENCODE_FRAMES             8
#define ENCODE_FRAMES_BUFFERED    9
#define SET_MAX_PAYLOAD_SIZE      10
#define SET_MAX_KEY_FRAME_PERIOD  11
#define SET_TSTO                  12
#define SET_PROFILE_LEVEL         13
#define SET_MAX_NALU_SIZE         14


class H264Encoder
{
  public:
    H264Encoder();
    ~H264Encoder();

    bool Load(void * instance);

    bool SetProfileLevel(unsigned profile, unsigned level, unsigned constraints);
    bool SetFrameWidth(unsigned width);
    bool SetFrameHeight(unsigned height);
    bool SetFrameRate(unsigned rate);
    bool SetTargetBitrate(unsigned rate);
    bool SetMaxRTPPayloadSize(unsigned size);
    bool SetMaxNALUSize(unsigned size);
    bool SetTSTO(unsigned tsto);
    bool SetMaxKeyFramePeriod(unsigned period);

    bool ApplyOptions();

    bool EncodeFrames(
      const unsigned char * src,
      unsigned & srcLen,
      unsigned char * dst,
      unsigned & dstLen,
      unsigned headerLen,
      unsigned int & flags
    );

    unsigned GetWidth() const;
    unsigned GetHeight() const;

  protected:
#if X264_LICENSED || GPL_HELPER_APP

    x264_param_t   m_context;
    x264_t       * m_codec;
    H264Frame      m_encapsulation;

#else // X264_LICENSED || GPL_HELPER_APP

    bool OpenPipeAndExecute(void * instance, const char * executablePath);
    bool ReadPipe(void * ptr, size_t len);
    bool WritePipe(const void * ptr, size_t len);
    bool WriteValue(unsigned msg, unsigned value);

    bool m_loaded;

  #if WIN32
    HANDLE m_hNamedPipe;
  #else // WIN32
    char  m_dlName[100];
    char  m_ulName[100];
    int   m_pipeToProcess;
    int   m_pipeFromProcess;
    pid_t m_pid;
  #endif // WIN32

    bool m_startNewFrame;

#endif // X264_LICENSED || GPL_HELPER_APP
};

#endif /* OPAL_H264_ENC_H__ */
