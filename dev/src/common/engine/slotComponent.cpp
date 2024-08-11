/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "slotComponent.h"
#include "animatedComponent.h"
#include "../core/dataError.h"
#include "world.h"
#include "skeleton.h"
#include "entity.h"
#include "layer.h"
#include "utils.h"


IMPLEMENT_ENGINE_CLASS( SSlotInfo );
IMPLEMENT_ENGINE_CLASS( CSlotComponent );
IMPLEMENT_ENGINE_CLASS( CIndirectSlot );


CIndirectSlot::CIndirectSlot( Uint32 slotIndex )
: m_slotIndex( slotIndex )
{
}

Matrix CIndirectSlot::CalcSlotMatrix() const
{
	// We must be owned by slot component
	CSlotComponent* owner = SafeCast< CSlotComponent >( GetParent() );
	return owner->GetSlotWorldTransform( m_slotIndex );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CSlotComponent::OnAttached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttached( world );

	// Register slot display in behavior channel
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Behavior );
}

void CSlotComponent::OnDetached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnDetached( world );

	// Unregister slot display from behavior channel
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Behavior );
}

#ifndef NO_DATA_VALIDATION
void CSlotComponent::OnCheckDataErrors( Bool isInTemplate ) const
{
	// Pass to base class
	TBaseClass::OnCheckDataErrors( isInTemplate );

	// Slot components are deprecated !
	if ( isInTemplate )
	{
		DATA_HALT( DES_Major, CResourceObtainer::GetResource( this ), TXT("World"), TXT("Slot component are deprecated. Please remove component '%ls'"), GetName().AsChar() );
	}
}
#endif // NO_DATA_VALIDATION

void CSlotComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	TBaseClass::OnGenerateEditorFragments( frame, flags );

	// Don't spam behavior visial debug
	/*if ( flags == SHOW_Behavior )
	{
		for ( Uint32 i=0; i<m_slots.Size(); ++i )
		{
			SSlotInfo& slotInfo = m_slots[i];
			Matrix slotTransform = GetSlotWorldTransform( i );
			frame->AddDebugAxis( slotTransform.GetTranslation(), slotTransform, 0.1f, true );
			frame->AddDebugText( slotTransform.GetTranslation(), slotInfo.m_slotName.AsString(), false, Color::LIGHT_CYAN );
		}
	}*/
}

Bool CSlotComponent::ParseSkeleton( const CSkeleton* skeleton )
{
	if ( !skeleton )
	{
		return false;
	}
	if ( !GetEntity()->GetRootAnimatedComponent() || !GetEntity()->GetRootAnimatedComponent()->GetSkeleton() )
	{
		// No skeleton in entity, parse given skeleton outputing slot for every bone
		return ParseSkeletonNoRootSkeleton( skeleton );
	}
	const CSkeleton* rootSkeleton = GetEntity()->GetRootAnimatedComponent()->GetSkeleton();
	ASSERT( rootSkeleton );

	// Extract bones info from both skeletons
	TDynArray< SBoneInfo > rootSkeletonBones, slotSkeletonBones;
	ExtractBonesInfo( skeleton, slotSkeletonBones );
	ExtractBonesInfo( rootSkeleton, rootSkeletonBones );

	// Find slot candidates by looking for bones present in slot skeleton and missing in root skeleton
	TDynArray< SBoneInfo > slotBonesCandidates;
	TDynArray< SBoneInfo >::const_iterator slotBonesIter = slotSkeletonBones.Begin();
	for ( ; slotBonesIter != slotSkeletonBones.End(); ++slotBonesIter )
	{
		const SBoneInfo& slotboneInfo = *slotBonesIter;
		if ( !rootSkeletonBones.Exist( slotboneInfo ) )
		{
			slotBonesCandidates.PushBack( slotboneInfo );
		}
	}

	if ( slotBonesCandidates.Empty() )
	{
		// No slots
		return false;
	}

	// Filter slots candidates, to make sure their closest parent is present in root skeleton
	TDynArray< SBoneInfo >::const_iterator boneInfoIter = slotBonesCandidates.Begin();
	TDynArray< SBoneInfo > slotBones;
	for ( ; boneInfoIter != slotBonesCandidates.End(); ++boneInfoIter )
	{
		const SBoneInfo& boneInfo = *boneInfoIter;

		// Find parent bone index in root skeleton
		Int32 parentIndex = FindBoneIndex( rootSkeleton, boneInfo.m_parentName );
		
		if ( parentIndex != -1 )
		{
			// Qualified
			slotBones.PushBack( boneInfo );
			slotBones.Back().m_parentIndex = parentIndex;
		}
	}

	// Produce final slot entries
	m_slots.Clear();
	for ( Uint32 i=0; i<slotBones.Size(); ++i )
	{
		SBoneInfo& boneInfo = slotBones[i];

		Matrix parentTransform = skeleton->GetBoneMatrixLS( boneInfo.m_index );
		SSlotInfo newSlot;
		newSlot.Set( boneInfo.m_name, boneInfo.m_parentName, boneInfo.m_parentIndex, parentTransform.GetTranslation(), parentTransform.ToEulerAngles() );
		m_slots.PushBack( newSlot );
	}

	return false;
}

Bool CSlotComponent::ParseSkeletonNoRootSkeleton( const CSkeleton* skeleton )
{
	ASSERT( skeleton );

	// Extract bones info from skeleton
	TDynArray< SBoneInfo > slotSkeletonBones;
	ExtractBonesInfo( skeleton, slotSkeletonBones );

	// Produce slot entries
	m_slots.Clear();
	TDynArray< SBoneInfo >::const_iterator slotBoneIter = slotSkeletonBones.Begin();
	for ( ; slotBoneIter != slotSkeletonBones.End(); ++slotBoneIter )
	{
		const SBoneInfo& boneInfo = (*slotBoneIter);

		SSlotInfo newSlot;
		Matrix parentTransform = skeleton->GetBoneMatrixMS( boneInfo.m_index );
		newSlot.Set( boneInfo.m_name, CName::NONE, -1, parentTransform.GetTranslation(), parentTransform.ToEulerAngles() );
		m_slots.PushBack( newSlot );
	}

	return !m_slots.Empty();
}

const SSlotInfo* CSlotComponent::GetSlotByName( CName slotName ) const
{
	TDynArray< SSlotInfo >::const_iterator slotIter = m_slots.Begin();
	for ( ; slotIter != m_slots.End(); ++slotIter )
	{
		if ( (*slotIter).m_slotName == slotName )
		{
			return &( *slotIter );
		}
	}

	return NULL;
}

Bool CSlotComponent::ExtractBonesInfo( const CSkeleton* skeleton, TDynArray< SBoneInfo >& bones ) const
{
	if ( !skeleton )
	{
		// No skeleton
		return false;
	}

	// Extract bones
	bones.Clear();
	const Uint32 numBones = skeleton->GetBonesNum();
	for ( Uint32 i=0; i<numBones; i++ )
	{
		// Setup bone info
		SBoneInfo info;

		// Store bone index
		info.m_index = i;

		// Store bone name
		info.m_name = CName( ANSI_TO_UNICODE( skeleton->GetBoneNameAnsi(i) ) );

		// Store parent bone name
		Int32 parentBoneIndex = skeleton->GetParentBoneIndex(i);
		if ( parentBoneIndex != -1 )
		{
			info.m_parentName = CName( ANSI_TO_UNICODE( skeleton->GetBoneNameAnsi( parentBoneIndex ) ) );
			bones.PushBack( info );
		}
	}

	// Any bones extracted?
	return !bones.Empty();
}

Int32 CSlotComponent::FindBoneIndex( const CSkeleton* skeleton, CName bone ) const
{
	// Linear search
	const Uint32 numBones = skeleton->GetBonesNum();
	for ( Uint32 i=0; i<numBones; i++ )
	{
		// Store bone name
		const AnsiChar* boneName = skeleton->GetBoneNameAnsi(i);
		if ( bone == ANSI_TO_UNICODE( boneName ) )// mind the order, CName has a comparision function with AnsiChar*
		{
			// Found, return index
			return i;
		}
	}

	// Not found
	return -1;

}

void CSlotComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	// Parse source skeleton
	if ( property->GetName() == CNAME( sourceSkeleton ) )
	{
		CSkeleton* sourceSkeleton = m_sourceSkeleton.Get();
		if ( sourceSkeleton )
		{
			ParseSkeleton( sourceSkeleton );
		}
	}
}

Uint32 CSlotComponent::GetNumberOfSlots() const
{
	return m_slots.Size();
}

Matrix CSlotComponent::GetSlotTransform( Uint32 slotIndex ) const
{
	ASSERT( m_slots.Size() > slotIndex );

	Matrix outMatrix;
	const SSlotInfo& slot = m_slots[ slotIndex ];
	slot.m_relativeRotation.ToMatrix( outMatrix );
	outMatrix.SetTranslation( slot.m_relativePosition );

	return outMatrix;
}

Matrix CSlotComponent::GetSlotWorldTransform( Uint32 slotIndex ) const
{
	ASSERT( m_slots.Size() > slotIndex );
	Matrix localSlot = GetSlotTransform( slotIndex );

	if ( m_slots[ slotIndex ].m_parentSlotName )
	{
		// Parent slot defined, get it's world transform
		CAnimatedComponent* animComponent = GetEntity()->GetRootAnimatedComponent();
		if ( animComponent )
		{
			Matrix boneWorld = animComponent->GetBoneMatrixWorldSpace( m_slots[ slotIndex ].m_parentSlotIndex );
			return localSlot * boneWorld;
		}
		
	}
	
	// Parent slot transform couldn't be used, use component's localToWorld
	return localSlot * m_localToWorld;
}


ISlot* CSlotComponent::CreateSlot( const String& slotName )
{
	// Linear search by name
	for ( Uint32 i=0; i<m_slots.Size(); ++i )
	{
		const SSlotInfo& slot = m_slots[i];
		if ( slot.m_slotName == CName( slotName ) )
		{
			// Create skeleton slot
			CIndirectSlot *newSlot = new CIndirectSlot( i );
			newSlot->SetParent( this );
			return newSlot;
		}
	}

	// Slot not found
	return NULL;
}

void CSlotComponent::EnumSlotNames( TDynArray< String >& slotNames ) const
{
	for ( Uint32 i = 0; i < m_slots.Size(); ++i )
	{
		slotNames.PushBack( m_slots[i].m_slotName.AsString() );
	}
}

ISlotProvider* CSlotComponent::QuerySlotProvider()
{
	return static_cast< ISlotProvider* >( this );
}

void CSlotComponent::BaseOn( const CAnimatedComponent* animComponent )
{
	ASSERT( animComponent );
	if ( !animComponent )
	{
		return;
	}
	ParseSkeletonNoRootSkeleton( animComponent->GetSkeleton() );

#ifndef NO_COMPONENT_GRAPH
	Int32 x,y;
	animComponent->GetGraphPosition( x, y );
	SetGraphPosition( x, y );
#endif

	SetGUID( animComponent->GetGUID() );
}
