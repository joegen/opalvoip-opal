//
// h45010.cxx
//
// Code automatically generated by asnparse.
//

#ifdef P_USE_PRAGMA
#pragma implementation "h45010.h"
#endif

#include <ptlib.h>
#include "h45010.h"

#define new PNEW


#if ! H323_DISABLE_H45010



#ifndef PASN_NOPRINTON
const static PASN_Names Names_H45010_H323CallOfferOperations[]={
        {"callOfferRequest",34}
       ,{"remoteUserAlerting",115}
       ,{"cfbOverride",49}
};
#endif
//
// H323CallOfferOperations
//

H45010_H323CallOfferOperations::H45010_H323CallOfferOperations(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Enumeration(tag, tagClass, 115, FALSE
#ifndef PASN_NOPRINTON
    ,(const PASN_Names *)Names_H45010_H323CallOfferOperations,3
#endif
    )
{
}


H45010_H323CallOfferOperations & H45010_H323CallOfferOperations::operator=(unsigned v)
{
  SetValue(v);
  return *this;
}


PObject * H45010_H323CallOfferOperations::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H45010_H323CallOfferOperations::Class()), PInvalidCast);
#endif
  return new H45010_H323CallOfferOperations(*this);
}


//
// ArrayOf_MixedExtension
//

H45010_ArrayOf_MixedExtension::H45010_ArrayOf_MixedExtension(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Array(tag, tagClass)
{
}


PASN_Object * H45010_ArrayOf_MixedExtension::CreateObject() const
{
  return new H4504_MixedExtension;
}


H4504_MixedExtension & H45010_ArrayOf_MixedExtension::operator[](PINDEX i) const
{
  return (H4504_MixedExtension &)array[i];
}


PObject * H45010_ArrayOf_MixedExtension::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H45010_ArrayOf_MixedExtension::Class()), PInvalidCast);
#endif
  return new H45010_ArrayOf_MixedExtension(*this);
}


//
// CoReqOptArg
//

H45010_CoReqOptArg::H45010_CoReqOptArg(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Sequence(tag, tagClass, 1, TRUE, 0)
{
  m_extension.SetConstraints(PASN_Object::FixedConstraint, 0, 255);
}


#ifndef PASN_NOPRINTON
void H45010_CoReqOptArg::PrintOn(ostream & strm) const
{
  int indent = strm.precision() + 2;
  strm << "{\n";
  if (HasOptionalField(e_extension))
    strm << setw(indent+12) << "extension = " << setprecision(indent) << m_extension << '\n';
  strm << setw(indent-1) << setprecision(indent-2) << "}";
}
#endif


PObject::Comparison H45010_CoReqOptArg::Compare(const PObject & obj) const
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(&obj, H45010_CoReqOptArg), PInvalidCast);
#endif
  const H45010_CoReqOptArg & other = (const H45010_CoReqOptArg &)obj;

  Comparison result;

  if ((result = m_extension.Compare(other.m_extension)) != EqualTo)
    return result;

  return PASN_Sequence::Compare(other);
}


PINDEX H45010_CoReqOptArg::GetDataLength() const
{
  PINDEX length = 0;
  if (HasOptionalField(e_extension))
    length += m_extension.GetObjectLength();
  return length;
}


BOOL H45010_CoReqOptArg::Decode(PASN_Stream & strm)
{
  if (!PreambleDecode(strm))
    return FALSE;

  if (HasOptionalField(e_extension) && !m_extension.Decode(strm))
    return FALSE;

  return UnknownExtensionsDecode(strm);
}


void H45010_CoReqOptArg::Encode(PASN_Stream & strm) const
{
  PreambleEncode(strm);

  if (HasOptionalField(e_extension))
    m_extension.Encode(strm);

  UnknownExtensionsEncode(strm);
}


PObject * H45010_CoReqOptArg::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H45010_CoReqOptArg::Class()), PInvalidCast);
#endif
  return new H45010_CoReqOptArg(*this);
}


//
// RUAlertOptArg
//

H45010_RUAlertOptArg::H45010_RUAlertOptArg(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Sequence(tag, tagClass, 1, TRUE, 0)
{
  m_extension.SetConstraints(PASN_Object::FixedConstraint, 0, 255);
}


#ifndef PASN_NOPRINTON
void H45010_RUAlertOptArg::PrintOn(ostream & strm) const
{
  int indent = strm.precision() + 2;
  strm << "{\n";
  if (HasOptionalField(e_extension))
    strm << setw(indent+12) << "extension = " << setprecision(indent) << m_extension << '\n';
  strm << setw(indent-1) << setprecision(indent-2) << "}";
}
#endif


PObject::Comparison H45010_RUAlertOptArg::Compare(const PObject & obj) const
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(&obj, H45010_RUAlertOptArg), PInvalidCast);
#endif
  const H45010_RUAlertOptArg & other = (const H45010_RUAlertOptArg &)obj;

  Comparison result;

  if ((result = m_extension.Compare(other.m_extension)) != EqualTo)
    return result;

  return PASN_Sequence::Compare(other);
}


PINDEX H45010_RUAlertOptArg::GetDataLength() const
{
  PINDEX length = 0;
  if (HasOptionalField(e_extension))
    length += m_extension.GetObjectLength();
  return length;
}


BOOL H45010_RUAlertOptArg::Decode(PASN_Stream & strm)
{
  if (!PreambleDecode(strm))
    return FALSE;

  if (HasOptionalField(e_extension) && !m_extension.Decode(strm))
    return FALSE;

  return UnknownExtensionsDecode(strm);
}


void H45010_RUAlertOptArg::Encode(PASN_Stream & strm) const
{
  PreambleEncode(strm);

  if (HasOptionalField(e_extension))
    m_extension.Encode(strm);

  UnknownExtensionsEncode(strm);
}


PObject * H45010_RUAlertOptArg::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H45010_RUAlertOptArg::Class()), PInvalidCast);
#endif
  return new H45010_RUAlertOptArg(*this);
}


//
// CfbOvrOptArg
//

H45010_CfbOvrOptArg::H45010_CfbOvrOptArg(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Sequence(tag, tagClass, 1, TRUE, 0)
{
  m_extension.SetConstraints(PASN_Object::FixedConstraint, 0, 255);
}


#ifndef PASN_NOPRINTON
void H45010_CfbOvrOptArg::PrintOn(ostream & strm) const
{
  int indent = strm.precision() + 2;
  strm << "{\n";
  if (HasOptionalField(e_extension))
    strm << setw(indent+12) << "extension = " << setprecision(indent) << m_extension << '\n';
  strm << setw(indent-1) << setprecision(indent-2) << "}";
}
#endif


PObject::Comparison H45010_CfbOvrOptArg::Compare(const PObject & obj) const
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(&obj, H45010_CfbOvrOptArg), PInvalidCast);
#endif
  const H45010_CfbOvrOptArg & other = (const H45010_CfbOvrOptArg &)obj;

  Comparison result;

  if ((result = m_extension.Compare(other.m_extension)) != EqualTo)
    return result;

  return PASN_Sequence::Compare(other);
}


PINDEX H45010_CfbOvrOptArg::GetDataLength() const
{
  PINDEX length = 0;
  if (HasOptionalField(e_extension))
    length += m_extension.GetObjectLength();
  return length;
}


BOOL H45010_CfbOvrOptArg::Decode(PASN_Stream & strm)
{
  if (!PreambleDecode(strm))
    return FALSE;

  if (HasOptionalField(e_extension) && !m_extension.Decode(strm))
    return FALSE;

  return UnknownExtensionsDecode(strm);
}


void H45010_CfbOvrOptArg::Encode(PASN_Stream & strm) const
{
  PreambleEncode(strm);

  if (HasOptionalField(e_extension))
    m_extension.Encode(strm);

  UnknownExtensionsEncode(strm);
}


PObject * H45010_CfbOvrOptArg::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H45010_CfbOvrOptArg::Class()), PInvalidCast);
#endif
  return new H45010_CfbOvrOptArg(*this);
}


#endif // if ! H323_DISABLE_H45010


// End of h45010.cxx
