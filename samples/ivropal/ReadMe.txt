ivropal
=======

You can try it out with the provided test file, by entering the command:

    ./ivropal test.vxml

Then from another client connenct to host and use keypad to send commends for
the menu. test.vxml has the following dtmf options:
      Press 1 to play an announcement
      Press 2 to record a message
      Press 3 to transfer call
      Press 4 to speak to an operator
      Press 0 to disconnect

it also has a 30 second timeout where it will play timeout message.


test.vxml file needs a text to speach system (Windows SAPI or Linux Festival)
installed or the following WAV file to function correctly:

    menu_prompt.wav
    menu_timeout.wav
    menu_nomatch.wav
    message_recorded.wav
    next_operator.wav
    transfer_failed.wav
    transfer_timeout.wav
    transfer_failed.wav
    goodbye.wav


usage: ivropal [ options ] vxml [ url ]

Global options:
  -u or --user <arg>              : Set local username, defaults to OS username.
  -p or --password <arg>          : Set password for authentication.
  -D or --disable <arg>           : Disable use of specified media formats (codecs).
  -P or --prefer <arg>            : Set preference order for media formats (codecs).
  -O or --option <arg>            : Set options for media formatm argument is of form fmt:opt=val.
        --inband-detect           : Disable detection of in-band tones.
        --inband-send             : Disable transmission of in-band tones.
        --tel <arg>               : Protocol to use for tel: URI, e.g. sip

SIP options:
        --no-sip                  : Disable SIP
  -S or --sip <arg>               : SIP listens on interface, defaults to udp$*:5060.
  -r or --register <arg>          : SIP registration to server.
        --register-auth-id <arg>  : SIP registration authorisation id, default is username.
        --register-proxy <arg>    : SIP registration proxy, default is none.
        --register-ttl <arg>      : SIP registration Time To Live, default 300 seconds.
        --register-mode <arg>     : SIP registration mode (normal, single, public, ALG, RFC5626).
        --proxy <arg>             : SIP outbound proxy.
        --sip-ui <arg>            : SIP User Indication mode (inband,rfc2833,info-tone,info-string)

H.323 options:
        --no-h323                 : Disable H.323
  -H or --h323 <arg>              : H.323 listens on interface, defaults to tcp$*:1720.
  -g or --gk-host <arg>           : H.323 gatekeeper host.
  -G or --gk-id <arg>             : H.323 gatekeeper identifier.
        --no-fast                 : H.323 fast connect disabled.
        --no-tunnel               : H.323 tunnel for H.245 disabled.
        --h323-ui <arg>           : H.323 User Indication mode (inband,rfc2833,h245-signal,h245-string)

PSTN options:
        --no-lid                  : Disable Line Interface Devices
  -L or --lines <arg>             : Set Line Interface Devices.
        --country <arg>           : Select country to use for LID (eg "US", "au" or "+61").

IP options:
        --nat-method <arg>        : Set NAT method, defaults to STUN
        --nat-server <arg>        : Set NAT server for the above method
        --stun <arg>              : Set NAT traversal STUN server
        --translate <arg>         : Set external IP address if masqueraded
        --portbase <arg>          : Set TCP/UDP/RTP port base
        --portmax <arg>           : Set TCP/UDP/RTP port max
        --tcp-base <arg>          : Set TCP port base (default 0)
        --tcp-max <arg>           : Set TCP port max (default base+99)
        --udp-base <arg>          : Set UDP port base (default 6000)
        --udp-max <arg>           : Set UDP port max (default base+199)
        --rtp-base <arg>          : Set RTP port base (default 5000)
        --rtp-max <arg>           : Set RTP port max (default base+199)
        --rtp-tos <arg>           : Set RTP packet IP TOS bits to n
        --rtp-size <arg>          : Set RTP maximum payload size in bytes.
        --aud-qos <arg>           : Set Audio RTP Quality of Service to n
        --vid-qos <arg>           : Set Video RTP Quality of Service to n

Debug & General:
  -t or --trace                   : Trace enable (use multiple times for more detail).
        --trace-level <arg>       : Specify trace detail level.
  -o or --output <arg>            : Specify filename for trace output
                                    May be special value such as "stderr" dependent on platform.
        --trace-rollover <arg>    : Specify trace file rollover file name pattern.
        --trace-option <arg>      : Specify trace option(s),
                                    use +X or -X to add/remove option where X is one of:
                                      block    PTrace::Block constructs in output
                                      time     time since prgram start
                                      date     date and time
                                      gmt      Date/time is in UTC
                                      thread   thread name and identifier
                                      level    log level
                                      file     source file name and line number
                                      object   PObject pointer
                                      context  context identifier
                                      daily    rotate output file daily
                                      hour     rotate output file hourly
                                      minute   rotate output file every minute
                                      append   append to output file, otherwise overwrites
  -V or --version                 : Display application version.
  -h or --help                    : This help message.

where vxml is a VXML script, a URL to a VXML script or a WAV file, or a
series of commands separated by ';'.
Commands are:
  repeat=n    Repeat next WAV file n times.
  delay=n     Delay after repeats n milliseconds.
  voice=name  Set Text To Speech voice to name
  tone=t      Emit DTMF tone t
  silence=n   Emit silence for n milliseconds
  speak=text  Speak the text using the Text To Speech system.
  speak=$var  Speak the internal variable using the Text To Speech system.

Variables may be one of:
  Time
  Originator-Address
  Remote-Address
  Source-IP-Address

If a second parameter is provided and outgoing call is made and when answered
the script is executed. If no second paramter is provided then the program
will continuosly listen for incoming calls and execute the script on each
call. Simultaneous calls to the limits of the operating system are possible.

e.g. ivropal file://message.wav sip:fred@bloggs.com
     ivropal http://voicemail.vxml
     ivropal "repeat=5;delay=2000;speak=Hello, this is IVR!"

Sample output:

$ ./ivropal 
Default user name: markf
TCP ports: 0-0
UDP ports: 0-0
RTP ports: 5000-5999
Audio QoS: C5
Video QoS: C4
RTP payload size: 1400
Detected 11 network interfaces:
fe80::1%lo0 (lo0)
127.0.0.1 (lo0)
::1 (lo0)
fe80::225:ff:fea7:6310%en0 (en0)
130.15.48.199 (en0)
fe80::224:36ff:feb5:b916%en1 (en1)
130.15.41.24 (en1)
fe80::2cf8:4169:260e:70bb%utun0 (utun0)
fd57:8e7b:f637:4539:2cf8:4169:260e:70bb (utun0)
172.16.243.1 (vmnet1)
192.168.229.1 (vmnet8)
SIP listening on: udp$*:5060,tcp$0.0.0.0:5060,tcp$[::]:5060
SIP options: SendUserInputInBand
H.323 listening on: tcp$0.0.0.0:1720,tcp$[::]:1720
H.323 options: Fast connect, Tunnelled H.245, SendUserInputInBand
Line Interface Devices disabled
Media Formats: G.722.2,G.722.1-32K,G.722.1-24K,GSM-AMR,iLBC,GSM-06.10,G.726-40k,G.726-32k,G.726-24k,G.726-16k,G.711-uLaw-64k,G.711-ALaw-64k,H.264-1,H.264-0,MPEG4,H.263,H.263plus,H.261,PCM-16,YUV420P,RGB24,PCM-16-16kHz,PCM-16-32kHz,PCM-16-48kHz,PCM-16S-16kHz,PCM-16S-32kHz,PCM-16S-48kHz,G.722-64k,MS-GSM,iLBC-13k3,iLBC-15k2,MS-IMA-ADPCM,LPC-10,SILK-8,SILK-16,SpeexIETFNarrow-5.95k,SpeexIETFNarrow-8k,SpeexIETFNarrow-11k,SpeexIETFNarrow-15k,SpeexIETFNarrow-18.2k,SpeexIETFNarrow-24.6k,SpeexIETFWide-20.6k,SpeexWNarrow-8k,SpeexWide-20.6k,SpeexNB,SpeexWB,theora,VP8-WebM,VP8-OM,UserInput/RFC2833,NamedSignalEvent
s199:obj_Darwin_x86_64 markf$ 


