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

if test "x$OPALDIR" != "x" ; then
   VERSION_DIR="$OPALDIR"
else
   VERSION_DIR=".."
fi

MY_VERSION_FILE([$VERSION_DIR/version.h], [OPAL])


dnl ########################################################################
dnl Directories

OPAL_DETERMINE_PLUGIN_DIR()

AC_SUBST(AUD_PLUGIN_DIR, "${PLUGIN_DIR}/codecs/audio")
AC_SUBST(VID_PLUGIN_DIR, "${PLUGIN_DIR}/codecs/video")
AC_SUBST(LID_PLUGIN_DIR, "${PLUGIN_DIR}/lid")
AC_SUBST(FAX_PLUGIN_DIR, "${PLUGIN_DIR}/fax")

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
OPAL_SIMPLE_PLUGIN([G7221],    [g.722.1],  [G.722.1 wideband audio codec],          [$PLUGIN_SRC_DIR/audio/G.722.1])
OPAL_SIMPLE_PLUGIN([G7222],    [g.722.2],  [G.722.2 wideband audio codec],          [$PLUGIN_SRC_DIR/audio/G.722.2])
OPAL_SIMPLE_PLUGIN([LPC10],    [lpc10],    [LPC-10 very low bandwidth audio codec], [$PLUGIN_SRC_DIR/audio/LPC_10])

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
   [#include <iLBC/iLBC_encode.h>],
   [
      struct iLBC_Enc_Inst_t_ * context;
      initEncode(context, 30);
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
   [speex_encoder_init(&speex_wb_mode)]
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


dnl ########################################################################
dnl Video Codecs
dnl ########################################################################

OPAL_SIMPLE_PLUGIN([H261], [h.261], [H.261 video codec], [$PLUGIN_SRC_DIR/video/H.261-vic])

MY_MODULE_OPTION(
   [LIBAVCODEC],
   [libavcodec],
   [FFMPEG libavcodec library],
   [libavcodec >= 51.11.0],
   [],
   [-lavcodec],
   [#include <libavcodec/avcodec.h>],
   [avcodec_version()],
   [HAVE_LIBAVCODEC=yes],
   [HAVE_LIBAVCODEC=no]
)

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
      [$PLUGIN_SRC_DIR/video/H.264],
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
      ],
      [
         x264_param_t param;
         x264_encoder_open(&param);
      ],
      [
         AC_ARG_ENABLE([x264-licensed], [AC_HELP_STRING([--enable-x264-licensed],[enable commercially licensed x264])])
         if test "$enableval" = "yes" ;  then
            X264_CFLAGS="$X264_CFLAGS -DX264_LICENSED"
            HAVE_H264="yes (Licensed x264)"
         else
            HAVE_H264="yes (GPL)"
         fi
      ]
   )
else
   HAVE_H263="no (missing FFMPEG)"
   HAVE_H264="no (missing FFMPEG)"
   HAVE_MPEG4="no (missing FFMPEG)"
fi


OPAL_SYSTEM_PLUGIN(
   [VP8],
   [VP8],
   [VP8 (WebM) video codec],
   [$PLUGIN_SRC_DIR/video/VP8-WebM],
   [vpx],
   [],
   [-lvpx],
   [#include <vpx/vpx_encoder.h>],
   [
      vpx_codec_ctx_t codec;
      vpx_codec_destroy(&codec);
   ]
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
   [#include <theora/theora.h>],
   [
      theora_info info;
      theora_info_init(&info);
   ]
)

dnl ########################################################################
dnl Various checks
dnl ########################################################################

MY_COMPILE_IFELSE(
   [for posix_memalign],
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
echo ""
echo "========================= Plugin configuration ========================="
echo "                         OS Type  :  $target_os"
echo "                    Machine Type  :  $target_cpu"
echo "                          prefix  : $prefix"
echo "                     exec_prefix  : $exec_prefix"
echo "                          libdir  : $libdir"
echo "  Audio Codec plugin install dir  : "'${libdir}'"/$AUD_PLUGIN_DIR"
echo "  Video Codec plugin install dir  : "'${libdir}'"/$VID_PLUGIN_DIR"
echo "       LID plugin install prefix  : "'${libdir}'"/$LID_PLUGIN_DIR"
echo "       FAX plugin install prefix  : "'${libdir}'"/$FAX_PLUGIN_DIR"

echo ""

echo "                              GSM :  ${HAVE_GSM}"
echo "                          GSM-AMR :  ${HAVE_GSMAMR}"
echo "                             iLBC :  ${HAVE_ILBC}"
echo "                            Speex :  ${HAVE_SPEEX}"
echo "                            G.722 :  ${HAVE_G722}"
echo "                          G.722.1 :  ${HAVE_G7221}"
echo "                          G.722.2 :  ${HAVE_G7222}"
echo "                            G.726 :  ${HAVE_G726}"
echo "                            LPC10 :  ${HAVE_LPC10}"
echo "                        IMA_ADPCM :  ${HAVE_IMA_ADPCM}"
echo "                              SBC :  ${HAVE_SBC}"
echo "                             CELT :  ${HAVE_CELT}"
echo "                             SILK :  ${HAVE_SILK}"

echo ""

echo "                           H.261  :  ${HAVE_H261} (internal)"
echo "                     H.263/H.263+ :  ${HAVE_H263} (ffmpeg)"
echo "                           H.264  :  ${HAVE_X264}"
echo "                             VP8  :  ${HAVE_VP8}"
echo "                          THEORA  :  ${HAVE_THEORA}"
echo "                    MPEG4 Part 2  :  ${HAVE_MPEG4}"

echo ""
echo "                     SpanDSP FAX  :  $HAVE_SPANDSP"

echo ""

echo "         Voicetronics VPB support :  $HAVE_VPB"

echo ""
echo "========================================================================"
