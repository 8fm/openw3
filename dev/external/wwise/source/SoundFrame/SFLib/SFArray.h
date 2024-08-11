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

#pragma once

#pragma push_macro("new")

#ifndef _INC_NEW
	#include <new.h>
#endif

#include <malloc.h>

template<class TYPE, class ARG_TYPE = const TYPE&>
class SFArray 
{
public:
	SFArray()
		: m_pData( NULL )
		, m_nSize( 0 )
		, m_nMaxSize( 0 )
	{
	}

	~SFArray()
	{
		if (m_pData != NULL)
		{
			for( int i = 0; i < m_nSize; i++ )
				(m_pData + i)->~TYPE();
			delete[] (BYTE*)m_pData;
		}
	}

	int GetSize() const
	{
		return m_nSize;
	}

	bool IsEmpty() const
	{
		return m_nSize == 0;
	}

	int GetUpperBound() const
	{
		return m_nSize-1;
	}

	void SetSize( int nNewSize, int nGrowBy = 0 )
	{
//		ASSERT(nNewSize >= 0);

		if (nNewSize == 0)
		{
			// shrink to nothing
			if (m_pData != NULL)
			{
				for( int i = 0; i < m_nSize; i++ )
					(m_pData + i)->~TYPE();
				delete[] (BYTE*)m_pData;
				m_pData = NULL;
			}
			m_nSize = m_nMaxSize = 0;
		}
		else if (m_pData == NULL)
		{
			// create buffer big enough to hold number of requested elements or
			// m_nGrowBy elements, whichever is larger.
			int nAllocSize = max(nNewSize, nGrowBy);
			m_pData = (TYPE*) new BYTE[(size_t)nAllocSize * sizeof(TYPE)];
			memset((void*)m_pData, 0, (size_t)nAllocSize * sizeof(TYPE));
			for( int i = 0; i < nNewSize; i++ )
	#pragma push_macro("new")
	#undef new
				::new( (void*)( m_pData + i ) ) TYPE;
	#pragma pop_macro("new")
			m_nSize = nNewSize;
			m_nMaxSize = nAllocSize;
		}
		else if (nNewSize <= m_nMaxSize)
		{
			// it fits
			if (nNewSize > m_nSize)
			{
				// initialize the new elements
				memset((void*)(m_pData + m_nSize), 0, (size_t)(nNewSize-m_nSize) * sizeof(TYPE));
				for( int i = 0; i < nNewSize-m_nSize; i++ )
	#pragma push_macro("new")
	#undef new
					::new( (void*)( m_pData + m_nSize + i ) ) TYPE;
	#pragma pop_macro("new")
			}
			else if (m_nSize > nNewSize)
			{
				// destroy the old elements
				for( int i = 0; i < m_nSize-nNewSize; i++ )
					(m_pData + nNewSize + i)->~TYPE();
			}
			m_nSize = nNewSize;
		}
		else
		{
			// otherwise, grow array
			if (nGrowBy == 0)
			{
				// heuristically determine growth when nGrowBy == 0
				//  (this avoids heap fragmentation in many situations)
				nGrowBy = m_nSize / 8;
				nGrowBy = (nGrowBy < 4) ? 4 : ((nGrowBy > 1024) ? 1024 : nGrowBy);
			}
			int nNewMax;
			if (nNewSize < m_nMaxSize + nGrowBy)
				nNewMax = m_nMaxSize + nGrowBy;  // granularity
			else
				nNewMax = nNewSize;  // no slush

//			ASSERT(nNewMax >= m_nMaxSize);  // no wrap around
			
//			if(nNewMax  < m_nMaxSize)
//				AfxThrowInvalidArgException();

			TYPE* pNewData = (TYPE*) new BYTE[(size_t)nNewMax * sizeof(TYPE)];

			// copy new data from old
			memcpy(pNewData, m_pData, (size_t)m_nSize * sizeof(TYPE));

			// construct remaining elements
//			ASSERT(nNewSize > m_nSize);
			memset((void*)(pNewData + m_nSize), 0, (size_t)(nNewSize-m_nSize) * sizeof(TYPE));
			for( int i = 0; i < nNewSize-m_nSize; i++ )
	#pragma push_macro("new")
	#undef new
				::new( (void*)( pNewData + m_nSize + i ) ) TYPE;
	#pragma pop_macro("new")

			// get rid of old stuff (note: no destructors called)
			delete[] (BYTE*)m_pData;
			m_pData = pNewData;
			m_nSize = nNewSize;
			m_nMaxSize = nNewMax;
		}
	}

	void RemoveAll()
	{
		SetSize( 0 );
	}

	const TYPE& GetAt( int nIndex ) const
	{
//		_ASSERT(nIndex >= 0 && nIndex < m_nSize);
		if(nIndex >= 0 && nIndex < m_nSize)
			return m_pData[nIndex]; 
		else
		{
			static TYPE t;
			return (TYPE&) t;
		}
	}

	TYPE& GetAt( int nIndex )
	{
//		_ASSERT(nIndex >= 0 && nIndex < m_nSize);
		if(nIndex >= 0 && nIndex < m_nSize)
			return m_pData[nIndex]; 
		else
		{
			static TYPE t;
			return (TYPE&) t;
		}
	}

	void SetAt( int nIndex, ARG_TYPE newElement )
	{
//		ASSERT(nIndex >= 0 && nIndex < m_nSize);
		if(nIndex >= 0 && nIndex < m_nSize)
			m_pData[nIndex] = newElement; 
	}

	// Potentially growing the array
	void SetAtGrow( int nIndex, ARG_TYPE newElement )
	{
//		ASSERT(nIndex >= 0);
		if(nIndex >= 0)
		{
			if (nIndex >= m_nSize)
				SetSize(nIndex+1, -1);

			m_pData[nIndex] = newElement;
		}
	}

	INT_PTR Add(ARG_TYPE newElement)
	{
		int nIndex = m_nSize;
		SetAtGrow(nIndex, newElement);
		return nIndex; 
	}

	void RemoveAt(int nIndex, int nCount = 1)
	{
		BOOL bAllowed = ( nIndex >= 0 && nCount >= 0 && (nIndex + nCount <= m_nSize) );
		ASSERT( bAllowed );
		if ( bAllowed )
		{
			// just remove a range
			int nMoveCount = m_nSize - (nIndex + nCount);
			for( int i = 0; i < nCount; i++ )
				(m_pData + nIndex + i)->~TYPE();
			if (nMoveCount)
				memmove( m_pData + nIndex, m_pData + nIndex + nCount,
					(size_t)nMoveCount * sizeof(TYPE));
			m_nSize -= nCount;
		}
	}

	// overloaded operator helpers
	const TYPE& operator[](int nIndex) const
	{
		return GetAt( nIndex );
	}

	TYPE& operator[](int nIndex)
	{
		return GetAt( nIndex );
	}

// Implementation
protected:
	TYPE* m_pData;   // the actual array of data
	int m_nSize;     // # of elements (upperBound - 1)
	int m_nMaxSize;  // max allocated
};

#pragma pop_macro("new")

