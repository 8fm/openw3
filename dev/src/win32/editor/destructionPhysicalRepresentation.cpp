#include "build.h"
#include "destructionPhysicalRepresentation.h"
#include "meshPreviewPanel.h"
#include "../../common/engine/mesh.h"

#include "../../common/engine/collisionMesh.h"
#include "../../common/engine/collisionShape.h"
#include "wx/string.h"
#include "../../common/engine/physicsDestructionResource.h"
#include "wx/xrc/xmlres.h"
#include "undoManager.h"

CEdDestructionPhysicalRepresentation::CEdDestructionPhysicalRepresentation( wxWindow* parent, CEdMeshPreviewPanel* preview, CPhysicsDestructionResource* mesh, CEdUndoManager* undoManager )
	: m_parent( parent )
	, m_preview( preview )
	, m_mesh( mesh )
{
	m_shapeList = XRCCTRL( *parent, "physrepShapeList1", wxChoice );
	ASSERT( m_shapeList );

	m_simTypeList = XRCCTRL( *parent, "simulationtypeChoice", wxChoice );
	ASSERT( m_simTypeList );

	m_shapeList->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdDestructionPhysicalRepresentation::OnSelectedShapeChanged ), NULL, this );
	m_simTypeList->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdDestructionPhysicalRepresentation::OnSimulationTypeChanged ), NULL, this );

	m_undoManager = undoManager;	

	// Create properties panel
	{
		wxPanel* rp = XRCCTRL( *parent, "DebrisPropertiesPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL|wxALL );
		PropertiesPageSettings settings;
		m_properties = new CEdPropertiesBrowserWithStatusbar( rp, settings, m_undoManager );
		sizer1->Add( m_properties, 1, wxALL, 5 );
		rp->SetSizer( sizer1 );
	}

	// Show additional info properties
	m_properties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdDestructionPhysicalRepresentation::OnPropertyModified ), NULL, this );
	m_properties->Get().SetObject( m_mesh->GetAdditionalInfoPtr(0) );

	m_simTypeList->Clear();

	wxString str;
	str.Printf( "DYNAMIC" );
	m_simTypeList->Append( str );
	str.Printf( "STATIC" );
	m_simTypeList->Append( str );

	m_previewComponent = (CMeshTypePreviewPhysicsDestructionComponent*)m_preview->GetMeshPreviewComponent();

	Refresh();
}


CEdDestructionPhysicalRepresentation::~CEdDestructionPhysicalRepresentation( )
{
}

void CEdDestructionPhysicalRepresentation::Refresh()
{
	Uint32 sel = GetSelectedShape();

	m_shapeList->Clear();

	const CCollisionMesh* collisionMesh = m_mesh->GetCollisionMesh();

	m_shapeList->Enable( collisionMesh != 0 );
	m_simTypeList->Enable( collisionMesh != 0 );

	const TDynArray< ICollisionShape* >& shapes = collisionMesh->GetShapes();

	m_simTypeList->Enable( !shapes.Empty() );
	m_shapeList->Enable( !shapes.Empty() );

	if( shapes.Empty() ) return;

	for ( Uint32 i = 0; i < shapes.Size(); ++i )
	{
		wxString str;
		str.Printf( "%d: %s", i, shapes[i]->GetClass()->GetName().AsString().AsChar() );
		m_shapeList->Append( str );
	}

	if( sel < 0 || sel >= m_shapeList->GetCount() )
	{
		sel = 0;
	}

	m_shapeList->Select( sel );

	OnSelectedShapeChanged( wxCommandEvent() );
}

void CEdDestructionPhysicalRepresentation::OnSelectedShapeChanged(wxCommandEvent& event)
{
	Int32 selectedShape = GetSelectedShape();
	if ( selectedShape < 0 ) return;

	// Refresh other controls to reflect this shape.
	const CCollisionMesh* collisionMesh = m_mesh->GetCollisionMesh();
	ASSERT( collisionMesh );

	const TDynArray< ICollisionShape* >& shapes = collisionMesh->GetShapes();
	ASSERT( ( Uint32 )selectedShape < shapes.Size() );

	// Mark this shape as active, for debug rendering.
	m_previewComponent->SetActiveCollisionShape( selectedShape );

	//CPhysicsDestructionResource* physRes = SafeCast< CPhysicsDestructionResource >( m_previewComponent->GetMeshTypeResource() );
	m_simTypeList->SetSelection( m_mesh->GetSimTypeForChunk( selectedShape ) );

	// Show additional info properties
	m_properties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdDestructionPhysicalRepresentation::OnPropertyModified ), NULL, this );
	m_properties->Get().SetObject( m_mesh->GetAdditionalInfoPtr( selectedShape ) );
}

void CEdDestructionPhysicalRepresentation::OnPropertyModified( wxCommandEvent& event )
{

}

void CEdDestructionPhysicalRepresentation::OnSimulationTypeChanged(wxCommandEvent& event)
{
	Int32 selectedSimType = GetSelectedSimType();
	if ( selectedSimType < 0 ) return;

	Int32 selectedShape = GetSelectedShape();
	if ( selectedShape < 0 ) return;

	//CPhysicsDestructionResource* physRes = SafeCast< CPhysicsDestructionResource >( m_previewComponent->GetMeshTypeResource() );
	m_mesh->SetSimTypeForChunk( selectedShape, (EPhysicsDestructionSimType)selectedSimType );
}

void CEdDestructionPhysicalRepresentation::UpdatePhysXDestructionResource()
{

}

Int32 CEdDestructionPhysicalRepresentation::GetSelectedShape() const
{
	return m_shapeList->GetSelection();
}

Int32 CEdDestructionPhysicalRepresentation::GetSelectedSimType()
{
	return m_simTypeList->GetSelection();
}

