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
// Abstract collection of objects class

/**A collection is a container that collects together descendents of the
   #PObject# class. The objects contained in the collection are always
   pointers to objects, not the objects themselves. The life of an object in
   the collection should be carefully considered. Typically, it is allocated
   by the user of the collection when it is added. The collection then
   automatically deletes it when it is removed or the collection is destroyed,
   ie when the container class has no more references to the collection. Other
   models may be accommodated but it is up to the programmer to determine the
   scope and life of the objects.

   The exact form of the collection depends on the descendent of PCollection
   and determines the access modes for the objects in it. Thus a collection
   can be an array which allows fast random access at the expense of slow
   insertion and deletion. Or the collection may be a list which has fast
   insertion and deletion but very slow random access.

   The basic paradigm of all collections is the "virtual array". Regardless of
   the internal implementation of the collection; array, list, sorted list etc,
   the user may access elements via an ordinal index. The implementation then
   optimises the access as best it can. For instance, in a list ordinal zero
   will go directly to the head of the list. Stepping along sequential indexes
   then will return the next element of the list, remembering the new position
   at each step, thus allowing sequential access with little overhead as is
   expected for lists. If a random location is specified, then the list
   implementation must sequentially search for that ordinal from either the
   last location or an end of the list, incurring an overhead.

   All collection classes implement a base set of functions, though they may
   be meaningless or degenerative in some collection types eg #Insert()#
   for #PSortedList# will degenerate to be the same as #Append()#.
 */
class PCollection : PContainer
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib\contain.h>
%End
  public:
  /**@name Construction */
  //@{
    /**Create a new collection
     */
    PCollection(
      PINDEX initialSize = 0  ///< Initial number of things in the collection.
    );
  //@}


  /**@name Common functions for collections */
  //@{
    /**Append a new object to the collection.
    
       The exact semantics depends on the specific type of the collection. So
       the function may not place the object at the "end" of the collection at
       all. For example, in a #PSortedList# the object is placed in the
       correct ordinal position in the list.

       @return index of the newly added object.
     */
    virtual PINDEX Append(
      PObject * obj   ///< New object to place into the collection.
    ) = 0;

    /**Insert a new object immediately before the specified object. If the
       object to insert before is not in the collection then the equivalent of
       the #Append()# function is performed.
       
       The exact semantics depends on the specific type of the collection. So
       the function may not place the object before the specified object at
       all. For example, in a #PSortedList# the object is placed in the
       correct ordinal position in the list.

       Note that the object values are compared for the search of the
       #before# parameter, not the pointers. So the objects in the
       collection must correctly implement the #PObject::Compare()#
       function.

       @return index of the newly inserted object.
     */
    virtual PINDEX Insert(
      const PObject & before,   ///< Object value to insert before.
      PObject * obj             ///< New object to place into the collection.
    ) = 0;

    /**Insert a new object at the specified ordinal index. If the index is
       greater than the number of objects in the collection then the
       equivalent of the #Append()# function is performed.

       The exact semantics depends on the specific type of the collection. So
       the function may not place the object at the specified index at all.
       For example, in a #PSortedList# the object is placed in the correct
       ordinal position in the list.

       @return index of the newly inserted object.
     */
    virtual PINDEX InsertAt(
      PINDEX index,   ///< Index position in collection to place the object.
      PObject * obj   ///< New object to place into the collection.
    ) = 0;

    /**Remove the object from the collection. If the AllowDeleteObjects option
       is set then the object is also deleted.

       Note that the comparison for searching for the object in collection is
       made by pointer, not by value. Thus the parameter must point to the
       same instance of the object that is in the collection.

       @return PTrue if the object was in the collection.
     */
    virtual PBoolean Remove(
      const PObject * obj   ///< Existing object to remove from the collection.
    ) = 0;

    /**Remove the object at the specified ordinal index from the collection.
       If the AllowDeleteObjects option is set then the object is also deleted.

       Note if the index is beyond the size of the collection then the
       function will assert.

       @return pointer to the object being removed, or NULL if it was deleted.
     */
    virtual PObject * RemoveAt(
      PINDEX index   ///< Index position in collection to place the object.
    ) = 0;

    /**Remove all of the elements in the collection. This operates by
       continually calling #RemoveAt()# until there are no objects left.

       The objects are removed from the last, at index
       #(GetSize()-1)# toward the first at index zero.
     */
    virtual void RemoveAll();

    /**Set the object at the specified ordinal position to the new value. This
       will overwrite the existing entry. If the AllowDeleteObjects option is
       set then the old object is also deleted.

       The exact semantics depends on the specific type of the collection. For
       some, eg #PSortedList#, the object inserted will not stay at the
       ordinal position. Also the exact behaviour when the index is greater
       than the size of the collection depends on the collection type, eg in
       an array collection the array is expanded to accommodate the new index,
       whereas in a list it will return PFalse.

       @return PTrue if the object was successfully added.
     */
    virtual PBoolean SetAt(
      PINDEX index,   ///< Index position in collection to set.
      PObject * val   ///< New value to place into the collection.
    ) = 0;

    /**Get the object at the specified ordinal position. If the index was
       greater than the size of the collection then NULL is returned.

       @return pointer to object at the specified index.
     */
    virtual PObject * GetAt(
      PINDEX index  ///< Index position in the collection of the object.
    ) const = 0;

    /**Search the collection for the specific instance of the object. The
       object pointers are compared, not the values. The fastest search
       algorithm is employed depending on the collection type.

       @return ordinal index position of the object, or P_MAX_INDEX.
     */
    virtual PINDEX GetObjectsIndex(
      const PObject * obj  ///< Object to search for.
    ) const = 0;

    /**Search the collection for the specified value of the object. The object
       values are compared, not the pointers.  So the objects in the
       collection must correctly implement the #PObject::Compare()#
       function. The fastest search algorithm is employed depending on the
       collection type.

       @return ordinal index position of the object, or P_MAX_INDEX.
     */
    virtual PINDEX GetValuesIndex(
      const PObject & obj  ///< Object to search for.
    ) const = 0;

    /**Allow or disallow the deletion of the objects contained in the
       collection. If PTrue then whenever an object is removed, overwritten or
       the colelction is deleted due to all references being destroyed, the
       object is deleted.

       For example:
<code>
              coll.SetAt(2, new PString("one"));
              coll.SetAt(2, new PString("Two"));
</code>
       would automatically delete the string containing "one" on the second
       call to SetAt().
     */
    void AllowDeleteObjects(
      PBoolean yes = PTrue   ///< New value for flag for deleting objects
    );

    /**Disallow the deletion of the objects contained in the collection. See
       the #AllowDeleteObjects()# function for more details.
     */
    void DisallowDeleteObjects();
  //@}

  protected:
    /**Constructor used in support of the Clone() function. This creates a
       new unique reference of a copy of the contents. It does {\b not}
       create another reference.

       The dummy parameter is there to prevent the contructor from being
       invoked automatically by the compiler when a pointer is used by accident
       when a normal instance or reference was expected. The container would
       be silently cloned and the copy used instead of the container expected
       leading to unpredictable results.
     */
    PCollection(
      int dummy,        ///< Dummy to prevent accidental use of the constructor.
      const PCollection * coll  ///< Collection class to clone.
    );
};

