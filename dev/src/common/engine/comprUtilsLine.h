
#pragma once

#include "comprUtilsDataBuffer.h"

namespace AnimCompressionUtils
{
	class LineCompression
	{
	public:
		struct CompressionParams
		{
			int		m_minKnots;
			float	m_eps;
			int		m_maxKnotDistance;

			CompressionParams() : m_minKnots( 5 ), m_eps( FLT_MAX ), m_maxKnotDistance( -1 ) {}
		};

	private:
		UsedPointsTable m_usedPoints;
		Int32				m_maxKnotDistance;

	private:
		RED_INLINE float VectorDistanceSqr( int pointSize, const float* pointA, const float* pointB ) const
		{
			float ret = 0.f;

			for ( int i=0; i<pointSize; ++i )
			{
				ret += ( pointA[ i ] - pointB[ i ] ) * ( pointA[ i ] - pointB[ i ] );
			}

			return ret;
		}

		RED_INLINE float VectorDot( int pointSize, const float* pointA1, const float* pointA2, const float* pointB1, const float* pointB2 ) const
		{
			float ret = 0.f;

			for ( int i=0; i<pointSize; ++i )
			{
				ret += ( pointA1[ i ] - pointA2[ i ] ) * ( pointB1[ i ] - pointB2[ i ] );
			}

			return ret;
		}

		RED_INLINE float CalcDistPointToLine( int pointSize, const float* point, const float* lineA, const float* lineB ) const
		{
			float lenSq = VectorDistanceSqr( pointSize, lineB, lineA );
			float dot = VectorDot( pointSize, point, lineA, lineB, lineA ) / lenSq;

			float ret = 0.f;

			for ( int i=0; i<pointSize; ++i )
			{
				float val = ( point[ i ] - lineA[ i ] ) - ( lineB[ i ] - lineA[ i ] ) * dot;
				ret += val * val;
			}

			return ret;
		}

		RED_INLINE float LineInterpolation( float a, float b, float p ) const
		{
			return a + ( b - a ) * p;
		}

		RED_INLINE float CalcLineErrorAtPoint( int pointSize, const float* inputPoint, const float* lineA, const float* lineB, float progress ) const
		{
			float ret = 0.f;

			for ( int i=0; i<pointSize; ++i )
			{
				float lineVal = LineInterpolation( lineA[ i ], lineB[ i ], progress );
				ret += sqrtf( ( inputPoint[ i ] - lineVal ) * ( inputPoint[ i ] - lineVal ) );
			}

			return ret;
		}

		float CalcErrorWithoutPoint( int point, const DataBuffer& dataIn, UsedPointsTable& usedPoints ) const
		{
			float totalErr = 0.f;

			UsedPointsInterator it( usedPoints );

			{
				const float* valA = dataIn[ it.Prev() ];
				const float* valB = dataIn[ it.Curr() ];

				for ( int i=it.Prev(); i<it.Curr(); ++i )
				{
					const float* valInput = dataIn[ i ];

					float p = (float)( ( i - it.Prev() ) / ( it.Curr() - it.Prev() ) );
					ASSERT( p >= 0.f && p <= 1.f );

					float err = CalcLineErrorAtPoint( dataIn.m_pointSize, valInput, valA, valB, p );
					totalErr += err;
				}
			}

			for ( ; it; ++it )
			{
				const float* valA = dataIn[ it.Curr() ];
				const float* valB = dataIn[ it.Next() ];

				for ( int i=it.Curr(); i<it.Next(); ++i )
				{
					const float* valInput = dataIn[ i ];

					float p = (float)( ( i - it.Curr() ) / ( it.Next() - it.Curr() ) );
					ASSERT( p >= 0.f && p <= 1.f );

					float err = CalcLineErrorAtPoint( dataIn.m_pointSize, valInput, valA, valB, p );
					totalErr += err;
				}
			}

			return totalErr;
		}

		RED_INLINE void SelectTheWorstPoint( const DataBuffer& dataIn, const UsedPointsTable& usedPoints, int& point, float& cost ) const
		{
			float minCost = FLT_MAX;
			int toRemovePoint = -1;

			for ( UsedPointsInterator it( usedPoints ); it; ++it )
			{
				const float* valA = dataIn[ it.Prev() ];
				const float* valB = dataIn[ it.Next() ];
				const float* valMid = dataIn[ it.Curr() ];

				if ( m_maxKnotDistance > 0 )
				{
					ASSERT( it.Next() - it.Prev() >= 0 );

					if ( it.Next() - it.Prev() > m_maxKnotDistance )
					{
						continue;
					}
				}

				float removePointCost = CalcDistPointToLine( dataIn.m_pointSize, valMid, valA, valB );
				if ( removePointCost < minCost )
				{
					toRemovePoint = it.Curr();
					minCost = removePointCost;
				}
			}

			point = toRemovePoint;
			cost = sqrt( minCost );
		}

	public:
		bool Compress( const CompressionParams& params, const DataBuffer& dataIn )
		{
			m_usedPoints.Create( dataIn.m_numPoints );
			m_maxKnotDistance = params.m_maxKnotDistance;
			
			while ( 1 )
			{
				int point = -1;
				float cost = FLT_MAX;

				SelectTheWorstPoint( dataIn, m_usedPoints, point, cost );

				if ( point == -1 )
				{
					break;
				}

				//float err = CalcErrorWithoutPoint( point, dataIn, usedPoints );

				if ( cost > params.m_eps )
				{
					break;
				}

				int usedPointsNum = m_usedPoints.GetNumberOfUsedPoints();
				if ( usedPointsNum == params.m_minKnots )
				{
					break;
				}

				if ( m_usedPoints.GetNumberOfUsedPoints() <= 2 )
				{
					break;
				}

				// Remove point
				m_usedPoints.SetNotUsed( point );
			}

			return true;
		}

		void GenerateDataOut( const DataBuffer& dataIn, DataBuffer& dataOut )
		{
			dataOut.Create( m_usedPoints.GetNumberOfUsedPoints(), dataIn.m_pointSize );
			int it = 0;

			for ( int i=0; i<m_usedPoints.Size(); ++i )
			{
				if ( m_usedPoints.IsUsed( i ) )
				{
					Red::System::MemoryCopy( dataOut[ it ], dataIn[ i ], sizeof( float ) * dataIn.m_pointSize );
					++it;
				}
			}
		}

		Int32 GetPointsNum() const
		{
			return m_usedPoints.Size();
		}

		Int32 GetUsedPointsNum() const
		{
			return m_usedPoints.GetNumberOfUsedPoints();
		}

		Bool IsPointUsed( Int32 i ) const
		{
			return m_usedPoints.IsUsed( i );
		}
	};
}
