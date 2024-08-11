/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef __RED_DATE_TIME_H__
#define __RED_DATE_TIME_H__

#include "../redSystem/clock.h"

#include "namesPool.h"
#include "rttiType.h"

// This class should never contain any virtual functions
// It serves as a kind of wrapper class at the Core project level
// to allow for it to be integrated into the RTTI system
class CDateTime : public Red::System::DateTime
{
public:

	RED_INLINE CDateTime() {}
	RED_INLINE explicit CDateTime( const CDateTime& other )
	:	Red::System::DateTime( other )
	{
	}
	
	RED_INLINE explicit CDateTime( const Red::System::DateTime& other )
	:	Red::System::DateTime( other )
	{
	}

	RED_INLINE void operator=( const CDateTime& other ) { Red::System::DateTime::operator=( other ); }
	RED_INLINE void operator=( const Red::System::DateTime& other ) { Red::System::DateTime::operator=( other ); }

	void ImportFromOldFileTimeFormat( Uint64 winStyleTimestamp );

	// Serialize to binary file
	void Serialize( IFile& file );

	// Serialization operator
	friend IFile& operator<<( IFile& file, CDateTime& dt )
	{
		dt.Serialize( file );
		return file;
	}

	static CDateTime INVALID;
};


//////////////////////////////////////////////////////////////////////////
// DateTime RTTI

RED_DECLARE_RTTI_NAME( CDateTime );

class CSimpleRTTITypeCDateTime : public TSimpleRTTIType< CDateTime >
{
public:
	virtual const CName& GetName() const { return CNAME( CDateTime ); }
	virtual ERTTITypeType GetType() const { return RT_Simple; }

	// No conversion from string
	//virtual Bool FromString( void*, const String& ) const { return false; }
};

template<>
struct TTypeName< CDateTime >
{
	static const CName& GetTypeName() { return CNAME( CDateTime ); }
};

template<> RED_INLINE String ToString( const CDateTime& dt )
{
	return ToString( dt, DateFormat_DayMonthYear );
}

template<> RED_INLINE Bool FromString( const String& strValue, CDateTime& dt )
{
	return FromString( strValue, dt, DateFormat_DayMonthYear );
}

#endif // __RED_DATE_TIME_H__
