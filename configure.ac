AC_INIT(include/opal/manager.h)

AC_PROG_CC
AC_PROG_CXX

dnl this must be updated for every minor revision, and for
dnl builds when and as necessary
REQUIRED_PWLIB_MAJOR=1
REQUIRED_PWLIB_MINOR=10
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
MAJOR_VERSION=`cat ${OPALDIR}/version.h | grep MAJOR_VERSION | cut -f3 -d' '`
MINOR_VERSION=`cat ${OPALDIR}/version.h | grep MINOR_VERSION | cut -f3 -d' '`
BUILD_NUMBER=`cat ${OPALDIR}/version.h | grep BUILD_NUMBER | cut -f3 -d' '`
OPAL_VERSION="${MAJOR_VERSION}.${MINOR_VERSION}.${BUILD_NUMBER}"
AC_DEFINE_UNQUOTED(OPAL_MAJOR,   ${MAJOR_VERSION})
AC_DEFINE_UNQUOTED(OPAL_MINOR,   ${MINOR_VERSION})
AC_DEFINE_UNQUOTED(OPAL_BUILD,   ${BUILD_NUMBER})
AC_DEFINE_UNQUOTED(OPAL_VERSION, "$OPAL_VERSION")


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
        fi
      fi
    fi
  fi
fi

if test "${HAS_PTLIB:-unset}" = "unset" ; then
  echo "Cannot find pwlib - please install or set PWLIBDIR and try again"
  exit
fi

if test "${PWLIBDIR:-unset}" = "unset" ; then
  if test "${PTLIB_CONFIG:-unset}" = "unset" ; then
    echo "Cannot find ptlib-config - please install and try again"
    exit
  fi
  PWLIBDIR=`$PTLIB_CONFIG --prefix`
fi

if test "x$PWLIBDIR" = "x/usr" -o "x$PWLIBDIR" = "x/usr/"; then
  PWLIBDIR="/usr/share/pwlib/"
fi
if test "x$PWLIBDIR" = "x/usr/local" -o "x$PWLIBDIR" = "x/usr/"; then
  PWLIBDIR="/usr/local/share/pwlib/"
fi

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
    AC_MSG_RESULT(Major version less than required $REQUIRED_PWLIB_MAJOR)
    exit 1
  fi
  if test $PW_MINOR_VERSION -lt $REQUIRED_PWLIB_MINOR ; then
    AC_MSG_RESULT(Minor version less than required $REQUIRED_PWLIB_MINOR)
    exit 1
  fi
  if test $PW_BUILD_NUMBER -lt $REQUIRED_PWLIB_BUILD ; then
    AC_MSG_RESULT(Build version less than required $REQUIRED_PWLIB_BUILD)
    exit 1
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
dnl Look for H263 codec
AC_ARG_ENABLE(h263avcodec,
		[  --enable-h263avcodec    specify RFC 2190 compatible avcodec
		headers],
		RFC2190_AVCODEC_DIR=$enableval)

if test "${RFC2190_AVCODEC_DIR}x" = "x" ; then
	AC_MSG_NOTICE(Skipping tests for RFC 2190 H.263 support)
else
	AC_MSG_NOTICE(Searching for standard RFC2190 compliant H.263 codec)

	dnl ################################################################
	dnl look for avcodec compatible with RFC 2190

	dnl MSWIN_DISPLAY    h263avcodec,ffmpeg RFC2190 Library
	dnl MSWIN_CHECK_FILE h263avcodec,avcodec_rfc2190.h,RFC2190_AVCODEC=1
	dnl MSWIN_DIR_SYMBOL h263avcodec,RFC2190_AVCODEC_DIR
	dnl MSWIN_CHECK_DIR  h263avcodec,..\ffmpeg\libavcodec\
	dnl MSWIN_CHECK_DIR  h263avcodec,..\external\ffmpeg\libavcodec\
	dnl MSWIN_CHECK_DIR  h263avcodec,\ffmpeg\libavcodec\
	dnl MSWIN_CHECK_DIR  h263avcodec,c:\ffmpeg\libavcodec\

	AC_CHECK_HEADER(${RFC2190_AVCODEC_DIR}/avcodec.h, RFC2190_AVCODEC=1)
	if test "${RFC2190_AVCODEC}" == "1" ; then
		AC_SUBST(RFC2190_AVCODEC, 1)
		AC_DEFINE(RFC2190_AVCODEC, 1, "FFMPEG standard, RFC2190 compliant H.263 codec found")
		AC_SUBST(RFC2190_AVCODEC_DIR, ${RFC2190_AVCODEC_DIR})
		STDCCFLAGS="${STDCCFLAGS} -I${RFC2190_AVCODEC_DIR}"
	fi
	AC_SUBST(RFC2190_AVCODEC)
fi


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
      AC_MSG_CHECKING(library Speex version)
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
          die;
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
dnl
dnl Allow iLBC to be disabled for Debian compatibility
dnl
dnl MSWIN_DISPLAY ilbc,iLBC support
dnl MSWIN_DEFINE  ilbc,OPAL_ILBC

ilbc=yes
AC_ARG_ENABLE(ilbc,
       [  --disable-ilbc          disable iLBC codec support],
       ilbc=$enableval)
AC_MSG_CHECKING(iLBC codec)
if test "$ilbc" != "yes" ; then
  ilbc=no
  AC_MSG_RESULT(disabled)
elif test -f src/codec/ilbccodec.cxx && test -f src/codec/iLBC/iLBC_encode.c ; then
  ilbc=yes
  AC_MSG_RESULT(enabled)
else
  ilbc=no
  AC_MSG_RESULT(not present)
fi

if test "$ilbc" = "yes" ; then
  AC_SUBST(OPAL_ILBC, 1)
  AC_DEFINE(OPAL_ILBC)
fi

dnl ########################################################################
dnl enable audio, video, T.38

dnl MSWIN_DISPLAY audio,Audio
dnl MSWIN_DEFINE  audio,OPAL_AUDIO
AC_SUBST(OPAL_AUDIO, 1)
AC_DEFINE(OPAL_AUDIO)

dnl MSWIN_DISPLAY video,Video
dnl MSWIN_DEFINE  video,OPAL_VIDEO
AC_SUBST(OPAL_VIDEO, 1)
AC_DEFINE(OPAL_VIDEO)

dnl ########################################################################
dnl check if SIP, H.323, IAX2 and T.38 are enabled

dnl MSWIN_DISPLAY sip,SIP support
dnl MSWIN_DEFINE  sip,OPAL_SIP

sip=yes
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

dnl MSWIN_DISPLAY    t38,T.38
dnl MSWIN_DEFINE     t38,OPAL_T38FAX
dnl MSWIN_IF_FEATURE t38,h323

h323=yes
t38=yes
AC_ARG_ENABLE(h323,
       [  --disable-h323          disable H.323 protocol support],
       h323=$enableval)
AC_MSG_CHECKING(H.323 protocol)
if test "$h323" = "yes" ; then
  AC_SUBST(OPAL_H323, 1)
  AC_DEFINE(OPAL_H323)
  AC_MSG_RESULT(enabled)
else
  AC_MSG_RESULT(disabled (T.38 also disabled))
  t38=no
fi
AC_SUBST(OPAL_H323, $OPAL_H323)

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

dnl MSWIN_DISPLAY iax2,IAX2 support
dnl MSWIN_DEFINE  iax2,OPAL_IAX2

iax=yes
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

dnl MSWIN_DISPLAY h224,H.224 support
dnl MSWIN_DEFINE  h224,OPAL_H224

h224=yes
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
dnl H.460
dnl
dnl MSWIN_DISPLAY h460,H.460
dnl MSWIN_DEFINE  h460,H323_H460

AC_ARG_ENABLE(h460,
       [  --disable-h460          disable H.460])
if test "${enable_h460}x" = "x" ; then
  enable_h460=yes
fi
if test "$enable_h460" == "yes" ; then
  H323_H460=1
  AC_DEFINE(H323_H460, 1)
else
  H323_H460=
  AC_MSG_NOTICE(Disabling H.460)
fi
AC_SUBST(H323_H460)

dnl ########################################################################
dnl
dnl Quicknet xJACK cards
dnl 
dnl MSWIN_DISPLAY ixj,Quicknet Internet xJACK cards
dnl MSWIN_DEFINE  ixj,HAS_IXJ

HAS_IXJ=
AC_CHECK_HEADERS(linux/telephony.h sys/telephony.h /usr/local/include/sys/telephony.h, HAS_IXJ=1)
if test "x$HAS_IXJ" != "x" ; then
  AC_MSG_CHECKING(if ixjuser.h actually compiles)
  AC_TRY_COMPILE([#include <linux/ixjuser.h>],
                 [int main(int ac,char **av) { return 0; }], HAS_IXJ=1, HAS_IXJ=)
  if test "x$HAS_IXJ" != "x" ; then
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAS_IXJ, 1)
    HAS_IXJ=1
  else
    AC_MSG_RESULT(no)
  fi
fi
AC_SUBST(HAS_IXJ)


dnl ########################################################################
dnl
dnl VoiceBlaster
dnl 
dnl MSWIN_DISPLAY vblaster,Voice Blaster
dnl MSWIN_DEFINE  vblaster,HAS_VBLASTER


dnl ########################################################################
dnl
dnl VoiceTronics VPB card
dnl 
dnl MSWIN_DISPLAY     vpb,VoiceTronics VPB
dnl MSWIN_CHECK_FILE  vpb,src\vpbapi.h,HAS_VPB=1
dnl MSWIN_DIR_SYMBOL  vpb,VPB_DIR
dnl MSWIN_CHECK_DIR   vpb,..\vpb-driver\
dnl MSWIN_CHECK_DIR   vpb,..\external\vpb-driver
dnl MSWIN_CHECK_DIR   vpb,\vpb-driver
dnl MSWIN_CHECK_DIR   vpb,c:\vpb-driver


dnl ########################################################################
dnl
dnl Sangoma libraries
dnl 

OLD_CPPFLAGS=$CPPFLAGS
CPPFLAGS=-D__LINUX__

HAS_SANGOMA=
AC_CHECK_HEADERS(/usr/local/include/libpri.h, HAS_SANGOMA=1)
if test "x$HAS_SANGOMA" != "x" ; then
  AC_CHECK_HEADERS(/usr/local/include/libsangoma.h, HAS_SANGOMA=1)
  if test "x$HAS_SANGOMA" != "x" ; then
    AC_CHECK_LIB(pri, pri_new_cb, HAS_SANGOMA=1)
    if test "x$HAS_SANGOMA" = "x" ; then
      AC_MSG_NOTICE(*** If the latest libri is installed in /usr/local/lib, ensure it is not obscured by an older version in /usr/lib)
    else
      AC_CHECK_LIB(sangoma, sangoma_create_socket, HAS_SANGOMA=1)
      if test "x$HAS_SANGOMA" != "x" ; then
        AC_DEFINE(HAS_SANGOMA, 1)
        HAS_SANGOMA=1
      fi
    fi
  fi
fi
AC_SUBST(HAS_SANGOMA)

CPPFLAGS=$OLD_CPPFLAGS


dnl ########################################################################
dnl output make directives

AC_SUBST(STDCCFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(ENDLDLIBS)
AC_PROG_INSTALL

dnl ########################################################################
dnl output header file

AC_CONFIG_FILES(opal_inc.mak)
AC_CONFIG_FILES(Makefile)
AC_CONFIG_HEADERS(include/opal/buildopts.h)

AC_OUTPUT(src/codec/speex/libspeex/speex_config_types.h)
