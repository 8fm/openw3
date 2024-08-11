#include "build.h"
#include "spawnTreeInitializerRainHandler.h"
#include "spawnTreeBaseEntry.h"
#include "commonGame.h"
#include "behTreeReactionManager.h"

#include "../core/instanceDataLayoutCompiler.h"

IMPLEMENT_ENGINE_CLASS( CSpawnTreeInitializerRainHandler );


void CSpawnTreeInitializerRainHandler::OnEvent( const CBaseCreatureEntry* const entry, CSpawnTreeInstance& instance, CName eventName, CSpawnTreeInstance* entryBuffer ) const
{
	Bool update = false;
	Bool& isRaining = instance[ i_isRaining ];

	if( eventName == CNAME( RainStarted) )
	{
		if ( !isRaining )
		{
			isRaining = true;
			update = true;
		}
	}	
	else if( eventName == CNAME( RainEnded ) )
	{
		if ( isRaining )
		{
			isRaining = false;
			update = true;
		}
	}

	if ( update )
	{
		entry->UpdateSetup( *entryBuffer );
	}
	
	TBaseClass::OnEvent( entry, instance, eventName, entryBuffer );
}

void CSpawnTreeInitializerRainHandler::UpdateEntrySetup( const CBaseCreatureEntry* const  entry, CSpawnTreeInstance& instance, SSpawnTreeEntrySetup& setup ) const
{
	if( instance[ i_isRaining] )
	{
		setup.m_spawnRatio *= m_ratioWhenRaining;
	}
}

void CSpawnTreeInitializerRainHandler::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler <<	i_isRaining;
}

void CSpawnTreeInitializerRainHandler::OnInitData( CSpawnTreeInstance& instance )
{
	TBaseClass::OnInitData( instance );
	
	instance[ i_isRaining ] = false;
}
String CSpawnTreeInitializerRainHandler::GetBlockCaption() const
{	
	return TXT("Rain handler");	
}	

String CSpawnTreeInitializerRainHandler::GetEditorFriendlyName() const
{
	static String STR( TXT("Rain Handler") );
	return STR;
}

Bool CSpawnTreeInitializerRainHandler::IsSpawnableOnPartyMembers() const
{
	return false;
}

ISpawnTreeInitializer::EOutput CSpawnTreeInitializerRainHandler::Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason )	const 
{
	GCommonGame->GetBehTreeReactionManager()->AddRainAwareNPC( Cast< CNewNPC >( actor) );
	return TBaseClass::Activate( actor, instance, entry, reason );
}

void CSpawnTreeInitializerRainHandler::Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const
{
	GCommonGame->GetBehTreeReactionManager()->RemoveRainAwareNPC( Cast< CNewNPC >( actor) );
	TBaseClass::Deactivate( actor, instance, entry );
}

void CSpawnTreeInitializerRainHandler::OnCreatureRemoval( CSpawnTreeInstance& instance, CActor* actor, ESpawnTreeCreatureRemovalReason removalReason, CBaseCreatureEntry* entry ) const
{
	GCommonGame->GetBehTreeReactionManager()->RemoveRainAwareNPC( Cast< CNewNPC >( actor) );
	TBaseClass::OnCreatureRemoval( instance, actor, removalReason,entry );
}