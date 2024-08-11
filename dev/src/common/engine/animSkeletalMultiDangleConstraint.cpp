
#include "build.h"
#include "animSkeletalMultiDangleConstraint.h"
#include "animatedComponent.h"
#include "skeleton.h"

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CAnimSkeletalMultiDangleConstraint );

CAnimSkeletalMultiDangleConstraint::CAnimSkeletalMultiDangleConstraint() 
	: CAnimSkeletalDangleConstraint()
{
}

#ifndef NO_EDITOR
void CAnimSkeletalMultiDangleConstraint::OnPropertyPostChange( IProperty* property )
{
	if ( property->GetName() == TXT("skeleton") )
	{
		FindDangleBonesAndCreateConstraints();
	}

	TBaseClass::OnPropertyPostChange( property );
}
#endif

const CSkeleton* CAnimSkeletalMultiDangleConstraint::FindMainSkeleton() const
{
	const CComponent* comp = GetParentComponent();
	if ( comp && comp->GetTransformParent() )
	{
		const CAnimatedComponent* compParent = Cast< CAnimatedComponent >( comp->GetTransformParent()->GetParent() );
		if ( compParent )
		{
			return compParent->GetSkeleton();
		}
	}

	return NULL;
}

void CAnimSkeletalMultiDangleConstraint::FindDangleBonesAndCreateConstraints()
{
	TDynArray< Int32 > dangleRootBones_skDangle;
	TDynArray< TPair< Int32, TDynArray< Int32 > > > dangleChildrenBones_skDangle;
	TDynArray< Int32 > dangleParentBones_skMain;

	const CSkeleton* skDangle = GetSkeleton();
	const CSkeleton* mainSkeleton = FindMainSkeleton();

	if ( skDangle && mainSkeleton )
	{
		const Int32 numBones_skDangle = skDangle->GetBonesNum();
		for ( Int32 i=0; i<numBones_skDangle; ++i )
		{
			const Int32 boneIndex_skDangle = i;

			const AnsiChar* boneName_skDangle = skDangle->GetBoneNameAnsi( (Uint32)boneIndex_skDangle );

			const Int32 boneIndex_skMain = mainSkeleton->FindBoneByName( boneName_skDangle );
			if ( boneIndex_skMain == -1 )
			{
				Bool alreadyAdded = false;
				{
					Int32 boneIndex = boneIndex_skDangle;
					while ( boneIndex != -1 )
					{
						const Int32 boneParentIndex = skDangle->GetParentBoneIndex( boneIndex );
						
						const Int32 fIdx = (Int32)dangleRootBones_skDangle.GetIndex( boneParentIndex );
						if ( fIdx != -1 )
						{
							alreadyAdded = true;

							ASSERT( boneParentIndex == dangleRootBones_skDangle[ fIdx ] );
							
							TDynArray< Int32 >& children = dangleChildrenBones_skDangle[ fIdx ].m_second;
							children.PushBack( boneIndex_skDangle );

							break;
						}

						boneIndex = boneParentIndex;
					}
				}

				if ( !alreadyAdded )
				{
					dangleRootBones_skDangle.PushBack( boneIndex_skDangle );
					dangleChildrenBones_skDangle.PushBack( TPair< Int32, TDynArray< Int32 > >( boneIndex_skDangle, TDynArray< Int32 >() ) );

					const Int32 boneParentIndex_skDangle = skDangle->GetParentBoneIndex( boneIndex_skDangle );
					const AnsiChar* boneParentName_skDangle = skDangle->GetBoneNameAnsi( (Uint32)boneParentIndex_skDangle );

					const Int32 boneParentIndex_skMain = mainSkeleton->FindBoneByName( boneParentName_skDangle );
					ASSERT( boneParentIndex_skMain != -1 );

					dangleParentBones_skMain.PushBack( boneParentIndex_skMain );
				}
			}
		}
	}

	ASSERT( dangleRootBones_skDangle.Size() == dangleParentBones_skMain.Size() );
	ASSERT( dangleRootBones_skDangle.Size() == dangleChildrenBones_skDangle.Size() );

	CreateConstraints( dangleParentBones_skMain, dangleRootBones_skDangle, dangleChildrenBones_skDangle );
}

void CAnimSkeletalMultiDangleConstraint::CreateConstraints( const TDynArray< Int32 >& dangleParentBones_skeletonMain, const TDynArray< Int32 >& dangleRootBones_skeletonDangle, const TDynArray< TPair< Int32, TDynArray< Int32 > > >& dangleChildrenBones_skeletonDangle )
{
	
}

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",on)
#endif
