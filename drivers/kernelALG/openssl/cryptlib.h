/* cryptlib.h */

#ifndef HEADER_CRYPTLIB_H
#define HEADER_CRYPTLIB_H

//#include <stdlib.h>
//#include <string.h>

//#include "e_os.h"

#include "crypto.h"
#include "opensslconf.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* size of string representations */
#define DECIMAL_SIZE(type)	((sizeof(type)*8+2)/3+1)
#define HEX_SIZE(type)		(sizeof(type)*2)

#ifdef  __cplusplus
}
#endif

#endif
