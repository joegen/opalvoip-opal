opalcodecinfo
=============

Display plugin information, media formats, codec plugins, plugin list, info
about a media format.

available options:
  -m --mediaformats   display media formats
  -f --format fmt     display info about a media format
  -c --codecs         display standard codecs
  -p --pluginlist     display codec plugins
  -i --info name      display info about a plugin
  -a --capabilities   display all registered capabilities that match the specification
  -C --caps spec      display capabilities that match the specification
  -T --transcoders    display available transcoders
  -A --audio-test fmt Test audio transcoder: PCM-16->fmt then fmt->PCM-16
  -V --video-test fmt Test video transcoder: YUV420P->fmt then fmt->YUV420P
  -t --trace          Increment trace level
  -o --output         Trace output file
  -h --help           display this help message


Sample output. Output will vary based on plugins installed.

./codecinfo  -p
OpalCodecInfo Version 1.0.1 by Post Increment on Unix Darwin (12.3.0-x86_64)

Plugin codecs:
   h263_ffmpeg_ptplugin.dylib
   silk_ptplugin.dylib
   g7222_ptplugin.dylib
   gsm0610_ptplugin.dylib
   gsmamrcodec_ptplugin.dylib
   g7221_ptplugin.dylib
   iLBC_ptplugin.dylib
   h264_x264_ptplugin.dylib
   speex_ptplugin.dylib
   ima_adpcm_ptplugin.dylib
   theora_ptplugin.dylib
   mpeg4_ffmpeg_ptplugin.dylib
   lpc10_ptplugin.dylib
   vp8_webm_ptplugin.dylib
   g726_ptplugin.dylib
   g722_ptplugin.dylib
   h261_vic_ptplugin.dylib

./codecinfo  -i h264_x264_ptplugin.dylib
/codecinfo  -i h264_x264_ptplugin.dylib
OpalCodecInfo Version 1.0.1 by Post Increment on Unix Darwin (12.3.0-x86_64)

h264_x264_ptplugin.dylib contains 6 coders:
---------------------------------------
Coder 1
  Version:             7
  License
    Timestamp: Mon, 18 Mar 2013 16:33:36 -04:00
    Source
      Author:       Matthias Schneider
Robert Jongbloed, Vox Lucida Pty.Ltd.
      Version:      1.0
      Email:        robertj@voxlucida.com.au
      URL:          http://www.voxlucida.com.au
      Copyright:    Copyright (C) 2006 by Matthias Schneider
Copyright (C) 2011 by Vox Lucida Pt.Ltd., All Rights Reserved
      License:      MPL 1.0
      License Type: MPL
    Codec
      Description:  x264 Video Codec
      Author:       x264: Laurent Aimar, ffmpeg: Michael Niedermayer
      Version:      
      Email:        fenrir@via.ecp.fr, ffmpeg-devel-request@mplayerhq.hu
      URL:          http://developers.videolan.org/x264.html,    http://ffmpeg.mplayerhq.hu
      Copyright:    x264: Copyright (C) 2003 Laurent Aimar,    ffmpeg: Copyright (c) 2002-2003 Michael Niedermayer
      License:      x264: GNU General Public License as published Version 2,    ffmpeg: GNU Lesser General Public License, Version 2.1
      License Type: LGPL
  Flags:               Video, RTP input, RTP output, dynamic RTP payload
  Description:         x264 Video Codec
  Source format:       YUV420P
  Dest format:         H.264-0
  Userdata:            0x10a2d3b20
  Sample rate:         90000
  Bits/sec:            240000000
  Frame time (us):     33333
  Frame width (max):   1920
  Frame height (max):  1200
  Frame rate:          0
  Frame rate (max):    30
  RTP payload:         (not used)
  SDP format:          H264
  Create function:     0x10a2bc3c0
  Destroy function:    0x10a2bc020
  Encode function:     0x10a2bc500
  Codec controls:      
    get_output_data_size(0x10a2bc040)
    to_normalised_options(0x10a2bc060)
    to_customised_options(0x10a2bc090)
    set_codec_options(0x10a2bc0c0)
    get_active_options(0x10a2bc100)
    get_codec_options(0x10a2bc270)
    free_codec_options(0x10a2bc2b0)
    valid_for_protocol(0x10a2bc310)
    set_instance_id(0x10a2bc350)
    get_statistics(0x10a2bc380)
    terminate_codec(0x10a2bc3b0)
    set_log_function(0x10a2bb540)
  Capability type:     generic0x10a2d2820
---------------------------------------
Coder 2
  Version:             7
  License

...
Coder 6
  Version:             7
...



./codecinfo -m
OpalCodecInfo Version 1.0.1 by Post Increment on Unix Darwin (12.3.0-x86_64)

Registered media formats:
                 PCM-16  audio, bandwidth=128kb/s, RTP=[pt=127], h323=yes, sip=no
         G.711-uLaw-64k  audio, bandwidth=64kb/s, RTP=PCMU, h323=yes, sip=yes
         G.711-ALaw-64k  audio, bandwidth=64kb/s, RTP=PCMA, h323=yes, sip=yes
                YUV420P  video, bandwidth=583.9Mb/s, RTP=[pt=127], h323=yes, sip=no
    RFC4175_YCbCr-4:2:0  video, bandwidth=186.6Mb/s, RTP=[pt=97], h323=yes, sip=yes
                  RGB24  video, bandwidth=1.167Gb/s, RTP=[pt=127], h323=yes, sip=no
            RFC4175_RGB  video, bandwidth=373.2Mb/s, RTP=[pt=98], h323=yes, sip=yes
           PCM-16-16kHz  audio, bandwidth=256kb/s, RTP=[pt=127], h323=yes, sip=no
           PCM-16-32kHz  audio, bandwidth=512kb/s, RTP=[pt=127], h323=yes, sip=no
           PCM-16-48kHz  audio, bandwidth=768kb/s, RTP=[pt=127], h323=yes, sip=no
          PCM-16S-16kHz  audio, bandwidth=512kb/s, RTP=[pt=127], h323=yes, sip=no
          PCM-16S-32kHz  audio, bandwidth=1.024Mb/s, RTP=[pt=127], h323=yes, sip=no
          PCM-16S-48kHz  audio, bandwidth=1.536Mb/s, RTP=[pt=127], h323=yes, sip=no
            G.722.1-24K  audio, bandwidth=24kb/s, RTP=[pt=99], h323=yes, sip=yes
            G.722.1-32K  audio, bandwidth=32kb/s, RTP=[pt=100], h323=yes, sip=yes
                G.722.2  audio, bandwidth=24.8kb/s, RTP=[pt=101], h323=yes, sip=yes
              G.722-64k  audio, bandwidth=64kb/s, RTP=G722, h323=yes, sip=yes
              G.726-40k  audio, bandwidth=40kb/s, RTP=[pt=102], h323=yes, sip=yes
              G.726-32k  audio, bandwidth=32kb/s, RTP=[pt=103], h323=yes, sip=yes
              G.726-24k  audio, bandwidth=24kb/s, RTP=[pt=104], h323=yes, sip=yes
              G.726-16k  audio, bandwidth=16kb/s, RTP=[pt=105], h323=yes, sip=yes
              GSM-06.10  audio, bandwidth=13.2kb/s, RTP=GSM, h323=yes, sip=yes
                 MS-GSM  audio, bandwidth=13.2kb/s, RTP=[pt=106], h323=yes, sip=no
                GSM-AMR  audio, bandwidth=12.2kb/s, RTP=[pt=107], h323=yes, sip=yes
                   iLBC  audio, bandwidth=15.2kb/s, RTP=[pt=108], h323=yes, sip=yes
              iLBC-13k3  audio, bandwidth=13.33kb/s, RTP=[pt=109], h323=yes, sip=no
              iLBC-15k2  audio, bandwidth=15.2kb/s, RTP=[pt=110], h323=yes, sip=no
           MS-IMA-ADPCM  audio, bandwidth=32.44kb/s, RTP=PCMU, h323=yes, sip=no
                 LPC-10  audio, bandwidth=2.4kb/s, RTP=[pt=111], h323=yes, sip=yes
                 SILK-8  audio, bandwidth=483.8kb/s, RTP=[pt=112], h323=yes, sip=yes
                SILK-16  audio, bandwidth=188.8kb/s, RTP=[pt=113], h323=yes, sip=yes
  SpeexIETFNarrow-5.95k  audio, bandwidth=5.95kb/s, RTP=[pt=114], h323=yes, sip=no
     SpeexIETFNarrow-8k  audio, bandwidth=8kb/s, RTP=[pt=115], h323=yes, sip=no
    SpeexIETFNarrow-11k  audio, bandwidth=11kb/s, RTP=[pt=116], h323=yes, sip=no
    SpeexIETFNarrow-15k  audio, bandwidth=15kb/s, RTP=[pt=117], h323=yes, sip=no
  SpeexIETFNarrow-18.2k  audio, bandwidth=18.2kb/s, RTP=[pt=118], h323=yes, sip=no
  SpeexIETFNarrow-24.6k  audio, bandwidth=24.6kb/s, RTP=[pt=119], h323=yes, sip=no
    SpeexIETFWide-20.6k  audio, bandwidth=20.6kb/s, RTP=[pt=120], h323=yes, sip=no
        SpeexWNarrow-8k  audio, bandwidth=8kb/s, RTP=[pt=121], h323=yes, sip=no
        SpeexWide-20.6k  audio, bandwidth=20.6kb/s, RTP=[pt=122], h323=yes, sip=no
                SpeexNB  audio, bandwidth=8kb/s, RTP=[pt=123], h323=no, sip=yes
                SpeexWB  audio, bandwidth=20.6kb/s, RTP=[pt=124], h323=no, sip=yes
                  H.261  video, bandwidth=621.7kb/s, RTP=H261, h323=yes, sip=yes
                  H.263  video, bandwidth=327.6kb/s, RTP=H263, h323=yes, sip=yes
              H.263plus  video, bandwidth=327.6kb/s, RTP=[pt=125], h323=yes, sip=yes
                H.264-0  video, bandwidth=240Mb/s, RTP=[pt=126], h323=yes, sip=yes
                H.264-1  video, bandwidth=240Mb/s, RTP=[pt=95], h323=yes, sip=yes
             H.264-High  video, bandwidth=240Mb/s, RTP=[pt=94], h323=yes, sip=yes
                  MPEG4  video, bandwidth=8Mb/s, RTP=[pt=93], h323=yes, sip=yes
                 theora  video, bandwidth=768kb/s, RTP=[pt=92], h323=no, sip=yes
               VP8-WebM  video, bandwidth=16Mb/s, RTP=[pt=91], h323=yes, sip=yes
                 VP8-OM  video, bandwidth=16Mb/s, RTP=[pt=96], h323=yes, sip=yes


./codecinfo -f h.261 
OpalCodecInfo Version 1.0.1 by Post Increment on Unix Darwin (12.3.0-x86_64)

        Media Format Name       = H.261
       Default Session ID (R/O) = 2
             Payload type (R/O) = 31
            Encoding name (R/O) = h261
                  Annex D (R/O) = 0           FMTP name: D (0)
                  CIF MPI (R/W) = 1           FMTP name: CIF (33)
               Clock Rate (R/O) = 90000     
             Content Role (R/W) = No Role   
        Content Role Mask (R/W) = 0         
             Frame Height (R/W) = 288       
               Frame Time (R/W) = 1500      
              Frame Width (R/W) = 352       
             Max Bit Rate (R/W) = 621700    
      Max Rx Frame Height (R/O) = 288       
       Max Rx Frame Width (R/O) = 352       
       Max Tx Packet Size (R/O) = 1444      
      Min Rx Frame Height (R/O) = 144       
       Min Rx Frame Width (R/O) = 176       
                 Protocol (R/O) =           
                 QCIF MPI (R/W) = 1           FMTP name: QCIF (33)
      Rate Control Period (R/W) = 1000      
          Rate Controller (R/W) =           
            RTCP Feedback (R/W) = pli fir tmmbr tstr
          Target Bit Rate (R/W) = 621700    
      Tx Key Frame Period (R/W) = 125       
Use Image Attribute in SDP (R/W) = 0         

s199:obj_Darwin_x86_64 markf$ ./codecinfo -f h.263 
OpalCodecInfo Version 1.0.1 by Post Increment on Unix Darwin (12.3.0-x86_64)

        Media Format Name       = H.263
       Default Session ID (R/O) = 2
             Payload type (R/O) = 34
            Encoding name (R/O) = H263
Annex F - Advanced Prediction (R/W) = 1           FMTP name: F (0)
                  CIF MPI (R/W) = 1           FMTP name: CIF (33)
                CIF16 MPI (R/W) = 1           FMTP name: CIF16 (33)
                 CIF4 MPI (R/W) = 1           FMTP name: CIF4 (33)
               Clock Rate (R/O) = 90000     
             Content Role (R/W) = No Role   
        Content Role Mask (R/W) = 0         
             Frame Height (R/W) = 288       
               Frame Time (R/W) = 3000      
              Frame Width (R/W) = 352       
             Max Bit Rate (R/W) = 327600    
      Max Rx Frame Height (R/W) = 1200      
       Max Rx Frame Width (R/W) = 1920      
       Max Tx Packet Size (R/O) = 1444      
                    MaxBR (R/W) = 0           FMTP name: maxbr (0)
      Media Packetization (R/O) = RFC2190   
      Min Rx Frame Height (R/W) = 96        
       Min Rx Frame Width (R/W) = 128       
                 Protocol (R/O) =           
                 QCIF MPI (R/W) = 1           FMTP name: QCIF (33)
      Rate Control Period (R/W) = 1000      
          Rate Controller (R/W) =           
            RTCP Feedback (R/W) = pli fir tmmbr tstr
                SQCIF MPI (R/W) = 1           FMTP name: SQCIF (33)
          Target Bit Rate (R/W) = 327600    
      Tx Key Frame Period (R/W) = 125       
Use Image Attribute in SDP (R/W) = 0         

./codecinfo -f h.264-0 
OpalCodecInfo Version 1.0.1 by Post Increment on Unix Darwin (12.3.0-x86_64)

        Media Format Name       = H.264-0
       Default Session ID (R/O) = 2
             Payload type (R/O) = 126
            Encoding name (R/O) = H264
               Clock Rate (R/O) = 90000     
             Content Role (R/W) = No Role   
        Content Role Mask (R/W) = 0         
             Frame Height (R/W) = 288       
               Frame Time (R/W) = 3000      
              Frame Width (R/W) = 352       
              H.241 Level (R/O) = 71          H.245 Ordinal: 42 Collapsing TCS OLC RM
             H.241 Max BR (R/O) = 0           H.245 Ordinal: 6 Collapsing TCS OLC RM
             H.241 Max FS (R/O) = 0           H.245 Ordinal: 4 Collapsing TCS OLC RM
           H.241 Max MBPS (R/O) = 0           H.245 Ordinal: 3 Collapsing TCS OLC RM
          H.241 Max SMBPS (R/O) = 0           H.245 Ordinal: 7 Collapsing TCS OLC RM
       H.241 Profile Mask (R/O) = 64          H.245 Ordinal: 41 Collapsing TCS OLC RM
                    Level (R/W) = 3.1       
             Max Bit Rate (R/W) = 240000000 
            Max NALU Size (R/W) = 1400        FMTP name: max-rcmd-nalu-size (1400)  H.245 Ordinal: 9 Collapsing TCS OLC RM
      Max Rx Frame Height (R/W) = 1200      
       Max Rx Frame Width (R/W) = 1920      
       Max Tx Packet Size (R/O) = 1444      
     Media Packetizations (R/W) = 0.0.8.241.0.0.0.0
      Min Rx Frame Height (R/W) = 96        
       Min Rx Frame Width (R/W) = 128       
       Packetization Mode (R/O) = 0           FMTP name: packetization-mode (0)
                  Profile (R/W) = Baseline  
                 Protocol (R/O) =           
      Rate Control Period (R/W) = 1000      
          Rate Controller (R/W) =           
            RTCP Feedback (R/W) = pli fir tmmbr tstr
Send Access Unit Delimiters (R/W) = 0         
           SIP/SDP Max BR (R/O) = 0           FMTP name: max-br (0)
           SIP/SDP Max FS (R/O) = 0           FMTP name: max-fs (0)
         SIP/SDP Max MBPS (R/O) = 0           FMTP name: max-mbps (0)
        SIP/SDP Max SMBPS (R/O) = 0           FMTP name: max-smbps (0)
  SIP/SDP Profile & Level (R/O) = 42801f      FMTP name: profile-level-id (42800A)
          Target Bit Rate (R/W) = 240000000 
Temporal Spatial Trade Off (R/W) = 31        
      Tx Key Frame Period (R/W) = 125       
Use Image Attribute in SDP (R/W) = 0   

./codecinfo -f h.264-1
OpalCodecInfo Version 1.0.1 by Post Increment on Unix Darwin (12.3.0-x86_64)

        Media Format Name       = H.264-1
       Default Session ID (R/O) = 2
             Payload type (R/O) = 95
            Encoding name (R/O) = H264
               Clock Rate (R/O) = 90000     
             Content Role (R/W) = No Role   
        Content Role Mask (R/W) = 0         
             Frame Height (R/W) = 288       
               Frame Time (R/W) = 3000      
              Frame Width (R/W) = 352       
              H.241 Level (R/O) = 71          H.245 Ordinal: 42 Collapsing TCS OLC RM
             H.241 Max BR (R/O) = 0           H.245 Ordinal: 6 Collapsing TCS OLC RM
             H.241 Max FS (R/O) = 0           H.245 Ordinal: 4 Collapsing TCS OLC RM
           H.241 Max MBPS (R/O) = 0           H.245 Ordinal: 3 Collapsing TCS OLC RM
          H.241 Max SMBPS (R/O) = 0           H.245 Ordinal: 7 Collapsing TCS OLC RM
       H.241 Profile Mask (R/O) = 64          H.245 Ordinal: 41 Collapsing TCS OLC RM
                    Level (R/W) = 3.1       
             Max Bit Rate (R/W) = 240000000 
            Max NALU Size (R/W) = 1400        FMTP name: max-rcmd-nalu-size (1400)  H.245 Ordinal: 9 Collapsing TCS OLC RM
      Max Rx Frame Height (R/W) = 1200      
       Max Rx Frame Width (R/W) = 1920      
       Max Tx Packet Size (R/O) = 1444      
     Media Packetizations (R/W) = 0.0.8.241.0.0.0.0,0.0.8.241.0.0.0.1
      Min Rx Frame Height (R/W) = 96        
       Min Rx Frame Width (R/W) = 128       
       Packetization Mode (R/O) = 1           FMTP name: packetization-mode (0)
                  Profile (R/W) = Baseline  
                 Protocol (R/O) =           
      Rate Control Period (R/W) = 1000      
          Rate Controller (R/W) =           
            RTCP Feedback (R/W) = pli fir tmmbr tstr
Send Access Unit Delimiters (R/W) = 0         
           SIP/SDP Max BR (R/O) = 0           FMTP name: max-br (0)
           SIP/SDP Max FS (R/O) = 0           FMTP name: max-fs (0)
         SIP/SDP Max MBPS (R/O) = 0           FMTP name: max-mbps (0)
        SIP/SDP Max SMBPS (R/O) = 0           FMTP name: max-smbps (0)
  SIP/SDP Profile & Level (R/O) = 42801f      FMTP name: profile-level-id (42800A)
          Target Bit Rate (R/W) = 240000000 
Temporal Spatial Trade Off (R/W) = 31        
      Tx Key Frame Period (R/W) = 125       
Use Image Attribute in SDP (R/W) = 0         

./codecinfo -f h.264-high
OpalCodecInfo Version 1.0.1 by Post Increment on Unix Darwin (12.3.0-x86_64)

        Media Format Name       = H.264-High
       Default Session ID (R/O) = 2
             Payload type (R/O) = 94
            Encoding name (R/O) = H264
               Clock Rate (R/O) = 90000     
             Content Role (R/W) = No Role   
        Content Role Mask (R/W) = 0         
             Frame Height (R/W) = 288       
               Frame Time (R/W) = 3000      
              Frame Width (R/W) = 352       
              H.241 Level (R/O) = 71          H.245 Ordinal: 42 Collapsing TCS OLC RM
             H.241 Max BR (R/O) = 0           H.245 Ordinal: 6 Collapsing TCS OLC RM
             H.241 Max FS (R/O) = 0           H.245 Ordinal: 4 Collapsing TCS OLC RM
           H.241 Max MBPS (R/O) = 0           H.245 Ordinal: 3 Collapsing TCS OLC RM
          H.241 Max SMBPS (R/O) = 0           H.245 Ordinal: 7 Collapsing TCS OLC RM
       H.241 Profile Mask (R/O) = 8           H.245 Ordinal: 41 Collapsing TCS OLC RM
                    Level (R/W) = 3.1       
             Max Bit Rate (R/W) = 240000000 
            Max NALU Size (R/W) = 1400        FMTP name: max-rcmd-nalu-size (1400)  H.245 Ordinal: 9 Collapsing TCS OLC RM
      Max Rx Frame Height (R/W) = 1200      
       Max Rx Frame Width (R/W) = 1920      
       Max Tx Packet Size (R/O) = 1444      
     Media Packetizations (R/W) = 0.0.8.241.0.0.0.0,0.0.8.241.0.0.0.1
      Min Rx Frame Height (R/W) = 96        
       Min Rx Frame Width (R/W) = 128       
       Packetization Mode (R/O) = 1           FMTP name: packetization-mode (0)
                  Profile (R/W) = High      
                 Protocol (R/O) =           
      Rate Control Period (R/W) = 1000      
          Rate Controller (R/W) =           
            RTCP Feedback (R/W) = pli fir tmmbr tstr
Send Access Unit Delimiters (R/W) = 0         
           SIP/SDP Max BR (R/O) = 0           FMTP name: max-br (0)
           SIP/SDP Max FS (R/O) = 0           FMTP name: max-fs (0)
         SIP/SDP Max MBPS (R/O) = 0           FMTP name: max-mbps (0)
        SIP/SDP Max SMBPS (R/O) = 0           FMTP name: max-smbps (0)
  SIP/SDP Profile & Level (R/O) = 42801f      FMTP name: profile-level-id (42800A)
          Target Bit Rate (R/W) = 240000000 
Temporal Spatial Trade Off (R/W) = 31        
      Tx Key Frame Period (R/W) = 125       
Use Image Attribute in SDP (R/W) = 0         


./codecinfo -C h.261
OpalCodecInfo Version 1.0.1 by Post Increment on Unix Darwin (12.3.0-x86_64)

Capability set using "h.261" :
     Table:
       H.261 <1>
     Set:
       0:
         0:
           H.261 <1>

./codecinfo -C h.263
OpalCodecInfo Version 1.0.1 by Post Increment on Unix Darwin (12.3.0-x86_64)

Capability set using "h.263" :
     Table:
       H.263 <1>
     Set:
       0:
         0:
           H.263 <1>


./codecinfo -T
OpalCodecInfo Version 1.0.1 by Post Increment on Unix Darwin (12.3.0-x86_64)

Available trancoders:
   G.711-ALaw-64k->PCM-16
   G.711-uLaw-64k->PCM-16
   G.722-64k->PCM-16-16kHz
   G.722.1-24K->PCM-16-16kHz
   G.722.1-32K->PCM-16-16kHz
   G.722.2->PCM-16-16kHz
   G.726-16k->PCM-16
   G.726-24k->PCM-16
   G.726-32k->PCM-16
   G.726-40k->PCM-16
   GSM-06.10->PCM-16
   GSM-AMR->PCM-16
   H.261->YUV420P
   H.263->YUV420P  CANNOT BE INSTANTIATED
   H.263plus->YUV420P  CANNOT BE INSTANTIATED
   H.264-0->YUV420P  CANNOT BE INSTANTIATED
   H.264-1->YUV420P  CANNOT BE INSTANTIATED
   H.264-High->YUV420P  CANNOT BE INSTANTIATED
   LPC-10->PCM-16
   MPEG4->YUV420P  CANNOT BE INSTANTIATED
   MS-GSM->PCM-16
   MS-IMA-ADPCM->PCM-16
   PCM-16->G.711-ALaw-64k
   PCM-16->G.711-uLaw-64k
   PCM-16->G.726-16k
   PCM-16->G.726-24k
   PCM-16->G.726-32k
   PCM-16->G.726-40k
   PCM-16->GSM-06.10
   PCM-16->GSM-AMR
   PCM-16->LPC-10
   PCM-16->MS-GSM
   PCM-16->MS-IMA-ADPCM
   PCM-16->SILK-8
   PCM-16->SpeexIETFNarrow-11k
   PCM-16->SpeexIETFNarrow-15k
   PCM-16->SpeexIETFNarrow-18.2k
   PCM-16->SpeexIETFNarrow-24.6k
   PCM-16->SpeexIETFNarrow-5.95k
   PCM-16->SpeexIETFNarrow-8k
   PCM-16->SpeexNB
   PCM-16->SpeexWNarrow-8k
   PCM-16->iLBC
   PCM-16->iLBC-13k3
   PCM-16->iLBC-15k2
   PCM-16-16kHz->G.722-64k
   PCM-16-16kHz->G.722.1-24K
   PCM-16-16kHz->G.722.1-32K
   PCM-16-16kHz->G.722.2
   PCM-16-16kHz->SILK-16
   PCM-16-16kHz->SpeexIETFWide-20.6k
   PCM-16-16kHz->SpeexWB
   PCM-16-16kHz->SpeexWide-20.6k
   RFC4175_RGB->RGB24
   RFC4175_YCbCr-4:2:0->YUV420P
   RGB24->RFC4175_RGB
   SILK-16->PCM-16-16kHz
   SILK-8->PCM-16
   SpeexIETFNarrow-11k->PCM-16
   SpeexIETFNarrow-15k->PCM-16
   SpeexIETFNarrow-18.2k->PCM-16
   SpeexIETFNarrow-24.6k->PCM-16
   SpeexIETFNarrow-5.95k->PCM-16
   SpeexIETFNarrow-8k->PCM-16
   SpeexIETFWide-20.6k->PCM-16-16kHz
   SpeexNB->PCM-16
   SpeexWB->PCM-16-16kHz
   SpeexWNarrow-8k->PCM-16
   SpeexWide-20.6k->PCM-16-16kHz
   VP8-OM->YUV420P
   VP8-WebM->YUV420P
   YUV420P->H.261
   YUV420P->H.263  CANNOT BE INSTANTIATED
   YUV420P->H.263plus  CANNOT BE INSTANTIATED
   YUV420P->H.264-0x264-help	
   YUV420P->H.264-1Error reading down link: Invalid argument
x264-help
   YUV420P->H.264-High	Error reading down link: Invalid argument
x264-help
   YUV420P->MPEG4  CANNOT BE INSTANTIATED
   YUV420P->RFC4175_YCbCr-4:2:0
   YUV420P->VP8-OM	Error reading down link: Invalid argument

   YUV420P->VP8-WebM
   YUV420P->theora
   iLBC->PCM-16
   iLBC-13k3->PCM-16
   iLBC-15k2->PCM-16
   theora->YUV420P


                                   --oOo--
