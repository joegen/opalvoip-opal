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

class PTimer
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib\timer.h>
%End

  public:
    typedef unsigned IDType;

  /**@name Construction */
  //@{
    /** Create a new timer object and start it in one shot mode for the
       specified amount of time. If the time was zero milliseconds then the
       timer is {\bf not} started, ie the callback function is not executed
       immediately.
      */
    PTimer(
      long milliseconds = 0,  ///< Number of milliseconds for timer.
      int seconds = 0,        ///< Number of seconds for timer.
      int minutes = 0,        ///< Number of minutes for timer.
      int hours = 0,          ///< Number of hours for timer.
      int days = 0            ///< Number of days for timer.
    );
	
    PTimer(
      const PTimeInterval & time    ///< New time interval for timer.
    );


    /** Destroy the timer object, removing it from the applications timer list
       if it was running.
     */
    virtual ~PTimer();
  //@}

  /**@name Control functions */
  //@{
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

    /** Start a timer in continous cycle mode. Whenever the timer runs out it
       is automatically reset to the time specified. Thus, it calls the
       notification function every time interval.
     */
    void RunContinuous(
      const PTimeInterval & time    // New time interval for timer.
    );

    /** Stop a running timer. The timer will not call the notification function
       and is reset back to the original timer value. Thus when the timer
       is restarted it begins again from the beginning.

       The wait flag indicates that the function should wait for the timeout
       callback to complete before returning. That way external logic can be
       assured there is no race condition. However, under some circumstances
       this can cause a deadlock if the timeout function tries to acquire a
       mutex the calling thread already has, so an aysnchronouse version is
       provided. It is then the responsibility of the caller to handle the
       race condition with the timeout function.
     */
    void Stop(
      bool wait = true  
    );

    /** Determine if the timer is currently running. This really is only useful
       for one shot timers as repeating timers are always running.
       
       @return
       PTrue if timer is still counting.
     */
    PBoolean IsRunning() const;

    /** Pause a running timer. This differs from the #Stop()# function in
       that the timer may be resumed at the point that it left off. That is
       time is "frozen" while the timer is paused.
     */
    void Pause();

    /** Restart a paused timer continuing at the time it was paused. The time
       left at the moment the timer was paused is the time until the next
       call to the notification function.
     */
    void Resume();

    /** Determine if the timer is currently paused.

       @return
       PTrue if timer paused.
     */
    PBoolean IsPaused() const;

    /** Restart a timer continuing from the time it was initially.
     */
    void Reset();

    /** Get the time this timer was set to initially.
     */
    const PTimeInterval & GetResetTime() const;
  //@}

  /**@name Notification functions */
  //@{
    /**This function is called on time out. That is when the system timer
       processing decrements the timer from a positive value to less than or
       equal to zero. The interval is then reset to zero and the function
       called.

       Please note that the application should not execute large amounts of
       code in this call back or the accuracy of ALL timers can be severely
       impacted.

       The default behaviour of this function is to call the #PNotifier# 
       callback function.
     */
    virtual void OnTimeout();

    /** Get the current call back function that is called whenever the timer
       expires. This is called by the #OnTimeout()# function.

       @return
       current notifier for the timer.
     */
/*     const PNotifier & GetNotifier() const; */

    /** Set the call back function that is called whenever the timer expires.
       This is called by the #OnTimeout()# function.
     */
/*     void SetNotifier(
      const PNotifier & func  // New notifier function for the timer.
    ); */
  //@}

  /**@name Global real time functions */
  //@{
    /** Get the number of milliseconds since some arbtrary point in time. This
       is a platform dependent function that yields a real time counter.
       
       Note that even though this function returns milliseconds, the value may
       jump in minimum quanta according the platforms timer system, eg under
       MS-DOS and MS-Windows the values jump by 55 every 55 milliseconds. The
       #Resolution()# function may be used to determine what the minimum
       time interval is.
    
       @return
       millisecond counter.
     */
    static PTimeInterval Tick();

    /** Get the smallest number of milliseconds that the timer can be set to.
       All actual timing events will be rounded up to the next value. This is
       typically the platforms internal timing units used in the #Tick()#
       function.
       
       @return
       minimum number of milliseconds per timer "tick".
     */
    static unsigned Resolution();
  //@}

    IDType GetTimerId();
};