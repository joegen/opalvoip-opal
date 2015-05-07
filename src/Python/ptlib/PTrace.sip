/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

/**Class to encapsulate tracing functions.
   This class does not require any instances and is only being used as a
   method of grouping functions together in a name space.
  */
class PTrace
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib\object.h>
%End
public:
  /// Options for trace output.
  enum Options {
    /**Include PTrace::Block constructs in output
       If this is bit is clear, all PTrace::Block output is inhibited
       regardless of the trace level. If set, the PTrace::Block may occur
       provided the trace level is greater than zero.
    */
    Blocks = 1,
    /// Include date and time in all output
    DateAndTime = 2,
    /// Include (millisecond) timestamp in all output
    Timestamp = 4,
    /// Include identifier for thread trace is made from in all output
    Thread = 8,
    /// Include trace level in all output
    TraceLevel = 16,
    /// Include the file and line for the trace call in all output
    FileAndLine = 32,
    /// Include thread object pointer address in all trace output
    ThreadAddress = 64,
    /// Append to log file rather than resetting every time
    AppendToFile = 128,
    /** Output timestamps in GMT time rather than local time
      */
    GMTTime = 256,
    /** If set, log file will be rotated 
      */
    RotateDaily = 512,
    RotateHourly = 1024,
    RotateMinutely = 2048,
    RotateLogMask = 3584, //RotateDaily + RotateHourly + RotateMinutely,
    /** SystemLog flag for tracing within a PServiceProcess application. Must
        be set in conjection with SetStream(new PSystemLog).
      */
    SystemLogStream = 32768
  };


  /**Set the most common trace options.
     If filename is not NULL then a PTextFile is created and attached the
     trace output stream. This object is never closed or deleted until the
     termination of the program.

     A trace output of the program name version and OS is written as well.
    */
  static void Initialise(
    unsigned level,
    const char * filename = NULL,
    unsigned options = Timestamp | Thread | Blocks
  );

  /**Set the most common trace options.
     If filename is not NULL then a PTextFile is created and attached the
     trace output stream. This object is never closed or deleted until the
     termination of the program.

     If rolloverPatterm is not NULL it is used as the time format patterm
     appended to filename if the RotateDaily is set. Default: yyyy_MM_dd

     A trace output of the program name version and OS is written as well.
    */
  static void Initialise(
    unsigned level,
    const char * filename,
    const char * rolloverPattern,
    unsigned options = Timestamp | Thread | Blocks
  );

  /** Set the trace options.
  The PTRACE(), PTRACE_BLOCK() and PTRACE_LINE() macros output trace text that
  may contain assorted values. These are defined by the Options enum.

  Note this function OR's the bits included in the options parameter.
  */
  static void SetOptions(unsigned options /** New level for trace */ );

  /** Clear the trace options.
  The #PTRACE(), #PTRACE_BLOCK() and #PTRACE_LINE() macros output trace text that
  may contain assorted values. These are defined by the Options enum.

  Note this function AND's the complement of the bits included in the options
  parameter.
  */
  static void ClearOptions(unsigned options /** New level for trace */ );

  /** Get the current trace options.
  The #PTRACE(), #PTRACE_BLOCK() and #PTRACE_LINE() macros output trace text that
  may contain assorted values. These are defined by the Options enum.
  */
  static unsigned GetOptions();

  /** Set the trace level.
  The #PTRACE() macro checks to see if its level is equal to or lower then the
  level set by this function. If so then the trace text is output to the trace
  stream.
  */
  static void SetLevel(unsigned level /** New level for trace */ );

  /** Get the trace level.
  The #PTRACE() macro checks to see if its level is equal to or lower then the
  level set by this function. If so then the trace text is output to the trace
  stream.
  */
  static unsigned GetLevel();

  /** Determine if the level may cause trace output.
  This checks against the current global trace level set by #PSetTraceLevel
  for if the trace output may be emitted. This is used by the PTRACE macro.
  */
  static PBoolean CanTrace(unsigned level /** Trace level to check */);
  
   /** Cleanup the trace system for a specific thread
      When using thread local storage, this will delete the per-thread trace context
    */
  static void Cleanup();
 };