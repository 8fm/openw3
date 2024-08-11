#pragma once

#include "spawnTreeInitializer.h"

class CSpawnTreeInitializerRainHandler : public ISpawnTreeInitializer
{
	DECLARE_ENGINE_CLASS( CSpawnTreeInitializerRainHandler, ISpawnTreeInitializer, 0 );
protected:
	Float					m_ratioWhenRaining;
	TInstanceVar< Bool >	i_isRaining;	

public:
	CSpawnTreeInitializerRainHandler()
		: m_ratioWhenRaining( 0.6f )										{}
	
	void				OnEvent( const CBaseCreatureEntry* const  entry, CSpawnTreeInstance& instance, CName eventName, CSpawnTreeInstance* entryBuffer ) const override;
	String				GetBlockCaption() const override;
	String				GetEditorFriendlyName() const override;	
	void				UpdateEntrySetup( const CBaseCreatureEntry* const  entry, CSpawnTreeInstance& instance, SSpawnTreeEntrySetup& setup ) const override;
	
	void				OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void				OnInitData( CSpawnTreeInstance& instance  ) override;

	Bool				IsSpawnableOnPartyMembers() const;

	EOutput				Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason )	const override;
	void				Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry )const override;
	void				OnCreatureRemoval( CSpawnTreeInstance& instance, CActor* actor, ESpawnTreeCreatureRemovalReason removalReason, CBaseCreatureEntry* entry ) const override;
};

BEGIN_CLASS_RTTI( CSpawnTreeInitializerRainHandler );
	PARENT_CLASS( ISpawnTreeInitializer );
	PROPERTY_EDIT( m_ratioWhenRaining, TXT("How many % [ 0 - 1 ] of npcs should stay when its raining") );
END_CLASS_RTTI();
