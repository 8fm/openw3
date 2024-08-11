/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

#ifndef _AKSTATICARRAY_H
#define _AKSTATICARRAY_H

#include <AK/Tools/Common/AkObject.h>

template <class T, class ARG_T, AkUInt32 TSize> class AkStaticArray
{
public:
	AkStaticArray()
		: m_cItems( 0 )
	{
	}

	~AkStaticArray()
	{
		AKASSERT( m_cItems == 0 );
	}
	
	void Term()
	{
		RemoveAll();
	}

	AkUInt32 Length() const
	{
		return m_cItems;
	}

	bool IsEmpty() const
	{
		return m_cItems == 0;
	}

	T& operator[]( AkUInt32 in_iItem )
	{
		return m_Items[ in_iItem ];
	}

	T * AddLast()
	{
		if ( m_cItems >= TSize )
			return NULL;

		T * pItem = &m_Items[ m_cItems++ ];
		
		AkPlacementNew( pItem ) T; 
			
		return pItem;
	}

	T& Last()
	{
		AKASSERT( m_cItems );

		return m_Items[ m_cItems - 1 ];
	}

    void RemoveLast()
    {
		AKASSERT( m_cItems );

        m_Items[ --m_cItems ].~T();
    }

	void RemoveAll()
	{
		for ( AkUInt32 iItem = 0, cItems = m_cItems; iItem < cItems; ++iItem )
			m_Items[ iItem ].~T();

		m_cItems = 0;
	}

protected:
	AkUInt32	m_cItems;	// how many we currently have
	T			m_Items[ TSize ];
};

#endif // _AKSTATICARRAY_H
