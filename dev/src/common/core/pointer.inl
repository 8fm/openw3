/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

RED_INLINE CPointer::CPointer()
	: m_class( NULL )
{
}

RED_INLINE CPointer::~CPointer()
{
	release();
}

RED_INLINE CPointer::CPointer( void* pointer, CClass* theClass )
	: m_class( NULL )
{
	if ( NULL != pointer )
	{
		initialize( pointer, theClass );
	}
}

RED_INLINE CPointer::CPointer( const CPointer& pointer )
	: m_class( NULL )
{
	initialize( pointer.m_data.Get(), pointer.m_class );
}

RED_INLINE CPointer& CPointer::operator=( const CPointer& other )
{
	initialize( other.m_data.Get(), other.m_class );
	return *this;
}
	 
RED_INLINE Bool CPointer::operator==( const CPointer& other ) const
{
	// only the data pointers are compared directly
	return m_data == other.m_data;
}

RED_INLINE Bool CPointer::operator!=( const CPointer& other ) const
{
	// only the data pointers are compared directly
	return m_data != other.m_data;
}