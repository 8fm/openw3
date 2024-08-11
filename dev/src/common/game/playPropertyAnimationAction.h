/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "entityTargetingAction.h"

class CPlayPropertyAnimationAction : public IEntityTargetingAction
{
	DECLARE_ENGINE_CLASS( CPlayPropertyAnimationAction, IEntityTargetingAction, 0 )
protected:
	CName					m_animationName;
	Uint32					m_loopCount;
	Float					m_lengthScale;
	EPropertyCurveMode		m_mode;

	void					PerformOnEntity( CEntity* entity ) override;

public:
	CPlayPropertyAnimationAction()
		: m_loopCount( 1 )
		, m_lengthScale( 1.f )
		, m_mode( PCM_Forward )															{}

};


BEGIN_CLASS_RTTI( CPlayPropertyAnimationAction )
	PARENT_CLASS( IEntityTargetingAction )
	PROPERTY_EDIT( m_animationName, TXT("Animation name") )
	PROPERTY_EDIT( m_loopCount, TXT("Loops count. 0 == inifinite") )
	PROPERTY_EDIT( m_lengthScale, TXT("Indicates how much to scale the total animation time by") )
	PROPERTY_EDIT( m_mode, TXT("Playback mode") )
END_CLASS_RTTI()