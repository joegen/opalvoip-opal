/*
 * vpblid.h
 *
 * Voicetronix VPB4 line interface device
 *
 * Copyright (c) 1999-2001 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: vpblid.h,v $
 * Revision 1.2001  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.8  2001/02/09 05:16:24  robertj
 * Added #pragma interface for GNU C++.
 *
 * Revision 1.7  2001/01/25 07:27:14  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 * Revision 1.6  2001/01/24 05:34:49  robertj
 * Altered volume control range to be percentage, ie 100 is max volume.
 *
 * Revision 1.5  2000/11/24 10:50:52  robertj
 * Modified the ReadFrame/WriteFrame functions to allow for variable length codecs.
 *
 * Revision 1.4  2000/11/20 04:35:40  robertj
 * Changed tone detection API slightly to allow detection of multiple
 * simultaneous tones
 *
 * Revision 1.3  2000/05/02 04:32:25  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.2  2000/01/07 08:28:09  robertj
 * Additions and changes to line interface device base class.
 *
 * Revision 1.1  1999/12/23 23:02:35  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 */

#ifndef __LIDS_VPBLID_H
#define __LIDS_VPBLID_H

#ifdef __GNUC__
#pragma interface
#endif


#include <lids/lid.h>


///////////////////////////////////////////////////////////////////////////////

/**This class describes the xJack line interface device.
 */
class OpalVpbDevice : public OpalLineInterfaceDevice
{
  PCLASSINFO(OpalVpbDevice, OpalLineInterfaceDevice);

  public:
    /**Create a new, closed, device for a VPB card.
      */
    OpalVpbDevice();

    /**Destroy line interface device.
       This calls Close() on the device.
      */
    ~OpalVpbDevice() { Close(); }

    /**Open the xJack device.
      */
    virtual BOOL Open(
      const PString & device      /// Device identifier name.
    );

    /**Close the xJack device.
      */
    virtual BOOL Close();

    /**Get the device name.
      */
    virtual PString GetName() const;

    /**Get the total number of lines supported by this device.
      */
    virtual unsigned GetLineCount();


    /**Determine if line is currently off hook.
       This returns TRUE if GetLineState() is a state that implies the line is
       off hook (eg OffHook or LineBusy).
      */
    virtual BOOL IsLineOffHook(
      unsigned line   /// Number of line
    );

    /**Set the state of the line.
       Note that not be possible on a given line.
      */
    virtual BOOL SetLineOffHook(
      unsigned line,        /// Number of line
      BOOL newState = TRUE  /// New state to set
    );


    /**Determine if line is ringing.
      */
    virtual BOOL IsLineRinging(
      unsigned line,          /// Number of line
      DWORD * cadence = NULL  /// Cadence of incoming ring
    );


    /**Get the media formats this device is capable of using.
      */
    virtual OpalMediaFormat::List GetMediaFormats() const;

    /**Set the codec for reading.
      */
    virtual BOOL SetReadFormat(
      unsigned line,    /// Number of line
      const OpalMediaFormat & mediaFormat   /// Codec type
    );

    /**Set the codec for writing.
      */
    virtual BOOL SetWriteFormat(
      unsigned line,    /// Number of line
      const OpalMediaFormat & mediaFormat   /// Codec type
    );

    /**Get the media format (codec) for reading on the specified line.
      */
    virtual OpalMediaFormat GetReadFormat(
      unsigned line    /// Number of line
    );

    /**Get the media format (codec) for writing on the specified line.
      */
    virtual OpalMediaFormat GetWriteFormat(
      unsigned line    /// Number of line
    );

    /**Stop the read codec.
      */
    virtual BOOL StopReadCodec(
      unsigned line   /// Number of line
    );

    /**Stop the write codec.
      */
    virtual BOOL StopWriteCodec(
      unsigned line   /// Number of line
    );

    /**Set the read frame size in bytes.
       Note that a LID may ignore this value so always use GetReadFrameSize()
       for I/O.
      */
    virtual BOOL SetReadFrameSize(
      unsigned line,    /// Number of line
      PINDEX frameSize  /// New frame size
    );

    /**Set the write frame size in bytes.
       Note that a LID may ignore this value so always use GetReadFrameSize()
       for I/O.
      */
    virtual PINDEX SetWriteFrameSize(
      unsigned line,    /// Number of line
      PINDEX frameSize  /// New frame size
    );

    /**Get the read frame size in bytes.
       All calls to ReadFrame() will return this number of bytes.
      */
    virtual PINDEX GetReadFrameSize(
      unsigned line   /// Number of line
    );

    /**Get the write frame size in bytes.
       All calls to WriteFrame() must be this number of bytes.
      */
    virtual PINDEX GetWriteFrameSize(
      unsigned line   /// Number of line
    );

    /**Low level read of a frame from the device.
     */
    virtual BOOL ReadFrame(
      unsigned line,    /// Number of line
      void * buf,   /// Pointer to a block of memory to receive the read bytes.
      PINDEX & count    /// Number of bytes read, <= GetReadFrameSize()
    );

    /**Low level write frame to the device.
     */
    virtual BOOL WriteFrame(
      unsigned line,    /// Number of line
      const void * buf, /// Pointer to a block of memory to write.
      PINDEX count,     /// Number of bytes to write, <= GetWriteFrameSize()
      PINDEX & written  /// Number of bytes written, <= GetWriteFrameSize()
    );


    /**Set volume level for recording.
       A value of 100 is the maximum volume possible for the hardware.
       A value of 0 is the minimum volume possible for the hardware.
      */
    virtual BOOL SetRecordVolume(
      unsigned line,    /// Number of line
      unsigned volume   /// Volume level from 0 to 100%
    );

    /**Set volume level for playing.
       A value of 100 is the maximum volume possible for the hardware.
       A value of 0 is the minimum volume possible for the hardware.
      */
    virtual BOOL SetPlayVolume(
      unsigned line,    /// Number of line
      unsigned volume   /// Volume level from 0 to 100%
    );


    /**Read a DTMF digit detected.
       This may be characters from the set 0-9, A-D, * or #. A null ('\0')
       character indicates that there are no tones in the queue.

      */
    virtual char ReadDTMF(
      unsigned line   /// Number of line
    );

    /**Play a DTMF digit.
       Any characters that are not in the set 0-9, A-D, * or # will be ignored.
      */
    virtual BOOL PlayDTMF(
      unsigned line,            /// Number of line
      const char * digits,      /// DTMF digits to be played
      DWORD onTime = 90,        /// Number of milliseconds to play each DTMF digit
      DWORD offTime = 30        /// Number of milliseconds between digits
    );


    /**See if a tone is detected.
      */
    virtual unsigned IsToneDetected(
      unsigned line   /// Number of line
    );


  protected:
    unsigned cardNumber;
    unsigned lineCount;

    enum { MaxLineCount = 8 };

    struct LineState {
      BOOL Open(unsigned cardNumber, unsigned lineNumber);
      BOOL SetLineOffHook(BOOL newState);
      BOOL IsLineRinging(DWORD *);

      int    handle;
      BOOL   currentHookState;
      PINDEX readFormat,    writeFormat;
      PINDEX readFrameSize, writeFrameSize;
      BOOL   readIdle,      writeIdle;
      PTimer ringTimeout;
    } lineState[MaxLineCount];
};


#endif // __LIDS_VPBLID_H


/////////////////////////////////////////////////////////////////////////////
