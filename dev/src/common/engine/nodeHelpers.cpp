
#include "build.h"
#include "nodeHelpers.h"
#include "skeletonBoneSlot.h"

Matrix GetNodeLocalToWorldMatrix( CNode* node )
{
	Matrix toWorld = Matrix::IDENTITY;

	if ( node->GetTransformParent() )
	{
		CSkeletonBoneSlot* bone = Cast< CSkeletonBoneSlot >( node->GetTransformParent()->GetParentSlot() );
		if ( bone )
		{
			toWorld = bone->CalcSlotMatrix();
		}
		else if ( node->GetTransformParent()->GetParent() )
		{
			toWorld = node->GetTransformParent()->GetParent()->GetLocalToWorld();
		}
	}
	else if ( CNode* parentNode = Cast< CNode >( node->GetParent() ) )
	{
		toWorld = parentNode->GetLocalToWorld();
	}

	return toWorld;
}
