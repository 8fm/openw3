/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "editorExternalResources.h"
#include "curveEditorTool.h"
#include "sceneExplorer.h"

#include "../../common/engine/curveEntity.h"
#include "../../common/engine/curveControlPointEntity.h"
#include "../../common/engine/curveTangentControlPointEntity.h"
#include "../../common/engine/pathComponent.h"
#include "../../common/core/feedback.h"

CCurveEditorPanelHelper::CCurveEditorPanelHelper()
	: m_panel( nullptr )
	, m_dlg( new CEdPropertyDialog( wxTheFrame, TXT("Entities spawner") ) )
{
	SEvents::GetInstance().RegisterListener( CNAME( Detached ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SelectionChanged ), this );
}

CCurveEditorPanelHelper::~CCurveEditorPanelHelper()
{
	SEvents::GetInstance().UnregisterListener( this );
}


void CCurveEditorPanelHelper::HandleContextMenu( CEdRenderingPanel* panel, wxMenuWithSeparators* rootMenu )
{
	m_panel = panel;

	SCurveSelectionInfo info;
	GetSelectionInfo( m_panel, info );
	CCurveEntity* curveEntity = info.m_singleCurve;
	if ( !curveEntity )
	{
		return;
	}

	SMultiCurve* curve = curveEntity->GetCurve();

	m_rootMenu = rootMenu;
	m_currentItemId = 0;

	// Adding control points

	CCurveControlPointEntity* curveControlPoint = info.m_singleControlPoint;
	if ( curveControlPoint )
	{
		curveEntity = curveControlPoint->GetCurveEntity();

		AddCommand( rootMenu, TXT("Add control point after"), wxCommandEventHandler( CCurveEditorPanelHelper::OnAddControlPointAfter ) );
		AddCommand( rootMenu, TXT("Add control point before"), wxCommandEventHandler( CCurveEditorPanelHelper::OnAddControlPointBefore ) );
	}

	AddCommand( rootMenu, TXT("Select all control points"), wxCommandEventHandler( CCurveEditorPanelHelper::OnSelectAllCurveControlPoints ) );
	AddCommand( rootMenu, TXT("Hide curve"), wxCommandEventHandler( CCurveEditorPanelHelper::OnHideCurve ) );
	rootMenu->AppendSeparator();

	// Time recalculation

	if ( curve->HasPosition() && info.m_enableTimeRecalculation )
	{
		const Bool enableManualTimeRecalculation = !curveEntity->IsAutomaticTimeRecalculationEnabled() && !curveEntity->IsAutomaticTimeByDistanceRecalculationEnabled();

		// Automatic time by distance recalculation

		AddCommand( rootMenu, curveEntity->IsAutomaticTimeByDistanceRecalculationEnabled() ? TXT("Disable automatic time by distance recalculation") : TXT("Enable automatic time by distance recalculation"), wxCommandEventHandler( CCurveEditorPanelHelper::OnToggleAutomaticCurveTimeByDistanceCalculation ) );

		if ( enableManualTimeRecalculation )
		{
			AddCommand( rootMenu, TXT("Recalculate time by distance"), wxCommandEventHandler( CCurveEditorPanelHelper::OnRecalculateCurveTimeByDistance ) );
			AddCommand( rootMenu, TXT("Recalculate time by distance for selected control points"), wxCommandEventHandler( CCurveEditorPanelHelper::OnRecalculateCurveTimeByDistance_SelectedControlPointsOnly ) );
		}

		// Automatic time by index recalculation

		AddCommand( rootMenu, curveEntity->IsAutomaticTimeRecalculationEnabled() ? TXT("Disable automatic time recalculation by control point indices") : TXT("Enable automatic time recalculation by control point indices"), wxCommandEventHandler( CCurveEditorPanelHelper::OnToggleAutomaticCurveTimeCalculation ) );

		if ( enableManualTimeRecalculation )
		{
			AddCommand( rootMenu, TXT("Recalculate time by control point indices"), wxCommandEventHandler( CCurveEditorPanelHelper::OnRecalculateCurveTimeByIndices ) );
		}
	}

	// Rotation recalculation

	if ( curve->HasPosition() && curve->HasRotation() && info.m_enableRotationRecalculation )
	{
		AddCommand( rootMenu, curveEntity->IsAutomaticRotationFromDirectionRecalculationEnabled() ? TXT("Disable automatic rotation from direction recalculation") : TXT("Enable automatic rotation from direction recalculation"), wxCommandEventHandler( CCurveEditorPanelHelper::OnToggleAutomaticCurveRotationFromDirectionCalculation ) );

		if ( !curveEntity->IsAutomaticRotationFromDirectionRecalculationEnabled() )
		{
			AddCommand( rootMenu, TXT("Recalculate rotation from curve direction"), wxCommandEventHandler( CCurveEditorPanelHelper::OnRecalculateCurveRotationFromDirection ) );
		}
	}

	AddCommand( rootMenu, curveEntity->IsCurveLoopingEnabled() ? TXT("Disable looping") : TXT("Enable looping"), wxCommandEventHandler( CCurveEditorPanelHelper::OnToggleCurveLooping ) );

	if ( info.m_enableEaseInOutOptions )
	{
		AddCommand( rootMenu, curve->HasEaseParams() ? TXT("Disable ease in/out parameters") : TXT("Enable ease in/out parameters"), wxCommandEventHandler( CCurveEditorPanelHelper::OnToggleEaseParams ) );
	}

	rootMenu->AppendSeparator();

	// Position interpolation

	const Bool addPositionInterpolationModes = curve->HasPosition() && info.m_enablePositionInterpolationModes;
	if ( addPositionInterpolationModes )
	{
		wxMenu* positionMenu = new wxMenu();
		{
			// Interpolation mode

			wxMenu* interpolationModeMenu = new wxMenu();

			if ( curveEntity->GetPositionInterpolationMode() != ECurveInterpolationMode_Linear )
			{
				AddCommand( interpolationModeMenu, TXT("Linear"), wxCommandEventHandler( CCurveEditorPanelHelper::OnEnablePositionLinearInterpolationMode ) );
			}
			if ( curveEntity->GetPositionInterpolationMode() != ECurveInterpolationMode_Automatic )
			{
				AddCommand( interpolationModeMenu, TXT("Automatic"), wxCommandEventHandler( CCurveEditorPanelHelper::OnEnablePositionAutomaticInterpolationMode ) );
			}
			if ( curveEntity->GetPositionInterpolationMode() != ECurveInterpolationMode_Manual )
			{
				AddCommand( interpolationModeMenu, TXT("Manual"), wxCommandEventHandler( CCurveEditorPanelHelper::OnEnablePositionManualInterpolationMode ) );
			}

			positionMenu->Append( wxID_ANY, TXT("Interpolation Mode"), interpolationModeMenu );

			// Manual mode

			if ( curveEntity->GetPositionInterpolationMode() == ECurveInterpolationMode_Manual )
			{
				wxMenu* manualModeMenu = new wxMenu();

				if ( curveEntity->GetPositionManualMode() != ECurveManualMode_Bezier )
				{
					AddCommand( manualModeMenu, TXT("Bezier"), wxCommandEventHandler( CCurveEditorPanelHelper::OnEnableBezierManualMode ) );
				}
				if ( curveEntity->GetPositionManualMode() != ECurveManualMode_BezierSymmetric )
				{
					AddCommand( manualModeMenu, TXT("Bezier Symmetric"), wxCommandEventHandler( CCurveEditorPanelHelper::OnEnableBezierSymmetricManualMode ) );
				}
				if ( curveEntity->GetPositionManualMode() != ECurveManualMode_BezierSymmetricDirection )
				{
					AddCommand( manualModeMenu, TXT("Bezier Symmetric Direction"), wxCommandEventHandler( CCurveEditorPanelHelper::OnEnableBezierSymmetricDirectionManualMode ) );
				}

				positionMenu->Append( wxID_ANY, TXT("Interpolation Editing Mode"), manualModeMenu );

				switch ( curveEntity->GetPositionManualMode() )
				{
				case ECurveManualMode_Bezier:
				case ECurveManualMode_BezierSymmetric:
				case ECurveManualMode_BezierSymmetricDirection:
					AddCommand( positionMenu, TXT("Recalculate Bezier Tangents"), wxCommandEventHandler( CCurveEditorPanelHelper::OnRecalculateBezierTangents ) );
					break;
				}
			}

			rootMenu->Append( wxID_ANY, TXT("Position"), positionMenu );
		}
	}

	// Rotation interpolation

	const Bool addRotationInterpolationModes = curve->HasRotation() && info.m_enableRotationInterpolationModes;
	if ( addRotationInterpolationModes )
	{
		wxMenu* rotationMenu = new wxMenu();
		{
			// Interpolation mode

			wxMenu* interpolationModeMenu = new wxMenu();

			if ( curveEntity->GetRotationInterpolationMode() != ECurveInterpolationMode_Linear )
			{
				AddCommand( interpolationModeMenu, TXT("Linear"), wxCommandEventHandler( CCurveEditorPanelHelper::OnEnableRotationLinearInterpolationMode ) );
			}
			if ( curveEntity->GetRotationInterpolationMode() != ECurveInterpolationMode_Automatic )
			{
				AddCommand( interpolationModeMenu, TXT("Automatic"), wxCommandEventHandler( CCurveEditorPanelHelper::OnEnableRotationAutomaticInterpolationMode ) );
			}

			rotationMenu->Append( wxID_ANY, TXT("Interpolation Mode"), interpolationModeMenu );

			rootMenu->Append( wxID_ANY, TXT("Rotation"), rotationMenu );
		}
	}

	if ( addPositionInterpolationModes || addRotationInterpolationModes )
	{
		rootMenu->AppendSeparator();
	}

	// Visualization

	wxMenu* visualizationMenu = new wxMenu();

	if ( curve->HasPosition() )
	{
		AddCommand( visualizationMenu, curveEntity->IsPositionRenderingEnabled() ? TXT("Disable position rendering") : TXT("Enable position rendering"), wxCommandEventHandler( CCurveEditorPanelHelper::OnToggleCurvePositionRendering ) );
		//AddCommand( visualizationMenu, curveEntity->IsTimeVisualizationEnabled() ? TXT("Disable time visualization") : TXT("Enable time visualization"), wxCommandEventHandler( CCurveEditorPanelHelper::OnToggleCurveTimeVisualization ) );
		AddCommand( visualizationMenu, curveEntity->IsDebugObjectCurveRunEnabled() ? TXT("Disable curve run") : TXT("Enable curve run"), wxCommandEventHandler( CCurveEditorPanelHelper::OnToggleDebugCurveRun ) );
	}
	if ( curve->HasRotation() )
	{
		AddCommand( visualizationMenu, curveEntity->IsRotationRenderingEnabled() ? TXT("Disable rotation rendering") : TXT("Enable rotation rendering"), wxCommandEventHandler( CCurveEditorPanelHelper::OnToggleCurveRotationRendering ) );
	}
	AddCommand( visualizationMenu, curveEntity->GetDrawOnTop() ? TXT("Disable draw on top") : TXT("Enable draw on top"), wxCommandEventHandler( CCurveEditorPanelHelper::OnToggleDrawOnTop ) );

	rootMenu->Append( wxID_ANY, TXT("Visualization"), visualizationMenu );

	// Activate layer option

	if ( curveEntity && info.m_enableParentOptions )
	{
		SMultiCurve* curve = curveEntity->GetCurve();
		CNode* curveParent = curve->GetParent();
		if ( curveParent )
		{
			m_rootMenu->BeginGroup();
			AddCommand( m_rootMenu, String::Printf( TXT("Activate: %s"), curveParent->GetLayer()->GetFriendlyName().AsChar() ).AsChar(), wxCommandEventHandler( CCurveEditorPanelHelper::OnActivateEntityLayer ) );
			AddCommand( m_rootMenu, String::Printf( TXT("Select curve owner: %s"), curveParent->GetFriendlyName().AsChar() ).AsChar(), wxCommandEventHandler( CCurveEditorPanelHelper::OnSelectCurveOwner ) );
		}
	}

	rootMenu->AppendSeparator();

	if ( curveEntity )
	{
		m_rootMenu->BeginGroup();
		AddCommand( m_rootMenu, String::Printf( TXT("Spawn entities")).AsChar(), wxCommandEventHandler( CCurveEditorPanelHelper::OnSpawnEntitiesTool ) );
	}
}

void CCurveEditorPanelHelper::OnActivateEntityLayer( wxCommandEvent& event )
{
	CCurveEntity* curveEntity = GetSelectedCurveEntity();
	if ( !curveEntity )
	{
		return;
	}

	SMultiCurve* curve = curveEntity->GetCurve();
	CNode* curveParent = curve->GetParent();
	if ( !curveParent )
	{
		return;
	}

	CEdSceneExplorer* sceneExplorer = m_panel->GetSceneExplorer();
	if ( !sceneExplorer )
	{
		return;
	}

	// Change selection to curve parent
	m_panel->GetSelectionManager()->DeselectAll();
	m_panel->GetSelectionManager()->Select( curveParent );

	// Change active layer
	sceneExplorer->ChangeActiveLayer( curveParent->GetLayer()->GetLayerInfo() );
	sceneExplorer->UpdateSelected();
}

void CCurveEditorPanelHelper::AddCommand( wxMenu* menu, const wxString& caption, wxObjectEventFunction func )
{
	menu->Append( m_currentItemId, caption, wxEmptyString, false );
	m_rootMenu->Connect( m_currentItemId, wxEVT_COMMAND_MENU_SELECTED, func, NULL, this );
	m_currentItemId++;
}

Bool CCurveEditorPanelHelper::GetSelectionInfo( CEdRenderingPanel* panel, SCurveSelectionInfo& info )
{
	TDynArray< CNode* > selection;
	CWorld* world = panel->GetWorld();
	if ( !world )
	{
		return false;
	}
	panel->GetSelectionManager()->GetSelectedNodes( selection );

	info.m_singleCurve = NULL;
	Bool processedAnyCurve = false;
	info.m_singleControlPoint = NULL;
	Bool processedAnyControlPoint = false;

	for ( auto it = selection.Begin(); it != selection.End(); ++it )
	{
		CCurveEntity* currentCurve = NULL;
		CCurveControlPointEntity* currentControlPoint = NULL;

		if ( CCurveEntity* curve = Cast<CCurveEntity>( *it ) )
		{
			currentCurve = curve;
			info.m_curves.PushBackUnique( currentCurve );
		}
		else if ( CCurveControlPointEntity* controlPoint = Cast<CCurveControlPointEntity>( *it ) )
		{
			currentCurve = controlPoint->GetCurveEntity();
			currentControlPoint = controlPoint;
			info.m_controlPoints.PushBack( currentControlPoint );
		}
		else if ( CCurveTangentControlPointEntity* tangentControlPoint = Cast<CCurveTangentControlPointEntity>( *it ) )
		{
			currentCurve = tangentControlPoint->GetCurveEntity();
			currentControlPoint = tangentControlPoint->GetControlPointEntity();
			info.m_tangentControlPoints.PushBack( tangentControlPoint );
		}
		else if ( CPathComponent* pathComponent = Cast<CPathComponent>( *it ) )
		{
			currentCurve = CCurveEntity::FindCurveEditor( panel->GetWorld(), &pathComponent->GetCurve() );
			info.m_curves.PushBackUnique( currentCurve );
		}

		if ( currentCurve )
		{
			if ( !processedAnyCurve )
			{
				info.m_singleCurve = currentCurve;
				processedAnyCurve = true;
			}
			else if ( info.m_singleCurve != currentCurve )
			{
				info.m_singleCurve = NULL;
			}
		}

		if ( currentControlPoint )
		{
			if ( !processedAnyControlPoint )
			{
				info.m_singleControlPoint = currentControlPoint;
				processedAnyControlPoint = true;
			}
			else if ( info.m_singleControlPoint != currentControlPoint )
			{
				info.m_singleControlPoint = NULL;
			}
		}
	}

	if ( info.m_curves.Empty() && info.m_controlPoints.Empty() && info.m_tangentControlPoints.Empty() )
	{
		return false;
	}

	// Set up feature support options based on parent node / use case

	if ( info.m_singleCurve )
	{
		if ( CNode* parent = info.m_singleCurve->GetCurve()->GetParent() )
		{
			if ( parent->IsA< CPathComponent >() )
			{
				info.m_enablePositionInterpolationModes = false;
				info.m_enableRotationInterpolationModes = false;
				info.m_enableEaseInOutOptions = false;
				info.m_enableRotationRecalculation = false;
				info.m_enableTimeRecalculation = false;
			}
			else if ( !parent->IsA< CGameplayEntity >() )
			{
				info.m_enableParentOptions = false;
			}
		}
	}

	return true;
}

CCurveControlPointEntity* CCurveEditorPanelHelper::GetSelectedCurveControlPointEntity()
{
	SCurveSelectionInfo info;
	GetSelectionInfo( m_panel, info );
	return info.m_singleControlPoint;
}

CCurveEntity* CCurveEditorPanelHelper::GetSelectedCurveEntity()
{
	SCurveSelectionInfo info;
	if ( m_panel )
	{
		GetSelectionInfo( m_panel, info );
	}
	return info.m_singleCurve;
}

void CCurveEditorPanelHelper::OnAddControlPointAfter( wxCommandEvent& event )
{
	if ( CCurveControlPointEntity* curveControlPointEntity = GetSelectedCurveControlPointEntity() )
	{
		CCurveControlPointEntity* newControlPoint = curveControlPointEntity->GetCurveEntity()->AddControlPointAfter( curveControlPointEntity );
		CUndoCurveControlPointsAddOrDelete::CreateStep( m_panel, newControlPoint, true );
		CUndoCurveControlPointsAddOrDelete::FinishStep( m_panel );
	}
}

void CCurveEditorPanelHelper::OnAddControlPointBefore( wxCommandEvent& event )
{
	if ( CCurveControlPointEntity* curveControlPointEntity = GetSelectedCurveControlPointEntity() )
	{
		CCurveControlPointEntity* newControlPoint = curveControlPointEntity->GetCurveEntity()->AddControlPointBefore( curveControlPointEntity );
		CUndoCurveControlPointsAddOrDelete::CreateStep( m_panel, newControlPoint, true );
		CUndoCurveControlPointsAddOrDelete::FinishStep( m_panel );
	}
}

void CCurveEditorPanelHelper::OnToggleCurvePositionRendering( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->EnablePositionRendering( !curveEntity->IsPositionRenderingEnabled() );
	}
}

void CCurveEditorPanelHelper::OnToggleCurveRotationRendering( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->EnableRotationRendering( !curveEntity->IsRotationRenderingEnabled() );
	}
}

void CCurveEditorPanelHelper::OnToggleDebugCurveRun( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->EnableDebugObjectCurveRun( !curveEntity->IsDebugObjectCurveRunEnabled() );
	}
}

void CCurveEditorPanelHelper::OnHideCurve( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->Destroy();
	}
}

void CCurveEditorPanelHelper::OnToggleCurveTimeVisualization( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->EnableTimeVisualization( !curveEntity->IsTimeVisualizationEnabled() );
	}
}

void CCurveEditorPanelHelper::OnToggleAutomaticCurveTimeByDistanceCalculation( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->EnableAutomaticTimeByDistanceRecalculation( !curveEntity->IsAutomaticTimeByDistanceRecalculationEnabled() );
	}
}

void CCurveEditorPanelHelper::OnToggleAutomaticCurveTimeCalculation( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->EnableAutomaticTimeRecalculation( !curveEntity->IsAutomaticTimeRecalculationEnabled() );
	}
}

void CCurveEditorPanelHelper::OnToggleAutomaticCurveRotationFromDirectionCalculation( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->EnableAutomaticRotationFromDirectionRecalculation( !curveEntity->IsAutomaticRotationFromDirectionRecalculationEnabled() );
	}
}

void CCurveEditorPanelHelper::OnToggleCurveLooping( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->SetCurveLooping( !curveEntity->IsCurveLoopingEnabled() );
	}
}

void CCurveEditorPanelHelper::OnSelectAllCurveControlPoints( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->SelectAllControlPoints();
	}
}

void CCurveEditorPanelHelper::OnRecalculateCurveTimeByDistance( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->RecalculateTimeByDistance();
	}
}

void CCurveEditorPanelHelper::OnRecalculateCurveTimeByDistance_SelectedControlPointsOnly( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->RecalculateTimeByDistance( true );
	}
}

void CCurveEditorPanelHelper::OnRecalculateCurveTimeByIndices( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->RecalculateTime();
	}
}

void CCurveEditorPanelHelper::OnRecalculateCurveRotationFromDirection( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->RecalculateRotationFromDirection();
	}
}

void CCurveEditorPanelHelper::OnEnablePositionLinearInterpolationMode( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->SetPositionInterpolationMode( ECurveInterpolationMode_Linear );
	}
}

void CCurveEditorPanelHelper::OnEnablePositionAutomaticInterpolationMode( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->SetPositionInterpolationMode( ECurveInterpolationMode_Automatic );
	}
}

void CCurveEditorPanelHelper::OnEnablePositionManualInterpolationMode( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->SetPositionInterpolationMode( ECurveInterpolationMode_Manual );
	}
}

void CCurveEditorPanelHelper::OnEnableBezierManualMode( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->SetPositionManualMode( ECurveManualMode_Bezier );
	}
}

void CCurveEditorPanelHelper::OnEnableBezierSymmetricManualMode( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->SetPositionManualMode( ECurveManualMode_BezierSymmetric );
	}
}

void CCurveEditorPanelHelper::OnEnableBezierSymmetricDirectionManualMode( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->SetPositionManualMode( ECurveManualMode_BezierSymmetricDirection );
	}
}

void CCurveEditorPanelHelper::OnRecalculateBezierTangents( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->RecalculateBezierTangents();
	}
}

void CCurveEditorPanelHelper::OnEnableRotationLinearInterpolationMode( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->SetRotationInterpolationMode( ECurveInterpolationMode_Linear );
	}
}

void CCurveEditorPanelHelper::OnEnableRotationAutomaticInterpolationMode( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->SetRotationInterpolationMode( ECurveInterpolationMode_Automatic );
	}
}

void CCurveEditorPanelHelper::OnToggleDrawOnTop( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		curveEntity->SetDrawOnTop( !curveEntity->GetDrawOnTop() );
	}
}

void CCurveEditorPanelHelper::OnToggleEaseParams( wxCommandEvent& event )
{
	if ( CCurveEntity* curveEntity = GetSelectedCurveEntity() )
	{
		SMultiCurve* curve = curveEntity->GetCurve();
		curve->EnableEaseParams( !curve->HasEaseParams() );
	}
}

void CCurveEditorPanelHelper::OnSelectCurveOwner( wxCommandEvent& event )
{
	CCurveEntity* curveEntity = GetSelectedCurveEntity();
	if ( !curveEntity )
	{
		return;
	}

	SMultiCurve* curve = curveEntity->GetCurve();
	CNode* curveParent = curve->GetParent();
	if ( !curveParent )
	{
		return;
	}

	// Change selection to curve parent

	m_panel->GetSelectionManager()->DeselectAll();
	m_panel->GetSelectionManager()->Select( curveParent );
}

void CCurveEditorPanelHelper::OnSpawnEntitiesTool(wxCommandEvent& event)
{
	m_curEnt = GetSelectedCurveEntity();
	if( !m_curEnt ) return;
	CCurveEntitySpawner* spawner = m_curEnt->GetCurveEntitySpawner();
	if ( !spawner ) return;
	m_dlg->SetOkCancelLabels( TXT("Spawn!"), TXT("Close") );
	m_dlg->Show( spawner, [this, spawner]( Bool result )
		{
			if ( result )
			{
				spawner->SpawnEntities( m_curEnt );
			}
			else
			{ // cancel
				m_dlg->Hide();
			}
		}
	);
}

void CCurveEditorPanelHelper::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	/*
	if ( name == CNAME( Detached ) )
	{
		CObject *obj = GetEventData< CObject* >( data );
		if ( obj == m_curEnt )
		{
			m_dlg.Hide();
		}
	}
	else 
	if ( name == CNAME( SelectionChanged ) )
	{
		m_curEnt = GetSelectedCurveEntity();
	}
	*/
}
