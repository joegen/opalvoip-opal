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

/** This class defines a thread synchronisation object.
  This form of semaphore is used to indicate an <i>event</i> has occurred. A
  thread may block on the sync point and wait until another thread signals
  that it may continue. eg:
<pre><code>
    ... thread one
    while (condition) {
      sync.Wait();
      do_something();
    }

    ... thread 2
    do_something_else();
    sync.Signal();    // At this point thread 1 wake up and does something.
    do_yet_more();
</code></pre>

  Note that events are boolean in nature. If "thread one" is not waiting on the
  event, multiple calls to Signal() are ignored, thread one will only exit the
  Wait() call a single time once it gets there. Similarly, if multiple threads
  are waiting on the event, only one thread will be released. As you cannot know
  which thread will be released this mode of use is not recommended.
 */
class PSyncPoint : PSemaphore
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib/syncpoint.h>
%End
  public:
    /** Create a new sync point.
     */
    PSyncPoint();
    PSyncPoint(const PSyncPoint &);

  private:
	PSyncPoint &operator =(const PSyncPoint &);

// Include platform dependent part of class
public:
    virtual void Signal();
};