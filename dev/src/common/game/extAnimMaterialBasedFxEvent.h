/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CExtAnimMaterialBasedFxEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimMaterialBasedFxEvent )

public:
	CExtAnimMaterialBasedFxEvent();

	CExtAnimMaterialBasedFxEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName );
	virtual ~CExtAnimMaterialBasedFxEvent() {}

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
	virtual void ProcessPostponed( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

protected:
	CName	m_bone;
	Bool	m_vfxKickup;
	Bool	m_vfxFootstep;
};

BEGIN_CLASS_RTTI( CExtAnimMaterialBasedFxEvent )
	PARENT_CLASS( CExtAnimEvent )
	PROPERTY_CUSTOM_EDIT( m_bone, TXT( "Bone on which the FX is triggered" ), TXT( "EntityBoneList" ) )
	PROPERTY_EDIT( m_vfxKickup, TXT( "" ) )
	PROPERTY_EDIT( m_vfxFootstep, TXT( "" ) )
END_CLASS_RTTI()
