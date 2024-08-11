/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entityEditorSlotItem.h"

CEntityEditorSlotItem::CEntityEditorSlotItem( CEdEntityPreviewPanel* editorPreview, const CName& slotName )
	: m_preview( editorPreview )
	, m_slotName( slotName )
{

}

Bool CEntityEditorSlotItem::IsValid() const
{
	return m_preview->GetEntity()->GetEntityTemplate()->FindSlotByName( m_slotName, false ) != NULL;
}

void CEntityEditorSlotItem::Refresh()
{
	CEntity* ent = m_preview->GetEntity();

	const EntitySlot* slot = ent->GetEntityTemplate()->FindSlotByName( m_slotName, false );
	if ( slot )
	{
		Matrix mat;
		slot->CalcMatrix( ent, mat, NULL );
		RefreshTransform( mat.GetTranslationRef(), mat.ToEulerAngles() );
	}
}

void CEntityEditorSlotItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos ) 
{
	CEntity* ent = m_preview->GetEntity();

	const EntitySlot* slot = ent->GetEntityTemplate()->FindSlotByName( m_slotName, false );
	if ( slot )
	{
		ent->GetEntityTemplate()->SetSlotTransform( ent, m_slotName, &newPos, NULL );

		GetItemContainer()->OnItemTransformChangedFromPreview( this );
	}
}

void CEntityEditorSlotItem::SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) 
{
	CEntity* ent = m_preview->GetEntity();

	const EntitySlot* slot = ent->GetEntityTemplate()->FindSlotByName( m_slotName, false );
	if ( slot )
	{
		ent->GetEntityTemplate()->SetSlotTransform( ent, m_slotName, NULL, &newRot );

		GetItemContainer()->OnItemTransformChangedFromPreview( this );
	}
}

IPreviewItemContainer* CEntityEditorSlotItem::GetItemContainer() const
{
	return m_preview;
}

