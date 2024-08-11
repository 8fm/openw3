/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "apexClothWrapper.h"

#include "clothComponent.h"
#include "../physics/physicsIncludes.h"
#include "../physics/physicsEngine.h"
#include "../physics/physicsWorldPhysXImpl.h"
#include "renderCommands.h"
#include "apexResource.h"
#include "apexClothResource.h"
#include "../core/dataError.h"
#include "../core/scriptStackFrame.h"
#include "collisionCache.h"

#ifdef USE_APEX
#include "NxParamUtils.h"
#include "NxResourceProvider.h"
#include "NxClothingActor.h"
#include "NxClothingAsset.h"

#endif
#include "../physics/physicsSettings.h"
#include "../physics/physicsSimpleBodyWrapper.h"
#include "renderFragment.h"
#include "meshSkinningAttachment.h"
#include "renderSkinningData.h"
#include "skeletonProvider.h"
#include "renderProxy.h" 
#include "world.h"
#include "layer.h"
#include "entity.h"
#include "tickManager.h"
#include "memory.h"
#include "phantomComponent.h"
#include "component.h"
#include "physicsDataProviders.h"

SClothParameters::SClothParameters()
	: m_dispacherSelection( EDS_GPU_IF_AVAILABLE )
	, m_recomputeNormals( false )
	, m_correctSimulationNormals( true )
	, m_slowStart( false )
	, m_useStiffSolver( false )
	, m_pressure( 0.0f )
	, m_simulationMaxDistance( -1.0f )
	, m_distanceWeight( 1.0f )
	, m_bias( 0.0f )
	, m_benefitsBias( 0.0f )
	, m_maxDistanceBlendTime( 1.0f )
	, m_uvChannelForTangentUpdate( 0 )
	, m_maxDistanceScaleMultipliable( true )
	, m_maxDistanceScaleScale( 1.0f )
	, m_collisionResponseCoefficient( 1.0f )
	, m_allowAdaptiveTargetFrequency( true )
	, m_windScaler( 1.0f )
#ifndef NO_EDITOR
	, m_usePreviewAsset( false )
#endif
{}

IMPLEMENT_ENGINE_CLASS( CClothComponent );
IMPLEMENT_RTTI_ENUM( ETriggerShape );

CClothComponent::CClothComponent()
	: m_triggerType( TS_None )
	, m_triggerDimensions( 0.0f, 0.0f, 0.0f )
	, m_triggerWrapper( nullptr )
	, m_triggerLocalOffset( Matrix::IDENTITY )
#ifdef USE_APEX
	, m_clothWrapper( nullptr )
#endif
	, m_shadowDistanceOverride( -1.0f )
	, m_shouldUpdateWetness( false )
	, m_teleportRequested( false )
#ifndef NO_EDITOR_FRAGMENTS
	, m_isSkinnningUpdating( false )
#endif //NO_EDITOR_FRAGMENTS
	, m_cachedLastSkinningMatrices( nullptr )
	, m_cachedLastSkinningMatricesCount( 0 )
{
}

void CClothComponent::OnParentAttachmentAdded( IAttachment* attachment )
{
	TBaseClass::OnParentAttachmentAdded( attachment );

	ClearCachedSkinningMatrices();

#ifdef USE_APEX
	CWorld* world = GetWorld();
	if( !world ) return;

	ISkeletonDataProvider* provider = const_cast< ISkeletonDataProvider* >( GetProvider() );
	if( !provider ) return;

#ifndef RED_FINAL_BUILD
	if( SPhysicsSettings::m_dontCreateCloth )
	{
		return;
	}
#endif

	CPhysicsWorld* physcWorld = nullptr;
	world->GetPhysicsWorldSecondary( physcWorld );
	if( m_parameters.m_dispacherSelection == EDS_GPU_ONLY && !CApexClothWrapper::IsClothGPUSimulationEnabled( physcWorld ) )
	{
		return;
	}

#ifdef NO_EDITOR
	if( !( m_parameters.m_resource && GCollisionCache->HasCollision( m_parameters.m_resource->GetDepotPath(), m_parameters.m_resource->GetFileTime() ) ) )
	{
		return;
	}
#endif

	if( physcWorld )
	{
		if( m_clothWrapper )
		{
			if( m_clothWrapper->GetPhysicsWorld() == physcWorld )
			{
				return;
			}
			( ( IRenderObject* ) m_clothWrapper )->Release();
			m_clothWrapper = nullptr;
		}
		if ( m_clothWrapper == nullptr )
		{
			// fill parameters
			m_parameters.m_isClothSkinned = attachment->ToSkinningAttachment() ? true : false;

			// Always set valid position to wrapper
			const CNode* parentAtt = attachment ? attachment->GetParent() : nullptr;
			if ( parentAtt && parentAtt->IsAttached() )
			{
				// Parent node is attached to the world
				m_parameters.m_pose = parentAtt->GetLocalToWorld();
			}
			else
			{
				// Parent is not attached to the world - we use entity's position because it is more or less valid. 
				// The proper transform will be set this frame in OnUpdateSkinning (with proper parent transform as well).
				m_parameters.m_pose = GetEntity()->GetLocalToWorld();
			}

			m_clothWrapper = physcWorld->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( this ), physcWorld, &m_parameters, GetEntity()->GetVisibilityQuery() );

			SWrapperContext* context = physcWorld->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->GetContext( m_clothWrapper );

			// update autohide distances
			Float desiredDistance = GetAutoHideDistance();
			context->m_desiredDistanceSquared = desiredDistance;
			m_clothWrapper->SetAutoHideDistance( context->m_desiredDistanceSquared );
			context->m_desiredDistanceSquared *= context->m_desiredDistanceSquared;
			CheckWetnessSupport( attachment->GetParent()->FindParent<CEntity>() );
			if( m_parameters.m_isClothSkinned )
			{
				RequestTeleport();
			}
		}
	}

#endif	//USE_APEX
}

//////////////////////////////////////////////////////////////
void CClothComponent::OnParentAttachmentBroken( IAttachment* attachment )
{
	TBaseClass::OnParentAttachmentBroken( attachment );

	ClearCachedSkinningMatrices();
}

void CClothComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	{
		PC_SCOPE_PIX( CClothComponent_OnAttached );

		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_BBoxesCloth );
		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_PhantomShapes );
		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_TeleportDetector );

		Bool isSkinned = false;
#ifdef USE_APEX
		CPhysicsWorldPhysXImpl* physicalWorld;
		ISkeletonDataProvider* provider = const_cast< ISkeletonDataProvider* >( GetProvider() );

		if( !provider )
		{
			 GetWorld()->GetPhysicsWorld( physicalWorld );
		}
		else
		{
			GetWorld()->GetPhysicsWorldSecondary( physicalWorld );
			isSkinned = true;
		}
		if( physicalWorld )
		{
#ifndef RED_FINAL_BUILD
			if( !SPhysicsSettings::m_dontCreateCloth )
#endif
			{
				if( !( m_parameters.m_dispacherSelection == EDS_GPU_ONLY && !CApexClothWrapper::IsClothGPUSimulationEnabled( physicalWorld ) ) )
				{
#ifdef NO_EDITOR
					if( m_parameters.m_resource && GCollisionCache->HasCollision( m_parameters.m_resource->GetDepotPath(), m_parameters.m_resource->GetFileTime() ) )
#endif
					{
						if ( m_clothWrapper == nullptr )
						{
							// Why do we need this?
							ForceUpdateTransformNodeAndCommitChanges();

							// fill some extra params
							m_parameters.m_pose = GetLocalToWorld();
							m_parameters.m_isClothSkinned = isSkinned;

							m_clothWrapper = physicalWorld->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( this ),  physicalWorld, &m_parameters, GetEntity()->GetVisibilityQuery() );
							if( m_clothWrapper )
							{
								SWrapperContext* context = physicalWorld->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->GetContext( m_clothWrapper );

								// update autohide distances
								Float desiredDistance = GetAutoHideDistance();
								context->m_desiredDistanceSquared = desiredDistance;
								m_clothWrapper->SetAutoHideDistance( context->m_desiredDistanceSquared );
								context->m_desiredDistanceSquared *= context->m_desiredDistanceSquared;
								if( isSkinned )
								{
									RequestTeleport();
								}
								CheckWetnessSupport( GetEntity() );
							}
						}
						if( m_clothWrapper )
						{
							SWrapperContext* context = physicalWorld->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->GetContext( m_clothWrapper );
							TRenderVisibilityQueryID vsQueryid = GetEntity()->GetVisibilityQuery();
							if( vsQueryid != context->m_visibilityQueryId )
							{
								context->m_visibilityQueryId = vsQueryid;
							}
						}
					}
				}
			}
		}
#endif
	}
}

void CClothComponent::OnDetached( CWorld* world )
{
	ClearCachedSkinningMatrices();

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_BBoxesCloth );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_PhantomShapes );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_TeleportDetector );

#ifdef USE_APEX
	if( m_clothWrapper )
	{
		( ( IRenderObject* ) m_clothWrapper )->Release();
		m_clothWrapper = nullptr;
	}
	if( m_triggerWrapper ) 
	{
		m_triggerWrapper->Release();
		m_triggerWrapper = nullptr;
	}
#endif
	TBaseClass::OnDetached( world );
}

Bool CClothComponent::CanAttachToRenderScene() const
{
#ifdef USE_APEX
	if( !m_clothWrapper ) return false;
	if( m_clothWrapper->IsToDestroy() ) return false;
	if( GetTransformParent() )
	{
		CMeshSkinningAttachment* skinAtt = GetTransformParent()->ToSkinningAttachment();
		if ( skinAtt && !skinAtt->IsSkinningMappingValid() ) return false;
	}
#endif
	return TBaseClass::CanAttachToRenderScene();
}

const ISkeletonDataProvider* CClothComponent::GetProvider() const
{
	CHardAttachment* attachment = GetTransformParent();
	if( !attachment ) return 0;

	CNode* parentNode = attachment->GetParent();
	if( !parentNode ) return 0;

	const ISkeletonDataProvider* provider = parentNode->QuerySkeletonDataProvider();
	return provider;
}

void CClothComponent::SetResource( CResource* resource )
{
	RED_ASSERT( !resource || resource->IsA< CApexClothResource >(), TXT("Cannot set '%ls' to '%ls' component."),resource->GetFile()->GetFileName().AsChar(), m_name.AsChar() ); 
	CApexClothResource* clothRes = Cast< CApexClothResource >( resource );
	if ( clothRes != nullptr || resource == nullptr )
	{
		m_parameters.m_resource = THandle<CApexClothResource>(clothRes);
#ifdef USE_APEX
		CApexClothWrapper::FillResourceParameters( m_parameters );
		// Refresh
		if ( m_clothWrapper )
		{
			m_clothWrapper->RequestReCreate( m_parameters );
			// We aren't creating a new wrapper, but we need to pass along a new auto-hide distance.
			if ( clothRes )
			{
				m_clothWrapper->SetAutoHideDistance( clothRes->GetAutoHideDistance() );
			}
			SheduleRenderingResourceChange();
		}
#endif
	}
}


Float CClothComponent::GetShadowDistance( Uint8& outRenderMask ) const
{
	// No resource and no override, we can't really get a default...
	if ( m_shadowDistanceOverride < 0.0f && m_parameters.m_resource == nullptr )
	{
		return 0.0f;
	}


	CWorld* world = GetWorld();
	RED_ASSERT( world != nullptr, TXT("Requesting shadow distance and I'm not attached to a world") );

	Float extent = GetBoundingBox().CalcExtents().Mag3();

	outRenderMask = MCR_Scene | MCR_LocalShadows | MCR_Cascade1 | MCR_Cascade2;

	Float shadowDistance = m_shadowDistanceOverride;
	if ( shadowDistance < 0.0f )
	{
		shadowDistance = m_parameters.m_resource->CalcShadowDistance( world, GetBoundingBox() );
	}

	// Extend render mask if needed.
	if ( shadowDistance + extent > world->GetShadowConfig().m_cascadeRange2 )
	{
		outRenderMask |= MCR_Cascade3;
	}
	if ( shadowDistance + extent > world->GetShadowConfig().m_cascadeRange3 )
	{
		WARN_ENGINE( TXT("Cloth component '%ls' has shadow reaching into cascade 4! Seriously consider shortening the distance! If this is using default distance (-1), and so is the resource, then it's probably a code bug"), GetFriendlyName().AsChar() );
		outRenderMask |= MCR_Cascade4;
	}

	return shadowDistance;
}


void CClothComponent::onTriggerEntered( const STriggeringInfo& info )
{
#ifdef USE_APEX
	if( !m_clothWrapper ) return;

	m_clothWrapper->AddCollider( info );
#endif
}

void CClothComponent::onTriggerExited( const STriggeringInfo& info )
{
#ifdef USE_APEX
	if( !m_clothWrapper ) return;

	m_clothWrapper->RemoveCollider( info );
#endif
}

#ifndef NO_EDITOR_FRAGMENTS
void CClothComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	if ( IsAttached() && !IsAttaching() )
	{
#ifdef USE_APEX
		if ( flag == SHOW_BBoxesCloth )
		{
			if ( m_clothWrapper && m_clothWrapper->GetPhysicsWorld() )
			{
				frame->AddDebugBox( m_boundingBox, Matrix::IDENTITY, Color::RED );
				frame->AddDebugAxis( m_localToWorld.GetTranslationRef(), m_localToWorld, 0.1f, true );
				Float distFromViewport = 0.f;

				CPhysicsWorld* world = m_clothWrapper->GetPhysicsWorld();
				if( CPhysicsWorldPhysXImpl* physcWorld = static_cast< CPhysicsWorldPhysXImpl* >( world ) )
				{
					SWrapperContext* context = physcWorld->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->GetContext( m_clothWrapper );
					distFromViewport = context->m_resultDistanceSquared;
				}

				String dist = String::Printf( TXT("%.2f"), Red::Math::MSqrt( distFromViewport ) );
				Color col = Color::RED;
				if ( m_clothWrapper->IsSimulating() )
				{
					col = Color::GREEN;
				}
				frame->AddDebugText( m_boundingBox.CalcCenter(), dist, 0, 0, false, col );

				if( m_isSkinnningUpdating )
				{
					col = Color::GREEN;
				}
				else
				{
					col = Color::RED;
				}
				dist = m_isSkinnningUpdating ? TXT("Skinning updating") : TXT("No skinning ATM");
				frame->AddDebugText( m_boundingBox.CalcCenter(), dist, 0, 2, false, col );
			}
		}

		if ( flag == SHOW_PhantomShapes )
		{
			const Vector& scale = m_localToWorld.GetScale33();
			Vector dimensions;
			// verify shape size
			dimensions.X = Clamp< Float > ( m_triggerDimensions.X, 0.05f, FLT_MAX );
			dimensions.Y = Clamp< Float > ( m_triggerDimensions.Y, 0.05f, FLT_MAX );
			dimensions.Z = Clamp< Float > ( m_triggerDimensions.Z, 0.05f, FLT_MAX );
			dimensions *= scale;
			Matrix matrix = Matrix::IDENTITY;
			matrix.SetTranslation( m_triggerLocalOffset.GetTranslationRef() * scale );
			matrix = matrix * GetLocalToWorld();
			switch( m_triggerType )
			{
			case TS_Sphere:
				{
					frame->AddDebugSphere( Vector::ZERO_3D_POINT, dimensions.X, matrix, Color::RED );
					break;
				}
			case TS_Box:
				{
					Box box( -dimensions / 2.0f, dimensions / 2.0f );
					frame->AddDebugBox( box, matrix, Color::RED );
					break;
				}
			}
		}
#endif //USE_APEX
	}
}
#endif //NO_EDITOR_FRAGMENTS

Uint32 CClothComponent::SelectVertex( const Vector& worldPos, const Vector& worldDir, Vector& hitPos )
{
#ifdef USE_APEX
	if( !m_clothWrapper ) return 0;
	return m_clothWrapper->SelectVertex(worldPos, worldDir, hitPos);
#else
	return 0;
#endif
}

void CClothComponent::MoveVertex( Uint32 vertexIndex, const Vector& worldPos )
{
#ifdef USE_APEX
	if( !m_clothWrapper ) return;
	m_clothWrapper->MoveVertex(vertexIndex, worldPos);
#endif
}

void CClothComponent::FreeVertex( Uint32 vertexIndex )
{
#ifdef USE_APEX
	if( !m_clothWrapper ) return;
	m_clothWrapper->FreeVertex(vertexIndex);
#endif
}

void CClothComponent::SetMaxDistanceScale( Float ratio )
{
#ifdef USE_APEX
	if( !m_clothWrapper ) return;
	ratio = Clamp< Float >( ratio, 0.f, 1.f );
	m_clothWrapper->SetMaxDistanceScale( ratio );
#endif
}

void CClothComponent::SetMaxDistanceBlendTime( Float ratio )
{
#ifdef USE_APEX
	if( !m_clothWrapper ) return;
	ratio = Max< Float >( 0.01f, ratio );
	m_clothWrapper->SetMaxDistanceBlendTime( ratio );
#endif
}

void CClothComponent::FreezeCloth( Bool shouldFreeze /*=false*/ )
{
#ifdef USE_APEX
	if( !m_clothWrapper ) return;
	m_clothWrapper->FreezeCloth( shouldFreeze );
#endif
}

void CClothComponent::SetSimulated( Bool simulated )
{
#ifdef USE_APEX
	if( !m_clothWrapper ) return;
	m_clothWrapper->SwitchToKinematic( !simulated );
#endif
}


#ifndef NO_EDITOR

void CClothComponent::ForceLODLevel( Int32 lodOverride )
{
#ifdef USE_APEX
	if ( m_clothWrapper )
	{
		m_clothWrapper->ForceLODLevel( lodOverride );
	}
#endif
}

void CClothComponent::OnPropertyPostChange( IProperty* property )
{
	if ( property->GetName() == TXT("m_windScaler") || property->GetName() == TXT("m_windAdaptation") ) return;

	if ( property->GetName() == TXT("m_resource") )
	{
#ifdef USE_APEX
		if( !CApexClothWrapper::FillResourceParameters( m_parameters ) ) return;
#endif
		EDITOR_DISPATCH_EVENT( CNAME( RefreshPropertiesPage ), NULL );
	}

	CEntity* entity = GetEntity();
	IRenderScene* ex = GetLayer()->GetWorld()->GetRenderSceneEx();

	Bool refreshProxy = m_renderProxy && IsAttached() && GetLayer()->IsAttached();
	if ( refreshProxy )
	{
		( new CRenderCommand_RemoveProxyFromScene( ex, m_renderProxy ) )->Commit();

		m_renderProxy->Release();
		m_renderProxy = nullptr;
		entity->OnProxyDetached( this );

	}

#ifdef USE_APEX
	if( m_clothWrapper )
	{
		( ( IRenderObject* ) m_clothWrapper )->Release();
		m_clothWrapper = nullptr;
		CPhysicsWorldPhysXImpl* physicalWorld = nullptr;
		GetWorld()->GetPhysicsWorld( physicalWorld );
		CompiledCollisionPtr compiledCollision = CompiledCollisionPtr();
		m_clothWrapper = physicalWorld->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( this ), physicalWorld, &m_parameters );
		RequestTeleport();
	}
#endif

}


void CClothComponent::SetUsePreview( Bool usePreview )
{
	m_parameters.m_usePreviewAsset = usePreview;
#ifdef USE_APEX
	if ( m_clothWrapper )
	{
		m_clothWrapper->RequestReCreate( m_parameters );
		SheduleRenderingResourceChange();
	}
#endif
}

void CClothComponent::CopyParameters( CPhantomComponent* phantomComponent )
{
	phantomComponent->GetTransform().CalcLocalToWorld( m_triggerLocalOffset );
	if( phantomComponent->GetShapeType() == PS_Box || phantomComponent->GetShapeType() == PS_EntityBounds )
	{
		m_triggerType = TS_Box;
	}
	else if( phantomComponent->GetShapeType() == PS_Sphere )
	{
		m_triggerType = TS_Sphere;
	}
	m_triggerDimensions = phantomComponent->GetDimensions();
	m_triggeringCollisionGroupNames = phantomComponent->GetTriggeringCollisionGroupNames();
}

#endif

void CClothComponent::OnCutsceneStarted()
{
	TBaseClass::OnCutsceneStarted();
#ifdef USE_APEX
	if( m_clothWrapper )
	{
		CPhysicsWorld* world = m_clothWrapper->GetPhysicsWorld();
		if( CPhysicsWorldPhysXImpl* physcWorld = static_cast< CPhysicsWorldPhysXImpl* >( world ) )
		{
			SWrapperContext* context = physcWorld->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->GetContext( m_clothWrapper );
			context->m_desiredDistanceSquared = FLT_MAX;
		}
	}
#endif
}

void CClothComponent::OnCutsceneEnded()
{
	TBaseClass::OnCutsceneEnded();
#ifdef USE_APEX
	if( m_clothWrapper )
	{
		CPhysicsWorld* world = m_clothWrapper->GetPhysicsWorld();
		if( CPhysicsWorldPhysXImpl* physcWorld = static_cast< CPhysicsWorldPhysXImpl* >( world ) )
		{
			SWrapperContext* context = physcWorld->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->GetContext( m_clothWrapper );
			context->m_desiredDistanceSquared = GetAutoHideDistance();
			context->m_desiredDistanceSquared *= context->m_desiredDistanceSquared;
		}
	}
#endif
}

void CClothComponent::OnCinematicStorySceneStarted()
{
	TBaseClass::OnCinematicStorySceneStarted();
#ifdef USE_APEX
	if( m_clothWrapper )
	{
		CPhysicsWorld* world = m_clothWrapper->GetPhysicsWorld();
		if( CPhysicsWorldPhysXImpl* physcWorld = static_cast< CPhysicsWorldPhysXImpl* >( world ) )
		{
			SWrapperContext* context = physcWorld->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->GetContext( m_clothWrapper );
			context->m_desiredDistanceSquared = FLT_MAX;
		}
	}
#endif
}

void CClothComponent::OnCinematicStorySceneEnded()
{
	TBaseClass::OnCinematicStorySceneEnded();
#ifdef USE_APEX
	if( m_clothWrapper )
	{
		CPhysicsWorld* world = m_clothWrapper->GetPhysicsWorld();
		if( CPhysicsWorldPhysXImpl* physcWorld = static_cast< CPhysicsWorldPhysXImpl* >( world ) )
		{
			SWrapperContext* context = physcWorld->GetWrappersPool< CApexClothWrapper, SWrapperContext >()->GetContext( m_clothWrapper );
			context->m_desiredDistanceSquared = GetAutoHideDistance();
			context->m_desiredDistanceSquared *= context->m_desiredDistanceSquared;
		}
	}
#endif
}

CMeshTypeResource* CClothComponent::GetMeshTypeResource() const
{ 
	return m_parameters.m_resource.Get(); 
}

void CClothComponent::GetResource( TDynArray< const CResource* >& resources ) const
{
	resources.PushBack( m_parameters.m_resource.Get() );
}

#ifdef USE_APEX
const TDynArray< String >& CClothComponent::GetApexMaterialNames() const 
{ 
	return m_parameters.m_resource.Get()->GetApexMaterialNames(); 
}

const CMeshTypeResource::TMaterials& CClothComponent::GetMaterials() const 
{ 
	return m_parameters.m_resource.Get()->GetMaterials(); 
}
#endif

void CClothComponent::ActivateTrigger()
{
#ifndef RED_FINAL_BUILD
	if( SPhysicsSettings::m_dontCreateTriggers ) return;
#endif

	if( m_triggerType == TS_None ) return;
	if ( m_triggerWrapper ) return;

	CPhysicsWorld* physicsWorld = nullptr;
	if( !GetWorld()->GetPhysicsWorld( physicsWorld ) ) return;

	CPhysicsEngine::CollisionMask collisionGroup = GPhysicEngine->GetCollisionTypeBit( m_triggeringCollisionGroupNames );
	if( collisionGroup == 0  )
	{
		collisionGroup = GPhysicEngine->GetWithAllCollisionMask(); 
	}

	Vector dimensions;
	// verify shape size
	dimensions.X = Clamp< Float > ( m_triggerDimensions.X, 0.05f, FLT_MAX );
	dimensions.Y = Clamp< Float > ( m_triggerDimensions.Y, 0.05f, FLT_MAX );
	dimensions.Z = Clamp< Float > ( m_triggerDimensions.Z, 0.05f, FLT_MAX );
	dimensions *= m_localToWorld.GetScale33();

	if( m_localToWorld.GetTranslationRef() == Vector::ZEROS )
	{
		m_localToWorld = Matrix::IDENTITY;
	}

	{
		CompiledCollisionPtr ptr;
		switch( m_triggerType )
		{
		case TS_Sphere:
			{
				m_triggerWrapper = physicsWorld->GetWrappersPool< CPhysicsSimpleWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( this ), 0, collisionGroup, ptr, nullptr, &dimensions.X, m_triggerLocalOffset );
				break;
			}
		case TS_Box:
		default:
			{
				m_triggerWrapper = physicsWorld->GetWrappersPool< CPhysicsSimpleWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( this ), 0, collisionGroup, ptr, &dimensions, nullptr, m_triggerLocalOffset );
				break;
			}
		}
	}

	if( !m_triggerWrapper ) return;
	m_triggerWrapper->SwitchToKinematic( true );

	m_triggerWrapper->SetCodeCallback( CPhysicsWrapperInterface::EPCCT_OnTriggerFocusFound, this, this );
	m_triggerWrapper->SetCodeCallback( CPhysicsWrapperInterface::EPCCT_OnTriggerFocusLost, this, this );

}

void CClothComponent::funcSetSimulated( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, simulated, false );
	FINISH_PARAMETERS;

	SetSimulated( simulated );

	RETURN_VOID();
}

void CClothComponent::funcSetMaxDistanceScale( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, simulationDistanceScale, false );
	FINISH_PARAMETERS;

	SetMaxDistanceScale( simulationDistanceScale );

	RETURN_VOID();
}

void CClothComponent::funcSetFrozen( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, frozen, false );
	FINISH_PARAMETERS;

	FreezeCloth( frozen );

	RETURN_VOID();
}

void CClothComponent::OnUpdateSkinning( const ISkeletonDataProvider* provider, IRenderSkinningData* renderSkinningData, const Box& boxMS, const Matrix& l2w, SMeshSkinningUpdateContext& skinningContext )
{
	RED_ASSERT( renderSkinningData->GetMatrixType() == SDMT_4x4 );
#ifndef NO_EDITOR_FRAGMENTS
	m_isSkinnningUpdating = true;
#endif //NO_EDITOR_FRAGMENTS
#ifdef USE_APEX
	if( m_clothWrapper && provider && renderSkinningData )
	{
		// we have to update l2w here cause we dont want to call base class here
		m_localToWorld = l2w;
		m_clothWrapper->SetVisible( ECVO_Visible );
		m_clothWrapper->SetWrapperTransform( m_localToWorld );

		Matrix* matrices = static_cast< Matrix* >( renderSkinningData->GetWriteData() );

		if( m_shouldUpdateWetness )
		{
			// [ HACK horzel wetcloth ]++
			Float averageWetness = 0.f;

			const Uint32 numBones = renderSkinningData->GetMatrixCount();
			for (Uint32 i=0; i<numBones; ++i )
			{
				averageWetness += 1.f - matrices[ i ].V[ 3 ].A[ 3 ];
				matrices[ i ].V[ 3 ].A[ 3 ] = 1.f;
			}
			averageWetness /= numBones ? numBones : 1.f;
			// [ HACK horzel wetcloth ]--

			m_clothWrapper->SetWetness( averageWetness );
		}

		Uint32 numMatrices = GetMeshTypeResource()->GetBoneCount();

		///////////////////////////////////
		//
		//	So we have 4 cases when teleport request appears
		//
		//	1. req: poseChanged=0 && pelvisChanged=0 => Continuous
		//	2. req: poseChanged=1 && pelvisChanged=0 => TeleportAndReset
		//	3. req: poseChanged=1 && pelvisChanged=1 => TeleportAndReset 
		//	4. req: poseChanged=0 && pelvisChanged=1 => Teleport
		//
		///////////////////////////////////

		//case 1
		EClothTeleportMode teleportMode = EClothTeleportMode::Continuous;
		// This is a simple version of checking teleport - we check only base skeleton
		// We have to do this here cause in window between requesting teleport and update pose could change and that why
		// we are checking it here only when teleport requested
		// can't do it better for now
		if( m_teleportRequested )
		{
			if ( const CAnimatedComponent* ac = CAnimatedComponent::GetRootAnimatedComponentFromAttachment( this ) )
			{
				const CTeleportDetector* detector = ac->GetTeleportDetector();
				const Bool poseChanged = detector ? detector->DoesPoseChanged() : false;
				const Bool pelvisChangedMS = detector ? detector->DoesPelvisChangedMS() : false;
				
				if ( poseChanged )
				{
					// case 2 and 3 doesnt matter if pelvis changed
					teleportMode = EClothTeleportMode::TeleportAndReset;
					m_clothWrapper->SetCurrentMaxDistanceScale( 0.f );
				}
				else
				{
					if ( pelvisChangedMS )
					{
						// case 4
						teleportMode = EClothTeleportMode::TeleportAndReset;
						m_clothWrapper->SetCurrentMaxDistanceScale( 0.f );
					}
					else
					{
						// cloth skipping in idle state case
						teleportMode = EClothTeleportMode::Teleport;
					}
				}
			}
			m_teleportRequested = false;
		}
		const Box boxWS = l2w.TransformBox( boxMS );
		m_clothWrapper->UpdateStateFromBonesMatricesBuffer( m_localToWorld, matrices, numMatrices, teleportMode, boxWS );

		m_cachedLastSkinningMatrices = matrices;
		m_cachedLastSkinningMatricesCount = numMatrices;
	}
	if ( m_triggerWrapper )
	{
		m_triggerWrapper->SetFlag( PRBW_PoseIsDirty, true );
	}
#endif // USE_APEX
}

void CClothComponent::OnUpdateTransformWithoutSkinning( const Box& boxMS, const Matrix& l2w, SMeshSkinningUpdateContext& skinningContext )
{
#ifdef USE_APEX
#ifndef NO_EDITOR_FRAGMENTS
	m_isSkinnningUpdating = false;
#endif //NO_EDITOR_FRAGMENTS
	if ( m_clothWrapper )
	{
		// we have to update l2w here cause we dont want to call base class here
		m_localToWorld = l2w;
		m_clothWrapper->SetVisible( ECVO_NotVisible );
		m_clothWrapper->SetWrapperTransform( m_localToWorld );
		const Box boxWS = l2w.TransformBox( boxMS );
		m_clothWrapper->UpdateStateFromBonesMatricesBuffer( m_localToWorld, m_cachedLastSkinningMatrices, m_cachedLastSkinningMatricesCount, EClothTeleportMode::Continuous, boxWS );
	}
#endif // USE_APEX
}

void CClothComponent::OnUpdateTransformComponent(SUpdateTransformContext& context, const Matrix& prevLocalToWorld)
{
	if( m_clothWrapper && !IsSkinned() )
	{
		// we have to pass m_l2w to params cause later on clothing actor in cloth wrapper will be created with current updated l2w (from placement event f.ex.)
		m_parameters.m_pose = m_localToWorld;
		m_clothWrapper->SetWrapperTransform( m_localToWorld );
		m_clothWrapper->SetFlag(PRBW_PoseIsDirty, true);
	}
	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );
}

void CClothComponent::OnResetClothAndDangleSimulation()
{
	TBaseClass::OnResetClothAndDangleSimulation();
	RequestTeleport();
}

void CClothComponent::CheckWetnessSupport( const CEntity* par )
{
	m_shouldUpdateWetness = par->FindComponent< CWetnessComponent >() != nullptr;
}

RED_DEFINE_STATIC_NAME( CGuiScenePlayer );

void CClothComponent::OnItemEntityAttached( const CEntity* par )
{
	TBaseClass::OnItemEntityAttached( par );
	CheckWetnessSupport( par );

	CWorld* world = GetWorld();
	CObject* worldParent = world ? world->GetParent() : nullptr;
	CClass* worldParentClass = worldParent ? worldParent->GetClass() : nullptr;
	if ( worldParentClass && worldParentClass->GetName() == CNAME(CGuiScenePlayer) && !m_clothWrapper->IsReady() )
	{
		const SClothParameters&	clothParameters = GetParameters();
		m_clothWrapper->Create_Hack( clothParameters );
	}
}

#ifndef NO_EDITOR
void CClothComponent::EditorOnTransformChanged()
{
	TBaseClass::EditorOnTransformChanged();
#ifdef USE_APEX
	if ( m_clothWrapper )
	{
		if( IsSkinned() )
		{
			RequestTeleport();
		}
		else
		{
			m_clothWrapper->SetWrapperTransform( m_localToWorld );
			m_clothWrapper->SetFlag(PRBW_PoseIsDirty, true);
		}
	}
	if( m_triggerWrapper )
	{
		m_triggerWrapper->SetFlag(PRBW_PoseIsDirty, true);
	}
#endif //USE_APEX
}
#endif // NO_EDITOR
