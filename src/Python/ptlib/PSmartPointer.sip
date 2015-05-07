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
// "Smart" pointers.

/** This is the base class for objects that use the <i>smart pointer</i> system.
   In conjunction with the #PSmartPointer# class, this class creates
   objects that can have the automatic deletion of the object instance when
   there are no more smart pointer instances pointing to it.

   A #PSmartObject# carries the reference count that the #PSmartPointer# 
   requires to determine if the pointer is needed any more and should be
   deleted.
 */
class PSmartObject : PObject
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib/smartptr.h>
%End
  public:
    /** Construct a new smart object, subject to a #PSmartPointer# instance
       referencing it.
     */
    PSmartObject();

};



/** This is the class for pointers to objects that use the <i>smart pointer</i>
   system. In conjunction with the #PSmartObject# class, this class
   references objects that can have the automatic deletion of the object
   instance when there are no more smart pointer instances pointing to it.

   A PSmartPointer carries the pointer to a #PSmartObject# instance which
   contains a reference count. Assigning or copying instances of smart pointers
   will automatically increment and decrement the reference count. When the
   last instance that references a #PSmartObject# instance is destroyed or
   overwritten, the #PSmartObject# is deleted.

   A NULL value is possible for a smart pointer. It can be detected via the
   #IsNULL()# function.
 */
class PSmartPointer : PObject
{
  public:
  /**@name Construction */
  //@{
    /** Create a new smart pointer instance and have it point to the specified
       #PSmartObject# instance.
     */
    PSmartPointer(
      PSmartObject * obj = NULL   /// Smart object to point to.
    );

    /** Create a new smart pointer and point it at the data pointed to by the
       #ptr# parameter. The reference count for the object being
       pointed at is incremented.
     */
    PSmartPointer(
      const PSmartPointer & ptr  /// Smart pointer to make a copy of.
    );

    /** Destroy the smart pointer and decrement the reference count on the
       object being pointed to. If there are no more references then the
       object is deleted.
     */
    virtual ~PSmartPointer();

    /** Assign this pointer to the value specified in the #ptr#
       parameter.

       The previous object being pointed to has its reference count
       decremented as this will no longer point to it. If there are no more
       references then the object is deleted.

       The new object being pointed to after the assignment has its reference
       count incremented.
     */
    // PSmartPointer & operator=(
      // const PSmartPointer & ptr  /// Smart pointer to assign.
    // );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /** Determine the relative rank of the pointers. This is identical to
       determining the relative rank of the integer values represented by the
       memory pointers.

       @return
       #EqualTo# if objects point to the same object instance,
       otherwise #LessThan# and #GreaterThan# may be
       returned depending on the relative values of the memory pointers.
     */
    virtual Comparison Compare(
      const PObject & obj   // Other smart pointer to compare against.
    ) const;
  //@}

  /**@name Pointer access functions */
  //@{
    /** Determine if the smart pointer has been set to point to an actual
       object instance.

       @return
       PTrue if the pointer is NULL.
     */
    PBoolean IsNULL() const;

    /** Get the current value if the internal smart object pointer.

       @return
       pointer to object instance.
     */
    PSmartObject * GetObject() const;
  //@}

};