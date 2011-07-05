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

/** Abstract class to embody the base functionality of a <i>container</i>.

Fundamentally, a container is an object that contains other objects. There
are two main areas of support for tha that are provided by this class. The
first is simply to keep a count of the number of things that the container
contains. The current size is stored and accessed by members of this class.
The setting of size is determined by the semantics of the descendent class
and so is a pure function.

The second area of support is for reference integrity. When an instance of
a container is copied to another instance, the two instance contain the
same thing. There can therefore be multiple references to the same things.
When one reference is destroyed this must; destroy the contained
object as it may be referenced by another instance of a container class.
To this end a reference count is provided by the PContainer class. This
assures that the container only destroys the objects it contains when there
are no more references to them.

In support of this, descendent classes must provide a #DestroyContents()#
function. As the normal destructor cannot be used, this function will free
the memory or unlock the resource the container is wrapping.
*/
class PContainer : PObject
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib\contain.h>
%End
  public:
  /**@name Construction */
  //@{
    /**Create a new unique container.
     */
    PContainer(
      PINDEX initialSize = 0  ///< Initial number of things in the container.
    );

    /**Create a new refernce to container.
       Create a new container referencing the same contents as the container
       specified in the parameter.
     */
    PContainer(
      const PContainer & cont  ///< Container to create a new reference from.
    );

    /**Assign one container reference to another.
       Set the current container to reference the same thing as the container
       specified in the parameter.

       Note that the old contents of the container is dereferenced and if
       it was unique, destroyed using the DestroyContents() function.
     */
    // PContainer & operator=(
      // const PContainer & cont  //< Container to create a new reference from.
    // );

    /**Destroy the container class.
       This will decrement the reference count on the contents and if unique,
       will destroy it using the #DestroyContents()# function.
     */
    virtual ~PContainer()
   ;

  //@}

  /**@name Common functions for containers */
  //@{
    /**Get the current size of the container.
       This represents the number of things the container contains. For some
       types of containers this will always return 1.

       @return number of objects in container.
     */
    virtual PINDEX GetSize() const;

    /**Set the new current size of the container.
       The exact behavious of this is determined by the descendent class. For
       instance an array class would reallocate memory to make space for the
       new number of elements.

       Note for some types of containers this does not do anything as they
       inherently only contain one item. The function returns PTrue always and
       the new value is ignored.

       @return
       PTrue if the size was successfully changed. The value PFalse usually
       indicates failure due to insufficient memory.
     */
    virtual PBoolean SetSize(
      PINDEX newSize  ///< New size for the container.
    ) = 0;

    /**Set the minimum size of container.
       This function will set the size of the object to be at least the size
       specified. The #SetSize()# function is always called, either with the
       new value or the previous size, whichever is the larger.
     */
    PBoolean SetMinSize(
      PINDEX minSize  ///< Possible, new size for the container.
    );

    /**Determine if the container is empty.
       Determine if the container that this object references contains any
       elements.

       @return PTrue if #GetSize()# returns zero.
     */
    virtual PBoolean IsEmpty() const;

    /**Determine if container is unique reference.
       Determine if this instance is the one and only reference to the
       container contents.

       @return PTrue if the reference count is one.
     */
    PBoolean IsUnique() const;

    /**Make this instance to be the one and only reference to the container
       contents. This implicitly does a clone of the contents of the container
       to make a unique reference. If the instance was already unique then
       the function does nothing.

       @return
       PTrue if the instance was already unique.
     */
    virtual PBoolean MakeUnique();
  //@}

  protected:
    /**Constructor used in support of the Clone() function. This creates a
       new unique reference of a copy of the contents. It does;
       create another reference.

       The dummy parameter is there to prevent the contructor from being
       invoked automatically by the compiler when a pointer is used by accident
       when a normal instance or reference was expected. The container would
       be silently cloned and the copy used instead of the container expected
       leading to unpredictable results.
     */
    PContainer(
      int dummy,        ///< Dummy to prevent accidental use of the constructor.
      const PContainer * cont  ///< Container class to clone.
    );

    /**Destroy the container contents. This function must be defined by the
       descendent class to do the actual destruction of the contents. It is
       automatically declared when the #PDECLARE_CONTAINER()# macro is used.

       For all descendent classes not immediately inheriting off the PContainer
       itself, the implementation of DestroyContents() should always call its
       ancestors function. This is especially relevent if many of the standard
       container classes, such as arrays, are descended from as memory leaks
       will occur.
     */
    virtual void DestroyContents() = 0;

    /**Copy the container contents. This copies the contents from one reference
       to another.

       No duplication of contents occurs, for instance if the container is an
       array, the pointer to the array memory is copied, not the array memory
       block itself.

       This function will get called by the base assignment operator.
     */
    virtual void AssignContents(const PContainer & c);

    /**Copy the container contents. This copies the contents from one reference
       to another. It is automatically declared when the #PDECLARE_CONTAINER()#
       macro is used.
       
       No duplication of contents occurs, for instance if the container is an
       array, the pointer to the array memory is copied, not the array memory
       block itself.

       This function will get called once for every class in the heirarchy, so
       the ancestor function should; be called.
     */
    void CopyContents(const PContainer & c);

    /**Create a duplicate of the container contents. This copies the contents
       from one container to another, unique container. It is automatically
       declared when the #PDECLARE_CONTAINER()# macro is used.
       
       This class will duplicate the contents completely, for instance if the
       container is an array, the actual array memory is copied, not just the
       pointer. If the container contains objects that descend from #PObject#,
       they too should also be cloned and not simply copied.

       This function will get called once for every class in the heirarchy, so
       the ancestor function should; be called.
       
       <i><b>Note well</b></i>, the logic of the function must be able to
       accept the passed in parameter to clone being the same instance as the
       destination object, ie during execution #this == src#.
     */
    void CloneContents(const PContainer * src);

    /**Internal function called from container destructors. This will
       conditionally call #DestroyContents()# to destroy the container contents.
     */
    void Destruct();
};

