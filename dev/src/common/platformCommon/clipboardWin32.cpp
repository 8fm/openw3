/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../../common/core/clipboardBase.h"
#include "clipboardWin32.h"

class CWin32Clipboard : public IClipboard
{
public:
	EClipboardFormat	m_format;
	UINT				m_blobFormat;

	CWin32Clipboard()
		: m_format( CF_Unknown )
	{
		m_blobFormat = ::RegisterClipboardFormat( TXT("RedEngineBlob") );
	}

	Bool Open( EClipboardFormat format )
	{
		// Try to open the windows clipboard
		if ( !::OpenClipboard( NULL ) )
		{
			return false;
		}

		// Keep the format
		m_format = format;

		return true;
	}

	void Close()
	{
		::CloseClipboard();
	}

	Bool CopyText( const String& sourceText )
	{
		// Make a local copy of the text 
		String text = sourceText;

		// Replace newline with CR-NL
		text.ReplaceAll( TXT("\n"), TXT("\r\n") );

		// Create a global moveable handle
		HGLOBAL handle = ::GlobalAlloc( GMEM_MOVEABLE, ( text.GetLength() + 1 )*sizeof( Char ) );
		if ( !handle )
		{
			return false;
		}

		// Put the text in the handle
		void* handleData = ::GlobalLock( handle );
		if ( !handleData )
		{
			::GlobalFree( handle );
			return false;
		}
		memcpy( handleData, text.AsChar(), ( text.GetLength() + 1 )*sizeof( Char ) );

		// Unlock it
		::GlobalUnlock( handle );

		// Put the handle to the clipboard
#ifdef UNICODE
		if ( !::SetClipboardData( CF_UNICODETEXT, handle ) )
#else
		if ( !::SetClipboardData( CF_TEXT, handle ) )
#endif
		{
			::GlobalFree( handle );
			return false;
		}

		return true;
	}

	Bool CopyBlob( const void* data, size_t length )
	{
		// Create a global moveable handle
		HGLOBAL handle = ::GlobalAlloc( GMEM_MOVEABLE, length + 4 );
		if ( !handle )
		{
			return false;
		}

		// Put the data in the handle
		void* handleData = ::GlobalLock( handle );
		if ( !handleData )
		{
			::GlobalFree( handle );
			return false;
		}
		Int32 lengthInt32 = static_cast< Int32 >( length );
		memcpy( handleData, &lengthInt32, 4 );
		memcpy( static_cast< BYTE* >( handleData ) + 4, data, length );

		// Unlock it
		::GlobalUnlock( handle );

		// Put the handle to the clipboard
		if ( !::SetClipboardData( m_blobFormat, handle ) )
		{
			::GlobalFree( handle );
			return false;
		}

		return true;
	}

	Bool Copy( const void* data, size_t length )
	{
		// Try to remove any previous contents
		if ( !::EmptyClipboard() )
		{
			return false;
		}

		switch ( m_format )
		{
		case CF_Text:
			return length == sizeof(String*) ? CopyText( *static_cast< const String* >( data ) ) : false;
		case CF_Blob:
			return CopyBlob( data, length );
		default:
			return false;
		}
	}

	Bool PasteText( String& text )
	{
		// Get the handle stored in the clipboard for text
#ifdef UNICODE
		HGLOBAL handle = ::GetClipboardData( CF_UNICODETEXT );
#else
		HGLOBAL handle = ::GetClipboardData( CF_TEXT );
#endif
		if ( !handle )
		{
			return false;
		}

		// Get the text from the handle
		void* handleData = ::GlobalLock( handle );
		if ( !handleData )
		{
			::GlobalFree( handle );
			return false;
		}
		text = static_cast< Char* >( handleData );

		// Replace CR-NL with newline
		text.ReplaceAll( TXT("\r\n"), TXT("\n") );

		// Unlock it
		::GlobalUnlock( handle );

		return true;
	}

	Bool PasteBlob( void* data, size_t length )
	{
		// Get the handle stored in the clipboard for text
		HGLOBAL handle = ::GetClipboardData( m_blobFormat );
		if ( !handle )
		{
			return false;
		}

		// Get the data from the handle
		void* handleData = ::GlobalLock( handle );
		if ( !handleData )
		{
			::GlobalFree( handle );
			return false;
		}
		memcpy( data, handleData, length );

		// Unlock it
		::GlobalUnlock( handle );

		return true;
	}

	Bool Paste( void* data, size_t length )
	{
		switch ( m_format )
		{
		case CF_Text:
			return length == sizeof(String*) ? PasteText( *static_cast< String* >( data ) ) : false;
		case CF_Blob:
			return PasteBlob( data, length );
		default:
			return false;
		}
	}

	Bool HasData()
	{
		switch ( m_format )
		{
		case CF_Text:
#ifdef UNICODE
			return ::IsClipboardFormatAvailable( CF_UNICODETEXT ) ? true : false;
#else
			return ::IsClipboardFormatAvailable( CF_TEXT ) ? true : false;
#endif
		default:
			return ::IsClipboardFormatAvailable( m_blobFormat ) ? true : false;
		}
	}
};

void SInitializeWin32Clipboard()
{
	GClipboard = new CWin32Clipboard();
}
