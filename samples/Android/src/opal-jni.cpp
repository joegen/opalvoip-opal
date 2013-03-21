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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <jni.h>
#include <ptlib.h>
#include <ptlib/sound.h>
#include <ptclib/dtmf.h>


// Include symbol hook so links in all the SWIG interface.
extern int opal_java_swig_wrapper_link;
static int const * const force_java_swig_wrapper_link = &opal_java_swig_wrapper_link;


extern "C" {

  jboolean Java_org_opalvoip_opal_andsample_AndOPAL_TestAudio(JNIEnv* env, jclass clazz)
  {
    PTRACE(1, "AndOPAL", "Testing audio output");
    std::auto_ptr<PSoundChannel> player(PSoundChannel::CreateOpenedChannel("*", "voice", PSoundChannel::Player));
    if (player.get() == NULL)
      return false;

    player->SetBuffers(400, 8);
    {
      PTones tones("C:0.2/D:0.2/E:0.2/F:0.2/G:0.2/A:0.2/B:0.2/C5:1.0");
      PTRACE(1, "AndOPAL", "Tones using " << tones.GetSize() << " samples, "
              << PTimeInterval(1000*tones.GetSize()/tones.GetSampleRate()) << " seconds");
      PTime then;
      if (!tones.Write(*player))
        return false;

      PTRACE(1, "AndOPAL", "Audio queued");
      player->WaitForPlayCompletion();
      PTRACE(1, "AndOPAL", "Finished tone output: " << PTime() - then << " seconds");
    }

    std::auto_ptr<PSoundChannel> recorder(PSoundChannel::CreateOpenedChannel("*", "*", PSoundChannel::Recorder));
    if (player.get() == NULL)
      return false;

    PShortArray recording(5*recorder->GetSampleRate()); // Five seconds

    {
      PTRACE(1, "AndOPAL", "Started recording");
      PTime then;
      if (!recorder->ReadBlock(recording.GetPointer(), recording.GetSize()*sizeof(short)))
        return false;
      PTRACE(1, "AndOPAL", "Finished recording " << PTime() - then << " seconds");
    }

    {
      PTRACE(1, "AndOPAL", "Started play back");
      PTime then;
      if (!player->Write(recording.GetPointer(), recording.GetSize()*sizeof(short)))
        return false;
      PTRACE(1, "AndOPAL", "Finished play back " << PTime() - then << " seconds");
    }
  }

};

// End of file
