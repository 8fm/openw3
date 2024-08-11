/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once 
#include "../engine/renderSwarmData.h"

/// Swarm data for rendering
class CRenderSwarmData : public IRenderSwarmData
{
protected:
	TDynArray< CSwarmBoidData, MC_SkinningData >	m_dataBuffer;	//!< lots lots of data
	Uint32											m_numBoids;		//!< Number of boids
	Bool											m_phase;		//!< Read/write phase

public:
	//! Get number of matrices
	RED_INLINE Uint32 GetNumBoids() const { return m_numBoids; }

	//! Get skinning data
	RED_INLINE CSwarmBoidData* GetReadData() { return &(m_dataBuffer[m_phase ? 0 : m_numBoids]); }
	virtual const CSwarmBoidData* GetWriteData() const;
	virtual CSwarmBoidData* GetWriteData();

	void TogglePhase();

public:
	CRenderSwarmData( Uint32 numBoids );
	virtual ~CRenderSwarmData();
};