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

%ModuleHeaderCode
#include <ptlib.h>
#include <ptlib/psync.h>
%End

class PSync : PObject /Abstract/
{
  public:
  /**@name Operations */
  //@{
    /**Block until the synchronisation object is available.
     */
    virtual void Wait() = 0;

    /**Signal that the synchronisation object is available.
     */
    virtual void Signal() = 0;
  //@}

};

class PSyncNULL : PSync
{
  public:
    virtual void Wait();
    virtual void Signal();
};

/**This class waits for the semaphore on construction and automatically
   signals the semaphore on destruction. Any descendent of PSemaphore
   may be used.

  This is very useful for constructs such as:
<pre><code>
    void func()
    {
      PWaitAndSignal mutexWait(myMutex);
      if (condition)
        return;
      do_something();
      if (other_condition)
        return;
      do_something_else();
    }
</code></pre>
 */

class PWaitAndSignal {
  public:
    /**Create the semaphore wait instance.
       This will wait on the specified semaphore using the Wait() function
       before returning.
      */
     PWaitAndSignal(
      const PSync & sem,   ///< Semaphore descendent to wait/signal.
      PBoolean wait = PTrue    ///< Wait for semaphore before returning.
    );

    /** Signal the semaphore.
        This will execute the Signal() function on the semaphore that was used
        in the construction of this instance.
     */
    ~PWaitAndSignal();

};


// End Of File ///////////////////////////////////////////////////////////////
