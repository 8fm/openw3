/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "destructionComponent.h"
#include "..\physics\physicsSettings.h"
#include "..\physics\physicsIncludes.h"
#include "..\physics\physicsWorldPhysXImpl.h"
#include "physicsDestructionWrapper.h"
#include "physicsDestructionResource.h"
#include "collisionCache.h"
#include "renderer.h"
#include "renderCommands.h"
#include "collisionMesh.h"
#include "world.h"
#include "tickManager.h"
#include "soundEmitter.h"
#include "performableAction.h"
#include "../core/scriptStackFrame.h"
#include "physicsDataProviders.h"

IMPLEMENT_ENGINE_CLASS( CDestructionComponent );

CDestructionComponent::CDestructionComponent( )
	: m_wasDestroyed( false )
	, m_pathLibCollisionType( PLC_Disabled )
	, m_wrapper( nullptr )
{
	m_drawableFlags = DF_IsVisible | DF_CastShadows | DF_UseInAllApperances;
}

CDestructionComponent::~CDestructionComponent(void)
{
	RED_ASSERT( m_wrapper == nullptr, TXT("Destruction wrapper was not released. Was the OnDetach called ?") );
}


void CDestructionComponent::OnAttached(CWorld* world)
{
	{
		PC_SCOPE_PIX( CDestructionComponent_OnAttached );

#ifndef NO_EDITOR
		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Collision );
		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_BBoxesDestruction );
		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_PhysActorMasses );
#endif

		m_parameters.m_pose = GetLocalToWorld();
		m_parameters.m_parent = this;

		if ( !(IsStreamed() && GetEntity()->ShouldBeStreamed()) )
		{
			ScheduleFindCompiledCollision();
		}

		// Pass to base class
		TBaseClass::OnAttached( world );
	}
}

void CDestructionComponent::OnStreamIn()
{
	TBaseClass::OnStreamIn();
}

void CDestructionComponent::OnStreamOut()
{
	TBaseClass::OnStreamOut();
}


void CDestructionComponent::ScheduleTickPostPhysics()
{
	if( CWorld* world = GetWorld() )
	{
		world->GetTickManager()->AddToGroup( this, TICK_PostPhysics );
	}
}

void CDestructionComponent::ScheduleFindCompiledCollision()
{
	if( ShouldScheduleFindCompiledCollision() )
	{
		if( ( m_compiledCollisionFractured != nullptr) && ( m_compiledCollisionBase != nullptr ) )
		{
			InitWrapper();
		}
		else
		{
			if(  m_parameters.m_baseResource )
			{
				const String meshResourcePath = m_parameters.m_baseResource->GetDepotPath();
				CDiskFile* file = GDepot->FindFile( meshResourcePath );
				if( file )
				{
					GCollisionCache->FindCompiled_Async( this, meshResourcePath,file->GetFileTime() );	
				}
			}

			if(  m_parameters.m_fracturedResource )
			{
				const String meshResourceFractPath = m_parameters.m_fracturedResource->GetDepotPath();
				CDiskFile* fileFract = GDepot->FindFile( meshResourceFractPath );
				if( fileFract )
				{
					GCollisionCache->FindCompiled_Async( this, meshResourceFractPath, fileFract->GetFileTime() );	
				}
			}
		}
	}
}

Bool CDestructionComponent::ShouldScheduleFindCompiledCollision() const
{
#ifndef RED_FINAL_BUILD
	if( SPhysicsSettings::m_dontCreateDestruction ) return false;
#endif

	if( GetPhysicsRigidBodyWrapper() )
	{
		return false;
	}

	return true;
}

Bool CDestructionComponent::IsFractured() const
{
#ifndef RED_FINAL_BUILD
	if( SPhysicsSettings::m_dontCreateDestruction ) return false;
#endif

	if( !m_wrapper )
	{
		return false;
	}

	return m_wrapper->GetDestructionFlag( EPDF_IsFractured );
}

void CDestructionComponent::InitWrapper()
{
	{
		PC_SCOPE_PIX( CDestructionComponent_InitWrapper );

		Bool shouldAdjustAutohide = true;
#ifndef RED_FINAL_BUILD
		shouldAdjustAutohide &= !SPhysicsSettings::m_dontCreateDestruction;
#endif
#ifdef NO_EDITOR
		shouldAdjustAutohide &= ( m_parameters.m_baseResource && ICollisionCache::eResult_Valid == GCollisionCache->HasCollision( m_parameters.m_baseResource->GetDepotPath(), m_parameters.m_baseResource->GetFileTime() ) );
#endif

		if( shouldAdjustAutohide )
		{
			CPhysicsWorldPhysXImpl* physicalWorld = nullptr;
			if( GetWorld()->GetPhysicsWorld( physicalWorld ) )
			{
				if( !m_wrapper )
				{
					m_parameters.m_pose = GetLocalToWorld();
					m_parameters.m_parent = this;

					m_wrapper = physicalWorld->GetWrappersPool< CPhysicsDestructionWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( this ), m_parameters, physicalWorld, GetEntity()->GetVisibilityQuery(), m_compiledCollisionBase, m_compiledCollisionFractured );
					if( m_wrapper->GetPoolIndex() == 12 )
					{
						int a = 0;
					}
				}
				if( m_wrapper )
				{
					SWrapperContext* context = physicalWorld->GetWrappersPool< CPhysicsDestructionWrapper, SWrapperContext >()->GetContext(m_wrapper);
					context->m_desiredDistanceSquared = m_parameters.m_simulationDistance;

					m_wrapper->SetAutoHideDistance( context->m_desiredDistanceSquared );
					context->m_desiredDistanceSquared *= context->m_desiredDistanceSquared;
					context->m_requestProcessingFlag = true;

#ifndef NO_EDITOR
					if( !GGame->IsActive() )
					{
						if( !m_wrapper->GetDestructionFlag( EPDF_IsFractured ) )
						{
							m_wrapper->SwitchToKinematic( true );
						}
					}
#endif
				}
			}
		}

		RefreshRenderProxies();
	}


	{
		PC_SCOPE_PHYSICS( CDestruction_OnAttached2 );

		if( m_wrapper )
		{
			m_wrapper->SetCodeCallback( CPhysicsWrapperInterface::EPCCT_OnCollision, this, this );

			if( !m_parentEntityCollisionScriptEventName.Empty() )
			{
				m_wrapper->SetScriptCallback( CPhysicsWrapperInterface::EPSCT_OnCollision, this, m_parentEntityCollisionScriptEventName );
			}
		}

		if( !IsObstacleDisabled() )
		{
			IObstacleComponent::Attach( GetWorld() );
		}
	}
}

void CDestructionComponent::OnDetached(CWorld* world)
{
	TBaseClass::OnDetached( world );
	GCollisionCache->CancelFindCompiled_Async(this);
	ReleaseWrapper();
}

void CDestructionComponent::OnTickPostPhysics( Float timeDelta )
{
	PC_SCOPE( CDestructionComponent_OnTickPostPhysics )

	CWorld* world = GetWorld();
	world->GetTickManager()->RemoveFromGroup( this, TICK_PostPhysics );

	if( !m_wrapper )
	{
		return;
	}
	
	if( m_wrapper->GetDestructionFlag( EPDF_RequestFracture ) )
	{
		CEntity* entity = GetEntity();
		if( !m_fxName.Empty() )
		{
			RED_ASSERT( entity->HasEffect( m_fxName ) == false, TXT("Efect on fracture defined in resource isnt existing in entity") );
			entity->PlayEffect( m_fxName );
		}

		m_wasDestroyed = true;

		if ( m_disableObstacleOnDestruction && (m_pathLibCollisionType == PLC_Dynamic || m_pathLibCollisionType == PLC_Immediate) )
		{
			CWorld* world = entity->GetLayer()->GetWorld();
			if ( world )
			{
				IObstacleComponent::Disable( world );
			}
		}

		IPerformableAction::PerformAll( m_eventOnDestruction, entity );		
	}	

	if( m_fractureSoundEvent != StringAnsi::EMPTY )
	{
		CSoundEmitterComponent* soundEmitterComponent = GetEntity()->GetSoundEmitterComponent();
		soundEmitterComponent->SoundEvent( m_fractureSoundEvent.AsChar() );
	}
}

void CDestructionComponent::RefreshRenderProxies()
{
	ForceUpdateBoundsNode();
	TBaseClass::RefreshRenderProxies();
}

EPathLibCollision CDestructionComponent::GetPathLibCollisionGroup() const
{
	return m_pathLibCollisionType;
}

EPathLibCollision CDestructionComponent::GetPathLibCollisionType()
{
	return m_pathLibCollisionType;
}

void CDestructionComponent::SetPathLibCollisionGroupInternal(EPathLibCollision collisionGroup)
{
	m_pathLibCollisionType = collisionGroup;
}

CComponent* CDestructionComponent::AsEngineComponent()
{
	return this;
}

PathLib::IComponent* CDestructionComponent::AsPathLibComponent()
{
	return this;
}

void CDestructionComponent::OnCompiledCollisionFound(CompiledCollisionPtr collision)
{
	if( collision->GetGeometries().Size() == 1 )
	{
		// Base resource can have only one collision.
		// TODO: Lame, change it later?
		m_compiledCollisionBase = collision;
	}
	else
	{
		m_compiledCollisionFractured = collision;
	}
	if( ( m_compiledCollisionFractured != nullptr) && ( m_compiledCollisionBase != nullptr ) && !m_wrapper )
	{
		InitWrapper();
		RefreshRenderProxies();
	}
}

void CDestructionComponent::OnCompiledCollisionInvalid()
{
	RED_LOG(CDestructionComponent, TXT("CompiledCollision invalid!") );

#ifndef NO_EDITOR
	if(  m_parameters.m_baseResource && !m_compiledCollisionBase )
	{
		const String meshResourcePath = m_parameters.m_baseResource->GetDepotPath();
		CDiskFile* file = GDepot->FindFile( meshResourcePath );
		if( file )
		{
			GCollisionCache->Compile_Sync(m_compiledCollisionBase, m_parameters.m_baseResource, meshResourcePath, file->GetFileTime() );
		}
	}

	if(  m_parameters.m_fracturedResource && !m_compiledCollisionFractured )
	{
		const String meshResourceFractPath = m_parameters.m_fracturedResource->GetDepotPath();
		CDiskFile* fileFract = GDepot->FindFile( meshResourceFractPath );
		if( fileFract )
		{
			GCollisionCache->Compile_Sync(m_compiledCollisionFractured, m_parameters.m_fracturedResource, meshResourceFractPath, fileFract->GetFileTime() );
		}
	}

	if( ( m_compiledCollisionFractured != nullptr) && ( m_compiledCollisionBase != nullptr ) && !m_wrapper)
	{
		InitWrapper();
	}
#else
#endif
}


void CDestructionComponent::OnUpdateBounds()
{
	PC_SCOPE_PHYSICS(CDestructionComponent OnUpdateBounds )
	if ( m_wrapper && m_wrapper->GetDestructionFlag( EPDF_IsReady ) )
	{
		m_boundingBox = m_wrapper->GetBounds();
	}
	else
	{
		CMeshTypeComponent::OnUpdateBounds();
	}
}

#ifndef NO_EDITOR

void CDestructionComponent::EditorOnTransformChanged()
{
	TBaseClass::EditorOnTransformChangeStop();
	EditorRecreateCollision();
}

void CDestructionComponent::EditorRecreateCollision()
{
	if( m_wrapper )
	{
		if( m_wrapper->GetDestructionFlag( EPDF_IsFractured ) )
		{
			ReleaseWrapper();
			m_parameters.m_pose = GetLocalToWorld();
			if( ( m_compiledCollisionFractured != nullptr) && ( m_compiledCollisionBase != nullptr ) && !m_wrapper )
			{
				InitWrapper();
				RefreshRenderProxies();
			}
			else
			{
				ScheduleFindCompiledCollision();
			}
		}
		else
		{
			m_wrapper->SetFlag( PRBW_PoseIsDirty, true );
		}
	}
}

void CDestructionComponent::OnSelectionChanged()
{
	TBaseClass::OnSelectionChanged();
	if ( GetWorld() && !GetWorld()->GetPreviewWorldFlag() && m_wrapper )
	{
		if( !m_wrapper->GetDestructionFlag( EPDF_IsFractured ) )
		{
			if( (GetFlags() & NF_Selected) )
			{
				m_wrapper->SwitchToKinematic( true );
			}
		}
	}
}

void CDestructionComponent::EditorOnTransformChangeStop()
{
	// TODO: AD
}

void CDestructionComponent::EditorPreDeletion()
{
	// TODO: AD
}

void CDestructionComponent::PostNavigationCook(CWorld* world)
{
	ReleaseWrapper();
}

void CDestructionComponent::OnNavigationCook(CWorld* world, CNavigationCookingContext* cookerData)
{
	ASSERT( GIsCooker );
	// We load and pass only m_compiledCollisionBase because we don't really need to know about fractured shapes for nav cooking 
	if ( !m_wrapper )
	{
		if(  m_parameters.m_baseResource && !m_compiledCollisionBase )
		{
			const String meshResourcePath = m_parameters.m_baseResource->GetDepotPath();
			CDiskFile* file = GDepot->FindFile( meshResourcePath );
			if( file )
			{
				GCollisionCache->Compile_Sync(m_compiledCollisionBase, m_parameters.m_baseResource, meshResourcePath, file->GetFileTime() );
			}
		}

		if( m_compiledCollisionBase != nullptr )
		{
			CPhysicsWorldPhysXImpl* physicalWorld = nullptr;
			if( world->GetPhysicsWorld( physicalWorld ) )
			{
				m_parameters.m_pose = GetLocalToWorld();
				m_wrapper = physicalWorld->GetWrappersPool< CPhysicsDestructionWrapper, SWrapperContext > ()->Create( CPhysicsWrapperParentComponentProvider( this ), m_parameters, physicalWorld, 0, m_compiledCollisionBase, m_compiledCollisionFractured );
			}
		}
	}
	if ( m_wrapper && !m_wrapper->GetDestructionFlag( EPDF_IsReady ) )
	{
		m_wrapper->Create( m_parameters, m_compiledCollisionBase, m_compiledCollisionBase );
	}
}

void CDestructionComponent::SetPreviewResource(CPhysicsDestructionResource* physRes, CWorld* world)
{
	m_parameters.m_baseResource = physRes;
	m_parameters.m_fracturedResource = physRes;

	ConditionalAttachToRenderScene( world );
}

#endif

void CDestructionComponent::onCollision(const SPhysicalCollisionInfo& info)
{
	if( m_wrapper )
	{
		 m_wrapper->ApplyForce( info.m_force );		
	}
}

void CDestructionComponent::ReleaseWrapper()
{
	if( m_wrapper )
	{
		m_wrapper->Release();
		m_wrapper = 0;
	}
}

CMeshTypeResource* CDestructionComponent::GetMeshTypeResource() const 
{
	return TryGetMesh();
}

CMesh* CDestructionComponent::TryGetMesh() const
{
	if( m_wrapper )
	{
		if( m_wrapper->GetDestructionFlag( EPDF_IsFractured ) )
		{
			return m_parameters.m_fracturedResource->AsCMesh();
		}
		else
		{
			return m_parameters.m_baseResource->AsCMesh();
		}
	}

	if( m_parameters.m_baseResource)
	{
		return m_parameters.m_baseResource->AsCMesh();
	}

	return nullptr;
}

CompiledCollisionPtr CDestructionComponent::GetCompCollisionBase()
{
	return m_compiledCollisionBase;
}

CompiledCollisionPtr CDestructionComponent::GetCompCollisionFractured()
{
	return m_compiledCollisionFractured;
}

void CDestructionComponent::OnUpdateTransformComponent(SUpdateTransformContext& context, const Matrix& prevLocalToWorld)
{
	PC_SCOPE_PHYSICS(CDestructionComponent OnUpdateTransformComponent )

	if( m_wrapper )
	{
		if( m_wrapper->GetDestructionFlag( EPDF_IsFractured ) )
		{	
			m_wrapper->ScheduleSkinningUpdate();
		}
		else
		{
			m_wrapper->SetFlag( PRBW_PoseIsDirty, true );
		}
		TBaseClass::OnUpdateTransformComponent(context, prevLocalToWorld);
	}
}

void CDestructionComponent::GetSkinningMatrices(void* skinningMatrices, const Float* vertexEpsilons)
{
	if( !m_wrapper || !m_renderProxy )
	{
		return;
	}

	Float* outMatrices = reinterpret_cast<Float*>( skinningMatrices );
	const Int32 numSkeletonBones = m_wrapper->GetFracturedActorsCount();
	ptrdiff_t outMatricesStep = ( m_skinningData->GetMatrixType() == SDMT_3x4Transposed ? 12 : 16 );

	TDynArray< Matrix > bonePoses;
	TDynArray< Bool > bonesActive;

	Bool shouldUpdateActiveIndices = m_wrapper->SampleBonesModelSpace( bonePoses, bonesActive );

	Uint32 bpSize = bonePoses.Size();
	for ( Int32 i = 0; i < numSkeletonBones && bpSize == numSkeletonBones; i++, outMatrices+= outMatricesStep )
	{
		const Int32 boneIndex = i;
		Matrix actorMat = bonePoses[ boneIndex ];
		SkeletonBonesUtils::CopyMatrix( outMatrices, actorMat, m_skinningData->GetMatrixType() );
	}

	if( shouldUpdateActiveIndices )
	{
		SBoneIndicesHelper helper = m_parameters.m_fracturedResource->GetIndicesForActiveBones( bonesActive );

		if( helper.m_activeIndices.Size() > 0 )
		{
			(new CRenderCommand_UpdateDestructionMeshActiveIndices( m_renderProxy,  Move( helper.m_activeIndices ), Move( helper.m_chunkOffsets ), Move( helper.m_chunkNumIndices )) )->Commit();
		}
		else // fully dissolved & we haven't scheduled release yet
		{
			// Destroy current proxy
			CWorld* world = GetLayer()->GetWorld();

			// Detach if attached
			if ( world->GetRenderSceneEx() )
			{
				// Detach proxy from rendering scene
				( new CRenderCommand_RemoveProxyFromScene( world->GetRenderSceneEx(), m_renderProxy ) )->Commit();
			}				

			CEntity* entity = GetEntity();

			if( entity )
			{
				// Inform entity that rendering proxy has been detached
				entity->OnProxyDetached( this );
			}

			// Free proxy
			m_renderProxy->Release();
			m_renderProxy = NULL;
			
		}
	}
}

void CDestructionComponent::ComputeSkinningData( )
{
	const CMeshTypeResource* mesh = m_parameters.m_fracturedResource;

	if ( mesh && mesh->GetBoneCount() && GRender && m_renderProxy && m_wrapper && m_wrapper->IsReady() )
	{
		const Uint32 numMeshBones = m_wrapper->GetFracturedActorsCount();
		RED_FATAL_ASSERT( numMeshBones == mesh->GetBoneCount(), "Destruction fractured mesh mapping does not match the mesh" );

		if ( m_skinningData && m_skinningData->GetMatrixCount() != numMeshBones )
		{
			m_skinningData->Release();
			m_skinningData = nullptr;
		}

		// Recreate skinning data if needed
		if ( !m_skinningData )
		{
			m_skinningData = GRender->CreateSkinningBuffer( numMeshBones, true );
		}

		// Update the skinning data
		if ( m_skinningData )
		{
			const Float* vertexEpsilons = mesh->GetBoneVertexEpsilons();

			void* dataToFill = m_skinningData->GetWriteData();

			// We can fill
			{
				GetSkinningMatrices( dataToFill, vertexEpsilons );
			}
		}
	}
}

Bool CDestructionComponent::UpdateSkinning()
{
	ComputeSkinningData( );

	if( m_skinningData )
	{
		RenderProxyUpdateInfo info;
		info.m_boundingBox = &m_boundingBox;
		info.m_localToWorld = &GetLocalToWorld();

		m_skinningData->AdvanceWrite();

		if( m_renderProxy )
		{
			(new CRenderCommand_UpdateSkinningDataAndRelink( m_renderProxy, m_skinningData, info) )->Commit();
			return true;
		}
	}

	return false;
}


Bool CDestructionComponent::EnableDissolve(Bool enable)
{
	IRenderProxy* proxy = GetRenderProxy();
	if( !proxy )
	{
		return false;
	}
	(new CRenderCommand_SetDestructionMeshDissolving( proxy, enable ) )->Commit();
	return true;
}

void CDestructionComponent::OnSaveGameplayState( IGameSaver* saver )
{
	TBaseClass::OnSaveGameplayState( saver );

	saver->WriteValue( CNAME( state ), m_wasDestroyed );
}

void CDestructionComponent::OnLoadGameplayState( IGameLoader* loader )
{
	TBaseClass::OnLoadGameplayState( loader );

	loader->ReadValue( CNAME( state ), m_wasDestroyed );
	if( m_wasDestroyed )
	{
		m_parameters.m_isDestroyed = m_wasDestroyed;
		// We check if wrapper was already created. In that case, we need to release it and
		// recreate as destroyed already. It may seem that simply requesting fracture would suffice,
		// but that would result in destruction collapsing in place and fading away (if set) which wouldn't look good.
		if( m_wrapper )
		{
			ReleaseWrapper();
			InitWrapper();
			RefreshRenderProxies();
		}
	}	
}

#ifndef NO_EDITOR_FRAGMENTS
void CDestructionComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flags );
	if( m_wrapper )
	{
		if ( flags == SHOW_BBoxesDestruction )
		{
			frame->AddDebugAxis( m_localToWorld.GetTranslationRef(), m_localToWorld, 0.1f, true );
			Float distFromViewport = 0.f;
			Vector wPos(0,0,0);
			CPhysicsWorld* world = m_wrapper->GetPhysicsWorld();
			if( CPhysicsWorldPhysXImpl* physcWorld = static_cast< CPhysicsWorldPhysXImpl* >( world ) )
			{
				SWrapperContext* context = physcWorld->GetWrappersPool< CPhysicsDestructionWrapper, SWrapperContext >()->GetContext( m_wrapper );
				distFromViewport = context->m_resultDistanceSquared;
				wPos.X = context->m_x;
				wPos.Y = context->m_y;
				wPos.Z = 0;
			}

			String dist = String::Printf( TXT("%.2f"), Red::Math::MSqrt( distFromViewport ) );
			Color col = Color::GREEN;

			frame->AddDebugText( m_localToWorld.GetTranslationRef(), dist, 0, 0, false, col );				// distance to components l2w
			dist = String::Printf( TXT("%.2f"),m_wrapper->GetAccumulatedDamage() );
			frame->AddDebugText( m_localToWorld.GetTranslationRef(), dist, 0, 2, false, col );				// distance to components l2w
			frame->AddDebugSphere( m_boundingBox.CalcCenter(), 0.05f, Matrix::IDENTITY, col, true, true );	// bbox center
			frame->AddDebugSphere( wPos, 0.15f, Matrix::IDENTITY, Color::YELLOW, true, true );				// wrapper pos
			frame->AddDebugBox( m_boundingBox, Matrix::IDENTITY, Color::RED, true );						// bbox from component
			frame->AddDebugBox( m_wrapper->GetBounds(), Matrix::IDENTITY, Color::MAGENTA, true );			// bbox from apex
		}

#ifndef NO_EDITOR

		//	Bool masses = frame->GetFrameInfo().IsShowFlagOn( SHOW_PhysActorMasses );
		if( flags == SHOW_PhysActorMasses )
		{
			const TDynArray<Float>& massesArray = m_wrapper->GetMasses();
			if( massesArray.Size() )
			{
				TDynArray< Matrix > positionsArray;
				TDynArray< Bool > bonesActive;
				m_wrapper->SampleBonesModelSpace( positionsArray, bonesActive );
				String dist = String::Printf( TXT("%.2f"), massesArray[0] );
				Color col = Color::GREEN;

				frame->AddDebugText(  m_localToWorld.GetTranslationRef(), dist, 0, 0, false, col );	
				for(Uint32 i = 1, size = positionsArray.Size() + 1; i < size; i++)
				{
					dist = String::Printf( TXT("%.2f"), massesArray[i] );
					Matrix finalPos =  positionsArray[ i - 1 ] * m_localToWorld;
					frame->AddDebugText( finalPos.GetTranslationRef(), dist, 0, 0, false, col );	
				}
			}			
		}

#ifndef NO_RESOURCE_IMPORT
		if ( flags == SHOW_Collision )
		{
			const CCollisionMesh* mesh = TryGetMesh() ? TryGetMesh()->GetCollisionMesh() : NULL;
			if ( mesh )
			{
				// Setup rendering context
				CCollisionMesh::RenderContext renderContext;
#ifndef NO_COMPONENT_GRAPH
				renderContext.m_hitProxyID = GetHitProxyID();
#else
				renderContext.m_hitProxyID = CHitProxyID();
#endif
				renderContext.m_localToWorld = GetLocalToWorld();
				renderContext.m_selected = IsSelected();
				renderContext.m_solid = true;

				// Generate collision mesh fragments
				mesh->GenerateFragments( frame, renderContext );
			}
		}
#endif  // NO_RESOURCE_IMPORT
#endif	// NO_EDITOR
	}

}

#endif // NO_EDITOR_FRAGMENTS

void CDestructionComponent::funcIsDestroyed( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
#ifdef USE_APEX
	if( !m_wrapper )
	{
		RETURN_BOOL( false );
		return;
	}
	RETURN_BOOL( m_wrapper->GetDestructionFlag( EPDF_IsFractured ) );
#else
	RETURN_BOOL( false );
#endif
}

void CDestructionComponent::funcIsObstacleDisabled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsObstacleDisabled() );
}

void CDestructionComponent::funcApplyFracture( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;


	const Bool res = 
#ifdef USE_APEX
		GetDestructionBodyWrapper() ? GetDestructionBodyWrapper()->ApplyFracture() : false;
#else
		false;
#endif

	RETURN_BOOL( res );
}
