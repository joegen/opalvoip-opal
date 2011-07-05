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

class PCriticalSection : PSync
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib/critsec.h> 
%End
  public:
  /**@name Construction */
  //@{
    /**Create a new critical section object .
     */
    PCriticalSection();

    /**Allow copy constructor, but it actually does not copy the critical section,
       it creates a brand new one as they cannot be shared in that way.
     */
    PCriticalSection(const PCriticalSection &);

    /**Destroy the critical section object
     */
    ~PCriticalSection();

  //@}

  /**@name Operations */
  //@{
    /** Create a new PCriticalSection
      */
    PObject * Clone() const;

    /** Enter the critical section by waiting for exclusive access.
     */
    void Wait();
	
    void Enter();

    /** Leave the critical section by unlocking the mutex
     */
    void Signal();
	
    void Leave();

    /** Try to enter the critical section for exlusive access. Does not wait.
        @return true if cirical section entered, leave/Signal must be called.
      */
    bool Try();
  //@}

	private:
		    /**Assignment operator is allowed but does nothing. Overwriting the old critical
       section information would be very bad.
      */
    PCriticalSection & operator=(const PCriticalSection &);
};

typedef PCriticalSection PMutex;