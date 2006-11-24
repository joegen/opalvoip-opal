/*
 * capi2032.h
 */

# ifndef __CAPI20_H
# define __CAPI20_H

# ifdef __cplusplus
extern "C" {
# endif

#include <windows.h>

DWORD APIENTRY CAPI_REGISTER(DWORD MessageBufferSize,
                             DWORD MaxLogicalConnection,
                             DWORD MaxBDataBlocks,
                             DWORD MaxBDataLen,
                             DWORD *pApplID);

DWORD APIENTRY CAPI_RELEASE(DWORD ApplID);

DWORD APIENTRY CAPI_PUT_MESSAGE(DWORD ApplID, PVOID pCAPIMessage);

DWORD APIENTRY CAPI_GET_MESSAGE(DWORD ApplID, PVOID *ppCAPIMessage);

DWORD APIENTRY CAPI_WAIT_FOR_SIGNAL(DWORD ApplID);

VOID  APIENTRY CAPI_GET_MANUFACTURER(CHAR *szBuffer);

DWORD APIENTRY CAPI_GET_VERSION(DWORD *pCAPIMajor,
                                DWORD *pCAPIMinor,
                                DWORD *pManufacturerMajor,
                                DWORD *pManufacturerMinor);

DWORD APIENTRY CAPI_GET_SERIAL_NUMBER(CHAR *szBuffer);

DWORD APIENTRY CAPI_GET_PROFILE(PVOID pBuffer, DWORD CtrlNr);

DWORD APIENTRY CAPI_INSTALLED(VOID);

DWORD APIENTRY CAPI_MANUFACTURER(DWORD Function, DWORD Qualifier,
				 CHAR *InData,   DWORD sizeInData,
			  CHAR *OutData,  DWORD sizeOutData);


#ifdef __cplusplus
}
#endif

#ifndef __NO_CAPIUTILS__
#include "capiutils.h"
#endif

#endif /* __CAPI20_H */

