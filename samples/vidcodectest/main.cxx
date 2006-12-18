#include <ptlib.h>

#include <ptclib/pvfiledev.h>
#include <opal/transcoders.h>
#include <codec/vidcodec.h>

#include "intel.h"

#define MAJOR_VERSION 1
#define MINOR_VERSION 0
#define BUILD_TYPE    ReleaseCode
#define BUILD_NUMBER  0

#define RAW_VIDEO_FORMAT       OpalYUV420P

class VidCodecTest : public PProcess
{
  PCLASSINFO(VidCodecTest, PProcess)

  public:
    VidCodecTest();
    void Main();
};

namespace PWLibStupidLinkerHacks {
extern int opalLoader;
};

extern int intelLoader;


PCREATE_PROCESS(VidCodecTest);

VidCodecTest::VidCodecTest()
  : PProcess("Post Increment", "VidCodecTest",
             MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{
  PWLibStupidLinkerHacks::opalLoader = 1;
  intelLoader = 1;
}

void ListCodecs()
{
  OpalTranscoderList keys = OpalTranscoderFactory::GetKeyList();
  OpalTranscoderList::const_iterator r;
  for (r = keys.begin(); r != keys.end(); ++r) {
    const OpalMediaFormatPair & transcoder = *r;
    if (transcoder.GetInputFormat().GetDefaultSessionID() == OpalMediaFormat::DefaultVideoSessionID) {
      cout << "Name: " << transcoder << "\n"
           << "  Input:  " << transcoder.GetInputFormat() << "\n"
           << "  Output: " << transcoder.GetOutputFormat() << "\n";
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void VidCodecTest::Main()
{
  cout << GetName()
       << " Version " << GetVersion(TRUE)
       << " by " << GetManufacturer()
       << " on " << GetOSClass() << ' ' << GetOSName()
       << " (" << GetOSVersion() << '-' << GetOSHardware() << ")\n\n";

  PConfigArgs args(GetArguments());

  args.Parse(
#if PTRACING
             "o-output:"
             "t-trace."
#endif
             "h-help."
          , FALSE);

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL);
#endif

  if ((args.GetCount() == 1) && (args[0] *= "list")) {
    ListCodecs();
    return;
  }

  if (args.GetCount() < 4) {
    PError << "usage: h263test 'encode'|'decode'|'xcode' codec infilename outfilename" << endl;
    return;
  }

  char coding = tolower(args[0][0]);

  PString inCodec, outCodec;
  OpalTranscoder * encoder = NULL;
  OpalTranscoder * decoder = NULL;

  BOOL error = FALSE;
  if (coding == 'd' || coding == 'x') {
    inCodec  = args[1]; 
    //inCodec = "H.264-QCIF";
    outCodec = RAW_VIDEO_FORMAT;
    decoder = OpalTranscoder::Create(inCodec, outCodec);
    if (decoder == NULL) {
      PError << "error: unable to create decoder of " << inCodec << endl;
      error = TRUE;
    }
    else {
      cout << "Created decoder from " << inCodec << " to " << outCodec << endl;
    }
  }
  if (coding == 'e' || coding == 'x') {
    inCodec = RAW_VIDEO_FORMAT;
    outCodec  = args[1];
    encoder = OpalTranscoder::Create(inCodec, outCodec);
    if (encoder == NULL) {
      PError << "error: unable to create encoder of " << outCodec << endl;
      error = TRUE;
    }
    else
      cout << "Created encoder from " << inCodec << " to " << outCodec << endl;
  }
  
  if (error) {
    PError << "Valid transcoders are:";
    ListCodecs();
    return;
  }

  PYUVFile yuvIn;
  PYUVFile yuvOut;
  if (coding == 'e' || coding == 'x') {
    if (!yuvIn.Open(args[2], PFile::ReadOnly, PFile::MustExist)) {
      PError << "error: cannot open YUV input file " << args[2] << endl;
      return;
    }
  }
  if (coding == 'd' || coding == 'x') {
    if (!yuvOut.Open(args[3], PFile::WriteOnly)) {
      PError << "error: cannot open YUV output file " << args[3] << endl;
      return;
    }
  }

  if (coding == 'd') {
    PError << "error: decoding not yet implemented" << endl;
    return;
  }
  if (coding == 'e') {
    PError << "error: encoding not yet implemented" << endl;
    return;
  }
  if (coding == 'x') {

    PINDEX frameCount = 0;
    WORD sequence = 1;

    for (;;) {

      RTP_DataFrame yuvInFrame(sizeof(PluginCodec_Video_FrameHeader) + (yuvIn.GetWidth() * yuvOut.GetHeight() * 3) / 2);
      RTP_DataFrameList encodedFrames, yuvOutFrame;

      PluginCodec_Video_FrameHeader * header = (PluginCodec_Video_FrameHeader *)yuvInFrame.GetPayloadPtr();

      header->x      = 0;
      header->y      = 0;
      header->width  = yuvIn.GetWidth();
      header->height = yuvIn.GetHeight();

      if (!yuvIn.ReadFrame(OPAL_VIDEO_FRAME_DATA_PTR(header))) {
        break;
      }

      encodedFrames.Append(new RTP_DataFrame(100000));
      if (!encoder->ConvertFrames(yuvInFrame, encodedFrames)) {
        PError << "error: encoder returned error" << endl;
        break;
      }

      yuvOutFrame.RemoveAll();
      PINDEX i;
      for (i = 0; i < encodedFrames.GetSize(); ++i) {
        if (!decoder->ConvertFrames(encodedFrames[i], yuvOutFrame)) {
          PError << "error: decoder returned error" << endl;
          break;
        }
        if (yuvOutFrame.GetSize() > 0)
          break;
      }
      if (i != encodedFrames.GetSize()-1) {
        PError << "warning: frame created from incomplete input frame list" << endl;
      }

      PluginCodec_Video_FrameHeader * headerOut = (PluginCodec_Video_FrameHeader *)yuvOutFrame[0].GetPayloadPtr();

      if (!yuvOut.WriteFrame(OPAL_VIDEO_FRAME_DATA_PTR(headerOut))) {
        PError << "error: output file write failed" << endl;
        break;
      }

      ++frameCount;
    }

    cout << frameCount << " frames transcoded" << endl;
  }
}

