/*
 * capi_win32.h
 *
 * ISDN via CAPI Functions
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 2010 Vox Lucida Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <tchar.h>

// All thes defines must be compatible with the Linux /usr/include/linux/isdn/capicmd.h file

#define CapiNoError                       0x0000

#define CapiToManyAppls                   0x1001
#define CapiLogBlkSizeToSmall             0x1002
#define CapiBuffExeceeds64k               0x1003
#define CapiMsgBufSizeToSmall             0x1004
#define CapiAnzLogConnNotSupported        0x1005
#define CapiRegReserved                   0x1006
#define CapiRegBusy                       0x1007
#define CapiRegOSResourceErr              0x1008
#define CapiRegNotInstalled               0x1009
#define CapiRegCtrlerNotSupportExtEquip   0x100a
#define CapiRegCtrlerOnlySupportExtEquip  0x100b

#define CapiIllAppNr                      0x1101
#define CapiIllCmdOrSubcmdOrMsgToSmall		0x1102
#define CapiSendQueueFull                 0x1103
#define CapiReceiveQueueEmpty             0x1104
#define CapiReceiveOverflow               0x1105
#define CapiUnknownNotPar                 0x1106
#define CapiMsgBusy                       0x1107
#define CapiMsgOSResourceErr              0x1108
#define CapiMsgNotInstalled               0x1109
#define CapiMsgCtrlerNotSupportExtEquip   0x110a
#define CapiMsgCtrlerOnlySupportExtEquip  0x110b


#define CAPI_ALERT                  0x01
#define CAPI_CONNECT                0x02
#define CAPI_CONNECT_ACTIVE         0x03
#define CAPI_CONNECT_B3_ACTIVE      0x83
#define CAPI_CONNECT_B3             0x82
#define CAPI_CONNECT_B3_T90_ACTIVE  0x88
#define CAPI_DATA_B3                0x86
#define CAPI_DISCONNECT_B3          0x84
#define CAPI_DISCONNECT             0x04
#define CAPI_FACILITY               0x80
#define CAPI_INFO                   0x08
#define CAPI_LISTEN                 0x05
#define CAPI_MANUFACTURER           0xff
#define CAPI_RESET_B3               0x87
#define CAPI_SELECT_B_PROTOCOL      0x41

#define CAPI_REQ    0x80
#define CAPI_CONF   0x81
#define CAPI_IND    0x82
#define CAPI_RESP   0x83

#define CAPICMD(cmd,subcmd)     (((cmd)<<8)|(subcmd))


class OpalCapiFunctions
{
private:
  HMODULE m_hDLL;

  DWORD (WINAPI *m_REGISTER)(DWORD MessageBufferSize,
                             DWORD MaxLogicalConnection,
                             DWORD MaxBDataBlocks,
                             DWORD MaxBDataLen,
                             DWORD *pApplID);
  DWORD (WINAPI *m_RELEASE)(DWORD ApplID);
  DWORD (WINAPI *m_PUT_MESSAGE)(DWORD ApplID, PVOID pCAPIMessage);
  DWORD (WINAPI *m_GET_MESSAGE)(DWORD ApplID, PVOID *ppCAPIMessage);
  DWORD (WINAPI *m_WAIT_FOR_SIGNAL)(DWORD ApplID);
  DWORD (WINAPI *m_GET_MANUFACTURER)(char *szBuffer);
  DWORD (WINAPI *m_GET_VERSION)(DWORD *pCAPIMajor,
                                DWORD *pCAPIMinor,
                                DWORD *pManufacturerMajor,
                                DWORD *pManufacturerMinor);
  DWORD (WINAPI *m_GET_SERIAL_NUMBER)(char *szBuffer);
  DWORD (WINAPI *m_GET_PROFILE)(PVOID pBuffer, DWORD CtrlNr);
  DWORD (WINAPI *m_INSTALLED)();

  bool CheckFn(WORD ordinal, FARPROC & proc)
  {
    return m_hDLL != NULL && (proc || (proc = GetProcAddress(m_hDLL, (LPCTSTR)ordinal)) != NULL);
  }

public:
  typedef DWORD Result;
  typedef DWORD ApplID;
  typedef DWORD UInt;

  OpalCapiFunctions()
  {
    memset(this, 0, sizeof(*this));
    m_hDLL = LoadLibrary(_T("CAPI2032.DLL"));
  }

  ~OpalCapiFunctions()
  {
    if (m_hDLL != NULL)
      FreeLibrary(m_hDLL);
  }

  DWORD REGISTER(DWORD MsgBufferSize, DWORD MaxLogicalConn, DWORD MaxBDataBlocks, DWORD MaxBDataLen, DWORD *pApplID)
  {
    return CheckFn(1, (FARPROC &)m_REGISTER) ? m_REGISTER(MsgBufferSize, MaxLogicalConn, MaxBDataBlocks, MaxBDataLen, pApplID) : 0x10000;
  }

  DWORD RELEASE(DWORD ApplID)
  {
    return CheckFn(2, (FARPROC &)m_RELEASE) ? m_RELEASE(ApplID) : 0x10000;
  }

  DWORD PUT_MESSAGE(DWORD ApplID, PVOID pCAPIMessage)
  {
    return CheckFn(3, (FARPROC &)m_PUT_MESSAGE) ? m_PUT_MESSAGE(ApplID, pCAPIMessage) : 0x10000;
  }

  DWORD GET_MESSAGE(DWORD ApplID, PVOID *ppCAPIMessage)
  {
    return CheckFn(4, (FARPROC &)m_GET_MESSAGE) ? m_GET_MESSAGE(ApplID, ppCAPIMessage) : 0x10000;
  }

  DWORD WAIT_FOR_SIGNAL(DWORD ApplID)
  {
    return CheckFn(5, (FARPROC &)m_WAIT_FOR_SIGNAL) ? m_WAIT_FOR_SIGNAL(ApplID) : 0x10000;
  }

  DWORD GET_MANUFACTURER(char *szBuffer)
  {
    return CheckFn(6, (FARPROC &)m_GET_MANUFACTURER) ? m_GET_MANUFACTURER(szBuffer) : 0x10000;
  }

  DWORD GET_VERSION(DWORD *pCAPIMajor, DWORD *pCAPIMinor, DWORD *pManufacturerMajor, DWORD *pManufacturerMinor)
  {
    return CheckFn(7, (FARPROC &)m_GET_VERSION) ? m_GET_VERSION(pCAPIMajor, pCAPIMinor, pManufacturerMajor, pManufacturerMinor) : 0x10000;
  }

  DWORD GET_SERIAL_NUMBER(char *szBuffer)
  {
    return CheckFn(8, (FARPROC &)m_GET_SERIAL_NUMBER) ? m_GET_SERIAL_NUMBER(szBuffer) : 0x10000;
  }

  DWORD GET_PROFILE(PVOID pBuffer, DWORD CtrlNr)
  {
    return CheckFn(9, (FARPROC &)m_GET_PROFILE) ? m_GET_PROFILE(pBuffer, CtrlNr) : 0x10000;
  }

  DWORD INSTALLED()
  {
    return CheckFn(10, (FARPROC &)m_INSTALLED) ? m_INSTALLED() : 0x10000;
  }
};


// End of File ///////////////////////////////////////////////////////////////
