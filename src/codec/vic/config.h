/************ Change log
 *
 * $Log: config.h,v $
 * Revision 1.2001  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.5  2001/05/16 06:30:29  yurik
 * INT_64 is defined here
 *
 * Revision 1.4  2001/05/10 14:21:34  craigs
 * Added BYTE, as ptlib.h has been removed
 *
 * Revision 1.3  2001/05/10 05:25:44  robertj
 * Removed need for VIC code to use ptlib.
 *
 * Revision 1.2  2000/08/25 03:18:49  dereks
 * Add change log facility (Thanks Robert for the info on implementation)
 *
 *
 *
 ********/

#if defined(_WIN32) && !defined(WIN32)
#define WIN32
#endif

#include <iostream.h>
#include <memory.h>

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int  u_int;
typedef unsigned char BYTE;

#ifdef _WIN32_WCE
#define INT_64 __int64
#endif

