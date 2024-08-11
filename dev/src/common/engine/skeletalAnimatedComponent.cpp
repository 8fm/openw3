/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphOutput.h"
#include "skeletalAnimatedComponent.h"

#include "extAnimCutsceneSoundEvent.h"
#include "skeletonUtils.h"
#include "skinningAttachment.h"
#include "skeletonBoneSlot.h"
#include "skeleton.h"
#include "meshSkinningAttachment.h"
#include "poseProvider.h"

IMPLEMENT_ENGINE_CLASS( CSkeletalAnimatedComponent );

CSkeletalAnimatedComponent::CSkeletalAnimatedComponent()
	: m_isOk( false )
	, m_controller( NULL )
	, m_processEvents( false )
{

}

void CSkeletalAnimatedComponent::Initialize()
{
	CreateMatrix();

	InitController();
}

void CSkeletalAnimatedComponent::Deinitialize()
{
	DestroyController();
}

void CSkeletalAnimatedComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	//Initialize();
}

void CSkeletalAnimatedComponent::OnDetached( CWorld *world )
{
	TBaseClass::OnDetached( world );

	Deinitialize();
}

void CSkeletalAnimatedComponent::SyncTo( const CSyncInfo& info )
{
	if ( m_controller )
	{
		m_controller->SyncTo( info );
	}
}

Bool CSkeletalAnimatedComponent::GetSyncInfo( CSyncInfo& info )
{
	if ( m_controller )
	{
		return m_controller->GetSyncInfo( info );
	}
	return false;
}

void CSkeletalAnimatedComponent::RandSync()
{
	if ( m_controller )
	{
		m_controller->RandSync();
	}
}

void CSkeletalAnimatedComponent::CalcBox( Box& box ) const
{
	if ( m_controller )
	{
		m_controller->CalcBox( box );
		box = m_localToWorld.TransformBox( box );
	}
	else
	{
		box = m_localToWorld.TransformBox( Box( Vector( -0.5f, -0.5f, 0.f ), Vector( 0.5f, 0.5f, 2.f ) ) );
	}
}

Bool CSkeletalAnimatedComponent::IsValid() const
{
	return m_isOk;
}

void CSkeletalAnimatedComponent::UpdateAsync( Float timeDelta )
{
	PC_SCOPE_PIX( SkeletalAnimCompUpdate );
	
	ASSERT( m_isOk );
	ASSERT( m_skeletonModelSpace.Size() > 0 );

	// Update
	m_controller->Update( timeDelta );

	// Sample
	SampleAnimations();

	// Calc WS
	SkeletonBonesUtils::MulMatrices( m_skeletonModelSpace.TypedData(), m_skeletonWorldSpace.TypedData(), m_skeletonModelSpace.Size(), &m_localToWorld );

	// Update skinning
	UpdateAttachedSkinningComponentsTransforms();
}

void CSkeletalAnimatedComponent::SampleAnimations()
{
	CPoseProvider* alloc = m_skeleton->GetPoseProvider();
	ASSERT( alloc );

	SAnimationControllerPose pose( alloc->AcquirePose() );
	if ( pose.IsOk() && m_controller->Sample( this, pose ) )
	{
		if ( !pose.m_pose->IsTouched() )
		{
			// Reset pose
			// ctremblay todo already reset from acquire ...
			//alloc->ResetPose( pose.m_pose, true );
		}

		pose.m_pose->GetBonesModelSpace( m_skeleton.Get(), m_skeletonModelSpace );

		if ( m_processEvents )
		{
			ProcessAnimEvents( pose.m_pose.Get() );
		}
	}
}

void CSkeletalAnimatedComponent::ProcessAnimEvents( const SBehaviorGraphOutput* pose )
{
	// We can process only thread-safe events :(
	const Uint32 size = pose->m_numEventsFired;
	for ( Uint32 i=0; i<size; ++i )
	{
		const CAnimationEventFired& evt = pose->m_eventsFired[i];

		// This is hack because of mcinek's sound system.... yeeeee kill me please
		if( IsType< CExtAnimCutsceneSoundEvent >( evt.m_extEvent ) )
		{
			const CExtAnimCutsceneSoundEvent* event = static_cast< const CExtAnimCutsceneSoundEvent* >( evt.m_extEvent );
			event->PlaySound( this );

		}
	}
}

void CSkeletalAnimatedComponent::UpdateAttachedSkinningComponentsTransforms() const
{
	const TList< IAttachment* >& attachments = GetChildAttachments();

	SMeshSkinningUpdateContext skinningContext;

	for ( TList< IAttachment* >::const_iterator it = attachments.Begin(); it != attachments.End(); ++it )
	{
		IAttachment* att = *it;
		CMeshSkinningAttachment* skinAtt = att ? att->ToSkinningAttachment() : nullptr;
		if ( skinAtt )
		{
			Box box( Box::RESET_STATE );
			skinAtt->UpdateTransformAndSkinningData( box, skinningContext );
		}
	}

	skinningContext.CommitCommands();
}

void CSkeletalAnimatedComponent::CreateMatrix()
{
	if ( m_skeleton && m_skeletonWorldSpace.Empty() )
	{
		m_skeletonWorldSpace.Resize( m_skeleton->GetBonesNum() );
		m_skeletonModelSpace.Resize( m_skeleton->GetBonesNum() );
	}
}

void CSkeletalAnimatedComponent::InitController()
{
	m_isOk = true;

	if ( !m_skeleton )
	{
		m_isOk = false;
		return;
	}

	if ( m_controller )
	{
		m_isOk = m_controller->Init( m_skeleton.Get(), m_animset.Get() );
		if ( m_isOk )
		{
			m_controller->CollectEvents( m_processEvents );
		}
	}
	else
	{
		m_isOk = false;
	}
}

void CSkeletalAnimatedComponent::DestroyController()
{
	if ( m_controller )
	{
		m_controller->Destroy();
	}

	m_isOk = false;
}

//////////////////////////////////////////////////////////////////////////

const ISkeletonDataProvider* CSkeletalAnimatedComponent::QuerySkeletonDataProvider() const
{
	return static_cast< const ISkeletonDataProvider* >( this );
}

Int32 CSkeletalAnimatedComponent::FindBoneByName( const Char* name ) const
{
	//dex++
	return m_skeleton ? m_skeleton->FindBoneByName( name ) : -1;
	//dex--
}

Int32 CSkeletalAnimatedComponent::FindBoneByName( const AnsiChar* name ) const
{
	//dex++
	return m_skeleton ? m_skeleton->FindBoneByName( name ) : -1;
	//dex--
}

Int32 CSkeletalAnimatedComponent::GetBonesNum() const
{
	return m_skeleton ? m_skeleton->GetBonesNum() : 0;
}

Uint32 CSkeletalAnimatedComponent::GetBones( TDynArray< BoneInfo >& bones ) const
{
	//dex++
	return m_skeleton ? m_skeleton->GetBones( bones ) : 0;
	//dex--
}

Uint32 CSkeletalAnimatedComponent::GetBones( TAllocArray< BoneInfo >& bones ) const
{
	//dex++
	return m_skeleton ? m_skeleton->GetBones( bones ) : 0;
	//dex--
}

Matrix CSkeletalAnimatedComponent::CalcBoneMatrixModelSpace( Uint32 boneIndex ) const
{
	return Matrix::IDENTITY;
}

Matrix CSkeletalAnimatedComponent::GetBoneMatrixModelSpace( Uint32 boneIndex ) const
{
	return Matrix::IDENTITY;
}

Matrix CSkeletalAnimatedComponent::GetBoneMatrixWorldSpace( Uint32 boneIndex ) const
{
	return m_skeletonWorldSpace.Size() > boneIndex ? m_skeletonWorldSpace[ boneIndex ] : Matrix::IDENTITY;
}

void CSkeletalAnimatedComponent::GetBoneMatricesAndBoundingBoxModelSpace( const ISkeletonDataProvider::SBonesData& bonesData ) const
{
	// Check CAnimatedComponent::GetBoneMatricesAndBoundingBoxWorldSpace
	SkeletonBonesUtils::GetBoneMatricesModelSpace( bonesData, m_skeletonModelSpace );
	SkeletonBonesUtils::CalcBoundingBoxModelSpace( bonesData, nullptr, 0, m_skeletonModelSpace );
}

Uint32 CSkeletalAnimatedComponent::GetRuntimeCacheIndex() const
{
	return m_skeleton ? m_skeleton->GetRuntimeIndex() : 0;
}

const struct SSkeletonSkeletonCacheEntryData* CSkeletalAnimatedComponent::GetSkeletonMaping( const ISkeletonDataProvider* parentSkeleton ) const
{
	if ( m_skeleton )
		return m_skeleton->GetMappingCache().GetMappingEntry( parentSkeleton );

	return &SSkeletonSkeletonCacheEntryData::FAKE_DATA;
}

//////////////////////////////////////////////////////////////////////////

ISlotProvider* CSkeletalAnimatedComponent::QuerySlotProvider()
{
	return static_cast< ISlotProvider* >( this );
}

ISlot* CSkeletalAnimatedComponent::CreateSlot( const String& slotName )
{
	// Try bone slot
	TDynArray< BoneInfo > bones;
	GetBones( bones );

	// Linear search by name
	for ( Uint32 i=0; i<bones.Size(); i++ )
	{
		const BoneInfo& bone = bones[i];
		if ( bone.m_name == CName( slotName ) )
		{
			// Create skeleton slot
			CSkeletonBoneSlot *newSlot = new CSkeletonBoneSlot( i );
			newSlot->SetParent( this );
			return newSlot;
		}
	}

	// Slot not found
	return NULL;
}

void CSkeletalAnimatedComponent::EnumSlotNames( TDynArray< String >& slotNames ) const
{
	TDynArray< BoneInfo > bones;
	GetBones( bones );

	for ( Uint32 i = 0; i < bones.Size(); ++i )
	{
		slotNames.PushBack( bones[i].m_name.AsString() );
	}
}
