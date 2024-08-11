#include "build.h"

#include "apexClothWrapper.h"
#include "../physics/physicsEngine.h"
#include "../physics/physicsWorldPhysXImpl.h"
#include "renderCommands.h"
#include "../core/dataError.h"
#include "collisionCache.h"

#ifdef USE_APEX
#include "NxParamUtils.h"
#include "NxResourceProvider.h"
#include "NxClothingAsset.h"
#include "NxClothingActor.h"
#include "NxApexScene.h"
#include "NxClothingCollision.h"
#include "NxClothingRenderProxy.h"
#endif
#include "apexResource.h"
#include "../physics/physicsSettings.h"
#include "clothComponent.h"
#include "world.h"
#include "baseEngine.h"
#include "renderFrame.h"
#include "entity.h"
#include "layer.h"
#include "utils.h"
#include "../physics/PhysicsWrappersDefinition.h"


#ifdef USE_APEX
using namespace physx;
using namespace physx::apex;

DECLARE_PHYSICS_WRAPPER(CApexClothWrapper,EPW_ApexCloth,true, true)

CApexClothWrapper::CApexClothWrapper( CPhysicsWorld* physicalWorld, SClothParameters* parameters, Uint32 visibiltyId )
	: CApexWrapper()
	, m_actor( nullptr )
	, m_simulationMaxDistance( -1.f )
	, m_windScaler( 1.f )
	, m_windAdaptationCurrent( 0.0f )
	, m_windAdaptationTarget( 0.0f )
	, m_motionIntensity( 0.0f )
	, m_updateMotionIntensity( false )
	, m_forcedLod( -1 )
	, m_targetMaxDistanceScale( 1.f )
	, m_currentMaxDistanceScale( 0.f )
	, m_maxDistanceBlendTime( 1.0f )
	, m_isFrozen( false )
	, m_isClothSkinned( false )
	, m_wrapperTransform( Matrix::IDENTITY )
	, m_isSimulating( false )
	, m_wetness( 0.0f )
	, m_skipMaxDistanceUpdate( false )
	, m_isVisibleOverride( ECVO_NoOverride )
{
	m_world = static_cast< CPhysicsWorldPhysXImpl* >( physicalWorld );

	m_simulationType = SM_DYNAMIC;

	m_simulationMaxDistance = parameters->m_simulationMaxDistance;
	m_windScaler = parameters->m_windScaler;
	m_maxDistanceBlendTime = parameters->m_maxDistanceBlendTime;
	m_isFrozen = false;

	// update wrapper pose from compoenent
	m_wrapperTransform = parameters->m_pose;
	// update wrapper if clothing is skinned or not
	m_isClothSkinned = parameters->m_isClothSkinned;

	SWrapperContext* position = m_world->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	if( !position )
	{
		int a = 0;
	}
	position->m_x = m_wrapperTransform.GetTranslationRef().X;
	position->m_y = m_wrapperTransform.GetTranslationRef().Y;
	position->m_resultDistanceSquared = FLT_MAX;
	position->m_visibilityQueryId = visibiltyId;
}

CApexClothWrapper::~CApexClothWrapper()
{
	Destroy();
}

void CApexClothWrapper::Release( Uint32 actorIndex )
{
	for( auto i = m_currentColliders.Begin(); i != m_currentColliders.End(); ++i )
	{
		RemoveCollider( i->m_info );
	}
	RED_ASSERT( m_ref.GetValue() > 0 )
	if( !m_ref.Decrement() )
	{
		static_cast< CPhysicsWorldPhysXImpl* >( m_world )->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->PushWrapperToRemove( this );
	}
}

Bool CApexClothWrapper::MakeReadyToDestroy( TDynArray< void* >* toRemove )
{
	if( m_currentColliders.Empty() ) return true;
	for( Int32 i = m_currentColliders.Size(); i > 0; --i )
	{
		SSceneCollider& sceneCollider = m_currentColliders[ i - 1 ];
		Uint32 collisionShapesCount = sceneCollider.m_collisionShapes.Size();
		for( Uint32 j = 0; j != collisionShapesCount; ++j )
		{
			NxClothingCollision* sceneCollision = sceneCollider.m_collisionShapes[ j ];
			NxClothingCapsule* sceneColliderCapsule = sceneCollision->isCapsule();
			if( sceneColliderCapsule )
			{
				sceneColliderCapsule->releaseWithSpheres();
				continue;
			}
			NxClothingConvex* sceneColliderConvex = sceneCollision->isConvex();
			if( sceneColliderConvex )
			{
				sceneColliderConvex->releaseWithPlanes();
				continue;
			}
			sceneCollider.m_collisionShapes[ j ]->release();
		}
		m_currentColliders.RemoveAtFast( i - 1 );
	}
	m_currentColliders.Clear();
	return false;
}

void CApexClothWrapper::FreezeCloth( Bool shouldFreeze /*= false */ )
{
	if ( !m_actor ) return;
	m_actor->setFrozen( shouldFreeze );
}

void CApexClothWrapper::RequestReCreate( const SClothParameters& parameters )
{
	Destroy();
}

Bool CApexClothWrapper::Create( const SClothParameters& parameters )
{
	PC_SCOPE_PHYSICS( CApexClothWrap_Create )

	if( m_actor )
	{
		Destroy();
	};

	CApexResource* resource = parameters.m_resource.Get();
	if( !resource ) return false;

	CompiledCollisionPtr compiledCollision;
	{
		PC_SCOPE_PHYSICS( CApexClothWrap_TryPreload )
#ifdef NO_EDITOR
		if( !resource->GetAsset() )
		{	
			PC_SCOPE_PHYSICS( CApexClothWrapper Create find )	

			ICollisionCache::EResult result = GCollisionCache->FindCompiled( compiledCollision, resource->GetDepotPath(), resource->GetFileTime() );
			if( ICollisionCache::eResult_Valid != result )
			{
				if( ICollisionCache::eResult_NotReady == result )
				{
					m_world->MarkSectorAsStuffAdded();
				}
				return false;
			}
		}
#endif
	}

	if( !resource->TryPreload( compiledCollision ) ) return false;
	resource->AddRef();

	NxClothingAsset* asset = GetResourceAsset( parameters );
	if( !asset )
	{
		resource->ReleaseRef();
		return false;
	}

	resource->AddAssetRef( asset );

	NxParameterized::Interface* actorDesc = asset->getDefaultActorDesc();

	if (!actorDesc)
	{
		resource->ReleaseRef();
		return false;
	}

	NxParameterized::Handle handle(actorDesc);
		
	Vector xAxis = parameters.m_pose.V[ 0 ];
	Vector yAxis = parameters.m_pose.V[ 1 ];
	Vector zAxis = parameters.m_pose.V[ 2 ];
	Vector pos = parameters.m_pose.V[ 3 ];

	PxMat44 myPose( TO_PX_VECTOR( xAxis ), TO_PX_VECTOR( yAxis ), TO_PX_VECTOR( zAxis ), TO_PX_VECTOR( pos ) ); 

	PxF32 scale = myPose.column0.magnitude();
	NxParameterized::setParamF32(*actorDesc, "actorScale", scale);

	NxParameterized::setParamMat44( *actorDesc, "globalPose", myPose);

	NxParameterized::setParamBool(*actorDesc, "flags.RecomputeNormals", parameters.m_recomputeNormals );
	NxParameterized::setParamBool(*actorDesc, "flags.CorrectSimulationNormals", parameters.m_correctSimulationNormals );
	NxParameterized::setParamBool(*actorDesc, "slowStart", parameters.m_slowStart );
	NxParameterized::setParamBool(*actorDesc, "useStiffSolver", parameters.m_useStiffSolver );
	NxParameterized::setParamF32(*actorDesc, "pressure", parameters.m_pressure );
	if( parameters.m_simulationMaxDistance == -1  )
	{
		NxParameterized::setParamF32(*actorDesc, "lodWeights.maxDistance", 0 );
	}
	else
	{
		NxParameterized::setParamF32(*actorDesc, "lodWeights.maxDistance", parameters.m_simulationMaxDistance );
	}

	NxParameterized::setParamF32(*actorDesc, "lodWeights.distanceWeight", parameters.m_distanceWeight );
	NxParameterized::setParamF32(*actorDesc, "lodWeights.bias", parameters.m_bias );
	NxParameterized::setParamF32(*actorDesc, "lodWeights.benefitsBias", parameters.m_benefitsBias );
	NxParameterized::setParamF32(*actorDesc, "maxDistanceBlendTime", parameters.m_maxDistanceBlendTime );
	NxParameterized::setParamU32(*actorDesc, "uvChannelForTangentUpdate", parameters.m_uvChannelForTangentUpdate );
	NxParameterized::setParamBool(*actorDesc, "maxDistanceScale.Multipliable", parameters.m_maxDistanceScaleMultipliable );
	NxParameterized::setParamF32(*actorDesc, "maxDistanceScale.Scale", parameters.m_maxDistanceScaleScale );
	NxParameterized::setParamF32(*actorDesc, "clothDescTemplate.collisionResponseCoefficient", parameters.m_collisionResponseCoefficient );
	NxParameterized::setParamBool(*actorDesc, "allowAdaptiveTargetFrequency", parameters.m_allowAdaptiveTargetFrequency );

	NxParameterized::setParamBool(*actorDesc, "flags.ParallelCpuSkinning", true);
	NxParameterized::setParamBool(*actorDesc, "useInternalBoneOrder", true);
	NxParameterized::setParamBool(*actorDesc, "maxDistanceScale.Multipliable", true);
	
	Bool isClothGPUSimulationEnabled = IsClothGPUSimulationEnabled( m_world );

	if( isClothGPUSimulationEnabled && ( parameters.m_dispacherSelection == EDS_GPU_ONLY || parameters.m_dispacherSelection == EDS_GPU_IF_AVAILABLE ) )
	{
		NxParameterized::setParamBool(*actorDesc, "useHardwareCloth", true );
	}
	else
	{
		NxParameterized::setParamBool(*actorDesc, "useHardwareCloth", false );
	}

	NxParameterized::setParamBool(*actorDesc, "updateStateWithGlobalMatrices", true );

	//NxParameterized::setParamBool(*actorDesc, "fallbackSkinning", true );
	NxParameterized::setParamBool(*actorDesc, "localSpaceSim", true );

	NxParameterized::setParamVec3(*actorDesc, "windParams.Velocity", PxVec3( 0.0f, 0.0f, 0.0f ) );
	NxParameterized::setParamF32(*actorDesc, "windParams.Adaption", 0.0f );

	m_world->AddRef();

	NxApexScene* apexScene = ( NxApexScene* ) ( m_world->GetPxScene()->userData );

	{
//		String path = parameters.m_resource->GetFile()->GetFileName();
//		PcScopePhysicsLogger prof( UNICODE_TO_ANSI( path.AsChar() ) );
		physx::NxApexActor* actor = asset->createApexActor(*actorDesc, *apexScene);
		m_actor = static_cast<physx::NxClothingActor*>(actor);
	}

	if ( !m_actor )
	{
		return false;
	}

	AddActorRef( m_actor );

	m_actor->forcePhysicalLod( 0 );
	m_actor->setGraphicalLOD( 0 );

	// update wrapper pose from compoenent
	m_wrapperTransform = parameters.m_pose;
	// update wrapper if clothing is skinned or not
	m_isClothSkinned = parameters.m_isClothSkinned;

	PxBounds3 pxBounds = asset->getBoundingBox();
	m_bounds.Min = TO_VECTOR( pxBounds.minimum );
	m_bounds.Max = TO_VECTOR( pxBounds.maximum );
	m_bounds = m_wrapperTransform.TransformBox( m_bounds );

	PHYSICS_STATISTICS_INC(ClothsInstanced)
	return true;
}

Bool CApexClothWrapper::Destroy()
{
	if( !m_actor ) return false;

	while( m_currentColliders.Size() )
	{
		RemoveCollider( m_currentColliders.Begin()->m_info );
	}
	m_currentColliders.Clear();

	NxClothingAsset* asset = reinterpret_cast<physx::apex::NxClothingAsset*>( m_actor->getOwner() );

	ReleaseActorRef( m_actor );
	// Don't clear out userdata, since someone else might still be holding a reference (e.g. a render command or something). userdata will
	// be NULL'd when the last reference is released.
	m_actor = nullptr;

	CApexResource* resource = CApexResource::FromAsset( asset );
	resource->ReleaseAssetRef( asset );
	resource->ReleaseRef();
	m_world->ReleaseRef();

	PHYSICS_STATISTICS_DEC(ClothsInstanced)

	return true;
}

NxClothingAsset* CApexClothWrapper::GetResourceAsset( const SClothParameters& parameters )
{
	CApexResource* resource = parameters.m_resource.Get();
#ifdef NO_EDITOR
	return reinterpret_cast<physx::apex::NxClothingAsset*>( resource->GetAsset() );
#else
	if ( parameters.m_usePreviewAsset )
		return reinterpret_cast<NxClothingAsset*>( resource->GetPreviewAsset() );
	else
		return reinterpret_cast<NxClothingAsset*>( resource->GetAsset() );
#endif
}

Bool CApexClothWrapper::ShouldBeSimulated( Float distanceFromViewportSquared ) const
{
	if ( m_simulationType != SM_DYNAMIC )
	{
		return false;
	}
	Float distanceLimit = m_simulationMaxDistance;
	if( m_simulationMaxDistance == -1 )
	{
		distanceLimit = SPhysicsSettings::m_clothSimulationDistanceLimit;
	}
	distanceLimit *= distanceLimit;

	if( distanceFromViewportSquared > distanceLimit )
	{
		return false;
	}
	return true;
}

void CApexClothWrapper::UpdateGraphicalLod( float distanceFromViewportSquared )
{
	Int32 forcedLod = m_forcedLod.GetValue();
	if ( forcedLod >= 0 )
	{
		m_actor->setGraphicalLOD( forcedLod );
		return;
	}

	CMeshTypeComponent* component = nullptr;
	if( GetParent( component ) )
	{
		if ( CMeshTypeResource* resource = component->GetMeshTypeResource() )
		{
			Uint32 numLODs = resource->GetNumLODLevels();
			if ( numLODs > 0)
			{
				Uint32 whichLOD = 0;
				for ( Uint32 i = numLODs - 1; i > 0; --i )
				{
					const SMeshTypeResourceLODLevel& lodLevel = resource->GetLODLevel( i );
					Float lodDistance = lodLevel.GetDistance();
					if ( lodLevel.IsUsedOnPlatform() && distanceFromViewportSquared >= lodDistance*lodDistance )
					{
						whichLOD = i;
						break;
					}
				}

				Uint32 currentLOD = m_actor->getGraphicalLod();
				if( currentLOD != whichLOD )
				{
					m_actor->setGraphicalLOD( whichLOD );
				}
			}
		}
	}
}

void CApexClothWrapper::SetMaxDistanceScale( Float ratio )
{
	if( !m_actor ) return;
	m_targetMaxDistanceScale = ratio;
}

void CApexClothWrapper::SetCurrentMaxDistanceScale( Float ratio )
{
	if( !m_actor ) return;
	m_currentMaxDistanceScale = ratio;
	m_skipMaxDistanceUpdate = true;
}

void CApexClothWrapper::SetMaxDistanceBlendTime( Float ratio )
{
	if( !m_actor ) return;
	m_maxDistanceBlendTime = ratio;
}

void CApexClothWrapper::ForceLODLevel( Int32 lodOverride )
{
	m_forcedLod.SetValue( lodOverride );
}

void CApexClothWrapper::SetVisible(EClothVisibilityOverride currentVisiblity)
{
	m_isVisibleOverride = currentVisiblity;

	// Semi hack for case of npc horses leaving their tails behind
	// To be removed once PostSim is moved to tasks.
	if( currentVisiblity == ECVO_Visible )
	{
		SWrapperContext* position = m_world->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
		position->m_requestProcessingFlag = true;
	}
}

void CApexClothWrapper::PreSimulation( SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd )
{
	PC_SCOPE_PHYSICS(CApexClothWrapper PreSimulation )

	CComponent* component = nullptr;
	if( !GetParentProvider()->GetParent( component ) ) return;

	// TODO: change component->GetLocalToWorld() to m_wrapperTrasform
	const Matrix& pose = m_wrapperTransform;

	// calculate blend factor
	Float blendFactor = timeDelta / m_maxDistanceBlendTime;

#ifdef PHYSICS_NAN_CHECKS
	if( !pose.IsOk() )
	{
		RED_FATAL_ASSERT( pose.IsOk(), "NANS" );
		return;
	}
#endif

	const Float distanceFromViewportSquared = simulationContext->m_resultDistanceSquared;
	Bool currentFrameInAutoHideDistance = (m_autoHideDistance*m_autoHideDistance) > distanceFromViewportSquared;
	if( currentFrameInAutoHideDistance && !m_actor )
	{
		static Uint64 currentTickMarker = 0;
		if( currentTickMarker < tickMarker || !tickMarker ) 
		{
			CClothComponent* clothComponent = nullptr;
			if( GetParent( clothComponent ) )
			{
				const SClothParameters&	clothParameters = clothComponent->GetParameters();
				Create( clothParameters );
				if( m_actor )
				{
					currentTickMarker = tickMarker;

					// update bbox in component f.ex for umbra
					clothComponent->SetBoundingBox( m_bounds );
					clothComponent->ActivateTrigger();
					clothComponent->RefreshRenderProxies();
				}
				else return;
			}
			else return;
		}

	}

	Bool shouldBeSimulated = ShouldBeSimulated( distanceFromViewportSquared );
	Float currentPhysicalLod = 0;
	if( m_actor ) currentPhysicalLod = m_actor->getActivePhysicalLod();
	Bool currentVisiblity = true;
	Bool wasVisible = true;
	if( m_actor )
	{
		wasVisible = m_actor->isVisible();

		const ERenderVisibilityResult visibilityQueryResult = ( const ERenderVisibilityResult ) simulationContext->m_visibilityQueryResult;
		currentVisiblity = visibilityQueryResult != ERenderVisibilityResult::RVR_NotVisible;

		if( m_isVisibleOverride == ECVO_Visible )
		{
			m_actor->setVisible( true );
			currentVisiblity = true;
		}
		else
		{
			if( wasVisible != currentVisiblity )
			{
				m_actor->setVisible( currentVisiblity );
			}
		}
	}
	// Clear visibility overrider
	m_isVisibleOverride = ECVO_NoOverride;

	if( !shouldBeSimulated )
	{
		if( currentPhysicalLod > 0 )
		{
			if( m_currentMaxDistanceScale <= 0.0f )
			{
				// we blended out so we can disable simulation
				if( m_actor )
				{
					// disable simulation
					if( m_isFrozen )
					{
						m_actor->setFrozen( false );
						m_isFrozen = false;
					}
					m_actor->forcePhysicalLod( 0 );
					m_isSimulating = false;
				}
			}
			else
			{
				// blend out
				m_currentMaxDistanceScale -= blendFactor;
				m_currentMaxDistanceScale = Clamp< Float >( m_currentMaxDistanceScale, 0.f, 1.f );
				if( m_actor )
				{
					m_actor->updateMaxDistanceScale( m_currentMaxDistanceScale, true );
				}
			}
		}
	}
	else if( shouldBeSimulated )
	{
		if( m_actor )
		{
			if ( !m_isClothSkinned )
			{
				if( !currentVisiblity )
				{
					if( !m_isFrozen )
					{
						m_actor->setFrozen( true );
						m_isFrozen = true;
					}
				}
				else
				{
					if( m_isFrozen )
					{
						m_actor->setFrozen( false );
						m_isFrozen = false;
					}
				}
			}
			PHYSICS_STATISTICS_INC_IF(ClothsSimulated,currentPhysicalLod > 0 && !m_actor->isFrozen() );
		}

		if( !m_isFrozen )
		{
			// started simulating
			if( currentPhysicalLod == 0 )
			{
				// enable simulation
				if( m_actor )
				{
					m_actor->forcePhysicalLod( 1 );
					m_targetMaxDistanceScale = 1.f;
					m_isSimulating = true;
				}
			}
			else if ( currentPhysicalLod == 1.f )
			{
				// manual blending in/out to full max dist/skin
				if ( m_currentMaxDistanceScale != m_targetMaxDistanceScale )
				{
					if ( !m_skipMaxDistanceUpdate )
					{
						Float diff = Red::Math::MAbs( m_targetMaxDistanceScale - m_currentMaxDistanceScale);
						if( m_currentMaxDistanceScale < m_targetMaxDistanceScale )
						{
							m_currentMaxDistanceScale += Min< Float >( blendFactor, diff );
						}
						else if( m_currentMaxDistanceScale > m_targetMaxDistanceScale )
						{
							m_currentMaxDistanceScale -= Min< Float >( blendFactor, diff );
						}
					}
					else
					{
						m_skipMaxDistanceUpdate = false;
					}

					m_currentMaxDistanceScale = Clamp< Float >( m_currentMaxDistanceScale, 0.f, 1.f );

					if( m_actor )
					{
						m_actor->updateMaxDistanceScale( m_currentMaxDistanceScale, true );
					}
				}
			}
		}
	}

	if( GetFlag( PRBW_PoseIsDirty ) )
	{
		SetPose( pose );
	}

	for( Int32 i = m_currentColliders.Size(); i > 0; --i )
	{
		SSceneCollider& sceneCollider = m_currentColliders[ i - 1 ];
		if( !sceneCollider.m_isToRemove ) continue;
		Uint32 collisionShapesCount = sceneCollider.m_collisionShapes.Size();
		for( Uint32 j = 0; j != collisionShapesCount; ++j )
		{
			NxClothingCollision* sceneCollision = sceneCollider.m_collisionShapes[ j ];
			NxClothingCapsule* sceneColliderCapsule = sceneCollision->isCapsule();
			if( sceneColliderCapsule )
			{
				sceneColliderCapsule->releaseWithSpheres();
				continue;
			}
			NxClothingConvex* sceneColliderConvex = sceneCollision->isConvex();
			if( sceneColliderConvex )
			{
				sceneColliderConvex->releaseWithPlanes();
				continue;
			}
			sceneCollider.m_collisionShapes[ j ]->release();
		}
		m_currentColliders.RemoveAtFast( i - 1 );
	}

	if( !m_actor ) return;

	Uint32 collidersCount = m_currentColliders.Size();

	if( collidersCount)
	{
		PC_SCOPE_PHYSICS(CApexClothWrap updatecoliders )

		Vector scale = pose.GetScale33();
		Matrix localToWorld = Matrix::IDENTITY;
		localToWorld.V[0].A[0] = 1.0f / scale.X;
		localToWorld.V[0].A[1] = 0.0f;
		localToWorld.V[0].A[2] = 0.0f;
		localToWorld.V[1].A[0] = 0.0f;
		localToWorld.V[1].A[1] = 1.0f / scale.Y;
		localToWorld.V[1].A[2] = 0.0f;
		localToWorld.V[2].A[0] = 0.0f;
		localToWorld.V[2].A[1] = 0.0f;
		localToWorld.V[2].A[2] = 1.0f / scale.Z;
		localToWorld.SetTranslation( pose.GetTranslationRef() );

#ifdef PHYSICS_NAN_CHECKS
		if( !localToWorld.IsOk() )
		{
			RED_FATAL_ASSERT( localToWorld.IsOk(), "NANS" );
			return;
		}
#endif

		Matrix localToWorldInverse = localToWorld.Inverted();

		PxMat44 inverted = TO_PX_MAT( localToWorldInverse );

		for( Uint32 j = 0; j != collidersCount; ++j )
		{
			SSceneCollider& sceneCollider = m_currentColliders[ j ];
			IScriptable* scriptable = sceneCollider.m_info.m_triggeredObject.Get();
			if( !scriptable )
			{
				sceneCollider.m_isToRemove = true;
				continue;
			}

			Uint32 collisionShapesCount = sceneCollider.m_collisionShapes.Size();
			for( Uint32 i = 0; i != collisionShapesCount; ++i )
			{
				CPhysicsWrapperInterface* wrapper = sceneCollider.m_info.m_triggeredWrapper;
				if( !wrapper ) continue;

				PxShape* shape = ( PxShape* ) wrapper->GetShape( sceneCollider.m_info.m_triggeredBodyIndex.m_shapeIndex, sceneCollider.m_info.m_triggeredBodyIndex.m_actorIndex );
				if( !shape ) continue;
				PxMat44 shapePose( shape->getLocalPose() );
				PxMat44 actorPose;
				PxActor* actor = shape->getActor();
				PxRigidActor* rigidActor = actor->isRigidActor();
				if( !rigidActor ) continue;
				PxRigidDynamic* dynamic = actor->isRigidDynamic();
				PxTransform target;
				if( dynamic && ( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) && dynamic->getKinematicTarget( target ) )
				{
					actorPose = target;
				}
				else
				{
					actorPose = rigidActor->getGlobalPose();
				}
				PxMat44 resultPose = actorPose * shapePose;

				PxVec3 pos = resultPose.getPosition();
				pos = inverted.transform( pos );

				resultPose.column3.x = localToWorld.V[ 3 ].X + pos.x;
				resultPose.column3.y = localToWorld.V[ 3 ].Y + pos.y;
				resultPose.column3.z = localToWorld.V[ 3 ].Z + pos.z;

				NxClothingCollision* collision = sceneCollider.m_collisionShapes[ i ];
				if( collision->isSphere() )
				{
					NxClothingSphere* sphere = collision->isSphere();
					PxSphereGeometry geometry;
					shape->getSphereGeometry( geometry );
					sphere->setPosition( resultPose.getPosition() );
					sphere->setRadius( geometry.radius );
				}
				else if( collision->isCapsule() )
				{
					NxClothingCapsule* capsule = collision->isCapsule();
					PxCapsuleGeometry geometry;
					shape->getCapsuleGeometry( geometry );

					PxMat44 orientation = actorPose;
					orientation.column3.setZero();

					NxClothingSphere** sphere = capsule->getSpheres();
					( *sphere )->setPosition( resultPose.getPosition() + orientation.transform( PxVec3( geometry.halfHeight, 0.0f, 0.0f ) ) );
					++sphere;
					( *sphere )->setPosition( resultPose.getPosition() - orientation.transform( PxVec3( geometry.halfHeight, 0.0f, 0.0f ) ) );
				}
				else if( collision->isConvex() )
				{
					NxClothingConvex* convex = collision->isConvex();

					PxVec3 dimensions;

					PxBoxGeometry boxGeometry;

					PxConvexMeshGeometry convexGeometry;
					if( shape->getConvexMeshGeometry( convexGeometry ) )
					{
						dimensions = convexGeometry.convexMesh->getLocalBounds().getExtents();
					}
					else if( shape->getBoxGeometry( boxGeometry ) )
					{
						dimensions = boxGeometry.halfExtents;

					}

					dimensions.x /= scale.X;
					dimensions.y /= scale.Y;
					dimensions.z /= scale.Z;

					convex->getPlanes()[ 0 ]->setPlane(PxPlane(resultPose.transform(PxVec3(dimensions.x, 0.0f, 0.0f)), resultPose.rotate(PxVec3(1.0f, 0.0f, 0.0f))));
					convex->getPlanes()[ 1 ]->setPlane(PxPlane(resultPose.transform(PxVec3(-dimensions.x, 0.0f, 0.0f)), resultPose.rotate(PxVec3(-1.0f, 0.0f, 0.0f))));
					convex->getPlanes()[ 2 ]->setPlane(PxPlane(resultPose.transform(PxVec3(0.0f, dimensions.y, 0.0f)), resultPose.rotate(PxVec3(0.0f, 1.0f, 0.0f))));
					convex->getPlanes()[ 3 ]->setPlane(PxPlane(resultPose.transform(PxVec3(0.0f, -dimensions.y, 0.0f)), resultPose.rotate(PxVec3(0.0f, -1.0f, 0.0f))));
					convex->getPlanes()[ 4 ]->setPlane(PxPlane(resultPose.transform(PxVec3(0.0f, 0.0f, dimensions.z)), resultPose.rotate(PxVec3(0.0f, 0.0f, 1.0f))));
					convex->getPlanes()[ 5 ]->setPlane(PxPlane(resultPose.transform(PxVec3(0.0f, 0.0f, -dimensions.z)), resultPose.rotate(PxVec3(0.0f, 0.0f, -1.0f))));
				}
				else if( collision->isTriangleMesh() )
				{
					NxClothingTriangleMesh* trimesh = collision->isTriangleMesh();

					PxHeightFieldGeometry heightfieldGeometry;

					PxTriangleMeshGeometry trimeshGeometry;
					if( shape->getTriangleMeshGeometry( trimeshGeometry ) )
					{
						trimesh->setPose( resultPose );
					}
					else if( shape->getHeightFieldGeometry( heightfieldGeometry ) )
					{
						PxHeightFieldGeometry heightfieldGeometry;
						shape->getHeightFieldGeometry( heightfieldGeometry );

						PxHeightField* mesh = heightfieldGeometry.heightField;

						PxVec3 actorPos = resultPose.getPosition();

						CPhysicsWrapperInterface* triggerWrapper = sceneCollider.m_info.m_triggerWrapper;

						Box worldBounds = triggerWrapper->GetWorldBounds();
						PxBounds3 bounds( TO_PX_VECTOR( worldBounds.Min ), TO_PX_VECTOR( worldBounds.Max ) );

						Int32 i0, imax, j0, jmax;

						if( actorPos.x < bounds.minimum.x )
						{
							i0 = Int32( ( bounds.minimum.x - actorPos.x ) / heightfieldGeometry.rowScale ) ;
						}
						else
						{
							i0 = 0;
						}

						PxReal heightfieldRowSize = mesh->getNbRows() * heightfieldGeometry.rowScale;
						if( actorPos.x + heightfieldRowSize > bounds.maximum.x )
						{
							imax = i0 + Int32( ( bounds.maximum.x - bounds.minimum.x ) / heightfieldGeometry.rowScale ) + 2;
						}
						else
						{
							imax = ( Int32 ) mesh->getNbRows() - 1;
						}


						if( actorPos.y < bounds.minimum.y )
						{
							j0 = Int32( ( bounds.minimum.y - actorPos.y ) / heightfieldGeometry.columnScale );
						}
						else
						{
							j0 = 0;
						}

						PxReal heightfieldColumnSize = mesh->getNbColumns() * heightfieldGeometry.columnScale;
						if( actorPos.y + heightfieldColumnSize > bounds.maximum.y )
						{
							jmax = j0 + Int32( ( bounds.maximum.y - bounds.minimum.y ) / heightfieldGeometry.columnScale ) + 2;
						}
						else
						{
							jmax = ( Int32 ) mesh->getNbColumns() - 1;
						}

						if( sceneCollider.m_i0 != i0 || sceneCollider.m_imax != imax || sceneCollider.m_j0 != j0 || sceneCollider.m_jmax != jmax )
						{
							sceneCollider.m_i0 = i0;
							sceneCollider.m_imax = imax;
							sceneCollider.m_j0 = j0;
							sceneCollider.m_jmax = jmax;

							PxVec3 p00( (Float)i, (Float)j, 0.0f );
							PxVec3 p01( (Float)i, (Float)( j + 1 ), 0.0f );
							PxVec3 p10( (Float)( i + 1 ), (Float)j , 0.0f );
							PxVec3 p11( (Float)( i + 1 ), (Float)( j + 1 ) , 0.0f );
							p00.z = mesh->getHeight( p00.y, p00.x ) * heightfieldGeometry.heightScale;
							p01.z = mesh->getHeight( p01.y, p01.x ) * heightfieldGeometry.heightScale;
							p10.z = mesh->getHeight( p10.y, p10.x ) * heightfieldGeometry.heightScale;
							p11.z = mesh->getHeight( p11.y, p11.x ) * heightfieldGeometry.heightScale;
							p00.x *= heightfieldGeometry.rowScale;
							p00.y *= heightfieldGeometry.columnScale;
							p01.x *= heightfieldGeometry.rowScale;
							p01.y *= heightfieldGeometry.columnScale;
							p10.x *= heightfieldGeometry.rowScale;
							p10.y *= heightfieldGeometry.columnScale;
							p11.x *= heightfieldGeometry.rowScale;

							p11.y *= heightfieldGeometry.columnScale;

							Vector2 min, max;
							min.X = ( i0 + 1 ) * heightfieldGeometry.rowScale + actorPos.x;
							min.Y = ( j0 + 1 ) * heightfieldGeometry.columnScale + actorPos.y;
							max.X = ( imax - 1 ) * heightfieldGeometry.rowScale + actorPos.x;
							max.Y = ( jmax - 1 ) * heightfieldGeometry.columnScale + actorPos.y;
							Box2 bbox( min, max );
							const PxVec3* currentTriangles = trimesh->getTriangleBuffer();
							Uint32 currentTrianglesCount = trimesh->getNumTriangles();
							for( Int32 i = 0; i < ( Int32 )currentTrianglesCount; i+=2 )
							{
								Vector2 p00( currentTriangles[ i * 3 ].x, currentTriangles[ i * 3 ].y );
								Vector2 p01( currentTriangles[ i * 3 + 1 ].x, currentTriangles[ i * 3 + 1 ].y );
								Vector2 p10( currentTriangles[ i * 3 + 2 ].x, currentTriangles[ i * 3 + 2 ].y );
								Vector2 p11( currentTriangles[ i * 3 + 5 ].x, currentTriangles[ i * 3 + 5 ].y );

								if( !( ( bbox.Contains( p00 ) || bbox.Contains( p01 ) || bbox.Contains( p10 ) || bbox.Contains( p11 ) ) ) )
								{
									trimesh->removeTriangle( i );
									trimesh->removeTriangle( i );
									currentTriangles = trimesh->getTriangleBuffer();
									i-=2;
									currentTrianglesCount-=2;
								}
							}

							for( Int32 i = i0; i < imax; ++i )
							{
								for( Int32 j = j0; j < jmax; ++j )
								{
									PxVec3 p00( (Float)i, (Float)j, 0.0f );
									PxVec3 p01( (Float)i, (Float)( j + 1 ), 0.0f );
									PxVec3 p10( (Float)( i + 1 ), (Float)j , 0.0f );
									PxVec3 p11( (Float)( i + 1 ), (Float)( j + 1 ) , 0.0f );
									p00.z = mesh->getHeight( p00.y, p00.x ) * heightfieldGeometry.heightScale;
									p01.z = mesh->getHeight( p01.y, p01.x ) * heightfieldGeometry.heightScale;
									p10.z = mesh->getHeight( p10.y, p10.x ) * heightfieldGeometry.heightScale;
									p11.z = mesh->getHeight( p11.y, p11.x ) * heightfieldGeometry.heightScale;
									p00.x *= heightfieldGeometry.rowScale;
									p00.y *= heightfieldGeometry.columnScale;
									p01.x *= heightfieldGeometry.rowScale;
									p01.y *= heightfieldGeometry.columnScale;
									p10.x *= heightfieldGeometry.rowScale;
									p10.y *= heightfieldGeometry.columnScale;
									p11.x *= heightfieldGeometry.rowScale;
									p11.y *= heightfieldGeometry.columnScale;

									p00 += actorPos;
									p01 += actorPos;
									p10 += actorPos;
									p11 += actorPos;

									Bool isToAdd = true;
									currentTriangles = trimesh->getTriangleBuffer();
									currentTrianglesCount = trimesh->getNumTriangles();
									for( Uint32 k = 0; k < currentTrianglesCount; k++ )
									{
										if( currentTriangles[ k * 3 ] == p00 && currentTriangles[ k * 3 + 1 ] == p10 && currentTriangles[ k * 3 + 2 ] == p01 &&
											currentTriangles[ k * 3 + 5 ] == p11 )
										{
											isToAdd = false;
											break;
										}
									}

									if( isToAdd )
									{
										if( IsClothGPUSimulationEnabled( m_world ) && m_actor->getCollisionTriangleCount() * 3 >= 498 )
										{
											RED_LOG_ERROR( PhysX, TXT( "cloth vectices number of 500 exceded at %i %s" ) RED_PRIWas, this, component->GetFriendlyName().AsChar() );
											i = imax;
											j = jmax;
											break;
										}

										trimesh->addTriangle( p00, p10, p01 );
										if( IsClothGPUSimulationEnabled( m_world ) && m_actor->getCollisionTriangleCount() * 3 >= 498 )
										{
											RED_LOG_ERROR( PhysX, TXT( "cloth vectices number of 500 exceded at %i %s" ) RED_PRIWas, this, component->GetFriendlyName().AsChar() );
											i = imax;
											j = jmax;
											break;
										}
										trimesh->addTriangle( p01, p10, p11 );
									}
								}
							}
						}

					}

				}
			}
		}
	}

	if( currentPhysicalLod > 0 )
	{
		if( timeDelta > 0.0f )
		{
			if( m_windAdaptationTarget > m_windAdaptationCurrent )
			{
				m_windAdaptationCurrent += timeDelta;
				if( m_windAdaptationTarget < m_windAdaptationCurrent )
				{
					m_windAdaptationCurrent = m_windAdaptationTarget;
				}
			}
			else if( m_windAdaptationTarget < m_windAdaptationCurrent )
			{
				m_windAdaptationCurrent -= timeDelta;
				if( m_windAdaptationTarget > m_windAdaptationCurrent )
				{
					m_windAdaptationCurrent = m_windAdaptationTarget;
				}
			}
			else
			{
				m_windAdaptationTarget = GEngine->GetRandomNumberGenerator().Get< Float >( 0.5f , 1.0f );
			}

			if( IPhysicsWorldParentProvider* provider = m_world->GetWorldParentProvider() )
			{
				Vector wind = provider->GetWindAtPoint( pose.GetTranslationRef() );
				wind = wind * m_windScaler * SPhysicsSettings::m_clothWindScaler;
				wind *= m_windAdaptationTarget;
				m_actor->setWind( m_windAdaptationCurrent, TO_PX_VECTOR( wind ) );
			}
		}

		if( m_updateMotionIntensity)
		{
			PC_SCOPE_PHYSICS(CApexClothWrap PreSim motionintens )

			//this will be slow, but if aproach will be correct it can be done diffrently

			Uint32 count = m_actor->getNumSimulationVertices();
			if( m_previousPositions.Empty() || count > m_previousPositions.Size() )
			{
				m_previousPositions.ResizeFast( count );
				memcpy( m_previousPositions.Data(), m_actor->getSimulationPositions(), count * sizeof( PxVec3 ) );
			}
			else
			{
				m_updateMotionIntensity = false;
				m_motionIntensity = 0.0f;
				for( Uint32 i = 0; i != count; ++i )
				{
					PxVec3 velocityVector = m_previousPositions[ i ] - m_actor->getSimulationPositions()[ i ];
					Float velocity = velocityVector.magnitudeSquared();
					if( m_motionIntensity < velocity )
					{
						m_motionIntensity = velocity;
					}

				}

				if( m_motionIntensity > 0.0f )
				{
					m_motionIntensity = sqrtf( m_motionIntensity );
				}
			}
		}
	}

	UpdateGraphicalLod( distanceFromViewportSquared );
}

void CApexClothWrapper::PostSimulation( Uint8 visibilityQueryResult )
{
	PC_SCOPE_PHYSICS(CApexClothWrapper PostSimulation )

	if( !m_actor ) return;

	// frozen state is set for non-skinned clothing so we dont want to update renderable for them
	// it will be set in presimulate
	if( m_isFrozen ) return;

	CMeshTypeComponent* mtc = nullptr;
	if( !GetParent( mtc ) ) return;

	IRenderProxy* proxy = mtc->GetRenderProxy();
	if( !proxy ) return;

	NxApexRenderable* renderable = AcquireRenderable();
	if (!renderable) return;

	//update bbox for component
	PxBounds3 pxBounds = m_actor->getBounds();
	m_bounds.Min = TO_VECTOR( pxBounds.minimum );
	m_bounds.Max = TO_VECTOR( pxBounds.maximum );

	// propagate to component
	const Box& bbox = GetBounds();
	mtc->SetBoundingBox( bbox );

	( new CRenderCommand_UpdateApexRenderable( proxy, renderable, bbox, m_wrapperTransform, m_wetness ) )->Commit();
}

Bool CApexClothWrapper::FillResourceParameters( SClothParameters& parameters )
{
	CApexResource* resource = parameters.m_resource.Get();
	if( !resource ) return false; 

	NxClothingAsset* asset = GetResourceAsset( parameters );
	if( !asset ) return false;

	NxParameterized::Interface* descParams = asset->getDefaultActorDesc();
	if( !descParams ) return false;
	
	NxParameterized::getParamBool( *descParams, "flags.RecomputeNormals", parameters.m_recomputeNormals );
	NxParameterized::getParamBool( *descParams, "flags.CorrectSimulationNormals", parameters.m_correctSimulationNormals );
	NxParameterized::getParamBool( *descParams, "slowStart", parameters.m_slowStart );
	NxParameterized::getParamBool( *descParams, "useStiffSolver", parameters.m_useStiffSolver);
	NxParameterized::getParamF32( *descParams, "pressure", parameters.m_pressure );
	NxParameterized::getParamF32( *descParams, "lodWeights.distanceWeight", parameters.m_distanceWeight );
	NxParameterized::getParamF32( *descParams, "lodWeights.bias", parameters.m_bias );
	NxParameterized::getParamF32( *descParams, "lodWeights.benefitsBias", parameters.m_benefitsBias );
	NxParameterized::getParamF32( *descParams, "maxDistanceBlendTime", parameters.m_maxDistanceBlendTime );
	NxParameterized::getParamU32( *descParams, "uvChannelForTangentUpdate", parameters.m_uvChannelForTangentUpdate );
	NxParameterized::getParamBool( *descParams, "maxDistanceScale.Multipliable", parameters.m_maxDistanceScaleMultipliable );
	NxParameterized::getParamF32( *descParams, "maxDistanceScale.Scale", parameters.m_maxDistanceScaleScale );
	NxParameterized::getParamF32( *descParams, "clothDescTemplate.collisionResponseCoefficient", parameters.m_collisionResponseCoefficient );
	NxParameterized::getParamBool( *descParams, "allowAdaptiveTargetFrequency", parameters.m_allowAdaptiveTargetFrequency );

	return true;
}

Bool CApexClothWrapper::IsFrozen()
{
	return m_isFrozen;
}

void CApexClothWrapper::GetDebugTriangles( TDynArray< Vector >& verts )
{
	if( !m_actor ) return;

	Uint32 count = m_actor->getNumSimulationVertices();
	if( !count ) return;

	verts.Resize( count );
	const physx::PxVec3* vectors = m_actor->getSimulationPositions();
	for( Uint32 i = 0; i != count; ++i )
	{
		verts.PushBack( TO_VECTOR( vectors[ i ] ) );
	}
}

Bool CApexClothWrapper::IsClothGPUSimulationEnabled( CPhysicsWorld* physicsWorld )
{
	if( SPhysicsSettings::m_dontCreateClothOnGPU ) return false;

#ifdef USE_PHYSX_GPU
	if( !GPhysXEngine->GetCudaContext() ) return false;
	if( static_cast< CPhysicsWorldPhysXImpl* >( physicsWorld )->GetPxScene()->getGpuDispatcher() == nullptr )
	{
		return false;
	}
#endif

	return true;
}

float CApexClothWrapper::GetMotionIntensity()
{
	if( !m_actor ) return 0.0f;
	m_updateMotionIntensity = true;
	return m_motionIntensity;
}

void CApexClothWrapper::SetPose( const Matrix& localToWorld, Uint32 actorIndex )
{
	SetFlag( PRBW_PoseIsDirty, false );
	SetWrapperTransform( localToWorld );
	const Vector& trans = m_wrapperTransform.GetTranslationRef();
	
	SWrapperContext* position = m_world->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	if( !position )
	{
		int a = 0;
	}
	position->m_x = trans.X;
	position->m_y = trans.Y;

	if( !m_actor ) return;

	NxParameterized::Interface* actorDesc = m_actor->getActorDesc();
	PxF32 scale = TO_PX_VECTOR( m_wrapperTransform.GetRow(0) ).magnitude();
	NxParameterized::setParamF32(*actorDesc, "actorScale", scale);

	m_actor->updateState( TO_PX_MAT( m_wrapperTransform ), nullptr, sizeof( PxMat44 ), 0, ClothingTeleportMode::Teleport );
}

void CApexClothWrapper::SetFlag( EPhysicsRigidBodyWrapperFlags flag, Bool decision )
{
	CPhysicsWrapperInterface::SetFlag( flag, decision );
	if( decision && ( flag & PRBW_PoseIsDirty ) )
	{
		m_world->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->PushDirtyWrapper( this );
	}
}

void CApexClothWrapper::UpdateStateFromBonesMatricesBuffer( const Matrix& pose, const Matrix* matrices, Uint32 numMatrices, EClothTeleportMode teleportMode /* =Continuous */, const Box& box )
{
	const Vector trans = box.CalcCenter();
	SWrapperContext* position = m_world->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	if( !position )
	{
		int a = 0;
	}
	position->m_x = trans.X;
	position->m_y = trans.Y;

	if( !m_actor ) return;
	// update with teleport mode.
	m_actor->updateState( TO_PX_MAT( pose ), reinterpret_cast< const physx::PxMat44* >( matrices ), sizeof( PxMat44 ), numMatrices, ClothingTeleportMode::Enum(teleportMode) );
}

void CApexClothWrapper::UpdateWind( const Vector& wind, Float newAdaptation )
{
	PC_SCOPE_PHYSICS(CApexClothWrap UpdateWind )

	if( !m_actor || !m_actor->getActorDesc() ) return;
	NxParameterized::Interface* actorDesc = m_actor->getActorDesc();
	if( wind == Vector::ZEROS ) newAdaptation = 0.0f;
	PxVec3 velocity, newVelocity = TO_PX_VECTOR( wind );
	NxParameterized::getParamVec3( *actorDesc, "windParams.Velocity", velocity );
	if( velocity != newVelocity )
	{
		NxParameterized::setParamVec3( *actorDesc, "windParams.Velocity", TO_PX_VECTOR( wind ) );
	}
	PxF32 adaptation;
	NxParameterized::getParamF32( *actorDesc, "windParams.Adaption", adaptation );
	if( adaptation != newAdaptation )
	{
		NxParameterized::setParamF32(*actorDesc, "windParams.Adaption", newAdaptation );
	}

}

void CApexClothWrapper::AddCollider( const STriggeringInfo& info )
{
	if( !m_actor ) return;

	CPhysicsWrapperInterface* wrapper = info.m_triggeredWrapper;
	if( !wrapper ) return;

	PxShape* shape = ( PxShape* ) wrapper->GetShape( info.m_triggeredBodyIndex.m_shapeIndex, info.m_triggeredBodyIndex.m_actorIndex );
	if( !shape ) return;

	SSceneCollider sceneCollider( info );
	for( auto i : m_currentColliders )
	{
		if( i == sceneCollider )
		{
			if( i.m_isToRemove == false ) return;
		}
	}

	CComponent* parentComponent = nullptr;
	if( !GetParentProvider()->GetParent( parentComponent ) ) return;

	if( shape->getGeometryType() == PxGeometryType::eSPHERE )
	{
		PxSphereGeometry geometry;
		if( shape->getSphereGeometry( geometry ) )
		{
			Float radius = geometry.radius;
			if( radius < SPhysicsSettings::m_clothColiderSphereMinimalRadius )
			{
				radius = SPhysicsSettings::m_clothColiderSphereMinimalRadius;
			}
			NxClothingSphere* sphere = m_actor->createCollisionSphere( PxVec3( 0.0f, 0.0f, 0.0f ), radius );
			sceneCollider.m_collisionShapes.PushBack( sphere );
		}
	}
	else if( shape->getGeometryType() == PxGeometryType::eCAPSULE )
	{
		PxCapsuleGeometry geometry;
		if( shape->getCapsuleGeometry( geometry ) )
		{
			Float radius = geometry.radius;
			if( radius < SPhysicsSettings::m_clothColiderCapsuleMinimalRadius )
			{
				radius = SPhysicsSettings::m_clothColiderCapsuleMinimalRadius;
			}
			NxClothingSphere* sphere1 = m_actor->createCollisionSphere( PxVec3( 0.0f, 0.0f, 0.0f ), radius );
			NxClothingSphere* sphere2 = m_actor->createCollisionSphere( PxVec3( 0.0f, 0.0f, 0.0f ), radius );
			NxClothingCapsule* capsule = m_actor->createCollisionCapsule( *sphere1, *sphere2 );
			sceneCollider.m_collisionShapes.PushBack( capsule );
		}
	}
	else if( shape->getGeometryType() == PxGeometryType::eBOX )
	{
		PxBoxGeometry geometry;
		if( shape->getBoxGeometry( geometry ) )
		{
			TDynArray< NxClothingPlane* > planes;
			for( Uint32 i = 0; i != 6; ++i )
			{
				planes.PushBack( m_actor->createCollisionPlane(PxPlane( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
			}
			NxClothingConvex* convexMesh = m_actor->createCollisionConvex( &planes[ 0 ], planes.Size() );

			sceneCollider.m_collisionShapes.PushBack( convexMesh );
		}
	}
	else if( shape->getGeometryType() == PxGeometryType::eHEIGHTFIELD )
	{
		NxClothingTriangleMesh* triangleMesh = m_actor->createCollisionTriangleMesh();
		sceneCollider.m_collisionShapes.PushBack( triangleMesh );
	}
	else if( shape->getGeometryType() == PxGeometryType::eTRIANGLEMESH )
	{
		PxTriangleMeshGeometry geometry;
		if( shape->getTriangleMeshGeometry( geometry ) && geometry.triangleMesh )
		{
			if( CComponent* component = Cast< CComponent >( sceneCollider.m_info.m_triggeredObject.Get() ) )
			{
				Vector clothScale = parentComponent->GetLocalToWorld().GetScale33();
				Vector resultScale = component->GetLocalToWorld().GetScale33();
				resultScale.X /= clothScale.X;
				resultScale.Y /= clothScale.Y;
				resultScale.Z /= clothScale.Z;

				NxClothingTriangleMesh* triangleMesh = m_actor->createCollisionTriangleMesh();

				PxMat44 shapePose( shape->getLocalPose() );
				PxMat44 actorPose( shape->getActor()->getGlobalPose() );
				PxMat44 resultPose = actorPose * shapePose;

				Uint32 triangleCount = geometry.triangleMesh->getNbTriangles() ;
				{
					PxTriangleMesh* mesh = geometry.triangleMesh;
					unsigned short* triangleIndexes = ( unsigned short* ) mesh->getTriangles();
					const PxVec3* triangleVertex = mesh->getVertices();
					for( Uint32 i = 0; i != triangleCount; i++ )
					{
						if( IsClothGPUSimulationEnabled( m_world ) && m_actor->getCollisionTriangleCount() * 3 >= 498 )
						{
							RED_LOG_ERROR( PhysX, TXT( "cloth vectices number of 500 exceded at %i %s" ) RED_PRIWas, this, component->GetFriendlyName().AsChar() );
							break;
						}
						unsigned short* index0 = triangleIndexes++;
						unsigned short* index1 = triangleIndexes++;
						unsigned short* index2 = triangleIndexes++;
						PxVec3 vec0 = triangleVertex[ *index0 ];
						PxVec3 vec1 = triangleVertex[ *index1 ];
						PxVec3 vec2 = triangleVertex[ *index2 ];

						vec0.x *= resultScale.X;
						vec0.y *= resultScale.Y;
						vec0.z *= resultScale.Z;

						vec1.x *= resultScale.X;
						vec1.y *= resultScale.Y;
						vec1.z *= resultScale.Z;

						vec2.x *= resultScale.X;
						vec2.y *= resultScale.Y;
						vec2.z *= resultScale.Z;

						triangleMesh->addTriangle( vec0, vec1, vec2 );

					}
				}

				sceneCollider.m_collisionShapes.PushBack( triangleMesh );
			}
		}
	}
	else if( shape->getGeometryType() == PxGeometryType::eCONVEXMESH )
	{
		PxConvexMeshGeometry geometry;
		if( shape->getConvexMeshGeometry( geometry ) && geometry.convexMesh )
		{
			TDynArray< NxClothingPlane* > planes;
			for( Uint32 i = 0; i != 6; ++i )
			{
				planes.PushBack( m_actor->createCollisionPlane(PxPlane( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
			}
			NxClothingConvex* convexMesh = m_actor->createCollisionConvex( &planes[ 0 ], planes.Size() );

			sceneCollider.m_collisionShapes.PushBack( convexMesh );
		}
	}

	if( sceneCollider.m_collisionShapes.Empty() ) return;

	m_currentColliders.PushBack( sceneCollider );
}

void CApexClothWrapper::RemoveCollider( const STriggeringInfo& info )
{
	SSceneCollider* sceneCollider = 0;
	if( !info.TriggeredBodyWasRemoved() )
	{
		sceneCollider = m_currentColliders.FindPtr( info ); 
	}
	else for( Uint32 i = 0; i != m_currentColliders.Size(); ++i )
		if( m_currentColliders[ i ].m_info.m_triggeredBodyId == info.m_triggeredBodyId )
		{
			sceneCollider = &m_currentColliders[ i ];
			break;
		}

	if( !sceneCollider ) return;

	sceneCollider->m_isToRemove = true;
	SWrapperContext* position = m_world->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	position->m_requestProcessingFlag = true;
}

Matrix CApexClothWrapper::GetBoneBaseModelSpacePose( Uint32 index )
{
	if( !m_actor ) return Matrix::IDENTITY;
	NxClothingAsset* asset = reinterpret_cast<physx::apex::NxClothingAsset*>( m_actor->getOwner() );
	if( !asset ) return Matrix::IDENTITY;
	PxMat44 pose;
	if( !asset->getBoneBasePose( index, pose ) ) return Matrix::IDENTITY;
	return TO_MAT( pose );
}

Vector CApexClothWrapper::GetFirstSimulationPosition()
{
	if( !m_actor ) return Vector::ZERO_3D_POINT;

	const PxVec3* pos = m_actor->getSimulationPositions();
	if( !pos ) return Vector::ZERO_3D_POINT;
	PxVec3 firstPos = *pos;
	firstPos = m_actor->getGlobalPose().transform( firstPos );
	return TO_VECTOR( firstPos );
}

Uint32 CApexClothWrapper::SelectVertex( const Vector& worldPos, const Vector& worldDir, Vector& hitPos )
{
	if( !m_actor ) return 0;
	physx::PxF32 time;
	physx::PxVec3 normal;
	physx::PxU32 vertexIndex;

	Uint32 count = m_actor->getNumSimulationVertices();
	if( !count ) return 0;

	Bool hit = m_actor->rayCast(TO_PX_VECTOR(worldPos), TO_PX_VECTOR(worldDir), time, normal, vertexIndex);
	if(hit)
	{
		hitPos = worldPos + worldDir * time;
		return static_cast< Uint32 >( vertexIndex );
	}

	return 0;
}

void CApexClothWrapper::MoveVertex( Uint32 vertexIndex, const Vector& worldPos )
{
	m_actor->attachVertexToGlobalPosition(static_cast<physx::PxU32>(vertexIndex), TO_PX_VECTOR(worldPos));
}

void CApexClothWrapper::FreeVertex( Uint32 vertexIndex )
{
	m_actor->freeVertex(static_cast<physx::PxU32>(vertexIndex));
}

physx::apex::NxApexActor* CApexClothWrapper::GetApexActor()
{
	return m_actor;
}

physx::apex::NxApexRenderable* CApexClothWrapper::AcquireRenderable()
{
	//temp solution 
	return m_actor ? m_actor->acquireRenderProxy() : nullptr;
}

void CApexClothWrapper::ReleaseRenderable( physx::apex::NxApexRenderable* renderable )
{
	NxClothingRenderProxy* rend = static_cast< NxClothingRenderProxy* >( renderable );
	rend->release();
}

#ifndef NO_EDITOR

void CApexClothWrapper::GetDebugVertex( TDynArray< DebugVertex >& debugVertex )
{
	Uint32 collidersCount = m_currentColliders.Size();
	for( Uint32 j = 0; j != collidersCount; ++j )
	{
		const CApexClothWrapper::SSceneCollider& sceneCollider = m_currentColliders[ j ];
		Uint32 collisionShapesCount = sceneCollider.m_collisionShapes.Size();
		for( Uint32 i = 0; i != collisionShapesCount; ++i )
		{
			NxClothingCollision* collision = sceneCollider.m_collisionShapes[ i ];
			NxClothingCapsule* capsule = collision->isCapsule();
			if( capsule )
			{
				NxClothingSphere** sphere = capsule->getSpheres();

				const Uint32 numRings = 5;
				const Uint32 numPoints = 12;
				const Vector uaxes[3] = { Vector::EY, Vector::EX, Vector::EX };
				const Vector vaxes[3] = { Vector::EZ, Vector::EZ, Vector::EY };
				const Vector waxes[3] = { Vector::EX, Vector::EY, Vector::EZ };
				const Float angletable[5] = { 30.0f, 60.0f, 90.0f, 120.0f, 150.0f };

				debugVertex.Reserve( debugVertex.Size() + 3 * numRings * numPoints * 2 );

				Float radius = ( *sphere )->getRadius();
				Matrix matrix = Matrix::IDENTITY;

				Vector center = TO_VECTOR( ( *sphere )->getPosition() );

				for ( Uint32 axis=0; axis<3; axis++ )
				{
					for ( Uint32 ring=0; ring<numRings; ring++ )
					{			
						const Float angle = DEG2RAD( angletable[ring] );
						// Generate points			
						TStaticArray< Vector, numPoints + 1 > ringPoints;
						for ( Uint32 i=0; i<=numPoints; i++ )
						{
							const Float localAngle = 2.0f * M_PI * ( i / (Float)numPoints );
							const Float u = cos( localAngle ) * sin( angle ) * radius;
							const Float v = sin( localAngle ) * sin( angle ) * radius;
							const Float w = cos( angle ) * radius;
							Vector point = ( uaxes[axis] * u ) + ( vaxes[axis] * v ) + ( waxes[axis] * w );
							ringPoints.PushBack( center + matrix.TransformPoint( point ) );
						}

						// Generate lines
						for ( Uint32 i=0; i<numPoints; i++ )
						{
							debugVertex.PushBack( DebugVertex( ringPoints[i], Color::GREEN ) );
							debugVertex.PushBack( DebugVertex( ringPoints[i+1], Color::GREEN ) );
						}
					}
				}

				++sphere;

				debugVertex.Grow( 3 * numRings * numPoints * 2 );

				center = TO_VECTOR( ( *sphere )->getPosition() );

				for ( Uint32 axis=0; axis<3; axis++ )
				{
					for ( Uint32 ring=0; ring<numRings; ring++ )
					{			
						const Float angle = DEG2RAD( angletable[ring] );
						// Generate points			
						TStaticArray< Vector, numPoints + 1 > ringPoints;
						for ( Uint32 i=0; i<=numPoints; i++ )
						{
							const Float localAngle = 2.0f * M_PI * ( i / (Float)numPoints );
							const Float u = cos( localAngle ) * sin( angle ) * radius;
							const Float v = sin( localAngle ) * sin( angle ) * radius;
							const Float w = cos( angle ) * radius;
							Vector point = ( uaxes[axis] * u ) + ( vaxes[axis] * v ) + ( waxes[axis] * w );
							ringPoints.PushBack( center + matrix.TransformPoint( point ) );
						}

						// Generate lines
						for ( Uint32 i=0; i<numPoints; i++ )
						{
							debugVertex.PushBack( DebugVertex( ringPoints[i], Color::GREEN ) );
							debugVertex.PushBack( DebugVertex( ringPoints[i+1], Color::GREEN ) );
						}
					}
				}

				continue;
			}

			NxClothingTriangleMesh* trimesh = collision->isTriangleMesh();
			if( trimesh )
			{
				CPhysicsWrapperInterface* wrapper = sceneCollider.m_info.m_triggeredWrapper;
				PxTriangleMeshGeometry geometry;
				PxShape* shape = ( PxShape* ) wrapper->GetShape( sceneCollider.m_info.m_triggeredBodyIndex.m_shapeIndex, sceneCollider.m_info.m_triggeredBodyIndex.m_actorIndex );
				if( shape && shape->getTriangleMeshGeometry( geometry ) && geometry.triangleMesh )
				{
					Uint32 triangleCount = geometry.triangleMesh->getNbTriangles();
					debugVertex.Reserve( debugVertex.Size() + 6 * triangleCount );
					PxTriangleMesh* mesh = geometry.triangleMesh;
					unsigned short* triangleIndexes = ( unsigned short* ) mesh->getTriangles();
					const PxVec3* triangleVertex = mesh->getVertices();
					for( Uint32 i = 0; i < triangleCount; ++i )
					{
						unsigned short* index0 = triangleIndexes++;
						unsigned short* index1 = triangleIndexes++;
						unsigned short* index2 = triangleIndexes++;
						PxVec3 vec0 = triangleVertex[ *index0 ];
						PxVec3 vec1 = triangleVertex[ *index1 ];
						PxVec3 vec2 = triangleVertex[ *index2 ];

						PxMat44 shapePose( shape->getLocalPose() );
						PxMat44 actorPose( shape->getActor()->getGlobalPose() );
						PxMat44 resultPose = actorPose * shapePose;

						vec0 = resultPose.transform( vec0 );
						vec1 = resultPose.transform( vec1 );
						vec2 = resultPose.transform( vec2 );

						debugVertex.PushBack( DebugVertex( TO_VECTOR( vec0 ), Color::GREEN ) );
						debugVertex.PushBack( DebugVertex( TO_VECTOR( vec1 ), Color::GREEN ) );
						debugVertex.PushBack( DebugVertex( TO_VECTOR( vec1 ), Color::GREEN ) );
						debugVertex.PushBack( DebugVertex( TO_VECTOR( vec2 ), Color::GREEN ) );
						debugVertex.PushBack( DebugVertex( TO_VECTOR( vec0 ), Color::GREEN ) );
						debugVertex.PushBack( DebugVertex( TO_VECTOR( vec2 ), Color::GREEN ) );
					}

				}
			}
		}
	}

}

#endif

#endif
