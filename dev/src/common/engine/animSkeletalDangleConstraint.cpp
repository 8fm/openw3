
#include "build.h"
#include "animSkeletalDangleConstraint.h"
#include "animDangleBufferComponent.h"
#include "skeletonUtils.h"
#include "skinningAttachment.h"
#include "skeleton.h"
#include "renderFrame.h"
#include "meshSkinningAttachment.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CAnimSkeletalDangleConstraint );

CAnimSkeletalDangleConstraint::CAnimSkeletalDangleConstraint() 
	: m_skeleton( NULL )
	, m_firstUpdate( false )
	, m_dispSkeleton( false )
	, m_dispBoneNames( false )
	, m_dispBoneAxis( false )
	, m_bbox( Box::RESET_STATE )
	, m_wetnessSupplier( nullptr )
{
}

#ifndef NO_EDITOR
void CAnimSkeletalDangleConstraint::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("skeleton") )
	{
		if ( GetSkeleton() )
		{
			m_skeletonModelSpace.Resize( GetSkeleton()->GetBonesNum() );
			m_skeletonWorldSpace.Resize( GetSkeleton()->GetBonesNum() );
			m_dirtyBones.Reserve( GetSkeleton()->GetBonesNum() );
		}
		else
		{
			m_skeletonModelSpace.Clear();
			m_skeletonWorldSpace.Clear();
			m_dirtyBones.Clear();
		}

		CacheBones( GetParentComponent(), GetSkeleton() );
	}
}
#endif

void CAnimSkeletalDangleConstraint::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	const CSkeleton* skeleton = GetSkeleton();
	if ( skeleton )
	{
		m_skeletonModelSpace.Resize( skeleton->GetBonesNum() );
		m_skeletonWorldSpace.Resize( skeleton->GetBonesNum() );
		m_dirtyBones.Reserve( skeleton->GetBonesNum() );
	}

	if ( !HasCachedBones() )
	{
		CacheBones( GetParentComponent(), skeleton );
	}

	if ( !HasCachedBones() )
	{
		ForceTPose();
	}

	if ( CWetnessComponent* wc = GetComponent()->GetEntity()->FindComponent< CWetnessComponent >() )
	{
		if( skeleton )
		{
			RED_FATAL_ASSERT( m_wetnessSupplier == nullptr, "Something went wrong with attach. Wetness will leak" );
			m_wetnessSupplier = new CWetnessSupplier( wc, skeleton->GetBonesNum() );
		}
	}
}

void CAnimSkeletalDangleConstraint::OnDetached( CWorld* world )
{
	m_skeletonModelSpace.Clear();
	m_skeletonWorldSpace.Clear();
	m_dirtyBones.Clear();

	if( m_wetnessSupplier )
	{
		delete m_wetnessSupplier;
		m_wetnessSupplier = nullptr;
	}

	TBaseClass::OnDetached( world );
}

void CAnimSkeletalDangleConstraint::OnItemEntityAttached( const CEntity* par )
{
	TBaseClass::OnItemEntityAttached( par );
	if ( CWetnessComponent* wc = par->FindComponent< CWetnessComponent >() )
	{
		if( const CSkeleton* sk = GetSkeleton() )
		{
			if ( m_wetnessSupplier )
			{
				delete m_wetnessSupplier;
			}
			m_wetnessSupplier = new CWetnessSupplier( wc, sk->GetBonesNum() );
		}
	}
}

void CAnimSkeletalDangleConstraint::OnAttachmentAdded( const IAttachment* attachment )
{
	TBaseClass::OnAttachmentAdded( attachment );

	if ( !HasCachedBones() )
	{
		CacheBones( GetParentComponent(), GetSkeleton() );
	}
}

void CAnimSkeletalDangleConstraint::OnAttachmentBroken( const IAttachment* attachment )
{
	TBaseClass::OnAttachmentBroken( attachment );

	CacheBones( GetParentComponent(), GetSkeleton() );
}

void CAnimSkeletalDangleConstraint::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	PC_SCOPE( CAnimSkeletalDangleConstraint );

	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );
	
	const Matrix& l2w = GetComponent()->GetLocalToWorld();

	const Bool doNotrecalcAll = HasCachedBones();
	if ( doNotrecalcAll )
	{
		const Uint32 numDirtyBones = m_dirtyBones.Size();
		for ( Uint32 i=0; i<numDirtyBones; ++i )
		{
			const Int32 boneToCalc = m_dirtyBones[ i ];
			m_skeletonWorldSpace[ boneToCalc ] = Matrix::Mul( l2w, m_skeletonModelSpace[ boneToCalc ] );
		}
	}
	else if( m_skeletonModelSpace.Size() > 0 && m_skeletonWorldSpace.Size() > 0 )
	{
		SkeletonBonesUtils::MulMatrices( m_skeletonModelSpace.TypedData(), m_skeletonWorldSpace.TypedData(), m_skeletonModelSpace.Size(), &l2w );
	}

	if ( m_skeletonWorldSpace.Size() > 0 )
	{
		const TList< IAttachment* >& attachments = GetComponent()->GetChildAttachments();
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

void CAnimSkeletalDangleConstraint::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	TBaseClass::OnGenerateEditorFragments( frame, flags );	

	OnDrawSkeletonWS( frame );
}

Int32 CAnimSkeletalDangleConstraint::FindBoneByName( const Char* name ) const
{
	const CSkeleton* skeleton = GetSkeleton();
	return skeleton ? skeleton->FindBoneByName( name ) : -1;
}

Uint32 CAnimSkeletalDangleConstraint::GetBones( TDynArray< ISkeletonDataProvider::BoneInfo >& bones ) const
{
	const CSkeleton* skeleton = GetSkeleton();
	if ( skeleton )
	{
		return skeleton->GetBones( bones );
	}
	return 0;
}

Uint32 CAnimSkeletalDangleConstraint::GetBones( TAllocArray< ISkeletonDataProvider::BoneInfo >& bones ) const
{
	const CSkeleton* skeleton = GetSkeleton();
	if ( skeleton )
	{
		return skeleton->GetBones( bones );
	}
	return 0;
}

Uint32 CAnimSkeletalDangleConstraint::GetRuntimeCacheIndex() const
{
	const CSkeleton* skeleton = GetSkeleton();
	if ( skeleton )
	{
		return skeleton->GetRuntimeIndex();
	}
	return 0;
}

const struct SSkeletonSkeletonCacheEntryData* CAnimSkeletalDangleConstraint::GetSkeletonMaping( const ISkeletonDataProvider* parentSkeleton ) const
{
	const CSkeleton* skeleton = GetSkeleton();
	if ( skeleton )
	{
		return skeleton->GetMappingCache().GetMappingEntry(parentSkeleton);
	}

	return &SSkeletonSkeletonCacheEntryData::FAKE_DATA;
}

Matrix CAnimSkeletalDangleConstraint::CalcBoneMatrixModelSpace( Uint32 boneIndex ) const
{
	return Matrix::IDENTITY;
}

Matrix CAnimSkeletalDangleConstraint::GetBoneMatrixModelSpace( Uint32 boneIndex ) const
{
	return Matrix::IDENTITY;
}

Matrix CAnimSkeletalDangleConstraint::GetBoneMatrixWorldSpace( Uint32 boneIndex ) const
{
	return Matrix::IDENTITY;
}

void CAnimSkeletalDangleConstraint::GetBoneMatricesAndBoundingBoxModelSpace( const ISkeletonDataProvider::SBonesData& bonesData ) const
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

void CAnimSkeletalDangleConstraint::OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	if ( !m_firstUpdate )
	{
		if ( !HasCachedBones() )
		{
			const CSkeleton* skeleton = GetSkeleton();
			CacheBones( GetParentComponent(), skeleton );
		}

		m_firstUpdate = true;
	}
}

void CAnimSkeletalDangleConstraint::OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	if ( !m_firstUpdate )
	{
		if ( !HasCachedBones() )
		{
			const CSkeleton* skeleton = GetSkeleton();
			CacheBones( GetParentComponent(), skeleton );
		}

		m_firstUpdate = true;
	}
}

void CAnimSkeletalDangleConstraint::OnParentUpdatedAttachedAnimatedObjectsLSSync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{

}

void CAnimSkeletalDangleConstraint::OnParentUpdatedAttachedAnimatedObjectsLSConstrainted( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{

}

void CAnimSkeletalDangleConstraint::OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones )
{
	if ( !m_firstUpdate )
	{
		if ( !HasCachedBones() )
		{
			const CSkeleton* skeleton = GetSkeleton();
			CacheBones( GetParentComponent(), skeleton );
		}

		m_firstUpdate = true;
	}
	if ( m_wetnessSupplier && !m_skeletonWorldSpace.Empty() )
	{
		const CSkeleton* skeleton = GetSkeleton();
		m_wetnessSupplier->CalcWetness( m_skeletonWorldSpace );
	}
}

void CAnimSkeletalDangleConstraint::OnDrawSkeletonWS( CRenderFrame* frame ) const
{
	const CSkeleton* skeleton = GetSkeleton();
	if ( skeleton )
	{
		Color c( 255,255,255 );
		SkeletonRenderingUtils::DrawSkeleton( m_skeletonWorldSpace, skeleton, c, frame, m_dispSkeleton, m_dispBoneNames, m_dispBoneAxis );
	}
}

const CComponent* CAnimSkeletalDangleConstraint::GetParentComponent() const
{
	const CComponent* c = GetComponent();
	if ( c && c->GetTransformParent() )
	{
		const CComponent* ac = Cast< const CComponent >( c->GetTransformParent()->GetParent() );

		// TODO - if null go deeper
		//...

		return ac;
	}

	return NULL;
}

void CAnimSkeletalDangleConstraint::ForceTPose()
{
	const CSkeleton* skeleton = GetSkeleton();
	if ( skeleton )
	{
		skeleton->GetBonesMS( m_skeletonModelSpace );
	}
}

void CAnimSkeletalDangleConstraint::AlignBones( const TDynArray< Matrix >* poseMS, const TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones )
{
	IAnimDangleConstraint::AlignBones( poseMS, poseWS, bones, m_skeletonModelSpace, m_skeletonWorldSpace );
}

void CAnimSkeletalDangleConstraint::StartUpdate()
{
	ClearDirtyBones();
}

void CAnimSkeletalDangleConstraint::StartUpdateWithBoneAlignment( const TDynArray< Matrix >* poseMS, const TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones )
{
	ClearDirtyBones();

	if ( HasSkeletalMatrices() )
	{
		AlignBones( poseMS, poseWS, bones );
	}
}

void CAnimSkeletalDangleConstraint::StartUpdateWithBoneAlignmentFull( const TDynArray< Matrix >* poseMS, const TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones )
{
	ClearDirtyBones();

	const CSkeleton* skel = GetSkeleton();

	if( HasSkeletalMatrices() && skel )
	{
		IAnimDangleConstraint::AlignBonesFull( poseMS, poseWS, bones, m_skeletonModelSpace, m_skeletonWorldSpace, skel );
	}
}

void CAnimSkeletalDangleConstraint::EndUpdate( TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones )
{
	// TODO
	if ( Cast< CAnimDangleBufferComponent >( GetParentComponent() ) )
	{
		const Int32 numBones = bones.SizeInt();

		const Uint32 numDirtyBones = m_dirtyBones.Size();
		for ( Uint32 i=0; i<numDirtyBones; ++i )
		{
			const Int32 boneToCalc = m_dirtyBones[ i ];

			if ( boneToCalc < numBones && bones[ boneToCalc ].m_boneA == boneToCalc )
			{
				(*poseMS)[ bones[ boneToCalc ].m_boneB ] = m_skeletonModelSpace[ boneToCalc ];
			}
			else
			{
				for ( Int32 j=0; j<numBones; ++j )
				{
					if ( bones[ j ].m_boneA == boneToCalc )
					{
						(*poseMS)[ bones[ j ].m_boneB ] = m_skeletonModelSpace[ boneToCalc ];
						break;
					}
				}
			}
		}
	}
}

Bool CAnimSkeletalDangleConstraint::HasCachedBones() const
{
	return true;
}

void CAnimSkeletalDangleConstraint::CacheBones( const CComponent* parent, const CSkeleton* skeleton )
{

}

#ifndef NO_EDITOR
Bool CAnimSkeletalDangleConstraint::PrintDebugComment( String& str ) const
{
	Bool ret = TBaseClass::PrintDebugComment( str );

	const CSkeleton* s = GetSkeleton();
	if ( !s )
	{
		str += TXT("m_skeleton is null; ");
		ret = true;
	}

	return ret;
}
#endif

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",on)
#endif

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
