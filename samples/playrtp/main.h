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
 * $Revision$
 * $Author$
 * $Date$
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
    bool m_noDelay;
    bool m_writeEventLog;

    PFile     m_payloadFile;
    PTextFile m_eventLog;
    PYUVFile  m_yuvFile;
    PFilePath m_encodedFileName;
    PString   m_extraText;
    int       m_extraHeight;

    OpalTranscoder     * m_transcoder;
    PSoundChannel      * m_player;
    PVideoOutputDevice * m_display;

    unsigned m_packetCount;

    bool     m_vfu;
    bool     m_videoError;
    unsigned m_videoFrames;
};


#endif  // _PlayRTP_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
