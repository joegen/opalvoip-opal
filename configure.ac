dnl
dnl OPAL configuration
dnl

AC_INIT([OPAL], m4_esyscmd([sed -n \
                            -e "s/ *#define *MAJOR_VERSION *\\(.*\\)/\\1./p" \
                            -e "s/ *#define *MINOR_VERSION *\\(.*\\)/\\1/p" \
                            -e "s/ *#define *BUILD_TYPE *AlphaCode/alpha/p" \
                            -e "s/ *#define *BUILD_TYPE *BetaCode/beta/p" \
                            -e "s/ *#define *BUILD_TYPE *ReleaseCode/./p" \
                            -e "s/ *#define *BUILD_NUMBER *\\(.*\\)/\\1/p" \
                            version.h | tr -d '\n']))

AC_PREREQ([2.65])

AC_CONFIG_AUX_DIR(make)

AC_CONFIG_SRCDIR([include/opal.h])

m4_include(make/my_macros.m4)
m4_include(make/opal.m4)


dnl ########################################################################
dnl Defaults
dnl ########################################################################

OPAL_H323=yes
OPAL_SIP=yes
OPAL_IAX2=yes
OPAL_VIDEO=yes
OPAL_T38_CAPABILITY=yes
OPAL_FAX=yes
OPAL_HAS_H224=yes
OPAL_HAS_H281=yes
OPAL_H450=yes
OPAL_H460=yes
OPAL_H460_NAT=yes
OPAL_H239=yes
OPAL_H501=yes
OPAL_T120DATA=no
OPAL_LID=yes
OPAL_CAPI=yes
OPAL_STATISTICS=yes
OPAL_RTCP_XR=yes
OPAL_AEC=yes
OPAL_IVR=yes
OPAL_RFC4175=yes
OPAL_RFC2435=no
OPAL_G711PLC=yes
OPAL_PLUGINS=yes
OPAL_SAMPLES=no
OPAL_ZRTP=no
OPAL_JAVA=yes
OPAL_RUBY=yes
OPAL_SCRIPT=yes
OPAL_HAS_MSRP=yes
OPAL_HAS_SIPIM=yes
OPAL_HAS_RFC4103=yes
OPAL_HAS_MIXER=yes
OPAL_HAS_PCSS=yes

# experimental options
OPAL_DAHDI=no


dnl ########################################################################
dnl Check for non-installed "environment variable" mode
if test "x$OPALDIR" != "x" ; then
   AC_MSG_NOTICE(OPALDIR environment variable detected as $OPALDIR)
   ac_default_prefix=$OPALDIR
   if test "x$prefix" = "xNONE" ; then
      libdir+=_$target
   fi
else
   OPALDIR=`pwd`
fi

AC_SUBST(OPALDIR)


dnl ########################################################################
dnl Extract OPAL version number

dnl MSWIN_DISPLAY version,Version
dnl MSWIN_VERSION version,OPAL_MAJOR=MAJOR_VERSION
dnl MSWIN_VERSION version,OPAL_MINOR=MINOR_VERSION
dnl MSWIN_VERSION version,OPAL_BUILD=BUILD_NUMBER
dnl MSWIN_VERSION version,OPAL_VERSION=VERSION

MY_VERSION_FILE([${OPALDIR}/version.h], [OPAL])


dnl ################################
dnl Special options

AC_RUN_IFELSE(
   [AC_LANG_PROGRAM(
      [#include <stdlib.h>],
      [
         unsigned int val = 0x00000FF0;
         if (__builtin_clz(val) == 20)
            return 0;
         return 1;
      ]
   )],
   [AC_DEFINE([GCC_HAS_CLZ], [1], [bit scan intrinsic])]
)


dnl ########################################################################
dnl Check for PTLib

PKG_CHECK_MODULES(
   [PTLIB],
   [ptlib >= 2.13.0],
   [MY_ADD_MODULE_FLAGS([PTLIB])],
   [AC_MSG_ERROR([No PTLib or old version!])]
)

OPAL_CHECK_PTLIB_OPTION([URL],          [PTLIB_URL],  [Must have URL functionality enabled in PTLib])
OPAL_CHECK_PTLIB_OPTION([HTTP],         [PTLIB_HTTP], [Must have HTTP functionality enabled in PTLib])
OPAL_CHECK_PTLIB_OPTION([STUN],         [PTLIB_STUN])
OPAL_CHECK_PTLIB_OPTION([SSL/TLS],      [PTLIB_SSL])
OPAL_CHECK_PTLIB_OPTION([SSL/AES],      [PTLIB_SSL_AES])
OPAL_CHECK_PTLIB_OPTION([ASN.1],        [PTLIB_ASN])
OPAL_CHECK_PTLIB_OPTION([Expat],        [PTLIB_EXPAT])
OPAL_CHECK_PTLIB_OPTION([video],        [PTLIB_VIDEO])
OPAL_CHECK_PTLIB_OPTION([WAV file],     [PTLIB_WAVFILE])
OPAL_CHECK_PTLIB_OPTION([VXML],         [PTLIB_VXML])
OPAL_CHECK_PTLIB_OPTION([IP v6],        [PTLIB_IPV6])
OPAL_CHECK_PTLIB_OPTION([DTMF],         [PTLIB_DTMF])
OPAL_CHECK_PTLIB_OPTION([LDAP],         [PTLIB_LDAP])
OPAL_CHECK_PTLIB_OPTION([DNS resolver], [PTLIB_DNS_RESOLVER])
OPAL_CHECK_PTLIB_OPTION([audio],        [PTLIB_AUDIO])
OPAL_CHECK_PTLIB_OPTION([config],       [PTLIB_CONFIG_FILE])
OPAL_CHECK_PTLIB_OPTION([CLI],          [PTLIB_CLI])


dnl ########################################################################
dnl SRTP
dnl MSWIN_DISPLAY libsrtp,Cisco libSRTP support
dnl MSWIN_DEFINE  libsrtp,OPAL_SRTP=2

OPAL_MODULE_OPTION(
   [SRTP],
   [srtp],
   [SRTP protocol support],
   [libsrtp],
   [],
   [-lsrtp],
   [#include <srtp/srtp.h>],
   [
      crypto_policy_t p;
      p.cipher_key_len = SRTP_MASTER_KEY_LEN;
   ]
)


dnl ########################################################################
dnl ZRTP

OPAL_MODULE_OPTION(
   [ZRTP],
   [zrtp],
   [ZRTP protocol support],
   [libzrtp],
   [],
   [-lzrtpi -lbn],
   [#include <zrtp/zrtp.h>],
   [;]
)


dnl ########################################################################
dnl Speex

MY_MODULE_OPTION(
   [SPEEXDSP],
   [speexdsp],
   [Speex DSP support],
   [speexdsp local-source],
   [],
   [-lspeexdsp],
   [#include <speex/speex_echo.h>],
   [speex_echo_state_init(0, 0)],
   [
      AC_DEFINE(OPAL_SYSTEM_SPEEX, [1], [Has system speex dsp library])
      AC_DEFINE(OPAL_HAVE_SPEEX_SPEEX_H, [1])
      MY_LINK_IFELSE(
         [for Speex noise reduction],
         [$SPEEXDSP_CFLAGS],
         [$SPEEXDSP_LDFLAGS],
         [
            #include <speex/speex.h>
            #include <speex/speex_preprocess.h>
         ],
         [
            SpeexEchoState *st;
            spx_int16_t *x;
            float *echo;
            speex_echo_cancel(st, x, x, x, echo);
         ],
         [AC_DEFINE([OPAL_SPEEX_FLOAT_NOISE], [1])]
      )
   ]
)


dnl ########################################################################
dnl CAPI ISDN

dnl MSWIN_DISPLAY capi,CAPI (isdn) Line Interface Device
dnl MSWIN_DEFINE  capi,OPAL_CAPI

OPAL_MODULE_OPTION(
   [CAPI],
   [capi],
   [CAPI ISDN support],
   [capi20],
   [],
   [-lcapi],
   [#include <capi20.h>],
   [capi20_release(0)]
)


dnl ########################################################################
dnl See if DAHDI interface is available

OPAL_MODULE_OPTION(
   [DAHDI],
   [dahdi],
   [DAHDI E1 support],
   [dahdi],
   [],
   [-ltonezone],
   [#include <dahdi/user.h>],
   [;]
)


dnl ########################################################################
dnl We should use SWIG if it is there
AC_PATH_PROG(SWIG, swig)


dnl ########################################################################
dnl Java JNI Support
dnl MSWIN_DISPLAY     java,Java interface
dnl MSWIN_CHECK_FILE  java,include\jni.h,OPAL_JAVA=1
dnl MSWIN_DIR_SYMBOL  java,JAVA_DIR
dnl MSWIN_CHECK_DIR   java,%JAVA_HOME%\
dnl MSWIN_CHECK_DIR   java,%ProgramFiles%\java\jdk*
dnl MSWIN_CHECK_DIR   java,%ProgramW6432%\java\jdk*

OPAL_MODULE_OPTION(
   [JAVA],
   [java],
   [Java JNI support],
   [openjdk],
   [],
   [-ljdk],
   [#include <jni.h>],
   [JNIEnv * env]
)


dnl ########################################################################
dnl Ruby Support
dnl MSWIN_DISPLAY    ruby,Ruby interface
dnl MSWIN_DEFAULT    ruby,Disabled
dnl MSWIN_CHECK_FILE ruby,ruby.h,OPAL_RUBY=1
dnl MSWIN_DIR_SYMBOL ruby,RUBY_DIR
dnl MSWIN_CHECK_DIR  ruby,%ProgramFiles%\Ruby\include\*
dnl MSWIN_CHECK_DIR  ruby,%ProgramW6432%\Ruby\include\*
dnl MSWIN_CHECK_DIR  ruby,c:\Ruby\include\*
dnl MSWIN_CHECK_DIR  ruby,c:\tools\Ruby\include\*

OPAL_MODULE_OPTION(
   [RUBY],
   [ruby],
   [Ruby support],
   [ruby],
   [`ruby -e 'puts $:.join(" -I")'`],
   [-lruby],
   [#include <ruby.h>],
   [JNIEnv * env]
)


dnl ########################################################################
dnl Check for optional features

dnl MSWIN_DISPLAY  video,Video
dnl MSWIN_DEFINE   video,OPAL_VIDEO
OPAL_SIMPLE_OPTION(video,OPAL_VIDEO, [video support], [OPAL_PTLIB_VIDEO])

dnl MSWIN_DISPLAY  sip,SIP support
dnl MSWIN_DEFINE   sip,OPAL_SIP
OPAL_SIMPLE_OPTION(sip,OPAL_SIP, [SIP protocol support])

dnl MSWIN_DISPLAY  h323,H.323 support
dnl MSWIN_DEFINE   h323,OPAL_H323
OPAL_SIMPLE_OPTION(h323,OPAL_H323, [H.323 protocol support], [OPAL_PTLIB_ASN])

dnl MSWIN_DISPLAY  iax2,IAX2 support
dnl MSWIN_DEFINE   iax2,OPAL_IAX2
OPAL_SIMPLE_OPTION(iax2,OPAL_IAX2, [IAX2 protocol support])

dnl MSWIN_DISPLAY  h224,H.224 support
dnl MSWIN_DEFINE   h224,OPAL_HAS_H224
OPAL_SIMPLE_OPTION(h224,OPAL_HAS_H224, [H.224 protocol support], [OPAL_H323])

dnl MSWIN_DISPLAY  h281,H.281 (Far End Camera Control) support
dnl MSWIN_DEFINE   h281,OPAL_HAS_H281
OPAL_SIMPLE_OPTION(h281,OPAL_HAS_H281, [H.281 (Far End Camera Control) protocol support], [OPAL_HAS_H224])

dnl MSWIN_DISPLAY  t38,T.38 capability
dnl MSWIN_DEFINE   t38,OPAL_T38_CAPABILITY
OPAL_SIMPLE_OPTION(t38,OPAL_T38_CAPABILITY, [T.38 capability support])

dnl MSWIN_DISPLAY  msrp,MSRP capability
dnl MSWIN_DEFINE   msrp,OPAL_HAS_MSRP
OPAL_SIMPLE_OPTION(msrp,OPAL_HAS_MSRP, [MSRP support])

dnl MSWIN_DISPLAY  sipim,SIPIM capability
dnl MSWIN_DEFINE   sipim,OPAL_HAS_SIPIM
OPAL_SIMPLE_OPTION(sipim,OPAL_HAS_SIPIM, [SIPIM session support])

dnl MSWIN_DISPLAY  rfc4103,RFC4103 capability
dnl MSWIN_DEFINE   rfc4103,OPAL_HAS_RFC4103
OPAL_SIMPLE_OPTION(rfc4103,OPAL_HAS_RFC4103, [RFC4103 support])

dnl MSWIN_DISPLAY  statistics,Statistics gathering support
dnl MSWIN_DEFINE   statistics,OPAL_STATISTICS
OPAL_SIMPLE_OPTION(statistics,OPAL_STATISTICS, [statistics gathering support])

dnl MSWIN_DISPLAY    fax,T.38 FAX protocol
dnl MSWIN_IF_FEATURE fax,t38
dnl MSWIN_DEFINE     fax,OPAL_FAX
OPAL_SIMPLE_OPTION(  fax,OPAL_FAX, [T.38 FAX protocol support],[OPAL_T38_CAPABILITY],[OPAL_PTLIB_ASN],[OPAL_STATISTICS])

dnl MSWIN_DISPLAY    h450,H.450
dnl MSWIN_IF_FEATURE h450,h323
dnl MSWIN_DEFINE     h450,OPAL_H450
OPAL_SIMPLE_OPTION(  h450,OPAL_H450, [H.450], [OPAL_H323])

dnl MSWIN_DISPLAY    h460,H.460
dnl MSWIN_IF_FEATURE h460,h323
dnl MSWIN_DEFINE     h460,OPAL_H460
OPAL_SIMPLE_OPTION(  h460,OPAL_H460, [H.460], [OPAL_H323])

dnl MSWIN_DISPLAY    h460nat,H.460 NAT traversal
dnl MSWIN_IF_FEATURE h460nat,h323
dnl MSWIN_DEFINE     h460nat,OPAL_H460_NAT
OPAL_SIMPLE_OPTION(  h460nat,OPAL_H460_NAT, [H.460.24], [OPAL_H323])

dnl MSWIN_DISPLAY    h239,H.239
dnl MSWIN_IF_FEATURE h239,h323
dnl MSWIN_DEFINE     h239,OPAL_H239
OPAL_SIMPLE_OPTION(  h239,OPAL_H239, [H.239], [OPAL_H323])

dnl MSWIN_DISPLAY    h501,H.501
dnl MSWIN_IF_FEATURE h501,h323
dnl MSWIN_DEFINE     h501,OPAL_H501
OPAL_SIMPLE_OPTION(  h501,OPAL_H501, [H.501], [OPAL_H323])

dnl T.120 is disabled
dnl _MSWIN_DISPLAY    t120,T.120
dnl _MSWIN_IF_FEATURE t120,h323
dnl _MSWIN_DEFINE     t120,OPAL_T120DATA
OPAL_SIMPLE_OPTION(   t120,OPAL_T120DATA, [T.120], [OPAL_H323])

dnl MSWIN_DISPLAY  lid,Line Interface Device
dnl MSWIN_DEFINE   lid,OPAL_LID
OPAL_SIMPLE_OPTION(lid,OPAL_LID, [LID support])

dnl MSWIN_DISPLAY  ivr,IVR support
dnl MSWIN_DEFINE   ivr,OPAL_IVR
OPAL_SIMPLE_OPTION(ivr,OPAL_IVR, [IVR support], [OPAL_PTLIB_EXPAT], [OPAL_PTLIB_VXML])

dnl MSWIN_DISPLAY  rfc4175,RFC4175 Raw Video support
dnl MSWIN_DEFINE   rfc4175,OPAL_RFC4175
OPAL_SIMPLE_OPTION(rfc4175,OPAL_RFC4175, [RFC4175 support], [OPAL_VIDEO])

dnl MSWIN_DISPLAY    rfc2435,RFC2435 JPEG Video support (experimental)
dnl MSWIN_DEFAULT    rfc2435,Disabled
dnl MSWIN_CHECK_FILE rfc2435,jpeglib.h,OPAL_RFC2435=1
dnl MSWIN_DIR_SYMBOL rfc2435,JPEGLIB_DIR
dnl MSWIN_CHECK_DIR  rfc2435,%ProgramFiles%\GnuWin32\include\
dnl MSWIN_CHECK_DIR  rfc2435,%ProgramW6432%\GnuWin32\include\
OPAL_SIMPLE_OPTION(rfc2435,OPAL_RFC2435, [RFC2435 support (experimental)], [OPAL_VIDEO])

dnl MSWIN_DISPLAY  aec,Accoustic Echo Cancellation
dnl MSWIN_DEFINE   aec,OPAL_AEC
OPAL_SIMPLE_OPTION(aec,OPAL_AEC, [accoustic echo cancellation support])

dnl MSWIN_DISPLAY  g711plc,Packet Loss Concealment for G.711
dnl MSWIN_DEFINE   g711plc,OPAL_G711PLC
OPAL_SIMPLE_OPTION(g711plc,OPAL_G711PLC, [Packet Loss Concealment for G.711])

dnl MSWIN_DISPLAY  rtcpxr,RTCP Extended Reports support
dnl MSWIN_DEFINE   rtcpxr,OPAL_RTCP_XR
OPAL_SIMPLE_OPTION(rtcpxr,OPAL_RTCP_XR, [RTCP Extended Reports support])

dnl MSWIN_DISPLAY  mixer,Media mixing
dnl MSWIN_DEFINE   mixer,OPAL_HAS_MIXER
OPAL_SIMPLE_OPTION(mixer,OPAL_HAS_MIXER, [media mixing support])

dnl MSWIN_DISPLAY  pcss,PC Sound System Endpoint
dnl MSWIN_DEFINE   pcss,OPAL_HAS_PCSS
OPAL_SIMPLE_OPTION(pcss,OPAL_HAS_PCSS, [PC sound system support])

dnl MSWIN_DISPLAY  script,Script language interface
dnl MSWIN_DEFINE   script,OPAL_SCRIPT
OPAL_SIMPLE_OPTION(script,OPAL_SCRIPT, [script language support])


dnl ########################################################################
dnl Compile plugins and sample directories
dnl ########################################################################

OPAL_SIMPLE_OPTION([samples],[OPAL_SAMPLES], [enable samples build])

OPAL_SIMPLE_OPTION([plugins],[OPAL_PLUGINS], [plugin support])
if test "x$OPAL_PLUGINS" = "xyes" ; then
  AC_CONFIG_SUBDIRS(plugins)
fi

dnl Construct the plugins install path

OPAL_DETERMINE_PLUGIN_DIR()


dnl ########################################################################
dnl Output files
dnl ########################################################################

cfg_prefix=$libdir
if test "$exec_prefix" = "NONE" ; then
  cfg_prefix=./include
fi

AC_CONFIG_FILES(make/opal_defs.mak)
AC_CONFIG_HEADERS(${cfg_prefix}/opal/buildopts.h:include/opal/buildopts.h.in)
AC_CONFIG_FILES(opal.pc)
AC_CONFIG_FILES(opal.spec)
AC_CONFIG_FILES(opal_cfg.dxy)
AC_CONFIG_FILES(samples/openphone/version.wxs)

AC_OUTPUT
