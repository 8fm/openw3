
#include "build.h"
#include "animDangleComponent.h"
#include "animDangleConstraint.h"
#include "renderFrame.h"
#include "world.h"
#include "materialInstance.h"
#include "meshSkinningAttachment.h"
#include "meshComponent.h"
#include "mesh.h"
#include "dyngConstraint.h"
#include "dyngResource.h"
#include "skeletonSkeletonMappingCache.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CAnimDangleComponent );

CAnimDangleComponent::CAnimDangleComponent()
	: m_debugRender( false )
	, m_attPrio( 0 )
	, m_constraint( nullptr )
{
}

void CAnimDangleComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CAnimDangleComponent_OnAttached );

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_AnimDangles );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Wetness );

	if ( m_constraint )
	{
		m_constraint->OnAttached( world );
	}
}

void CAnimDangleComponent::OnDetached( CWorld* world )
{
	if ( m_constraint )
	{
		m_constraint->OnDetached( world );
	}

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_AnimDangles );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Wetness );
	TBaseClass::OnDetached( world );
}

void CAnimDangleComponent::OnChildAttachmentAdded( IAttachment* attachment )
{
	TBaseClass::OnChildAttachmentAdded( attachment );

	if ( m_constraint )
	{
		m_constraint->OnAttachmentAdded( attachment );
	}
}

void CAnimDangleComponent::OnChildAttachmentBroken( IAttachment* attachment )
{
	if ( m_constraint )
	{
		m_constraint->OnAttachmentBroken( attachment );
	}

	TBaseClass::OnChildAttachmentBroken( attachment );
}

void CAnimDangleComponent::OnItemEntityAttached( const CEntity* par )
{
	if ( m_constraint )
	{
		m_constraint->OnItemEntityAttached( par );
	}
}

void CAnimDangleComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	PC_SCOPE( AnimDangleUpdateTransform );

	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );

	if ( m_constraint )
	{
		m_constraint->OnUpdateTransformComponent( context, prevLocalToWorld );
	}
}

void CAnimDangleComponent::SetResource( CResource* res )
{
	if ( CDyngResource* dyngRes = Cast< CDyngResource >( res ) )
	{
		CAnimDangleConstraint_Dyng* dyngConstraint = CreateObject< CAnimDangleConstraint_Dyng >( this );
		if ( dyngConstraint )
		{
			dyngConstraint->SetResource( dyngRes );
			m_constraint = dyngConstraint;
		}
	}
}

void CAnimDangleComponent::GetResource( TDynArray< const CResource* >& resources ) const
{
	TDynArray< CObject* > children;
	GetChildren( children );

	CAnimDangleConstraint_Dyng* dyngConstraint = Cast< CAnimDangleConstraint_Dyng >( m_constraint );
	if ( dyngConstraint )
	{
		resources.PushBack( dyngConstraint->GetResource() );
	}
}

void CAnimDangleComponent::ShowDebugRender( Bool flag )
{
	m_debugRender = flag;

	if ( m_constraint )
	{
		m_constraint->OnShowDebugRender( flag );
	}
}

void CAnimDangleComponent::OnCollectAnimationSyncTokens( CName animationName, TDynArray< CAnimationSyncToken* >& tokens )
{
	if ( m_constraint )
	{
		m_constraint->OnCollectAnimationSyncTokens( animationName, tokens );
	}
}

void CAnimDangleComponent::SetBlendToAnimationWeight( Float w )
{
	if ( m_constraint )
	{
		m_constraint->SetBlendToAnimationWeight( w );
	}
}

void CAnimDangleComponent::ForceReset()
{
	if ( m_constraint )
	{
		m_constraint->ForceReset();
	}
}

void CAnimDangleComponent::ForceResetWithRelaxedState()
{
	if ( m_constraint )
	{
		m_constraint->ForceResetWithRelaxedState();
	}
}

#ifndef NO_EDITOR_FRAGMENTS
void CAnimDangleComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	TBaseClass::OnGenerateEditorFragments( frame, flags );

	if ( m_debugRender && m_constraint && flags == SHOW_AnimDangles )
	{
		m_constraint->OnGenerateEditorFragments( frame, flags );
	}
}
#endif

const ISkeletonDataProvider* CAnimDangleComponent::QuerySkeletonDataProvider() const
{
	return static_cast< const ISkeletonDataProvider* >( this );
}

Uint32 CAnimDangleComponent::GetRuntimeCacheIndex() const
{
	return m_constraint ? m_constraint->GetRuntimeCacheIndex() : 0;
}

const struct SSkeletonSkeletonCacheEntryData* CAnimDangleComponent::GetSkeletonMaping( const ISkeletonDataProvider* parentSkeleton ) const
{
	return m_constraint ? m_constraint->GetSkeletonMaping( parentSkeleton ) : &SSkeletonSkeletonCacheEntryData::FAKE_DATA;
}

Int32 CAnimDangleComponent::FindBoneByName( const Char* name ) const
{
	return m_constraint ? m_constraint->FindBoneByName( name ) : -1;
}

Uint32 CAnimDangleComponent::GetBones( TDynArray< ISkeletonDataProvider::BoneInfo >& bones ) const
{
	return m_constraint ? m_constraint->GetBones( bones ) : 0;
}

Uint32 CAnimDangleComponent::GetBones( TAllocArray< ISkeletonDataProvider::BoneInfo >& bones ) const
{
	return m_constraint ? m_constraint->GetBones( bones ) : 0;
}

Matrix CAnimDangleComponent::CalcBoneMatrixModelSpace( Uint32 boneIndex ) const
{
	return m_constraint ? m_constraint->CalcBoneMatrixModelSpace( boneIndex ) : Matrix::IDENTITY;
}

Matrix CAnimDangleComponent::GetBoneMatrixModelSpace( Uint32 boneIndex ) const
{
	return m_constraint ? m_constraint->GetBoneMatrixModelSpace( boneIndex ) : Matrix::IDENTITY;
}

Matrix CAnimDangleComponent::GetBoneMatrixWorldSpace( Uint32 boneIndex ) const
{
	return m_constraint ? m_constraint->GetBoneMatrixWorldSpace( boneIndex ) : Matrix::IDENTITY;
}

void CAnimDangleComponent::GetBoneMatricesAndBoundingBoxModelSpace( const ISkeletonDataProvider::SBonesData& bonesData ) const
{
	if ( m_constraint )
	{
		m_constraint->GetBoneMatricesAndBoundingBoxModelSpace( bonesData );
	}
}

IAnimatedObjectInterface* CAnimDangleComponent::QueryAnimatedObjectInterface()
{
	return static_cast< IAnimatedObjectInterface* >( this );
}

Int32 CAnimDangleComponent::GetAttPrio() const
{
	return m_attPrio;
}

void CAnimDangleComponent::OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	if ( m_constraint && IsAttached() )
	{
		m_constraint->OnParentUpdatedAttachedAnimatedObjectsLS( dt, poseLS, bones );
	}
}

void CAnimDangleComponent::OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	if ( m_constraint && IsAttached() )
	{
		m_constraint->OnParentUpdatedAttachedAnimatedObjectsLSAsync( dt, poseLS, bones );
	}
}

void CAnimDangleComponent::OnParentUpdatedAttachedAnimatedObjectsLSSync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	if ( m_constraint && IsAttached() )
	{
		m_constraint->OnParentUpdatedAttachedAnimatedObjectsLSSync( dt, poseLS, bones );
	}
}

void CAnimDangleComponent::OnParentUpdatedAttachedAnimatedObjectsLSConstrainted( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	if ( m_constraint && IsAttached() )
	{
		m_constraint->OnParentUpdatedAttachedAnimatedObjectsLSConstrainted( dt, poseLS, bones );
	}
}

void CAnimDangleComponent::OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones )
{
	if ( m_constraint && poseLS && poseMS && poseWS && poseMS->Size() > 0 && IsAttached() )
	{
		m_constraint->OnParentUpdatedAttachedAnimatedObjectsWS( parent, poseLS, poseMS, poseWS, bones );
	}
}

// reset physical simulation on component
void CAnimDangleComponent::OnResetClothAndDangleSimulation()
{
	TBaseClass::OnResetClothAndDangleSimulation();
	ForceResetWithRelaxedState();
}

void CAnimDangleComponent::SetShakeFactor( Float factor )
{
	if ( m_constraint )
	{
		m_constraint->SetShakeFactor( factor );
	}
}

#ifndef NO_EDITOR

Bool CAnimDangleComponent::PrintDebugComment( String& str ) const
{
	Bool ret = false;

	String boneMappingWarn;
	const TList< IAttachment* >& attachments = GetChildAttachments();
	for ( auto it = attachments.Begin(); it != attachments.End(); ++it )
	{
		if ( CMeshSkinningAttachment* skinAtt = Cast< CMeshSkinningAttachment >( *it ) )
		{
			String localWarn;

			const CMeshComponent* mc = Cast< const CMeshComponent >( skinAtt->GetChild() );
			const CMesh* mesh = mc ? mc->GetMeshNow() : nullptr;
			
			const Uint32 numMeshBones = mesh ? mesh->GetBoneCount() : 0;
			const CName* meshBoneNames = mesh ? mesh->GetBoneNames() : nullptr;

			const auto& mapping = skinAtt->GetBoneMapping();
			for ( Uint32 i=0; i<mapping.Size(); ++i )
			{
				Int32 b = mapping[ i ];
				if ( b == -1 )
				{
					if ( localWarn.Empty() )
					{
						localWarn += String::Printf( TXT("Mesh: '%ls'\n"), skinAtt->GetChild()->GetName().AsChar() );
						localWarn += TXT("Check skeleton bone(s):\n");
					}

					CName boneName = i < numMeshBones ? meshBoneNames[ i ] : CName::NONE;

					localWarn += String::Printf( TXT("   >%d (%s)\n"), i, boneName.AsString().AsChar() );
				}
			}

			if ( !localWarn.Empty() )
			{
				boneMappingWarn += localWarn;
			}
		}
	}

	if ( !boneMappingWarn.Empty() )
	{
		str += String::Printf( TXT("Bone mapping error:\n%s\n"), boneMappingWarn.AsChar() );
		ret = true;
	}

	ret |= m_constraint ? m_constraint->PrintDebugComment( str ) : false;

	return ret;
}

#endif

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",on)
#endif

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
