#pragma once

#include "spawnTreeInitializer.h"


class CSpawnTreeInitializerForceCombat : public ISpawnTreeInitializer
{
	DECLARE_ENGINE_CLASS( CSpawnTreeInitializerForceCombat, ISpawnTreeInitializer, 0 );
protected:
	CName			m_targetTag;
public:
	EOutput							Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const override;
	void							Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const override;
	String							GetBlockCaption() const override;
	String							GetEditorFriendlyName() const override;
};

BEGIN_CLASS_RTTI( CSpawnTreeInitializerForceCombat );
PARENT_CLASS( ISpawnTreeInitializer );
PROPERTY_EDIT( m_targetTag, TXT( "Tag for the target to be forced for combat" ) );
END_CLASS_RTTI();
