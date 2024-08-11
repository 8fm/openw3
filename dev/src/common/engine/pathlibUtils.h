/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once




namespace PathLib
{

	namespace NavUtils
	{
		struct SBinSearchContext
		{
			SBinSearchContext( CAreaDescription* area, const Vector3& v, const Vector3& p0, const Vector3& p1, Float personalSpace, Float precision, Uint32 testFlags )
				: m_area(area), m_position(v), m_personalSpace(personalSpace), m_testFlags(testFlags)
			{
				Float dist = ( p1 - p0 ).Mag();
				m_iterationsCounter = 0;
				while ( dist > precision )
				{
					++m_iterationsCounter;
					dist *= 0.5f;
				}
			}
			SBinSearchContext& LimitIterations( Int32 iterationsLimit )
			{
				if ( m_iterationsCounter > iterationsLimit )
				{
					m_iterationsCounter = iterationsLimit;
				}
				return *this;
			}
			RED_INLINE Bool Accept(const Vector3& v)
			{
				CWideLineQueryData::MultiArea query( CWideLineQueryData( m_testFlags, m_position, v, m_personalSpace ) );
				return m_area->TMultiAreaQuery( query );
			}
			RED_INLINE Bool Stop(const Vector3& v1, const Vector3& v2)
			{
				return --m_iterationsCounter < 0;
			}
			CAreaDescription*	m_area;
			Vector3				m_position;
			Float				m_personalSpace;
			Int32				m_iterationsCounter;
			Uint32				m_testFlags;
		};
	};

};		// namespace PathLib