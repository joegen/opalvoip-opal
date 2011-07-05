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

///////////////////////////////////////////////////////////////////////////////
// Difference between two system times

/** This class defines an arbitrary time interval to millisecond accuracy. The
   interval can be both positive and negative.
   
   A long int is used to store the time interval so it is limited to LONG_MAX
   (found in the standard C header file limits.h) milliseconds. This is
   approximately 596 hours for 32 bit integers.
   
   There is a constant, #PMaxTimeInterval# which defines the
   maximum number of milliseconds that a time interval may be.
 */
class PTimeInterval
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib/timeint.h>
%End

  public:
  /**@name Construction */
  //@{
    /** Create a new time interval object. The time interval, in milliseconds,
       is the sum of all of the parameters. For example all of the following
       are equivalent:
\begin{verbatim}
              PTimeInterval(120000)
              PTimeInterval(60000, 60)
              PTimeInterval(60000, 0, 1)
              PTimeInterval(0, 60, 1)
              PTimeInterval(0, 0, 2)
\end{verbatim}
     */
    PTimeInterval(
      PInt64 millisecs = 0   ///< Number of milliseconds for interval.
    );
	
    PTimeInterval(
      long millisecs,       ///< Number of milliseconds for interval.
      long seconds,         ///< Number of seconds for interval.
      long minutes = 0,     ///< Number of minutes for interval.
      long hours = 0,       ///< Number of hours for interval.
      int days = 0          ///< Number of days for interval.
    );
	
    PTimeInterval(
      const PString & str   ///< String representation of time interval.
    );
  //@}


  /**@name Conversion functions */
  //@{
    enum Formats {
      NormalFormat,
      IncludeDays,
      SecondsOnly
    };

    PString AsString(
      int decimals = 3,
      Formats format = NormalFormat,
      int width = 1
    ) const;
  //@}

  /**@name Access functions */
  //@{
    /** Get the number of milliseconds for the time interval.
    
       @return
       very long integer number of milliseconds.
     */
    PInt64 GetMilliSeconds() const;

    /** Get the number of whole seconds for the time interval.
    
       @return
       long integer number of seconds.
     */
    long GetSeconds() const;

    /** Get the number of whole minutes for the time interval.
    
       @return
       integer number of minutes.
     */
    long GetMinutes() const;

    /** Get the number of whole hours for the time interval.
    
       @return
       integer number of hours.
     */
    int GetHours() const;

    /** Get the number of whole days for the time interval.
    
       @return
       integer number of days.
     */
    int GetDays() const;

    /** Get the number of milliseconds for the time interval.
    
       @return
       long integer number of milliseconds.
     */
   unsigned long GetInterval() const;

    /** Set the value of the time interval. The time interval, in milliseconds,
       is the sum of all of the parameters. For example all of the following
       are equivalent:
\begin{verbatim}
              SetInterval(120000)
              SetInterval(60000, 60)
              SetInterval(60000, 0, 1)
              SetInterval(0, 60, 1)
              SetInterval(0, 0, 2)
\end{verbatim}
     */
    virtual void SetInterval(
      PInt64 milliseconds = 0,  ///< Number of milliseconds for interval.
      long seconds = 0,         ///< Number of seconds for interval.
      long minutes = 0,         ///< Number of minutes for interval.
      long hours = 0,           ///< Number of hours for interval.
      int days = 0              ///< Number of days for interval.
    );
  //@}

  /**@name Operations */
  //@{
    /** Unary minus, get negative of time interval.
    
       @return
       difference of the time intervals.
     */
    PTimeInterval operator-() const;

    /** Add the two time intervals yielding a third time interval.
    
       @return
       sum of the time intervals.
     */
    PTimeInterval operator+(
      const PTimeInterval & interval   ///< Time interval to add.
    ) const;

    /** Add the second time interval to the first time interval.
    
       @return
       reference to first time interval.
     */
    PTimeInterval & operator+=(
      const PTimeInterval & interval   ///< Time interval to add.
    );

    /** Subtract the two time intervals yielding a third time interval.
    
       @return
       difference of the time intervals.
     */
    PTimeInterval operator-(
      const PTimeInterval & interval   ///< Time interval to subtract.
    ) const;

    /** Subtract the second time interval from the first time interval.
    
       @return
       reference to first time interval.
     */
    PTimeInterval & operator-=(
      const PTimeInterval & interval   ///< Time interval to subtract.
    );

    /** Multiply the time interval by a factor yielding a third time interval.
    
       @return
       the time intervals times the factor.
     */
    PTimeInterval operator*(
      int factor   ///< factor to multiply.
    ) const;

    /** Multiply the time interval by a factor.
    
       @return
       reference to time interval.
     */
    PTimeInterval & operator*=(
      int factor   ///< factor to multiply.
    );

    /** Divide the time interval by a factor yielding a third time interval.
    
       @return
       the time intervals divided by the factor.
     */
    PTimeInterval operator/(
      int factor   ///< factor to divide.
    ) const;

    /** Divide the time interval by a factor.
    
       @return
       reference to time interval.
     */
    PTimeInterval & operator/=(
      int factor   ///< factor to divide.
    );
  //@}

  /**@name Comparison functions */
  //@{
    /** Compare to the two time intervals. This is provided as an override to
       the default in PObject so that comparisons can be made to integer
       literals that represent milliseconds.

       @return
       PTrue if intervals are equal.
     */
    bool operator==(
      const PTimeInterval & interval   ///< Time interval to compare.
    ) const;

    /** Compare to the two time intervals. This is provided as an override to
       the default in PObject so that comparisons can be made to integer
       literals that represent milliseconds.

       @return
       PTrue if intervals are not equal.
     */
    bool operator!=(
      const PTimeInterval & interval   ///< Time interval to compare.
    ) const;

    /** Compare to the two time intervals. This is provided as an override to
       the default in PObject so that comparisons can be made to integer
       literals that represent milliseconds.

       @return
       PTrue if intervals are greater than.
     */
    bool operator> (
      const PTimeInterval & interval   ///< Time interval to compare.
    ) const;
	

    /** Compare to the two time intervals. This is provided as an override to
       the default in PObject so that comparisons can be made to integer
       literals that represent milliseconds.

       @return
       PTrue if intervals are greater than or equal.
     */
    bool operator>=(
      const PTimeInterval & interval   ///< Time interval to compare.
    ) const;
	

    /** Compare to the two time intervals. This is provided as an override to
       the default in PObject so that comparisons can be made to integer
       literals that represent milliseconds.

       @return
       PTrue if intervals are less than.
     */
    bool operator< (
      const PTimeInterval & interval   ///< Time interval to compare.
    ) const;
	

    /** Compare to the two time intervals. This is provided as an override to
       the default in PObject so that comparisons can be made to integer
       literals that represent milliseconds.

       @return
       PTrue if intervals are less than or equal.
     */
    bool operator<=(
      const PTimeInterval & interval   ///< Time interval to compare.
    ) const;
	
  //@}
};