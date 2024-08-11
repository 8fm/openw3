/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "spawnTreeInitializerMonitor.h"


class CSpawnTreeInitializerSpawnLimitMonitor : public ISpawnTreeSpawnMonitorBaseInitializer
{
	DECLARE_ENGINE_CLASS( CSpawnTreeInitializerSpawnLimitMonitor, ISpawnTreeSpawnMonitorBaseInitializer, 0 )
protected:
	Uint16								m_totalSpawnLimitMin;
	Uint16								m_totalSpawnLimitMax;
	CName								m_creatureDefinition;
	Bool								m_resetOnFullRespawn;

	TInstanceVar< Int16 >				i_spawnLimit;
	TInstanceVar< Int16 >				i_spawnedNow;
	TInstanceVar< Int16 >				i_deadCount;
	TInstanceVar< Bool >				i_isTriggered;

	Int16					RandomizeSpawnLimit() const								{ return m_totalSpawnLimitMin + Int16( Red::Math::MRound( Float( m_totalSpawnLimitMax - m_totalSpawnLimitMin ) * GEngine->GetRandomNumberGenerator().Get< Float >() ) ); }

	void					MonitorCreature( CSpawnTreeInstance& instance, CActor* actor, CBaseCreatureEntry* entry, ESpawnMonitorEvent eventType ) const override;
public:
	CSpawnTreeInitializerSpawnLimitMonitor()
		: m_totalSpawnLimitMin( 15 )
		, m_totalSpawnLimitMax( 15 )
		, m_resetOnFullRespawn( true )												{}

	void					UpdateEntrySetup( const CBaseCreatureEntry* const  entry, CSpawnTreeInstance& instance, SSpawnTreeEntrySetup& setup ) const override;
	void					OnFullRespawn( CSpawnTreeInstance& instance ) const override;

	String					GetEditorFriendlyName() const override;
	String					GetBlockDebugCaption( const CSpawnTreeInstance& instance ) const;

	void					OnPropertyPostChange( IProperty* property ) override;

	// Instance buffer interface
	void					OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void					OnInitData( CSpawnTreeInstance& instance ) override;

	// Save system support
	Bool					IsStateSaving( CSpawnTreeInstance& instance ) const override;
	void					SaveState( CSpawnTreeInstance& instance, IGameSaver* writer ) const override;
	void					LoadState( CSpawnTreeInstance& instance, IGameLoader* reader ) const override;
};

BEGIN_CLASS_RTTI( CSpawnTreeInitializerSpawnLimitMonitor )
	PARENT_CLASS( ISpawnTreeSpawnMonitorBaseInitializer )
	PROPERTY_EDIT( m_totalSpawnLimitMin, TXT("Total spawn limit minimum (inclusive) boundings") )
	PROPERTY_EDIT( m_totalSpawnLimitMax, TXT("Total spawn limit maximum (inclusive) boundings") )
	PROPERTY_CUSTOM_EDIT( m_creatureDefinition, TXT("Limit monitor to given creature definition. Leave empty for no filtering."), TXT("CreatureDefinitionsEditor") )
	PROPERTY_EDIT( m_resetOnFullRespawn, TXT("Reset spawn limit on encounter full respawn") )
END_CLASS_RTTI()