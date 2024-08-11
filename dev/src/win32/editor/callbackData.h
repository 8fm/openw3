/*
 * Copyright © 2007-2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

template <class T>
class TCallbackData : public wxObject
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Editor );
public:
	TCallbackData( const T& data )
		: m_data( data ) {}

	RED_INLINE const T& GetData() const
	{ return m_data; }

private:
	T m_data;
};
