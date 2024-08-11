/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Generic tick stats
struct STickGenericStats
{
	Uint32		m_statsCount;			//!< Number of ticked components
	Uint64		m_statsTime;			//!< Time it took to tick all components

	RED_INLINE STickGenericStats()
		: m_statsCount( 0 )
		, m_statsTime( 0 )
	{};

	//! Reset stats
	void Reset()
	{
		m_statsCount = 0;
		m_statsTime = 0;
	}
};