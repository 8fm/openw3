/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "sceneExplorer.h"
#include "undoCreate.h"
#include "../../common/core/depot.h"
#include "../../common/engine/pathlibWorld.h"
#include "../../common/engine/areaComponent.h"

class CEdSeedToolPanel;

/// Editor tool for navigation meshes
class CEdSeedTool : public IEditorTool
{
	DECLARE_ENGINE_CLASS( CEdSeedTool, IEditorTool, 0 );

protected:
	CEdSeedToolPanel*				m_panel;
	CEdRenderingPanel*				m_viewport;
	CWorld*							m_world;
	TDynArray< CAreaComponent* >	m_areasToSeedIn;

public:
	virtual String GetCaption() const { return TXT("Seed"); }

	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void End() {}

	void Seed( Float distMean, Float distVariance, Float rotMean, Float rotVariance, Float zShift );
};

BEGIN_CLASS_RTTI( CEdSeedTool );
	PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CEdSeedTool );

/// Navigation mesh tool panel
class CEdSeedToolPanel : public CEdDraggablePanel
{
	DECLARE_EVENT_TABLE();

protected:
	CEdSeedTool*					m_tool;

public:
	CEdSeedToolPanel( CEdSeedTool* tool, wxWindow* parent )
		: m_tool( tool )
	{
		// Load layout from XRC
		wxXmlResource::Get()->LoadPanel( this, parent, wxT("SeedTool") );
	}

protected:
	void OnSeed( wxCommandEvent &event )
	{
		wxTextCtrl * txtDistMean     = XRCCTRL( *this, "txtDistMean", wxTextCtrl );
		wxTextCtrl * txtDistVariance = XRCCTRL( *this, "txtDistVariance", wxTextCtrl );
		wxTextCtrl * txtRotMean      = XRCCTRL( *this, "txtRotMean", wxTextCtrl );
		wxTextCtrl * txtRotVariance  = XRCCTRL( *this, "txtRotVariance", wxTextCtrl );
		wxTextCtrl * txtZShift       = XRCCTRL( *this, "txtZShift", wxTextCtrl );

		Float distMean, distVariance, rotMean, rotVariance, zShift;
		if ( ! FromString( txtDistMean->GetValue().wc_str(), distMean ) )
			distMean = 4.f;
		if ( ! FromString( txtDistVariance->GetValue().wc_str(), distVariance ) )
			distVariance = 1.f;
		if ( ! FromString( txtRotMean->GetValue().wc_str(), rotMean ) )
			rotMean = 0.f;
		if ( ! FromString( txtRotVariance->GetValue().wc_str(), rotVariance ) )
			rotVariance = 180.f;
		if ( ! FromString( txtZShift->GetValue().wc_str(), zShift ) )
			zShift = 0.f;

		m_tool->Seed( distMean, distVariance, rotMean, rotVariance, zShift );
	}
};

BEGIN_EVENT_TABLE( CEdSeedToolPanel, CEdDraggablePanel )
	EVT_BUTTON( XRCID("btnSeed"), CEdSeedToolPanel::OnSeed )
END_EVENT_TABLE()

Bool CEdSeedTool::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	m_areasToSeedIn.Clear();
	for ( Uint32 i=0; i<selection.Size(); i++ )
	{
		CAreaComponent * area = Cast< CAreaComponent >( selection[ i ] );
		if ( area != NULL )
		{
			m_areasToSeedIn.PushBack( area );
		}
	}
	if ( m_areasToSeedIn.Empty() )
	{
		wxMessageBox( TXT("Cannot seed, no areas selected!"), TXT("Seed tool"), wxOK | wxCENTRE, m_viewport );
		return false;
	}

	m_world    = world;
	m_viewport = viewport;

	// Create tool panel
	m_panel = new CEdSeedToolPanel( this, panel );

	// Create panel for custom window
	panelSizer->Add( m_panel, 1, wxEXPAND, 5 );
	panel->Layout();

	return true;
}

void CEdSeedTool::Seed( Float distMean, Float distVariance, Float rotMean, Float rotVariance, Float zShift )
{
	// Get selected template
	String resource;
	if ( !GetActiveResource( resource ) )
	{
		wxMessageBox( TXT("Cannot seed, no entity template selected!"), TXT("Seed tool"), wxOK | wxCENTRE, m_viewport );
		return;
	}	

	// Load template
	CEntityTemplate * entityTemplate = Cast< CEntityTemplate >( GDepot->LoadResource( resource ) );
	if ( entityTemplate == NULL )
	{
		wxMessageBox( TXT("Cannot seed, no entity template selected!"), TXT("Seed tool"), wxOK | wxCENTRE, m_viewport );
		return;
	}

	// Make sure distance is reasonable
	if ( distMean < 0.1f )
	{
		distMean = 0.1f;
	}

	// Calculate total bounding box
	ASSERT( ! m_areasToSeedIn.Empty() );
	Box boundingBox = m_areasToSeedIn[ 0 ]->GetBoundingBox();
	for ( Uint32 i = 1; i < m_areasToSeedIn.Size(); ++i )
	{
		boundingBox.AddBox( m_areasToSeedIn[ i ]->GetBoundingBox() );
	}

	// Get selected layer
	CLayer *layer = wxTheFrame->GetSceneExplorer()->GetActiveLayer();
	if ( layer == NULL )
	{
		wxMessageBox( TXT("Cannot seed, no layer activated!"), TXT("Seed tool"), wxOK | wxCENTRE, m_viewport );
		return;
	}

	CPathLibWorld* pathlib = m_world->GetPathLibWorld();
	
	Bool added = false;
	for ( Float x = boundingBox.Min.X; x < boundingBox.Max.X; x += distMean )
	{
		for ( Float y = boundingBox.Min.Y; y < boundingBox.Max.Y; y += distMean )
		{
			// Calculate position with jitter
			Vector pos = Vector( x + GEngine->GetRandomNumberGenerator().Get< Float >( -distVariance , distVariance ), y + GEngine->GetRandomNumberGenerator().Get< Float >( -distVariance , distVariance ), boundingBox.Max.Z );

			// Try to place in position
			if ( pathlib && !pathlib->TestLocation( pos, 0.2f, PathLib::CT_DEFAULT ) )
			{
				
				if ( !pathlib->FindSafeSpot( PathLib::INVALID_AREA_ID, pos.AsVector3(), distVariance, 0.2f, pos.AsVector3() ) )
				{
					// No position can be found
					continue;
				}
			}

			// Snap to mesh
			TraceResultPlacement result;
			if ( CTraceTool::StaticPlacementTraceTest( m_world, pos, 0.4f, result ) )
			{
				pos.Z = result.m_height;
			}

			// Adjust
			pos.Z += zShift;

			// Accept only points inside the areas
			Bool inArea = false;
			for ( Uint32 i = 0; i < m_areasToSeedIn.Size(); ++i )
			{
				if ( m_areasToSeedIn[ i ]->GetBoundingBox().Contains( pos ) && m_areasToSeedIn[ i ]->TestPointOverlap( pos ) )
				{
					inArea = true;
					break;
				}
			}

			// Well, point was not in any area
			if ( !inArea )
			{
				continue;
			}

			// Spawn entity
			EntitySpawnInfo spawnInfo;
			spawnInfo.m_template = entityTemplate;
			spawnInfo.m_spawnPosition = pos;
			spawnInfo.m_spawnRotation = EulerAngles( 0.f, 0.f, rotMean + GEngine->GetRandomNumberGenerator().Get< Float >( -rotVariance , rotVariance ) );

			// Create undo step
			CEntity * entity = layer->CreateEntitySync( spawnInfo );
			CUndoCreateDestroy::CreateStep( m_viewport->GetUndoManager(), entity, true );
			added = true;
		}
	}

	// Finish undo step if anything was added
	if ( added )
	{
		CUndoCreateDestroy::FinishStep( m_viewport->GetUndoManager() );
	}
}
