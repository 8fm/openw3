/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "entityEditorWoundItem.h"
#include "entityPreviewPanel.h"
#include "../../common/engine/entityDismemberment.h"
#include "../../common/engine/renderFrame.h"


CEntityEditorWoundItem::CEntityEditorWoundItem( CEdEntityPreviewPanel* editorPreview, const CName& woundName )
	: m_preview( editorPreview )
	, m_woundName( woundName )
{

}

Bool CEntityEditorWoundItem::IsValid() const
{
	return CEntityDismemberment::FindWoundByNameRecursive( m_preview->GetEntity()->GetEntityTemplate(), m_woundName ) != nullptr;
}

void CEntityEditorWoundItem::Refresh()
{
	CEntity* ent = m_preview->GetEntity();

	const CDismembermentWound* wound = CEntityDismemberment::FindWoundByNameRecursive( ent->GetEntityTemplate(), m_woundName );
	if ( wound != nullptr )
	{
		RefreshTransform( wound->GetTransform() );
	}
}

void CEntityEditorWoundItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos ) 
{
	CEntity* ent = m_preview->GetEntity();
	CEntityDismemberment* dismember = ent->GetEntityTemplate()->FindParameter< CEntityDismemberment >();
	if ( dismember != nullptr )
	{
		if ( dismember->SetWoundTransform( m_woundName, &newPos, nullptr, nullptr ) )
		{
			GetItemContainer()->OnItemTransformChangedFromPreview( this );
		}
	}
}

void CEntityEditorWoundItem::SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) 
{
	CEntity* ent = m_preview->GetEntity();
	CEntityDismemberment* dismember = ent->GetEntityTemplate()->FindParameter< CEntityDismemberment >();
	if ( dismember != nullptr )
	{
		if ( dismember->SetWoundTransform( m_woundName, nullptr, &newRot, nullptr ) )
		{
			GetItemContainer()->OnItemTransformChangedFromPreview( this );
		}
	}
}

void CEntityEditorWoundItem::SetScaleFromPreview( const Vector& prevScale, Vector& newScale )
{
	CEntity* ent = m_preview->GetEntity();
	CEntityDismemberment* dismember = ent->GetEntityTemplate()->FindParameter< CEntityDismemberment >();
	if ( dismember != nullptr )
	{
		if ( dismember->SetWoundTransform( m_woundName, nullptr, nullptr, &newScale ) )
		{
			GetItemContainer()->OnItemTransformChangedFromPreview( this );
		}
	}
}

IPreviewItemContainer* CEntityEditorWoundItem::GetItemContainer() const
{
	return m_preview;
}

void CEntityEditorWoundItem::DrawGizmo( CRenderFrame* frame )
{
	CEntity* ent = m_preview->GetEntity();
	const CDismembermentWound* wound = CEntityDismemberment::FindWoundByNameRecursive( ent->GetEntityTemplate(), m_woundName );
	if ( wound == nullptr )
	{
		return;
	}

	Matrix woundMatrix;
	wound->GetTransform().CalcLocalToWorld( woundMatrix );

	frame->AddDebugEllipsoid( Vector::ZEROS, Vector::ONES, woundMatrix, Color::RED, false );

	const Vector o = woundMatrix.TransformPoint( Vector::ZEROS );
	const Vector dy = woundMatrix.GetAxisY();
	frame->AddDebug3DArrow( o, dy, 1.0f, 0.01f, 0.02f, 0.02f, Color::BLUE, Color::BLUE );
}
