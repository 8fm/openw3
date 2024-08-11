/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_POSE_PROVIDER_STATS_H_
#define _RED_POSE_PROVIDER_STATS_H_

struct SPoseProviderStats
{
	Uint32	m_memTotal;
	Uint32	m_memAlloc;
	Uint32	m_memCached;
	Uint32	m_memFreeAlloc;
	Uint32	m_memFreeCached;

	Uint32	m_numTotal;
	Uint32	m_numAlloc;
	Uint32	m_numCached;
	Uint32	m_numFreeAlloc;
	Uint32	m_numFreeCached;
};

#endif
