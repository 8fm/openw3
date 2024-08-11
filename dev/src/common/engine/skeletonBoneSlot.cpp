/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "skeletonBoneSlot.h"
#include "skeletonProvider.h"
#include "node.h"
#include "component.h"

IMPLEMENT_ENGINE_CLASS( CSkeletonBoneSlot );


CSkeletonBoneSlot::CSkeletonBoneSlot( Uint32 boneIndex )
	: m_boneIndex( boneIndex )
	, m_lastBoneTransform( Matrix::IDENTITY )
{
}

Matrix CSkeletonBoneSlot::CalcSlotMatrix() const
{
	// We must be owned by component
	CComponent* owner = SafeCast< CComponent >( GetParent() );

	// Get skeleton provider from it
	const ISkeletonDataProvider* skeleton = owner->QuerySkeletonDataProvider();

	// Get bone world space matrix
	ASSERT( skeleton );
	return skeleton->GetBoneMatrixWorldSpace( m_boneIndex );
}
