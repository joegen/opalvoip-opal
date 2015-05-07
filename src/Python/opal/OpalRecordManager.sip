/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
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
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

%ModuleHeaderCode
#include <ptlib.h>
#include <opal/recording.h>
%End


/** This is an abstract class for recording OPAL calls.
    A factory is used to created concrete classes based on the file extension
    supported by the individual record manager.
  */
class OpalRecordManager /Abstract/
{
  public:
    //typedef PFactory<OpalRecordManager, PCaselessString> Factory;

    enum VideoMode {
      eSideBySideLetterbox, /**< Two images side by side with black bars top and bottom.
                                 It is expected that the input frames and output are all
                                 the same aspect ratio, e.g. 4:3. Works well if inputs
                                 are QCIF and output is CIF for example. */
      eSideBySideScaled,    /**< Two images side by side, scaled to fit halves of output
                                 frame. It is expected that the output frame be double
                                 the width of the input data to maintain aspect ratio.
                                 e.g. for CIF inputs, output would be 704x288. */
      eStackedPillarbox,    /**< Two images, one on top of the other with black bars down
                                 the sides. It is expected that the input frames and output
                                 are all the same aspect ratio, e.g. 4:3. Works well if
                                 inputs are QCIF and output is CIF for example. */
      eStackedScaled,       /**< Two images, one on top of the other, scaled to fit halves
                                 of output frame. It is expected that the output frame be
                                 double the height of the input data to maintain aspect
                                 ratio. e.g. for CIF inputs, output would be 352x576. */
      eSeparateStreams,     ///< Unsupported
      NumVideoMixingModes
    };

    struct Options {
      bool      m_stereo;
      PString   m_audioFormat;

      OpalRecordManager::VideoMode m_videoMixing;
      PString   m_videoFormat;
      unsigned  m_videoWidth;
      unsigned  m_videoHeight;
      unsigned  m_videoRate;

      Options(
        bool         stereo = true,
        OpalRecordManager::VideoMode  videoMixing = OpalRecordManager::eSideBySideLetterbox,
        const char * audioFormat = NULL,
        const char * videoFormat = NULL,
        unsigned width = PVideoFrameInfo::CIFWidth,
        unsigned height = PVideoFrameInfo::CIFHeight,
        unsigned rate = 15
      );
    };

    virtual ~OpalRecordManager();

    /**Open the recording file.
      */
    bool Open(const PFilePath & fn);

    /**Open the recording file indicating the options to be used.
      */
    bool Open(const PFilePath & fn, const Options & options);

    /**Indicate if the recording file is open.
      */
    virtual bool IsOpen() const = 0;

    /**Close the recording file.
       Note this may block until various sub-threads are termianted so
       care may be needed to avoid deadlocks.
      */
    virtual bool Close() = 0;

    /**Open an individual media stream using the provided identifier and format.
      */
    virtual bool OpenStream(
      const PString & strmId,         ///< Identifier for media stream.
      const OpalMediaFormat & format  ///< Media format for new stream
    ) = 0;

    /**Close the media stream based on the identifier provided.
      */
    virtual bool CloseStream(
      const PString & strmId  ///< Identifier for media stream.
    ) = 0;

    /**Write audio to the recording file.
      */
    virtual bool WriteAudio(
      const PString & strmId,     ///< Identifier for media stream.
      const RTP_DataFrame & rtp   ///< RTP data containing PCM-16 data
    ) = 0;

    /**Write video to the recording file.
      */
    virtual bool WriteVideo(
      const PString & strmId,     ///< Identifier for media stream.
      const RTP_DataFrame & rtp   ///< RTP data containing a YUV420P frame
    ) = 0;

    /**Get the options for this recording.
      */
    const Options & GetOptions() const;

    /**Set the options for this recording.
      */
    void SetOptions(const Options & options);

  protected:
    virtual bool OpenFile(const PFilePath & fn) = 0;
};


