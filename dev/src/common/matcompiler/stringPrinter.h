/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// String writing helper
class CStringPrinter
{
public:
	TDynArray< AnsiChar >		m_text;

public:
	RED_INLINE CStringPrinter()
	{
		m_text.PushBack(0);
	}

	// Get string
	RED_INLINE const AnsiChar* AsChar() const
	{
		ASSERT( m_text.Size() );
		return m_text.TypedData();
	}

	// Append string
	RED_INLINE void Append( const AnsiChar* text )
	{
		const Uint32 len = text ? static_cast< Uint32 >( Red::System::StringLength( text ) ) : 0;
		if ( len )
		{
			const Uint32 oldSize = static_cast< Uint32 >( m_text.Grow( len ) );
			Red::System::MemoryCopy( &m_text[ oldSize-1 ], text, len+1 );
		}
	}

	// Prepend string
	RED_INLINE void Prepend( const AnsiChar* text )
	{
		const Uint32 len = text ? static_cast< Uint32 >( Red::System::StringLength( text ) ) : 0;
		if ( len )
		{
			const Uint32 oldSize = static_cast< Uint32 >( m_text.Grow( len ) );
			Red::System::MemoryCopy( &m_text[ len ], &m_text[ 0 ], oldSize+1 );
			Red::System::MemoryCopy( &m_text[ 0 ], text, len );
		}
	}

	// Print line
	RED_INLINE void Print( const AnsiChar* text, ... )
	{
		va_list arglist;
		va_start(arglist, text);
		AnsiChar formattedBuf[ 4096 ];
		vsprintf_s( formattedBuf,  ARRAY_COUNT(formattedBuf), text, arglist ); 
		const Uint32 len = static_cast< Uint32 >( Red::System::StringLength( formattedBuf ) );
		if ( len )
		{
			Append( formattedBuf );
			Append( "\r\n" );
		}
	}
};