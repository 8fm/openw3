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

//////////////////////////////////////////////////////////////////////
//
// AkConversionTable.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _CONVERSION_TABLE_H_
#define _CONVERSION_TABLE_H_

#include "AkRTPC.h"
#include "AkInterpolation.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/Tools/Common/AkObject.h>

template <class T_GraphPointType, class Y_Type>
class CAkConversionTable
{
public:

	//Constructor
	CAkConversionTable()
	:m_pArrayGraphPoints( NULL )
	,m_ulArraySize( 0 )
	,m_eScaling( AkCurveScaling_None )
	{
	}

	AkForceInline Y_Type Convert( AkReal32 in_valueToConvert )
	{
		AkUInt32 index = 0;
		return ConvertInternal(in_valueToConvert, 0, index);
	}

	AkForceInline Y_Type ConvertProgressive( AkReal32 in_valueToConvert, AkUInt32& io_index )
	{
		return ConvertInternal( in_valueToConvert, io_index, io_index );
	}

	//Convert the input value into the output value, applying conversion if the converter is available
	Y_Type ConvertInternal( AkReal32 in_valueToConvert, AkUInt32 in_index, AkUInt32& out_index )
	{
		AKASSERT( m_pArrayGraphPoints && m_ulArraySize );

		Y_Type l_returnedValue;

		if(m_ulArraySize == 1)
		{
			l_returnedValue = m_pArrayGraphPoints[0].To;
		}
		else
		{
			for( ; in_index < m_ulArraySize; ++in_index )
			{
				if( in_valueToConvert <= m_pArrayGraphPoints[in_index].From)
				{
					// You will get here if:
					//    * You evaluate before the first point
					//    * or you evaluate exactly on a point
					l_returnedValue = m_pArrayGraphPoints[in_index].To;
					break;
				}
				else if( ( in_index < m_ulArraySize - 1 ) && ( in_valueToConvert < m_pArrayGraphPoints[in_index+1].From ) )
				{
					// You will get here if:
					//    * You evaluate between 2 points (in the "if" we check in_index first to
					//      make sure there *is* a next point)

					const T_GraphPointType& firstPoint( m_pArrayGraphPoints[in_index] );
					const T_GraphPointType& secondPoint( m_pArrayGraphPoints[in_index+1] );

					if ( m_pArrayGraphPoints[in_index].Interp == AkCurveInterpolation_Linear )
					{
						// Linear interpolation between the two points
						l_returnedValue = static_cast<Y_Type>( AkMath::InterpolateNoCheck(
							firstPoint.From,
							static_cast<AkReal32>( firstPoint.To ),
							secondPoint.From,
							static_cast<AkReal32>( secondPoint.To ),
							in_valueToConvert
							) );
					}
					else if ( m_pArrayGraphPoints[in_index].Interp == AkCurveInterpolation_Constant )
					{
						// Constant interpolation -> Value of the first point
						l_returnedValue = firstPoint.To;
					}
					else
					{
						l_returnedValue = static_cast<Y_Type>( AkInterpolation::InterpolateNoCheck(
							( in_valueToConvert - static_cast<AkReal32>( firstPoint.From ) )
								/ ( static_cast<AkReal32>( secondPoint.From ) - static_cast<AkReal32>( firstPoint.From ) ),
							static_cast<AkReal32>( firstPoint.To ),
							static_cast<AkReal32>( secondPoint.To ),
							firstPoint.Interp ) );
					}

					break;
				}
				else if( in_index == m_ulArraySize - 1 )
				{
					// You will get here if:
					//    * You evaluate after the last point

					//Then we take the last available value
					l_returnedValue = m_pArrayGraphPoints[in_index].To;

					break;
				}
			}

			out_index = in_index;
		}

		switch( m_eScaling )
		{
			case AkCurveScaling_None:
				break;

			case AkCurveScaling_dB:
				l_returnedValue = static_cast<Y_Type>( AkMath::ScalingFromLin_dB( static_cast<AkReal32>( l_returnedValue ) ) );
				break;

			case AkCurveScaling_Log:
				l_returnedValue = static_cast<Y_Type>( AkMath::ScalingFromLog( static_cast<AkReal32>( l_returnedValue ) ) );
				break;

			case AkCurveScaling_dBToLin:
				l_returnedValue = static_cast<Y_Type>( AkMath::dBToLin( static_cast<AkReal32>( l_returnedValue ) ) );
				break;

			default:
				AKASSERT( ! "Unknown scaling mode!" );
		}

		return l_returnedValue;
	}

	AKRESULT Set(
		T_GraphPointType*			in_pArrayConversion,
		AkUInt32						in_ulConversionArraySize,
		AkCurveScaling				in_eScaling
		)
	{
		AKRESULT eResult = AK_InvalidParameter;
		Unset();
		AKASSERT( !m_pArrayGraphPoints );

		if(in_pArrayConversion && in_ulConversionArraySize)
		{
			m_pArrayGraphPoints = (T_GraphPointType*)AkAlloc( g_DefaultPoolId, in_ulConversionArraySize*sizeof(T_GraphPointType) );
			if(m_pArrayGraphPoints)
			{
				AKPLATFORM::AkMemCpy( m_pArrayGraphPoints, in_pArrayConversion, in_ulConversionArraySize*sizeof(T_GraphPointType) );
				m_ulArraySize = in_ulConversionArraySize;
				m_eScaling = in_eScaling;
				eResult = AK_Success;
			}
			else
			{
				eResult = AK_InsufficientMemory;
				m_ulArraySize = 0;
			}
		}
		
		return eResult;
	}

	// REVIEW: this should eventually be done at soundbank generation time.
	void Linearize()
	{
		if ( m_eScaling == AkCurveScaling_None )
		{
			m_eScaling = AkCurveScaling_dBToLin;
		}
		else if ( m_eScaling == AkCurveScaling_dB )
		{
			for( AkUInt32 i = 0; i < m_ulArraySize; ++i )
			{
				AKASSERT( m_pArrayGraphPoints[ i ].To <= 0.0f );
				m_pArrayGraphPoints[ i ].To = m_pArrayGraphPoints[ i ].To + 1;
			}

			m_eScaling = AkCurveScaling_None;
		}
		else
		{
			AKASSERT( false && "Scaling type cannot be linearized!" );
		}
	}

	void Unset()
	{
		if(m_pArrayGraphPoints)
		{
			AkFree( g_DefaultPoolId, m_pArrayGraphPoints );
			m_pArrayGraphPoints = NULL;
		}

		m_ulArraySize = 0;
		m_eScaling = AkCurveScaling_None;
	}

	AkReal32 GetMidValue()
	{
		AkReal32 l_DefaultVal = 0;
		if( m_pArrayGraphPoints && m_ulArraySize)
		{
			AkReal32 l_LowerBound = m_pArrayGraphPoints[0].From;
			AkReal32 l_HigherBound = m_pArrayGraphPoints[m_ulArraySize-1].From;
			l_DefaultVal = ( l_LowerBound + l_HigherBound )* 0.5f;
		}
		return l_DefaultVal;
	}

public://public to allow direct access
	T_GraphPointType*			m_pArrayGraphPoints;
	AkUInt32					m_ulArraySize;		//The number of sets of points in the array
	AkCurveScaling				m_eScaling;
};

#endif //_CONVERSION_TABLE_H_
