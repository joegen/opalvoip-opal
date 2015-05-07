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
#include <ptlib/safecoll.h>
%End


/** This class defines a thread-safe object in a collection.

  This is part of a set of classes to solve the general problem of a
  collection (eg a PList or PDictionary) of objects that needs to be a made
  thread safe. Any thread can add, read, write or remove an object with both
  the object and the database of objects itself kept thread safe.

  The act of adding a new object is simple and only requires locking the
  collection itself during the add.

  Locating an object is more complicated. The obvious lock on the collection
  is made for the initial search. But we wish to have the full collection lock
  for as short a period as possible (for performance reasons) so we lock the
  individual object and release the lock on the collection.

  A simple mutex on the object however is very dangerous as it can be (and
  should be able to be!) locked from other threads independently of the
  collection. If one of these threads subsequently needs to get at the
  collection (eg it wants to remove the object) then we will have a deadlock.
  Also, to avoid a race condition with the object begin deleted, the objects
  lock must be made while the collection lock is set. The performance gains
  are then lost as if something has the object locked for a long time, then
  another object wanting that object will actually lock the collection for a
  long time as well.

  So, an object has 4 states: unused, referenced, reading & writing. With the
  additional rider of "being removed". This flag prevents new locks from being
  acquired and waits for all locks to be relinquished before removing the
  object from the system. This prevents numerous race conditions and accesses
  to deleted objects.

  The "unused" state indicates the object exists in the collection but no
  threads anywhere is using it. It may be moved to any state by any thread
  while in this state. An object cannot be deleted (ie memory deallocated)
  until it is unused.

  The "referenced" state indicates that a thread has a reference (eg pointer)
  to the object and it should not be deleted. It may be locked for reading or
  writing at any time thereafter.

  The "reading" state is a form of lock that indicates that a thread is
  reading from the object but not writing. Multiple threads can obtain a read
  lock. Note the read lock has an implicit "reference" state in it.

  The "writing" state is a form of lock where the data in the object may
  be changed. It can only be obtained exclusively and if there are no read
  locks present. Again there is an implicit reference state in this lock.

  Note that threads going to the "referenced" state may do so regardless of
  the read or write locks present.

  Access to safe objects (especially when in a safe collection) is recommended
  to by the PSafePtr<> class which will manage reference counting and the
  automatic unlocking of objects ones the pointer goes out of scope. It may
  also be used to lock each object of a collection in turn.

  The enumeration
 */
class PSafeObject : PObject /NoDefaultCtors/
{
  public:
  /**@name Construction */
  //@{
    /**Create a thread safe object.
     */
    PSafeObject(
        PSafeObject * indirectLock = NULL ///< Other safe object to be locked when this is locked
    );
  //@}

  /**@name Operations */
  //@{
    /**Increment the reference count for object.
       This will guarantee that the object is not deleted (ie memory
       deallocated) as the caller thread is using the object, but not
       necessarily at this time locking it.

       If the function returns PFalse, then the object has been flagged for
       deletion and the calling thread should immediately cease using the
       object.

       A typical use of this would be when an entity (eg a thread) has a
       pointer to the object but is not currenty accessing the objects data.
       The LockXXX functions may be called independetly of the reference
       system and the pointer beiong used for the LockXXX call is guaranteed
       to be usable.

       It is recommended that the PSafePtr<> class is used to manage this
       rather than the application calling this function directly.
      */
    PBoolean SafeReference();

    /**Decrement the reference count for object.
       This indicates that the thread no longer has anything to do with the
       object and it may be deleted (ie memory deallocated).

       It is recommended that the PSafePtr<> class is used to manage this
       rather than the application calling this function directly.

       @return PTrue if reference count has reached zero and is not being
               safely deleted elsewhere ie SafeRemove() not called
      */
    PBoolean SafeDereference();

    /**Lock the object for Read Only access.
       This will lock the object in read only mode. Multiple threads may lock
       the object read only, but only one thread can lock for read/write.
       Also, no read only threads can be present for the read/write lock to
       occur and no read/write lock can be present for any read only locks to
       occur.

       If the function returns PFalse, then the object has been flagged for
       deletion and the calling thread should immediately cease use of the
       object, possibly executing the SafeDereference() function to remove
       any references it may have acquired.

       It is expected that the caller had already called the SafeReference()
       function (directly or implicitly) before calling this function. It is
       recommended that the PSafePtr<> class is used to automatically manage
       the reference counting and locking of objects.
      */
    PBoolean LockReadOnly() const;

    /**Release the read only lock on an object.
       Unlock the read only mutex that a thread had obtained. Multiple threads
       may lock the object read only, but only one thread can lock for
       read/write. Also, no read only threads can be present for the
       read/write lock to occur and no read/write lock can be present for any
       read only locks to occur.

       It is recommended that the PSafePtr<> class is used to automatically
       manage the reference counting and unlocking of objects.
      */
    void UnlockReadOnly() const;

    /**Lock the object for Read/Write access.
       This will lock the object in read/write mode. Multiple threads may lock
       the object read only, but only one thread can lock for read/write.
       Also no read only threads can be present for the read/write lock to
       occur and no read/write lock can be present for any read only locks to
       occur.

       If the function returns PFalse, then the object has been flagged for
       deletion and the calling thread should immediately cease use of the
       object, possibly executing the SafeDereference() function to remove
       any references it may have acquired.

       It is expected that the caller had already called the SafeReference()
       function (directly or implicitly) before calling this function. It is
       recommended that the PSafePtr<> class is used to automatically manage
       the reference counting and locking of objects.
      */
    PBoolean LockReadWrite();

    /**Release the read/write lock on an object.
       Unlock the read/write mutex that a thread had obtained. Multiple threads
       may lock the object read only, but only one thread can lock for
       read/write. Also, no read only threads can be present for the
       read/write lock to occur and no read/write lock can be present for any
       read only locks to occur.

       It is recommended that the PSafePtr<> class is used to automatically
       manage the reference counting and unlocking of objects.
      */
    void UnlockReadWrite();

    /**Set the removed flag.
       This flags the object as beeing removed but does not physically delete
       the memory being used by it. The SafelyCanBeDeleted() can then be used
       to determine when all references to the object have been released so it
       may be safely deleted.

       This is typically used by the PSafeCollection class and is not expected
       to be used directly by an application.
      */
    void SafeRemove();

    /**Determine if the object can be safely deleted.
       This determines if the object has been flagged for deletion and all
       references to it have been released.

       This is typically used by the PSafeCollection class and is not expected
       to be used directly by an application.
      */
    PBoolean SafelyCanBeDeleted() const;

    /**Do any garbage collection that may be required by the object so that it
       may be finally deleted. This is especially useful if there a references
       back to this object which this object is in charge of disposing of. This
       reference "glare" is to be resolved by this function being called every
       time the owner collection is cleaning up, causing a cascade of clean ups
       that might need to be required.

       Default implementation simply returns true.

       @return true if object may be deleted.
      */
    virtual bool GarbageCollection();
  //@}

};


/**Lock a PSafeObject for read only and automatically unlock it when go out of scope.
  */
class PSafeLockReadOnly
{
  public:
    PSafeLockReadOnly(const PSafeObject & object);
    ~PSafeLockReadOnly();
    PBoolean Lock();
    void Unlock();
    PBoolean IsLocked() const;
    // bool operator!() const;
	
	int __nonzero__();
%MethodCode
	return !sipCpp;
%End

};



/**Lock a PSafeObject for read/write and automatically unlock it when go out of scope.
  */
class PSafeLockReadWrite
{
  public:
    PSafeLockReadWrite(const PSafeObject & object);
    ~PSafeLockReadWrite();
    PBoolean Lock();
    void Unlock();
    PBoolean IsLocked() const;
    // bool operator!() const;
	
	int __nonzero__();
%MethodCode
	return !sipCpp;
%End

};



/** This class defines a thread-safe collection of objects.
  This class is a wrapper around a standard PCollection class which allows
  only safe, mutexed, access to the collection.

  This is part of a set of classes to solve the general problem of a
  collection (eg a PList or PDictionary) of objects that needs to be a made
  thread safe. Any thread can add, read, write or remove an object with both
  the object and the database of objects itself kept thread safe.

  See the PSafeObject class for more details. Especially in regard to
  enumeration of collections.
 */
class PSafeCollection : PObject /NoDefaultCtors/
{
  public:
  /**@name Construction */
  //@{
    /**Create a thread safe collection of objects.
       Note the collection is automatically deleted on destruction.
     */
    PSafeCollection(
      PCollection * collection    ///< Actual collection of objects
     );

    /**Destroy the thread safe collection.
       The will delete the collection object provided in the constructor.
      */
    ~PSafeCollection();
  //@}

  /**@name Operations */
  //@{
  protected:
    /**Remove an object to the collection.
       This function removes the object from the collection itself, but does
       not actually delete the object. It simply moves the object to a list
       of objects to be garbage collected at a later time.

       As for Append() full mutual exclusion locking on the collection itself
       is maintained.
      */
    virtual PBoolean SafeRemove(
      PSafeObject * obj   ///< Object to remove from collection
    );

    /**Remove an object to the collection.
       This function removes the object from the collection itself, but does
       not actually delete the object. It simply moves the object to a list
       of objects to be garbage collected at a later time.

       As for Append() full mutual exclusion locking on the collection itself
       is maintained.
      */
    virtual PBoolean SafeRemoveAt(
      PINDEX idx    ///< Object index to remove
    );

  public:
    /**Remove all objects in collection.
      */
    virtual void RemoveAll(
      PBoolean synchronous = PFalse  ///< Wait till objects are deleted before returning
    );

    /**Disallow the automatic delete any objects that have been removed.
       Objects are simply removed from the collection and not marked for
       deletion using PSafeObject::SafeRemove() and DeleteObject().
      */
    void AllowDeleteObjects(
      PBoolean yes = PTrue   ///< New value for flag for deleting objects
    );

    /**Disallow the automatic delete any objects that have been removed.
       Objects are simply removed from the collection and not marked for
       deletion using PSafeObject::SafeRemove() and DeleteObject().
      */
    void DisallowDeleteObjects();

    /**Delete any objects that have been removed.
       Returns PTrue if all objects in the collection have been removed and
       their pending deletions carried out.
      */
    virtual PBoolean DeleteObjectsToBeRemoved();

    /**Delete an objects that has been removed.
      */
    virtual void DeleteObject(PObject * object) const;

    /**Start a timer to automatically call DeleteObjectsToBeRemoved().
      */
    virtual void SetAutoDeleteObjects();

    /**Get the current size of the collection.
       Note that usefulness of this function is limited as it is merely an
       instantaneous snapshot of the state of the collection.
      */
    PINDEX GetSize() const;

    /**Determine if the collection is empty.
       Note that usefulness of this function is limited as it is merely an
       instantaneous snapshot of the state of the collection.
      */
    PBoolean IsEmpty() const;

    /**Get the mutex for the collection.
      */
    const PMutex & GetMutex() const;
  //@}
  
	protected:
		void SafeRemoveObject(PSafeObject * obj);
};


enum PSafetyMode {
  PSafeReference,
  PSafeReadOnly,
  PSafeReadWrite
};

/** This class defines a base class for thread-safe pointer to an object.

  This is part of a set of classes to solve the general problem of a
  collection (eg a PList or PDictionary) of objects that needs to be a made
  thread safe. Any thread can add, read, write or remove an object with both
  the object and the database of objects itself kept thread safe.

  NOTE: the PSafePtr will allow safe and mutexed access to objects but is not
  thread safe itself! You should not share PSafePtr instances across threads.

  See the PSafeObject class for more details.
 */
class PSafePtrBase : PObject
{

  /**@name Construction */
  //@{
  protected:
    /**Create a new pointer to a PSafeObject.
       An optional locking mode may be provided to lock the object for reading
       or writing and automatically unlock it on destruction.

       Note that this version is not associated with a collection so the ++
       and -- operators will not work.
     */
    PSafePtrBase(
      PSafeObject * obj = NULL,         ///< Physical object to point to.
      PSafetyMode mode = PSafeReference ///< Locking mode for the object
    );

    /**Create a new pointer to a PSafeObject.
       An optional locking mode may be provided to lock the object for reading
       or writing and automatically unlock it on destruction.

       The idx'th entry of the collection is pointed to by this object. If the
       idx is beyond the size of the collection, the pointer is NULL.
     */
    PSafePtrBase(
      const PSafeCollection & safeCollection, ///< Collection pointer will enumerate
      PSafetyMode mode,                       ///< Locking mode for the object
      PINDEX idx                              ///< Index into collection to point to
    );

    /**Create a new pointer to a PSafeObject.
       An optional locking mode may be provided to lock the object for reading
       or writing and automatically unlock it on destruction.

       The obj parameter is only set if it contained in the collection,
       otherwise the pointer is NULL.
     */
    PSafePtrBase(
      const PSafeCollection & safeCollection, ///< Collection pointer will enumerate
      PSafetyMode mode,                       ///< Locking mode for the object
      PSafeObject * obj                       ///< Inital object in collection to point to
    );

    /**Copy the pointer to the PSafeObject.
       This will create a copy of the pointer with the same locking mode and
       lock on the PSafeObject. It will also increment the reference count on
       the PSafeObject as well.
      */
    PSafePtrBase(
      const PSafePtrBase & enumerator   ///< Pointer to copy
    );

  public:
    /**Unlock and dereference the PSafeObject this is pointing to.
      */
    ~PSafePtrBase();
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare the pointers.
       Note this is not a value comparison and will only return EqualTo if the
       two PSafePtrBase instances are pointing to the same instance.
      */
    virtual Comparison Compare(
      const PObject & obj   ///< Other instance to compare against
    ) const;
  //@}

  /**@name Operations */
  //@{
    /**Set the pointer to NULL, unlocking/dereferencing existing pointer value.
      */
    virtual void SetNULL();

    /**Return true if pointer is NULL.
      */
    int __nonzero__();
%MethodCode
	return !sipCpp;
%End

    /**Get the locking mode used by this pointer.
      */
    PSafetyMode GetSafetyMode() const;

    /**Change the locking mode used by this pointer.

       If the function returns false, then the object has been flagged for
       deletion and the calling thread should immediately cease use of the
       object. This instance pointer will be set to NULL.
      */
    virtual PBoolean SetSafetyMode(
      PSafetyMode mode  ///< New locking mode
    );

    /**Get the associated collection this pointer may be contained in.
      */
    const PSafeCollection * GetCollection() const;
  //@}

    virtual void Assign(const PSafePtrBase & ptr);
    virtual void Assign(const PSafeCollection & safeCollection);
    virtual void Assign(PSafeObject * obj);
    virtual void Assign(PINDEX idx);

  protected:
    virtual void Next();
    virtual void Previous();
    virtual void DeleteObject(PSafeObject * obj);

    enum EnterSafetyModeOption {
      WithReference,
      AlreadyReferenced
    };
    PBoolean EnterSafetyMode(EnterSafetyModeOption ref);

    enum ExitSafetyModeOption {
      WithDereference,
      NoDereference
    };
    void ExitSafetyMode(ExitSafetyModeOption ref);

};



/** This class defines a base class for thread-safe pointer to an object.

  This is part of a set of classes to solve the general problem of a
  collection (eg a PList or PDictionary) of objects that needs to be a made
  thread safe. Any thread can add, read, write or remove an object with both
  the object and the database of objects itself kept thread safe.

  NOTE: unlikel PSafePtrBase, pointers based on this class are thread safe
  themseleves, at the expense of performance on every operation.

  See the PSafeObject class for more details.
 */
class PSafePtrMultiThreaded : PSafePtrBase /Abstract, NoDefaultCtors/
{

  /**@name Construction */
  //@{
  protected:
    /**Create a new pointer to a PSafeObject.
       An optional locking mode may be provided to lock the object for reading
       or writing and automatically unlock it on destruction.

       Note that this version is not associated with a collection so the ++
       and -- operators will not work.
     */
    PSafePtrMultiThreaded(
      PSafeObject * obj = NULL,         ///< Physical object to point to.
      PSafetyMode mode = PSafeReference ///< Locking mode for the object
    );

    /**Create a new pointer to a PSafeObject.
       An optional locking mode may be provided to lock the object for reading
       or writing and automatically unlock it on destruction.

       The idx'th entry of the collection is pointed to by this object. If the
       idx is beyond the size of the collection, the pointer is NULL.
     */
    PSafePtrMultiThreaded(
      const PSafeCollection & safeCollection, ///< Collection pointer will enumerate
      PSafetyMode mode,                       ///< Locking mode for the object
      PINDEX idx                              ///< Index into collection to point to
    );

    /**Create a new pointer to a PSafeObject.
       An optional locking mode may be provided to lock the object for reading
       or writing and automatically unlock it on destruction.

       The obj parameter is only set if it contained in the collection,
       otherwise the pointer is NULL.
     */
    PSafePtrMultiThreaded(
      const PSafeCollection & safeCollection, ///< Collection pointer will enumerate
      PSafetyMode mode,                       ///< Locking mode for the object
      PSafeObject * obj                       ///< Inital object in collection to point to
    );

    /**Copy the pointer to the PSafeObject.
       This will create a copy of the pointer with the same locking mode and
       lock on the PSafeObject. It will also increment the reference count on
       the PSafeObject as well.
      */
    PSafePtrMultiThreaded(
      const PSafePtrMultiThreaded & enumerator   ///< Pointer to copy
    );

  public:
    /**Unlock and dereference the PSafeObject this is pointing to.
      */
    ~PSafePtrMultiThreaded();
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare the pointers.
       Note this is not a value comparison and will only return EqualTo if the
       two PSafePtrBase instances are pointing to the same instance.
      */
    virtual Comparison Compare(
      const PObject & obj   ///< Other instance to compare against
    ) const;
  //@}

  /**@name Operations */
  //@{
    /**Set the pointer to NULL, unlocking/dereferencing existing pointer value.
      */
    virtual void SetNULL();

    /**Change the locking mode used by this pointer.

       If the function returns false, then the object has been flagged for
       deletion and the calling thread should immediately cease use of the
       object. This instance pointer will be set to NULL.
      */
    virtual PBoolean SetSafetyMode(
      PSafetyMode mode  ///< New locking mode
    );
  //@}

    virtual void Assign(const PSafePtrMultiThreaded & ptr /NoCopy/);
    virtual void Assign(const PSafePtrBase & ptr /NoCopy/);
    virtual void Assign(const PSafeCollection & safeCollection /NoCopy/);
    virtual void Assign(PSafeObject * obj);
    virtual void Assign(PINDEX idx);

  protected:
    virtual void Next();
    virtual void Previous();

};


/** This class defines a thread-safe enumeration of object in a collection.

  This is part of a set of classes to solve the general problem of a
  collection (eg a PList or PDictionary) of objects that needs to be a made
  thread safe. Any thread can add, read, write or remove an object with both
  the object and the database of objects itself kept thread safe.

  There are two modes of safe pointer, one that is enumerating a collection
  and one that is independent of the collection that the safe object is in.
  There are some subtle semantics that must be observed in each of these two
  modes especially when switching from one to the other.

  NOTE: the PSafePtr will allow safe and mutexed access to objects but may not
  thread safe itself! This depends on the base calass being used. If the more
  efficiant PSafePtrBase class is used you should not share PSafePtr instances
  across threads.

  See the PSafeObject class for more details. Especially in regard to
  enumeration of collections.
 */
template <T> 
class PSafePtr : PSafePtrBase
{
  public:
  /**@name Construction */
  //@{
    /**Create a new pointer to a PSafeObject.
       An optional locking mode may be provided to lock the object for reading
       or writing and automatically unlock it on destruction.

       Note that this version is not associated with a collection so the ++
       and -- operators will not work.
     */
    PSafePtr(
      T * obj = NULL,                   ///< Physical object to point to.
      PSafetyMode mode = PSafeReference ///< Locking mode for the object
    );

    /**Create a new pointer to a PSafeObject.
       An optional locking mode may be provided to lock the object for reading
       or writing and automatically unlock it on destruction.

       The idx'th entry of the collection is pointed to by this object. If the
       idx is beyond the size of the collection, the pointer is NULL.
     */
    PSafePtr(
      const PSafeCollection & safeCollection, ///< Collection pointer will enumerate
      PSafetyMode mode = PSafeReadWrite,      ///< Locking mode for the object
      PINDEX idx = 0                          ///< Index into collection to point to
    );

    /**Create a new pointer to a PSafeObject.
       An optional locking mode may be provided to lock the object for reading
       or writing and automatically unlock it on destruction.

       The obj parameter is only set if it contained in the collection,
       otherwise the pointer is NULL.
     */
    PSafePtr(
      const PSafeCollection & safeCollection, ///< Collection pointer will enumerate
      PSafetyMode mode,                       ///< Locking mode for the object
      PSafeObject * obj                       ///< Inital object in collection to point to
    );

    /**Copy the pointer to the PSafeObject.
       This will create a copy of the pointer with the same locking mode and
       lock on the PSafeObject. It will also increment the reference count on
       the PSafeObject as well.
      */
    PSafePtr(
      const PSafePtr & ptr   ///< Pointer to copy
    );


    /**Set the new pointer to a collection index.
       This will set the pointer to the new object to the index entry in the
       colelction that the pointer was created with. The old object pointed to
       will be unlocked and dereferenced and the new object referenced and set
       to the same locking mode as the previous pointer value.

       If the idx'th object is not in the collection, then the safe pointer
       is set to NULL.
     */
    // PSafePtr & operator=(PINDEX idx);
  //@}

  /**@name Operations */
  //@{
    /**Return the physical pointer to the object.
      */
	  
	SIP_PYOBJECT __getattr__();
%MethodCode
	PyObject* p;
	T& obj = *sipCpp;
	
	if( (p = sipConvertFromType(&obj, sipType_T, NULL)) != NULL)
		sipRes = p;
	else
		*sipErr = sipErrorFail;
%End
	
	
    // operator T*()    const;

    /**Return the physical pointer to the object.
      */
    // T & operator*()  const;

    /**Allow access to the physical object the pointer is pointing to.
      */
    // T * operator->() const;

    /**Post-increment the pointer.
       This requires that the pointer has been created with a PSafeCollection
       object so that it can enumerate the collection.
      */
    // T * operator++(int);

    /**Pre-increment the pointer.
       This requires that the pointer has been created with a PSafeCollection
       object so that it can enumerate the collection.
      */
    // T * operator++();

    /**Post-decrement the pointer.
       This requires that the pointer has been created with a PSafeCollection
       object so that it can enumerate the collection.
      */
    // T * operator--(int);

    /**Pre-decrement the pointer.
       This requires that the pointer has been created with a PSafeCollection
       object so that it can enumerate the collection.
      */
    // T * operator--();
  //@}

};


// End Of File ///////////////////////////////////////////////////////////////
