//
// h4501.cxx
//
// Code automatically generated by asnparse.
//

#ifdef P_USE_PRAGMA
#pragma implementation "h4501.h"
#endif

#include <ptlib.h>
#include "asn/h4501.h"

#define new PNEW


#if ! H323_DISABLE_H4501



//
// EntityType
//

H4501_EntityType::H4501_EntityType(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Choice(tag, tagClass, 2, TRUE
#ifndef PASN_NOPRINTON
      , "endpoint "
        "anyEntity "
#endif
    )
{
}


BOOL H4501_EntityType::CreateObject()
{
  choice = (tag <= e_anyEntity) ? new PASN_Null() : NULL;
  return choice != NULL;
}


PObject * H4501_EntityType::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_EntityType::Class()), PInvalidCast);
#endif
  return new H4501_EntityType(*this);
}


//
// AddressInformation
//

H4501_AddressInformation::H4501_AddressInformation(unsigned tag, PASN_Object::TagClass tagClass)
  : H225_AliasAddress(tag, tagClass)
{
}


PObject * H4501_AddressInformation::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_AddressInformation::Class()), PInvalidCast);
#endif
  return new H4501_AddressInformation(*this);
}


//
// InterpretationApdu
//

H4501_InterpretationApdu::H4501_InterpretationApdu(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Choice(tag, tagClass, 3, TRUE
#ifndef PASN_NOPRINTON
      , "discardAnyUnrecognizedInvokePdu "
        "clearCallIfAnyInvokePduNotRecognized "
        "rejectAnyUnrecognizedInvokePdu "
#endif
    )
{
}


BOOL H4501_InterpretationApdu::CreateObject()
{
  choice = (tag <= e_rejectAnyUnrecognizedInvokePdu) ? new PASN_Null() : NULL;
  return choice != NULL;
}


PObject * H4501_InterpretationApdu::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_InterpretationApdu::Class()), PInvalidCast);
#endif
  return new H4501_InterpretationApdu(*this);
}


//
// ServiceApdus
//

H4501_ServiceApdus::H4501_ServiceApdus(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Choice(tag, tagClass, 1, TRUE
#ifndef PASN_NOPRINTON
      , "rosApdus "
#endif
    )
{
}


#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
H4501_ServiceApdus::operator H4501_ArrayOf_ROS &() const
#else
H4501_ServiceApdus::operator H4501_ArrayOf_ROS &()
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), H4501_ArrayOf_ROS), PInvalidCast);
#endif
  return *(H4501_ArrayOf_ROS *)choice;
}


H4501_ServiceApdus::operator const H4501_ArrayOf_ROS &() const
#endif
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), H4501_ArrayOf_ROS), PInvalidCast);
#endif
  return *(H4501_ArrayOf_ROS *)choice;
}


BOOL H4501_ServiceApdus::CreateObject()
{
  switch (tag) {
    case e_rosApdus :
      choice = new H4501_ArrayOf_ROS();
      choice->SetConstraints(PASN_Object::FixedConstraint, 1, MaximumValue);
      return TRUE;
  }

  choice = NULL;
  return FALSE;
}


PObject * H4501_ServiceApdus::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_ServiceApdus::Class()), PInvalidCast);
#endif
  return new H4501_ServiceApdus(*this);
}


//
// InvokeIdSet
//

H4501_InvokeIdSet::H4501_InvokeIdSet(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Integer(tag, tagClass)
{
}


H4501_InvokeIdSet & H4501_InvokeIdSet::operator=(int v)
{
  SetValue(v);
  return *this;
}


H4501_InvokeIdSet & H4501_InvokeIdSet::operator=(unsigned v)
{
  SetValue(v);
  return *this;
}


PObject * H4501_InvokeIdSet::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_InvokeIdSet::Class()), PInvalidCast);
#endif
  return new H4501_InvokeIdSet(*this);
}


//
// InvokeIDs
//

H4501_InvokeIDs::H4501_InvokeIDs(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Integer(tag, tagClass)
{
  SetConstraints(PASN_Object::FixedConstraint, 0, 65535);
}


H4501_InvokeIDs & H4501_InvokeIDs::operator=(int v)
{
  SetValue(v);
  return *this;
}


H4501_InvokeIDs & H4501_InvokeIDs::operator=(unsigned v)
{
  SetValue(v);
  return *this;
}


PObject * H4501_InvokeIDs::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_InvokeIDs::Class()), PInvalidCast);
#endif
  return new H4501_InvokeIDs(*this);
}


//
// PresentedAddressScreened
//

H4501_PresentedAddressScreened::H4501_PresentedAddressScreened(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Choice(tag, tagClass, 4, TRUE
#ifndef PASN_NOPRINTON
      , "presentationAllowedAddress "
        "presentationRestricted "
        "numberNotAvailableDueToInterworking "
        "presentationRestrictedAddress "
#endif
    )
{
}


#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
H4501_PresentedAddressScreened::operator H4501_AddressScreened &() const
#else
H4501_PresentedAddressScreened::operator H4501_AddressScreened &()
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), H4501_AddressScreened), PInvalidCast);
#endif
  return *(H4501_AddressScreened *)choice;
}


H4501_PresentedAddressScreened::operator const H4501_AddressScreened &() const
#endif
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), H4501_AddressScreened), PInvalidCast);
#endif
  return *(H4501_AddressScreened *)choice;
}


BOOL H4501_PresentedAddressScreened::CreateObject()
{
  switch (tag) {
    case e_presentationAllowedAddress :
    case e_presentationRestrictedAddress :
      choice = new H4501_AddressScreened();
      return TRUE;
    case e_presentationRestricted :
    case e_numberNotAvailableDueToInterworking :
      choice = new PASN_Null();
      return TRUE;
  }

  choice = NULL;
  return FALSE;
}


PObject * H4501_PresentedAddressScreened::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_PresentedAddressScreened::Class()), PInvalidCast);
#endif
  return new H4501_PresentedAddressScreened(*this);
}


//
// PresentedAddressUnscreened
//

H4501_PresentedAddressUnscreened::H4501_PresentedAddressUnscreened(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Choice(tag, tagClass, 4, TRUE
#ifndef PASN_NOPRINTON
      , "presentationAllowedAddress "
        "presentationRestricted "
        "numberNotAvailableDueToInterworking "
        "presentationRestrictedAddress "
#endif
    )
{
}


#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
H4501_PresentedAddressUnscreened::operator H4501_Address &() const
#else
H4501_PresentedAddressUnscreened::operator H4501_Address &()
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), H4501_Address), PInvalidCast);
#endif
  return *(H4501_Address *)choice;
}


H4501_PresentedAddressUnscreened::operator const H4501_Address &() const
#endif
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), H4501_Address), PInvalidCast);
#endif
  return *(H4501_Address *)choice;
}


BOOL H4501_PresentedAddressUnscreened::CreateObject()
{
  switch (tag) {
    case e_presentationAllowedAddress :
    case e_presentationRestrictedAddress :
      choice = new H4501_Address();
      return TRUE;
    case e_presentationRestricted :
    case e_numberNotAvailableDueToInterworking :
      choice = new PASN_Null();
      return TRUE;
  }

  choice = NULL;
  return FALSE;
}


PObject * H4501_PresentedAddressUnscreened::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_PresentedAddressUnscreened::Class()), PInvalidCast);
#endif
  return new H4501_PresentedAddressUnscreened(*this);
}


//
// PresentedNumberScreened
//

H4501_PresentedNumberScreened::H4501_PresentedNumberScreened(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Choice(tag, tagClass, 4, TRUE
#ifndef PASN_NOPRINTON
      , "presentationAllowedAddress "
        "presentationRestricted "
        "numberNotAvailableDueToInterworking "
        "presentationRestrictedAddress "
#endif
    )
{
}


#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
H4501_PresentedNumberScreened::operator H4501_NumberScreened &() const
#else
H4501_PresentedNumberScreened::operator H4501_NumberScreened &()
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), H4501_NumberScreened), PInvalidCast);
#endif
  return *(H4501_NumberScreened *)choice;
}


H4501_PresentedNumberScreened::operator const H4501_NumberScreened &() const
#endif
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), H4501_NumberScreened), PInvalidCast);
#endif
  return *(H4501_NumberScreened *)choice;
}


BOOL H4501_PresentedNumberScreened::CreateObject()
{
  switch (tag) {
    case e_presentationAllowedAddress :
    case e_presentationRestrictedAddress :
      choice = new H4501_NumberScreened();
      return TRUE;
    case e_presentationRestricted :
    case e_numberNotAvailableDueToInterworking :
      choice = new PASN_Null();
      return TRUE;
  }

  choice = NULL;
  return FALSE;
}


PObject * H4501_PresentedNumberScreened::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_PresentedNumberScreened::Class()), PInvalidCast);
#endif
  return new H4501_PresentedNumberScreened(*this);
}


//
// PresentedNumberUnscreened
//

H4501_PresentedNumberUnscreened::H4501_PresentedNumberUnscreened(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Choice(tag, tagClass, 4, TRUE
#ifndef PASN_NOPRINTON
      , "presentationAllowedAddress "
        "presentationRestricted "
        "numberNotAvailableDueToInterworking "
        "presentationRestrictedAddress "
#endif
    )
{
}


#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
H4501_PresentedNumberUnscreened::operator H225_PartyNumber &() const
#else
H4501_PresentedNumberUnscreened::operator H225_PartyNumber &()
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), H225_PartyNumber), PInvalidCast);
#endif
  return *(H225_PartyNumber *)choice;
}


H4501_PresentedNumberUnscreened::operator const H225_PartyNumber &() const
#endif
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), H225_PartyNumber), PInvalidCast);
#endif
  return *(H225_PartyNumber *)choice;
}


BOOL H4501_PresentedNumberUnscreened::CreateObject()
{
  switch (tag) {
    case e_presentationAllowedAddress :
    case e_presentationRestrictedAddress :
      choice = new H225_PartyNumber();
      return TRUE;
    case e_presentationRestricted :
    case e_numberNotAvailableDueToInterworking :
      choice = new PASN_Null();
      return TRUE;
  }

  choice = NULL;
  return FALSE;
}


PObject * H4501_PresentedNumberUnscreened::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_PresentedNumberUnscreened::Class()), PInvalidCast);
#endif
  return new H4501_PresentedNumberUnscreened(*this);
}


//
// PartySubaddress
//

H4501_PartySubaddress::H4501_PartySubaddress(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Choice(tag, tagClass, 2, TRUE
#ifndef PASN_NOPRINTON
      , "userSpecifiedSubaddress "
        "nsapSubaddress "
#endif
    )
{
}


#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
H4501_PartySubaddress::operator H4501_UserSpecifiedSubaddress &() const
#else
H4501_PartySubaddress::operator H4501_UserSpecifiedSubaddress &()
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), H4501_UserSpecifiedSubaddress), PInvalidCast);
#endif
  return *(H4501_UserSpecifiedSubaddress *)choice;
}


H4501_PartySubaddress::operator const H4501_UserSpecifiedSubaddress &() const
#endif
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), H4501_UserSpecifiedSubaddress), PInvalidCast);
#endif
  return *(H4501_UserSpecifiedSubaddress *)choice;
}


#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
H4501_PartySubaddress::operator H4501_NSAPSubaddress &() const
#else
H4501_PartySubaddress::operator H4501_NSAPSubaddress &()
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), H4501_NSAPSubaddress), PInvalidCast);
#endif
  return *(H4501_NSAPSubaddress *)choice;
}


H4501_PartySubaddress::operator const H4501_NSAPSubaddress &() const
#endif
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), H4501_NSAPSubaddress), PInvalidCast);
#endif
  return *(H4501_NSAPSubaddress *)choice;
}


BOOL H4501_PartySubaddress::CreateObject()
{
  switch (tag) {
    case e_userSpecifiedSubaddress :
      choice = new H4501_UserSpecifiedSubaddress();
      return TRUE;
    case e_nsapSubaddress :
      choice = new H4501_NSAPSubaddress();
      return TRUE;
  }

  choice = NULL;
  return FALSE;
}


PObject * H4501_PartySubaddress::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_PartySubaddress::Class()), PInvalidCast);
#endif
  return new H4501_PartySubaddress(*this);
}


//
// NSAPSubaddress
//

H4501_NSAPSubaddress::H4501_NSAPSubaddress(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_OctetString(tag, tagClass)
{
  SetConstraints(PASN_Object::FixedConstraint, 1, 20);
}


H4501_NSAPSubaddress::H4501_NSAPSubaddress(const char * v)
{
  SetValue(v);
}


H4501_NSAPSubaddress::H4501_NSAPSubaddress(const PString & v)
{
  SetValue(v);
}


H4501_NSAPSubaddress::H4501_NSAPSubaddress(const PBYTEArray & v)
{
  SetValue(v);
}


H4501_NSAPSubaddress & H4501_NSAPSubaddress::operator=(const char * v)
{
  SetValue(v);
  return *this;
}


H4501_NSAPSubaddress & H4501_NSAPSubaddress::operator=(const PString & v)
{
  SetValue(v);
  return *this;
}


H4501_NSAPSubaddress & H4501_NSAPSubaddress::operator=(const PBYTEArray & v)
{
  SetValue(v);
  return *this;
}


PObject * H4501_NSAPSubaddress::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_NSAPSubaddress::Class()), PInvalidCast);
#endif
  return new H4501_NSAPSubaddress(*this);
}


//
// SubaddressInformation
//

H4501_SubaddressInformation::H4501_SubaddressInformation(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_OctetString(tag, tagClass)
{
  SetConstraints(PASN_Object::FixedConstraint, 1, 20);
}


H4501_SubaddressInformation::H4501_SubaddressInformation(const char * v)
{
  SetValue(v);
}


H4501_SubaddressInformation::H4501_SubaddressInformation(const PString & v)
{
  SetValue(v);
}


H4501_SubaddressInformation::H4501_SubaddressInformation(const PBYTEArray & v)
{
  SetValue(v);
}


H4501_SubaddressInformation & H4501_SubaddressInformation::operator=(const char * v)
{
  SetValue(v);
  return *this;
}


H4501_SubaddressInformation & H4501_SubaddressInformation::operator=(const PString & v)
{
  SetValue(v);
  return *this;
}


H4501_SubaddressInformation & H4501_SubaddressInformation::operator=(const PBYTEArray & v)
{
  SetValue(v);
  return *this;
}


PObject * H4501_SubaddressInformation::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_SubaddressInformation::Class()), PInvalidCast);
#endif
  return new H4501_SubaddressInformation(*this);
}


//
// ScreeningIndicator
//

H4501_ScreeningIndicator::H4501_ScreeningIndicator(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Enumeration(tag, tagClass, 3, TRUE
#ifndef PASN_NOPRINTON
      , "userProvidedNotScreened "
        "userProvidedVerifiedAndPassed "
        "userProvidedVerifiedAndFailed "
        "networkProvided "
#endif
    )
{
}


H4501_ScreeningIndicator & H4501_ScreeningIndicator::operator=(unsigned v)
{
  SetValue(v);
  return *this;
}


PObject * H4501_ScreeningIndicator::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_ScreeningIndicator::Class()), PInvalidCast);
#endif
  return new H4501_ScreeningIndicator(*this);
}


//
// PresentationAllowedIndicator
//

H4501_PresentationAllowedIndicator::H4501_PresentationAllowedIndicator(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Boolean(tag, tagClass)
{
}


H4501_PresentationAllowedIndicator & H4501_PresentationAllowedIndicator::operator=(BOOL v)
{
  SetValue(v);
  return *this;
}


PObject * H4501_PresentationAllowedIndicator::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_PresentationAllowedIndicator::Class()), PInvalidCast);
#endif
  return new H4501_PresentationAllowedIndicator(*this);
}


//
// GeneralErrorList
//

H4501_GeneralErrorList::H4501_GeneralErrorList(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Enumeration(tag, tagClass, 43, FALSE
#ifndef PASN_NOPRINTON
      , "userNotSubscribed "
        "rejectedByNetwork "
        "rejectedByUser "
        "notAvailable "
        "insufficientInformation=5 "
        "invalidServedUserNumber "
        "invalidCallState "
        "basicServiceNotProvided "
        "notIncomingCall "
        "supplementaryServiceInteractionNotAllowed "
        "resourceUnavailable "
        "callFailure=25 "
        "proceduralError=43 "
#endif
    )
{
}


H4501_GeneralErrorList & H4501_GeneralErrorList::operator=(unsigned v)
{
  SetValue(v);
  return *this;
}


PObject * H4501_GeneralErrorList::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_GeneralErrorList::Class()), PInvalidCast);
#endif
  return new H4501_GeneralErrorList(*this);
}


//
// H225InformationElement
//

H4501_H225InformationElement::H4501_H225InformationElement(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_OctetString(tag, tagClass)
{
}


H4501_H225InformationElement::H4501_H225InformationElement(const char * v)
{
  SetValue(v);
}


H4501_H225InformationElement::H4501_H225InformationElement(const PString & v)
{
  SetValue(v);
}


H4501_H225InformationElement::H4501_H225InformationElement(const PBYTEArray & v)
{
  SetValue(v);
}


H4501_H225InformationElement & H4501_H225InformationElement::operator=(const char * v)
{
  SetValue(v);
  return *this;
}


H4501_H225InformationElement & H4501_H225InformationElement::operator=(const PString & v)
{
  SetValue(v);
  return *this;
}


H4501_H225InformationElement & H4501_H225InformationElement::operator=(const PBYTEArray & v)
{
  SetValue(v);
  return *this;
}


PObject * H4501_H225InformationElement::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_H225InformationElement::Class()), PInvalidCast);
#endif
  return new H4501_H225InformationElement(*this);
}


//
// Extension
//

H4501_Extension::H4501_Extension(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Sequence(tag, tagClass, 0, FALSE, 0)
{
}


#ifndef PASN_NOPRINTON
void H4501_Extension::PrintOn(ostream & strm) const
{
  int indent = strm.precision() + 2;
  strm << "{\n";
  strm << setw(indent+14) << "extensionId = " << setprecision(indent) << m_extensionId << '\n';
  strm << setw(indent+20) << "extensionArgument = " << setprecision(indent) << m_extensionArgument << '\n';
  strm << setw(indent-1) << setprecision(indent-2) << "}";
}
#endif


PObject::Comparison H4501_Extension::Compare(const PObject & obj) const
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(&obj, H4501_Extension), PInvalidCast);
#endif
  const H4501_Extension & other = (const H4501_Extension &)obj;

  Comparison result;

  if ((result = m_extensionId.Compare(other.m_extensionId)) != EqualTo)
    return result;
  if ((result = m_extensionArgument.Compare(other.m_extensionArgument)) != EqualTo)
    return result;

  return PASN_Sequence::Compare(other);
}


PINDEX H4501_Extension::GetDataLength() const
{
  PINDEX length = 0;
  length += m_extensionId.GetObjectLength();
  length += m_extensionArgument.GetObjectLength();
  return length;
}


BOOL H4501_Extension::Decode(PASN_Stream & strm)
{
  if (!PreambleDecode(strm))
    return FALSE;

  if (!m_extensionId.Decode(strm))
    return FALSE;
  if (!m_extensionArgument.Decode(strm))
    return FALSE;

  return UnknownExtensionsDecode(strm);
}


void H4501_Extension::Encode(PASN_Stream & strm) const
{
  PreambleEncode(strm);

  m_extensionId.Encode(strm);
  m_extensionArgument.Encode(strm);

  UnknownExtensionsEncode(strm);
}


PObject * H4501_Extension::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_Extension::Class()), PInvalidCast);
#endif
  return new H4501_Extension(*this);
}


//
// ArrayOf_ROS
//

H4501_ArrayOf_ROS::H4501_ArrayOf_ROS(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Array(tag, tagClass)
{
}


PASN_Object * H4501_ArrayOf_ROS::CreateObject() const
{
  return new X880_ROS;
}


X880_ROS & H4501_ArrayOf_ROS::operator[](PINDEX i) const
{
  return (X880_ROS &)array[i];
}


PObject * H4501_ArrayOf_ROS::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_ArrayOf_ROS::Class()), PInvalidCast);
#endif
  return new H4501_ArrayOf_ROS(*this);
}


//
// ArrayOf_AliasAddress
//

H4501_ArrayOf_AliasAddress::H4501_ArrayOf_AliasAddress(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Array(tag, tagClass)
{
}


PASN_Object * H4501_ArrayOf_AliasAddress::CreateObject() const
{
  return new H225_AliasAddress;
}


H225_AliasAddress & H4501_ArrayOf_AliasAddress::operator[](PINDEX i) const
{
  return (H225_AliasAddress &)array[i];
}


PObject * H4501_ArrayOf_AliasAddress::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_ArrayOf_AliasAddress::Class()), PInvalidCast);
#endif
  return new H4501_ArrayOf_AliasAddress(*this);
}


//
// NetworkFacilityExtension
//

H4501_NetworkFacilityExtension::H4501_NetworkFacilityExtension(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Sequence(tag, tagClass, 2, TRUE, 0)
{
}


#ifndef PASN_NOPRINTON
void H4501_NetworkFacilityExtension::PrintOn(ostream & strm) const
{
  int indent = strm.precision() + 2;
  strm << "{\n";
  strm << setw(indent+15) << "sourceEntity = " << setprecision(indent) << m_sourceEntity << '\n';
  if (HasOptionalField(e_sourceEntityAddress))
    strm << setw(indent+22) << "sourceEntityAddress = " << setprecision(indent) << m_sourceEntityAddress << '\n';
  strm << setw(indent+20) << "destinationEntity = " << setprecision(indent) << m_destinationEntity << '\n';
  if (HasOptionalField(e_destinationEntityAddress))
    strm << setw(indent+27) << "destinationEntityAddress = " << setprecision(indent) << m_destinationEntityAddress << '\n';
  strm << setw(indent-1) << setprecision(indent-2) << "}";
}
#endif


PObject::Comparison H4501_NetworkFacilityExtension::Compare(const PObject & obj) const
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(&obj, H4501_NetworkFacilityExtension), PInvalidCast);
#endif
  const H4501_NetworkFacilityExtension & other = (const H4501_NetworkFacilityExtension &)obj;

  Comparison result;

  if ((result = m_sourceEntity.Compare(other.m_sourceEntity)) != EqualTo)
    return result;
  if ((result = m_sourceEntityAddress.Compare(other.m_sourceEntityAddress)) != EqualTo)
    return result;
  if ((result = m_destinationEntity.Compare(other.m_destinationEntity)) != EqualTo)
    return result;
  if ((result = m_destinationEntityAddress.Compare(other.m_destinationEntityAddress)) != EqualTo)
    return result;

  return PASN_Sequence::Compare(other);
}


PINDEX H4501_NetworkFacilityExtension::GetDataLength() const
{
  PINDEX length = 0;
  length += m_sourceEntity.GetObjectLength();
  if (HasOptionalField(e_sourceEntityAddress))
    length += m_sourceEntityAddress.GetObjectLength();
  length += m_destinationEntity.GetObjectLength();
  if (HasOptionalField(e_destinationEntityAddress))
    length += m_destinationEntityAddress.GetObjectLength();
  return length;
}


BOOL H4501_NetworkFacilityExtension::Decode(PASN_Stream & strm)
{
  if (!PreambleDecode(strm))
    return FALSE;

  if (!m_sourceEntity.Decode(strm))
    return FALSE;
  if (HasOptionalField(e_sourceEntityAddress) && !m_sourceEntityAddress.Decode(strm))
    return FALSE;
  if (!m_destinationEntity.Decode(strm))
    return FALSE;
  if (HasOptionalField(e_destinationEntityAddress) && !m_destinationEntityAddress.Decode(strm))
    return FALSE;

  return UnknownExtensionsDecode(strm);
}


void H4501_NetworkFacilityExtension::Encode(PASN_Stream & strm) const
{
  PreambleEncode(strm);

  m_sourceEntity.Encode(strm);
  if (HasOptionalField(e_sourceEntityAddress))
    m_sourceEntityAddress.Encode(strm);
  m_destinationEntity.Encode(strm);
  if (HasOptionalField(e_destinationEntityAddress))
    m_destinationEntityAddress.Encode(strm);

  UnknownExtensionsEncode(strm);
}


PObject * H4501_NetworkFacilityExtension::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_NetworkFacilityExtension::Class()), PInvalidCast);
#endif
  return new H4501_NetworkFacilityExtension(*this);
}


//
// AddressScreened
//

H4501_AddressScreened::H4501_AddressScreened(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Sequence(tag, tagClass, 1, TRUE, 0)
{
}


#ifndef PASN_NOPRINTON
void H4501_AddressScreened::PrintOn(ostream & strm) const
{
  int indent = strm.precision() + 2;
  strm << "{\n";
  strm << setw(indent+14) << "partyNumber = " << setprecision(indent) << m_partyNumber << '\n';
  strm << setw(indent+21) << "screeningIndicator = " << setprecision(indent) << m_screeningIndicator << '\n';
  if (HasOptionalField(e_partySubaddress))
    strm << setw(indent+18) << "partySubaddress = " << setprecision(indent) << m_partySubaddress << '\n';
  strm << setw(indent-1) << setprecision(indent-2) << "}";
}
#endif


PObject::Comparison H4501_AddressScreened::Compare(const PObject & obj) const
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(&obj, H4501_AddressScreened), PInvalidCast);
#endif
  const H4501_AddressScreened & other = (const H4501_AddressScreened &)obj;

  Comparison result;

  if ((result = m_partyNumber.Compare(other.m_partyNumber)) != EqualTo)
    return result;
  if ((result = m_screeningIndicator.Compare(other.m_screeningIndicator)) != EqualTo)
    return result;
  if ((result = m_partySubaddress.Compare(other.m_partySubaddress)) != EqualTo)
    return result;

  return PASN_Sequence::Compare(other);
}


PINDEX H4501_AddressScreened::GetDataLength() const
{
  PINDEX length = 0;
  length += m_partyNumber.GetObjectLength();
  length += m_screeningIndicator.GetObjectLength();
  if (HasOptionalField(e_partySubaddress))
    length += m_partySubaddress.GetObjectLength();
  return length;
}


BOOL H4501_AddressScreened::Decode(PASN_Stream & strm)
{
  if (!PreambleDecode(strm))
    return FALSE;

  if (!m_partyNumber.Decode(strm))
    return FALSE;
  if (!m_screeningIndicator.Decode(strm))
    return FALSE;
  if (HasOptionalField(e_partySubaddress) && !m_partySubaddress.Decode(strm))
    return FALSE;

  return UnknownExtensionsDecode(strm);
}


void H4501_AddressScreened::Encode(PASN_Stream & strm) const
{
  PreambleEncode(strm);

  m_partyNumber.Encode(strm);
  m_screeningIndicator.Encode(strm);
  if (HasOptionalField(e_partySubaddress))
    m_partySubaddress.Encode(strm);

  UnknownExtensionsEncode(strm);
}


PObject * H4501_AddressScreened::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_AddressScreened::Class()), PInvalidCast);
#endif
  return new H4501_AddressScreened(*this);
}


//
// NumberScreened
//

H4501_NumberScreened::H4501_NumberScreened(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Sequence(tag, tagClass, 0, TRUE, 0)
{
}


#ifndef PASN_NOPRINTON
void H4501_NumberScreened::PrintOn(ostream & strm) const
{
  int indent = strm.precision() + 2;
  strm << "{\n";
  strm << setw(indent+14) << "partyNumber = " << setprecision(indent) << m_partyNumber << '\n';
  strm << setw(indent+21) << "screeningIndicator = " << setprecision(indent) << m_screeningIndicator << '\n';
  strm << setw(indent-1) << setprecision(indent-2) << "}";
}
#endif


PObject::Comparison H4501_NumberScreened::Compare(const PObject & obj) const
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(&obj, H4501_NumberScreened), PInvalidCast);
#endif
  const H4501_NumberScreened & other = (const H4501_NumberScreened &)obj;

  Comparison result;

  if ((result = m_partyNumber.Compare(other.m_partyNumber)) != EqualTo)
    return result;
  if ((result = m_screeningIndicator.Compare(other.m_screeningIndicator)) != EqualTo)
    return result;

  return PASN_Sequence::Compare(other);
}


PINDEX H4501_NumberScreened::GetDataLength() const
{
  PINDEX length = 0;
  length += m_partyNumber.GetObjectLength();
  length += m_screeningIndicator.GetObjectLength();
  return length;
}


BOOL H4501_NumberScreened::Decode(PASN_Stream & strm)
{
  if (!PreambleDecode(strm))
    return FALSE;

  if (!m_partyNumber.Decode(strm))
    return FALSE;
  if (!m_screeningIndicator.Decode(strm))
    return FALSE;

  return UnknownExtensionsDecode(strm);
}


void H4501_NumberScreened::Encode(PASN_Stream & strm) const
{
  PreambleEncode(strm);

  m_partyNumber.Encode(strm);
  m_screeningIndicator.Encode(strm);

  UnknownExtensionsEncode(strm);
}


PObject * H4501_NumberScreened::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_NumberScreened::Class()), PInvalidCast);
#endif
  return new H4501_NumberScreened(*this);
}


//
// Address
//

H4501_Address::H4501_Address(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Sequence(tag, tagClass, 1, TRUE, 0)
{
}


#ifndef PASN_NOPRINTON
void H4501_Address::PrintOn(ostream & strm) const
{
  int indent = strm.precision() + 2;
  strm << "{\n";
  strm << setw(indent+14) << "partyNumber = " << setprecision(indent) << m_partyNumber << '\n';
  if (HasOptionalField(e_partySubaddress))
    strm << setw(indent+18) << "partySubaddress = " << setprecision(indent) << m_partySubaddress << '\n';
  strm << setw(indent-1) << setprecision(indent-2) << "}";
}
#endif


PObject::Comparison H4501_Address::Compare(const PObject & obj) const
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(&obj, H4501_Address), PInvalidCast);
#endif
  const H4501_Address & other = (const H4501_Address &)obj;

  Comparison result;

  if ((result = m_partyNumber.Compare(other.m_partyNumber)) != EqualTo)
    return result;
  if ((result = m_partySubaddress.Compare(other.m_partySubaddress)) != EqualTo)
    return result;

  return PASN_Sequence::Compare(other);
}


PINDEX H4501_Address::GetDataLength() const
{
  PINDEX length = 0;
  length += m_partyNumber.GetObjectLength();
  if (HasOptionalField(e_partySubaddress))
    length += m_partySubaddress.GetObjectLength();
  return length;
}


BOOL H4501_Address::Decode(PASN_Stream & strm)
{
  if (!PreambleDecode(strm))
    return FALSE;

  if (!m_partyNumber.Decode(strm))
    return FALSE;
  if (HasOptionalField(e_partySubaddress) && !m_partySubaddress.Decode(strm))
    return FALSE;

  return UnknownExtensionsDecode(strm);
}


void H4501_Address::Encode(PASN_Stream & strm) const
{
  PreambleEncode(strm);

  m_partyNumber.Encode(strm);
  if (HasOptionalField(e_partySubaddress))
    m_partySubaddress.Encode(strm);

  UnknownExtensionsEncode(strm);
}


PObject * H4501_Address::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_Address::Class()), PInvalidCast);
#endif
  return new H4501_Address(*this);
}


//
// EndpointAddress
//

H4501_EndpointAddress::H4501_EndpointAddress(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Sequence(tag, tagClass, 1, TRUE, 0)
{
}


#ifndef PASN_NOPRINTON
void H4501_EndpointAddress::PrintOn(ostream & strm) const
{
  int indent = strm.precision() + 2;
  strm << "{\n";
  strm << setw(indent+21) << "destinationAddress = " << setprecision(indent) << m_destinationAddress << '\n';
  if (HasOptionalField(e_remoteExtensionAddress))
    strm << setw(indent+25) << "remoteExtensionAddress = " << setprecision(indent) << m_remoteExtensionAddress << '\n';
  strm << setw(indent-1) << setprecision(indent-2) << "}";
}
#endif


PObject::Comparison H4501_EndpointAddress::Compare(const PObject & obj) const
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(&obj, H4501_EndpointAddress), PInvalidCast);
#endif
  const H4501_EndpointAddress & other = (const H4501_EndpointAddress &)obj;

  Comparison result;

  if ((result = m_destinationAddress.Compare(other.m_destinationAddress)) != EqualTo)
    return result;
  if ((result = m_remoteExtensionAddress.Compare(other.m_remoteExtensionAddress)) != EqualTo)
    return result;

  return PASN_Sequence::Compare(other);
}


PINDEX H4501_EndpointAddress::GetDataLength() const
{
  PINDEX length = 0;
  length += m_destinationAddress.GetObjectLength();
  if (HasOptionalField(e_remoteExtensionAddress))
    length += m_remoteExtensionAddress.GetObjectLength();
  return length;
}


BOOL H4501_EndpointAddress::Decode(PASN_Stream & strm)
{
  if (!PreambleDecode(strm))
    return FALSE;

  if (!m_destinationAddress.Decode(strm))
    return FALSE;
  if (HasOptionalField(e_remoteExtensionAddress) && !m_remoteExtensionAddress.Decode(strm))
    return FALSE;

  return UnknownExtensionsDecode(strm);
}


void H4501_EndpointAddress::Encode(PASN_Stream & strm) const
{
  PreambleEncode(strm);

  m_destinationAddress.Encode(strm);
  if (HasOptionalField(e_remoteExtensionAddress))
    m_remoteExtensionAddress.Encode(strm);

  UnknownExtensionsEncode(strm);
}


PObject * H4501_EndpointAddress::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_EndpointAddress::Class()), PInvalidCast);
#endif
  return new H4501_EndpointAddress(*this);
}


//
// UserSpecifiedSubaddress
//

H4501_UserSpecifiedSubaddress::H4501_UserSpecifiedSubaddress(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Sequence(tag, tagClass, 1, TRUE, 0)
{
}


#ifndef PASN_NOPRINTON
void H4501_UserSpecifiedSubaddress::PrintOn(ostream & strm) const
{
  int indent = strm.precision() + 2;
  strm << "{\n";
  strm << setw(indent+24) << "subaddressInformation = " << setprecision(indent) << m_subaddressInformation << '\n';
  if (HasOptionalField(e_oddCountIndicator))
    strm << setw(indent+20) << "oddCountIndicator = " << setprecision(indent) << m_oddCountIndicator << '\n';
  strm << setw(indent-1) << setprecision(indent-2) << "}";
}
#endif


PObject::Comparison H4501_UserSpecifiedSubaddress::Compare(const PObject & obj) const
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(&obj, H4501_UserSpecifiedSubaddress), PInvalidCast);
#endif
  const H4501_UserSpecifiedSubaddress & other = (const H4501_UserSpecifiedSubaddress &)obj;

  Comparison result;

  if ((result = m_subaddressInformation.Compare(other.m_subaddressInformation)) != EqualTo)
    return result;
  if ((result = m_oddCountIndicator.Compare(other.m_oddCountIndicator)) != EqualTo)
    return result;

  return PASN_Sequence::Compare(other);
}


PINDEX H4501_UserSpecifiedSubaddress::GetDataLength() const
{
  PINDEX length = 0;
  length += m_subaddressInformation.GetObjectLength();
  if (HasOptionalField(e_oddCountIndicator))
    length += m_oddCountIndicator.GetObjectLength();
  return length;
}


BOOL H4501_UserSpecifiedSubaddress::Decode(PASN_Stream & strm)
{
  if (!PreambleDecode(strm))
    return FALSE;

  if (!m_subaddressInformation.Decode(strm))
    return FALSE;
  if (HasOptionalField(e_oddCountIndicator) && !m_oddCountIndicator.Decode(strm))
    return FALSE;

  return UnknownExtensionsDecode(strm);
}


void H4501_UserSpecifiedSubaddress::Encode(PASN_Stream & strm) const
{
  PreambleEncode(strm);

  m_subaddressInformation.Encode(strm);
  if (HasOptionalField(e_oddCountIndicator))
    m_oddCountIndicator.Encode(strm);

  UnknownExtensionsEncode(strm);
}


PObject * H4501_UserSpecifiedSubaddress::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_UserSpecifiedSubaddress::Class()), PInvalidCast);
#endif
  return new H4501_UserSpecifiedSubaddress(*this);
}


//
// SupplementaryService
//

H4501_SupplementaryService::H4501_SupplementaryService(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Sequence(tag, tagClass, 2, TRUE, 0)
{
}


#ifndef PASN_NOPRINTON
void H4501_SupplementaryService::PrintOn(ostream & strm) const
{
  int indent = strm.precision() + 2;
  strm << "{\n";
  if (HasOptionalField(e_networkFacilityExtension))
    strm << setw(indent+27) << "networkFacilityExtension = " << setprecision(indent) << m_networkFacilityExtension << '\n';
  if (HasOptionalField(e_interpretationApdu))
    strm << setw(indent+21) << "interpretationApdu = " << setprecision(indent) << m_interpretationApdu << '\n';
  strm << setw(indent+14) << "serviceApdu = " << setprecision(indent) << m_serviceApdu << '\n';
  strm << setw(indent-1) << setprecision(indent-2) << "}";
}
#endif


PObject::Comparison H4501_SupplementaryService::Compare(const PObject & obj) const
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(&obj, H4501_SupplementaryService), PInvalidCast);
#endif
  const H4501_SupplementaryService & other = (const H4501_SupplementaryService &)obj;

  Comparison result;

  if ((result = m_networkFacilityExtension.Compare(other.m_networkFacilityExtension)) != EqualTo)
    return result;
  if ((result = m_interpretationApdu.Compare(other.m_interpretationApdu)) != EqualTo)
    return result;
  if ((result = m_serviceApdu.Compare(other.m_serviceApdu)) != EqualTo)
    return result;

  return PASN_Sequence::Compare(other);
}


PINDEX H4501_SupplementaryService::GetDataLength() const
{
  PINDEX length = 0;
  if (HasOptionalField(e_networkFacilityExtension))
    length += m_networkFacilityExtension.GetObjectLength();
  if (HasOptionalField(e_interpretationApdu))
    length += m_interpretationApdu.GetObjectLength();
  length += m_serviceApdu.GetObjectLength();
  return length;
}


BOOL H4501_SupplementaryService::Decode(PASN_Stream & strm)
{
  if (!PreambleDecode(strm))
    return FALSE;

  if (HasOptionalField(e_networkFacilityExtension) && !m_networkFacilityExtension.Decode(strm))
    return FALSE;
  if (HasOptionalField(e_interpretationApdu) && !m_interpretationApdu.Decode(strm))
    return FALSE;
  if (!m_serviceApdu.Decode(strm))
    return FALSE;

  return UnknownExtensionsDecode(strm);
}


void H4501_SupplementaryService::Encode(PASN_Stream & strm) const
{
  PreambleEncode(strm);

  if (HasOptionalField(e_networkFacilityExtension))
    m_networkFacilityExtension.Encode(strm);
  if (HasOptionalField(e_interpretationApdu))
    m_interpretationApdu.Encode(strm);
  m_serviceApdu.Encode(strm);

  UnknownExtensionsEncode(strm);
}


PObject * H4501_SupplementaryService::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H4501_SupplementaryService::Class()), PInvalidCast);
#endif
  return new H4501_SupplementaryService(*this);
}


#endif // if ! H323_DISABLE_H4501


// End of h4501.cxx
