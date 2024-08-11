
#pragma once

#include "entityTemplateParams.h"
#include "skeletalAnimationSet.h"
#include "behaviorGraph.h"

class CAnimMimicParam : public CEntityTemplateParam
{
	DECLARE_ENGINE_CLASS( CAnimMimicParam, CEntityTemplateParam, 0 );

public:
	typedef TDynArray< THandle< CSkeletalAnimationSet > >	TAnimationSets;
	typedef TDynArray< SBehaviorGraphInstanceSlot >			TInstanceSlots;

protected:
	TAnimationSets		m_animationSets;
	TInstanceSlots		m_behaviorInstanceSlots;

public:
	CAnimMimicParam();
	CAnimMimicParam( const TDynArray< THandle< CSkeletalAnimationSet > >& sets, const TDynArray< SBehaviorGraphInstanceSlot >& instances );

public:
	const TAnimationSets& GetAnimsets() const;
	const TInstanceSlots& GetInstanceSlots() const;
};

BEGIN_CLASS_RTTI( CAnimMimicParam );
	PARENT_CLASS( CEntityTemplateParam );
	PROPERTY_EDIT( m_animationSets, TXT("Animations sets") );
	PROPERTY_EDIT( m_behaviorInstanceSlots, TXT("Behavior instance slots ( first slot is on the top )") );
END_CLASS_RTTI();
