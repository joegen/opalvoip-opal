dnl
dnl my_macros.m4
dnl
dnl A bunch of useful macros for doing complex configure.ac
dnl scripts, especially when cross compiling.
dnl

dnl internal macro help
AC_DEFUN([MY_IFELSE],[
   m4_ifnblank([$2$3], [AS_IF([test "x$m4_normalize($1)" = "xyes"], [$2], [$3])])
])


dnl internal macro help
AC_DEFUN([MY_ARG_DEPENDENCY],[
   m4_ifnblank([$1],[
      AS_IF([test "x${enableval}" = "xyes" && \
             test "x$m4_normalize($1)" != "x1" && \
             test "x$m4_normalize($1)" != "xyes"],[
         AC_MSG_RESULT([disabled due to disabled dependency $1=$$1])
         enableval=no
      ])
   ])
])

dnl MY_ARG_ENABLE
dnl As AC_ARG_ENABLE, but with disable, defaults and dependecies
dnl $1 command line option name
dnl $2 command line help
dnl $3 default state
dnl $4 enabled script
dnl $5 disabled script
dnl $6..$9 dependency variable(s)
dnl The variable enable_$1 is defined to yes/no
AC_DEFUN([MY_ARG_ENABLE],[
   AC_MSG_CHECKING([$2])

   AC_ARG_ENABLE(
      [$1],
      AC_HELP_STRING([--m4_bmatch([$2],[enable.*],[enable],[disable])-$1],
                         m4_bmatch([$2],[enable.*],[],[disable ])[$2]),
      AS_VAR_IF([enableval], [no], AC_MSG_RESULT([disabled by user])),
      [
         enableval=m4_default([$3],m4_bmatch([$2],[enable.*],[no],[yes]))
         AS_VAR_IF([enableval], [no], AC_MSG_RESULT([disabled by default]))
      ]
   )

   MY_ARG_DEPENDENCY([$6])
   MY_ARG_DEPENDENCY([$7])
   MY_ARG_DEPENDENCY([$8])
   MY_ARG_DEPENDENCY([$9])

   AS_VAR_IF([enableval], [yes], AC_MSG_RESULT([yes]))
   MY_IFELSE([enableval], [$4], [$5])

   enable_$1="$enableval"
])


dnl MY_COMPILE_IFELSE
dnl As AC_COMPILE_IFELSE but saves and restores CPPFLAGS if fails
dnl $1 checking message
dnl $2 CPPFLAGS
dnl $3 program headers
dnl $4 program main
dnl $5 success code
dnl $6 failure code
AC_DEFUN([MY_COMPILE_IFELSE],[
   oldCPPFLAGS="$CPPFLAGS"
   CPPFLAGS="$CPPFLAGS $2"
   AC_MSG_CHECKING([$1])
   AC_COMPILE_IFELSE(
      [AC_LANG_PROGRAM([[$3]],[[$4]])],
      [usable=yes],
      [usable=no]
   )
   AC_MSG_RESULT($usable)
   CPPFLAGS="$oldCPPFLAGS"
   MY_IFELSE([usable], [$5], [$6])
])


dnl MY_LINK_IFELSE
dnl As AC_LINK_IFELSE but saves and restores CPPFLAGS & LIBS if fails
dnl $1 checking message
dnl $2 CPPFLAGS
dnl $3 LIBS
dnl $4 program headers
dnl $5 program main
dnl $6 success code
dnl $7 failure code
AC_DEFUN([MY_LINK_IFELSE],[
   MY_LINK_IFELSE_CPPFLAGS="$CPPFLAGS"
   MY_LINK_IFELSE_LIBS="$LIBS"
   CPPFLAGS="$CPPFLAGS $2"
   LIBS="$3 $LIBS"
   AC_MSG_CHECKING($1)
   AC_LINK_IFELSE(
      [AC_LANG_PROGRAM([[$4]],[[$5]])],
      [usable=yes],
      [usable=no]
   )
   AC_MSG_RESULT($usable)
   CPPFLAGS="$MY_LINK_IFELSE_CPPFLAGS"
   LIBS="$MY_LINK_IFELSE_LIBS"
   MY_IFELSE([usable], [$6], [$7])
])


dnl MY_PKG_CHECK_MODULE
dnl As PKG_CHECK_MODULES but does test compile so works wit cross compilers
dnl $1 module name
dnl $2 pkg name
dnl $3 program headers
dnl $4 program main
dnl $5 success code
dnl $6 failure code
AC_DEFUN([MY_PKG_CHECK_MODULE],[
   PKG_CHECK_MODULES(
      [$1],
      [$2],
      [MY_LINK_IFELSE(
         [for $1 usability],
         [$$1[_CFLAGS]],
         [$$1[_LIBS]],
         [$3],
         [$4],
         [
            CPPFLAGS="$CPPFLAGS $$1[_CFLAGS]"
            LIBS="$$1[_LIBS] $LIBS"
         ]
      )],
      [usable=no]
   )
   MY_IFELSE([usable], [$5], [$6])
])


dnl MY_ADD_FLAGS
dnl Prepend to CPPFLAGS, CFLAGS, CXXFLAGS & LIBS new flags
dnl $1 new LIBS
dnl $2 new CPPFLAGS
dnl $3 new CFLAGS
dnl $4 new CXXFLAGS
AC_DEFUN([MY_ADD_FLAGS],[
   m4_ifnblank([$2], [
      AC_MSG_NOTICE([Adding CPPFLAGS: $2])
      CPPFLAGS="$2 $CPPFLAGS"
   ])
   m4_ifnblank([$3], [
      AC_MSG_NOTICE([Adding CFLAGS: $3])
      CFLAGS="$3 $CFLAGS"
   ])
   m4_ifnblank([$4], [
      AC_MSG_NOTICE([Adding CXXFLAGS: $4])
      CXXFLAGS="$4 $CXXFLAGS"
   ])
   m4_ifnblank([$1], [
      AC_MSG_NOTICE([Adding LIBS: $1])
      LIBS="$1 $LIBS"
   ])
])


dnl MY_ADD_MODULE_FLAGS
dnl Add to CPPFLAGS, & LIBS new flags from xxx_CFLAGS, xxx_LIBS
dnl $1 module name
AC_DEFUN([MY_ADD_MODULE_FLAGS],[
   MY_ADD_FLAGS($$1[_LIBS], $$1[_CFLAGS])
])


dnl MY_MODULE_OPTION
dnl Check for modules existence, with --disable-XXX and optional --with-XXX-dir
dnl If one of the pkg names ($4) is local-source, then the --enable-local$1 option is
dnl provided, which will set $1_SYSTEM to no
dnl $1 module name
dnl $2 command line option name
dnl $3 command line option help text
dnl $4 pkg name(s)
dnl $5 default CPPFLAGS
dnl $6 default LIBS
dnl $7 program headers
dnl $8 program main
dnl $9 success code
dnl $10 failure code
dnl $11..$14 optional dependency
dnl returns $1_USABLE=yes/no, $1_SYSTEM=yes/no, $1_CFLAGS and $1_LIBS
AC_DEFUN([MY_MODULE_OPTION],[
   AC_SUBST($1[_SYSTEM], "yes")

   MY_ARG_ENABLE([$2], [$3], [${DEFAULT_$1:-yes}], [usable=yes], [usable=no], [$11], [$12], [$13], [$14])

   AS_VAR_IF([usable], [yes], [
      m4_bmatch([$4], [.*local-source.*], [
         AC_ARG_ENABLE(
            [local$2],
            [AC_HELP_STRING([--enable-local$2],[force use internal source for $3])],
            AS_VAR_IF([enableval], [yes], [
               $1[_SYSTEM]="no"
               AC_MSG_NOTICE(Forced use of internal source for $3)
            ],
               AC_MSG_NOTICE(Using system source for $3)
            )
         )
      ])

      AS_VAR_IF([$1[_SYSTEM]], [yes], [
         m4_ifnblank([$5$6],
            [AC_ARG_WITH(
               [$2-dir],
               AS_HELP_STRING([--with-$2-dir=<dir>],[location for $3]),
               [
                  AC_MSG_NOTICE(Using directory $withval for $3)
                  $1[_CFLAGS]="-I$withval/include $5"
                  $1[_LIBS]="-L$withval/lib $6"
               ],
               [PKG_CHECK_MODULES(
                  [$1],
                  [m4_bpatsubsts([$4],[local-source], [])],
                  [],
                  [
                     $1[_CFLAGS]="$5"
                     $1[_LIBS]="$6"
                  ]
               )]
            )],
            [PKG_CHECK_MODULES(
               [$1],
               [m4_bpatsubsts([$4],[local-source], [])],
               [],
               [usable=no]
            )]
         )

         AS_VAR_IF([usable], [yes],
            MY_LINK_IFELSE(
               [for $3 usability],
               [$$1[_CFLAGS]],
               [$$1[_LIBS]],
               [$7],
               [$8],
               [MY_ADD_MODULE_FLAGS([$1])],
               [usable=no]
            )
         )

         m4_bmatch([$4], [.*local-source.*], [
            AS_VAR_IF([usable], [no], [
               usable="yes"
               $1[_SYSTEM]="no"
               $1[_CFLAGS]=
               $1[_LIBS]=
               AC_MSG_NOTICE(Using internal source for $3)
            ])
         ])
      ])
   ])

   AC_SUBST($1[_USABLE], $usable)

   MY_IFELSE([usable], [$9], [$10])
])


dnl MY_CHECK_DLFCN
dnl Check for dlopen function and make sure library set in LIBS
dnl $1 action if found
dnl $2 action of not found
AC_DEFUN([MY_CHECK_DLFCN],[
   AS_CASE([$target_os],
      cygwin* | mingw*,
      [
         usable=yes
      ],
      AC_CHECK_LIB(
         [dl],
         [dlopen],
         [
            usable=yes
            DLFCN_LIBS="-ldl"
         ],
         AC_CHECK_LIB(
            [c],
            [dlopen],
            [
               usable=yes
               DLFCN_LIBS-"-lc"
            ],
            [usable=no]
         )
      )
   )

   MY_IFELSE([usable], [$1], [$2])
])


dnl MY_VERSION_FILE
dnl Parse version file and set variables
dnl $1 file location
dnl $2 variable prefix
AC_DEFUN([MY_VERSION_FILE],[
   AS_VAR_SET_IF([$2[_MAJOR]],[major=${$2[_MAJOR]}],[major=`cat $1 | grep MAJOR_VERSION | cut -f3 -d' '`])
   AS_VAR_SET_IF([$2[_MINOR]],[minor=${$2[_MINOR]}],[minor=`cat $1 | grep MINOR_VERSION | cut -f3 -d' '`])
   AS_VAR_SET_IF([$2[_BUILD]],[build=${$2[_BUILD]}],[build=`cat $1 | grep BUILD_NUMBER  | cut -f3 -d' ' | sed 's/0x@<:@1-9@:>@*0*//'`])
   AS_VAR_SET_IF([$2[_STAGE]],[stage=${$2[_STAGE]}],[stage=`cat $1 | grep BUILD_TYPE    | cut -f 3 -d ' ' | sed 's/BetaCode/-beta/' | sed 's/AlphaCode/-alpha/' | sed 's/ReleaseCode/\./'`])
   version="${major}.${minor}.${build}"

   AS_IF([test -z "$major" -o -z "$minor" -o -z "$build"], AC_MSG_ERROR(Could not determine version number from $1))

   AC_SUBST($2[_MAJOR], $major)
   AC_SUBST($2[_MINOR], $minor)
   AC_SUBST($2[_BUILD], $build)
   AC_SUBST($2[_STAGE], $stage)
   AC_SUBST($2[_VERSION], "${version}")

   AC_DEFINE_UNQUOTED([$2[_MAJOR]],   [${major}], [Major version])
   AC_DEFINE_UNQUOTED([$2[_MINOR]],   [${minor}], [Minor version])
   AC_DEFINE_UNQUOTED([$2[_BUILD]],   [${build}], [Build number])
   AC_DEFINE_UNQUOTED([$2[_VERSION]], ["$version"],[PTLib version])

   AC_MSG_NOTICE([$2 version is $version])
])


AC_DEFUN([INTERNAL_OUTPUT_SUMMARY],[
   m4_ifblank([$2],
      AS_ECHO("$1"),
      AS_VAR_SET_IF([$2],
         AS_ECHO("$1 : ${$2}"),
         AS_ECHO("$1 : no")
      )
   )
])

dnl MY_OUTPUT_SUMMARY
dnl Output summary of configuration at end of script
AC_DEFUN([MY_OUTPUT_SUMMARY],[
   AS_ECHO("")
   AS_ECHO("=========================== Configuration ==============================")
   INTERNAL_OUTPUT_SUMMARY([                          OS Type],[target_os])
   INTERNAL_OUTPUT_SUMMARY([                     Machine Type],[target_cpu])
   AS_VAR_SET_IF([P_PROFILING], [
      AS_ECHO("")
      INTERNAL_OUTPUT_SUMMARY([           ***** Profiling ***** ], P_PROFILING)
   ])
   AS_ECHO("")
   INTERNAL_OUTPUT_SUMMARY([                           prefix],[prefix])
   INTERNAL_OUTPUT_SUMMARY([                      exec_prefix],[exec_prefix])
   INTERNAL_OUTPUT_SUMMARY([                       includedir],[includedir])
   INTERNAL_OUTPUT_SUMMARY([                           libdir],[libdir])
   INTERNAL_OUTPUT_SUMMARY([                      datarootdir],[datarootdir])
   INTERNAL_OUTPUT_SUMMARY([                          makedir],[makedir])

   AS_ECHO("")
   m4_map_args_pair([INTERNAL_OUTPUT_SUMMARY], [AS_ECHO], $@)
   AS_ECHO("")
   INTERNAL_OUTPUT_SUMMARY([                         CPPFLAGS], [CPPFLAGS])
   INTERNAL_OUTPUT_SUMMARY([                           CFLAGS], [CFLAGS])
   INTERNAL_OUTPUT_SUMMARY([                         CXXFLAGS], [CXXFLAGS])
   INTERNAL_OUTPUT_SUMMARY([                          LDFLAGS], [LDFLAGS])
   INTERNAL_OUTPUT_SUMMARY([                             LIBS], [LIBS])
   AS_ECHO("")
   AS_ECHO("========================================================================")
])


dnl ##################################################################

dnl Now the common stuff, checks for stuffwe always use

PKG_PROG_PKG_CONFIG()
AS_VAR_SET_IF([PKG_CONFIG], , AC_MSG_ERROR(pkg-config is required, 1))

dnl  This generates "normalised" target_os, target_cpu, target_64bit and target
dnl  variables. The standard ones are a little too detailed for our use. Also,
dnl  we check for --enable-ios=X to preset above variables and compiler for
dnl  correct cross compilation, easier to remember that the --host command.
dnl  Finally, it defines the various compulsory progams (C, C++, ld, ar, etc)
dnl  and flags that are used by pretty much any build.

AC_ARG_ENABLE([ios], [AS_HELP_STRING([--enable-ios=iphone|simulator],[enable iOS support])])

AS_CASE([$enable_ios],
   "", [
      AC_CANONICAL_TARGET()
   ],
   iphone, [
      target_vendor=apple
      target_os=iPhoneOS
      target_cpu=armv7
      target_release=`xcodebuild -showsdks | sed -n 's/.*iphoneos\(.*\)/\1/p' | sort | tail -n 1`
   ],
   simulator, [
      target_vendor=apple
      target_os=iPhoneSimulator
      target_cpu=i686
      target_release=`xcodebuild -showsdks | sed -n 's/.*iphonesimulator\(.*\)/\1/p' | sort | tail -n 1`
   ],
   dnl default
      AC_MSG_ERROR([Unknown iOS variant \"${enable_ios}\" - use either iphone or simulator])
)

AS_CASE([$target_os],
   iPhone*, [
      AS_VAR_SET_IF([target_release], , AC_MSG_ERROR([Unable to determine iOS release number]))
   ],

   darwin*, [
      target_os=Darwin
      target_release=`xcodebuild -showsdks | sed -n 's/.*macosx\(.*\)/\1/p' | sort | tail -n 1`
   ],

   linux* | Linux* | uclibc*, [
      target_os=linux
   ],

   freebsd* | kfreebsd*, [
      target_os=FreeBSD
      target_release="`sysctl -n kern.osreldate`"
      AC_CHECK_TOOL(RANLIB, ranlib)
   ],

   openbsd*, [
      target_os=OpenBSD
      target_release="`sysctl -n kern.osrevision`"
   ],

   netbsd*, [
      target_os=NetBSD
      target_release="`/sbin/sysctl -n kern.osrevision`"
   ],

   solaris* | sunos*, [
      target_os=solaris
      target_release=`uname -r | sed "s/5\.//g"`
   ],

   beos*, [
      target_os=beos
   ],

   cygwin*, [
      target_os=cygwin
   ],

   mingw*, [
      target_os=mingw
   ],

   dnl default
      AC_MSG_WARN([Operating system \"$target_os\" not recognized - proceed with caution!])
)

AS_VAR_SET_IF([target_release], , target_release="`uname -r`")


dnl Set up compiler by platform
oldCFLAGS="$CFLAGS"
oldCXXFLAGS="$CXXFLAGS"

AC_PROG_CC()
AC_PROG_CXX()
AS_VAR_SET_IF([CXX], , AC_MSG_ERROR(C++ compiler is required, 1))

dnl Restore flags changed by AC_PROC_CC/AC_PROG_CXX
CFLAGS="$oldCFLAGS"
CXXFLAGS="$oldCXXFLAGS"

dnl Find some tools
AC_PROG_LN_S()
AC_PROG_RANLIB()
dnl AC_PROG_MKDIR_P()  -- Doesn't work!
AC_SUBST(MKDIR_P, "mkdir -p")
AC_PATH_PROG(SVN, svn)
AC_PATH_PROG(GIT, git)

AC_PROG_INSTALL()
AC_MSG_CHECKING([install support for -C])
touch /tmp/ptlib_install_test1
AS_IF([$INSTALL -C /tmp/ptlib_install_test1 /tmp/ptlib_install_test2 2> /dev/null],[
   AC_MSG_RESULT(yes)
   INSTALL="$INSTALL -C"
],
   AC_MSG_RESULT(no)
)
rm /tmp/ptlib_install_test?

AC_CHECK_TOOL(AR, ar)
dnl AS_VAR_SET_IF([AR], , AC_CHECK_TOOL(AR, gar))

AC_CHECK_TOOL(STRIP, strip)
AC_CHECK_TOOL(OBJCOPY, objcopy)   dnl GNU (Linux)
AC_CHECK_TOOL(DSYMUTIL, dsymutil) dnl clang (Mac-OS)

dnl Integer sizes, also defines HAVE_STDINT_H and HAVE_INTTYPES_H
AC_CHECK_SIZEOF(int)
AC_TYPE_INT8_T()
AC_TYPE_INT16_T()
AC_TYPE_INT32_T()
AC_TYPE_INT64_T()
AC_TYPE_INTPTR_T()
AC_TYPE_UINT8_T()
AC_TYPE_UINT16_T()
AC_TYPE_UINT32_T()
AC_TYPE_UINT64_T()
AC_TYPE_UINTPTR_T()
AC_TYPE_LONG_LONG_INT()
AC_TYPE_UNSIGNED_LONG_LONG_INT()
AC_TYPE_LONG_DOUBLE()


dnl Most unix'ish platforms are like this
AC_SUBST(LD, $CXX)
AC_SUBST(ARFLAGS, "rc")
AC_SUBST(STATICLIBEXT, "a")

AS_CASE([$target_os],
   iPhone* | Darwin, [
      AC_SUBST(SHAREDLIBEXT, "dylib")
      AC_SUBST(DEBUGINFOEXT, "dSYM")
   ],
   AC_SUBST(SHAREDLIBEXT, "so")
   AC_SUBST(DEBUGINFOEXT, "debug")
)

dnl Simplify the "known" CPU names

AS_CASE([$target_cpu],
   x86 | i686 | i586 | i486 | i386, [
      AC_MSG_CHECKING([64 bit system masquerading as 32 bit])
      AC_COMPILE_IFELSE(
         [AC_LANG_SOURCE([int t = __amd64__;])],
         [
            AC_MSG_RESULT(yes)
            target_cpu=x86_64
            target_64bit=1
         ],
         [
            AC_MSG_RESULT(no)
            target_cpu=x86
            target_64bit=0
            CFLAGS="-march=i686 $CFLAGS"
            CXXFLAGS="-march=i686 $CXXFLAGS"
         ]
      )
   ],

   x86_64 | amd64, [
      target_cpu=x86_64
      target_64bit=1
   ],

   alpha*, [
      target_cpu=alpha
      target_64bit=1
   ],

   sparc*, [
      target_cpu=sparc
      target_64bit=1
   ],

   ppc | powerpc, [
      target_cpu=ppc
      target_64bit=0
   ],

   ppc64 | powerpc64 | ppc64el | powerpc64le, [
      target_cpu=ppc64
      target_64bit=1
   ],

   hppa64 | ia64 | s390x, [
      target_64bit=1
   ],

   arm*, [
      target_64bit=0
   ],

   aarch64*, [
      target_64bit=1
   ],

   mips64 | mips64el, [
      target_64bit=1
   ],
      AC_MSG_WARN([CPU \"$target_cpu\" not recognized - assuming 32 bit])
      target_64bit=0
)

AC_ARG_ENABLE(force32, AS_HELP_STRING([--enable-force32],[Force 32-bit x86 build]))
AS_VAR_IF([enable_force32], [yes], [
   AS_VAR_IF([target_cpu], [x86_64], [], AC_MSG_ERROR([force32 option only available for 64 bit x86]))

   target_cpu=x86
   target_64bit=0

   AC_MSG_NOTICE(Forcing 32 bit x86 compile)
])


target=${target_os}_${target_cpu}

AC_MSG_NOTICE([Platform: \"$target_os\" release \"$target_release\" on \"$target_cpu\"])


dnl End of file

