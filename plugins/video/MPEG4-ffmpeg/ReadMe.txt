OPAL MPEG4 plugin
-----------------

This plugin allows OPAL to use MPEG4 part 2 high-resolution video codec.

The plugin uses the FFMPEG library, also known as libavcodec, to encode and
decode video data. Please follow the instructions in ../common/ReadMe.txt to
make sure the library is installed for your platform.


Using the plugin
----------------

The following settings have been tested together and provide good performance:

LAN (~2 Mbit) @ 704x480, 24 fps:
 Encoder:
   - Max Bit Rate: 180000
   - Minimum Quality: 2
   - Maximum Quality: 20
   - Encoding Quality: 2
   - Dynamic Video Quality: 1        (this is a boolean value)
   - Bandwidth Throttling: 0         (this is a boolean value)
   - IQuantFactor: -1.0
   - Frame Time: 3750                (90000 / 3750 = 24 fps)

  Decoder:
   - Error Recovery: 1               (this is a boolean value)
   - Error Threshold: 1

WAN (~0.5 - 1 Mbit) @ 704x480, 24 fps:
 Encoder:
   - Max Bit Rate: 512000
   - Minimum Quality: 2
   - Maximum Quality: 12
   - Encoding Quality: 2
   - Dynamic Video Quality: 1
   - Bandwidth Throttling: 1
   - IQuantFactor: default (-0.8)
   - Frame Time: 3750                (90000 / 3750 = 24 fps)

  Decoder:
   - Error Recovery: 1
   - Error Threshold: 4

Internet (256 kbit) @ 352x240, 15 fps:
 Encoder:
   - Max Bit Rate: 220000
   - Minimum Quality: 4
   - Maximum Quality: 31
   - Encoding Quality: 4
   - Dynamic Video Quality: 1
   - Bandwidth Throttling: 1
   - IQuantFactor: -2.0
   - Frame Time: 6000                (90000 / 6000 = 15 fps)

  Decoder:
   - Error Recovery: 1
   - Error Threshold: 8

The codec was tested at 128 kbit and 64 kbit, but not recently; just keep
scaling down the Max Bit Rate and scaling up the Error Threshold, which
sets the number of corrupted blocks in an I-Frame required to trigger a
resend.


Interoperability
----------------

This plugins has been tested with linphone and should comply with RFC 3016.


                                   _o0o_
