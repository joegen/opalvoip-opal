/*
 * h4601.h
 *
 * Virteos H.460 Implementation for the h323plus Library.
 *
 * Virteos is a Trade Mark of ISVO (Asia) Pte Ltd.
 *
 * Copyright (c) 2004 ISVO (Asia) Pte Ltd. All Rights Reserved.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the General Public License (the  "GNU License"), in which case the
 * provisions of GNU License are applicable instead of those
 * above. If you wish to allow use of your version of this file only
 * under the terms of the GNU License and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GNU License. If you do not delete
 * the provisions above, a recipient may use your version of this file
 * under either the MPL or the GNU License."
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is derived from and used in conjunction with the 
 * OpenH323 Project (www.openh323.org/)
 *
 * The Initial Developer of the Original Code is ISVO (Asia) Pte Ltd.
 *
 * Contributor(s): Many thanks to Simon Horne.
 *                 Robert Jongbloed (robertj@voxlucida.com.au).
*/

#ifndef OPAL_H460_H4601_H
#define OPAL_H460_H4601_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#if OPAL_H460

#include <asn/h225.h>
#include <h323/transaddr.h>
#include <opal/guid.h>
#include <h460/h460.h>
#include <ptlib/pluginmgr.h>
#include <ptclib/url.h>


class H323EndPoint;
class H323Connection;
class H460_Feature;


///////////////////////////////////////////////////////////////////////////////

/**This is a base class for H.460 FeatureID.
  Feature ID's can be in 3 formats:
    Standard     : h460 series feature unsigned values;
    OID         : Array of unsigned values;
    NonStandard  : PString Value

   The Derived Feature ID used as an index of the H460_FeatureContent
   is used to describe the parameters of a Feature.
  */
class H460_FeatureID : public H225_GenericIdentifier
{
    /* Due to some naughty reinterpret_cast<> of H225_GenericIdentifier to
       H460_FeatureID in various palces, do not add members or do any virtual
       function overrides in this class! */
  public:
  /**@name Construction */
  //@{
    /** Blank Feature
    */
    H460_FeatureID();

    /** Standard Feature ID 
    */
    H460_FeatureID(unsigned id);

    /** OID Feature ID 
    */
    H460_FeatureID(const PASN_ObjectId & id);

    /** NonStandard Feature ID 
    */
    H460_FeatureID(const PString & id);

    /** Generic Feature ID 
    */
    H460_FeatureID(const H225_GenericIdentifier & id);
  //@}

  /**@name Operators */
  //@{
    /** Get feature OID.
        For standard feature ID, this will be 0.0.8.460.XXXX.0.1
      */
    PString GetOID() const;

    /** Standard Feature ID 
    */
    operator unsigned () const { return (const PASN_Integer &)*this; }

    /** OID Feature ID 
    */
    operator PString () const { return ((const H225_GloballyUniqueID &)*this).AsString(); }
  //@}
};


///////////////////////////////////////////////////////////////////////////////

/**This is a base class for H.323 Feature handling.
   This implements the service class session management as per H460 Series.
  */

class H460_FeatureContent : public H225_Content
{
    /* Due to some naughty reinterpret_cast<> of H225_GenericIdentifier to
       H460_FeatureID in various palces, do not add members or do any virtual
       function overrides in this class! */
  public:
    /**@name Construction */
    //@{
    /** Blank Feature
    */
    H460_FeatureContent();

    /** PASN_OctetString Raw information which can 
      be deseminated by Derived Class
    */
    H460_FeatureContent(const PASN_OctetString & param);

    /** String Value
    */
    H460_FeatureContent(const PString & param);

    /** Blank Feature
    */
    H460_FeatureContent(const PASN_BMPString & param);

    /** Boolean Value
    */
    H460_FeatureContent(bool param);

    /** Integer Value
    */
    H460_FeatureContent(unsigned param, unsigned len);

    /** Feature Identifier
    */
    H460_FeatureContent(const H460_FeatureID & id);

    /** Alias Address Raw
    */
    H460_FeatureContent(const H225_AliasAddress & add);

    /** Alias Address Encoded
    */
    H460_FeatureContent(const PURL & add);

    /** Transport Address
    */
    H460_FeatureContent(const H323TransportAddress & add);

    /** Feature Table
    */
    H460_FeatureContent(const H225_ArrayOf_EnumeratedParameter & table);

    /** GUID
    */
    H460_FeatureContent(const OpalGloballyUniqueID & guid);

    /** Features
    */
    H460_FeatureContent(const H460_Feature & nested);
  //@}
};


///////////////////////////////////////////////////////////////////////////////

/**This is a base class for H.323 Feature handling.
   This implements the service class session management as per H460 Series.
  */

class H460_FeatureParameter : public H225_EnumeratedParameter
{
    /* Due to some naughty reinterpret_cast<> of H225_GenericIdentifier to
       H460_FeatureID in various palces, do not add members or do any virtual
       function overrides in this class! */
  public:
  /**@name Construction */
  //@{
    /**Create a Blank Feature.
     */
    H460_FeatureParameter();

    /**Create a new Handler from FeatureID
    */
    H460_FeatureParameter(const H460_FeatureID & id);

    /**Create a new Handler from FeatureID
    */
    H460_FeatureParameter(const H460_FeatureID & id, const H460_FeatureContent & content);

    /**Create a new handler from PDU
     */
    H460_FeatureParameter(const H225_EnumeratedParameter & param);
    //@}

    /**@name ID Operators */
    //@{
    /** Feature Parameter ID 
    */
    const H460_FeatureID & GetID() const { return reinterpret_cast<const H460_FeatureID &>(m_id); }

    /** Feature Parameter Contents
    */
    void SetContent(const H460_FeatureContent & content);
  //@}

  /**@name Parameter Value Operators */
  //@{
    operator const PASN_OctetString &() const;
    operator PString() const;
    operator const PASN_BMPString &() const;
    operator bool() const;
    operator unsigned() const;
    operator const H460_FeatureID &() const;
    operator const H225_AliasAddress &() const;
    operator H323TransportAddress() const;
    operator const H225_ArrayOf_EnumeratedParameter &() const;
    operator PURL() const;
    operator OpalGloballyUniqueID() const;

    H460_FeatureParameter & operator=(const PASN_OctetString & value);
    H460_FeatureParameter & operator=(const PString & value);
    H460_FeatureParameter & operator=(const PASN_BMPString & value);
    H460_FeatureParameter & operator=(bool value);
    H460_FeatureParameter & operator=(unsigned value);
    H460_FeatureParameter & operator=(const H460_FeatureID & value);
    H460_FeatureParameter & operator=(const H225_AliasAddress & value);
    H460_FeatureParameter & operator=(const H323TransportAddress & value);
    H460_FeatureParameter & operator=(const H225_ArrayOf_EnumeratedParameter & value);
    H460_FeatureParameter & operator=(const OpalGloballyUniqueID & value);
  //@}
};


///////////////////////////////////////////////////////////////////////////////

/**This is a base class for H.323 Feature handling.
   This implements the service class session management as per H460 Series.
  */
class H460_FeatureDescriptor : public H225_FeatureDescriptor
{
    /* Due to some naughty reinterpret_cast<> of H225_GenericIdentifier to
       H460_FeatureID in various palces, do not add members or do any virtual
       function overrides in this class! */
  public:
  /**@name Construction */
  //@{
    /** Blank Feature descriptor
    */
    H460_FeatureDescriptor();

    /** Feature descriptor for id
    */
    H460_FeatureDescriptor(const H460_FeatureID & id);

    /**Create a new handler for a PDU Received Feature.
     */
    H460_FeatureDescriptor(const H225_FeatureDescriptor & descriptor);
  //@}

  /**@name Parameter Control */
  //@{
    const H460_FeatureID & GetID() const
    { return reinterpret_cast<const H460_FeatureID &>(m_id); }

    /** Add Parameter 
    */
    H460_FeatureParameter & AddParameter(
      const H460_FeatureID & id,
      const H460_FeatureContent & content = H460_FeatureContent(),
      bool unique = true
    );

    /** Add Parameter from H460_FeatureParameter
    */
    H460_FeatureParameter & AddParameter(
      H460_FeatureParameter * param,
      bool unique = true
    );

    /** Delete Parameter
    */
    void RemoveParameterAt(PINDEX index);

    /** Delete Parameter
    */
    void RemoveParameter(const H460_FeatureID & id);

    /** Replace Parameter
    */
    void ReplaceParameter(const H460_FeatureID & id, const H460_FeatureContent & content);

    /** Get Parameter at index id
    */
    H460_FeatureParameter & GetParameterAt(PINDEX index) const
    { return reinterpret_cast<H460_FeatureParameter &>(m_parameters[index]); }

    /** Determine of forst parameter wth the feature ID.
      */
    PINDEX GetParameterIndex(const H460_FeatureID & id) const;

    /** Get Parameter with FeatureID
    */
    H460_FeatureParameter & GetParameter(const H460_FeatureID & id) const;

    /** Get boolean Parameter with FeatureID
    */
    bool GetBooleanParameter(const H460_FeatureID & id) const;

    /** Has Feature with FeatureID
    */
    bool HasParameter(const H460_FeatureID & id) const;

    /** ParameterIsUnique
      return true if there is only 1 instance of
      feature parameter with matching feature ID
      exists in the feature list. You cannot replace
      the contents of the parameter if the parameter ID
      is not unique.
    */
    bool IsParameterIsUnique(const H460_FeatureID & id) const;

    /** Get the Number of Parameters
    */
    PINDEX GetParameterCount() const
    { return m_parameters.GetSize(); }

    /** Operator
    */
    H460_FeatureParameter & operator[](
      PINDEX index  ///* Index position in the collection of the object.
    ) { return GetParameterAt(index); }
  //@}
};


///////////////////////////////////////////////////////////////////////////////

/**This is a base class for H.323 Feature handling.
   This implements the service class session management as per H460 Series.
  */
class H460_Feature : public PObject
{
    PCLASSINFO_WITH_CLONE(H460_Feature, PObject);
  public:
  /**@name Construction */
  //@{
    /**Create a new handler for a feature id
     */
    H460_Feature(const H460_FeatureID & id = H460_FeatureID());
  //@}

  /**@name Attributes */
  //@{
    /// Category for feature
    P_DECLARE_TRACED_ENUM(Category,
      Needed,      ///< The Feature is Needed 
      Desired,     ///< Desired Feature
      Supported    ///< Supported Feature
    );
    Category GetCategory() const { return m_category; }
    void SetCategory(Category cat) { m_category = cat; }
  //@}

  /**@name Operators */
  //@{
    /** Get the FeatureID 
    */
    const H460_FeatureID & GetID() const
    { return m_descriptor.GetID(); }

    /** set the FeatureID
    */
    void SetFeatureID(const H460_FeatureID & id) { m_descriptor.m_id = id; }

    /** Attach the endpoint. Override this to link your own Endpoint Instance.
      */
    virtual bool Initialise(H323EndPoint & ep, H323Connection * con);

    /**Negotiations completed
      */
    virtual bool IsNegotiated() const { return m_supportedByRemote; }
  //@}

  /**@name Parameter Control */
  //@{
    /** Add Parameter 
    */
    virtual H460_FeatureParameter & AddParameter(
      const H460_FeatureID & id,
      const H460_FeatureContent & content = H460_FeatureContent(),
      bool unique = true
    ) { return m_descriptor.AddParameter(id, content, unique); }

    /** Add Parameter from H460_FeatureParameter
    */
    virtual H460_FeatureParameter & AddParameter(
      H460_FeatureParameter * param,
      bool unique = true
    ) { return m_descriptor.AddParameter(param, unique); }

    /** Delete Parameter 
    */
    virtual void RemoveParameterAt(
      PINDEX index
    ) { m_descriptor.RemoveParameterAt(index); }

    /** Delete Parameter 
    */
    virtual void RemoveParameter(
      const H460_FeatureID & id
    ) { m_descriptor.RemoveParameter(id); }

    /** Replace Parameter
    */
    virtual void ReplaceParameter(
      const H460_FeatureID id,
      const H460_FeatureContent & content
    ) { m_descriptor.ReplaceParameter(id, content); }

    /** Get Parameter at index id
    */
    virtual H460_FeatureParameter & GetParameterAt(
      PINDEX index
    ) { return m_descriptor.GetParameterAt(index); }

    /** Get Parameter with FeatureID
    */
    virtual H460_FeatureParameter & GetParameter(
      const H460_FeatureID & id
    ) { return m_descriptor.GetParameter(id); }

    /** Has Feature with FeatureID
    */
    virtual bool HasParameter(
      const H460_FeatureID & id
    ) { return m_descriptor.HasParameter(id); }
 
    // Operators
    H460_FeatureParameter & operator[](
      PINDEX index  ///* Index position in the collection of the object.
    ) { return GetParameter(index); }

    /** Accessing the Parameters
    */
    H460_FeatureParameter & operator[](
      const H460_FeatureID & id  ///< FeatureID of the object.
    ) { return GetParameter(id); }

    /** Get the Number of Parameters
    */
    PINDEX GetParameterCount() const
    { return m_descriptor.GetParameterCount(); }
  //@}

  /**@name Plugin Management */
  //@{
    /** Get Feature Names 
    */
    static PStringList GetFeatureNames(PPluginManager * pluginMgr = NULL);

    /** Create instance of a feature 
    */
    static H460_Feature * CreateFeature(
      const PString & featurename,        ///< Feature Name Expression
      PPluginManager * pluginMgr = NULL   ///< Plugin Manager
    );
  //@}

  /**@name H323 RAS Interface */
  //@{
    /* These are the main calls which can be overridden to
      allow the various derived features access to the GEF
      interface.
    */
    virtual bool OnSendPDU(H460_MessageType pduType, H460_FeatureDescriptor & pdu);
    virtual void OnReceivePDU(H460_MessageType pduType, const H460_FeatureDescriptor & pdu);

    // PDU calls (Used in the H225_RAS Class)
    virtual bool OnSendGatekeeperRequest(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual bool OnSendGatekeeperConfirm(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual bool OnSendGatekeeperReject(H460_FeatureDescriptor & /*pdu*/) { return false; }

    virtual void OnReceiveGatekeeperRequest(const H460_FeatureDescriptor & /*pdu*/) { }
    virtual void OnReceiveGatekeeperConfirm(const H460_FeatureDescriptor & /*pdu*/) { }
    virtual void OnReceiveGatekeeperReject(const H460_FeatureDescriptor & /*pdu*/) { }

    virtual bool OnSendRegistrationRequest(H460_FeatureDescriptor & /*pdu*/, bool /*lightweight*/) { return false; }
    virtual bool OnSendRegistrationConfirm(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual bool OnSendRegistrationReject(H460_FeatureDescriptor & /*pdu*/) { return false; }

    virtual void OnReceiveRegistrationRequest(const H460_FeatureDescriptor & /*pdu*/) { }
    virtual void OnReceiveRegistrationConfirm(const H460_FeatureDescriptor & /*pdu*/) { }
    virtual void OnReceiveRegistrationReject(const H460_FeatureDescriptor & /*pdu*/) { }

    virtual bool OnSendAdmissionRequest(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual bool OnSendAdmissionConfirm(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual bool OnSendAdmissionReject(H460_FeatureDescriptor & /*pdu*/) { return false; }

    virtual void OnReceiveAdmissionRequest(const H460_FeatureDescriptor & /*pdu*/) { }
    virtual void OnReceiveAdmissionConfirm(const H460_FeatureDescriptor & /*pdu*/) { }
    virtual void OnReceiveAdmissionReject(const H460_FeatureDescriptor & /*pdu*/) { }

    virtual bool OnSendLocationRequest(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual bool OnSendLocationConfirm(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual bool OnSendLocationReject(H460_FeatureDescriptor & /*pdu*/) { return false; }

    virtual void OnReceiveLocationRequest(const H460_FeatureDescriptor & /*pdu*/) { }
    virtual void OnReceiveLocationConfirm(const H460_FeatureDescriptor & /*pdu*/) { }
    virtual void OnReceiveLocationReject(const H460_FeatureDescriptor & /*pdu*/) { }

    virtual bool OnSendServiceControlIndication(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual bool OnSendServiceControlResponse(H460_FeatureDescriptor & /*pdu*/) { return false; }

    virtual void OnReceiveServiceControlIndication(const H460_FeatureDescriptor & /*pdu*/) { }
    virtual void OnReceiveServiceControlResponse(const H460_FeatureDescriptor & /*pdu*/) { }

    virtual bool OnSendNonStandardMessage(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual void OnReceiveNonStandardMessage(const H460_FeatureDescriptor & /*pdu*/) { }

    virtual bool OnSendUnregistrationRequest(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual void OnReceiveUnregistrationRequest(const H460_FeatureDescriptor & /*pdu*/) { }

    virtual bool OnSendEndpoint(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual void OnReceiveEndpoint(const H460_FeatureDescriptor & /*pdu*/) { }

    virtual bool OnSendInfoRequestMessage(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual void OnReceiveInfoRequestMessage(const H460_FeatureDescriptor & /*pdu*/) { }

    virtual bool OnSendInfoRequestResponseMessage(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual void OnReceiveInfoRequestResponseMessage(const H460_FeatureDescriptor & /*pdu*/) { }

    virtual bool OnSendDisengagementRequestMessage(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual void OnReceiveDisengagementRequestMessage(const H460_FeatureDescriptor & /*pdu*/) { }

    virtual bool OnSendDisengagementConfirmMessage(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual void OnReceiveDisengagementConfirmMessage(const H460_FeatureDescriptor & /*pdu*/) { }
  //@}

  /**@name Signal PDU Interface */
  //@{
  // UUIE Calls (Used in the H323SignalPDU Class)
    virtual bool OnSendSetup_UUIE(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual void OnReceiveSetup_UUIE(const H460_FeatureDescriptor & /*pdu*/) { }

    virtual bool OnSendAlerting_UUIE(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual void OnReceiveAlerting_UUIE(const H460_FeatureDescriptor & /*pdu*/) { }

    virtual bool OnSendCallProceeding_UUIE(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual void OnReceiveCallProceeding_UUIE(const H460_FeatureDescriptor & /*pdu*/) { }

    virtual bool OnSendCallConnect_UUIE(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual void OnReceiveCallConnect_UUIE(const H460_FeatureDescriptor & /*pdu*/) { }

    virtual bool OnSendFacility_UUIE(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual void OnReceiveFacility_UUIE(const H460_FeatureDescriptor & /*pdu*/) { }

    virtual bool OnSendReleaseComplete_UUIE(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual void OnReceiveReleaseComplete_UUIE(const H460_FeatureDescriptor & /*pdu*/) { }

    virtual bool OnSendUnAllocatedPDU(H460_FeatureDescriptor & /*pdu*/) { return false; }
    virtual void OnReceivedUnAllocatedPDU(const H460_FeatureDescriptor & /*pdu*/) { }

    virtual bool OnSendingOLCGenericInformation(unsigned /*sessionID*/, H245_ArrayOf_GenericParameter & /*param*/, bool /*ack*/) { return false; }
    virtual void OnReceiveOLCGenericInformation(unsigned /*sessionID */, const H245_ArrayOf_GenericParameter & /*param*/, bool /*isAck*/) { }
  //@}

    H323EndPoint * GetEndPoint() const { return m_endpoint; }
    const H460_FeatureDescriptor & GetDescriptor() const { return m_descriptor; }

    static H460_Feature * FromContext(PObject * context, const H460_FeatureID & id);
    template <class FEAT> static bool FromContext(PObject * context, FEAT * & feature)
    {
      feature = dynamic_cast<FEAT *>(FromContext(context, FEAT::ID()));
      return feature != NULL;
    }

  protected:
    H460_Feature * GetFeatureOnGk(const H460_FeatureID & id) const;

    template <class FEAT> bool GetFeatureOnGk(FEAT * & feature, const H460_FeatureID & id = FEAT::ID())
    {
      feature = dynamic_cast<FEAT *>(GetFeatureOnGk(id));
      return feature != NULL;
    }

    bool IsFeatureNegotiatedOnGk(const H460_FeatureID & id) const;

#if OPAL_H460_NAT
    PNatMethod * GetNatMethod(const char * methodName) const;

    template <class METH> bool GetNatMethod(const char * methodName, METH * & natMethod) const
    {
      natMethod = dynamic_cast<METH *>(GetNatMethod(methodName));
      return natMethod != NULL;
    }
#endif // OPAL_H460_NAT

    Category               m_category;
    H323EndPoint         * m_endpoint;
    H323Connection       * m_connection;
    H460_FeatureDescriptor m_descriptor;
    bool                   m_supportedByRemote;
};


/** H.460 FeatureSet Main Calling Class
  */
class H460_FeatureSet : public PObject, public map<H460_FeatureID, H460_Feature *>
{
    PCLASSINFO(H460_FeatureSet, PObject);
  public:
    /** Build a new featureSet from a base featureset
    */
    H460_FeatureSet(
      H323EndPoint & ep
    );
    ~H460_FeatureSet();


    /** Load Entire Feature Sets from PFactory loader
    */
    virtual void LoadFeatureSet(
      H323Connection * con = NULL
    );

    /** Add a Feature to the Feature Set
    */
    bool AddFeature(H460_Feature * feat);

    /** Remove a Feature from the Feature Set
    */
    void RemoveFeature(const H460_FeatureID & id);

    /** Get Feature with id
    */
    H460_Feature * GetFeature(const H460_FeatureID & id);
    template <class FEAT> bool GetFeature(FEAT * & feature, const H460_FeatureID & id = FEAT::ID())
    {
      feature = dynamic_cast<FEAT *>(GetFeature(id));
      return feature != NULL;
    }

    /** Determine if the FeatureSet has a particular FeatureID.
    */
    bool HasFeature(const H460_FeatureID & feat);

    /** New Processing Paradigm
    Main PDU & RAS link to OpenH323
    */
    virtual void OnReceivePDU(
      H460_MessageType pduType,
      const H225_FeatureSet & pdu
    );

    /** New Processing Paradigm
    Main PDU & RAS link to OpenH323
    */
    virtual bool OnSendPDU(
      H460_MessageType pduType,
      H225_FeatureSet & pdu
    );

    /** Attach Endpoint to collect Events from
    */
    H323EndPoint & GetEndPoint() { return m_endpoint; }

    static bool Copy(H225_FeatureSet & fs, const H225_ArrayOf_GenericData & gd);
    static bool Copy(H225_ArrayOf_GenericData & gd, const H225_FeatureSet & fs);

  protected:
    void OnReceivePDU(H460_MessageType pduType, const H225_ArrayOf_FeatureDescriptor & descriptors);

    H323EndPoint & m_endpoint;
};


/////////////////////////////////////////////////////////////////////

PCREATE_PLUGIN_SERVICE(H460_Feature);

#define H460_FEATURE(name, friendlyName) \
    PCREATE_PLUGIN(name, H460_Feature, H460_Feature##name, PPlugin_H460_Feature, \
      virtual const char * GetFriendlyName() const { return friendlyName; } \
    )


#if OPAL_H460_NAT
  PPLUGIN_STATIC_LOAD(Std18, H460_Feature);
  PPLUGIN_STATIC_LOAD(Std19, H460_Feature);
  PPLUGIN_STATIC_LOAD(Std23, H460_Feature);
  PPLUGIN_STATIC_LOAD(Std24, H460_Feature);
#endif


#endif // OPAL_H460

#endif // OPAL_H460_H4601_H
