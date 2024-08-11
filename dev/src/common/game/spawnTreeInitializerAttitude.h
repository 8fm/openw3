#pragma once

#include "2daProperties.h"
#include "spawnTreeInitializer.h"


class CSpawnTreeInitializerAttitude : public ISpawnTreeInitializer
#ifndef NO_EDITOR
	, public CAttitude2dPropertyOwner
#endif
{
	DECLARE_ENGINE_CLASS( CSpawnTreeInitializerAttitude, ISpawnTreeInitializer, 0 );
protected:
	CName			m_attitudeGroup;
	static Int32	s_attitudeGroupPriority;

	static Int32	GetAttitudeGroupPriority();
public:
	ISpawnTreeInitializer::EOutput	Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const override;
	void							Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const override;
	String							GetBlockCaption() const override;
	String							GetEditorFriendlyName() const override;
};

BEGIN_CLASS_RTTI( CSpawnTreeInitializerAttitude );
	PARENT_CLASS( ISpawnTreeInitializer );
	PROPERTY_CUSTOM_EDIT( m_attitudeGroup, TXT( "Attitude group" ), TXT( "2daValueSelection" ) );
END_CLASS_RTTI();
