/*
 * mcu_type.h
 *
 *  Created on: 2023年8月10日
 *      Author: Lenovo
 */

#ifndef SRC_COMMON_INCLUDE_MCU_TYPE_H_
#define SRC_COMMON_INCLUDE_MCU_TYPE_H_
#include <stdint.h>

typedef signed long long xint64_t;

typedef unsigned long long xuint64_t;

typedef float  xsingle_t;

typedef double xdouble_t;

typedef signed int xint32_t;

typedef unsigned int xuint32_t;

typedef signed short xint16_t;

typedef unsigned short xuint16_t;

typedef signed char xint8_t;

typedef unsigned char xuint8_t;

typedef char	xbyte_t;

typedef void *	xhandle_t;

typedef void *	xpvoid_t;

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#endif /* SRC_COMMON_INCLUDE_MCU_TYPE_H_ */
