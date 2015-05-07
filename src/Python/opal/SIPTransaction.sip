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

%Module(name=PyQt4.QtGui, keyword_arguments="Optional")

/////////////////////////////////////////////////////////////////////////
// SIPTransaction

/** Session Initiation Protocol transaction.
    A transaction is a stateful independent entity that provides services to
    a connection (Transaction User). Transactions are contained within 
    connections.
    A client transaction handles sending a request and receiving its
    responses.
    A server transaction handles sending responses to a received request.
    In either case the SIP_PDU ancestor is the sent or received request.
 */

class SIPTransaction  /Abstract/
  public:
	 SIPTransaction();
    ~SIPTransaction();
	
	//We do not really need to wrap this function, since it's defined in subclasses.
    virtual SIPTransaction * CreateDuplicate() const = 0;

    bool IsTrying()     const;
    bool IsProceeding() const;
    bool IsInProgress() const;
    bool IsFailed()     const;
    bool IsCompleted()  const;
    bool IsCanceled()   const;

    void WaitForCompletion();
    void Abort();

/*   protected:

    enum States {
      NotStarted,
      Trying,
      Proceeding,
      Cancelling,
      Completed,
      Terminated_Success,
      Terminated_Timeout,
      Terminated_RetriesExceeded,
      Terminated_TransportError,
      Terminated_Cancelled,
      Terminated_Aborted,
      NumStates
    };
    virtual void SetTerminated(States newState); */
};

class SIPInvite : SIPTransaction
{
  public:
    virtual SIPTransaction * CreateDuplicate() const;
};

