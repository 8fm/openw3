/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "motionExtractionCompression.h"
#include "comprUtilsDataBuffer.h"
#include "comprUtilsLine.h"
#include "motionExtraction.h"

IMPLEMENT_ENGINE_CLASS( IMotionExtractionCompression );
IMPLEMENT_ENGINE_CLASS( CMotionExtractionLineCompression );
IMPLEMENT_ENGINE_CLASS( CMotionExtractionLineCompression2 );

CMotionExtractionLineCompression::CMotionExtractionLineCompression()
	: m_eps( 0.03f )
	, m_minKnots( 2 )
{

}

void CMotionExtractionLineCompression::SetMinKnots( Uint32 knots )
{
	m_minKnots = knots;
}

void CMotionExtractionLineCompression::SetEps( Float eps )
{
	m_eps = eps;
}

IMotionExtraction* CMotionExtractionLineCompression::Compress( const CUncompressedMotionExtraction *motionToCompress ) const
{
	CLineMotionExtraction* lineME = NULL;

	const TDynArray< Vector >& frames = motionToCompress->GetUncompressedFrames();

	const Float fps = 30.f;
	const Float timeDelta = 1.f / fps;

	AnimCompressionUtils::DataBuffer inputBuff;
	inputBuff.Create( frames.Size(), 5 );

	for ( Uint32 i=0; i<frames.Size(); ++i )
	{
		Float time = timeDelta * i;
		const Vector& frame = frames[ i ];

		inputBuff[ i ][ 0 ] = time;
		inputBuff[ i ][ 1 ] = frame.A[ 0 ];
		inputBuff[ i ][ 2 ] = frame.A[ 1 ];
		inputBuff[ i ][ 3 ] = frame.A[ 2 ];
		inputBuff[ i ][ 4 ] = frame.A[ 3 ];
	}

	AnimCompressionUtils::LineCompression::CompressionParams params;
	params.m_eps = m_eps;
	params.m_minKnots = m_minKnots;

	AnimCompressionUtils::LineCompression compression;
	if ( compression.Compress( params, inputBuff ) )
	{
		lineME = new CLineMotionExtraction;

		// Fill motion extraction data
		Int32 numPoints = compression.GetPointsNum();
		Int32 numUsedPoints = compression.GetUsedPointsNum();
		Int32 pointSize = inputBuff.m_pointSize - 1;

		if ( numPoints > NumericLimits< Uint16 >::Max() )
		{
			ASSERT( numPoints <= NumericLimits< Uint16 >::Max() );

			// Clean up
			inputBuff.Clear();

			return NULL;
		}

		if ( pointSize > NumericLimits< Uint8 >::Max() )
		{
			ASSERT( pointSize <= NumericLimits< Uint8 >::Max() );

			// Clean up
			inputBuff.Clear();

			return NULL;
		}

		TDynArray< Vector > compressedFrames;
		TDynArray< Float > times;

		compressedFrames.Resize( numUsedPoints );
		times.Resize( numUsedPoints );

		// Fill keys and times
		Int32 curr = 0;
		for ( Int32 i=0; i<numPoints; ++i )
		{
			if ( compression.IsPointUsed( i ) )
			{
				const Float t = inputBuff[ i ][ 0 ];

				times[ curr ] = t;

				// Frame
				Vector& f = compressedFrames[ curr ];
				f.A[ 0 ] = inputBuff[ i ][ 1 ];
				f.A[ 1 ] = inputBuff[ i ][ 2 ];
				f.A[ 2 ] = inputBuff[ i ][ 3 ];
				f.A[ 3 ] = inputBuff[ i ][ 4 ];

				++curr;
			}
		}
		ASSERT( curr == numUsedPoints );

		lineME->Initialize( compressedFrames, times );
	}

	inputBuff.Clear();

	Uint32 temp1 = motionToCompress->GetDataSize();
	Uint32 temp2 = lineME->GetDataSize();

	if ( GIsEditor )
	{
		LOG_ENGINE( TXT("Motion extraction line compression ratio: %.2f"), (Float)(temp1/temp2) );
	}

	return lineME;
}

//////////////////////////////////////////////////////////////////////////

CMotionExtractionLineCompression2::CMotionExtractionLineCompression2()
	: m_eps( 0.03f )
	, m_minKnots( 2 )
	, m_maxKnotsDistance( 255 )
{

}

void CMotionExtractionLineCompression2::SetMinKnots( Uint32 knots )
{
	m_minKnots = knots;
}

void CMotionExtractionLineCompression2::SetEps( Float eps )
{
	m_eps = eps;
}

IMotionExtraction* CMotionExtractionLineCompression2::Compress( const CUncompressedMotionExtraction *motionToCompress ) const
{
	CLineMotionExtraction2* lineME = NULL;

	//-------------------------------------------------------------
	// 1. Prepare uncompressed data

	// Duration and frames
	Float duration = motionToCompress->GetDuration();
	const TDynArray< Vector >& frames = motionToCompress->GetUncompressedFrames();

	// Calc size and flags
	Uint32 pointSize = 0;
	Uint8 flags = 0;
	CalcFlags( frames, flags, pointSize );

	if ( pointSize == 0 )
	{
		// Nothing to compress
		return NULL;
	}

	pointSize++; // Time
	ASSERT( pointSize <= 5 );

	// Map
	Uint32 paramMap[ 4 ];
	{
		Uint32 paramMapTemp = 0;

		if ( 0 != ( flags & LMEF_X ) )
		{
			paramMap[ paramMapTemp++ ] = 0;
		}
		if ( 0 != ( flags & LMEF_Y ) )
		{
			paramMap[ paramMapTemp++ ] = 1;
		}
		if ( 0 != ( flags & LMEF_Z ) )
		{
			paramMap[ paramMapTemp++ ] = 2;
		}
		if ( 0 != ( flags & LMEF_R ) )
		{
			paramMap[ paramMapTemp++ ] = 3;
		}

		ASSERT( paramMapTemp == pointSize - 1 );
	}

	// Create input buffer
	AnimCompressionUtils::DataBuffer inputBuff;
	inputBuff.Create( frames.Size(), pointSize );

	// Uncompressed time slice
	Float timeSlice = duration / (Float)( frames.Size() - 1 );

	Float time = 0.f;

	// Fill input buffer
	for ( Uint32 i=0; i<frames.Size(); ++i )
	{
		time = timeSlice * (Float)i;
		const Vector& frame = frames[ i ];

		inputBuff[ i ][ 0 ] = time;

		for ( Uint32 j=1; j<pointSize; j++ )
		{
			inputBuff[ i ][ j ] = frame.A[ paramMap[ j - 1 ] ];
		}
	}
	ASSERT( MAbs( time - duration ) < 0.01f );

	//-------------------------------------------------------------
	// 2. Compress
	AnimCompressionUtils::LineCompression::CompressionParams params;
	params.m_eps = m_eps;
	params.m_minKnots = m_minKnots;
	params.m_maxKnotDistance = m_maxKnotsDistance;

	AnimCompressionUtils::LineCompression compression;
	if ( compression.Compress( params, inputBuff ) )
	{
		//-------------------------------------------------------------
		// 3. Setup output data
		lineME = new CLineMotionExtraction2;

		// Fill motion extraction data
		Int32 numPoints = compression.GetPointsNum();
		Int32 numFrames = compression.GetUsedPointsNum();
		Int32 linePointSize = pointSize - 1; // Time

		ASSERT( numFrames >= 2 );

		// Setup line object
		//lineME->m_timeSlice = timeSlice;
		lineME->m_flags = flags;
		lineME->m_deltaTimes.Resize( numFrames - 1 );
		lineME->m_frames.Resize( numFrames * linePointSize );

		// Fill keys and times
		Int32 curr = 0;
		Int32 prevPoint = 0;
		for ( Int32 i=0; i<numPoints; ++i )
		{
			if ( compression.IsPointUsed( i ) )
			{
				// Write to line object

				// Time
				if ( i != 0 )
				{
					Int32 deltaTime = i - prevPoint;
					ASSERT( deltaTime >= 0 && deltaTime <= 255 );
					lineME->m_deltaTimes[ curr - 1 ] = (Uint8)deltaTime;
				}

				// Frame
				for ( Int32 k=0; k<linePointSize; ++k )
				{
					lineME->m_frames[ curr * linePointSize + k ] = inputBuff[ i ][ 1 + k ];
				}

				prevPoint = i;

				++curr;
				ASSERT( curr <= numFrames );
			}
		}
		ASSERT( curr == numFrames );
	}

	inputBuff.Clear();

	Uint32 temp1 = motionToCompress->GetDataSize();
	Uint32 temp2 = lineME->GetDataSize();

	if ( GIsEditor )
	{
		LOG_ENGINE( TXT("Motion extraction line compression 2 ratio: %.2f"), (Float)(temp1/temp2) );
	}

	// Done :)
	return lineME;
}

void CMotionExtractionLineCompression2::CalcFlags( const TDynArray< Vector >& frames, Uint8& flags, Uint32& size ) const
{
	flags = 0;
	size = 0;

	static const Float vecEps = 0.01f;
	static const Float rotEps = 0.33f;

	for ( Uint32 i=0; i<frames.Size(); ++i )
	{
		const Vector& frame = frames[ i ];

		// X
		if ( 0 == ( flags & LMEF_X ) && MAbs( frame.A[ 0 ] ) > vecEps )
		{
			flags |= LMEF_X;
			size++;
		}

		// Y
		if ( 0 == ( flags & LMEF_Y ) && MAbs( frame.A[ 1 ] ) > vecEps )
		{
			flags |= LMEF_Y;
			size++;
		}

		// Z
		if ( 0 == ( flags & LMEF_Z ) && MAbs( frame.A[ 2 ] ) > vecEps )
		{
			flags |= LMEF_Z;
			size++;
		}

		// Yaw
		if ( 0 == ( flags & LMEF_R ) && MAbs( frame.A[ 3 ] ) > rotEps )
		{
			flags |= LMEF_R;
			size++;
		}
	}
}
