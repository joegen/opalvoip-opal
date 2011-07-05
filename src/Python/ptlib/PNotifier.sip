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

template <ParmType>
class PNotifierFunctionTemplate : PSmartObject /Abstract/
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib/notifier.h>
%End
  public:
    /// Create a notification function instance.
    PNotifierFunctionTemplate(
      void * obj    ///< Object instance on which the function will be called on.
    );

    /** Execute the call to the actual notification function on the object
       instance contained in this object.
     */
    virtual void Call(
      PObject & notifier,  ///< Object that is making the notification.
      ParmType extra       ///< Extra information that may be passed to function.
    ) const = 0;

};


template <ParmType>
class PNotifierTemplate : PSmartPointer
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib/notifier.h>
%End

  public:
    /** Create a new notification function smart pointer. */
    PNotifierTemplate(
      PNotifierFunctionTemplate<ParmType> * func = NULL   ///< Notifier function to call.
    );

    /**Execute the call to the actual notification function on the object
       instance contained in this object. This will make a polymorphic call to
       the function declared by the #PDECLARE_NOTIFIER# macro which in
       turn calls the required function in the destination object.
     */
    // virtual void operator()(
      // PObject & notifier,  ///< Object that is making the notification.
      // ParmType extra       ///< Extra information that may be passed to function.
    // );
};


class PNotifier
{
	public:
		PNotifier();
		
};
