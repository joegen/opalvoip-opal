/*
 * cdr.cxx
 *
 * OPAL Server main program
 *
 * Copyright (c) 2012 BCS Global.
 * Copyright (c) 2014 Vox Lucida Pty. Ltd.
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): BCS Global, Inc.
 *                 Vox Lucida Pty. Ltd.
 *
 */

#include "precompile.h"
#include "main.h"

static const PConstString CDRTextFileKey("CDR Text File");
static const PConstString CDRTextHeadingsKey("CDR Text Headings");
static const PConstString CDRTextFormatKey("CDR Text Format");

#if P_ODBC
static const PConstString CDRDriverKey("CDR Database Driver");
static const PConstString CDRHostKey("CDR Database Host");
static const PConstString CDRPortKey("CDR Database Port");
static const PConstString CDRDatabaseKey("CDR Database Name");
static const PConstString CDRUsernameKey("CDR Database Username");
static const PConstString CDRPasswordKey("CDR Database Password");
static const PConstString CDRTableKey("CDR Database Table");
static const PConstString CDRCreateKey("CDR Create");

static const PConstString CDRFieldMapPrefix("CDR Database Field: ");
#endif // P_ODBC


static struct
{
  PVarType::BasicType m_type;
  unsigned            m_size;
  const char *        m_name;
} const CDRFields[MyCall::NumFieldCodes] = {
  { PVarType::VarDynamicString,  36, "CallId"                       },
  { PVarType::VarTime,            0, "StartTime"                    },
  { PVarType::VarTime,            0, "ConnectTime"                  },
  { PVarType::VarTime,            0, "EndTime"                      },
  { PVarType::VarInt16,           0, "CallState"                    },  // -4=Proceeding, -3=Alerting, -2=Connected, -1=Established, >=0 EndedByXXX
  { PVarType::VarDynamicString,   0, "CallResult"                   }, // EndedByXXX text
  { PVarType::VarDynamicString,  50, "OriginatorID"                 },
  { PVarType::VarDynamicString, 100, "OriginatorURI"                },
  { PVarType::VarDynamicString,  46, "OriginatorSignalAddress"      },   // ip4:port or [ipv6]:port form
  { PVarType::VarDynamicString,  15, "DialedNumber"                 },
  { PVarType::VarDynamicString,  50, "DestinationID"                },
  { PVarType::VarDynamicString, 100, "DestinationURI"               },
  { PVarType::VarDynamicString,  46, "DestinationSignalAddress"     },      // ip4:port or [ipv6]:port form
  { PVarType::VarDynamicString,  15, "AudioCodec"                   },
  { PVarType::VarDynamicString,  46, "AudioOriginatorMediaAddress"  },   // ip4:port or [ipv6]:port form
  { PVarType::VarDynamicString,  46, "AudioDestinationMediaAddress" },  // ip4:port or [ipv6]:port form
  { PVarType::VarDynamicString,  15, "VideoCodec"                   },
  { PVarType::VarDynamicString,  46, "VideoOriginatorMediaAddress"  },   // ip4:port or [ipv6]:port form
  { PVarType::VarDynamicString,  46, "VideoDestinationMediaAddress" },  // ip4:port or [ipv6]:port form
  { PVarType::VarInt32,           0, "Bandwidth"                    } // kbps
};


static PString GetDefaultTextHeadings()
{
  PStringStream strm;

  for (MyCall::FieldCodes f = MyCall::BeginFieldCodes; f < MyCall::EndFieldCodes; ++f) {
    if (f > MyCall::BeginFieldCodes)
      strm << ',';
    strm << CDRFields[f].m_name;
  }

  return strm;
}


static PString GetDefaultTextFormats()
{
  PStringStream strm;

  for (MyCall::FieldCodes f = MyCall::BeginFieldCodes; f < MyCall::EndFieldCodes; ++f) {
    if (f > MyCall::BeginFieldCodes)
      strm << ',';
    if (CDRFields[f].m_type != PVarType::VarDynamicString)
      strm << '%' << CDRFields[f].m_name << '%';
    else
      strm << "\"%" << CDRFields[f].m_name << "%\"";
  }

  return strm;
}


///////////////////////////////////////////////////////////////////////////////

bool MyManager::ConfigureCDR(PConfig & cfg, PConfigPage * rsrc)
{
  m_cdrMutex.Wait();

  m_cdrTextFile.Close();

  PString filename = rsrc->AddStringField(CDRTextFileKey, 0, m_cdrTextFile.GetFilePath(), "Call Detail Record text file name", 1, 75);
  PString cdrHeadings = rsrc->AddStringField(CDRTextHeadingsKey, 0, GetDefaultTextHeadings(), "Call Detail Record text output headings", 1, 75);
  m_cdrFormat = rsrc->AddStringField(CDRTextFormatKey, 0, GetDefaultTextFormats(), "Call Detail Record text output format", 1, 75);

  if (!filename.IsEmpty()) {
    if (m_cdrTextFile.Open(filename, PFile::WriteOnly, PFile::Create)) {
      m_cdrTextFile.SetPosition(0, PFile::End);
      if (m_cdrTextFile.GetPosition() == 0)
        m_cdrTextFile << cdrHeadings << endl;
    }
    else
      PSYSTEMLOG(Error, "Could not open CDR text file \"" << filename << '"');
  }

#if P_ODBC
  PHTML sources(PHTML::InBody);
  sources << "Call Detail Record source name for ODBC";

  PODBC::ConnectData connectData;
  connectData.m_driver = cfg.GetEnum(CDRDriverKey, PODBC::postgreSQL);
  static const char * const DriverNames[] = {
    "DSN", "mySQL", "postgreSQL", "Oracle", "IBM DB2", "Microsoft SQL", "Microsoft Access",
    "Paradox", "Foxpro", "dBase", "Excel", "Ascii", "Connection String"
  };
  rsrc->Add(new PHTTPEnumField<PODBC::DriverType>(CDRDriverKey, PARRAYSIZE(DriverNames), DriverNames,
    connectData.m_driver, "Call Detail Record driver for ODBC"));

  connectData.m_host = rsrc->AddStringField(CDRHostKey, 50, connectData.m_host,
                        "Call Detail Record database host for ODBC, if empty localhost is used");

  connectData.m_port = rsrc->AddIntegerField(CDRPortKey, 0, 65535, connectData.m_port,
                        "", "Call Detail Record database name for ODBC, if zero, default for driver is used");

  connectData.m_database = rsrc->AddStringField(CDRDatabaseKey, 50, connectData.m_database, "Call Detail Record database name for ODBC");
  connectData.m_username = rsrc->AddStringField(CDRUsernameKey, 30, connectData.m_username, "Call Detail Record user name for ODBC");

  connectData.m_password = PHTTPPasswordField::Decrypt(cfg.GetString(CDRPasswordKey));
  rsrc->Add(new PHTTPPasswordField(CDRPasswordKey, 30, connectData.m_password, "Call Detail Record password for ODBC"));

  m_cdrTable = rsrc->AddStringField(CDRTableKey, 30, m_cdrTable, "Call Detail Record table name for ODBC");
  bool create = rsrc->AddBooleanField(CDRCreateKey, true, "Create Call Detail Record database/table if does not exist");

  for (MyCall::FieldCodes f = MyCall::BeginFieldCodes; f < MyCall::EndFieldCodes; ++f)
    m_cdrFieldNames[f] = rsrc->AddStringField(CDRFieldMapPrefix + CDRFields[f].m_name, 30, CDRFields[f].m_name);

  if (!m_cdrTable.IsEmpty()) {
    if (!m_odbc.Connect(connectData))
      PSYSTEMLOG(Error, "Could not connect to ODBC source \"" << connectData.m_database << "\" - " << m_odbc.GetLastErrorText());
    else if (create && m_odbc.TableList("TABLE").GetValuesIndex(m_cdrTable) == P_MAX_INDEX) {
      PStringStream sql;
      sql << "CREATE TABLE " << m_cdrTable << " (";
      for (MyCall::FieldCodes f = MyCall::BeginFieldCodes; f < MyCall::EndFieldCodes; ++f) {
        if (f > MyCall::BeginFieldCodes)
          sql << ',';
        sql << CDRFields[f].m_name << ' ' << PODBC::GetFieldType(connectData.m_driver, CDRFields[f].m_type, CDRFields[f].m_size);
        if (f == MyCall::CallId)
          sql << " PRIMARY KEY";
      }
      sql << ");";
      if (!m_odbc.Execute(sql)) {
        PSYSTEMLOG(Error, "Could not create table \"" << m_cdrTable << "\" - " << m_odbc.GetLastErrorText());
      }
    }
  }
#endif // P_ODBC

  m_cdrListMax = rsrc->AddIntegerField("Web Page CDR Limit", 1, 1000000, m_cdrListMax,
                                       "", "Maximum number of CDR records saved for display on web page.");

  m_cdrMutex.Signal();

  return true;
}


void MyManager::DropCDR(const MyCall & call, bool final)
{
  m_cdrMutex.Wait();

  if (final) {
    m_cdrList.push_back(call);
    while (m_cdrList.size() > m_cdrListMax)
      m_cdrList.pop_front();

    if (m_cdrTextFile.IsOpen()) {
      call.OutputText(m_cdrTextFile, m_cdrFormat.IsEmpty() ? GetDefaultTextFormats() : m_cdrFormat);
      PSYSTEMLOG(Info, "Dropped text CDR for " << call.GetGUID());
    }
  }

#if P_ODBC
  if (m_odbc.IsConnected()) {
    PODBC::RecordSet data(m_odbc);
    if (data.Select(m_cdrTable, m_cdrFieldNames[MyCall::CallId] + "='" + call.GetGUID() + '\'') && data.First()) {
      call.OutputSQL(data[1], m_cdrFieldNames);
      if (data.Commit())
        PSYSTEMLOG(Info, "Dropped " << (final ? "final" : "intermediate") << " SQL CDR for " << call.GetGUID());
      else
        PSYSTEMLOG(Info, "Could not drop " << (final ? "final" : "intermediate") << " SQL CDR for " << call.GetGUID());
    }
    else if (data.Query(m_cdrTable)) {
      call.OutputSQL(data.NewRow(), m_cdrFieldNames);
      if (data.Commit())
        PSYSTEMLOG(Info, "Dropped first SQL CDR for " << call.GetGUID());
      else
        PSYSTEMLOG(Info, "Could not drop first SQL CDR for " << call.GetGUID());
    }
    else {
      PSYSTEMLOG(Info, "Could not drop SQL CDR for " << call.GetGUID());
    }
  }
#endif // P_ODBC

  m_cdrMutex.Signal();
}


MyManager::CDRList::const_iterator MyManager::BeginCDR()
{
  m_cdrMutex.Wait();
  return m_cdrList.begin();
}


bool MyManager::NotEndCDR(const CDRList::const_iterator & it)
{
  if (it != m_cdrList.end())
    return true;

  m_cdrMutex.Signal();
  return false;
}


bool MyManager::FindCDR(const PString & guid, CallDetailRecord & cdr)
{
  PWaitAndSignal mutex(m_cdrMutex);

  for (MyManager::CDRList::const_iterator it = m_cdrList.begin(); it != m_cdrList.end(); ++it) {
    if (it->GetGUID() == guid) {
      cdr = *it;
      return true;
    }
  }

  return false;
}


///////////////////////////////////////////////////////////////////////////////

MyCall::MyCall(MyManager & manager)
  : OpalCall(manager)
  , m_manager(manager)
{
}


PBoolean MyCall::OnSetUp(OpalConnection & connection)
{
  if (!OpalCall::OnSetUp(connection))
    return false;

  PSafePtr<OpalConnection> a = GetConnection(0);
  if (a != NULL) {
    m_OriginatorID = a->GetIdentifier();
    m_OriginatorURI = a->GetRemotePartyURL();
    m_DestinationURI = a->GetDestinationAddress();
    a->GetRemoteAddress().GetIpAndPort(m_OriginatorSignalAddress);
  }

  PSafePtr<OpalConnection> b = GetConnection(1);
  if (b != NULL) {
    m_DestinationID = b->GetIdentifier();
    m_DestinationURI = b->GetRemotePartyURL();
    m_DialedNumber = b->GetRemotePartyNumber();
  }

  m_manager.DropCDR(*this, false);

  return true;
}


void MyCall::OnProceeding(OpalConnection & connection)
{
  m_CallState = MyCall::CallProceeding;
  m_DestinationURI = connection.GetRemotePartyURL();
  connection.GetRemoteAddress().GetIpAndPort(m_DestinationSignalAddress);

  m_manager.DropCDR(*this, false);

  OpalCall::OnProceeding(connection);
}


PBoolean MyCall::OnAlerting(OpalConnection & connection)
{
  m_CallState = MyCall::CallAlerting;
  connection.GetRemoteAddress().GetIpAndPort(m_DestinationSignalAddress);

  m_manager.DropCDR(*this, false);

  return OpalCall::OnAlerting(connection);
}


PBoolean MyCall::OnConnected(OpalConnection & connection)
{
  m_ConnectTime.SetCurrentTime();
  m_CallState = MyCall::CallConnected;
  connection.GetRemoteAddress().GetIpAndPort(m_DestinationSignalAddress);

  m_manager.DropCDR(*this, false);

  return OpalCall::OnConnected(connection);
}


void MyCall::OnEstablishedCall()
{
  m_CallState = MyCall::CallEstablished;

  m_manager.DropCDR(*this, false);

  OpalCall::OnEstablishedCall();
}


void MyCall::OnStartMediaPatch(OpalConnection & connection, OpalMediaPatch & patch)
{
  OpalMediaStream & stream = patch.GetSource();

  Media & media = m_media[stream.GetID()];
  media.m_Codec = stream.GetMediaFormat();

  OpalRTPConnection * rtpConn = dynamic_cast<OpalRTPConnection *>(&connection);
  if (rtpConn == NULL)
    return;

  OpalMediaSession * session = rtpConn->GetMediaSession(stream.GetSessionID());
  if (session == NULL)
    return;

  session->GetRemoteAddress().GetIpAndPort(&connection == GetConnection(0) ? media.m_OriginatorAddress : media.m_DestinationAddress);
}


void MyCall::OnStopMediaPatch(OpalMediaPatch & patch)
{
  OpalMediaStream & stream = patch.GetSource();

  m_media[stream.GetID()].m_closed = true;

#if OPAL_STATISTICS
  OpalMediaStatistics stats;
  stream.GetStatistics(stats);
  int duration = (PTime() - stats.m_startTime).GetSeconds();
  if (duration > 0)
    m_Bandwidth += stats.m_totalBytes * 8 / duration;
#endif
}


void MyCall::OnCleared()
{
  m_CallState = MyCall::CallCompleted;
  m_CallResult = GetCallEndReason();
  m_EndTime.SetCurrentTime();

  m_manager.DropCDR(*this, true);

  OpalCall::OnCleared();
}


///////////////////////////////////////////////////////////////////////////////

CallDetailRecord::CallDetailRecord()
: m_ConnectTime(0)
, m_EndTime(0)
, m_CallState(CallSetUp)
, m_Bandwidth(0)
{
}


void CallDetailRecord::OutputText(ostream & strm, const PString & format) const
{
  PINDEX percent = format.Find('%');
  PINDEX last = 0;
  while (percent != P_MAX_INDEX) {
    if (percent > last)
      strm << format(last, percent - 1);

    bool quoted = percent > 0 && format[percent - 1] == '"';

    last = percent + 1;
    percent = format.Find('%', last);
    if (percent == P_MAX_INDEX || percent == last)
      return;

    PCaselessString fieldName = format(last, percent - 1);
    for (FieldCodes f = BeginFieldCodes; f < EndFieldCodes; ++f) {
      if (fieldName == CDRFields[f].m_name) {
        switch (f) {
          case CallId:
            strm << m_GUID;
            break;
          case StartTime:
            strm << m_StartTime.AsString(PTime::LoggingFormat);
            break;
          case ConnectTime:
            if (m_ConnectTime.IsValid())
              strm << m_ConnectTime.AsString(PTime::LoggingFormat);
            break;
          case EndTime:
            strm << m_EndTime.AsString(PTime::LoggingFormat);
            break;
          case CallState:
            if (m_CallState != CallCompleted)
              strm << -m_CallState;
            else
              strm << m_CallResult.AsInteger();
            break;
          case CallResult:
            if (m_CallState == CallCompleted)
              strm << OpalConnection::GetCallEndReasonText(m_CallResult);
            break;
          case OriginatorID:
            strm << m_OriginatorID;
            break;
          case OriginatorURI:
            if (quoted) {
              PString str = m_OriginatorURI;
              str.Replace("\"", "\"\"", true);
              strm << str;
            }
            else
              strm << m_OriginatorURI;
            break;
          case OriginatorSignalAddress:
            strm << m_OriginatorSignalAddress;
            break;
          case DialedNumber:
            strm << m_DialedNumber;
            break;
          case DestinationID:
            strm << m_DestinationID;
            break;
          case DestinationURI:
            if (quoted) {
              PString str = m_DestinationURI;
              str.Replace("\"", "\"\"", true);
              strm << str;
            }
            else
              strm << m_DestinationURI;
            break;
          case DestinationSignalAddress:
            if (m_DestinationSignalAddress.IsValid())
              strm << m_DestinationSignalAddress;
            break;
          case AudioCodec:
            strm << GetAudio().m_Codec;
            break;
          case AudioOriginatorMediaAddress:
            strm << GetAudio().m_OriginatorAddress;
            break;
          case AudioDestinationMediaAddress:
            strm << GetAudio().m_DestinationAddress;
            break;
#if OPAL_VIDEO
          case VideoCodec:
            strm << GetVideo().m_Codec;
            break;
          case VideoOriginatorMediaAddress:
            strm << GetVideo().m_OriginatorAddress;
            break;
          case VideoDestinationMediaAddress:
            strm << GetVideo().m_DestinationAddress;
            break;
#endif // OPAL_VIDEO
          case Bandwidth:
            if (m_Bandwidth > 0)
              strm << (m_Bandwidth + 999) / 1000;
            break;
          default:
            break;
        }
      }
    }

    last = percent + 1;
    percent = format.Find('%', last);
  }
  strm << format.Mid(last) << endl;
}


#if P_ODBC
void CallDetailRecord::OutputSQL(PODBC::Row & row, PString const map[NumFieldCodes]) const
{
  for (FieldCodes f = BeginFieldCodes; f < EndFieldCodes; ++f) {
    PINDEX column = row.ColumnByName(map[f]);
    if (column > 0) {
      PODBC::Field & field = row[column];
      switch (f) {
        case CallId:
          field = m_GUID;
          break;
        case StartTime:
          if (m_StartTime.IsValid())
            field = m_StartTime;
          break;
        case ConnectTime:
          if (m_ConnectTime.IsValid())
            field = m_ConnectTime;
          break;
        case EndTime:
          if (m_EndTime.IsValid())
            field = m_EndTime;
          break;
        case CallState:
          if (m_CallState != CallCompleted)
            field = -m_CallState;
          else
            field = m_CallResult.AsInteger();
          break;
        case CallResult:
          if (m_CallState == CallCompleted)
            field = OpalConnection::GetCallEndReasonText(m_CallResult);
          break;
        case OriginatorID:
          if (!m_OriginatorID.IsEmpty())
            field = m_OriginatorID;
          break;
        case OriginatorURI:
          if (!m_OriginatorURI.IsEmpty())
            field = m_OriginatorURI;
          break;
        case OriginatorSignalAddress:
          if (m_OriginatorSignalAddress.IsValid())
            field = m_OriginatorSignalAddress.AsString();
          break;
        case DialedNumber:
          if (!m_DialedNumber.IsEmpty())
            field = m_DialedNumber;
          break;
        case DestinationID:
          if (!m_DestinationID.IsEmpty())
            field = m_DestinationID;
          break;
        case DestinationURI:
          if (!m_DestinationURI.IsEmpty())
            field = m_DestinationURI;
          break;
        case DestinationSignalAddress:
          if (m_DestinationSignalAddress.IsValid())
            field = m_DestinationSignalAddress.AsString();
          break;
        case AudioCodec:
          if (!GetAudio().m_Codec.GetName().IsEmpty())
            field = GetAudio().m_Codec.GetName();
          break;
        case AudioOriginatorMediaAddress:
          if (GetAudio().m_OriginatorAddress.IsValid())
            field = GetAudio().m_OriginatorAddress.AsString();
          break;
        case AudioDestinationMediaAddress:
          if (GetAudio().m_DestinationAddress.IsValid())
            field = GetAudio().m_DestinationAddress.AsString();
          break;
#if OPAL_VIDEO
        case VideoCodec:
          if (!GetVideo().m_Codec.GetName().IsEmpty())
            field = GetVideo().m_Codec.GetName();
          break;
        case VideoOriginatorMediaAddress:
          if (GetVideo().m_OriginatorAddress.IsValid())
            field = GetVideo().m_OriginatorAddress.AsString();
          break;
        case VideoDestinationMediaAddress:
          if (GetVideo().m_DestinationAddress.IsValid())
            field = GetVideo().m_DestinationAddress.AsString();
          break;
#endif //OPAL_VIDEO
        case Bandwidth:
          if (m_Bandwidth > 0)
            field = (m_Bandwidth + 999) / 1000;
          break;
        default:
          break;
      }
    }
  }
}
#endif // P_ODBC


MyCall::Media CallDetailRecord::GetMedia(const OpalMediaType & mediaType) const
{
  for (MediaMap::const_iterator it = m_media.begin(); it != m_media.end(); ++it) {
    if (it->second.m_Codec.GetMediaType() == mediaType)
      return it->second;
  }

  return Media();
}


PString CallDetailRecord::ListMedia() const
{
  PStringStream strm;

  for (MediaMap::const_iterator it = m_media.begin(); it != m_media.end(); ++it) {
    if (!strm.IsEmpty())
      strm << ", ";
    if (it->second.m_closed)
      strm << '(' << it->second.m_Codec << ')';
    else
      strm << it->second.m_Codec;
  }

  if (strm.IsEmpty())
    strm << "None";

  return strm;
}


PString CallDetailRecord::GetCallState() const
{
  static const char * const CallStates[] = {
    "Completed", "Setting Up", "Proceeding", "Alerting", "Connected", "Established"
  };

  if (m_CallState < PARRAYSIZE(CallStates))
    return CallStates[m_CallState];

  return psprintf("CallState=%u", m_CallState);
}


///////////////////////////////////////////////////////////////////////////////

CDRListPage::CDRListPage(MyManager & mgr, const PHTTPAuthority & auth)
  : BaseStatusPage(mgr, auth, "CallDetailRecords")
{
}


const char * CDRListPage::GetTitle() const
{
  return "OPAL Server Call Detail Records";
}


void CDRListPage::CreateContent(PHTML & html, const PStringToString &) const
{
  html << PHTML::TableStart(PHTML::Border1, PHTML::CellPad4)
       << PHTML::TableRow()
       << PHTML::TableHeader() << "Call" << PHTML::NonBreakSpace() << "Identifier"
       << PHTML::TableHeader() << "Originator"
       << PHTML::TableHeader() << "Destination"
       << PHTML::TableHeader() << "Start" << PHTML::NonBreakSpace() << "Time"
       << PHTML::TableHeader() << "End" << PHTML::NonBreakSpace() << "Time"
       << PHTML::TableHeader() << "Call" << PHTML::NonBreakSpace() << "Result";

  for (MyManager::CDRList::const_iterator it = m_manager.BeginCDR(); m_manager.NotEndCDR(it); ++it)
    it->OutputSummaryHTML(html);

  html << PHTML::TableEnd();
}


void CallDetailRecord::OutputSummaryHTML(PHTML & html) const
{
  html << PHTML::TableRow()
       << PHTML::TableData() << PHTML::HotLink("CallDetailRecord?guid=" + m_GUID.AsString()) << m_GUID << PHTML::HotLink()
       << PHTML::TableData() << PHTML::Escape(m_OriginatorURI)
       << PHTML::TableData() << PHTML::Escape(m_DestinationURI)
       << PHTML::TableData() << m_StartTime.AsString(PTime::LoggingFormat)
       << PHTML::TableData() << m_EndTime.AsString(PTime::LoggingFormat)
       << PHTML::TableData();
  if (m_CallState != CallCompleted)
    html << "Incomplete";
  else
    html << OpalConnection::GetCallEndReasonText(m_CallResult);
}


///////////////////////////////////////////////////////////////////////////////

CDRPage::CDRPage(MyManager & mgr, const PHTTPAuthority & auth)
  : BaseStatusPage(mgr, auth, "CallDetailRecord")
{
  m_refreshRate = 0;
}


const char * CDRPage::GetTitle() const
{
  return "OPAL Server Call Detail Record";
}


void CDRPage::CreateContent(PHTML & html, const PStringToString & query) const
{
  CallDetailRecord cdr;
  if (m_manager.FindCDR(query("guid"), cdr))
    cdr.OutputDetailedHTML(html);
  else
    html << "No records found.";
}


void CallDetailRecord::OutputDetailedHTML(PHTML & html) const
{
  html << PHTML::TableStart(PHTML::Border1, PHTML::CellPad4);

  for (MyCall::FieldCodes f = MyCall::BeginFieldCodes; f < MyCall::EndFieldCodes; ++f) {
    html << PHTML::TableRow()
         << PHTML::TableHeader() << CDRFields[f].m_name
         << PHTML::TableData();
    switch (f) {
      case CallId:
        html << m_GUID;
        break;
      case StartTime:
        html << m_StartTime.AsString(PTime::LongDateTime);
        break;
      case ConnectTime:
        if (m_ConnectTime.IsValid())
          html << m_ConnectTime.AsString(PTime::LongDateTime);
        break;
      case EndTime:
        html << m_EndTime.AsString(PTime::LongDateTime);
        break;
      case CallState:
        html << (m_CallState != CallCompleted ? "Incomplete" : "Completed");
        break;
      case CallResult:
        if (m_CallState == CallCompleted)
          html << OpalConnection::GetCallEndReasonText(m_CallResult);
        break;
      case OriginatorID:
        html << m_OriginatorID;
        break;
      case OriginatorURI:
        html << m_OriginatorURI;
        break;
      case OriginatorSignalAddress:
        html << m_OriginatorSignalAddress;
        break;
      case DialedNumber:
        html << m_DialedNumber;
        break;
      case DestinationID:
        html << m_DestinationID;
        break;
      case DestinationURI:
        html << m_DestinationURI;
        break;
      case DestinationSignalAddress:
        if (m_DestinationSignalAddress.IsValid())
          html << m_DestinationSignalAddress;
        break;
      case AudioCodec:
        html << GetAudio().m_Codec;
        break;
      case AudioOriginatorMediaAddress:
        html << GetAudio().m_OriginatorAddress;
        break;
      case AudioDestinationMediaAddress:
        html << GetAudio().m_DestinationAddress;
        break;
#if OPAL_VIDEO
      case VideoCodec:
        html << GetVideo().m_Codec;
        break;
      case VideoOriginatorMediaAddress:
        html << GetVideo().m_OriginatorAddress;
        break;
      case VideoDestinationMediaAddress:
        html << GetVideo().m_DestinationAddress;
        break;
#endif //OPAL_VIDEO
      case Bandwidth:
        if (m_Bandwidth > 0)
          html << (m_Bandwidth + 999) / 1000;
        break;
      default:
        break;
    }
  }

  html << PHTML::TableEnd();
}


// End of File ///////////////////////////////////////////////////////////////
