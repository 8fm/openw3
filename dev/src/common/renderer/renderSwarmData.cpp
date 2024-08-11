/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderSwarmData.h"

CRenderSwarmData::CRenderSwarmData( Uint32 numBoids )
	: m_numBoids( numBoids )
	, m_dataBuffer( numBoids * 2 )
{
}

CRenderSwarmData::~CRenderSwarmData()
{
}

const CSwarmBoidData* CRenderSwarmData::GetWriteData() const
{
	return (const CSwarmBoidData*)(&(m_dataBuffer[m_phase ? m_numBoids : 0]));
}

CSwarmBoidData* CRenderSwarmData::GetWriteData()
{
	return (CSwarmBoidData*)(&(m_dataBuffer[m_phase ? m_numBoids : 0]));
}

void CRenderSwarmData::TogglePhase()
{
	m_phase = !m_phase;
}
