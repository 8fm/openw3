#include "build.h"
#include "createEntityHelper.h"

#include "../engine/dynamicLayer.h"
#include "../engine/jobSpawnEntity.h"

#include "actorsManager.h"
#include "commonGame.h"
#include "strayActorManager.h"
#include "entityPool.h"

//////////////////////////////////////////////////////////////
// CScriptSpawnEventHandler
CCreateEntityHelper::CScriptSpawnEventHandler::CScriptSpawnEventHandler( IScriptable *const caller, CName funcName )
	: m_caller( caller )
	, m_funcName( funcName )
{

}

void CCreateEntityHelper::CScriptSpawnEventHandler::OnPostAttach( CEntity* entity )
{
	CallOnPostSpawnCallback( entity );
}

void CCreateEntityHelper::CScriptSpawnEventHandler::CallOnPostSpawnCallback( CEntity* entity )
{
	IScriptable *const caller = m_caller.Get();
	if ( caller )
	{
		CallFunction( caller, m_funcName, THandle<CEntity>( entity ) );
	}
}

//////////////////////////////////////////////////////////////
// CCreateEntityHelper
IMPLEMENT_ENGINE_CLASS( CCreateEntityHelper );
CCreateEntityHelper::CCreateEntityHelper()
	: m_jobSpawnEntity( nullptr )
	, m_entity( nullptr )
	, m_scriptSpawnEventHandler( nullptr )
	, m_isCreating( false )
{
	EnableReferenceCounting();
}

CCreateEntityHelper::~CCreateEntityHelper()
{
	if ( m_scriptSpawnEventHandler )
	{
		delete m_scriptSpawnEventHandler;
		m_scriptSpawnEventHandler = nullptr;
	}

	if ( m_jobSpawnEntity )
	{
		m_jobSpawnEntity->Cancel();
		m_jobSpawnEntity->Release();
		m_jobSpawnEntity = nullptr;
	}
}

Bool  CCreateEntityHelper::IsCreating()const
{
	return m_isCreating;
}
void  CCreateEntityHelper::Reset()
{
	m_isCreating = false;
}

CEntity *const CCreateEntityHelper::GetCreatedEntity()
{
	return m_entity;
}

void CCreateEntityHelper::SetPostAttachedScriptCallback( IScriptable *const caller, CName funcName )
{
	if ( m_scriptSpawnEventHandler )
	{
		delete m_scriptSpawnEventHandler;
		m_scriptSpawnEventHandler = nullptr;
	}
	m_scriptSpawnEventHandler = new CScriptSpawnEventHandler( caller, funcName );
}

Bool CCreateEntityHelper::StartSpawnJob( EntitySpawnInfo&& entitySpawnInfo )
{
	if ( GGame->GetActiveWorld() == nullptr )
	{	
		return false;
	}
	OnPreSpawn( entitySpawnInfo );

	CEntityTemplate* entityTemplate = entitySpawnInfo.m_template.Get();
	if ( !entityTemplate )
	{
		return false;
	}

	CClass* templateType = nullptr;
	if ( CEntity* entityObject = entityTemplate->GetEntityObject() )
	{
		templateType = entityObject->GetClass();
	}

	IJobEntitySpawn * job = nullptr;
	if ( templateType && templateType->IsA< CActor >() )
	{
		job = GCommonGame->GetEntityPool()->SpawnEntity( std::move( entitySpawnInfo ), CEntityPool::SpawnType_Encounter );
	}
	else
	{
		job = GGame->GetActiveWorld()->GetDynamicLayer()->CreateEntityAsync( Move( entitySpawnInfo ) );
	}
	OnSpawnJobStarted( job );
	return job != nullptr;
}

void CCreateEntityHelper::OnPreSpawn( EntitySpawnInfo & entitySpawnInfo )
{
	if ( m_scriptSpawnEventHandler )
	{
		entitySpawnInfo.AddHandler( m_scriptSpawnEventHandler );
		m_scriptSpawnEventHandler = nullptr;
	}
}

void CCreateEntityHelper::OnSpawnJobStarted( IJobEntitySpawn* jobSpawnEntity )
{
	// same helper might be used for two jobs in a row
	m_entity			= nullptr;
	m_jobSpawnEntity	= jobSpawnEntity;
}

void CCreateEntityHelper::BeginProcessing()
{
	m_isCreating = true;
}

Bool CCreateEntityHelper::Update( CCreateEntityManager* manager )
{
	return UpdateSpawnJob();
}

void CCreateEntityHelper::Discard( CCreateEntityManager* manager )
{
	m_isCreating = false;
	if ( m_jobSpawnEntity )
	{
		m_jobSpawnEntity->Cancel();
	}
}


Bool CCreateEntityHelper::UpdateSpawnJob()
{
	ASSERT( m_jobSpawnEntity, TXT("Will not crash, please investigate") );
	if ( m_jobSpawnEntity && m_jobSpawnEntity->HasEnded() )
	{
		if ( m_jobSpawnEntity->HasFinishedWithoutErrors() )
		{
			if ( m_jobSpawnEntity->ValidateSpawn() )
			{
				OnSpawnJobFinished();
			}
			else
			{
				ERR_GAME( TXT("Unable to complete spawn job due to engine errors: probably layer got unloaded/deleted before job finished") );
			}
		}
		else
		{
			ERR_GAME( TXT("Unable to complete spawn job due to job error: probably entity template is corrupted or missing") );
		}

		return true;
	}
	return false;
}

void CCreateEntityHelper::OnSpawnJobFinished()
{
	// Link ! 
	m_jobSpawnEntity->LinkEntities();

	// Get
	m_entity = m_jobSpawnEntity->GetSpawnedEntity( 0 );

	// Spawn handler is now deleted
	m_scriptSpawnEventHandler = nullptr;

	// cleanup
	m_jobSpawnEntity->Release();
	m_jobSpawnEntity = nullptr;

	// Finally if the entity is an actor make it a stray actor automatically
	if ( m_entity->IsA< CActor >() )
	{
		GCommonGame->GetSystem< CStrayActorManager >()->ConvertToStrayActor( static_cast< CActor * >( m_entity ) );
	}
}

void CCreateEntityHelper::funcGetCreatedEntity( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetCreatedEntity() );
}

void CCreateEntityHelper::funcSetPostAttachedCallback( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< IScriptable >, callerHandle, nullptr );
	GET_PARAMETER( CName, funcName, CName() );
	FINISH_PARAMETERS;

	IScriptable *const caller = callerHandle.Get();
	if ( caller )
	{
		SetPostAttachedScriptCallback( caller, funcName );
	}
}

void CCreateEntityHelper::funcIsCreating( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( IsCreating() );
}
void CCreateEntityHelper::funcReset( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Reset();
}
