/**
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

//////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

namespace // anonymous
{
	const SF::SPInt DEFAULT_STRING_DECODE_STACK_CHARS = 256;

	template< size_t StackChars = DEFAULT_STRING_DECODE_STACK_CHARS >
	class CScaleformStringUtf8Converter
	{
	private:
		UniChar		m_stackMem[ StackChars ];
		UniChar*	m_dstBuf;

	private:
		void convert( const SFChar* src )
		{
			if ( ! src )
			{
				m_dstBuf = m_stackMem;
				*m_dstBuf = '\0';
				return;
			}

			SF::SPInt len = SF::UTF8Util::GetLength( src );
			if ( len + 1 > StackChars )
			{
				m_dstBuf = static_cast< UniChar* >( RED_MEMORY_ALLOCATE( MemoryPool_Strings, MC_String, ( len + 1 ) * sizeof( UniChar) ) );
				ASSERT( m_dstBuf );
			}
			else
			{
				m_dstBuf = m_stackMem;
			}

			SF::UTF8Util::DecodeString( m_dstBuf, src );
		}

	public:
		const UniChar* ucs2str() const
		{
			return m_dstBuf;
		}

	public:
		CScaleformStringUtf8Converter( const SFChar* src )
			: m_dstBuf( nullptr )
		{
			convert( src );
		}

		~CScaleformStringUtf8Converter()
		{
			if ( m_dstBuf && m_dstBuf != m_stackMem )
			{
				RED_MEMORY_FREE( MemoryPool_Strings, MC_String, m_dstBuf );
			}
		}
	};

	template< size_t StackChars = DEFAULT_STRING_DECODE_STACK_CHARS >
	class CScaleformStringUcs2Converter
	{
	private:
		SFChar	m_stackMem[ StackChars ];
		SFChar*	m_dstBuf;

	private:
		void convert( const UniChar* src )
		{
			if ( ! src )
			{
				m_dstBuf = m_stackMem;
				*m_dstBuf = '\0';
				return;
			}

			SF::SPInt len = SF::UTF8Util::GetEncodeStringSize( src );
			if ( len + 1 > StackChars )
			{
				m_dstBuf = static_cast< SFChar* >( RED_MEMORY_ALLOCATE( MemoryPool_Strings, MC_String, ( len + 1 ) * sizeof( SFChar) ) );
				ASSERT( m_dstBuf );
			}
			else
			{
				m_dstBuf = m_stackMem;
			}

			SF::UTF8Util::EncodeString( m_dstBuf, src );
		}

	public:
		const SFChar* utf8str() const
		{
			return m_dstBuf;
		}

	public:
		CScaleformStringUcs2Converter( const UniChar* src )
			: m_dstBuf( nullptr )
		{
			convert( src );
		}

		~CScaleformStringUcs2Converter()
		{
			if ( m_dstBuf && m_dstBuf != m_stackMem )
			{
				RED_MEMORY_FREE( MemoryPool_Strings, MC_String, m_dstBuf );
			}
		}
	};
}

#ifdef UNICODE
#	define FLASH_TXT_TO_UTF8( str ) (SFChar*)CScaleformStringUcs2Converter<>( (str) ).utf8str()
#	define FLASH_UTF8_TO_TXT( str ) (UniChar*)CScaleformStringUtf8Converter<>( (str) ).ucs2str()
#	define FLASH_LOG_UTF8_TO_TXT( str ) (UniChar*)CScaleformStringUtf8Converter< 2048 >( (str) ).ucs2str()
#else
#	define FLASH_TXT_TO_UTF8( str ) (str)
#	define FLASH_UTF8_TO_TXT( str ) (str)
#endif

// Convert value to string
template<>
RED_INLINE String ToString( const SF::String& value )
{
	return FLASH_UTF8_TO_TXT( value.ToCStr() );
}

// Convert value to string
template<>
RED_INLINE String ToString( const GFx::Value& value )
{
	if ( value.IsStringW() )
	{
		return value.GetStringW();
	}

	if ( value.IsString() )
	{
		return FLASH_UTF8_TO_TXT( value.GetString() );
	}

	return ToString( value.ToString() );
}

// Convert string to SF::String
template<>
RED_INLINE Bool FromString( const String& text, SF::String& value )
{
	value = SF::String( FLASH_TXT_TO_UTF8( text.AsChar() ) );
	return true;
};

//////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////