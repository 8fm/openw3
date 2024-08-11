/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_IO_PLATFORM_H
#define RED_IO_PLATFORM_H
#pragma once

#include "../redSystem/settings.h"
#include "../redSystem/os.h"
#include "../redSystem/crt.h"
#include "../redSystem/error.h"

#include "../redSystem/types.h"
#include "../redSystem/utility.h"
#include "../redSystem/log.h"

#define REDIO_NOCOPY_CLASS(cls)	\
	private:					\
	cls(const cls&);			\
	void operator=(const cls&);	\

#define REDIO_NOCOPY_STRUCT(cls) \
	cls(const cls&);			 \
	void operator=(const cls&);	 \

#define REDIO_LOG( format, ... )		RED_LOG( RedIO, format, ## __VA_ARGS__ )
#define REDIO_WARN( format, ... )		RED_LOG_WARNING( RedIO, format, ## __VA_ARGS__ )
#define REDIO_ERR( format, ... )		RED_LOG_ERROR( RedIO, format, ## __VA_ARGS__ )

#define REDIO_NAMESPACE_BEGIN			namespace Red { namespace IO {
#define REDIO_NAMESPACE_END				} }

#define REDIO_WINAPI_NAMESPACE_BEGIN		REDIO_NAMESPACE_BEGIN namespace WinAPI {
#define REDIO_WINAPI_NAMESPACE_END			REDIO_NAMESPACE_END }
#define REDIO_ORBISAPI_NAMESPACE_BEGIN		REDIO_NAMESPACE_BEGIN namespace OrbisAPI {
#define REDIO_ORBISAPI_NAMESPACE_END		REDIO_NAMESPACE_END }
#define REDIO_GENERICAPI_NAMESPACE_BEGIN	REDIO_NAMESPACE_BEGIN namespace GenericAPI {
#define REDIO_GENERICAPI_NAMESPACE_END		REDIO_NAMESPACE_END }
#define REDIO_UTILS_NAMESPACE_BEGIN			REDIO_NAMESPACE_BEGIN namespace Utils {
#define REDIO_UTILS_NAMESPACE_END			REDIO_NAMESPACE_END }

REDIO_NAMESPACE_BEGIN
#include "redIOTypes.h"
REDIO_NAMESPACE_END

#ifndef RED_UNIT_TEST
# define REDIO_ERR_TXT	TXT
# define REDIO_ASSERT	RED_ASSERT
# define REDIO_VERIFY	RED_VERIFY
# define REDIO_HALT		RED_HALT
#else
# define REDIO_ERR_TXT( txt ) txt
# define REDIO_ASSERT( expression, ... ) do{ if ( !(expression) ){ /*abort();*/ } }while(false)
# define REDIO_VERIFY( expression, ... ) do{ if (!(expression)){ /*abort();*/ } }while(false)
# define REDIO_HALT( expression, ... ) do{ /*abort();*/ }while(false)
#endif

# define REDIO_MACRO_ERR_TXT( txt ) REDIO_ERR_TXT( txt )

//////////////////////////////////////////////////////////////////////////
// OS specific error check wrappers
#if defined( RED_PLATFORM_ORBIS )
#	ifdef RED_ASSERTS_ENABLED
#		define REDIO_SCE_ERR( sceErr ) do{ REDIO_ASSERT( sceErr >= SCE_OK, TXT("Error value: 0x%08X"), sceErr ); }while(false)
#		define REDIO_FIOS_CHECK( expression ) do{ int sceret_ = (expression); (void)sceret_; REDIO_ASSERT( sceret_ >= SCE_FIOS_OK, REDIO_MACRO_ERR_TXT( #expression ), TXT("\nReturn value: 0x%08X"), sceret_ ); }while(false)
#		define REDIO_SCE_CHECK( expression ) do{ int sceret_ = (expression); (void)sceret_; REDIO_ASSERT( sceret_ == SCE_OK, REDIO_MACRO_ERR_TXT( #expression ), TXT("\nReturn value: 0x%08X"), sceret_ ); }while(false)
#	else
#		define REDIO_FIOS_CHECK( expression ) REDIO_VERIFY( (expression) )
#		define REDIO_SCE_CHECK( expression ) REDIO_VERIFY( (expression) )
#		define REDIO_SCE_ERR( sceErr ) REDIO_HALT( (sceErr) )
#	endif
#else
#	define REDIO_FIOS_CHECK( expression ) (void)(expression)
#	define REDIO_SCE_CHECK( expression ) (void)(expression)
#endif

#ifdef RED_COMPILER_MSC
#pragma warning( disable : 4127 ) // conditional expression is constant: from do{}while(false)
#endif // RED_COMPILER_MSC

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
#	ifdef RED_ASSERTS_ENABLED
// Speculatively GetLastError() in case assert checking changes it. Compare with != 0 so can check pointers, numbers, or bools without "performance" warnings.
// Technically relies on "expression" not doing some conversion that could clobber last error...
#		define REDIO_WIN_CHECK( expression ) do{ Red::IO::Bool winret_ = ( (expression) != 0 ); DWORD lastError_ = ::GetLastError(); (void)lastError_; REDIO_ASSERT( winret_, REDIO_MACRO_ERR_TXT( #expression ), REDIO_ERR_TXT("\nGetLastError() result: 0x%08X"), lastError_ ); }while(false)
#	else
#		define REDIO_WIN_CHECK( expression ) REDIO_VERIFY( (expression) )
#	endif
#else
#	define REDIO_WIN_CHECK( expression ) (void)(expression)
#endif

#define NO_STRUCT_DEFAULT_OPERATOR_NEW					\
	RED_INLINE void* operator new( size_t size );		\
	RED_INLINE void  operator delete( void* ptr );		\
	RED_INLINE void* operator new[]( size_t size );		\
	RED_INLINE void  operator delete[]( void* ptr );

#define NO_CLASS_DEFAULT_OPERATOR_NEW					\
	private:											\
	NO_STRUCT_DEFAULT_OPERATOR_NEW						\
	public:

//////////////////////////////////////////////////////////////////////////
// OS Namespaces for switching between platform-specific implementations

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
REDIO_WINAPI_NAMESPACE_BEGIN
REDIO_WINAPI_NAMESPACE_END
REDIO_NAMESPACE_BEGIN
namespace OSAPI = WinAPI;
REDIO_NAMESPACE_END
#elif defined( RED_PLATFORM_ORBIS )
REDIO_ORBISAPI_NAMESPACE_BEGIN
REDIO_ORBISAPI_NAMESPACE_END
REDIO_NAMESPACE_BEGIN
namespace OSAPI = OrbisAPI;
REDIO_NAMESPACE_END
#endif
	
#endif // RED_IO_PLATFORM_H
