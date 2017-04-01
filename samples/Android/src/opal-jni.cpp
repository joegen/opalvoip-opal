/*
 * opal-jni.h
 *
 * Copyright (c) 2013 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida (Robert Jongbloed)
 *
 * Contributor(s): ______________________________________.
 *
 */

#include <jni.h>
#include <ptlib.h>
#include <ptlib/sound.h>
#include <ptclib/dtmf.h>


// Include symbol hook so links in all the SWIG interface.
extern int opal_java_swig_wrapper_link;
static int const * const force_java_swig_wrapper_link = &opal_java_swig_wrapper_link;

const unsigned FrameTime = 20; // Milliseconds

extern "C" {

  jstring Java_org_opalvoip_opal_andsample_AndOPAL_TestPlayer(JNIEnv* env, jclass clazz, jint bufferTime)
  {
    PSoundChannel::Params playerParams(PSoundChannel::Player, "voice");
    playerParams.m_bufferSize = FrameTime*2*8; // 20ms
    playerParams.m_bufferCount = (bufferTime+FrameTime-1)/FrameTime;
    PString result = PSoundChannel::TestPlayer(playerParams);
    return env->NewStringUTF(result);
  }

  jstring Java_org_opalvoip_opal_andsample_AndOPAL_TestRecorder(JNIEnv* env, jclass clazz, jint bufferTime)
  {
    PSoundChannel::Params recorderParams(PSoundChannel::Recorder, "*");
    PSoundChannel::Params playerParams(PSoundChannel::Player, "voice");
    playerParams.m_bufferSize = recorderParams.m_bufferSize = FrameTime*2*8; // 20ms
    playerParams.m_bufferCount = recorderParams.m_bufferCount = (bufferTime+FrameTime-1)/FrameTime;
    PString result = PSoundChannel::TestRecorder(recorderParams, playerParams);
    return env->NewStringUTF(result);
  }

};

// End of file
