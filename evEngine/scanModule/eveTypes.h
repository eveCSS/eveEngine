/*
 * eveTypes.h
 *
 *  Created on: 30.09.2008
 *      Author: eden
 */

#ifndef EVETYPES_H_
#define EVETYPES_H_

#include <epicsTypes.h>

/*
 * types correspond to epicsTypes
 *
 */

typedef enum {
                eveInt8T,
                eveUInt8T,
                eveInt16T,
                eveUInt16T,
                eveEnum16T,
                eveInt32T,
                eveUInt32T,
                eveFloat32T,
                eveFloat64T,
                eveStringT,
                eveDateTimeT,
                eveUnknownT
}eveType;

// eclipse indigo parser needs this
#ifndef NULL
	#define NULL 0
#endif


#define eveINT eveInt32T
#define eveDOUBLE eveFloat64T
#define eveSTRING eveStringT

#endif /* EVETYPES_H_ */
