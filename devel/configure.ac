AC_INIT(include/opal/manager.h)

AC_PROG_CC
AC_PROG_CXX

h323=yes
t38=yes
h224=yes
h450=yes
h460=yes
sip=yes
iax=yes
audio=yes
video=yes
rfc4175=yes

dnl this must be updated for every minor revision, and for
dnl builds when and as necessary
REQUIRED_PWLIB_MAJOR=1
REQUIRED_PWLIB_MINOR=11
REQUIRED_PWLIB_BUILD=0

dnl ########################################################################
dnl set the OPAL directory to the current directory

OPALDIR=`pwd`
AC_SUBST(OPALDIR)

dnl ########################################################################
dnl set the PREFIX accordingly
if test "x$prefix" = "xNONE"; then
   INSTALLPREFIX="/usr/local"
else
   INSTALLPREFIX="${prefix}"
fi

AC_SUBST(INSTALLPREFIX)

dnl ########################################################################
dnl set LIBDIR accordingly
LIBDIR="${libdir}"

AC_SUBST(LIBDIR)

dnl ########################################################################
dnl extract the OPAL version

dnl MSWIN_DISPLAY version,Version
dnl MSWIN_VERSION version,OPAL_MAJOR=MAJOR_VERSION
dnl MSWIN_VERSION version,OPAL_MINOR=MINOR_VERSION
dnl MSWIN_VERSION version,OPAL_BUILD=BUILD_NUMBER
dnl MSWIN_VERSION version,OPAL_VERSION=VERSION

MAJOR_VERSION=`cat ${OPALDIR}/version.h | grep MAJOR_VERSION | cut -f3 -d' '`
MINOR_VERSION=`cat ${OPALDIR}/version.h | grep MINOR_VERSION | cut -f3 -d' '`
BUILD_NUMBER=`cat ${OPALDIR}/version.h | grep BUILD_NUMBER | cut -f3 -d' '`
OPAL_VERSION="${MAJOR_VERSION}.${MINOR_VERSION}.${BUILD_NUMBER}"
BUILD_TYPE=`cat ${OPALDIR}/version.h | grep BUILD_TYPE | cut -f 3 -d ' ' | sed 's/BetaCode/-beta/' | sed 's/AlphaCode/-alpha/' | sed 's/ReleaseCode/\./'`

AC_DEFINE_UNQUOTED(OPAL_MAJOR,   ${MAJOR_VERSION})
AC_DEFINE_UNQUOTED(OPAL_MINOR,   ${MINOR_VERSION})
AC_DEFINE_UNQUOTED(OPAL_BUILD,   ${BUILD_NUMBER})
AC_DEFINE_UNQUOTED(OPAL_VERSION, "$OPAL_VERSION")

AC_MSG_NOTICE(Opal version is ${OPAL_VERSION});

#use to set the version in opal.pc
AC_SUBST(OPAL_VERSION)
AC_SUBST(BUILD_TYPE)

dnl ########################################################################
dnl look for ptlib, use a preference order of explicit PWLIBDIR, directory
dnl at same level, home directory, /usr/local or /usr.

if test "${PWLIBDIR:-unset}" != "unset" ; then
  AC_CHECK_FILE(${PWLIBDIR}/version.h, HAS_PTLIB=1)
fi
if test "${HAS_PTLIB:-unset}" = "unset" ; then
  AC_CHECK_FILE(${OPALDIR}/../pwlib/version.h, HAS_PTLIB=1)
  if test "${HAS_PTLIB:-unset}" != "unset" ; then
    PWLIBDIR="${OPALDIR}/../pwlib"
  else
    AC_CHECK_FILE(${HOME}/pwlib/include/ptlib.h, HAS_PTLIB=1)
    if test "${HAS_PTLIB:-unset}" != "unset" ; then
      PWLIBDIR="${HOME}/pwlib"
    else
      AC_CHECK_FILE(/usr/local/include/ptlib.h, HAS_PTLIB=1)
      if test "${HAS_PTLIB:-unset}" != "unset" ; then
        AC_PATH_PROG(PTLIB_CONFIG, ptlib-config, , /usr/local/bin)
      else
        AC_CHECK_FILE(/usr/include/ptlib.h, HAS_PTLIB=1)
        if test "${HAS_PTLIB:-unset}" != "unset" ; then
          AC_PATH_PROG(PTLIB_CONFIG, ptlib-config, , /usr/share/pwlib/make/)
        else
          AC_CHECK_FILE(${prefix}/include/ptlib.h, HAS_PTLIB=1)
          if test "${HAS_PTLIB:-unset}" != "unset" ; then
            AC_PATH_PROG(PTLIB_CONFIG, ptlib-config, , ${prefix}/bin)
          fi
        fi
      fi
    fi
  fi
fi

if test "${HAS_PTLIB:-unset}" = "unset" ; then
  AC_MSG_ERROR("Cannot find pwlib - please install or set PWLIBDIR and try again")
fi

if test "${PWLIBDIR:-unset}" = "unset" ; then
  if test "${PTLIB_CONFIG:-unset}" = "unset" ; then
    AC_MSG_ERROR("Cannot find ptlib-config - please install and try again")
  fi
  PWLIBDIR=`$PTLIB_CONFIG --prefix`
fi

if test "x$PWLIBDIR" = "x/usr" -o "x$PWLIBDIR" = "x/usr/"; then
  PWLIBDIR="/usr/share/pwlib/"
fi
if test "x$PWLIBDIR" = "x/usr/local" -o "x$PWLIBDIR" = "x/usr/"; then
  PWLIBDIR="/usr/local/share/pwlib/"
fi
if test "x$PWLIBDIR" = "x${prefix}"; then
  PWLIBDIR="${prefix}/share/pwlib/"
fi
AC_SUBST(PWLIBDIR)

echo "PWLib prefix set to.... $PWLIBDIR"

if test "${PWLIBDIR:-unset}" = "unset" ; then
  PWVERSION=`$PTLIB_CONFIG --version`
  PW_MAJOR_VERSION=`echo $PW_VERSION | cut -d'.' -f1`
  PW_MINOR_VERSION=`echo $PW_VERSION | cut -d'.' -f2`
  PW_BUILD_NUMBER=`echo $PW_VERSION | cut -d'.' -f3`
  
else
  PW_MAJOR_VERSION=`cat ${PWLIBDIR}/version.h | grep MAJOR_VERSION | cut -f3 -d' '`
  PW_MINOR_VERSION=`cat ${PWLIBDIR}/version.h | grep MINOR_VERSION | cut -f3 -d' '`
  PW_BUILD_NUMBER=`cat ${PWLIBDIR}/version.h | grep BUILD_NUMBER | cut -f3 -d' '`
  PWVERSION="${PW_MAJOR_VERSION}.${PW_MINOR_VERSION}.${PW_BUILD_NUMBER}"
fi

AC_ARG_ENABLE(versioncheck,
		[  --disable-versioncheck    disable PWLib version check],
		PWLIB_VERSION_CHECK=$enableval)

AC_MSG_CHECKING(Checking PWLib version ${PWVERSION})
if test "${PWLIB_VERSION_CHECK}x" = "nox" ; then
  AC_MSG_RESULT(check disabled)
else
  if test $PW_MAJOR_VERSION -lt $REQUIRED_PWLIB_MAJOR ; then
    AC_MSG_ERROR(Major version less than required $REQUIRED_PWLIB_MAJOR)
  fi
  if test $PW_MINOR_VERSION -lt $REQUIRED_PWLIB_MINOR ; then
    AC_MSG_ERROR(Minor version less than required $REQUIRED_PWLIB_MINOR)
  fi
  if test $PW_BUILD_NUMBER -lt $REQUIRED_PWLIB_BUILD ; then
    AC_MSG_ERROR(Build version less than required $REQUIRED_PWLIB_BUILD)
  fi
  AC_MSG_RESULT(ok)
fi

AC_SUBST(PWLIBDIR)

dnl ########################################################################
dnl get the gcc version

if test x$GXX = xyes ; then
  USE_GCC=yes
  gcc_version=`$CXX -dumpversion`
  AC_MSG_NOTICE(gcc version is $gcc_version);
  GXX_MAJOR=`echo $gcc_version | sed 's/\..*$//'`
  GXX_MINOR=[`echo $gcc_version | sed -e 's/[0-9][0-9]*\.//' -e 's/\..*$//'`]
  GXX_PATCH=[`echo $gcc_version | sed -e 's/[0-9][0-9]*\.[0-9][0-9]*\.//' -e 's/\..*$//'`]

  dnl cannot compile for less than gcc 3
  AC_MSG_CHECKING(checking if gcc version is valid)
  if test ${GXX_MAJOR} -lt 3 ; then
    AC_MSG_RESULT(no)
    AC_MSG_ERROR([OPAL requires gcc version 3 or later])
  else
    AC_MSG_RESULT(yes)
  fi
fi

dnl ########################################################################
dnl check for BSR instruction

AC_MSG_CHECKING(for working bit scan intrinsic)
BSR_TEST_SRC="
#include <stdlib.h>
int main(int argc, char* argv[]) {
#ifdef __GNUC__
	unsigned int val = 0x00000FF0;
	if (__builtin_clz(val) == 20)
		exit(0);
#endif
	exit(1);
}"
AC_TRY_RUN($BSR_TEST_SRC, GCC_HAS_CLZ=1, GCC_HAS_CLZ=0, GCC_HAS_CLZ=1)
if test "${GCC_HAS_CLZ}" = "1" ; then
	AC_MSG_RESULT(yes)
	AC_DEFINE(GCC_HAS_CLZ, 1)
	GCC_HAS_CLZ=1
else
	AC_MSG_RESULT(no)
fi
AC_SUBST(GCC_HAS_CLZ)


dnl ########################################################################
dnl Look for system libspeex
localspeex="xxx"
AC_ARG_ENABLE(localspeex,
       [  --enable-localspeex     use local version of Speex library rather than system version],
       localspeex=$enableval)

AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)

case 2 in
	$ac_cv_sizeof_short) SIZE16="short";;
	$ac_cv_sizeof_int) SIZE16="int";;
esac

case 4 in
	$ac_cv_sizeof_int) SIZE32="int";;
	$ac_cv_sizeof_long) SIZE32="long";;
	$ac_cv_sizeof_short) SIZE32="short";;
esac

AC_SUBST(SIZE16)
AC_SUBST(SIZE32)

OPAL_HAVE_SPEEX_H=
OPAL_HAVE_SPEEX_SPEEX_H=
AC_CHECK_HEADERS("speex.h", OPAL_HAVE_SPEEX_H=1)
if test "${OPAL_HAVE_SPEEX_H}x" != "x" ; then
  AC_DEFINE(OPAL_HAVE_SPEEX_H)
else
  AC_CHECK_HEADERS("speex/speex.h", OPAL_HAVE_SPEEX_SPEEX_H=1)
  if test "${OPAL_HAVE_SPEEX_SPEEX_H}x" != "x" ; then
    AC_DEFINE(OPAL_HAVE_SPEEX_SPEEX_H)
  fi
fi

if test "${localspeex}" = "yes" ; then
  AC_MSG_NOTICE(Forcing use of OPAL Speex sources)
elif test "${localspeex}" = "no" ; then
  AC_MSG_NOTICE(Forcing use of system Speex library)
  AC_DEFINE(OPAL_SYSTEM_SPEEX, 1)
  AC_SUBST(OPAL_SYSTEM_SPEEX, 1)
else
  AC_CHECK_LIB(speex, speex_encoder_destroy, SPEEX=1)
  if test "${SPEEX}x" != "x" ; then
    AC_CHECK_LIB(speex, speex_encode_int, SPEEX=1, unset SPEEX)
  fi
  if test "x$SPEEX" = "x" -o \( "${OPAL_HAVE_SPEEX_H}x" = "x" -a "${OPAL_HAVE_SPEEX_SPEEX_H}x" = "x" \); then
    AC_MSG_NOTICE(Forcing use of OPAL Speex library)
  else
    AC_MSG_CHECKING(system Speex version)
    if test "${OPAL_HAVE_SPEEX_H}x" != "x" ; then
      echo "#include <speex.h>" > t.c 
      echo "#include <speex_header.h>" >> t.c
    else
      echo "#include <speex/speex.h>" > t.c 
      echo "#include <speex/speex_header.h>" >> t.c
    fi
    cat >> t.c <<C_FILE
    #include <stdio.h>
    int main(int argc,char *argv[])
    {
      SpeexHeader header;
      speex_init_header(&header, 1, 1, &speex_nb_mode);
      printf("%s\n", header.speex_version);
    }
C_FILE
    cc -o t t.c -lspeex > /dev/null 2>&1
    if test \! -x t ; then
      AC_MSG_RESULT(cannot determine - using OPAL version)
    else
      SYSVER=`./t`
      AC_MSG_RESULT($SYSVER)
      AC_MSG_CHECKING(OPAL Speex version)
      LIBVER=`grep "#define SPEEX_VERSION" ./src/codec/speex/libspeex/misc.h | sed -e 's/^.*[Ss][Pp][Ee][Ee][Xx]\-//' -e 's/\"//' -e 's/\/.*//'`
      AC_MSG_RESULT($LIBVER)
      AC_MSG_CHECKING(Speex versions)
      cat > t.pl <<P_FILE
      [
      \$sysver = @ARGV[0];
      \$libver = @ARGV[1];
      @lib = split /\./, \$libver;
      while (@lib < 3) {
        @lib[0+@lib] = "0";
      }
      @sys = split /\./, \$sysver;
      while (@sys < 3) {
        @sys[0+@sys] = "0";
      }
      \$i = 0;
      while (\$i < 3) {
        if (@sys[\$i] < @lib[\$i]) {
          print "0";
          exit;
        }
        \$i++;
      }
      print "1";
      ]
P_FILE
      SPEEX=`perl t.pl $SYSVER $LIBVER`
      rm t.pl
      if test "x$SPEEX" = "x0" ; then
        AC_MSG_RESULT(OPAL version is more recent)
      else
        AC_MSG_RESULT(system version is more recent)
        AC_DEFINE(OPAL_SYSTEM_SPEEX, 1)
        AC_SUBST(OPAL_SYSTEM_SPEEX, 1)
        AC_MSG_CHECKING(Speex noise type)
        if test "${OPAL_HAVE_SPEEX_H}x" != "x" ; then
          echo "#include <speex.h>" > t.c
          echo "#include <speex_preprocess.h>" >> t.c
        else
          echo "#include <speex/speex.h>" > t.c
          echo "#include <speex/speex_preprocess.h>" >> t.c
        fi
        cat >> t.c <<C_FILE
          #include <stdio.h>
          int main(int argc,char *argv[])
	  {
	     struct SpeexPreprocessState *st;
  	     spx_int16_t *x;
	     float *echo;
	     speex_preprocess(st, x, echo);
	  }
C_FILE
        rm -f t
        cc -Werror -o t t.c -lspeex > /dev/null 2>&1
        if test \! -x t ; then
          AC_MSG_RESULT(spx_int32_t)
        else
	  AC_MSG_RESULT(float)
	  AC_DEFINE(OPAL_SPEEX_FLOAT_NOISE)
	fi
      fi
    fi
    rm -f t t.c
  fi
fi

AC_SUBST(OPAL_HAVE_SPEEX_SPEEX_H)

dnl ########################################################################
dnl enable audio, video, SIP, H.323 and IAX

dnl MSWIN_DISPLAY audio,Audio
dnl MSWIN_DEFINE  audio,OPAL_AUDIO

AC_ARG_ENABLE(audio,
       [  --disable-audio         disable audio codec support],
       audio=$enableval)
AC_MSG_CHECKING(audio codec support)
if test "$audio" = "yes" ; then
  AC_SUBST(OPAL_AUDIO, 1)
  AC_DEFINE(OPAL_AUDIO)
  AC_MSG_RESULT(enabled)
else
  AC_MSG_RESULT(disabled)
fi

dnl ########################################################################
dnl check for dynamic library code (dlopen and friends)

HAS_LIBDL=0
AC_CHECK_HEADERS([dlfcn.h], [HAS_DLFCN=yes], [HAS_DLFCN=no])
if test "$HAS_DLFCN" = yes ; then
  AC_MSG_CHECKING(if dlopen is available)
  AC_TRY_COMPILE([#include <dlfcn.h>],
                  [void * p = dlopen("lib", 0);], [HAS_DLOPEN=yes], [HAS_DLOPEN=no])
  if test "$HAS_DLOPEN" = no ; then
    AC_MSG_RESULT(no)
  else
    AC_MSG_RESULT(yes)
    AC_CHECK_LIB([dl],[dlopen],[HAS_LIBDL=1], [HAS_LIBDL=0])
  fi
fi
AC_SUBST(HAS_LIBDL)


dnl MSWIN_DISPLAY video,Video
dnl MSWIN_DEFINE  video,OPAL_VIDEO


AC_ARG_ENABLE(video,
       [  --disable-video         disable video codec support],
       video=$enableval)
AC_MSG_CHECKING(video device support)
if test "$video" = "yes" ; then
  AC_SUBST(OPAL_VIDEO, 1)
  AC_DEFINE(OPAL_VIDEO)
  AC_MSG_RESULT(enabled)
else
  AC_MSG_RESULT(disabled)
  rfc4175=no
fi


dnl MSWIN_DISPLAY sip,SIP support
dnl MSWIN_DEFINE  sip,OPAL_SIP

AC_ARG_ENABLE(sip,
       [  --disable-sip           disable SIP protocol support],
       sip=$enableval)
AC_MSG_CHECKING(SIP protocol)
if test "$sip" = "yes" ; then
  AC_SUBST(OPAL_SIP, 1)
  AC_DEFINE(OPAL_SIP)
  AC_MSG_RESULT(enabled)
else
  AC_MSG_RESULT(disabled)
fi


dnl MSWIN_DISPLAY h323,H.323 support
dnl MSWIN_DEFINE  h323,OPAL_H323

AC_ARG_ENABLE(h323,
       [  --disable-h323          disable H.323 protocol support],
       h323=$enableval)
AC_MSG_CHECKING(H.323 protocol)
if test "$h323" = "yes" ; then
  AC_SUBST(OPAL_H323, 1)
  AC_DEFINE(OPAL_H323)
  AC_MSG_RESULT(enabled)
else
  AC_SUBST(OPAL_H323, 0)
  AC_MSG_RESULT(disabled (T.38, H.224, H.460 also disabled))
  h224=no
  h460=no
fi
AC_SUBST(OPAL_H323, $OPAL_H323)

dnl ########################################################################
dnl
dnl IAX

dnl MSWIN_DISPLAY iax2,IAX2 support
dnl MSWIN_DEFINE  iax2,OPAL_IAX2

AC_ARG_ENABLE(iax,
       [  --disable-iax           disable IAX2 protocol support],
       iax=$enableval)
AC_MSG_CHECKING(IAX2 protocol)
if test "$iax" = "yes" ; then
  AC_SUBST(OPAL_IAX2, 1)
  AC_DEFINE(OPAL_IAX2)
  AC_MSG_RESULT(enabled)
else
  AC_MSG_RESULT(disabled)
fi

dnl ########################################################################
dnl
dnl H.224

dnl MSWIN_DISPLAY h224,H.224 support
dnl MSWIN_DEFINE  h224,OPAL_H224

AC_ARG_ENABLE(h224,
		[  --disable-h224          disable H.224 protocol support],
		h224=$enableval)
AC_MSG_CHECKING(H224 protocol)
if test "$h224" = "yes" ; then
  AC_SUBST(OPAL_H224, 1)
  AC_DEFINE(OPAL_H224)
  AC_MSG_RESULT(enabled)
else
  AC_MSG_RESULT(disabled)
fi

dnl ########################################################################
dnl
dnl T.38

dnl MSWIN_DISPLAY    t38,T.38
dnl MSWIN_DEFINE     t38,OPAL_T38FAX

if test "$t38" = "yes" ; then
  AC_ARG_ENABLE(t38,
         [  --disable-t38           disable T.38 protocol support],
         t38=$enableval)
  AC_MSG_CHECKING(T.38 protocol)
  if test "$t38" = "yes" ; then
    AC_SUBST(OPAL_T38FAX, 1)
    AC_DEFINE(OPAL_T38FAX)
    AC_MSG_RESULT(enabled)
  else
    AC_MSG_RESULT(disabled)
  fi
fi


dnl ########################################################################
dnl
dnl H.450

dnl MSWIN_DISPLAY    h450,H.450
dnl MSWIN_DEFINE     h450,OPAL_H450
dnl MSWIN_IF_FEATURE h450,h323

OPAL_450=
if test "$h450" = "yes" ; then
  AC_ARG_ENABLE(h450,
         [  --disable-h450          disable H.450],
         h450=$enableval)
  AC_MSG_CHECKING(H.450 support)
  if test "$h450" = "yes" ; then
    AC_SUBST(OPAL_H450, 1)
    AC_DEFINE(OPAL_H450)
    AC_MSG_RESULT(enabled)
  else
    AC_MSG_RESULT(disabled)
  fi
fi


dnl ########################################################################
dnl
dnl H.460

dnl MSWIN_DISPLAY    h460,H.460
dnl MSWIN_DEFINE     h460,OPAL_H460
dnl MSWIN_IF_FEATURE h460,h323

OPAL_460=
if test "$h460" = "yes" ; then
  AC_ARG_ENABLE(h460,
         [  --disable-h460          disable H.460],
         h460=$enableval)
  AC_MSG_CHECKING(H.460 support)
  if test "$h460" = "yes" ; then
    AC_SUBST(OPAL_H460, 1)
    AC_DEFINE(OPAL_H460)
    AC_MSG_RESULT(enabled)
  else
    AC_MSG_RESULT(disabled)
  fi
fi


dnl ########################################################################
dnl
dnl LID, used for vpb, dialogic etc ...

dnl MSWIN_DISPLAY lids,Line Interface Device
dnl MSWIN_DEFINE  lids,OPAL_LID

OPAL_LID=
lid=yes
if test "$lid" = "yes" ; then
  AC_ARG_ENABLE(lid,
         [  --disable-lid           disable LID support],
         lid=$enableval)
  AC_MSG_CHECKING(LID support)
  if test "$lid" = "yes" ; then
    AC_DEFINE(OPAL_LID, 1)
    OPAL_LID=1
    AC_MSG_RESULT(enabled)
  else
    AC_MSG_RESULT(disabled)
  fi
fi
AC_SUBST(OPAL_LID)

dnl ########################################################################
dnl
dnl IVR

dnl MSWIN_DISPLAY ivr,IVR support
dnl MSWIN_DEFINE  ivr,OPAL_IVR

ivr=yes
if test "$ivr" = "yes" ; then
  AC_ARG_ENABLE(ivr,
         [  --disable-ivr           disable IVR support],
         ivr=$enableval)
  AC_MSG_CHECKING(IVR support)
  if test "$ivr" = "yes" ; then
    AC_DEFINE(OPAL_IVR, 1)
    OPAL_IVR=1
    AC_MSG_RESULT(enabled)
  else
    AC_MSG_RESULT(disabled)
  fi
fi
AC_SUBST(OPAL_IVR)


dnl ########################################################################
dnl
dnl RFC4175

dnl MSWIN_DISPLAY rfc4175,RFC4175 Raw Video support
dnl MSWIN_DEFINE  rfc4175,OPAL_RFC4175

if test "$rfc4175" = "yes" ; then
  AC_ARG_ENABLE(rfc4175,
         [  --disable-rfc4175       disable RFC4175 support],
         rfc4175=$enableval)
  AC_MSG_CHECKING(RFC4175 codec)
  if test "$rfc4175" = "yes" ; then
    AC_DEFINE(OPAL_RFC4175, 1)
    OPAL_RFC4175=1
    AC_MSG_RESULT(enabled)
  else
    AC_MSG_RESULT(disabled)
  fi
fi
AC_SUBST(OPAL_RFC4175)

dnl ########################################################################
dnl
dnl ZRTP from Phil Zimmermann
dnl 

dnl MSWIN_DISPLAY     libzrtp,ZRTP/SRTP support
dnl MSWIN_CHECK_FILE  libzrtp,include\zrtp_types.h,HAS_LIBZRTP=1
dnl MSWIN_DIR_SYMBOL  libzrtp,LIBZRTP_DIR
dnl MSWIN_CHECK_DIR   libsrtp,.\srtp_win32k
dnl MSWIN_CHECK_DIR   libzrtp,.\zrtp
dnl MSWIN_CHECK_DIR   libzrtp,..\external\zrtp
dnl MSWIN_CHECK_DIR   libzrtp,c:\libs

HAS_LIBZRTP=
dnl AC_MSG_CHECKING(for libZRTP)
dnl AC_TRY_COMPILE([#include "srtp/srtp.h"],
dnl  [int main(int argc, char *argv[]) { crypto_policy_t p; p.cipher_key_len = SRTP_MASTER_KEY_LEN; return 0; }],
dnl  HAS_LIBSRTP=1)
dnl if test "${HAS_LIBSRTP}" = "1" ; then
dnl   AC_MSG_RESULT(yes)
dnl   AC_DEFINE(HAS_LIBSRTP, 1)
dnl   HAS_LIBSRTP=1
dnl   OPAL_SRTP=1
dnl else
dnl   AC_MSG_RESULT(no)
dnl fi
AC_SUBST(OPAL_ZRTP)
AC_SUBST(HAS_LIBZRTP)


dnl ########################################################################
dnl
dnl Cisco libSRTP
dnl 

dnl MSWIN_DISPLAY     libsrtp,Cisco libSRTP support
dnl MSWIN_IF_FEATURE  libsrtp,!libzrtp
dnl MSWIN_CHECK_FILE  libsrtp,include\srtp.h,HAS_LIBSRTP=1
dnl MSWIN_DIR_SYMBOL  libsrtp,LIBSRTP_DIR
dnl MSWIN_CHECK_DIR   libsrtp,.\srtp
dnl MSWIN_CHECK_DIR   libsrtp,..\external\srtp
dnl MSWIN_CHECK_DIR   libsrtp,c:\libs

HAS_LIBSRTP=
AC_MSG_CHECKING(for libSRTP)
AC_TRY_COMPILE([#include "srtp/srtp.h"],
 [int main(int argc, char *argv[]) { crypto_policy_t p; p.cipher_key_len = SRTP_MASTER_KEY_LEN; return 0; }],
 HAS_LIBSRTP=1)
if test "${HAS_LIBSRTP}" = "1" ; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAS_LIBSRTP, 1)
  HAS_LIBSRTP=1
  OPAL_SRTP=1
else
  AC_MSG_RESULT(no)
fi
AC_SUBST(OPAL_SRTP)
AC_SUBST(HAS_LIBSRTP)
dnl ########################################################################
dnl output make directives

AC_SUBST(STDCCFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(ENDLDLIBS)
AC_PROG_INSTALL

dnl ########################################################################
dnl Output configured files

AC_CONFIG_SUBDIRS(plugins)

dnl ########################################################################
dnl output header file

AC_CONFIG_FILES(opal_inc.mak)
AC_CONFIG_FILES(Makefile)
AC_CONFIG_HEADERS(include/opal/buildopts.h)
AC_CONFIG_FILES(opal.pc)
AC_CONFIG_FILES(opal.spec)
AC_CONFIG_FILES(opal_cfg.dxy)

AC_OUTPUT(src/codec/speex/libspeex/speex_config_types.h)
