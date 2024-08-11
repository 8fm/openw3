
#include "build.h"
#include "animDangleBufferComponent.h"
#include "animDangleConstraint.h"
#include "skeletonUtils.h"
#include "skinningAttachment.h"
#include "animatedComponent.h"
#include "skeleton.h"
#include "renderFrame.h"
#include "world.h"
#include "materialInstance.h"
#include "skeletonSkeletonMappingCache.h"
#include "meshSkinningAttachment.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CAnimDangleBufferComponent );

CAnimDangleBufferComponent::CAnimDangleBufferComponent()
	: m_cachedParentAnimatedComponent( nullptr )
	, m_skeleton( nullptr )
	, m_debugRender( false )
	, m_bbox( Box::RESET_STATE )
{

}

void CAnimDangleBufferComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CAnimDangleBufferComponent_OnAttached );

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_AnimDangles );

	CacheAttachedToAnimParent();
	CacheAnimatedAttachments();

	if ( CWetnessComponent* wc = GetEntity()->FindComponent< CWetnessComponent >() )
	{
		if ( const CSkeleton* sk = GetSkeleton() )
		{
			RED_FATAL_ASSERT( m_wetnessSupplier == nullptr, "Something went wrong with attach. Wetness will leak" );
			m_wetnessSupplier = new CWetnessSupplier( wc, sk->GetBonesNum() );
		}
	}
}

void CAnimDangleBufferComponent::OnDetached( CWorld* world )
{
	m_cachedAnimatedAttachments.Clear();

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_AnimDangles );

	if( m_wetnessSupplier )
	{
		delete m_wetnessSupplier;
		m_wetnessSupplier = nullptr;
	}

	TBaseClass::OnDetached( world );
}

void CAnimDangleBufferComponent::OnParentAttachmentBroken( IAttachment* attachment )
{
	TBaseClass::OnParentAttachmentBroken( attachment );

	CacheAttachedToAnimParent();
}

void CAnimDangleBufferComponent::OnChildAttachmentAdded( IAttachment* attachment )
{
	TBaseClass::OnChildAttachmentAdded( attachment );

	if ( attachment->IsA< CAnimatedAttachment >() )
	{
		m_cachedAnimatedAttachments.PushBack( static_cast< CAnimatedAttachment* >( attachment ) );
	}

	CacheAttachedToAnimParent();
}

void CAnimDangleBufferComponent::OnChildAttachmentBroken( IAttachment* attachment )
{
	TBaseClass::OnChildAttachmentBroken( attachment );

	if ( attachment->IsA< CAnimatedAttachment >() )
	{
		m_cachedAnimatedAttachments.Remove( static_cast< CAnimatedAttachment* >( attachment ) );
	}

	CacheAttachedToAnimParent();
}

const CSkeleton* CAnimDangleBufferComponent::GetSkeleton() const
{
	return m_skeleton.Get(); //m_cachedParentAnimatedComponent ? m_cachedParentAnimatedComponent->GetSkeleton() : nullptr;
}

void CAnimDangleBufferComponent::CacheAttachedToAnimParent()
{
	m_cachedParentAnimatedComponent = GetTransformParent() ? Cast< CAnimatedComponent >( GetTransformParent()->GetParent() ) : nullptr;

	const CSkeleton* skeleton = GetSkeleton();
	if ( skeleton )
	{
		m_skeletonModelSpace.Resize( skeleton->GetBonesNum() );
		m_skeletonWorldSpace.Resize( skeleton->GetBonesNum() );

		ForceTPose();
	}
}

void CAnimDangleBufferComponent::CacheAnimatedAttachments()
{
	for ( TList< IAttachment* >::iterator it = m_childAttachments.Begin(); it != m_childAttachments.End(); ++it )
	{
		IAttachment* att = *it;

		ASSERT( att->GetChild(), TXT("Attachment without child?") );

		if ( att->IsA< CAnimatedAttachment >() )
		{
			CAnimatedAttachment* aa = static_cast< CAnimatedAttachment* >( att );

			// This should be already added to list in void CAnimatedComponent::OnChildAttachmentAdded( IAttachment* attachment )
			// No it doesn't work after some changes with streaming....
			//ASSERT( m_cachedAnimatedAttachments.Exist( aa ) );

			m_cachedAnimatedAttachments.PushBackUnique( aa );
		}
	}

	CacheAttachedToAnimParent();
}

void CAnimDangleBufferComponent::ForceTPose()
{
	const CSkeleton* skeleton = GetSkeleton();
	if ( skeleton )
	{
		skeleton->GetBonesMS( m_skeletonModelSpace );
	}
}

void CAnimDangleBufferComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	PC_SCOPE( CAnimDangleBufferComponent );

	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );

	if ( m_skeletonModelSpace.Size() > 0 )
	{
		const Matrix& l2w = GetLocalToWorld();
		SkeletonBonesUtils::MulMatrices( m_skeletonModelSpace.TypedData(), m_skeletonWorldSpace.TypedData(), m_skeletonModelSpace.Size(), &l2w );

		const TList< IAttachment* >& attachments = GetChildAttachments();
		for ( TList< IAttachment* >::const_iterator it = attachments.Begin(); it != attachments.End(); ++it )
		{
			IAttachment* att = *it;
			CMeshSkinningAttachment* skinAtt = att ? att->ToSkinningAttachment() : nullptr;
			if ( skinAtt )
			{
				skinAtt->UpdateTransformAndSkinningData( m_bbox, context.m_skinningContext );
			}
		}
	}
}
#ifndef NO_EDITOR_FRAGMENTS
void CAnimDangleBufferComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	TBaseClass::OnGenerateEditorFragments( frame, flags );

	if ( m_debugRender )
	{
		const CSkeleton* skeleton = GetSkeleton();
		if ( skeleton )
		{
			for ( Uint32 i=0; i<m_skeletonWorldSpace.Size(); ++i )
			{
				const Int32 parentIndex = skeleton->GetParentBoneIndex( (Int32)i );
				if ( parentIndex != -1 )
				{
					const Matrix& childBone = m_skeletonWorldSpace[ i ];
					const Matrix& parentBone = m_skeletonWorldSpace[ parentIndex ];

					frame->AddDebugLine( parentBone.GetTranslation(), childBone.GetTranslation(), Color( 255,255,255 ), true );
				}
			}
		}
	}
}
#endif

const ISkeletonDataProvider* CAnimDangleBufferComponent::QuerySkeletonDataProvider() const
{
	return static_cast< const ISkeletonDataProvider* >( this );
}

Int32 CAnimDangleBufferComponent::FindBoneByName( const Char* name ) const
{
	const CSkeleton* skeleton = GetSkeleton();
	return skeleton ? skeleton->FindBoneByName( name ) : -1;
}

Uint32 CAnimDangleBufferComponent::GetBones( TDynArray< ISkeletonDataProvider::BoneInfo >& bones ) const
{
	const CSkeleton* skeleton = GetSkeleton();
	if ( skeleton )
	{
		return skeleton->GetBones( bones );
	}
	return 0;
}

Uint32 CAnimDangleBufferComponent::GetBones( TAllocArray< ISkeletonDataProvider::BoneInfo >& bones ) const
{
	const CSkeleton* skeleton = GetSkeleton();
	if ( skeleton )
	{
		return skeleton->GetBones( bones );
	}
	return 0;
}

Bool CAnimDangleBufferComponent::HasSkeleton() const
{
	const CSkeleton* skeleton = GetSkeleton();
	return skeleton != nullptr;
}


Int32 CAnimDangleBufferComponent::GetBonesNum() const
{
	const CSkeleton* skeleton = GetSkeleton();
	return skeleton ? skeleton->GetBonesNum() : 0;
}

Int32 CAnimDangleBufferComponent::GetTracksNum() const
{
	const CSkeleton* skeleton = GetSkeleton();
	return skeleton ? skeleton->GetTracksNum() : 0;
}

Int32 CAnimDangleBufferComponent::GetParentBoneIndex( Int32 bone ) const
{
	const CSkeleton* skeleton = GetSkeleton();
	return skeleton ? skeleton->GetParentBoneIndex( bone ) : -1;
}

const Int16* CAnimDangleBufferComponent::GetParentIndices() const
{
	const CSkeleton* skeleton = GetSkeleton();
	return skeleton ? skeleton->GetParentIndices() : nullptr;
}

Uint32 CAnimDangleBufferComponent::GetRuntimeCacheIndex() const
{
	const CSkeleton* skeleton = GetSkeleton();
	if ( skeleton )
	{
		return skeleton->GetRuntimeIndex();
	}
	return 0;
}

const struct SSkeletonSkeletonCacheEntryData* CAnimDangleBufferComponent::GetSkeletonMaping( const ISkeletonDataProvider* parentSkeleton ) const
{
	const CSkeleton* skeleton = GetSkeleton();
	if ( skeleton )
	{
		return skeleton->GetMappingCache().GetMappingEntry( parentSkeleton );
	}

	return &SSkeletonSkeletonCacheEntryData::FAKE_DATA;
}

Matrix CAnimDangleBufferComponent::CalcBoneMatrixModelSpace( Uint32 boneIndex ) const
{
	return Matrix::IDENTITY;
}

Matrix CAnimDangleBufferComponent::GetBoneMatrixModelSpace( Uint32 boneIndex ) const
{
	return Matrix::IDENTITY;
}

Matrix CAnimDangleBufferComponent::GetBoneMatrixWorldSpace( Uint32 boneIndex ) const
{
	return Matrix::IDENTITY;
}

void CAnimDangleBufferComponent::GetBoneMatricesAndBoundingBoxModelSpace( const ISkeletonDataProvider::SBonesData& bonesData ) const
{
	if( m_wetnessSupplier != nullptr )
	{
		SkeletonBonesUtils::GetBoneMatricesModelSpaceWithWetnessData( bonesData, m_skeletonModelSpace, m_wetnessSupplier );
	}
	else
	{
		SkeletonBonesUtils::GetBoneMatricesModelSpace( bonesData, m_skeletonModelSpace );
	}
	SkeletonBonesUtils::CalcBoundingBoxModelSpace( bonesData, nullptr, 0, m_skeletonModelSpace );
}

IAnimatedObjectInterface* CAnimDangleBufferComponent::QueryAnimatedObjectInterface()
{
	return static_cast< IAnimatedObjectInterface* >( this );
}

void CAnimDangleBufferComponent::OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	const Uint32 size = m_cachedAnimatedAttachments.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CAnimatedAttachment* att = m_cachedAnimatedAttachments[ i ];

		att->ParentPoseUpdateLS( dt, poseLS );
	}
}

void CAnimDangleBufferComponent::OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	const Uint32 size = m_cachedAnimatedAttachments.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CAnimatedAttachment* att = m_cachedAnimatedAttachments[ i ];

		att->ParentPoseUpdateLSAsync( dt, poseLS );
	}
}

void CAnimDangleBufferComponent::OnParentUpdatedAttachedAnimatedObjectsLSSync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	const Uint32 size = m_cachedAnimatedAttachments.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CAnimatedAttachment* att = m_cachedAnimatedAttachments[ i ];

		att->ParentPoseUpdateLSSync( dt, poseLS );
	}
}

void CAnimDangleBufferComponent::OnParentUpdatedAttachedAnimatedObjectsLSConstrainted( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	const Uint32 size = m_cachedAnimatedAttachments.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CAnimatedAttachment* att = m_cachedAnimatedAttachments[ i ];

		att->ParentPoseUpdateLSConstrainted( dt, poseLS );
	}
}

void CAnimDangleBufferComponent::OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones )
{
	const CSkeleton* s = GetSkeleton();

	if ( poseMS && poseMS->Size() > 0 && poseWS && poseWS->Size() > 0 && s )
	{
		if ( m_skeletonModelSpace.Size() > 0 )
		{
			IAnimDangleConstraint::AlignBonesFull( poseMS, poseWS, bones, m_skeletonModelSpace, m_skeletonWorldSpace, s );
		}

		const Uint32 size = m_cachedAnimatedAttachments.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			CAnimatedAttachment* att = m_cachedAnimatedAttachments[ i ];

			att->ParentPoseUpdateWS( poseLS, &m_skeletonModelSpace, &m_skeletonWorldSpace );
		}
		if( m_wetnessSupplier )
		{
			m_wetnessSupplier->CalcWetness( m_skeletonWorldSpace );
		}
	}
}

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",on)
#endif

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
