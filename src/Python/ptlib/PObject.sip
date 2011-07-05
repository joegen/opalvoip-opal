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
// The root of all evil ... umm classes

/** Ultimate parent class for all objects in the class library.
This provides functionality provided to all classes, eg run-time types,
default comparison operations, simple stream I/O and serialisation support.
*/
class PObject {

%TypeHeaderCode
#include <ptlib.h>
%End

  protected:
    /** Constructor for PObject, make protected so cannot ever create one on
       its own.
     */
    PObject();

  public:
    /* Destructor required to get the "virtual". A PObject really has nothing
       to destroy.
     */
    virtual ~PObject();

    /**@name Run Time Type functions */
  //@{
    /** Get the name of the class as a C string. This is a static function which
       returns the type of a specific class. 
       
       When comparing class names, always use the #strcmp()
       function rather than comparing pointers. The pointers are not
       necessarily the same over compilation units depending on the compiler,
       platform etc.

       @return pointer to C string literal.
     */      
    static const char * Class();

    /** Get the current dynamic type of the object instance.

       When comparing class names, always use the #strcmp()
       function rather than comparing pointers. The pointers are not
       necessarily the same over compilation units depending on the compiler,
       platform etc.

       The #PCLASSINFO macro declares an override of this function for
       the particular class. The user need not implement it.

       @return pointer to C string literal.
     */
    virtual const char * GetClass(unsigned ancestor = 0) const;

    PBoolean IsClass(const char * cls) const;

    /** Determine if the dynamic type of the current instance is a descendent of
       the specified class. The class name is usually provided by the
       #Class() static function of the desired class.
    
       The #PCLASSINFO macro declares an override of this function for
       the particular class. The user need not implement it.

       @return PTrue if object is descended from the class.
     */
    virtual PBoolean InternalIsDescendant(
      const char * clsName    // Ancestor class name to compare against.
    ) const;

  //@}

  /**@name Comparison functions */
  //@{
    /** Result of the comparison operation performed by the #Compare()
       function.
      */
    enum Comparison {
      LessThan = -1,
      EqualTo = 0,
      GreaterThan = 1
    };

    /** Compare the two objects and return their relative rank. This function is
       usually overridden by descendent classes to yield the ranking according
       to the semantics of the object.
       
       The default function is to use the #CompareObjectMemoryDirect()
       function to do a byte wise memory comparison of the two objects.

       @return
       #LessThan, #EqualTo or #GreaterThan
       according to the relative rank of the objects.
     */
    virtual Comparison Compare(
      const PObject & obj   // Object to compare against.
    ) const;
    
    /** Determine the byte wise comparison of two objects. This is the default
       comparison operation for objects that do not explicitly override the
       #Compare() function.
    
       The #PCLASSINFO macro declares an override of this function for
       the particular class. The user need not implement it.

       @return
       #LessThan, #EqualTo or #GreaterThan
       according to the result #memcpy() function.
     */
    virtual Comparison CompareObjectMemoryDirect(
      const PObject & obj   // Object to compare against.
    ) const;

    /** Compare the two objects.
    
       @return
       PTrue if objects are equal.
     */
    bool operator==(
      const PObject & obj   // Object to compare against.
    ) const;

    /** Compare the two objects.
    
       @return
       PTrue if objects are not equal.
     */
    bool operator!=(
      const PObject & obj   // Object to compare against.
    ) const;

    /** Compare the two objects.
    
       @return
       PTrue if objects are less than.
     */
    bool operator<(
      const PObject & obj   // Object to compare against.
    ) const;

    /** Compare the two objects.
    
       @return
       PTrue if objects are greater than.
     */
    bool operator>(
      const PObject & obj   // Object to compare against.
    ) const;

    /** Compare the two objects.
    
       @return
       PTrue if objects are less than or equal.
     */
    bool operator<=(
      const PObject & obj   // Object to compare against.
    ) const;

    /** Compare the two objects.
    
       @return
       PTrue if objects are greater than or equal.
     */
    bool operator>=(
      const PObject & obj   // Object to compare against.
    ) const;
  //@}


  /**@name Miscellaneous functions */
  //@{
    /** Create a copy of the class on the heap. The exact semantics of the
       descendent class determine what is required to make a duplicate of the
       instance. Not all classes can even \b do a clone operation.
       
       The main user of the clone function is the #PDictionary class as
       it requires copies of the dictionary keys.

       The default behaviour is for this function to assert.

       @return
       pointer to new copy of the class instance.
     */
    virtual PObject * Clone() const;

    /** This function yields a hash value required by the #PDictionary
       class. A descendent class that is required to be the key of a dictionary
       should override this function. The precise values returned is dependent
       on the semantics of the class. For example, the #PString class
       overrides it to provide a hash function for distinguishing text strings.

       The default behaviour is to return the value zero.

       @return
       hash function value for class instance.
     */
    virtual PINDEX HashFunction() const;
  //@}
};
