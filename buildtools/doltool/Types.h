//
// typedefs for easier porting
//

#ifndef _DOLTOOL_TYPES_H_
#define _DOLTOOL_TYPES_H_

#ifndef __WIN32__
 #ifdef __linux__
  #include <endian.h>
 #endif
 #ifdef __APPLE__
  #include <machine/endian.h>
 #endif
#else
 #define LITTLE_ENDIAN 1234
 #define BIG_ENDIAN    4321
#endif

// set your machines endian format here
#define MY_ENDIAN		LITTLE_ENDIAN

// change endian format
#define CHANGE_ENDIAN_16BIT(num)	(	((num&0xFF00)>> 8)		|	\
										((num&0x00FF)<< 8)		)

#define CHANGE_ENDIAN_32BIT(num)	(	((num&0xFF000000)>>24)	|	\
										((num&0x00FF0000)>> 8)	|	\
										((num&0x0000FF00)<< 8)	|	\
										((num&0x000000FF)<<24)	)

// change the endian format if 'test' is true
#define TEST_CHANGE_ENDIAN_16BIT(test, num)	(u16)((test) ? CHANGE_ENDIAN_16BIT(num) : (num))
#define TEST_CHANGE_ENDIAN_32BIT(test, num)	(u32)((test) ? CHANGE_ENDIAN_32BIT(num) : (num))


// win32
typedef unsigned int	u32;
typedef unsigned short	u16;
typedef unsigned char	u8;

typedef signed int		s32;
typedef signed short	s16;
typedef signed char		s8;


// error return value
// (since -1 is not negative if returning unsigned int)
#define ERROR_RETURN	0xFFFFFFFF


#endif // _TYPES_H_

