opalecho
========

Opal echo has web interface on http://localhost:3246 for configuration.  Web
interface allow you to change all settings (logging, codec etc).

Default program uses SIP And only do Audio echo's. Video seem to be disabled.
SIP routing setup by default: sip:.* = echo:

Default welcome page is: welcome.html if it does not exist program will
generate one. Graphic by default is 300 x 100 name opalecho.gif.  Also if the
HTML file in same directory as executable, you need signatures currently --
see: httpsvc.cxx in ptlib for how it implemented.

Default directories for html files are: data and html in same directory as
executable. They do not need signature. Web URL are:

  http://localhost:3246/data/<filename>.html
  http://localhost:3246/html/<filename>.html



Usage can be displayed by: ./opalecho --help 

usage: opalecho [ options ]

Execution:
  -d or --daemon            : run as a daemon
  -x or --execute           : execute as a normal program
  -v or --version           : display version information and exit
  -h or --help              : output this help message and exit

Options:
  -p or --pid-file <arg>    : file path or directory for pid file (default /var/run/)
                              if directory, then file is <dir>/opalecho.pid
  -i or --ini-file <arg>    : set the ini file to use, may be explicit file path or
                              if directory, then file is <dir>/opalecho.ini
  -u or --uid <arg>         : set user id to run as
  -g or --gid <arg>         : set group id to run as
  -c or --console           : output messages to stdout instead of syslog
  -l or --log-file <arg>    : output messages to file or directory instead of syslog
                              if directory then file is <dir>/opalecho.log
  -r or --remote-log <arg>  : output messages to remote syslog server
  -H or --handle-max <arg>  : set maximum number of file handles, this is set
                              before the uid/gid, so can initially be superuser

Control:
  -s or --status            : check to see if daemon is running
  -t or --terminate         : orderly terminate process in pid file (SIGTERM)
  -k or --kill              : preemptively kill process in pid file (SIGKILL)
  -U or --trace-up          : increase the trace log level
  -D or --trace-down        : reduce the trace log level

  
                                   --oOo--
