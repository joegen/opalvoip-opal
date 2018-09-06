dnl
dnl OPAL configuration
dnl

AC_INIT([OPAL-Plugins], m4_esyscmd([sed -n \
                            -e "s/ *#define *MAJOR_VERSION *\\(.*\\)/\\1./p" \
                            -e "s/ *#define *MINOR_VERSION *\\(.*\\)/\\1/p" \
                            -e "s/ *#define *BUILD_TYPE *AlphaCode/alpha/p" \
                            -e "s/ *#define *BUILD_TYPE *BetaCode/beta/p" \
                            -e "s/ *#define *BUILD_TYPE *ReleaseCode/./p" \
                            -e "s/ *#define *BUILD_NUMBER *\\(.*\\)/\\1/p" \
                            ../version.h | tr -d '\n']))

AC_PREREQ([2.66])

AC_CONFIG_AUX_DIR(../make)

AC_CONFIG_SRCDIR([plugin_config.h.in])

m4_include(../make/my_macros.m4)
m4_include(../make/opal.m4)


dnl ########################################################################

AC_CHECK_TOOL([NM], [nm])
AC_CHECK_TOOL([LDD], [ldd])

AC_HEADER_STDC()

MY_CHECK_DLFCN()
AC_SUBST(DLFCN_LIBS)


dnl ########################################################################
dnl Extract OPAL version number

OPAL_DETERMINE_DIRECTORIES([$srcdir/..])

MY_VERSION_FILE([$VERSION_DIR/version.h], [OPAL])


dnl ########################################################################
dnl Directories

OPAL_DETERMINE_PLUGIN_DIR()

AC_SUBST(AUD_PLUGIN_DIR, "${OPAL_PLUGIN_DIR}/codecs/audio")
AC_SUBST(VID_PLUGIN_DIR, "${OPAL_PLUGIN_DIR}/codecs/video")
AC_SUBST(LID_PLUGIN_DIR, "${OPAL_PLUGIN_DIR}/lid")
AC_SUBST(FAX_PLUGIN_DIR, "${OPAL_PLUGIN_DIR}/fax")

AC_SUBST(PLUGIN_SRC_DIR, `cd $srcdir ; pwd`)

AC_SUBST(PLUGIN_SUBDIRS, "")


dnl ########################################################################
dnl Audio Codecs
dnl ########################################################################

OPAL_SIMPLE_PLUGIN([SILK],     [silk],     [Skype SILK audio codec],                [$PLUGIN_SRC_DIR/audio/SILK])
OPAL_SIMPLE_PLUGIN([GSMAMR],   [gsmamr],   [GSM-AMR audio codec],                   [$PLUGIN_SRC_DIR/audio/gsm-amr])
OPAL_SIMPLE_PLUGIN([G726],     [g.726],    [G.726 audio codec],                     [$PLUGIN_SRC_DIR/audio/G726])
OPAL_SIMPLE_PLUGIN([IMA_ADPCM],[ima-adpcm],[IMA ADPCM codec],                       [$PLUGIN_SRC_DIR/audio/IMA_ADPCM])
OPAL_SIMPLE_PLUGIN([G722],     [g.722],    [G.722 wideband audio codec],            [$PLUGIN_SRC_DIR/audio/G722])
OPAL_SIMPLE_PLUGIN([G7222],    [g.722.2],  [G.722.2 wideband audio codec],          [$PLUGIN_SRC_DIR/audio/G.722.2])
OPAL_SIMPLE_PLUGIN([LPC10],    [lpc10],    [LPC-10 very low bandwidth audio codec], [$PLUGIN_SRC_DIR/audio/LPC_10])
OPAL_SIMPLE_PLUGIN([ISAC],     [isac],     [iSAC audio codec],                      [$PLUGIN_SRC_DIR/audio/iSAC])


HAVE_G7221=no
OPAL_SIMPLE_PLUGIN([G7221],    [g.722.1],  [G.722.1 wideband audio codec],          [$PLUGIN_SRC_DIR/audio/G.722.1],[
   if test -n "${srcdir}" && test `cd ${srcdir} ; pwd` != `pwd`; then
      HAVE_G7221="no (internal libg722_1 does not support out-of-source configure)"
   else
      AC_CONFIG_SUBDIRS(./audio/G.722.1/libg722_1)
   fi
])


OPAL_SYSTEM_PLUGIN(
   [GSM],
   [gsm],
   [GSM 06.10 audio codec],
   [$PLUGIN_SRC_DIR/audio/GSM0610],
   [gsm local-source],
   [],
   [-lgsm],
   [#include <gsm.h>],
   [gsm_create()]
)

OPAL_SYSTEM_PLUGIN(
   [ILBC],
   [iLBC],
   [iLBC audio codec],
   [$PLUGIN_SRC_DIR/audio/iLBC],
   [ilbc local-source],
   [],
   [-lilbc],
   [#include <iLBC_encode.h>],
   [
      struct iLBC_Enc_Inst_t_ * context;
      initEncode(context, 30);
   ],
   [
      if test "$ILBC_SYSTEM" = "yes"; then
         AC_DEFINE(OPAL_SYSTEM_ILBC, [1])
      fi
   ]
)

OPAL_SYSTEM_PLUGIN(
   [SPEEX],
   [speex],
   [Speex audio codec],
   [$PLUGIN_SRC_DIR/audio/Speex],
   [speex local-source],
   [],
   [-lspeex],
   [#include <speex/speex.h>],
   [speex_encoder_init(&speex_wb_mode)],
   [
      if test "$SPEEX_SYSTEM" = "yes"; then
         AC_DEFINE(OPAL_SYSTEM_SPEEX, [1])
      fi
   ]
)

OPAL_SYSTEM_PLUGIN(
   [SBC],
   [sbc],
   [Bluetooth SBC audio codec],
   [$PLUGIN_SRC_DIR/audio/SBC/bluez_sbc],
   [samplerate],
   [],
   [-lsamplerate],
   [#include <samplerate.h>],
   [src_get_version()]
)

OPAL_SYSTEM_PLUGIN(
   [CELT],
   [celt],
   [CELT audio codec],
   [$PLUGIN_SRC_DIR/audio/celt],
   [celt],
   [],
   [],
   [#include <celt/celt.h>],
   [celt_mode_create(8000, 1 320, 0)],
   [
      CELT_CFLAGS="${CELT_CFLAGS} -DCELT_VERSION=\"`pkg-config --modversion celt | \
                  sed -e "s/\./*10000+/" -e "s/\./*100+/"`\""
   ]
)


OPAL_SYSTEM_PLUGIN(
   [OPUS],
   [opus],
   [Opus Interactive Audio Codec],
   [$PLUGIN_SRC_DIR/audio/Opus],
   [opus local-source],
   [],
   [-lopus],
   [#include <opus.h>],
   [opus_get_version_string()],
   AS_VAR_IF([OPUS_SYSTEM],[no],[
      AS_IF([test -n "${srcdir}" && test `cd ${srcdir} ; pwd` != `pwd`],[
         HAVE_OPUS="no (internal libopus does not support out-of-source configure)"
         OPUS_USABLE=no
      ],[
         AC_CONFIG_SUBDIRS(./audio/Opus/opus)
         ac_configure_args="$ac_configure_args --with-pic"
      ])
   ])
)

dnl ########################################################################
dnl Video Codecs
dnl ########################################################################

OPAL_SIMPLE_PLUGIN([H261], [h.261], [H.261 video codec], [$PLUGIN_SRC_DIR/video/H.261-vic])

dnl MY_MODULE_OPTION adds to LIBS and normally this right, but for plugins
dnl we want each plugin to only have its libs so don't add it for everyone
oldLIBS="$LIBS"

MY_MODULE_OPTION(
   [LIBAVUTIL],
   [libavutil],
   [FFMPEG libavutil library],
   [libavutil >= 54.0.0],
   [],
   [-lavutil],
   [#include <libavutil/avutil.h>],
   [avutil_version()],
   [MY_MODULE_OPTION(
      [LIBSWRESAMPLE],
      [libswresample],
      [FFMPEG libswresample library],
      [libswresample >= 1.1.0],
      [],
      [-lswresample],
      [#include <libswresample/swresample.h>],
      [swresample_version()],
      [MY_MODULE_OPTION(
         [LIBAVCODEC],
         [libavcodec],
         [FFMPEG libavcodec library],
         [libavcodec >= 56.0.0],
         [],
         [-lavcodec],
         [#include <libavcodec/avcodec.h>],
         [avcodec_version()],
         [
            HAVE_LIBAVCODEC=yes
            LIBAVCODEC_LIBS="$LIBAVCODEC_LIBS $LIBAVUTIL_LIBS $LIBSWRESAMPLE_LIBS"
         ],
         [HAVE_LIBAVCODEC=no]
      )],
      [HAVE_LIBAVCODEC=no]
   )],
   [HAVE_LIBAVCODEC=no]
)

LIBS="$oldLIBS"


AC_SUBST(HAVE_LIBAVCODEC_RTP_MODE, 0)

if test "x$HAVE_LIBAVCODEC" = "xyes" ; then
   AC_DEFINE_UNQUOTED(LIBAVCODEC_LIB_NAME, "libavcodec.$SHAREDLIBEXT")
   AC_DEFINE_UNQUOTED(LIBAVUTIL_LIB_NAME, "libavutil.$SHAREDLIBEXT")

   MY_COMPILE_IFELSE(
      [if libavcodec has rtp_mode],
      [$LIBAVCODEC_CFLAGS],
      [#include <libavcodec/avcodec.h>],
      [
         AVCodecContext c;
         c.rtp_mode = 1;
      ],
      [HAVE_LIBAVCODEC_RTP_MODE=1]
   )

   MY_COMPILE_IFELSE(
      [if libavutil has AVPictureType],
      [$LIBAVCODEC_CFLAGS],
      [#include <libavcodec/avcodec.h>],
      [enum AVPictureType t],
      [MY_COMPILE_IFELSE(
         [if libavutil has AV_PICTURE_TYPE_NONE],
         [$LIBAVCODEC_CFLAGS],
         [#include <libavcodec/avcodec.h>],
         [enum AVPictureType t = AV_PICTURE_TYPE_NONE],
         [],
         [AC_DEFINE(AV_PICTURE_TYPE_NONE, [(AVPictureType)0])]
      )],
      [
         AC_DEFINE(AV_PICTURE_TYPE_NONE, [0])
         AC_DEFINE(AV_PICTURE_TYPE_I, [FF_I_TYPE])
      ]
   )

   OPAL_SIMPLE_PLUGIN([H263],  [h.263], [H.263/H.263+ video codec], [$PLUGIN_SRC_DIR/video/H.263-1998])
   OPAL_SIMPLE_PLUGIN([MPEG4], [mpeg4], [MPEG4 part 2 video codec], [$PLUGIN_SRC_DIR/video/MPEG4-ffmpeg])

   OPAL_SYSTEM_PLUGIN(
      [X264],
      [x264],
      [VideoLan x264 H.264 video codec],
      [$PLUGIN_SRC_DIR/video/x264],
      [x264 >= 0.102],
      [],
      [-lx264],
      [
         #if defined HAVE_STDINT_H
           #include <stdint.h>
         #elif defined HAVE_INTTYPES_H
           #include <inttypes.h>
         #endif
         #include <x264.h>
         #if X264_BUILD < 102
         #error x264 header too old
         #endif
      ],
      [
         x264_param_t param;
         x264_encoder_open(&param);
      ]
   )

   AC_ARG_ENABLE([x264-licensed], [AC_HELP_STRING([--enable-x264-licensed],[enable commercially licensed x264])])
   if test "$enable_x264_licensed" = "yes" ;  then
      X264_CFLAGS="$X264_CFLAGS -DX264_LICENSED"
      AC_SUBST(X264_HELPER, "")
      HAVE_X264="Licensed $HAVE_X264"
   else
      AC_SUBST(X264_HELPER, "gpl")
      HAVE_X264="GPL $HAVE_X264"
   fi
else
   HAVE_H263="no (missing FFMPEG)"
   HAVE_H264="no (missing FFMPEG)"
   HAVE_MPEG4="no (missing FFMPEG)"
fi


MY_LINK_IFELSE(
   [Cisco Open H.264 shared library],
   [],
   [-lopenh264 -lpthread],
   [#include "$PLUGIN_SRC_DIR/video/openh264/svc/codec_api.h"],
   [
      ISVCEncoder * encoder;
      WelsCreateSVCEncoder(&encoder);
   ],
   [OPAL_SYSTEM_PLUGIN(
      [OPENH264],
      [openh264],
      [Cisco Open H.264 video codec],
      [$PLUGIN_SRC_DIR/video/openh264],
      [local-source],
      [],
      [-lopenh264 -lpthread]
   )],
   [
      HAVE_OPENH264="no (missing shared library)"
   ]
)

OPAL_SYSTEM_PLUGIN(
   [VP8],
   [VP8],
   [VP8 (WebM) video codec],
   [$PLUGIN_SRC_DIR/video/VP8-WebM],
   [vpx],
   [],
   [-lvpx],
   [#include <vpx/vpx_encoder.h>],
   [vpx_codec_iface_t * bp8 = vpx_codec_vp8_cx();]
)

OPAL_SYSTEM_PLUGIN(
   [THEORA],
   [theora],
   [Theora video codec],
   [$PLUGIN_SRC_DIR/video/THEORA],
   [theora],
   [],
   [-ltheora -logg],
   [#include <theora/theora.h>],
   [
      theora_info info;
      theora_info_init(&info);
   ]
)


dnl ########################
dnl Line Interface Devices
dnl ########################

OPAL_SYSTEM_PLUGIN(
   [VPB],
   [vpb],
   [Voicetronics VPB telephony card],
   [$PLUGIN_SRC_DIR/LID/VPB],
   [vpb],
   [],
   [-lvpb],
   [#include <vpbapi.h>]
)


dnl ########################################################################
dnl FAX Devices
dnl ########################################################################

OPAL_SYSTEM_PLUGIN(
   [SPANDSP],
   [spandsp],
   [SpanDSP based fax],
   [$PLUGIN_SRC_DIR/fax/fax_spandsp],
   [spandsp],
   [],
   [-lspandsp],
   [#include <spandsp.h>],
   [fax_state_t * fax = fax_init(NULL, 1)]
)

dnl ########################################################################
dnl Various checks
dnl ########################################################################

MY_LINK_IFELSE(
   [for posix_memalign],
   [],
   [],
   [#include <stdlib.h>], 
   [
      void * p;
      posix_memalign(&p, 16, 16);
   ],
   [AC_DEFINE(HAVE_POSIX_MEMALIGN, 1)]
)


dnl ########################################################################
dnl Output configured files

AC_CONFIG_FILES(plugin_config.mak)
AC_CONFIG_HEADERS(plugin_config.h)

AC_OUTPUT()


dnl ########################################################################
dnl Summary
dnl ########################################################################
MY_OUTPUT_SUMMARY(
   [   Audio Codec plugin install dir], AUD_PLUGIN_DIR,
   [   Video Codec plugin install dir], VID_PLUGIN_DIR,
   [        LID plugin install prefix], LID_PLUGIN_DIR,
   [        FAX plugin install prefix], FAX_PLUGIN_DIR,
   [], [],
   [                              GSM], HAVE_GSM,
   [                          GSM-AMR], HAVE_GSMAMR,
   [                             iLBC], HAVE_ILBC,
   [                             iSAC], HAVE_ISAC,
   [                            Speex], HAVE_SPEEX,
   [                            G.722], HAVE_G722,
   [                          G.722.1], HAVE_G7221,
   [                          G.722.2], HAVE_G7222,
   [                            G.726], HAVE_G726,
   [                            LPC10], HAVE_LPC10,
   [                        IMA_ADPCM], HAVE_IMA_ADPCM,
   [                              SBC], HAVE_SBC,
   [                             CELT], HAVE_CELT,
   [                             SILK], HAVE_SILK,
   [                             Opus], HAVE_OPUS,
   [], [],
   [                      H.261 (vic)], HAVE_H261,
   [            H.263/H.263+ (ffmpeg)], HAVE_H263,
   [                    VideoLAN x264], HAVE_X264,
   [                   Cisco OpenH264], HAVE_OPENH264,
   [                              VP8], HAVE_VP8,
   [                           THEORA], HAVE_THEORA,
   [                     MPEG4 Part 2], HAVE_MPEG4,
   [], [],
   [                     SpanDSP FAX ], HAVE_SPANDSP,
   [], [],
   [         Voicetronics VPB support], [HAVE_VPB]dnl
)
