
#include "build.h"
#include "dialogEditorActorsProvider.h"
#include "dialogEditor.h"

#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneCutsceneSection.h"
#include "../../common/game/storySceneItems.h"
#include "../../common/engine/particleSystem.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/particleComponent.h"
#include "../../common/engine/spotLightComponent.h"
#include "../../common/engine/pointLightComponent.h"
#include "../../common/engine/jobSpawnEntity.h"
#include "../../common/engine/dimmerComponent.h"

IMPLEMENT_ENGINE_CLASS( CEdSceneActorsProvider );

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

CEdSceneActorsProvider::CEdSceneActorsProvider()
	: m_editor( NULL )
	, m_syncMode( true )
{
}

void CEdSceneActorsProvider::Init( const CEdSceneEditor* editor, const CStoryScene* scene )
{
	m_editor = editor;

	Refresh( scene );
}

Bool CEdSceneActorsProvider::AreAllActorsReady() const
{
	return m_entitiesJob.Size() == 0;
}

void CEdSceneActorsProvider::ProcessSpawningActors()
{
	for ( Int32 i=m_entitiesJob.SizeInt()-1; i>=0; --i )
	{
		CJobSpawnEntity* job = m_entitiesJob[ i ];
		if ( job->HasEnded() )
		{
			if ( job->HasFinishedWithoutErrors() )
			{
				CEntity* entity = job->GetSpawnedEntity();
				if ( entity )
				{
					entity->SetHideInGame( true, true, CEntity::HR_Scene );

					m_entities.PushBack( entity );
				}
			}

			job->Release();
			
			m_entitiesJob.RemoveAtFast( i );
			m_entitiesJobData.RemoveAtFast( i );

			SCENE_ASSERT( m_entitiesJob.Size() == m_entitiesJobData.Size() );
		}
	}
}

void CEdSceneActorsProvider::Rebuild( const CStoryScene* s )
{
	CWorld* world = m_editor->GetWorld();

	// remove entities (actors)
	{
		const Uint32 size = m_entities.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			CEntity* ent = m_entities[ i ].Get();
			if ( ent && !m_externalEntities.Exist( ent ) )
			{
				world->UnignoreEntityStreaming( ent );
				ent->Destroy();
			}
		}
		m_entities.Clear();
	}

	// remove props
	{
		const Uint32 size = m_props.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			CEntity* ent = m_props[ i ].Get();
			if ( ent && !m_externalProps.Exist( ent ) )
			{
				world->UnignoreEntityStreaming( ent );
				ent->Destroy();
			}
		}
		m_props.Clear();
	}

	// remove effects
	{
		const Uint32 size = m_effects.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			CEntity* ent = m_effects[ i ].Get();
			if ( ent )
			{
				ent->Destroy();
			}
		}
		m_effects.Clear();
	}

	// remove lights
	{
		const Uint32 size = m_lights.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			CEntity* ent = m_lights[ i ].Get();
			if ( ent )
			{
				ent->Destroy();
			}
		}
		m_lights.Clear();
	}

	AddExtraActors();

	Refresh( s );
}

void CEdSceneActorsProvider::Refresh( const CStoryScene* scene )
{
	CWorld* world = m_editor->GetWorld();

	{
		const TDynArray< CStorySceneDialogsetInstance* >& ds = scene->GetDialogsetInstances();

		const Uint32 size = ds.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const CStorySceneDialogsetInstance* set = ds[ i ];
			if ( !set )
			{
				SCENE_ASSERT( set );
				continue;
			}

			const TDynArray< CStorySceneDialogsetSlot* >& dialogsetSlots = set->GetSlots();
			for ( Uint32 j = 0; j < dialogsetSlots.Size(); ++j )
			{
				if ( dialogsetSlots[ j ] )
				{
					const CName& actorVoicetag = dialogsetSlots[ j ]->GetActorName();
					if ( actorVoicetag )
					{
						THandle< CEntityTemplate > templ = NULL;
						CName app = CName::NONE;
						Bool searchByVT = false;

						if ( scene->GetActorSpawnDefinition( actorVoicetag, templ, app, searchByVT ) )
						{
							const CStorySceneActor* exActor = scene->GetActorDescriptionForVoicetag( actorVoicetag );
							SCENE_ASSERT( exActor );

							if ( exActor && !HasActor( actorVoicetag ) )
							{
								EntitySpawnInfo spawnInfo;
								spawnInfo.m_template = templ;
								spawnInfo.m_appearances.PushBack( app );
								spawnInfo.m_tags = exActor->m_actorTags;
								spawnInfo.m_previewOnly = true;

								OverrideSpawnClass( spawnInfo );

								SpawnEntity( world, Move( spawnInfo ), actorVoicetag );
							}
						}
						else
						{
							// Should be never here
							SCENE_ASSERT( 0 );
						}
					}
				}
			}
		}
	}

	{
		const Uint32 size = scene->GetNumberOfSections();
		for ( Uint32 i=0; i<size; ++i )
		{
			const CStorySceneSection* s = scene->GetSection( i );

			if ( const CStorySceneCutsceneSection* cs = Cast< const CStorySceneCutsceneSection >( s ) )
			{
				const CCutsceneTemplate* csTempl = cs->GetCsTemplate();
				if ( csTempl )
				{
					TDynArray< String > names;
					csTempl->GetActorsName( names );
					
					const Uint32 aSize = names.Size();
					for ( Uint32 j=0; j<aSize; ++j )
					{
						const SCutsceneActorDef* def = csTempl->GetActorDefinition( names[ j ] );
						{
							if ( def && def->m_type == CAT_Actor && def->m_template.Get() && !HasEntity( def->m_voiceTag ) )
							{
								EntitySpawnInfo spawnInfo;
								spawnInfo.m_template = def->m_template.Get();
								spawnInfo.m_appearances.PushBack( def->m_appearance );
								spawnInfo.m_tags = def->m_tag;
								spawnInfo.m_name = def->m_name;
								spawnInfo.m_previewOnly = true;

								OverrideSpawnClass( spawnInfo );

								SpawnEntity( world, Move( spawnInfo ), def->m_voiceTag );
							}
						}
					}
				}
			}
		}
	}

	if ( scene->GetDialogsetInstances().Size() == 0 )
	{
		const TDynArray< CStorySceneActor* >& actorsDef = scene->GetSceneActorsDefinitions();
		const Uint32 size = actorsDef.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const CStorySceneActor* aDef = actorsDef[ i ];
			if( aDef )
			{
				EntitySpawnInfo spawnInfo;
				spawnInfo.m_template = aDef->m_entityTemplate.Get();
				spawnInfo.m_appearances.PushBack( aDef->m_appearanceFilter );
				spawnInfo.m_tags = aDef->m_actorTags;
				spawnInfo.m_previewOnly = true;

				OverrideSpawnClass( spawnInfo );

				SpawnEntity( world, Move( spawnInfo ), aDef->m_id );
			}
		}
	}

	// spawn props
	{
		const TDynArray< CStorySceneProp* >& propDef = scene->GetScenePropDefinitions();
		const Uint32 size = propDef.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const CStorySceneProp* aDef = propDef[ i ];
			if( aDef && aDef->m_id != CName::NONE && aDef->m_entityTemplate.Get() )
			{
				EntitySpawnInfo spawnInfo;
				spawnInfo.m_template = aDef->m_entityTemplate.Get();
				spawnInfo.m_name = aDef->m_id.AsString();
				spawnInfo.m_previewOnly = true;

				// TODO: async load?				
				if( CEntity* entity = world->GetDynamicLayer()->CreateEntitySync( spawnInfo ) )
				{
					entity->SetHideInGame( true, false, CEntity::HR_Scene );

					world->IgnoreEntityStreaming( entity );

					m_props.PushBack( entity );
				}				
			}
		}
	}

	// spawn effects
	{
		const TDynArray< CStorySceneEffect* >& fxDef = scene->GetSceneEffectDefinitions();
		const Uint32 size = fxDef.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const CStorySceneEffect* aDef = fxDef[ i ];
			if( !aDef || aDef->m_id == CName::NONE )
			{
				continue;
			}

			if( CParticleSystem* ps = aDef->m_particleSystem.Get() )
			{
				EntitySpawnInfo spawnInfo;
				spawnInfo.m_name = aDef->m_id.AsString();
				spawnInfo.m_previewOnly = true;

				// TODO: async load?
				CEntity* entity = world->GetDynamicLayer()->CreateEntitySync( spawnInfo );

				// add particle effect component
				CParticleComponent* pc = Cast< CParticleComponent >( entity->CreateComponent( ClassID< CParticleComponent >(), SComponentSpawnInfo() ) );
				pc->ForceUpdateTransformNodeAndCommitChanges();
				pc->SetParticleSystem( ps );
				pc->RefreshRenderProxies();

				m_effects.PushBack( entity );
			}
		}
	}

	// spawn lights
	RefreshLights( scene );

	InitializeActorsItems();

	WaitForAllActorsItems();
}

Bool CEdSceneActorsProvider::RebuildLights( const CStoryScene* s )
{
	// remove lights
	{
		const Uint32 size = m_lights.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			CEntity* ent = m_lights[ i ].Get();
			if ( ent )
			{
				ent->Destroy();
			}
		}
		m_lights.Clear();
	}

	return RefreshLights( s );
}

Bool CEdSceneActorsProvider::RefreshLights( const CStoryScene* scene )
{
	CWorld* world = m_editor->GetWorld();

	TDynArray< String > usedLights;
	Bool wasAnyLightSpawned = false;

	const TDynArray< CStorySceneLight* >& lightDef = scene->GetSceneLightDefinitions();
	const Uint32 size = lightDef.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const CStorySceneLight* aDef = lightDef[ i ];
		if( !aDef || aDef->m_id == CName::NONE )
		{
			continue;
		}

		const String lightEntityName = aDef->m_id.AsString();
		usedLights.PushBack( lightEntityName );

		if ( RefreshLight( lightEntityName, aDef->m_type ) )
		{			
			continue;
		}

		EntitySpawnInfo spawnInfo;
		spawnInfo.m_name = lightEntityName;
		spawnInfo.m_previewOnly = true;

		EngineTransform sceneToWorld = m_editor->OnActorsProvider_GetSceneToWorld();
		Matrix sceneToWorldMat;
		sceneToWorld.CalcLocalToWorld( sceneToWorldMat );
		
		const Vector spawnPosSS = m_editor->OnActorsProvider_CalcLightSpawnPositionSS();
		const Vector spawnPosWS = sceneToWorldMat.TransformPoint( spawnPosSS );
		spawnInfo.m_spawnPosition = spawnPosWS;

		CEntity* entity = world->GetDynamicLayer()->CreateEntitySync( spawnInfo );
		if( !entity )
		{
			SCENE_ASSERT( entity );
			continue;
		}

		wasAnyLightSpawned = true;

		// add light component
		CLightComponent* light = nullptr;
		CDimmerComponent* dimmer = nullptr;
		switch( aDef->m_type )
		{
		case LT_SpotLight:
			{
				CSpotLightComponent* spotlight = Cast< CSpotLightComponent >( entity->CreateComponent( ClassID< CSpotLightComponent >(), SComponentSpawnInfo() ) );
				spotlight->SetInnerAngle( aDef->m_innerAngle );
				spotlight->SetOuterAngle( aDef->m_outerAngle );
				spotlight->SetSoftness( aDef->m_softness );
				light = spotlight;
			}
			break;

		case LT_PointLight:
			{
				light = Cast< CLightComponent >( entity->CreateComponent( ClassID< CPointLightComponent >(), SComponentSpawnInfo() ) );
			}
			break;			
		case LT_Dimmer:
			{
				dimmer = Cast< CDimmerComponent >( entity->CreateComponent( ClassID< CDimmerComponent >(), SComponentSpawnInfo() ) );			
			}
			break;
		}

		if ( light )
		{
			light->SetEnabled( false );			
			light->SetShadowCastingMode( aDef->m_shadowCastingMode );
			light->SetShadowFadeDistance( aDef->m_shadowFadeDistance );
			light->SetShadowFadeRange( aDef->m_shadowFadeRange );
			light->RefreshRenderProxies();
		}
		else if( dimmer )
		{
			dimmer->SetEnabled( false );			
			dimmer->SetDimmerType( aDef->m_dimmerType );
			dimmer->SetAreaMarker( aDef->m_dimmerAreaMarker );
			dimmer->RefreshRenderProxies();
		}

		entity->ForceUpdateTransformNodeAndCommitChanges();
		entity->ForceUpdateBoundsNode();

		m_lights.PushBack( entity );
	}

	Bool wasRemovedSomething = false;

	for ( Int32 i=m_lights.SizeInt()-1; i>=0; --i )
	{
		CEntity* l = m_lights[ i ].Get();

		Bool toRemove = true;

		if ( l )
		{
			const String& lightName = l->GetName();
			if ( usedLights.Exist( lightName ) )
			{
				toRemove = false;
			}
		}

		if ( toRemove )
		{
			m_lights.RemoveAtFast( i );
			wasRemovedSomething = true;
		}
	}

	return wasAnyLightSpawned || wasRemovedSomething;
}

CEntity* CEdSceneActorsProvider::FindActorById( CName id )
{
	const Uint32 num = m_entities.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		if ( CActor* a = Cast< CActor >( m_entities[ i ].Get() ) )
		{
			if ( a->GetVoiceTag() == id )
			{
				return a;
			}
		}
	}
	return nullptr;
}

CEntity* CEdSceneActorsProvider::FindPropById( CName id )
{
	const Uint32 num = m_props.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		if ( CEntity* e = m_props[ i ].Get() )
		{
			CName name( e->GetName() );
			if ( name == id )
			{
				return e;
			}
		}
	}
	return nullptr;
}

CEntity* CEdSceneActorsProvider::FindEffectById( CName id )
{
	const Uint32 num = m_effects.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		if ( CEntity* e = m_effects[ i ].Get() )
		{
			CName name( e->GetName() );
			if ( name == id )
			{
				return e;
			}
		}
	}
	return nullptr;
}

CEntity* CEdSceneActorsProvider::FindLightById( CName id )
{
	const Uint32 num = m_lights.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		if ( CEntity* e = m_lights[ i ].Get() )
		{
			CName name( e->GetName() );
			if ( name == id )
			{
				return e;
			}
		}
	}
	return nullptr;
}

CEntity* CEdSceneActorsProvider::FindAnyEntityByIdforLight( CName id )
{
	if ( CEntity* e = FindActorById( id ) )
	{
		return e;
	}
	else if ( CEntity* e = FindPropById( id ) )
	{
		return e;
	}
	else if ( CEntity* e = FindEffectById( id ) )
	{
		return e;
	}
	return nullptr;
}

void CEdSceneActorsProvider::OverrideSpawnClass( EntitySpawnInfo& spawnInfo )
{
	if( spawnInfo.m_template && spawnInfo.m_template->GetEntityObject() && spawnInfo.m_template->GetEntityObject()->IsA< CActor >() )
	{
		spawnInfo.m_entityClass = CActor::GetStaticClass();
	}
}

void CEdSceneActorsProvider::SpawnEntity( CWorld* world, EntitySpawnInfo&& spawnInfo, const CName& vt )
{
	if ( m_syncMode )
	{
		CEntity* entity = world->GetDynamicLayer()->CreateEntitySync( spawnInfo );
		if ( entity )
		{
			world->IgnoreEntityStreaming( entity );

			entity->SetHideInGame( true, true, CEntity::HR_Scene );

			m_entities.PushBack( entity );
		}
		else
		{
			SCENE_ASSERT( 0 );
		}
	}
	else
	{
		CJobSpawnEntity* job = world->GetDynamicLayer()->CreateEntityAsync( Move( spawnInfo ) );
		if ( job )
		{
			m_entitiesJob.PushBack( job );
			//m_entitiesJobData.PushBack( TPair< CName, CEntityTemplate >( vt, spawnInfo.m_template ) );

			SCENE_ASSERT( m_entitiesJob.Size() == m_entitiesJobData.Size() );
		}
	}
}

Bool CEdSceneActorsProvider::HasActor( const CName& vt ) const
{
	const Uint32 size = m_entities.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CEntity* ent = m_entities[ i ].Get();
		if ( ent )
		{
			CActor* a = Cast< CActor >( ent );
			if ( a && vt && a->GetVoiceTag() == vt )
			{				
				return true;
			}
		}
	}

	/*const Uint32 jobSize = m_entitiesJobData.Size();
	for ( Uint32 i=0; i<jobSize; ++i )
	{
		CName jobVt = m_entitiesJobData[ i ].m_first;
		const THandle< CEntityTemplate > jobTempl = m_entitiesJobData[ i ].m_second;

		if ( jobTempl == templ )
		{
			if ( vt != CName::NONE && jobVt != vt )
			{					
				continue;
			}

			return true;
		}
	}*/

	return false;
}

Bool CEdSceneActorsProvider::HasEntity( const CName& vt ) const
{
	const Uint32 size = m_entities.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CEntity* ent = m_entities[ i ].Get();
		if ( ent )
		{
			CActor* a = Cast< CActor >( ent );
			if ( a && vt && a->GetVoiceTag() == vt )
			{				
				return true;
			}
		}
	}

	const Uint32 jobSize = m_entitiesJobData.Size();
	for ( Uint32 i=0; i<jobSize; ++i )
	{
		CName jobVt = m_entitiesJobData[ i ].m_first;
		const THandle< CEntityTemplate > jobTempl = m_entitiesJobData[ i ].m_second;

		if ( vt && jobVt == vt )
		{
			return true;
		}
	}

	return false;
}

Bool CEdSceneActorsProvider::RefreshLight( const String& name, ELightType type ) const
{
	const Uint32 num = m_lights.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		if ( CEntity* l = m_lights[ i ].Get() )
		{
			if ( l->GetName() == name )
			{
				l->ForceUpdateBoundsNode();
				return true;
			}
		}
	}
	return false;
}

Bool CEdSceneActorsProvider::IsSomethingFromActors( const CComponent* c ) const
{
	// check entities
	{
		const Uint32 size = m_entities.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			CEntity* ent = m_entities[ i ].Get();
			if ( ent )
			{
				for ( BaseComponentIterator it( ent ); it; ++it )
				{
					if ( *it == c )
					{
						return true;
					}
				}
			}
		}

		if ( const CItemEntity* ie = Cast< CItemEntity >( c->GetEntity() ) )
		{
			if ( ie->GetItemProxy() )
			{
				if ( const CEntity* e = ie->GetItemProxy()->GetParentEntity() )
				{
					for ( Uint32 i=0; i<size; ++i )
					{
						CEntity* ent = m_entities[ i ].Get();
						if ( ent && ent == e )
						{
							return true;
						}
					}
				}
			}
		}
	}

	// check props
	{
		const Uint32 size = m_props.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			CEntity* ent = m_props[ i ].Get();
			if ( ent )
			{
				for ( BaseComponentIterator it( ent ); it; ++it )
				{
					if ( *it == c )
					{
						return true;
					}
				}
			}
		}
	}

	// check effects
	{
		const Uint32 size = m_effects.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			CEntity* ent = m_effects[ i ].Get();
			if ( ent )
			{
				for ( BaseComponentIterator it( ent ); it; ++it )
				{
					if ( *it == c )
					{
						return true;
					}
				}
			}
		}
	}

	// check lights
	{
		const Uint32 size = m_lights.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			CEntity* ent = m_lights[ i ].Get();
			if ( ent )
			{
				for ( BaseComponentIterator it( ent ); it; ++it )
				{
					if ( *it == c )
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

void CEdSceneActorsProvider::ResetSceneContextActors()
{
	if ( !m_syncMode )
	{
		SCENE_ASSERT( AreAllActorsReady() );
	}

	const Uint32 size = m_entities.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CEntity* ent = m_entities[ i ].Get();
		if ( ent )
		{
			ent->SetPosition( Vector::ZERO_3D_POINT );
			ent->SetRotation( EulerAngles( 0.0f, 0.0f, 0.0f ) );
			ent->SetScale( Vector::ONES );

			ent->SetHideInGame( true, false, CEntity::HR_Scene );
		}
	}
}

const TDynArray< THandle< CEntity > >& CEdSceneActorsProvider::GetActorsForEditor()
{
	if ( !m_syncMode )
	{
		SCENE_ASSERT( AreAllActorsReady() );
	}

	return m_entities;
}

void CEdSceneActorsProvider::ClearAllSceneDataForActors()
{
	const Uint32 size = m_entities.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CEntity* ent = m_entities[ i ].Get();
		if ( CActor* actor = Cast< CActor >( ent ) )
		{
			if ( actor->IsLockedByScene() )
			{
				actor->SetSceneLock( false, false );
				actor->CancelSpeech();

				SCENE_ASSERT( actor->GetStoryScenes().Size() == 0 );
			}
		}
	}
}

void CEdSceneActorsProvider::InitializeActorsItems()
{
	for ( Uint32 i=0; i<m_entities.Size(); ++i )
	{
		CActor* actor = Cast< CActor >( m_entities[ i ].Get() );
		if ( actor && actor->GetInventoryComponent() )
		{
			actor->InitInventory();
			actor->GetInventoryComponent()->SpawnMountedItems();
		}
	}
}

void CEdSceneActorsProvider::WaitForAllActorsItems()
{
	if ( SItemEntityManager::GetInstance().IsDoingSomething() )
	{
		CTimeCounter timer;

		while ( timer.GetTimePeriod() < 5.f && SItemEntityManager::GetInstance().IsDoingSomething() )
		{
			SItemEntityManager::GetInstance().OnTick( 0.001f );
		}
	}
}

void CEdSceneActorsProvider::AddExtraActors()
{
	m_editor->OnActorsProvider_AddExtraActors( m_externalEntities, m_externalProps );

	m_entities.PushBack( m_externalEntities );
	m_props.PushBack( m_externalProps );
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
