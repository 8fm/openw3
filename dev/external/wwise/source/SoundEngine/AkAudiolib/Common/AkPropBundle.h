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

#include <AK/Tools/Common/AkBankReadHelpers.h>
#include <AK/Tools/Common/AkObject.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

#include "AkParameters.h"

// Bundle of properties specified by AkPropID. 
// m_pProps points to a dynamic structure consisting of:
//
// 1 x AkUInt8 = number of properties (cProp)
// cProp x AkUInt8 = cProp times AkPropID of property
// padding to 32 bit
// cProp x T_VALUE = cProp times value of property
//

template<class T_VALUE>
class AkPropBundle
{
public:
	struct Iterator
	{
		AkUInt8* pID;
		T_VALUE* pValue;

		/// ++ operator
		Iterator& operator++()
		{
			AKASSERT( pID && pValue );
			++pID;
			++pValue;
			return *this;
		}

		/// -- operator
        Iterator& operator--()
		{
			AKASSERT( pID && pValue );
			--pID;
			--pValue;
			return *this;
		}

		/// * operator
		T_VALUE& operator*()
		{
			AKASSERT( pValue );
			return *pValue;
		}

		/// == operator
		bool operator ==( const Iterator& in_rOp ) const
		{
			return ( pID == in_rOp.pID );
		}

		/// != operator
		bool operator !=( const Iterator& in_rOp ) const
		{
			return ( pID != in_rOp.pID );
		}
	};

	/// Returns the iterator to the first item of the array, will be End() if the array is empty.
	Iterator Begin() const
	{
		Iterator returnedIt;
		if ( m_pProps )
		{
			AkUInt8 cProps = m_pProps[ 0 ];
			returnedIt.pID = m_pProps + 1;
			returnedIt.pValue = (T_VALUE *) ( m_pProps + FirstPropByteOffset( cProps ) );
		}
		else
		{
			returnedIt.pID = NULL;
			returnedIt.pValue = NULL;
		}
		return returnedIt;
	}

	/// Returns the iterator to the end of the array
	Iterator End() const
	{
		Iterator returnedIt;
		returnedIt.pValue = NULL; // Since iterator comparison checks only pID
		if ( m_pProps )
		{
			AkUInt8 cProps = m_pProps[ 0 ];
			returnedIt.pID = m_pProps + cProps + 1;
		}
		else
		{
			returnedIt.pID = NULL;
		}
		return returnedIt;
	}

	AkPropBundle() : m_pProps( NULL ) {}

	~AkPropBundle()
	{
		RemoveAll();
	}

	AKRESULT SetInitialParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize ) 
	{
		AKASSERT( !m_pProps );

		AkUInt8 cProps = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
		if ( cProps )
		{
			AkUInt32 uFirstPropByteOffset = FirstPropByteOffset( cProps );
			AkUInt32 uAllocSize = uFirstPropByteOffset + sizeof( T_VALUE ) * cProps;
			AkUInt8 * pProps = (AkUInt8 *) AkAlloc( g_DefaultPoolId, uAllocSize );
			if ( !pProps )
				return AK_InsufficientMemory;

			pProps[0] = cProps;

			// Copy prop ids
			memcpy( pProps + 1, io_rpData, sizeof( AkUInt8 ) * cProps );
			SKIPBANKBYTES( sizeof( AkUInt8 ) * cProps, io_rpData, io_rulDataSize );

			// Copy prop values
			memcpy( pProps + uFirstPropByteOffset, io_rpData, sizeof( T_VALUE ) * cProps );
			SKIPBANKBYTES( sizeof( T_VALUE ) * cProps, io_rpData, io_rulDataSize );

			m_pProps = pProps;
		}

		return AK_Success;
	}

	// Add a property value, when it is known that there is currently no value for this property in the bundle.
	T_VALUE * AddAkProp( AkUInt8 in_ePropID )
	{
		AKASSERT( !FindProp( in_ePropID ) );

		AkUInt32 cProps = m_pProps ? m_pProps[ 0 ] : 0;

		AkUInt32 uAllocSize = FirstPropByteOffset( cProps + 1 ) + (cProps + 1 ) * sizeof( T_VALUE );
		AkUInt8 * pProps = (AkUInt8 *) AkAlloc( g_DefaultPoolId, uAllocSize );
		if ( !pProps )
			return NULL;

		if ( m_pProps )
		{
			// Copy prop ids
			memcpy( pProps + 1, m_pProps + 1, sizeof( AkUInt8 ) * cProps );

			// Copy prop values
			memcpy( pProps + FirstPropByteOffset( cProps + 1 ), m_pProps + FirstPropByteOffset( cProps ), sizeof( T_VALUE ) * cProps );

			AkFree( g_DefaultPoolId, m_pProps );
		}

		pProps[ cProps + 1 ] = in_ePropID;

		pProps[ 0 ] = (AkUInt8) (cProps + 1);
		m_pProps = pProps;

		T_VALUE * pProp = (T_VALUE*) ( pProps + FirstPropByteOffset( cProps + 1 ) ) + cProps;
		return pProp;
	}

	// Set a property value, adding the property to bundle if necessary.
	AKRESULT SetAkProp( AkUInt8 in_ePropID, T_VALUE in_Value )
	{
		T_VALUE * pProp = FindProp( in_ePropID );
		if ( !pProp )
			pProp = AddAkProp( in_ePropID );

		if ( pProp )
		{
			*pProp = in_Value;
			return AK_Success;
		}

		return AK_Fail;
	}

	AkForceInline T_VALUE GetAkProp( AkUInt8 in_ePropID, T_VALUE in_DefaultValue ) const
	{
		T_VALUE * pProp = FindProp( in_ePropID );

		return pProp ? *pProp : in_DefaultValue;
	}

	T_VALUE * FindProp( AkUInt8 in_ePropID ) const
	{
		if ( m_pProps )
		{
			AkUInt32 iProp = 0;
			AkUInt32 cProp = m_pProps[ 0 ];
			AKASSERT( cProp > 0 );

			do
			{
				if ( m_pProps[ iProp + 1 ] == in_ePropID )
					return (T_VALUE*) ( m_pProps + FirstPropByteOffset( cProp ) ) + iProp;
			}
			while ( ++iProp < cProp );
		}

		return NULL;
	}

	void RemoveAll()
	{
		if ( m_pProps )
			AkFree( g_DefaultPoolId, m_pProps );

		m_pProps = NULL;
	}

	bool IsEmpty() const { return m_pProps == NULL; }

private:
	// Return byte offset of first property value
	static AkForceInline AkUInt32 FirstPropByteOffset( AkUInt32 in_cProp )
	{
		// Enforce 4-byte alignment for T_VALUE
		return ( ( in_cProp + 1 ) + 3 ) & ~0x3;
	}

	AkUInt8 * m_pProps;
};
