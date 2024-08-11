/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "dismembermentComponent.h"
#include "destructionSystemComponent.h"
#include "renderCommands.h"
#include "dropPhysicsComponent.h"
#include "mesh.h"
#include "particleSystem.h"
#include "../core/scriptStackFrame.h"
#include "../core/dataError.h"
#include "clothComponent.h"
#include "meshSkinningAttachment.h"
#include "animatedComponent.h"
#include "skeleton.h"
#include "hardAttachment.h"
#include "particleComponent.h"
#include "componentIterator.h"
#include "world.h"
#include "layer.h"
#include "dynamicLayer.h"
#include "soundEmitter.h"
#include "entity.h"
#include "../physics/physicsRagdollWrapper.h"
#include "../physics/physicsRagdollState.h"
#include "ragdollPhysX.h"
#include "tickManager.h"
#include "appearanceComponent.h"
#include "meshComponent.h"
#include "utils.h"


IMPLEMENT_ENGINE_CLASS( CDismembermentComponent );


//////////////////////////////////////////////////////////////////////////

void CDismembermentComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
}

CDismembermentComponent::CDismembermentComponent()
	: m_visibleWound( nullptr )
	, m_visibleWoundName( CName::NONE )
	, m_fillMeshComponent( nullptr )
	, m_decal( nullptr )
{
}


CDismembermentComponent::~CDismembermentComponent()
{
	RED_ASSERT( m_decal == nullptr, TXT("Wound decal was not removed") );
	SAFE_RELEASE( m_decal );
}


Bool CDismembermentComponent::IsWoundEnabled( const CName& woundName ) const
{
	return !CEntityDismemberment::IsWoundDisabledRecursive( GetEntity(), woundName );
}


void CDismembermentComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CDismembermentComponent_OnAttached );

	if ( m_visibleWoundName != CName::NONE )
	{
		// Prevent setting a wound if it's not available for the entity's current appearance.
		// If we're in a preview world, we allow it always, so it's not confusing in the entity editor :)
		if ( !world->GetPreviewWorldFlag() )
		{
			if ( !IsWoundEnabled( m_visibleWoundName ) )
			{
				m_visibleWoundName = CName::NONE;
				m_visibleWound = nullptr;
				return;
			}
		}

		// Force update, since we already have the wound name set, so SetVisibleWound() would do nothing.
		ForceUpdateWound();
	}

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Dismemberment );
}

void CDismembermentComponent::OnDetached( CWorld* world )
{
	SetVisibleWound( CName::NONE );

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Dismemberment );

	TBaseClass::OnDetached( world );
}

void CDismembermentComponent::GetWoundNames( TDynArray< CName >& outNames, EWoundTypeFlags woundTypeFlags /* = WTF_All */ ) const
{
	CEntityDismemberment::GetEnabledWoundNamesRecursive( GetEntity(), outNames, woundTypeFlags );
}


void CDismembermentComponent::SetVisibleWound( const CName& name )
{
	if ( name == m_visibleWoundName )
	{
		return;
	}

	// If we're not attached, just remember what wound we want to set, and we'll try later.
	if ( !IsAttached() )
	{
		m_visibleWoundName = name;
		return;
	}

	// If this wound is not available for our entity's appearance, don't do anything.
	// If we're in a preview world, we allow it always, so it's not confusing in the entity editor :)
	if ( !GetWorld()->GetPreviewWorldFlag() && !IsWoundEnabled( name ) )
	{
		return;
	}

	const CDismembermentWound* woundToSet = nullptr;

	if ( name != CName::NONE )
	{
		woundToSet = FindWoundByName( name );

		// If we didn't find the wound, just hide what we have now.
		RED_ASSERT( woundToSet != nullptr );
		if ( woundToSet == nullptr )
		{
			SetVisibleWound( CName::NONE );
			return;
		}
	}

	PreCreate( woundToSet );

	m_visibleWoundName = name;
	m_visibleWound = woundToSet;

	// Set up the fill mesh.
	if ( m_visibleWoundName == CName::NONE )
	{
		// Remove old fill mesh.
		if ( m_fillMeshComponent != nullptr )
		{
			GetEntity()->DestroyComponent( m_fillMeshComponent );
			m_fillMeshComponent = nullptr;
		}
	}
	else
	{
		CMeshComponent* fillComponent = m_fillMeshComponent.Get();

		// Create new fill mesh component if we don't have one yet.
		if ( fillComponent == nullptr )
		{
			SComponentSpawnInfo spawnInfo;
			fillComponent = static_cast< CMeshComponent* >( GetEntity()->CreateComponent( ClassID< CMeshComponent >(), spawnInfo ) );
			if ( fillComponent != nullptr )
			{
				// Skin it if we can
				if ( GetEntity()->GetRootAnimatedComponent() )
				{
					GetEntity()->GetRootAnimatedComponent()->Attach( fillComponent, ClassID< CMeshSkinningAttachment >() );
				}

				// Set default mesh component state -- drawable flags, light channels, etc.
				fillComponent->SetCastingShadows( true );
				fillComponent->SetNoDisolves( true );
			}
		}

		if ( fillComponent != nullptr && m_visibleWound != nullptr )
		{
			fillComponent->SetResource( m_visibleWound->GetFillMesh() );
		}

		// Assign to the handle only after setting it up. SetResource causes handles to be discarded...
		m_fillMeshComponent = fillComponent;
	}

	PostCreate();
}


void CDismembermentComponent::ClearVisibleWound()
{
	SetVisibleWound( CName::NONE );
}



void CDismembermentComponent::ForceUpdateWound()
{
	if ( m_visibleWoundName == CName::NONE )
	{
		return;
	}

	CName woundName = m_visibleWoundName;
	ClearVisibleWound();
	SetVisibleWound( woundName );
}


void CDismembermentComponent::ForceUpdateWoundTransform()
{
	if ( m_visibleWound != nullptr )
	{
		PreCreate( m_visibleWound );
		PostCreate();
	}
}



void CDismembermentComponent::PreCreate( const CDismembermentWound* wound )
{
	// Remove old decals.
	if ( m_decal != nullptr )
	{
		( new CRenderCommand_RemoveDynamicDecalFromScene( GetWorld()->GetRenderSceneEx(), m_decal ) )->Commit();
		SAFE_RELEASE( m_decal );
	}

	TDynArray< CMeshTypeComponent* > newAffectedComponents;
	CollectAffectedComponents( wound, Vector::ONES, Vector::ZEROS, newAffectedComponents );

	ClearUnaffectedComponents( newAffectedComponents );

	m_affectedComponents.ClearFast();
	for ( Uint32 i = 0; i < newAffectedComponents.Size(); ++i )
	{
		m_affectedComponents.PushBack( newAffectedComponents[i] );
	}
}

void CDismembermentComponent::PostCreate()
{
	UpdateAffectedComponents();
	CreateDecalOnAffectedComponents();

	ScheduleUpdateTransformNode();
}


void CDismembermentComponent::CreateDecalOnAffectedComponents()
{
	RED_ASSERT( m_decal == nullptr );

	if ( m_visibleWound == nullptr )
	{
		return;
	}

	const SDismembermentWoundDecal& decal = m_visibleWound->m_decal;

	// If wound doesn't have a diffuse texture specified, we won't create any decal.
	if ( decal.m_materialInfo.m_diffuseTexture == nullptr )
	{
		return;
	}

	// Collect components in the decal's area. We can't just use m_affectedComponents, because the decal is probably scaled differently
	// from the cutting ellipse, so it could affect a different set of components.
	TDynArray< CMeshTypeComponent* > tempComponents;
	Vector decalScale = Vector( decal.m_scale.X, decal.m_depthScale, decal.m_scale.Y, 1.0f );
	Vector decalOffset = Vector( decal.m_offset.X, 0.0f, decal.m_offset.Y, 0.0f );
	CollectAffectedComponents( m_visibleWound, decalScale, decalOffset, tempComponents );

	// Now grab all render proxies for the collected components.
	TDynArray< IRenderProxy* > renderProxies;
	renderProxies.Reserve( tempComponents.Size() );
	for ( Uint32 i = 0; i < tempComponents.Size(); ++i )
	{
		CMeshTypeComponent* component = tempComponents[i];
		if ( component != nullptr && component->IsVisible() )
		{
			IRenderProxy* proxy = component->GetRenderProxy();
			if ( proxy != nullptr )
			{
				renderProxies.PushBack( proxy );
			}
		}
	}

	// Also add one for the fill mesh.
	if ( decal.m_applyToFillMesh && m_fillMeshComponent != nullptr && m_fillMeshComponent->IsVisible() )
	{
		IRenderProxy* proxy = m_fillMeshComponent->GetRenderProxy();
		if ( proxy != nullptr )
		{
			renderProxies.PushBack( proxy );
		}
	}

	// If we didn't actually get any render proxies, there's nothing to put a decal on.
	if ( renderProxies.Size() == 0 )
	{
		return;
	}


	const EngineTransform& transform = m_visibleWound->GetTransform();

	Matrix orientation = transform.GetRotation().ToMatrix();
	Vector decalRight = orientation.GetAxisX();
	Vector decalFwd = orientation.GetAxisY();
	Vector decalUp = orientation.GetAxisZ();



	SDynamicDecalInitInfo initInfo;
	initInfo.m_dirFront				= decalFwd;
	initInfo.m_dirUp				= decalUp;
	initInfo.m_doubleSided			= decal.m_doubleSided;
	initInfo.m_farZ					= transform.GetScale().Y * decal.m_depthScale;
	initInfo.m_nearZ				= -transform.GetScale().Y * decal.m_depthScale;
	initInfo.m_width				= transform.GetScale().X * 2 * decal.m_scale.X;
	initInfo.m_height				= transform.GetScale().Z * 2 * decal.m_scale.Y;
	initInfo.m_normalFadeBias		= decal.m_normalFadeBias;
	initInfo.m_normalFadeScale		= decal.m_normalFadeScale;
	initInfo.m_depthFadePower		= decal.m_depthFadePower;
	initInfo.m_origin				= transform.GetPosition() + decalRight * decal.m_offset.X + decalUp * decal.m_offset.Y;
	initInfo.m_projectionMode		= decal.m_projectionMode;
	initInfo.m_applyInLocalSpace	= true;
	GetEntity()->GetWorldToLocal( initInfo.m_worldToDecalParent );

	initInfo.SetMaterialInfo( decal.m_materialInfo );

	m_decal = GRender->CreateDynamicDecal( initInfo );
	if ( m_decal != nullptr )
	{
		( new CRenderCommand_AddDynamicDecalToSceneForProxies( GetWorld()->GetRenderSceneEx(), m_decal, renderProxies ) )->Commit();
	}
}


void CDismembermentComponent::ClearUnaffectedComponents( const TDynArray< CMeshTypeComponent* >& newAffectedComponents )
{
	for ( Uint32 i = 0; i < m_affectedComponents.Size(); ++i )
	{
		CMeshTypeComponent* mtc = m_affectedComponents[ i ].Get();
		if ( mtc == nullptr )
		{
			continue;
		}

		// If this component is in the new affected components list, we won't clear it. It'll get updated.
		if ( newAffectedComponents.Exist( mtc ) )
		{
			continue;
		}

		// Clear the clipping ellipse.
		IRenderProxy* rp = mtc->GetRenderProxy();
		if ( rp )
		{
			( new CRenderCommand_ClearClippingEllipseMatrix( rp ) )->Commit();
		}

		// Turn cloth simulation back on.
		if ( CClothComponent* cloth = Cast< CClothComponent >( mtc ) )
		{
			cloth->SetSimulated( true );
		}
	}
}

void CDismembermentComponent::UpdateAffectedComponents()
{
	if ( m_visibleWound == nullptr )
	{
		return;
	}

	Matrix woundToEntity;
	m_visibleWound->GetTransform().CalcLocalToWorld( woundToEntity );
	Matrix entityToWound = woundToEntity.FullInverted();

	for ( Uint32 i = 0; i < m_affectedComponents.Size(); ++i )
	{
		CMeshTypeComponent* mtc = m_affectedComponents[ i ].Get();
		if ( mtc == nullptr )
		{
			continue;
		}

		IRenderProxy* rp = mtc->GetRenderProxy();
		if ( rp )
		{
			if ( mtc->IsSkinned() )
			{
				( new CRenderCommand_SetClippingEllipseMatrix( rp, entityToWound ) )->Commit();
			}
			else
			{
				// Non-skinned meshes will need the ellipse transformed into their own local space.
				Matrix worldToEntity = GetEntity()->GetLocalToWorld().FullInverted();
				Matrix meshToEntity = mtc->GetLocalToWorld() * worldToEntity;
				( new CRenderCommand_SetClippingEllipseMatrix( rp, meshToEntity * entityToWound ) )->Commit();
			}
		}

		// Turn cloth simulation off. Only skinned cloth is supported here!
		if ( CClothComponent* cloth = Cast< CClothComponent >( mtc ) )
		{
			cloth->SetSimulated( false );
		}
	}
}



Bool CDismembermentComponent::IsComponentAffected( CMeshTypeComponent* mtc, const CDismembermentWound* wound, const Matrix& entityToWound ) const
{
	// We want to leave the fill mesh alone.
	if ( mtc == nullptr || mtc == m_fillMeshComponent )
	{
		return false;
	}

	// If this component is excluded, skip it.
	if ( mtc->GetTags().HasTag( wound->GetExcludeTag() ) )
	{
		return false;
	}

	// If this component doesn't have any mesh loaded yet, skip it.
	if ( mtc->GetMeshTypeResource() == nullptr )
	{
		return false;
	}

	// The wound only affects meshes that overlap with the wound's bounds.

	// In the wound's own space, so just (-1,-1,-1)-(1,1,1)
	Box woundBox( -Vector::ONES, Vector::ONES );
	if ( mtc->IsSkinned() )
	{
		Box meshBoxInWound = entityToWound.TransformBox( mtc->GetMeshTypeResource()->GetBoundingBox() );
		if ( !meshBoxInWound.Touches( woundBox ) )
		{
			return false;
		}
	}
	else
	{
		// Need to transform the mesh's box into entity space.
		Matrix worldToEntity = GetEntity()->GetLocalToWorld().FullInverted();
		Matrix meshToEntity = mtc->GetLocalToWorld() * worldToEntity;
		Box meshBox = meshToEntity.TransformBox( mtc->GetMeshTypeResource()->GetBoundingBox() );
		Box meshBoxInWound = entityToWound.TransformBox( meshBox );
		if ( !meshBoxInWound.Touches( woundBox ) )
		{
			return false;
		}
	}

	return true;
}


void CDismembermentComponent::CollectAffectedComponents( const CDismembermentWound* wound, const Vector& scale, const Vector& offset, TDynArray< CMeshTypeComponent* >& components ) const
{
	components.ClearFast();

	if ( wound == nullptr )
	{
		// Having no wound means we don't affect any components
		return;
	}


	// offset is given in the wound's space, we need to transfer it to the entity space.
	Matrix orientation = wound->GetTransform().GetRotation().ToMatrix();
	Vector decalRight = orientation.GetAxisX();
	Vector decalFwd = orientation.GetAxisY();
	Vector decalUp = orientation.GetAxisZ();
	Vector entityOffset = decalRight * offset.X + decalFwd * offset.Y + decalUp * offset.Z;


	Matrix woundToEntity;
	EngineTransform scaledTransform = wound->GetTransform();
	scaledTransform.SetScale( scaledTransform.GetScale() * scale );
	scaledTransform.SetPosition( scaledTransform.GetPosition() + entityOffset );
	scaledTransform.CalcLocalToWorld( woundToEntity );
	Matrix entityToWound = woundToEntity.FullInverted();

	// First collect all meshes that are actually part of the entity. This will be anything that's specifically in
	// the entity, and any appearances.
	for ( ComponentIterator< CMeshTypeComponent > iter( GetEntity() ); iter; ++iter )
	{
		CMeshTypeComponent* mtc = *iter;

		if ( IsComponentAffected( mtc, wound, entityToWound ) )
		{
			components.PushBack( mtc );
		}
	}

	// In addition, we want to collect anything that's being skinned by one of the entity's animated components.
	// This covers equipped gear and such.
	for ( ComponentIterator< CAnimatedComponent > iter( GetEntity() ); iter; ++iter )
	{
		CAnimatedComponent* ac = *iter;

		for ( auto iter2 = ac->GetChildAttachments().Begin(); iter2 != ac->GetChildAttachments().End(); ++iter2 )
		{
			CMeshSkinningAttachment* skinning = (*iter2)->ToSkinningAttachment();
			if ( skinning != nullptr )
			{
				CMeshTypeComponent* mtc = Cast< CMeshTypeComponent >( skinning->GetChild() );

				if ( IsComponentAffected( mtc, wound, entityToWound ) )
				{
					// These may have already been pushed earlier!
					components.PushBackUnique( mtc );
				}
			}
		}
	}
}


const CDismembermentWound* CDismembermentComponent::FindWoundByName( const CName& name ) const
{
	return CEntityDismemberment::FindWoundByNameRecursive( GetEntity()->GetEntityTemplate(), name );
}

#ifndef NO_EDITOR
CDismembermentWound* CDismembermentComponent::FindWoundByName( const CName& name )
{
	return CEntityDismemberment::FindNonConstWoundByNameRecursive( GetEntity()->GetEntityTemplate(), name );
}
#endif


const CDismembermentWound* CDismembermentComponent::GetNearestWound( const Vector& positionMS, const Vector& directionMS, EWoundTypeFlags woundTypeFlags ) const
{
	const Float weightDistance = GGame->GetGameplayConfig().m_woundDistanceWeight;
	const Float weightDirection = GGame->GetGameplayConfig().m_woundDirectionWeight;
	Float bestScore = NumericLimits< Float >::Max();

	Vector normalizedDirectionMS = directionMS.Normalized3();

	const CDismembermentWound* nearestWound = nullptr;

	TDynArray< const CDismembermentWound* > wounds;
	CEntityDismemberment::GetEnabledWoundsRecursive( GetEntity(), wounds );
	for ( const CDismembermentWound* wound : wounds )
	{
		// if wound type does not match searching criteria
		if ( !( wound->GetTypeFlags() & woundTypeFlags ) )
		{
			continue;
		}

		Matrix rotWound2Model = wound->GetTransform().GetRotation().ToMatrix();
		Matrix rotModelToWound = rotWound2Model.Transposed();
		Vector translation = wound->GetTransform().GetPosition();
		Vector3 scale = wound->GetTransform().GetScale();
		#define SAFE_INVERT( a ) a == 0.0f ? 0.0f : 1.0f / a
		Vector3 invScale = Vector( SAFE_INVERT( scale.X ), SAFE_INVERT( scale.Y ), SAFE_INVERT( scale.Z ) );
		#undef SAFE_INVERT

		// now lets transform both position and direction to wound's local space
		Vector positionLS = Vector::Mul3( rotModelToWound.TransformPoint( positionMS - translation ), invScale );
		Vector directionLS = rotModelToWound.TransformVector( normalizedDirectionMS );

		// next we compute point on the sphere that is closest to the positionLS
		// and transform it back to model space
		Vector nearestLS = positionLS.Normalized3();
		Vector nearestMS = rotWound2Model.TransformPoint( Vector::Mul3( nearestLS, scale ) ) + translation;

		// finally we compute its distance to the hit position
		Float distance = ( nearestMS - positionMS ).Mag3();

		// to calculate "the most tangent" factor we choose the "most scaled" direction
		// and take local space direction vector value in "most scaled" direction
		Float direction = 1.0f;
		//Vector scale = wound->GetTransform().GetScale();
		if ( Abs( scale.X ) > Abs( scale.Y ) )
		{
			direction = ( Abs( scale.X ) > Abs( scale.Z ) ) ? directionLS.X : directionLS.Z;
		}
		else
		{
			direction = ( Abs( scale.Y ) > Abs( scale.Z ) ) ? directionLS.Y : directionLS.Z;
		}
		// normalize direction factor to [0, 1] - 0 being "the best" direction
		direction = 1.0f - Abs( direction );

		// finally we compute "score" based on the predefined weights
		Float score = distance * weightDistance + direction * weightDirection;
		if ( score < bestScore )
		{
			bestScore = score;
			nearestWound = wound;
		}
	}

	return nearestWound;
}

const CDismembermentWound* CDismembermentComponent::GetNearestWound( Int32 boneIndex, const Vector& directionWS, EWoundTypeFlags woundTypeFlags ) const
{
	if ( boneIndex == -1 )
	{
		return nullptr;
	}

	CAnimatedComponent* ac = GetEntity()->GetRootAnimatedComponent();
	if ( ac != nullptr && ac->GetSkeleton() != nullptr )
	{
		const AnimQsTransform boneTransformMS = ac->GetSkeleton()->GetBoneMS( boneIndex );

		// since wounds are defined in reference pose we need 
		// to obtain bone reference position in model space
		RedVector4 bonePosMS = boneTransformMS.GetTranslation();

		// we also need to transform direction vector from its current pose world space
		// to bone local space and then back to reference pose model space
		Matrix m = ac->GetBoneMatrixWorldSpace( boneIndex ).Transposed();
		Vector directionMS = m.TransformVector( directionWS );
		RedMatrix4x4 rm = boneTransformMS.ConvertToMatrix();
		m = reinterpret_cast< Matrix& >( rm );
		directionMS = m.TransformVector( directionMS );
		directionMS.Normalize3();

		return GetNearestWound( Vector( bonePosMS.X, bonePosMS.Y, bonePosMS.Z ), directionMS, woundTypeFlags );
	}
	return nullptr;
}

Bool CDismembermentComponent::SpawnWoundEntity( const CDismembermentWound* wound, Bool dropEquipment, const Vector& direction, Uint32 playedEffectsMask )
{
	if ( wound == nullptr )
	{
		return false;
	}

	const CDismembermentWound::SingleSpawnArray& ssArray = wound->GetSpawnArray();
	CEntity* baseEntity = GetEntity();
	RED_ASSERT( baseEntity != nullptr, TXT( "CDismembermentComponent is not attached to CEntity" ) );
	CDropPhysicsComponent* dropPhysics = baseEntity->FindComponent< CDropPhysicsComponent >();

	Bool toRet = false;

	for ( Uint32 i = 0; i < ssArray.Size(); ++i )
	{
		const SDismembermentWoundSingleSpawn& ss = ssArray[i];

		if ( ss.GetSpawnedEntity() == nullptr )
		{
			DATA_HALT( DES_Minor, CResourceObtainer::GetResource( this ), TXT( "Dismemberment" ), TXT( "m_spawnedEntity probably was ment to exist for this dismemberment wound." ) );
			continue;
		}

		EntitySpawnInfo spawnInfo;
		spawnInfo.m_template = ss.GetSpawnedEntity();
		spawnInfo.m_spawnPosition = baseEntity->GetWorldPosition();
		if ( ss.GetSpawnEntityBoneName() != CName::NONE )
		{
			CAnimatedComponent* ac = baseEntity->GetRootAnimatedComponent();
			if ( ac != nullptr )
			{
				Int32 boneIndex = ac->FindBoneByName( ss.GetSpawnEntityBoneName() );
				if ( boneIndex != -1 )
				{
					Matrix m = ac->GetBoneMatrixWorldSpace( boneIndex );
					spawnInfo.m_spawnPosition = m.GetTranslation();
					spawnInfo.m_spawnRotation = m.ToEulerAngles();
				}
			}
		}

		CEntity* spawnedEntity = GetWorld()->GetDynamicLayer()->CreateEntitySync( spawnInfo );
		if ( spawnedEntity == nullptr )
		{
			continue;
		}

		if ( playedEffectsMask != 0 )
		{
			TDynArray< CName > effectsNames;
			ss.CollectEffects( playedEffectsMask, effectsNames );
			for ( Uint32 j = 0; j < effectsNames.Size(); j++ )
			{
				spawnedEntity->PlayEffect( effectsNames[ j ] );
			}
		}

		toRet = true;

		// Sync spawned entity's position (and pose) to the base (character) entity.
		// Do not sync explosion wounds: we don't need it, and syncing forces kinematic ragdolls,
		// that would be switched again to dynamic after curve animation finishes
		// - that's not gonna happen for explosion wounds, as they're not "dropped" on curves.
		Bool wasSynced = false;
		if ( ss.m_syncPose && !wound->IsExplosionWound() && SyncSpawnedEntity( ss, spawnedEntity ) )
		{
			wasSynced = true;
			spawnedEntity->ForceUpdateTransformNodeAndCommitChanges();
			spawnedEntity->ForceUpdateBoundsNode();
		}

		// We need CDropPhysicsComponent to "drop" entity using predefined curves or to drop equipment.
		if ( dropPhysics == nullptr )
		{
			continue;
		}

		//hack
		if( CAnimatedComponent* animatedComponent = spawnedEntity->GetRootAnimatedComponent() )
		{
			if( CRagdoll* ragdoll =	const_cast< CRagdoll* >( animatedComponent->GetRagdoll() ) )
			{
				SPhysicsRagdollState& state = const_cast< SPhysicsRagdollState& >( ragdoll->GetState() );
				state.m_windScaler = 0.0f;
				state.m_forceWakeUpOnAttach = true;
			}

			animatedComponent->OverrideRagdollCollisionGroup( CPhysicalCollision( CNAME( Dismemberment ) ) );
		}
		//hack

		// Disable ragdoll parts that are not used.
		CDropPhysicsComponent::SMappedBones mappedBones;
		CDropPhysicsComponent::CollectMappedBones( spawnedEntity->GetRootAnimatedComponent(), mappedBones, ss.m_fixSpawnedBonesHierarchyType );
		CDropPhysicsComponent::SDisableRagdollInfo disableRagdollInfo;
		disableRagdollInfo.m_type = CDropPhysicsComponent::DRT_Both;
		disableRagdollInfo.m_spawnedEntity = spawnedEntity;
		disableRagdollInfo.m_baseEntity = baseEntity;
		disableRagdollInfo.m_spawnedEntityBones = &mappedBones;
		disableRagdollInfo.m_fixBaseBonesHierarchyType = ss.m_fixBaseBonesHierarchyType;
		disableRagdollInfo.m_fixSpawnedBonesHierarchyType = ss.m_fixSpawnedBonesHierarchyType;
		if ( m_fillMeshComponent != nullptr )
		{
			// we collect all bones present in the fill mesh to prevent
			// them from being disabled in the base ragdoll
			CMesh* mesh = m_fillMeshComponent->GetMeshNow();
			if ( mesh != nullptr )
			{
				Uint32 size = mesh->GetBoneCount();
				const CName* names = mesh->GetBoneNames();
				for ( Uint32 boneIndex = 0; boneIndex < size; boneIndex++ )
				{
					disableRagdollInfo.m_baseEntityBonesToSkip.Insert( names[ boneIndex ].AsString() );
				}
			}
		}
		dropPhysics->DisableRagdollParts( disableRagdollInfo );

		if ( !wound->IsExplosionWound() )
		{
			CDropPhysicsComponent::SDropPhysicsInfo dropInfo;
			dropInfo.m_curveName = ss.GetSpawnedEntityCurveName();
			dropInfo.m_direction = direction;
			dropInfo.m_initialPosition = spawnedEntity->GetPosition();
			dropInfo.m_initialRotation = spawnedEntity->GetRotation();
			dropInfo.m_initialPositionType = wasSynced ? CDropPhysicsComponent::SDropPhysicsInfo::DPIPT_Entity : CDropPhysicsComponent::SDropPhysicsInfo::DPIPT_CenterOfMass;

			if ( wasSynced )
			{
				// compute center of mass offset based on the base entity (because its physics and animations are already synchronized)
				if ( CAnimatedComponent* ac = baseEntity->GetRootAnimatedComponent() )
				{
					CDropPhysicsComponent::GetCenterOfMassOffsets( baseEntity, ac->GetRagdollPhysicsWrapper(),
						dropInfo.m_centerOfMassOffsetLS, dropInfo.m_initialCenterOfMassOffsetWS,
						0, &mappedBones.m_bones );
				}
			}

			if ( ss.m_despawnAlongWithBase )
			{
				dropInfo.m_flags |= CDropPhysicsComponent::SDropPhysicsInfo::DPIF_DespawnAlongWithBase;
			}
			if ( ss.m_syncPose && wasSynced )
			{
				// curves normalization is enabled for all "synchronized" entities
				dropInfo.m_flags |= CDropPhysicsComponent::SDropPhysicsInfo::DPIF_NormalizeCurves;
			}
			dropPhysics->DropExternalEntity( spawnedEntity, dropInfo );
		}
		else if ( ss.m_despawnAlongWithBase )
		{
			// wound comes from explosion but its entity needs to be despawned along with base entity
			// so we pass the ownership to CDropPhysicsComponent without need to update
			CDropPhysicsComponent::SDropPhysicsInfo dropInfo;
			dropInfo.m_flags |= CDropPhysicsComponent::SDropPhysicsInfo::DPIF_DespawnAlongWithBase;
			dropInfo.m_flags |= CDropPhysicsComponent::SDropPhysicsInfo::DPIF_DoNotUpdate;
			dropPhysics->DropExternalEntity( spawnedEntity, dropInfo );
		}

		if ( dropEquipment && ss.m_droppedEquipmentTag != CName::NONE )
		{
			CDropPhysicsComponent::SDropPhysicsInfo dropInfo;
			dropInfo.m_curveName = ss.m_droppedEquipmentTag;
			dropInfo.m_direction = direction;
			dropPhysics->DropMeshByTag( ss.m_droppedEquipmentTag, dropInfo );
		}
	}

	return toRet;	// info if something was spawned
}

Bool CDismembermentComponent::SyncSpawnedEntity( const SDismembermentWoundSingleSpawn& wound, CEntity* spawnedEntity ) const
{
	CAnimatedComponent* characterRootAc = GetEntity()->GetRootAnimatedComponent();
	CAnimatedComponent* spEntityRootAc = spawnedEntity->GetRootAnimatedComponent();

	// If there are two animated components we will try to sync its poses (spawned to base).
	if ( characterRootAc != nullptr && spEntityRootAc != nullptr )
	{
		// only poses sharing the same skeleton can be synchronized
		if ( characterRootAc->GetSkeleton() != spEntityRootAc->GetSkeleton() )
		{
			return false;
		}
		if ( spEntityRootAc->SyncToPoseWS( characterRootAc ) )
		{
			return true;
		}
	}

	return false; 
}

Matrix CDismembermentComponent::FixBoneRotationWS( const Matrix& boneMatrixWS ) const
{
	const Vector initX = boneMatrixWS.GetAxisX();
	const Vector initY = boneMatrixWS.GetAxisY();
	const Vector initZ = boneMatrixWS.GetAxisZ();
	//Vector x = ( MAbs( initX.X ) > MAbs( initY.X ) ) ? ( MAbs( initX.X ) > MAbs( initZ.X ) ? initX : initZ ) : ( MAbs( initY.X ) > MAbs( initZ.X ) ? initY : initZ );
	//Vector y = ( MAbs( initX.Y ) > MAbs( initY.Y ) ) ? ( MAbs( initX.Y ) > MAbs( initZ.Y ) ? initX : initZ ) : ( MAbs( initY.Y ) > MAbs( initZ.Y ) ? initY : initZ );
	Vector z = ( MAbs( initX.Z ) > MAbs( initY.Z ) ) ? ( MAbs( initX.Z ) > MAbs( initZ.Z ) ? initX : initZ ) : ( MAbs( initY.Z ) > MAbs( initZ.Z ) ? initY : initZ );
	Matrix entityWS = GetEntity()->GetLocalToWorld();
	Vector x = Vector::Cross( entityWS.GetAxisY(), z );
	Vector y = Vector::Cross( z, x );
	Matrix ret;
	ret.SetRows( x, y, z, Vector::EW );
	return ret;
}

/// @todo MS: this function looks like it causes visual artifacts around explosion !!!!
Bool CDismembermentComponent::CreateWoundParticles( const CName& woundName ) const
{
	const CDismembermentWound* wound = FindWoundByName( woundName );
	if ( wound == nullptr )
	{
		return false;
	}

	const EntitySlot* entitySlot = GetEntity()->GetEntityTemplate()->FindSlotByName( woundName, true );
	if ( entitySlot != nullptr )
	{
		return CreateWoundParticles( wound, entitySlot );
	}
	// slots should be named the same as wounds but...
	// sometimes they differ with upper/lowercases
	CName woundNameLC = CName( woundName.AsString().ToLower() );
	const EntitySlot* entitySlotLC = GetEntity()->GetEntityTemplate()->FindSlotByName( woundNameLC, true );
	if ( entitySlotLC != nullptr )
	{
		return CreateWoundParticles( wound, entitySlotLC );
	}

	return CreateWoundParticles( wound );

}

Bool CDismembermentComponent::CreateWoundParticles( const CDismembermentWound* wound ) const
{
	CEntity* entity = GetEntity();
	RED_ASSERT( entity != nullptr );

	Bool res = false;

	// Wound Y axis is also "projection" vector so it's directed towards entity.
	// To obtain model space orientation for particles we need to switch Y and Z axis and negate Z.
	// And this is why we need to start using quaternions:
	EngineTransform woundTransform = wound->GetTransform();
	Matrix particlesRotMS = Matrix( Vector::EX, Vector::EZ, -Vector::EY, Vector::EW ) * woundTransform.GetRotation().ToMatrix();
	Vector particlesPositionMS = woundTransform.GetPosition() - particlesRotMS.GetAxisZ() * woundTransform.GetScale().Y;
		
	Int32 nearestBone = FindNearestBone( particlesPositionMS );
	CAnimatedComponent* ac = entity->GetRootAnimatedComponent();
	RED_ASSERT( nearestBone == -1 || ( ac != nullptr && ac->GetSkeleton() != nullptr ) );

	// first we spawn "global" particles as a separate entity
	if ( !wound->GetParticles().IsEmpty() )
	{
		Vector pos = particlesPositionMS;
		Matrix rot = particlesRotMS;
		// transform position according to the nearest bone current matrix
		if ( nearestBone != -1 )
		{
			RedMatrix4x4 rm = ac->GetSkeleton()->GetBoneMS( nearestBone ).ConvertToMatrix();
			Matrix m = reinterpret_cast< Matrix& >( rm ).Inverted();
			pos = m.TransformPoint( pos );
			rot = rot * m;
			m = ac->GetBoneMatrixModelSpace( nearestBone );
			pos = m.TransformPoint( pos );
			rot = rot * m;
		}
		Matrix l2w;
		entity->GetTransform().CalcLocalToWorld( l2w );		
		EntitySpawnInfo spawnInfo;
		spawnInfo.m_spawnPosition = l2w.TransformPoint( pos );
		spawnInfo.m_spawnRotation = ( rot * l2w ).ToEulerAngles();
		CEntity* newEntity = GetWorld()->GetDynamicLayer()->CreateEntitySync( spawnInfo );
		CParticleComponent* pc = Cast< CParticleComponent >( newEntity->CreateComponent( ClassID< CParticleComponent >(), SComponentSpawnInfo() ) );
		newEntity->Attach( pc, HardAttachmentSpawnInfo() );
		newEntity->ForceUpdateTransformNodeAndCommitChanges();
		pc->SetParticleSystemAsync( wound->GetParticles() );
		pc->RefreshRenderProxies();
		res = true;
	}
	
	// next we spawn particles "attached" to base entity
	if ( !wound->GetAttachedParticles().IsEmpty() )
	{
		// if there is an animated component we attach particle component to the nearest bone
		CName boneName = CName::NONE;
		if ( nearestBone != -1 )
		{
			CSkeleton* skeleton = ac->GetSkeleton();
			boneName = CName( skeleton->GetBoneName( nearestBone ) );
			RedMatrix4x4 rm = skeleton->GetBoneMS( nearestBone ).ConvertToMatrix();
			Matrix m = reinterpret_cast< Matrix& >( rm ).Inverted();
			particlesPositionMS = m.TransformPoint( particlesPositionMS );
			particlesRotMS = particlesRotMS * m;
		}
		EulerAngles rotationMS = particlesRotMS.ToEulerAngles();

		CParticleComponent* pc = Cast< CParticleComponent >( entity->CreateComponent( ClassID< CParticleComponent >(), SComponentSpawnInfo() ) );
		if ( nearestBone != -1 )
		{
			HardAttachmentSpawnInfo attachInfo;
			attachInfo.m_parentSlotName = boneName;
			attachInfo.m_relativePosition = particlesPositionMS;
			attachInfo.m_relativeRotation = rotationMS;
			ac->Attach( pc, attachInfo );
		}
		else
		{
			HardAttachmentSpawnInfo attachInfo;
			attachInfo.m_relativePosition = particlesPositionMS;
			attachInfo.m_relativeRotation = rotationMS;
			entity->Attach( pc, attachInfo );
		}
		pc->ForceUpdateTransformNodeAndCommitChanges();
		pc->SetParticleSystemAsync( wound->GetAttachedParticles() );
		pc->RefreshRenderProxies();
		res = true;
	}
	
	return res;
 }

Bool CDismembermentComponent::CreateWoundParticles( const CDismembermentWound* wound, const EntitySlot* slot ) const
{
	Bool res = false;

	// first we spawn "global" particles as a separate entity
	if ( !wound->GetParticles().IsEmpty() )
	{
		Matrix l2w;
		String error;
		if ( slot->CalcMatrix( GetEntity(), l2w, &error ) )
		{
			EntitySpawnInfo spawnInfo;
			spawnInfo.m_spawnPosition = l2w.GetTranslation();
			spawnInfo.m_spawnRotation = l2w.ToEulerAngles();
			CEntity* newEntity = GetWorld()->GetDynamicLayer()->CreateEntitySync( spawnInfo );
			CParticleComponent* pc = Cast< CParticleComponent >( newEntity->CreateComponent( ClassID< CParticleComponent >(), SComponentSpawnInfo() ) );
			newEntity->Attach( pc, HardAttachmentSpawnInfo() );
			pc->ForceUpdateTransformNodeAndCommitChanges();
			pc->SetParticleSystemAsync( wound->GetParticles() );
			pc->RefreshRenderProxies();
			res = true;
		}
	}

	// next we spawn particles "attached" to base entity	
	if ( !wound->GetAttachedParticles().IsEmpty() )
	{
		CEntity* entity = GetEntity();
		RED_ASSERT( entity != nullptr );

		CParticleComponent* pc = Cast< CParticleComponent >( entity->CreateComponent( ClassID< CParticleComponent >(), SComponentSpawnInfo() ) );
		CNode* parent = slot->GetComponentName() != CName::NONE ? entity->FindComponent( slot->GetComponentName() ) : nullptr;
		if ( parent == nullptr )
		{
			parent = entity;
		}

		HardAttachmentSpawnInfo attachInfo;
		attachInfo.m_parentSlotName = slot->GetBoneName();
		attachInfo.m_relativePosition = slot->GetTransform().GetPosition();
		attachInfo.m_relativeRotation = slot->GetTransform().GetRotation();
		parent->Attach( pc, attachInfo );

		pc->ForceUpdateTransformNodeAndCommitChanges();
		pc->SetParticleSystemAsync( wound->GetAttachedParticles() );
		pc->RefreshRenderProxies();
		res = true;
	}

	return res;
}

Int32 CDismembermentComponent::FindNearestBone( const Vector& referencePositionMS ) const
{
	Int32 nearestBone = -1;
	CAnimatedComponent* ac = GetEntity()->GetRootAnimatedComponent();
	if ( ac != nullptr && ac->GetSkeleton() != nullptr )
	{
		Float minDist = NumericLimits< Float >::Max();
		CSkeleton* skeleton = ac->GetSkeleton();
		Int32 bonesNum = skeleton->GetBonesNum();		
		for ( Int32 i = 0; i < bonesNum; i++ )
		{
			AnimQsTransform boneMS = skeleton->GetBoneMS( i );
			RedVector4 p = boneMS.GetTranslation();
			Float dist = ( referencePositionMS - Vector( p.X, p.Y, p.Z ) ).SquareMag3();
			if ( dist < minDist )
			{
				nearestBone = i;
				minDist = dist;
			}
		}
	}
	return nearestBone;
}

Bool CDismembermentComponent::PlaySoundEvents( const CDismembermentWound* wound ) const
{
	CSoundEmitterComponent* sound = GetEntity()->GetSoundEmitterComponent();
	if ( !wound || !sound )
	{
		return false;
	}

	const CDismembermentWound::SingleSpawnArray& ssArray = wound->GetSpawnArray();

	for( Uint32 i = 0; i < ssArray.Size(); ++i )
	{
		const SDismembermentWoundSingleSpawn& ss = ssArray[i];

		const TDynArray< StringAnsi > & soundEvents = ss.GetSoundEvents();
		CAnimatedComponent* ac = GetEntity()->GetRootAnimatedComponent();
		if ( ac != nullptr )
		{
			Int32 boneIndex = -1;
			if ( ss.GetSpawnEntityBoneName() != CName::NONE )
			{
				boneIndex = ac->FindBoneByName( ss.GetSpawnEntityBoneName() );
			}
			if ( boneIndex == -1 )
			{
				boneIndex = FindNearestBone( wound->GetTransform().GetPosition() );
			}
			if ( boneIndex != -1 )
			{
				for ( Uint32 j = 0; j < soundEvents.Size(); j++ )
				{
					sound->SoundEvent( soundEvents[j].AsChar(), boneIndex );
				}
				
				continue;
			}
		}

		for ( Uint32 j = 0; j < soundEvents.Size(); j++ )
		{
			sound->SoundEvent( soundEvents[j].AsChar() );
		}
	}

	return true;
}

void CDismembermentComponent::funcIsWoundDefined( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;
	RETURN_BOOL( FindWoundByName( name ) != nullptr );
}

void CDismembermentComponent::funcSetVisibleWound( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER_OPT( Bool, spawnEntity, false );
	GET_PARAMETER_OPT( Bool, createParticles, false );
	GET_PARAMETER_OPT( Bool, dropEquipment, false );
	GET_PARAMETER_OPT( Bool, playSound, false );
	GET_PARAMETER_OPT( Vector, direction, Vector::ZEROS );
	GET_PARAMETER_OPT( Uint32, playedEffectsMask, 0 );
	FINISH_PARAMETERS;
	SetVisibleWound( name );
	if ( spawnEntity || createParticles )
	{
		const CDismembermentWound* wound = FindWoundByName( name );
		if ( wound != nullptr )
		{
			if ( spawnEntity )
			{
				SpawnWoundEntity( wound, dropEquipment, direction, playedEffectsMask );
			}
			if ( createParticles )
			{
				CreateWoundParticles( name );
			}
			if ( playSound )
			{
				PlaySoundEvents( wound );
			}
		}
	}
	RETURN_VOID();
}

void CDismembermentComponent::funcClearVisibleWound( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	ClearVisibleWound();
	RETURN_VOID();
}

void CDismembermentComponent::funcGetVisibleWoundName( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_NAME( GetVisibleWoundName() );
}


void CDismembermentComponent::funcCreateWoundParticles( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;
	RETURN_BOOL( CreateWoundParticles( name ) );
}

void CDismembermentComponent::funcGetNearestWoundName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, positionMS, Vector::ZEROS );
	GET_PARAMETER( Vector, directionMS, Vector::ZEROS );
	GET_PARAMETER_OPT( EWoundTypeFlags, woundTypeFlags, WTF_All );
	FINISH_PARAMETERS;
	const CDismembermentWound* wound = GetNearestWound( positionMS, directionMS, woundTypeFlags );
	CName woundName = ( wound != nullptr ) ? wound->GetName() : CName::NONE;
	RETURN_NAME( woundName );
}

void CDismembermentComponent::funcGetNearestWoundNameForBone( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, boneIndex, -1 );
	GET_PARAMETER( Vector, directionWS, Vector::ZEROS );
	GET_PARAMETER_OPT( EWoundTypeFlags, woundTypeFlags, WTF_All );
	FINISH_PARAMETERS;
	const CDismembermentWound* wound = GetNearestWound( boneIndex, directionWS, woundTypeFlags );
	CName woundName = ( wound != nullptr ) ? wound->GetName() : CName::NONE;
	RETURN_NAME( woundName );
}

void CDismembermentComponent::funcGetWoundsNames( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< CName >, outNames, TDynArray< CName >() );
	GET_PARAMETER_OPT( EWoundTypeFlags, woundTypeFlags, WTF_All );
	FINISH_PARAMETERS;
	GetWoundNames( outNames, woundTypeFlags );
	RETURN_VOID();
}



void CDismembermentComponent::funcIsExplosionWound( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	const CDismembermentWound* wound = FindWoundByName( name );
	RETURN_BOOL( wound && wound->m_isExplosionWound );
}

void CDismembermentComponent::funcIsFrostWound( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	const CDismembermentWound* wound = FindWoundByName( name );
	RETURN_BOOL( wound && wound->m_isFrostWound );
}


void CDismembermentComponent::funcGetMainCurveName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	const CDismembermentWound* wound = FindWoundByName( name );
	if( wound )
	{
		RETURN_NAME( wound->GetMainEntityCurveName() );
	}
	else
	{
		RETURN_NAME( CName::NONE );
	}
}
