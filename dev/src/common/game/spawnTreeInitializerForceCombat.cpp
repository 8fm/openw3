#include "build.h"
#include "spawnTreeInitializerForceCombat.h"
#include "actor.h"
#include "behTreeMachine.h"
#include "behTreeInstance.h"
#include "../core/dataError.h"
#include "../engine/tagManager.h"
#include "../engine/utils.h"

IMPLEMENT_ENGINE_CLASS( CSpawnTreeInitializerForceCombat );

////////////////////////////////////////////////////////////////////
// CSpawnTreeInitializerForceCombat
////////////////////////////////////////////////////////////////////
ISpawnTreeInitializer::EOutput CSpawnTreeInitializerForceCombat::Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const
{
	CActor* targetActor = NULL;
	if( !m_targetTag.Empty() )
	{
		targetActor = Cast< CActor >( GGame->GetActiveWorld()->GetTagManager()->GetTaggedEntity( m_targetTag ) );
	}

	if( targetActor )
	{
		if( !actor )
		{
			return OUTPUT_FAILED;
		}

		auto* behTreeMachine = actor->GetBehTreeMachine();
		if( !behTreeMachine )
		{
			DATA_HALT( DES_Major, CResourceObtainer::GetResource( actor ), TXT( "BehTree" ), TXT( "AI for this actor is not defined" ) );
			return OUTPUT_FAILED;
		}

		auto* behTreeInstance = behTreeMachine->GetBehTreeInstance();
		if( !behTreeInstance )
		{
			return OUTPUT_FAILED;
		}

		behTreeInstance->SetCombatTarget( targetActor );
	}

	return OUTPUT_SUCCESS;
}
void CSpawnTreeInitializerForceCombat::Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const
{
}
String CSpawnTreeInitializerForceCombat::GetBlockCaption() const
{
	return String::Printf( TXT("Force combat on '%ls'"), m_targetTag.AsString().AsChar() );
}
String CSpawnTreeInitializerForceCombat::GetEditorFriendlyName() const
{
	static String STR( TXT("ForceCombat") );
	return STR;
}
