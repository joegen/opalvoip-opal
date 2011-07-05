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
#include <ptclib/pnat.h>
%End

/** PNatMethod
    Base Network Address Traversal Method class
    All NAT Traversal Methods are derived off this class. 
    There are quite a few methods of NAT Traversal. The 
    only purpose of this class is to provide a common 
    interface. It is intentionally minimalistic.
*/
class PNatMethod  : PObject /Abstract/
{

  public:
  /**@name Construction */
  //@{
    /** Default Contructor
    */
    PNatMethod();

    /** Deconstructor
    */
    ~PNatMethod();
  //@}



  /**@name General Functions */
  //@{
    /** Factory Create
    */
    static PNatMethod * Create(
      const PString & name,        ///< Feature Name Expression
      PPluginManager * pluginMgr = NULL   ///< Plugin Manager
    );

    /** Get the NAT traversal method Name
    */
    virtual PString GetName() const = 0;

    /**Get the current server address name.
       Defaults to be "address:port" string form.
      */
    virtual PString GetServer() const;

    /**Get the current server address and port being used.
      */
    virtual bool GetServerAddress(
      PIPSocket::Address & address,   ///< Address of server
      WORD & port                     ///< Port server is using.
    ) const = 0;

    /** Get the acquired External IP Address.
    */
    virtual PBoolean GetExternalAddress(
      PIPSocket::Address & externalAddress, ///< External address of router
      const PTimeInterval & maxAge = 1000   ///< Maximum age for caching
    ) = 0;

    /**Return the interface NAT router is using.
      */
    virtual bool GetInterfaceAddress(
      PIPSocket::Address & internalAddress  ///< NAT router internal address returned.
    ) const = 0;

    /**Create a single socket.
       The NAT traversal protocol is used to create a socket for which the
       external IP address and port numbers are known. A PUDPSocket descendant
       is returned which will, in response to GetLocalAddress() return the
       externally visible IP and port rather than the local machines IP and
       socket.

       The will create a new socket pointer. It is up to the caller to make
       sure the socket is deleted to avoid memory leaks.

       The socket pointer is set to NULL if the function fails and returns
       false.
      */
    virtual PBoolean CreateSocket(
      PUDPSocket * & socket,
      const PIPSocket::Address & binding = PIPSocket::GetDefaultIpAny(),
      WORD localPort = 0
    ) = 0;

    /**Create a socket pair.
       The NAT traversal protocol is used to create a pair of sockets with
       adjacent port numbers for which the external IP address and port
       numbers are known. PUDPSocket descendants are returned which will, in
       response to GetLocalAddress() return the externally visible IP and port
       rather than the local machines IP and socket.

       The will create new socket pointers. It is up to the caller to make
       sure the sockets are deleted to avoid memory leaks.

       The socket pointers are set to NULL if the function fails and returns
       false.
      */
    virtual PBoolean CreateSocketPair(
      PUDPSocket * & socket1,
      PUDPSocket * & socket2,
      const PIPSocket::Address & binding = PIPSocket::GetDefaultIpAny()
    ) = 0;

    /**Create a socket pair.
       The NAT traversal protocol is used to create a pair of sockets with
       adjacent port numbers for which the external IP address and port
       numbers are known. PUDPSocket descendants are returned which will, in
       response to GetLocalAddress() return the externally visible IP and port
       rather than the local machines IP and socket.

       The will create new socket pointers. It is up to the caller to make
       sure the sockets are deleted to avoid memory leaks.

       The socket pointers are set to NULL if the function fails and returns
       false.
      */
    virtual PBoolean CreateSocketPair(
      PUDPSocket * & socket1,
      PUDPSocket * & socket2,
      const PIPSocket::Address & binding,
      void * userData
    );

    /**Returns whether the Nat Method is ready and available in
    assisting in NAT Traversal. The principal is function is
    to allow the EP to detect various methods and if a method
    is detected then this method is available for NAT traversal
    The Order of adding to the PNstStrategy determines which method
    is used
    */
    virtual bool IsAvailable(
      const PIPSocket::Address & binding = PIPSocket::GetDefaultIpAny()  ///< Interface to see if NAT is available on
    ) = 0;

    /**Acrivate
     Activate/DeActivate the NAT Method on a call by call basis
     Default does notthing
      */
    virtual void Activate(bool active);

    /**Set Alternate Candidate (ICE) or probe (H.460.24A) addresses.
       Default does nothing.
      */
    virtual void SetAlternateAddresses(
      const PStringArray & addresses,   ///< List of probe/candidates
      void * userData = NULL            ///< User data to link to NAT handler.
    );

    enum RTPSupportTypes {
      RTPSupported,
      RTPIfSendMedia,
      RTPUnsupported,
      RTPUnknown,
      NumRTPSupportTypes
    };

    /**Return an indication if the current STUN type supports RTP
    Use the force variable to guarantee an up to date test
    */
    virtual RTPSupportTypes GetRTPSupport(
      PBoolean force = false    ///< Force a new check
    ) = 0;

    /**Set the port ranges to be used on local machine.
    Note that the ports used on the NAT router may not be the same unless
    some form of port forwarding is present.

    If the port base is zero then standard operating system port allocation
    method is used.

    If the max port is zero then it will be automatically set to the port
    base + 99.
    */
    virtual void SetPortRanges(
      WORD portBase,          ///< Single socket port number base
      WORD portMax = 0,       ///< Single socket port number max
      WORD portPairBase = 0,  ///< Socket pair port number base
      WORD portPairMax = 0    ///< Socket pair port number max
    );
  //@}

};

/////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////

/** PNatStrategy
  The main container for all
  NAT traversal Strategies. 
*/

class PNatStrategy : PObject
{

public :

  /**@name Construction */
  //@{
  /** Default Contructor
  */
  PNatStrategy();

  /** Deconstructor
  */
  ~PNatStrategy();
  //@}

  /**@name Method Handling */
  //@{
  /** AddMethod
    This function is used to add the required NAT
    Traversal Method. The Order of Loading is important
    The first added has the highest priority.
  */
  void AddMethod(PNatMethod * method);

  /** GetMethod
    This function retrieves the first available NAT
    Traversal Method. If no available NAT Method is found
    then NULL is returned. 
  */
  PNatMethod * GetMethod(const PIPSocket::Address & address = PIPSocket::GetDefaultIpAny());

  /** GetMethodByName
    This function retrieves the NAT Traversal Method with the given name. 
    If it is not found then NULL is returned. 
  */
  PNatMethod * GetMethodByName(const PString & name);

  /** RemoveMethod
    This function removes a NAT method from the NATlist matching the supplied method name
   */
  PBoolean RemoveMethod(const PString & meth);

    /**Set the port ranges to be used on local machine.
       Note that the ports used on the NAT router may not be the same unless
       some form of port forwarding is present.

       If the port base is zero then standard operating system port allocation
       method is used.

       If the max port is zero then it will be automatically set to the port
       base + 99.
      */
    void SetPortRanges(
      WORD portBase,          ///< Single socket port number base
      WORD portMax = 0,       ///< Single socket port number max
      WORD portPairBase = 0,  ///< Socket pair port number base
      WORD portPairMax = 0    ///< Socket pair port number max
    );

    /** Get Loaded NAT Method List
     */
    SIP_PYLIST GetNATList();
%MethodCode
	PNatList &lst = sipCpp->GetNATList();
	PyObject *l;
	PyObject *pobj;
	SIP_SSIZE_T i=0;

	if((l = PyList_New(lst.GetSize())) == NULL)
		return NULL;

	for(PNatList::iterator iter = lst.begin(); iter != lst.end(); iter++)
	{
		PNatMethod *nat = reinterpret_cast<PNatMethod*>(iter->Clone());

		if ((pobj = sipConvertFromNewType(nat, sipType_PNatMethod, NULL)) == NULL)
		{
           // There was an error so garbage collect the Python list.
			Py_DECREF(l);
			sipIsErr = 1;
		}

	   	// Add the wrapper to the list.
		PyList_SET_ITEM(l, i++, pobj);
	}

	sipRes = l;
%End	

	PNatMethod * LoadNatMethod(const PString & name);

    static PStringArray GetRegisteredList();

  //@}

};

////////////////////////////////////////////////////////
//
// declare macros and structures needed for NAT plugins
//

template <className> 
class PNatMethodServiceDescriptor : PDevicePluginServiceDescriptor
{
  public:
    virtual PObject *    CreateInstance(int /*userData*/) const;
    virtual PStringArray GetDeviceNames(int /*userData*/) const;
    virtual bool  ValidateDeviceName(const PString & deviceName, int /*userData*/) const;
};



// End Of File ///////////////////////////////////////////////////////////////
