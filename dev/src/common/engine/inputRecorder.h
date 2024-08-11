/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "inputBufferedInputEvent.h"

#ifndef NO_TEST_FRAMEWORK

struct SRecordedInput : public SBufferedInputEvent
{
	DECLARE_RTTI_STRUCT( SRecordedInput );

	SRecordedInput()
	:	SBufferedInputEvent()
	{
	}

	SRecordedInput( const SBufferedInputEvent& other )
	:	SBufferedInputEvent( other.key, other.action, other.data )
	{
	}
	
	friend IFile& operator<<( IFile& file, SRecordedInput& input )
	{
		Uint32 keyInt = static_cast< Uint32 >( input.key );
		Uint32 actionInt = static_cast< Uint32 >( input.action );

		file << keyInt;
		file << actionInt;
		file << input.data;

		if( file.IsReader() )
		{
			input.key = static_cast< EInputKey >( keyInt );
			input.action = static_cast< EInputAction >( actionInt );
		}

		return file;
	}
};

BEGIN_CLASS_RTTI( SRecordedInput );
END_CLASS_RTTI();

class CInputRecorder
{
public:
	CInputRecorder( const String& testName );
	~CInputRecorder();
	
	//! Process input
	void ProcessInput( Uint64 currentRelativeTick, const BufferedInput& input );

	void OnSave();

protected:
	typedef THashMap< Uint64, TDynArray< SRecordedInput > > TRawInputMap;

	TRawInputMap	m_rawInput;

private:
	String			m_testName;
	Uint64			m_startTick;

#ifndef NO_RESOURCE_IMPORT
	void SaveRawInput();
#endif

	template< typename T >
	void AddToMap( THashMap< Uint64, TDynArray< T > >& itemMap, Uint64 key, const T& item ) const
	{
		TDynArray< T >* itemArray = itemMap.FindPtr( key );

		if ( itemArray )
		{
			itemArray->PushBack( item );
		}
		else
		{
			TDynArray< T > newItemArray;
			newItemArray.PushBack( item );
			itemMap.Insert( key, newItemArray );
		}
	}
};

#endif // NO_TEST_FRAMEWORK
