#include "build.h"
#include "getRiderOrMountHelper.h"
#include "ridingAiStorage.h"

void CGetRiderOrMountHelper::Initialise( CName riderTag )
{
	CEntity* riderEntity	= GGame->GetActiveWorld()->GetTagManager()->GetTaggedEntity( riderTag );
	m_riderEntity			= riderEntity;

	if ( riderEntity == nullptr || riderEntity->IsA<CActor>() == false )
	{
		return;
	}
	
	CActor *const riderActor				= static_cast<CActor*> ( riderEntity );
	CBehTreeMachine *const behTreeMachine	= riderActor->GetBehTreeMachine();
	if ( behTreeMachine == nullptr )
	{
		return;
	}

	CBehTreeInstance *const behTreeInstance = behTreeMachine->GetBehTreeInstance();
	if ( behTreeInstance == nullptr )
	{
		return;
	}
	m_riderExternalAIStorage = riderActor->GetScriptAiStorageData< CAIStorageRiderData >( CNAME( RiderData ) );			
}

CEntity *const CGetRiderOrMountHelper::GetEntity()const
{
	CAIStorageRiderData *const riderData	= m_riderExternalAIStorage.Get();
	if ( riderData == nullptr )
	{
		return m_riderEntity.Get();
	}

	CActor *const horseActor = riderData->m_sharedParams.Get()->m_horse;
	if ( horseActor && riderData->IsMounted() )
	{
		return horseActor;
	}
	return m_riderEntity.Get();
}
