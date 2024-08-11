#include "build.h"
#include "hardAttachmentBonePicker.h"
#include "../../common/engine/skeleton.h"
#include "../../common/engine/slotComponent.h"

CEdHardAttachmentBonePicker::CEdHardAttachmentBonePicker( CPropertyItem* item ) : ISelectionEditor( item )
{
}

void CEdHardAttachmentBonePicker::FillChoices()
{
	if ( const CHardAttachment* hAtt = m_propertyItem->GetParentObject(0).As< CHardAttachment >() )
	{
		if ( const CAnimatedComponent* animCmp = Cast< const CAnimatedComponent >( hAtt->GetParent() ) )
		{
			if ( const CSkeleton* skel = animCmp->GetSkeleton() )
			{
				for ( Int32 i=0; i<skel->GetBonesNum(); ++i )
				{
					m_ctrlChoice->AppendString( skel->GetBoneName(i).AsChar() );
				}
			}
		}
		else if ( const CSlotComponent* slotCmp = Cast< const CSlotComponent >( hAtt->GetParent() ) )
		{
			const TDynArray< SSlotInfo >& slots = slotCmp->GetSlots();
			const Uint32 slotSize = slots.Size();
			for ( Uint32 i=0; i<slotSize; ++i )
			{
				m_ctrlChoice->AppendString( slots[i].m_slotName.AsChar() );
			}
		}
	}
}