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

%Module(name=pyvoip.ptlib, keyword_arguments="All")

%Include std/vector.sip
%Include std/list.sip
%Include std/string.sip
%Include std/set.sip

%Import QtCore/qtcoremod.sip


typedef unsigned short      WORD;
typedef bool				PBoolean;
typedef int					PINDEX;
typedef unsigned char       BYTE;
typedef long long			PInt64;       
typedef unsigned long long  PUInt64;
typedef PInt64              time_t; 
typedef char *				va_list;
typedef unsigned long 		size_t;
typedef long				off_t;
typedef int					INT;
typedef unsigned long		HANDLE;
typedef DWORD				PThreadIdentifier;


%Include PObject.sip
%Include PMutex.sip
%Include PProcess.sip
%Include PThread.sip
%Include PArgList.sip
%Include PQoS.sip
%Include PString.sip
%Include PIPSocket.sip
%Include PContainer.sip
%Include PCollection.sip
%Include PSafeColl.sip
%Include PArray.sip
%Include PList.sip
%Include PSocket.sip
%Include PSmartPointer.sip
%Include PNotifier.sip
%Include PChannel.sip
%Include PIndirectChannel.sip
%Include PTime.sip
%Include PTimer.sip
%Include PTimeInterval.sip
%Include PSync.sip
%Include PCriticalSection.sip 
%Include PSemaphore.sip
%Include PSyncPoint.sip
%Include PNatMethod.sip
%Include PUDPSocket.sip
%Include PTrace.sip
%Include PDirectory.sip
%Include PFilePath.sip
%Include PFile.sip
%Include PluginMgr.sip
%Include PURL.sip
%Include PVideoInputDevice.sip
%Include PTCPSocket.sip
%Include PSound.sip
%Include PSoundChannelWin32.sip
%Include PHTTP.sip
%Include PInternetProtocol.sip
%Include PMimeInfo.sip
%Include PHTML.sip
%Include PStunClient.sip
%Include PGloballyUniqueID.sip
%Include PDict.sip