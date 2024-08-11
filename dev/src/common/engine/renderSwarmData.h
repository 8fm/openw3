/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "renderObject.h"

// the data that needs to be sent from engine to renderer per agent
class CSwarmBoidData
{
public:
	Vector3						m_position;
	EulerAngles					m_rotation;
	Float						m_scale;
	Float						m_distToCameraSquared;
	CName						m_previousAnimation;
	CName						m_currentAnimation;
	Uint32						m_animationInstance;
};

/// render side crowd data 
class IRenderSwarmData : public IRenderObject
{
public:
	virtual const CSwarmBoidData* GetWriteData() const = 0;
	virtual CSwarmBoidData* GetWriteData() = 0;
};
