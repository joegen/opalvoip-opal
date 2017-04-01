/*
 * main.cxx
 *
 * OPAL application source file for SDP
 *
 * Copyright (c) 2014 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 */

#include <opal/manager.h>
#include <sdp/sdpep.h>

class Test : public PProcess
{
    PCLASSINFO(Test, PProcess)
  public:
    Test();

    virtual void Main();
};


PCREATE_PROCESS(Test);


Test::Test()
  : PProcess("Open Phone Abstraction Library", "SDP Test", OPAL_MAJOR, OPAL_MINOR, ReleaseCode, OPAL_BUILD)
{
}


void Test::Main()
{
  PArgList & args = GetArguments();
  args.Parse("[Options:]"
             "f-file: Parse SDP from file and output from encoded\n"
             "v-verbose. Indicate verbose output.\n"
             PTRACE_ARGLIST
             "h-help."
             , false);
  if (!args.IsParsed()|| args.HasOption('h')) {
    args.Usage(cerr, "[ options ]");
    return;
  }

  PTRACE_INITIALISE(args);

  if (args.HasOption('f')) {
    PTextFile file;
    if (!file.Open(args.GetOptionString('f'), PFile::ReadOnly)) {
      cerr << "Could not open " << file.GetFilePath() << endl;
      return;
    }

    SDPSessionDescription sdp(0, 0, OpalTransportAddress());
    file >> sdp;
    cout << sdp;

    if (args.HasOption('v')) {
      cout << "\n\n";

      for (PINDEX s = 1; s <= sdp.GetMediaDescriptions().GetSize(); ++s) {
        SDPMediaDescription * md = sdp.GetMediaDescriptionByIndex(s);
        cout << "Media " << s << ' ' << md->GetMediaType() << ' ' << md->GetSDPTransportType() << '\n';
        SDPRTPAVPMediaDescription * avp = dynamic_cast<SDPRTPAVPMediaDescription *>(md);
        if (avp != NULL) {
          const SDPRTPAVPMediaDescription::SsrcInfo & ssrc = avp->GetSsrcInfo();
          for (SDPRTPAVPMediaDescription::SsrcInfo::const_iterator it = ssrc.begin(); it != ssrc.end(); ++it)
            cout << right << setw(12) << it->first << " cname=\"" << it->second("cname") << "\"\n";
        }
      }

      cout << "\n\n";
      SDPSessionDescription::MediaStreamMap ms;
      if (sdp.GetMediaStreams(ms)) {
        cout << "Media streams:\n";
        for (SDPSessionDescription::MediaStreamMap::iterator itms = ms.begin(); itms != ms.end(); ++itms) {
          cout << "id=" << itms->first << '\n';
          for (SDPSessionDescription::MediaStreamDescriptionMap::iterator itmsd = itms->second.begin(); itmsd != itms->second.end(); ++itmsd) {
            SDPMediaDescription * md = sdp.GetMediaDescriptionByIndex(itmsd->first);
            if (md == NULL)
              cout << "  No description at index " << itmsd->first;
            else {
              cout << "  Media " << itmsd->first << ' ' << md->GetMediaType() << "  SSRC=";
              for (size_t i = 0; i < itmsd->second.size(); ++i) {
                if (i > 0)
                  cout << ',';
                cout << itmsd->second[i];
              }
            }
            cout << '\n';
          }
        }
      }
      else
        cout << "No media streams present.";

      cout << endl;
    }
  }

  cout << "Test completed." << endl;
}


// End of File ///////////////////////////////////////////////////////////////
