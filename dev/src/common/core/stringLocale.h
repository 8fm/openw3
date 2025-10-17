/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#if defined( RED_PLATFORM_ORBIS ) || defined( RED_PLATFORM_LINUX )
#	define DIRECTORY_SEPARATOR_CHAR					'/'
#	define ALTERNATIVE_DIRECTORY_SEPARATOR_CHAR		'\\'
#else
#	define DIRECTORY_SEPARATOR_CHAR					'\\'
#	define ALTERNATIVE_DIRECTORY_SEPARATOR_CHAR		'/'
#endif

static const int MAX_STATIC_CONV_LEN = 256;

RED_WARNING_PUSH()
RED_DISABLE_WARNING_MSC( 4996 )

class CAnsiToUnicode
{
	UniChar		m_staticBuf[MAX_STATIC_CONV_LEN];
	UniChar*	m_buf;

public:
	CAnsiToUnicode(const AnsiChar* src)
		: m_buf(0)
	{
		convert(src);
	}

	~CAnsiToUnicode()
	{
		if (m_buf && m_buf != m_staticBuf)
		{
			RED_MEMORY_FREE( MemoryPool_Strings, MC_String, m_buf );
		}
	}

	void convert(const AnsiChar* src)
	{
		if ( src == NULL )
		{
			m_buf = m_staticBuf;
			m_buf[ 0 ] = 0;
			return;
		}

		const size_t len = Red::System::StringLength(src) + 1;

		if (len > (size_t) MAX_STATIC_CONV_LEN)
		{
			m_buf = reinterpret_cast<UniChar*>( RED_MEMORY_ALLOCATE( MemoryPool_Strings, MC_String, sizeof(UniChar) * len) );
		}
		else 
		{
			m_buf = m_staticBuf;
		}

		//::MultiByteToWideChar(CP_ACP, 0, src, len, m_buf, len);

		//FIXME> Use redSystem...
#if defined( RED_COMPILER_MSC )
		mbstate_t mbst = { 0 };
#elif defined( RED_COMPILER_CLANG )
		mbstate_t mbst; // has a ctor for C++
#else
#error Unsupported platform
#endif
#ifdef RED_PLATFORM_LINUX
		mbsrtowcs( m_buf, &src, len - 1, nullptr );
		// if no null character was written to dst after len wide characters were written, then
		// L'\0' is stored in dst[len], which means len+1 total wide characters are written
		m_buf[ len - 1 ] = L'\0';
#else
		mbsrtowcs( m_buf, &src, len, &mbst );
#endif
	}

	operator UniChar* () const
	{
		return m_buf;
	}
};

class CUnicodeToAnsi
{
	AnsiChar	m_staticBuf[MAX_STATIC_CONV_LEN];
	AnsiChar*	m_buf;

public:
	CUnicodeToAnsi(const UniChar* src, Bool changeSlash = false)
		: m_buf(0)
	{
		convert(src, changeSlash);
	}

	~CUnicodeToAnsi()
	{
		if (m_buf && m_buf != m_staticBuf)
		{
			RED_MEMORY_FREE( MemoryPool_Strings, MC_String, m_buf );
		}
	}

	void convert( const UniChar* src, Bool changeSlash )
	{
		if ( src == NULL )
		{
			m_buf = m_staticBuf;
			m_buf[ 0 ] = 0;
			return;
		}

		const size_t len = wcsrtombs( NULL, &src, 0, NULL ) + 1;

		if (len > (size_t) MAX_STATIC_CONV_LEN)
		{
			m_buf = reinterpret_cast<AnsiChar*>( RED_MEMORY_ALLOCATE( MemoryPool_Strings, MC_String, sizeof(AnsiChar) * len) );
		}
		else 
		{
			m_buf = m_staticBuf;
		}

		wcsrtombs( m_buf, &src, len, NULL );

		if ( changeSlash )
		{
			AnsiChar* temp = m_buf;
			while (*temp != '\0')
			{
				if ( *temp == ALTERNATIVE_DIRECTORY_SEPARATOR_CHAR )
				{
					*temp = DIRECTORY_SEPARATOR_CHAR;
				}
				temp++;
			}
		}
	}

	operator AnsiChar* () const
	{
		return m_buf;
	}
};

#define ANSI_TO_UNICODE(str)	(UniChar*)CAnsiToUnicode((const AnsiChar*)(str))
#define UNICODE_TO_ANSI(str)	(AnsiChar*)CUnicodeToAnsi((const UniChar*)(str))

#define UNICODE_TO_ANSIPATH(str)	(AnsiChar*)CUnicodeToAnsi((const UniChar*)(str), true)

#ifdef RED_PLATFORM_ORBIS
# ifdef UNICODE
#  define TO_FILECHAR(str)	UNICODE_TO_ANSI( str )
# else
#  define TO_FILECHAR(str)	str
# endif
#else
# define TO_FILECHAR(str)	str
#endif

class CLowerToUpper
{
public:
	CLowerToUpper( UniChar* ptr, Uint32 size )
	{
		for ( Uint32 i = 0; i < size; ++i, ++ptr )
		{
			if ( *ptr >= L'a' && *ptr <= L'z' )
			{
				*ptr += static_cast< UniChar >(L'A' - L'a');
			}
		}
	}
	CLowerToUpper( AnsiChar* ptr, Uint32 size )
	{
		for ( Uint32 i = 0; i < size; ++i, ++ptr )
		{
			if ( *ptr >= 'a' && *ptr <= 'z' )
			{
				*ptr += static_cast< AnsiChar >('A' - 'a');
			}
		}
	}
};

class CUpperToLower
{
public:
	CUpperToLower( UniChar* ptr, Uint32 size )
	{
		for ( Uint32 i = 0; i < size; ++i, ++ptr )
		{
			if ( *ptr >= L'A' && *ptr <= L'Z' )
			{
				*ptr += static_cast< UniChar >(L'a' - L'A');
			}
		}
	}
	CUpperToLower( AnsiChar* ptr, Uint32 size )
	{
		for ( Uint32 i = 0; i < size; ++i, ++ptr )
		{
			if ( *ptr >= 'A' && *ptr <= 'Z' )
			{
				*ptr += static_cast< AnsiChar >('a' - 'A');
			}
		}
	}
};

RED_INLINE Bool IsUpper( Char c )
{
	return c >= 'A' && c <= 'Z';
}

RED_INLINE Bool IsLower( Char c )
{
	return c >= 'a' && c <= 'z';
}

RED_INLINE Bool IsNumber( Char c )
{
	return c >= '0' && c <= '9';
}

RED_INLINE Bool IsWhiteSpace( AnsiChar c )
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

RED_INLINE Bool IsWhiteSpace( UniChar c )
{
	return c == L' ' || c == L'\t' || c == L'\r' || c == L'\n';
}


////////////////////////////////////////////////////

class CLocalTextBufferManager
{
private:
	struct LocalBuffer
	{
		DECLARE_STRUCT_MEMORY_POOL( MemoryPool_Strings, MC_String );
		Char*			m_buffer;
		Uint32			m_size;
		LocalBuffer*	m_next;

		LocalBuffer( Uint32 size );
		~LocalBuffer();
	};

private:
	LocalBuffer*			m_free;
	LocalBuffer*			m_used;
	Red::Threads::CMutex	m_accessMutex;

public:
	CLocalTextBufferManager();
	~CLocalTextBufferManager();

	//! Allocate temporary string buffer
	Char* AllocateTemporaryBuffer( Uint32 neededSize = 1024 );

	//! Deallocate temporary string buffer
	void DeallocateTemporaryBuffer( Char* buffer );
};

////////////////////////////////////////////////////

/// Local string singleton - helps to allocate local strings
typedef TSingleton< CLocalTextBufferManager >	SLocalTextBufferManager;

////////////////////////////////////////////////////

RED_WARNING_POP()

////////////////////////////////////////////////////