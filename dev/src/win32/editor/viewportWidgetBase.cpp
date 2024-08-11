/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "undoCreate.h"
#include "undoTransform.h"

CViewportWidgetBase::CViewportWidgetBase( const String& groupName, Bool enableDuplicateOnStart )
	: m_isEnabled( false )
	, m_groupName( groupName )
	, m_duplicateOnStart( enableDuplicateOnStart )
{
}

CViewportWidgetBase::~CViewportWidgetBase()
{

}

Float CViewportWidgetBase::CalcMovement( const CRenderFrameInfo &frameInfo, const Vector &axis, Int32 dx, Int32 dy )
{
	const Float moveFactor = 0.1f;
	Vector u = frameInfo.m_camera.GetCameraRight();
	Vector v = frameInfo.m_camera.GetCameraUp();
	Float uDot = Vector::Dot3( u, axis );
	Float vDot = Vector::Dot3( v, axis );

	return ( uDot * dx - vDot * dy ) * moveFactor;
}

Matrix CViewportWidgetBase::CalcLocalToWorld() const
{
	Matrix widgetSpace;
	widgetSpace.SetIdentity();

	// Get widget space
	if ( CNodeTransformManager* transMan = GetManager()->GetRenderingPanel()->GetTransformManager() )
	{
		widgetSpace = transMan->CalculatePivotSpace( GetManager()->GetWidgetSpace() );
	}

	// Return widget space
	return widgetSpace;
}

Vector CViewportWidgetBase::CalcLocalAxis( const Vector& axis ) const
{
	return CalcLocalToWorld().TransformVector( axis );
}

void CViewportWidgetBase::EnableWidget( Bool state )
{
	m_isEnabled = state;
}

Bool CViewportWidgetBase::Activate()
{
	CEdRenderingPanel* rendPanel = GetManager()->GetRenderingPanel();
	CEdUndoManager* undoManager = rendPanel->GetUndoManager();

	if ( rendPanel->GetWorld() == nullptr )
	{
		return false;
	}

	Bool duplicate = m_duplicateOnStart && RIM_IS_KEY_DOWN( IK_LShift ) && !RIM_IS_KEY_DOWN( IK_LControl );

	if ( !rendPanel->GetSelectionManager()->ModifySelection() )
	{
		return false;
	}

	Red::TUniquePtr< CEdUndoManager::Transaction > undoTransaction;

	if ( undoManager )
	{
		String undoGroupName = duplicate ? m_groupName + TXT(" + clone") : m_groupName;
		undoTransaction.Reset( new CEdUndoManager::Transaction( *undoManager, undoGroupName ) );
	}

	if ( duplicate )
	{
		TDynArray< CEntity* > clones = rendPanel->GetSelectionManager()->DuplicateSelectedEntities();

		if ( undoManager && !clones.Empty() )
		{
			for ( CEntity* clone : clones )
			{
				CUndoCreateDestroy::CreateStep( undoManager, clone, true );
			}
			rendPanel->GetWorld()->RequestStreamingUpdate();

			CUndoCreateDestroy::FinishStep( undoManager );
		}
	}

	if ( undoManager )
	{
		TDynArray< CNode* > nodes = rendPanel->GetSelectionManager()->GetSelectedRoots();
		CUndoTransform::CreateStep( *undoManager, nodes );
	}

	rendPanel->GetTransformManager()->TransformSelectionStart();

	SendPropertyEvent( RED_NAME( transform ), RED_NAME( EditorPropertyPreChange ) );

	return true;
}

void CViewportWidgetBase::Deactivate()
{
	CEdRenderingPanel* rendPanel = GetManager()->GetRenderingPanel();
	CEdUndoManager* undoManager = rendPanel->GetUndoManager();

	if ( rendPanel->GetWorld() == nullptr )
	{
		return;
	}

	SendPropertyEvent( RED_NAME( transform ), RED_NAME( EditorPropertyPostChange ) );

	rendPanel->GetTransformManager()->TransformSelectionStop();
}

void CViewportWidgetBase::SendPropertyEvent( const CName &propertyName, const CName& eventName )
{
	CEdRenderingPanel* rendPanel = GetManager()->GetRenderingPanel();

    if ( rendPanel->GetWorld() == nullptr )
	{
		return;
	}

	TDynArray< CNode* > nodes = rendPanel->GetSelectionManager()->GetSelectedRoots();

	for ( CNode* node : nodes )
	{
		CEdPropertiesPage::SPropertyEventData eventData( nullptr, STypedObject( node ), propertyName );
		SEvents::GetInstance().DispatchEvent( eventName, CreateEventData( eventData ) );
	}
}
