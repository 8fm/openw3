/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "curveEditorTool.h"
#include "areaVertexEdit.h"
#include "undoManager.h"
#include "undoCreate.h"
#include "multiCurvePropertyEditor.h"
#include "../../common/engine/curveEntity.h"
#include "../../common/engine/curveControlPointEntity.h"
#include "../../common/engine/curveTangentControlPointEntity.h"
#include "../../common/engine/clipMap.h"
#include "../../common/engine/hitProxyMap.h"
#include "../../common/engine/hitProxyObject.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/component.h"
#include "../../common/engine/pathComponent.h"

IMPLEMENT_ENGINE_CLASS( CUndoCurveControlPointsAddOrDelete );

CUndoCurveControlPointsAddOrDelete::CUndoCurveControlPointsAddOrDelete( CEdRenderingPanel* panel, Bool isAdd )
	: IUndoStep ( *panel->GetUndoManager() )
	, m_panel( panel )
	, m_isAdd( isAdd )
{}

String CUndoCurveControlPointsAddOrDelete::GetName()
{
	return m_isAdd ? TXT("Add control point(s)") : TXT("Delete control point(s)");
}

void CUndoCurveControlPointsAddOrDelete::CreateStep( CEdRenderingPanel* panel, CCurveControlPointEntity* controlPointEntity, Bool add )
{
	CEdUndoManager* undoManager = panel->GetUndoManager();
	if ( !undoManager )
	{
		return;
	}

	CUndoCurveControlPointsAddOrDelete* stepToAdd = Cast<CUndoCurveControlPointsAddOrDelete>( undoManager->GetStepToAdd() );
	if ( !stepToAdd )
	{
		undoManager->SetStepToAdd( stepToAdd = new CUndoCurveControlPointsAddOrDelete( panel, add ) );
		stepToAdd->m_curve = controlPointEntity->GetCurveEntity()->GetCurve();
	}

	SControlPoint* controlPoint = new( stepToAdd->m_controlPoints ) SControlPoint();
	controlPoint->m_index = controlPointEntity->GetControlPointIndex();
	controlPointEntity->GetCurveEntity()->GetCurve()->GetControlPointTransform( controlPoint->m_index, controlPoint->m_transform );
}

void CUndoCurveControlPointsAddOrDelete::FinishStep( CEdRenderingPanel* panel )
{
	CEdUndoManager* undoManager = panel->GetUndoManager();
	if ( !undoManager )
	{
		return;
	}

	CUndoCurveControlPointsAddOrDelete* stepToAdd = Cast<CUndoCurveControlPointsAddOrDelete>( undoManager->GetStepToAdd() );
	if ( !stepToAdd )
	{
		return;
	}

	stepToAdd->PushStep();
}

void CUndoCurveControlPointsAddOrDelete::DoUndo()
{
	CCurveEntity* curveEntity = CCurveEntity::CreateEditor( m_panel->GetWorld(), m_curve, true );

	for ( Int32 i = ((Int32) m_controlPoints.Size() - 1); i >= 0; --i )
	{
		ProcessControlPoint( curveEntity, m_controlPoints[ i ], true );
	}
}

void CUndoCurveControlPointsAddOrDelete::DoRedo()
{
	CCurveEntity* curveEntity = CCurveEntity::CreateEditor( m_panel->GetWorld(), m_curve, true );

	for ( Int32 i = ((Int32) m_controlPoints.Size() - 1); i >= 0; --i )
	{
		ProcessControlPoint( curveEntity, m_controlPoints[ i ], false );
	}
}

void CUndoCurveControlPointsAddOrDelete::ProcessControlPoint( CCurveEntity* curveEntity, const SControlPoint& controlPoint, Bool undo )
{
	if ( m_isAdd == undo )
	{
		curveEntity->DeleteControlPoint( curveEntity->GetControlPoints()[ controlPoint.m_index ] );
	}
	else
	{
		curveEntity->AddControlPoint( controlPoint.m_index, controlPoint.m_transform );
	}
}

// CCurveEditorTool

IMPLEMENT_ENGINE_CLASS( CCurveEditorTool );

CCurveEditorTool::CCurveEditorTool()
	: m_panel( NULL )
	, m_hoveredControlPoint( NULL )
{
}

String CCurveEditorTool::GetCaption() const
{
	return TXT("Curve editor");
}

Bool CCurveEditorTool::StartPersistent( CEdRenderingPanel* panel )
{
	m_panel = panel;
	return true;
}

void CCurveEditorTool::End()
{
	SCurveSelectionInfo selection;
	if ( !CCurveEditorPanelHelper::GetSelectionInfo( m_panel, selection ) )
	{
		return;
	}

	for ( auto it = selection.m_curves.Begin(); it != selection.m_curves.End(); ++it )
	{
		(*it)->EnableEditMode( false );
	}
}

Bool HACK_justDeletedControlPoints = false;

Bool CCurveEditorTool::OnCopy()
{
	// Don't allow copying the curve itself

	SCurveSelectionInfo selection;
	if ( !CCurveEditorPanelHelper::GetSelectionInfo( m_panel, selection ) || !selection.m_singleCurve )
	{
		return false;
	}

	// Let curve parent handle operation if selected

	CNode* curveParent = selection.m_singleCurve->GetCurve()->GetParent();
	if ( curveParent && curveParent->IsSelected() )
	{
		return false;
	}

	return true;
}

Bool CCurveEditorTool::OnCut()
{
	// Don't allow cutting the curve itself

	SCurveSelectionInfo selection;
	if ( !CCurveEditorPanelHelper::GetSelectionInfo( m_panel, selection ) || !selection.m_singleCurve )
	{
		return false;
	}

	// Let curve parent handle operation if selected

	CNode* curveParent = selection.m_singleCurve->GetCurve()->GetParent();
	if ( curveParent && curveParent->IsSelected() )
	{
		return false;
	}

	return true;
}

Bool CCurveEditorTool::OnDelete()
{
	SCurveSelectionInfo selection;
	if ( !CCurveEditorPanelHelper::GetSelectionInfo( m_panel, selection ) || !selection.m_singleCurve )
	{
		return false;
	}

	// Let curve parent handle deletion if selected

	CNode* curveParent = selection.m_singleCurve->GetCurve()->GetParent();
	if ( curveParent && curveParent->IsSelected() )
	{
		return false;
	}

	// Delete the whole entity if curve owner is path

	if ( selection.m_curves.Size() == 1 )
	{
		CCurveEntity* curveEntity = selection.m_curves[0];
		SMultiCurve* curve = curveEntity->GetCurve();
		CNode* curveParent = curve->GetParent();

		CEntity* parentEntity = NULL;
		if ( CPathComponent* pathComponent = Cast<CPathComponent>( curveParent ) )
		{
			parentEntity = pathComponent->GetEntity();
		}
		else
		{
			return false;
		}

		// Ask user
		if ( !YesNo( TXT("Sure to delete path?") ) )
		{
			return false;
		}

		// Delete entities
		m_panel->GetSelectionManager()->DeselectAll();
		if ( parentEntity->MarkModified() )
		{
			CUndoCreateDestroy::CreateStep( wxTheFrame->GetUndoManager(), parentEntity, false );
		}
		CUndoCreateDestroy::FinishStep( wxTheFrame->GetUndoManager() );
		return true;
	}

	// Delete control points

	if ( HACK_justDeletedControlPoints ) // Don't handle second OnDelete to prevent undesired deletion of post-delete auto-selected control point
	{
		HACK_justDeletedControlPoints = false;
		return true;
	}

	m_panel->GetSelectionManager()->DeselectAll();

	Bool anyControlPointDeleted = false;
	for ( auto it = selection.m_controlPoints.Begin(); it != selection.m_controlPoints.End(); ++it )
	{
		CCurveControlPointEntity* controlPoint = (*it);
		CCurveEntity* curveEntity = controlPoint->GetCurveEntity();
		if ( curveEntity->CanDeleteControlPoint() )
		{
			anyControlPointDeleted = true;
			CUndoCurveControlPointsAddOrDelete::CreateStep( m_panel, controlPoint, false );
			curveEntity->DeleteControlPoint( controlPoint );
		}
	}

	if ( anyControlPointDeleted )
	{
		CUndoCurveControlPointsAddOrDelete::FinishStep( m_panel );
	}

	HACK_justDeletedControlPoints = true;
	return true;
}

Bool CCurveEditorTool::OnViewportTrack( const CMousePacket& packet )
{
	CCurveEntity::SetHoverControlPoint( NULL );

	if ( packet.m_x < 0 || packet.m_y < 0 )
	{
		return false;
	}

	SCurveSelectionInfo selection;
	if ( !CCurveEditorPanelHelper::GetSelectionInfo( m_panel, selection ) )
	{
		return false;
	}

	CHitProxyMap map;
	CHitProxyObject* object = m_panel->GetHitProxyAtPoint( map, packet.m_x, packet.m_y );
	if ( !object )
	{
		return false;
	}

	CObject* parent = object->GetHitObject()->GetParent();

	if ( CCurveControlPointEntity* controlPoint = Cast< CCurveControlPointEntity >( parent ) )
	{
		SCurveSelectionInfo selection;
		CCurveEditorPanelHelper::GetSelectionInfo( m_panel, selection );
		if ( selection.m_singleCurve == controlPoint->GetCurveEntity() )
		{
			CCurveEntity::SetHoverControlPoint( controlPoint );
			return false;
		}
	}
	else if ( CCurveTangentControlPointEntity* tangentControlPoint = Cast< CCurveTangentControlPointEntity >( parent ) )
	{
		SCurveSelectionInfo selection;
		CCurveEditorPanelHelper::GetSelectionInfo( m_panel, selection );
		if ( selection.m_singleCurve == tangentControlPoint->GetControlPointEntity()->GetCurveEntity() )
		{
			CCurveEntity::SetHoverControlPoint( tangentControlPoint );
			return false;
		}
	}

	return false;
}

Bool CCurveEditorTool::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	// Check if we have single control point selected

	CWorld* world = m_panel->GetWorld();
	if ( !world )
	{
		return false;
	}

	SCurveSelectionInfo selection;
	if ( !CCurveEditorPanelHelper::GetSelectionInfo( m_panel, selection ) || !selection.m_singleControlPoint )
	{
		return false;
	}

	// Make sure it's a press key action

	if ( action != IACT_Press )
	{
		return false;
	}

	// Change selection to previous control point

	if ( key == IK_Comma )
	{
		selection.m_singleCurve->SelectPreviousControlPoint( selection.m_singleControlPoint );
		return true;
	}

	// Change selection to next control point

	if ( key == IK_Period )
	{
		selection.m_singleCurve->SelectNextControlPoint( selection.m_singleControlPoint );
		return true;
	}

	// Show / hide ease parameters

	if ( key == IK_Slash )
	{
		CCurveKeyDialog::ShowInstance( !CCurveKeyDialog::IsShown(), m_panel );
		return true;
	}

	return false;
}

Bool CCurveEditorTool::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
	if ( button == 0 && state )
	{
		return HandleActionClick( x, y );
	}
	return false;
}

Bool CCurveEditorTool::HandleActionClick( Int32 x, Int32 y )
{
	CWorld* world = m_panel->GetWorld();
	if ( !world )
	{
		return false;
	}

	SCurveSelectionInfo selection;
	if ( !CCurveEditorPanelHelper::GetSelectionInfo( m_panel, selection ) )
	{
		return false;
	}

	// Create new control point at the end of the curve

	if ( RIM_IS_KEY_DOWN( IK_Ctrl ) && !RIM_IS_KEY_DOWN( IK_Shift ))
	{
		if ( selection.m_singleCurve )
		{
			Vector clickPos, clickNormal;
			if ( GetWorldClickPos( x, y, clickPos, clickNormal ) )
			{
				const Vector desiredPoint = clickPos + clickNormal * 0.05f;
				CCurveControlPointEntity* newControlPoint = selection.m_singleCurve->AppendControlPointWorld( desiredPoint );
				CUndoCurveControlPointsAddOrDelete::CreateStep( m_panel, newControlPoint, true );
				CUndoCurveControlPointsAddOrDelete::FinishStep( m_panel );
				return true;
			}
		}
	}

	// Create / remove control point

	else if ( RIM_IS_KEY_DOWN( IK_Alt ) )
	{
		if ( selection.m_singleCurve )
		{
			CHitProxyMap map;
			CHitProxyObject* object = m_panel->GetHitProxyAtPoint( map, x, y );
			CCurveControlPointEntity* hoveredControlPoint = object ? Cast< CCurveControlPointEntity >( object->GetHitObject()->GetParent() ) : NULL;

			if ( hoveredControlPoint )
			{
				CUndoCurveControlPointsAddOrDelete::CreateStep( m_panel, hoveredControlPoint, false );
				CUndoCurveControlPointsAddOrDelete::FinishStep( m_panel );
				hoveredControlPoint->GetCurveEntity()->DeleteControlPoint( hoveredControlPoint );
				return true;
			}
			else
			{
				Vector clickPos, clickNormal;

				// Try to intersect with the world

				if ( GetWorldClickPos( x, y, clickPos, clickNormal ) )
				{
					const Vector desiredPoint = clickPos + clickNormal * 0.05f;
					CCurveControlPointEntity* newControlPoint = selection.m_singleCurve->AddControlPointWorld( desiredPoint );
					CUndoCurveControlPointsAddOrDelete::CreateStep( m_panel, newControlPoint, true );
					CUndoCurveControlPointsAddOrDelete::FinishStep( m_panel );
					return true;
				}
			}
		}
	}

	// Snap control points to terrain

	else if ( RIM_IS_KEY_DOWN( IK_X ) )
	{
		if ( const CClipMap* terrain = world->GetTerrain() )
		{
			// Collect control points to move

			TDynArray< CCurveControlPointEntity* > controlPointsToMove; 

			for ( auto it = selection.m_controlPoints.Begin(); it != selection.m_controlPoints.End(); ++it )
			{
				controlPointsToMove.PushBackUnique( *it );
			}

			for ( auto it = selection.m_curves.Begin(); it != selection.m_curves.End(); ++it )
			{
				CCurveEntity* curveEntity = *it;
				for ( auto it2 = curveEntity->GetControlPoints().Begin(); it2 != curveEntity->GetControlPoints().End(); ++it2 )
				{
					controlPointsToMove.PushBackUnique( *it2 );
				}
			}

			// Move control points

			for ( auto it = controlPointsToMove.Begin(); it != controlPointsToMove.End(); ++it )
			{
				CCurveControlPointEntity* controlPoint = *it;

				Vector pos = controlPoint->GetWorldPosition();
				terrain->GetHeightForWorldPosition( pos, pos.Z );
				controlPoint->GetCurveEntity()->SetControlPointPosition( controlPoint, pos );
			}

			if ( !controlPointsToMove.Empty() )
			{
				return true;
			}
		}
	}

	return false;
}

Bool CCurveEditorTool::GetWorldClickPos( Int32 clickX, Int32 clickY, Vector& clickPos, Vector& clickNormal )
{
	CWorld* world = m_panel->GetWorld();
	if ( !world )
	{
		return false;
	}

	// Try to intersect with the world

	if ( world->ConvertScreenToWorldCoordinates( m_panel->GetViewport(), clickX, clickY, clickPos, &clickNormal ) )
	{
		return true;
	}

	// Otherwise try to intersect with plane at height of 0.0f

	Vector origin, dir;
	m_panel->GetViewport()->CalcRay( clickX, clickY, origin, dir );
	dir *= 10000.0f;

	const Float minY = Min( origin.Z, origin.Z + dir.Z );
	const Float maxY = Max( origin.Z, origin.Z + dir.Z );

	const Float height = 0.0f;

	if ( minY <= height && height <= maxY )
	{
		const Float scale = -origin.Z / dir.Z;
		clickPos = origin + dir * scale;
		clickPos.W = 1.0f;

		clickNormal = Vector(0.0f, 0.0f, 1.0f, 0.0f);

		return true;
	}

	return false;
}

Bool CCurveEditorTool::HandleContextMenu( Int32 x, Int32 y, const Vector& collision )
{
	CWorld* world = m_panel->GetWorld();
	if ( !world )
	{

		return false;
	}

	wxMenuWithSeparators menu;
	m_curveEditorHelper.HandleContextMenu( m_panel, &menu );

	if ( menu.GetMenuItems().empty() )
	{
		return false;
	}

	m_panel->PopupMenu( &menu );
	return true;
}
