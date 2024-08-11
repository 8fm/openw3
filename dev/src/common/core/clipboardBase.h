/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "dynarray.h"

/*
 * Clipboard formats
 */
enum EClipboardFormat
{
	CF_Unknown,				/*<! Unknown clipboard format */
	CF_Text,				/*<! Text only clipboard format - data is a String pointer */
	CF_Blob					/*<! Binary blob clipboard format - data is raw pointer, when pasting the first 4 bytes are the length
							     To get the proper length, pass a Int32 pointer with the length of the remaining bytes */
};

/*!
 * Provides rudimentary access to the system clipboard
 */
class IClipboard
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:

	virtual ~IClipboard(){}

	/*! Open connection to the clipboard with the given format */
	virtual Bool Open( EClipboardFormat format )=0;

	/*! Close the clipboard connection */
	virtual void Close()=0;

	/*! Copy the passed data with the passed size to the clipboard - the data type depends on the format used */
	virtual Bool Copy( const void* data, size_t length )=0;

	/*! Paste the data from the clipboard to the passed buffer - the data type depends on the format used */
	virtual Bool Paste( void* data, size_t length )=0;

	/*! Returns true if the clipboard contains data (note: some OSes may always report true) */
	virtual Bool HasData()=0;

	/*! Helper to copy a string - also opens and closes the clipboard */
	RED_INLINE Bool Copy( const String& stringValue )
	{
		if ( Open( CF_Text ) )
		{
			Bool success = Copy( &stringValue, sizeof(String*) );
			Close();
			return success;
		}
		return false;
	}

	/*! Helper to paste a string - also opens and closes the clipboard */
	RED_INLINE Bool Paste( String& stringValue )
	{
		if ( Open( CF_Text ) )
		{
			Bool success = Paste( &stringValue, sizeof(String*) );
			Close();
			return success;
		}
		return false;
	}

	/*! Helper to copy a data blob - also opens and closes the clipboard */
	RED_INLINE Bool Copy( const TDynArray< Uint8 >& blob )
	{
		if ( Open( CF_Blob ) )
		{
			Bool success = Copy( blob.Data(), static_cast< size_t >( blob.Size() ) );
			Close();
			return success;
		}
		return false;
	}

	/*! Helper to paste a data blob (note: this will clear the passed array) - also opens and closes the clipboard */
	RED_INLINE Bool Paste( TDynArray< Uint8 >& blob )
	{
		if ( Open( CF_Blob ) )
		{
			Uint32 length;
			if ( Paste( &length, 4 ) )
			{
				blob.Clear();
				blob.Grow( length + 4 );
				if ( Paste( blob.Data(), length + 4 ) )
				{
					blob.Erase( blob.Begin(), blob.Begin() + 4 );
					Close();
					return true;
				}
			}
			Close();
		}
		return false;
	}
};

/*!
 * Global clipboard
 */
extern IClipboard* GClipboard;
