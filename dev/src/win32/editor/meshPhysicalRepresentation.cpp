/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "meshPhysicalRepresentation.h"
#include "meshPreviewPanel.h"
#include "../../common/engine/mesh.h"

#include "../../common/core/versionControl.h"
#include "../../common/engine/collisionMesh.h"
#include "../../common/engine/collisionShape.h"

class CEdChooseCollisionParamsDialog : public wxDialog 
{
	DECLARE_EVENT_TABLE();

public:
	Float	m_scale;

public:
	CEdChooseCollisionParamsDialog( wxWindow *parent )
		: m_scale( 1.0f )
	{
		wxXmlResource::Get()->LoadDialog( this, parent, wxT("ChooseCollisionParams") );
	}

public:	
	void OnCancel( wxCommandEvent &event )
	{
		EndDialog( SC_CANCEL ); 
	}

	void OnOK( wxCommandEvent &event )
	{
		// Allocate textures
		m_scale = (Float) XRCCTRL( *this, "Scale", wxSpinCtrl )->GetValue() / 100.0f;

		// End dialog
		EndDialog( SC_OK ); 
	}

};

BEGIN_EVENT_TABLE( CEdChooseCollisionParamsDialog, wxDialog )
	EVT_BUTTON( XRCID("OK"), CEdChooseCollisionParamsDialog::OnOK )
	EVT_BUTTON( XRCID("Cancel"), CEdChooseCollisionParamsDialog::OnCancel )
END_EVENT_TABLE()


CEdMeshPhysicalRepresentation::CEdMeshPhysicalRepresentation( wxWindow* parent, CEdMeshPreviewPanel* preview, CMesh* mesh )
	: m_parent( parent )
	, m_preview( preview )
	, m_mesh( mesh )
{
	SEvents::GetInstance().RegisterListener( CNAME( CSVFileSaved ), this );

	m_physicalMaterial = XRCCTRL( *parent, "PhysicalMaterial", wxChoice );
	ASSERT( m_physicalMaterial );

	m_shapeList = XRCCTRL( *parent, "physrepShapeList", wxChoice );
	ASSERT( m_shapeList );

	m_shapeList->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdMeshPhysicalRepresentation::OnSelectedShapeChanged ), NULL, this );
	m_physicalMaterial->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdMeshPhysicalRepresentation::OnPhysicalMaterialChoicebookPageChanged ), NULL, this );
	XRCCTRL( *parent, "CollisionGenerationRemoveAll", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMeshPhysicalRepresentation::OnMeshCollisionRemoveAll ), NULL, this );
	XRCCTRL( *parent, "CollisionGenerationRemove", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMeshPhysicalRepresentation::OnMeshCollisionRemove ), NULL, this );
	XRCCTRL( *parent, "CollisionBox", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMeshPhysicalRepresentation::OnMeshCollisionAdd ), NULL, this );
	XRCCTRL( *parent, "CollisionConvex", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMeshPhysicalRepresentation::OnMeshCollisionAdd ), NULL, this );
	XRCCTRL( *parent, "CollisionTrimesh", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMeshPhysicalRepresentation::OnMeshCollisionAdd ), NULL, this );
	XRCCTRL( *parent, "CollisionSphere", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMeshPhysicalRepresentation::OnMeshCollisionAdd ), NULL, this );
	XRCCTRL( *parent, "CollisionCapsule", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMeshPhysicalRepresentation::OnMeshCollisionAdd ), NULL, this );
	
	m_densityScaler = XRCCTRL( *parent, "DensityScaler", wxTextCtrl );
	XRCCTRL( *parent, "FillDensityScaler", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMeshPhysicalRepresentation::OnFillDensityScaler ), NULL, this );

	m_soundOcclusionEnabled = XRCCTRL( *parent, "SoundOcclusionEnabled", wxCheckBox );
	m_soundOcclusionAttenuation = XRCCTRL( *parent, "SoundOcclusionAttenuation", wxTextCtrl );
	m_soundOcclusionDiagonalLimit = XRCCTRL( *parent, "SoundOcclusionDiagonalLimit", wxTextCtrl );

	XRCCTRL( *parent, "DensityScaler", wxTextCtrl )->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdMeshPhysicalRepresentation::OnDensityScaler ), NULL, this );

	XRCCTRL( *parent, "SoundOcclusionEnabled", wxCheckBox )->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdMeshPhysicalRepresentation::OnSoundAttenuationSwitch ), NULL, this );
	XRCCTRL( *parent, "SoundOcclusionAttenuation", wxTextCtrl )->Connect( wxEVT_AFTER_CHAR, wxCommandEventHandler( CEdMeshPhysicalRepresentation::OnSoundAttenuationParametersChanged ), NULL, this );
	XRCCTRL( *parent, "SoundOcclusionDiagonalLimit", wxTextCtrl )->Connect( wxEVT_AFTER_CHAR, wxCommandEventHandler( CEdMeshPhysicalRepresentation::OnSoundAttenuationParametersChanged ), NULL, this );

	m_rotationAxis = XRCCTRL( *parent, "AxisChoice", wxChoice );
	ASSERT( m_rotationAxis );
	m_rotationAxis->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdMeshPhysicalRepresentation::OnRotationAxisChanged ), NULL, this );

	Refresh();
};

CEdMeshPhysicalRepresentation::~CEdMeshPhysicalRepresentation()
{
	SEvents::GetInstance().UnregisterListener( this );
}

void CEdMeshPhysicalRepresentation::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( CSVFileSaved ) )
	{
		if( GPhysicEngine->GetPhysicalMaterialFileResourceName().ContainsSubstring( GetEventData< String >( data ) ) )
			Refresh();
	}
}

void CEdMeshPhysicalRepresentation::Refresh()
{
	Uint32 sel = GetSelectedShape();
	m_physicalMaterial->Clear();
	m_shapeList->Clear();
	m_rotationAxis->Clear();

	const CCollisionMesh* collisionMesh = m_mesh->GetCollisionMesh();

	m_physicalMaterial->Enable( collisionMesh != 0 );
	m_rotationAxis->Enable( collisionMesh != 0 );
	m_shapeList->Enable( collisionMesh != 0 );
	m_densityScaler->Enable( collisionMesh != 0 );
	if( !collisionMesh )
	{
		m_densityScaler->SetValue( "" );
		m_soundOcclusionEnabled->SetValue( false );
		m_soundOcclusionAttenuation->Enable( false );
		m_soundOcclusionDiagonalLimit->Enable( false );
		return;
	}

	TDynArray< CName > materials;
	GPhysicEngine->GetPhysicalMaterialNames( materials );
	for( Uint32 i = 0; i != materials.Size(); ++i )
	{
		wxString string( materials[ i ].AsString().AsChar() );
		m_physicalMaterial->Insert( 1, &string, i ); 
	}

	//rotations
	wxString axisNames[4] = {"No rotation", "Around OX", "Around OY", "Around OZ"};

	for( Uint32 i = 0; i != 4; ++i )
	{
		m_rotationAxis->Insert( 1, &axisNames[i], i ); 
	}
	m_rotationAxis->Select(collisionMesh->GetSwimmingRotationAxis());
	//----

	const TDynArray< ICollisionShape* >& shapes = collisionMesh->GetShapes();

	m_physicalMaterial->Enable( !shapes.Empty() );
	m_rotationAxis->Enable( !shapes.Empty() );
	m_shapeList->Enable( !shapes.Empty() );
	m_densityScaler->Enable( !shapes.Empty() );

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

	CCollisionMesh* mesh = const_cast< CCollisionMesh* >( m_mesh->GetCollisionMesh() );

	Float occlusionDiameterLimit = mesh->GetOcclusionDiagonalLimit();
	Float occlusionAttenuation = mesh->GetOcclusionAttenuation();
	if( occlusionDiameterLimit < 0.0f && occlusionAttenuation < 0.0f )
	{
		m_soundOcclusionEnabled->SetValue( false );
	}
	else
	{
		m_soundOcclusionEnabled->SetValue( true );
	}

	if( m_soundOcclusionEnabled->GetValue() == false )
	{
		mesh->SetOcclusionAttenuation( -1.0f );
		mesh->SetOcclusionDiagonalLimit( -1.0f );
		m_soundOcclusionAttenuation->Enable( false );
		m_soundOcclusionDiagonalLimit->Enable( false );
		return;
	}

	m_soundOcclusionDiagonalLimit->SetValue( ToString( occlusionDiameterLimit ).AsChar() );
	m_soundOcclusionAttenuation->SetValue( ToString( occlusionAttenuation ).AsChar() );
}


void CEdMeshPhysicalRepresentation::OnSelectedShapeChanged( wxCommandEvent& event )
{
	Int32 selectedShape = GetSelectedShape();
	if ( selectedShape < 0 ) return;

	// Refresh other controls to reflect this shape.
	const CCollisionMesh* collisionMesh = m_mesh->GetCollisionMesh();
	ASSERT( collisionMesh );

	const TDynArray< ICollisionShape* >& shapes = collisionMesh->GetShapes();
	ASSERT( ( Uint32 )selectedShape < shapes.Size() );

	UpdateMaterials();

	// Mark this shape as active, for debug rendering.
	m_preview->GetMeshPreviewComponent()->SetActiveCollisionShape( selectedShape );

	Float fillmentRatio = shapes[ selectedShape ]->GetDensityScaler();
	m_densityScaler->SetValue( wxString::FromCDouble( fillmentRatio ) );
}

void CEdMeshPhysicalRepresentation::OnPhysicalMaterialChoicebookPageChanged( wxCommandEvent& event )
{
	Int32 selectedShape = GetSelectedShape();
	ASSERT( selectedShape >= 0 );

	Int32 carret = m_physicalMaterial->GetSelection();
	if( carret == -1 ) return;

	wxString selectionName = m_physicalMaterial->GetStringSelection();
	CName physicalMaterialName( selectionName.wx_str() );

	String temp = physicalMaterialName.AsString();

	const CCollisionMesh* collisionMesh = m_mesh->GetCollisionMesh();

	if( !collisionMesh ) return;

	const TDynArray< ICollisionShape* >& shapes = collisionMesh->GetShapes();
	if ( ( Uint32 )selectedShape < shapes.Size() )
	{
		ICollisionShape* shape = shapes[selectedShape];
		// Set all materials on this shape.
		shape->SetPhysicalMaterial( physicalMaterialName, 0, shape->GetNumPhysicalMaterials() );
	}

	UpdateMaterials();
}


void CEdMeshPhysicalRepresentation::OnRotationAxisChanged( wxCommandEvent& event )
{
	int selectionIndex = m_rotationAxis->GetSelection();

	CCollisionMesh* collisionMesh = const_cast< CCollisionMesh* >( m_mesh->GetCollisionMesh() );

	if( !collisionMesh ) return;

	collisionMesh->SetSwimmingRotationAxis(selectionIndex);

}


void CEdMeshPhysicalRepresentation::OnMeshCollisionRemoveAll( wxCommandEvent& event )
{
	if ( m_mesh->GetCollisionMesh() && m_mesh->MarkModified() )
	{
		// Deselect active collision shape in the preview mesh.
		m_preview->GetMeshPreviewComponent()->SetActiveCollisionShape( -1 );
		// Remove collision from the mesh.
		m_mesh->RemoveCollision();

	}
	Refresh();
}


void CEdMeshPhysicalRepresentation::OnMeshCollisionRemove( wxCommandEvent& event )
{
	Int32 selectedShape = GetSelectedShape();

	if ( selectedShape < 0 ) return;

	if ( m_mesh->GetCollisionMesh() && m_mesh->MarkModified() )
	{
		m_mesh->RemoveCollisionShape( selectedShape );

		m_shapeList->Delete( selectedShape );

		if ( m_mesh->GetCollisionMesh()->GetShapes().Size() > 0 )
		{
			// Cast is fine, since we checked negative above.
			if ( (Uint32)selectedShape < m_shapeList->GetCount() )
			{
				m_shapeList->Select( selectedShape );
			}
			else
			{
				m_shapeList->Select( Max( selectedShape - 1, 0 ) );
			}
			OnSelectedShapeChanged( wxCommandEvent() );
		}
	}
	Refresh();

}

void CEdMeshPhysicalRepresentation::OnMeshCollisionAdd( wxCommandEvent& event )
{
	const Bool shiftPressed = RIM_IS_KEY_DOWN( IK_LShift );
	CEdChooseCollisionParamsDialog dialog( NULL );
	if ( !shiftPressed || dialog.ShowModal() == SC_OK )
	{
		if ( m_mesh->MarkModified() )
		{
			m_mesh->RemoveCollision();
			Uint32 i = (Uint32)-1;
			if ( event.GetEventObject() == XRCCTRL( *m_parent, "CollisionBox", wxButton ) )
			{
				i = m_mesh->AddBoxCollision( dialog.m_scale );
			}
			else if ( event.GetEventObject() == XRCCTRL( *m_parent, "CollisionConvex", wxButton ) )
			{
				i = m_mesh->AddConvexCollision( dialog.m_scale );
			}
			else if ( event.GetEventObject() == XRCCTRL( *m_parent, "CollisionTrimesh", wxButton ) )
			{
				i = m_mesh->AddTriMeshCollision( dialog.m_scale );
			}
			else if ( event.GetEventObject() == XRCCTRL( *m_parent, "CollisionSphere", wxButton ) )
			{
				i = m_mesh->AddSphereCollision( dialog.m_scale );
			}
			else if ( event.GetEventObject() == XRCCTRL( *m_parent, "CollisionCapsule", wxButton ) )
			{
				i = m_mesh->AddCapsuleCollision( dialog.m_scale );
			}

			if ( i != (Uint32)-1 )
			{
				Refresh();
				m_shapeList->Select( ( Int32 )i );
				OnSelectedShapeChanged( wxCommandEvent() );
			}
		}
	}
}

void CEdMeshPhysicalRepresentation::OnDensityScaler( wxCommandEvent& event )
{
	CCollisionMesh* mesh = const_cast< CCollisionMesh* >( m_mesh->GetCollisionMesh() );

	Int32 selectedShape = GetSelectedShape();

	if( selectedShape < 0 ) return;

	const TDynArray< ICollisionShape* >& shapes = mesh->GetShapes();

	if( selectedShape >= (Int32)shapes.Size() ) return;

	ICollisionShape* shape = shapes[ selectedShape ];

	double densityScaler = 0.0f;
	m_densityScaler->GetValue().ToCDouble( &densityScaler );
	if( densityScaler <= 0.0f )
	{
		densityScaler = 0.0001f;
		m_densityScaler->SetValue( wxString::FromDouble( densityScaler ) );
	}
	shape->SetDensityScaler( densityScaler );
}

void CEdMeshPhysicalRepresentation::OnSoundAttenuationSwitch( wxCommandEvent& event )
{
	CCollisionMesh* mesh = const_cast< CCollisionMesh* >( m_mesh->GetCollisionMesh() );
	if( !mesh )
	{
		m_soundOcclusionAttenuation->Enable( false );
		m_soundOcclusionDiagonalLimit->Enable( false );
		return;
	}
	m_soundOcclusionAttenuation->Enable( m_soundOcclusionEnabled->GetValue() );
	m_soundOcclusionDiagonalLimit->Enable( m_soundOcclusionEnabled->GetValue() );
	if( m_soundOcclusionEnabled->GetValue() == false )
	{
		mesh->SetOcclusionAttenuation( -1.0f );
		mesh->SetOcclusionDiagonalLimit( -1.0f );
		m_soundOcclusionAttenuation->SetValue( wxString::FromDouble( -1.0f ) );
		m_soundOcclusionDiagonalLimit->SetValue( wxString::FromDouble( -1.0f ) );
	}
	else
	{
		mesh->SetOcclusionAttenuation( 0.0f );
		mesh->SetOcclusionDiagonalLimit( 0.0f );
		m_soundOcclusionAttenuation->SetValue( wxString::FromDouble( 0.0f ) );
		m_soundOcclusionDiagonalLimit->SetValue( wxString::FromDouble( 0.0f ) );
	}
}

void CEdMeshPhysicalRepresentation::OnSoundAttenuationParametersChanged( wxCommandEvent& event )
{
	CCollisionMesh* mesh = const_cast< CCollisionMesh* >( m_mesh->GetCollisionMesh() );

	double occlusionDiameterLimit = 0.0f;
	double occlusionAttenuation = 0.0f;
	m_soundOcclusionDiagonalLimit->GetValue().ToCDouble( &occlusionDiameterLimit );
	m_soundOcclusionAttenuation->GetValue().ToCDouble( &occlusionAttenuation );
	mesh->SetOcclusionDiagonalLimit( occlusionDiameterLimit );
	mesh->SetOcclusionAttenuation( occlusionAttenuation );
}


void CEdMeshPhysicalRepresentation::UpdateMaterials()
{
	const CCollisionMesh* collisionMesh = m_mesh->GetCollisionMesh();
	if ( !collisionMesh ) return;

	const TDynArray< ICollisionShape* >& shapes = collisionMesh->GetShapes();
	Int32 selectedShape = GetSelectedShape();

	ASSERT( selectedShape >= 0, TXT("No shape selected, but trying to update material list.") );

	Uint32 numMaterials = shapes[selectedShape]->GetNumPhysicalMaterials();
	TDynArray<String> mtlNames;
	for ( Uint32 i = 0; i < numMaterials; ++i )
	{
		mtlNames.PushBackUnique( shapes[selectedShape]->GetPhysicalMaterial( i ).AsString() );
	}

	String materialName = String::Join( mtlNames, TXT("; ") );

	// If this shape has more than one material, we want to list them in the label below the material selection.
	// This is the best we can do, since we can't (currently) select multiple materials or anything.
	if ( mtlNames.Size() > 1 )
	{
		XRCCTRL( *m_parent, "PhysicalMaterialMulti", wxStaticText )->SetLabelText( materialName.AsChar() );
	}
	// Only one material, so clear out the multi-material label.
	else
	{
		XRCCTRL( *m_parent, "PhysicalMaterialMulti", wxStaticText )->SetLabelText( "" );
	}

	// Select the material in the material list. If it's not found in the material list (e.g. a list of several
	// different materials), nothing will be selected (dropdown appears empty).
	Int32 carret = m_physicalMaterial->FindString( materialName.AsChar() );
	m_physicalMaterial->Select( carret );
}


Int32 CEdMeshPhysicalRepresentation::GetSelectedShape() const
{
	return m_shapeList->GetSelection();
}

void CEdMeshPhysicalRepresentation::OnFillDensityScaler( wxCommandEvent& event )
{
	CCollisionMesh* mesh = const_cast< CCollisionMesh* >( m_mesh->GetCollisionMesh() );
	Int32 selectedShape = GetSelectedShape();
	ASSERT( selectedShape >= 0 );

	if( selectedShape < 0 ) return;

	const TDynArray< ICollisionShape* >& shapes = mesh->GetShapes();

	const Int32 shapesSize = (Int32)shapes.Size();

	if( selectedShape >= shapesSize ) return;

	ICollisionShape* shape = NULL;
	for ( Int32 i=0; i<shapesSize; ++i )
	{
		shape = shapes[ i ];
		double densityScaler = 0.0f;
		m_densityScaler->GetValue().ToCDouble( &densityScaler );
		if( densityScaler <= 0.0f )
		{
			densityScaler = 0.0001f;
			m_densityScaler->SetValue( wxString::FromDouble( densityScaler ) );
		}
		shape->SetDensityScaler( densityScaler );
	}
}
