/*
 * main.h
 *
 * OPAL application source file for playing RTP from a PCAP file
 *
 * Copyright (c) 2007 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 */

#ifndef _PlayRTP_MAIN_H
#define _PlayRTP_MAIN_H

#include <ptclib/pvidfile.h>


class OpalPCAPFile;


class PlayRTP : public PProcess
{
  PCLASSINFO(PlayRTP, PProcess)

  public:
    PlayRTP();
    ~PlayRTP();

    virtual void Main();
    void Play(OpalPCAPFile & pcap);

    PDECLARE_NOTIFIER(OpalMediaCommand, PlayRTP, OnTranscoderCommand);

    bool m_singleStep;
    int  m_info;
    bool m_extendedInfo;
    unsigned m_rotateExtensionId;
    bool m_noDelay;
    PTimeInterval m_jitterLimit;

    PFile     m_payloadFile;
    PTextFile m_eventLog;
    PFilePath m_encodedFileName;

    unsigned m_packetCount;

    OpalTranscoder     * m_transcoder;
    PSoundChannel      * m_player;

#if OPAL_VIDEO
    PYUVFile  m_yuvFile;
    PString   m_extraText;
    int       m_extraHeight;

    PVideoOutputDevice * m_display;

    bool     m_vfu;
    bool     m_videoError;
    unsigned m_videoFrames;
#endif
};


#endif  // _PlayRTP_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
