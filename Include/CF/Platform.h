/*
 * file:        Platform.h
 * description: compile flags.
 */

#ifndef __CF_PLATFORM_FLAGS_H__
#define __CF_PLATFORM_FLAGS_H__

#define PLATFORM_SERVER_BIN_NAME    "CxxFramework"
#define PLATFORM_SERVER_TEXT_NAME   "CxxFramework"

#define DEBUG 0
#define ASSERT 1

#define __MinGW__ 1

#define USE_ENUM 0

#define MMAP_TABLES 0
#define USE_ATOMICLIB 0
#define MACOSXEVENTQUEUE 0
#define __PTHREADS__ 1
#define __PTHREADS_MUTEXES__ 1
#define ALLOW_NON_WORD_ALIGN_ACCESS 1
#define USE_THREAD 0
#define THREADING_IS_COOPERATIVE 0
#define USE_THR_YIELD 0
#define MACOSX_PUBLICBETA 0
#define __WinSock__ 1

/* #undef USE_DEFAULT_STD_LIB */
#if defined(USE_DEFAULT_STD_LIB) && !USE_DEFAULT_STD_LIB
#define USE_DEFAULT_STD_LIB 1
#endif

// Platform-specific switches
#if __MacOSX__
#define kPlatformNameString     "MacOSX"
#define EXPORT

#include <machine/endian.h>
#include <machine/limits.h>
#if BYTE_ORDER == BIG_ENDIAN
#define BIGENDIAN      1
#else
#define BIGENDIAN      0
#endif

#ifdef __LP64__
#define EVENTS_KQUEUE 0  // future
#define EVENTS_SELECT 0 // future
#define EVENTS_OSXEVENTQUEUE 0 // future
#define SET_SELECT_SIZE 1024
#else
#define EVENTS_KQUEUE 0
#define EVENTS_SELECT 0
#define EVENTS_OSXEVENTQUEUE 1
#define SET_SELECT_SIZE 0
#endif

#elif __Win32__
#define kPlatformNameString     "Win32"
#define EXPORT  __declspec(dllexport)

#elif __MinGW__
#define kPlatformNameString     "MinGW"
#define EXPORT

#elif __linux__
#define kPlatformNameString     "Linux"
#define EXPORT

#include <endian.h>
#if __BYTE_ORDER == BIG_ENDIAN
#define BIGENDIAN      1
#else
#define BIGENDIAN      0
#endif

#elif __linuxppc__
#define kPlatformNameString     "LinuxPPC"
#define EXPORT

#include <endian.h>
#if __BYTE_ORDER == BIG_ENDIAN
#define BIGENDIAN      1
#else
#define BIGENDIAN      0
#endif

#elif __FreeBSD__
#define kPlatformNameString     "FreeBSD"
#define EXPORT

#include <machine/endian.h>
#if BYTE_ORDER == BIG_ENDIAN
#define BIGENDIAN      1
#else
#define BIGENDIAN      0
#endif

#elif __solaris__
#define kPlatformNameString     "Solaris"
#define EXPORT

#ifdef sparc
#define BIGENDIAN 1
#endif
#ifdef _M_IX86
#define BIGENDIAN 0
#endif
#ifdef _M_ALPHA
#define BIGENDIAN 0
#endif
#ifndef BIGENDIAN
#error NEED BIGENDIAN DEFINITION 0 OR 1 FOR PLATFORM
#endif

#elif __sgi__
#define kPlatformNameString     "IRIX"
#define EXPORT
#define BIGENDIAN               1

#elif __hpux__
#define kPlatformNameString     "HP-UX"
#define EXPORT
#define BIGENDIAN               1

#elif defined(__osf__)
#define __osf__ 1
#define kPlatformNameString     "Tru64UNIX"
#define EXPORT
#define BIGENDIAN       0

#endif


#endif // __CF_PLATFORM_FLAGS_H__