/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "sceneExplorer.h"
#include "undoCreate.h"
#include "detachablePanel.h"
#include "../../common/engine/mesh.h"
#include "../../common/core/depot.h"
#include "renderingPanel.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/meshComponent.h"
#include "../../common/engine/staticMeshComponent.h"
#include "../../common/engine/renderFrame.h"


RED_DEFINE_STATIC_NAME( objectTemplate );
RED_DEFINE_STATIC_NAME( objectMesh );
RED_DEFINE_STATIC_NAME( scaleMean );
RED_DEFINE_STATIC_NAME( scaleVariance );

class CEdGardener;

//////////////////////////////////////////////////////////////////////////
// CGardenerDropTarget - drag & drop implementation
//////////////////////////////////////////////////////////////////////////
class CGardenerDropTarget : public CDropTarget
{
	CEdGardener*		m_gardener;
	Uint32				m_stampIdx;

public:
	CGardenerDropTarget( wxWindow *window, CEdGardener *gardener, Uint32 stampIdx )
		: CDropTarget( window ), m_gardener( gardener ), m_stampIdx( stampIdx )
	{}

	virtual Bool OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources );
	virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
};

//////////////////////////////////////////////////////////////////////////
// CEdGardenerStamp - gardener stamp definition
//////////////////////////////////////////////////////////////////////////
class CEdGardenerStamp : public CObject
{
	DECLARE_ENGINE_CLASS( CEdGardenerStamp, CObject, 0 );

public:
	THandle< CEntityTemplate >	m_objectTemplate;
	THandle< CMesh >			m_objectMesh;

	Float				m_verticalShift;
	Float				m_yawMean;
	Float				m_yawVariance;
	Float				m_scaleMean;
	Float				m_scaleVariance;
	Float				m_maxNormalSnapAngle;
	Bool				m_addCollisions;
	Bool				m_castShadows;
	Bool				m_localWindSimulation;

	CEdGardenerStamp()
		: m_objectTemplate( NULL ), m_objectMesh( NULL )
		, m_verticalShift( 0.f )
		, m_yawMean( 0.f ), m_yawVariance( 180.f )
		, m_scaleMean( 1.f ), m_scaleVariance( 0.f )
		, m_maxNormalSnapAngle( 0.f )
		, m_addCollisions( false )
		, m_castShadows( false )
		, m_localWindSimulation( false )
	{}
};

BEGIN_CLASS_RTTI( CEdGardenerStamp )
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_objectTemplate, TXT("Stamp template") );
	PROPERTY_EDIT( m_objectMesh, TXT("Stamp mesh") );
	PROPERTY_EDIT( m_verticalShift, TXT("Vertical shift") );
	PROPERTY_EDIT( m_yawMean, TXT("Mean rotation") );
	PROPERTY_EDIT( m_yawVariance, TXT("Rotation variance") );
	PROPERTY_EDIT( m_scaleMean, TXT("Mean scale") );
	PROPERTY_EDIT( m_scaleVariance, TXT("Scale variance") );
	PROPERTY_EDIT( m_maxNormalSnapAngle, TXT("Max normal snap angle") );
	PROPERTY_EDIT( m_addCollisions, TXT("Create static meshes") );
	PROPERTY_EDIT( m_castShadows, TXT("Inserted mesh should cast shadows") );
	PROPERTY_EDIT( m_localWindSimulation, TXT("Inserted mesh should use local wind simulation") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CEdGardenerStamp );

//////////////////////////////////////////////////////////////////////////
// CEdGardener - gardener panel
//////////////////////////////////////////////////////////////////////////
class CEdGardener : public CEdDraggablePanel
{
	wxDECLARE_CLASS( CEdGardener );

private:
	CEdRenderingPanel*					m_viewport;

	CEdSelectionProperties*				m_properties;
	TDynArray< CEdGardenerStamp* >		m_stampDefinitions;

	wxToggleButton*						m_btnEraser;
	wxToggleButton*						m_btnHeightmapOnly;
	wxToggleButton*						m_btnRandomStamp;
	TDynArray< wxToggleButton* >		m_toggleButtons;
	TDynArray< wxTextCtrl* >			m_meshNames;
	wxComboBox*							m_cmbPresets;

	Int32									m_selectedStamp;
	THandle< CEntity >					m_stampGhost;
	Box									m_stampBoundingBox;
	Vector								m_stampPoint;
	Vector								m_stampPointNormal;
	Float								m_yaw;

	Bool								m_erased;
	Bool								m_erasing;
	Float								m_eraserRadius;

	TDynArray< CGardenerDropTarget* >	m_dropTargets;

	CEdDetachablePanel					m_detachablePanel;

public:
	CEdGardener( wxWindow* parent, CEdRenderingPanel* viewport );
	~CEdGardener();

	CEdGardenerStamp* GetStamp( Uint32 stampIdx ) { return m_stampDefinitions[ stampIdx ]; }

	void UpdateStampName( Uint32 stampIdx );
	void UpdateStampGhost();

	void SelectStamp( Int32 stampIdx );
	void PlaceStamp();
	void EraseStamps( Bool finish );

public:
	void OnShow(wxShowEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnEraserSelected( wxCommandEvent& event );
	void OnStampSelected( wxCommandEvent& event );
	void OnStampNameSelected( wxCommandEvent& event );
	void OnPropertiesChanged( wxCommandEvent& event );

	void OnPresetSave( wxCommandEvent& event );
	void OnPresetDelete( wxCommandEvent& event );
	void OnPresetSelected( wxCommandEvent& event );

	Bool HandleSelection( const TDynArray< CHitProxyObject* >& objects ) { return m_stampGhost.Get() || m_btnEraser->GetValue(); }
	Bool OnViewportTrack( const CMousePacket& packet );
	Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y ) { return button == 0 && ( m_stampGhost.Get() || m_btnEraser->GetValue() ); }
	Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
	Bool OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
};

//////////////////////////////////////////////////////////////////////////
// CEdGardenerTool - gardener tool
//////////////////////////////////////////////////////////////////////////
class CEdGardenerTool : public IEditorTool
{
	DECLARE_ENGINE_CLASS( CEdGardenerTool, IEditorTool, 0 );

private:
	CEdGardener *	m_gardener;

public:
	virtual String GetCaption() const { return TXT("Gardener"); }

	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
	{
		m_gardener = new CEdGardener( panel, viewport );

		// Create panel for custom window
		panelSizer->Add( m_gardener, 1, wxEXPAND, 5 );
		panel->Layout();

		return true;
	}

	virtual void End()
	{
		m_gardener = NULL;
	}

	virtual Bool HandleSelection( const TDynArray< CHitProxyObject* >& objects )
		{ return m_gardener->HandleSelection( objects ); }
	virtual Bool OnViewportTrack( const CMousePacket& packet )
		{ return m_gardener->OnViewportTrack( packet ); }
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
		{ return m_gardener->OnViewportClick( view, button, state, x, y ); }
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
		{ return m_gardener->OnViewportInput( view, key, action, data ); }
	virtual Bool OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
		{ return m_gardener->OnViewportGenerateFragments( view, frame ); }
	virtual Bool OnDelete() { return false; }
};

DEFINE_SIMPLE_RTTI_CLASS( CEdGardenerTool, IEditorTool );
IMPLEMENT_ENGINE_CLASS( CEdGardenerTool );

wxIMPLEMENT_CLASS( CEdGardener, CEdDraggablePanel );

//////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////
CEdGardener::CEdGardener( wxWindow* parent, CEdRenderingPanel* viewport )
	: m_selectedStamp( -1 )
	, m_stampGhost( NULL )
	, m_viewport( viewport )
	, m_stampPointNormal( 0.f, 0.f, 1.f, 0.f )
	, m_erased( false )
	, m_erasing( false )
	, m_eraserRadius( 1.f )
{
	wxXmlResource::Get()->LoadPanel( this, parent, TXT("Gardener") );

	// Create properties
	{
		wxPanel* rp = XRCCTRL( *this, "m_pnlProperties", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		settings.m_showEntityComponents = false;
		m_properties = new CEdSelectionProperties( rp, settings, NULL );
		m_properties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdGardener::OnPropertiesChanged ), NULL, this );
		sizer1->Add( m_properties, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	for ( Uint32 i = 0; i < 10; ++i )
	{
		CEdGardenerStamp * stamp = CreateObject< CEdGardenerStamp >();
		stamp->AddToRootSet();

		m_stampDefinitions.PushBack( stamp );
	}

	m_btnHeightmapOnly	= XRCCTRL( *this, "m_btnHeightmapOnly", wxToggleButton );
	m_btnRandomStamp	= XRCCTRL( *this, "m_btnRandomStamp", wxToggleButton );

	m_btnEraser = XRCCTRL( *this, "m_btnEraser", wxToggleButton );
	m_btnEraser->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( CEdGardener::OnEraserSelected ), NULL, this );

	String buttonName  = TXT("m_btnMesh");
	String textBoxName = TXT("m_txtMeshName");
	for ( Uint32 i = 0; i < 10; ++i )
	{
		int xrcid = wxXmlResource::GetXRCID( ( buttonName + ToString( i ) ).AsChar() );
		wxToggleButton * btn = wxStaticCast( FindWindow( xrcid ), wxToggleButton );
		ASSERT( btn );
		m_toggleButtons.PushBack( btn );

		btn->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( CEdGardener::OnStampSelected ), NULL, this );

		xrcid = wxXmlResource::GetXRCID( ( textBoxName + ToString( i ) ).AsChar() );
		wxTextCtrl * lbl = wxStaticCast( FindWindow( xrcid ), wxTextCtrl );
		ASSERT( lbl );
		m_meshNames.PushBack( lbl );

		lbl->Connect( wxEVT_SET_FOCUS, wxCommandEventHandler( CEdGardener::OnStampNameSelected ), NULL, this );

		m_dropTargets.PushBack( new CGardenerDropTarget( lbl, this, i ) );
	}

	{
		wxBitmapButton * btn = XRCCTRL( *this, "m_btnPresetSave", wxBitmapButton );
		btn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdGardener::OnPresetSave ), NULL, this );
		
		btn = XRCCTRL( *this, "m_btnPresetDelete", wxBitmapButton );
		btn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdGardener::OnPresetDelete ), NULL, this );

		m_cmbPresets = XRCCTRL( *this, "m_cmbPresets", wxComboBox );
		m_cmbPresets->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( CEdGardener::OnPresetSelected ), NULL, this );
	}

	// Load presets
	{
		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Tools/Gardener") );

		TDynArray< String > settings;
		config.GetDirectories( settings );
		for ( Uint32 i = 0; i < settings.Size(); ++i )
		{
			m_cmbPresets->Append( settings[ i ].AsChar() );
		}


		Int32    lastPresetIdx = -1;
		String lastPresetName;
		if ( config.Read( TXT("LastPreset"), &lastPresetName ) )
		{
			lastPresetIdx = m_cmbPresets->FindString( lastPresetName.AsChar() );
		}

		if ( lastPresetIdx >= 0 && lastPresetIdx < (Int32)m_cmbPresets->GetCount() )
		{
			m_cmbPresets->Select( lastPresetIdx );
			OnPresetSelected( wxCommandEvent() );
		}
	}

	SelectStamp( 1 );

	m_detachablePanel.Initialize( this, TXT( "Gardener" ) );
}

CEdGardener::~CEdGardener()
{
	if ( m_stampGhost.Get() )
	{
		m_stampGhost.Get()->Destroy();
	}

	for ( Uint32 i = 0; i < m_stampDefinitions.Size(); ++i )
	{
		m_stampDefinitions[ i ]->RemoveFromRootSet();
		delete m_dropTargets[ i ];
	}
}

void CEdGardener::OnPresetSave( wxCommandEvent& event )
{
	String presetName = m_cmbPresets->GetValue().wc_str();
	presetName.Trim();

	if ( presetName.Empty() )
	{
		wxMessageBox( TXT("Cannot save preset with no name!"), TXT("Gardener"), wxOK | wxCENTRE, m_viewport );
		return;
	}

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	for ( Uint32 i = 0; i < m_stampDefinitions.Size(); ++i )
	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Tools/Gardener/") + presetName + TXT("/Stamp") + ToString( i ) );

		CEdGardenerStamp * stamp = m_stampDefinitions[ i ];

		config.Write( TXT("Template"), stamp->m_objectTemplate.Get() != NULL ? stamp->m_objectTemplate->GetFile()->GetDepotPath() : String::EMPTY );
		config.Write( TXT("Mesh"), stamp->m_objectMesh.Get() != NULL ? stamp->m_objectMesh->GetFile()->GetDepotPath() : String::EMPTY );
		
		config.Write( TXT("YawMean"), stamp->m_yawMean );
		config.Write( TXT("YawVariance"), stamp->m_yawVariance );
		config.Write( TXT("ScaleMean"), stamp->m_scaleMean );
		config.Write( TXT("ScaleVariance"), stamp->m_scaleVariance );
		config.Write( TXT("MaxNormalSnapAngle"), stamp->m_maxNormalSnapAngle );
		config.Write( TXT("AddCollisions"), stamp->m_addCollisions ? 1 : 0 );
		config.Write( TXT("CastShadows"), stamp->m_castShadows ? 1 : 0 );
		config.Write( TXT("LocalWindSimulation"), stamp->m_localWindSimulation ? 1 : 0 );
	}

	if ( m_cmbPresets->FindString( presetName.AsChar(), true ) < 0 )
	{
		m_cmbPresets->Append( presetName.AsChar() );
	}
}

void CEdGardener::OnPresetDelete( wxCommandEvent& event )
{
	String presetName = m_cmbPresets->GetValue().wc_str();
	presetName.Trim();

	if ( presetName.Empty() )
	{
		return;
	}

	m_cmbPresets->SetValue( wxEmptyString );

	Int32 idx = m_cmbPresets->FindString( presetName.AsChar(), true );
	if ( idx > 0 )
	{
		m_cmbPresets->Delete( idx );

		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Tools/Gardener") );
		config.DeleteDirectory( presetName );
	}
}

void CEdGardener::OnPresetSelected( wxCommandEvent& event )
{
	String presetName = m_cmbPresets->GetValue().wc_str();
	presetName.Trim();

	if ( presetName.Empty() )
	{
		return;
	}

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	for ( Uint32 i = 0; i < m_stampDefinitions.Size(); ++i )
	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Tools/Gardener/") + presetName + TXT("/Stamp") + ToString( i ) );

		CEdGardenerStamp * stamp = m_stampDefinitions[ i ];

		String resPath;
		if ( config.Read( TXT("Template"), &resPath ) )
		{
			stamp->m_objectTemplate = LoadResource< CEntityTemplate >( resPath );
			stamp->m_objectMesh     = NULL;
		}
		if ( stamp->m_objectTemplate.Get() == NULL &&
			 config.Read( TXT("Mesh"), &resPath ) )
		{
			stamp->m_objectTemplate = NULL;
			stamp->m_objectMesh     = LoadResource< CMesh >( resPath );
		}
		else
		{
			stamp->m_objectTemplate = NULL;
			stamp->m_objectMesh     = NULL;
		}

		stamp->m_yawMean			= config.Read( TXT("YawMean"), 0.f );
		stamp->m_yawVariance		= config.Read( TXT("YawVariance"), 180.f );
		stamp->m_scaleMean			= config.Read( TXT("ScaleMean"), 1.f );
		stamp->m_scaleVariance		= config.Read( TXT("ScaleVariance"), 0.f );
		stamp->m_maxNormalSnapAngle	= config.Read( TXT("MaxNormalSnapAngle"), 0.f );
		stamp->m_addCollisions		= config.Read( TXT("AddCollisions"), 0 ) == 1;
		stamp->m_castShadows		= config.Read( TXT("CastShadows"), 0 ) == 1;
		stamp->m_localWindSimulation= config.Read( TXT("LocalWindSimulation"), 0 ) == 1;

		UpdateStampName( i );
	}
	UpdateStampGhost();

	config.Write( TXT("/Tools/Gardener/LastPreset"), presetName );
}

void CEdGardener::OnEraserSelected( wxCommandEvent& event )
{
	if ( m_btnEraser->GetValue() )
	{
		SelectStamp( -1 );
		m_btnEraser->SetValue( true );
	}
}

void CEdGardener::OnStampSelected( wxCommandEvent& event )
{
	Int32 idx = m_toggleButtons.GetIndex( (wxToggleButton*) event.GetEventObject() );
	ASSERT( idx >= 0 );
	SelectStamp( m_selectedStamp == idx ? -1 : idx );
}

void CEdGardener::OnStampNameSelected( wxCommandEvent& event )
{
	Int32 idx = m_meshNames.GetIndex( (wxTextCtrl*) event.GetEventObject() );
	ASSERT( idx >= 0 );
	SelectStamp( m_selectedStamp == idx ? -1 : idx );
}

void CEdGardener::OnPropertiesChanged( wxCommandEvent& event )
{
	CEdPropertiesPage::SPropertyEventData* eventData = static_cast<CEdPropertiesPage::SPropertyEventData*>( event.GetClientData() );

	ASSERT( m_selectedStamp >= 0 );

	CEdGardenerStamp * stamp = m_stampDefinitions[ m_selectedStamp ];

	if ( eventData->m_propertyName == CNAME( objectTemplate ) || eventData->m_propertyName == CNAME( objectMesh ) )
	{
		UpdateStampName( m_selectedStamp );
		UpdateStampGhost();
	}
	else
	if ( eventData->m_propertyName == CNAME( scaleMean ) || eventData->m_propertyName == CNAME( scaleVariance ) )
	{
		UpdateStampGhost();
	}
}

static EulerAngles SnapToNormal( const Vector & originalNormal, Float maxNormalSnapAngle, Float yaw )
{
	maxNormalSnapAngle = DEG2RAD( maxNormalSnapAngle );

	Vector normal = originalNormal;

	Float cosAngle = Vector::Dot3( normal, Vector::EZ );
	Float angle    = MAcos_safe( cosAngle );

	if ( MAbs( angle ) > maxNormalSnapAngle )
	{
		angle = angle > 0
			?  maxNormalSnapAngle
			: -maxNormalSnapAngle;

		Float sinAngle = MSin( angle * 0.5f );
		      cosAngle = MCos( angle * 0.5f );

		Vector quat = Vector::Cross( normal, Vector::EZ ).Normalized3();
		quat.Mul3( sinAngle );
		quat.W = cosAngle;
		Matrix mtx;
		mtx.BuildFromQuaternion( quat );
		normal = mtx.GetAxisZ();
	}

	Matrix mtx;
	mtx.V[2] = normal.Normalized3();
	mtx.V[0] = Vector::Cross( EulerAngles::YawToVector( yaw ), mtx.V[2] ).Normalized3();
	mtx.V[1] = Vector::Cross( mtx.V[2], mtx.V[0] );
	mtx.V[3] = Vector::ZERO_3D_POINT;
	return mtx.ToEulerAngles();
}

Bool CEdGardener::OnViewportTrack( const CMousePacket& packet )
{
	if ( m_stampGhost.Get() == NULL && ! m_btnEraser->GetValue() )
	{
		return false;
	}
	
	if ( m_viewport->GetWorld()->ConvertScreenToWorldCoordinates( m_viewport->GetViewport(), packet.m_x, packet.m_y, m_stampPoint, &m_stampPointNormal, m_btnHeightmapOnly->GetValue() ) )
	{
		if ( m_stampGhost.Get() )
		{
			m_stampPoint.Z += m_stampDefinitions[ m_selectedStamp ]->m_verticalShift;
			m_stampGhost.Get()->SetPosition( m_stampPoint );

			Float maxNormalSnapAngle = m_stampDefinitions[ m_selectedStamp ]->m_maxNormalSnapAngle;
			if ( maxNormalSnapAngle > 0.f )
			{
				EulerAngles rotation = SnapToNormal( m_stampPointNormal, maxNormalSnapAngle, m_yaw );
				m_stampGhost.Get()->SetRotation( rotation );
			}
		}
	}

	if ( m_erasing )
	{
		EraseStamps( false);
	}

	m_viewport->GetRenderingWindow()->GetViewport()->SetFocus();

	return true;
}

Bool CEdGardener::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if ( action == IACT_Release )
	{
		if ( key == IK_Escape || key == IK_Tilde )
		{
			SelectStamp( -1 );
			return true;
		}
		if ( key == IK_Backspace )
		{
			SelectStamp( -1 );
			m_btnEraser->SetValue( true );
			return true;
		}
		if ( key >= IK_0 && key <= IK_9 )
		{
			Uint32 stampIdx = key - IK_0;

			if ( stampIdx != m_selectedStamp )
			{
				SelectStamp( stampIdx );
			}
			else
			{
				PlaceStamp();
			}

			return true;
		}
	}
	
	if ( m_stampGhost.Get() )
	{
		if ( action == IACT_Release )
		{
			if ( key == IK_LeftMouse )
			{
				PlaceStamp();
				return true;
			}
		}
		if ( action == IACT_Axis && key == IK_MouseZ )
		{
			Bool shiftDown = RIM_IS_KEY_DOWN( IK_LShift );
			Bool ctrlDown  = RIM_IS_KEY_DOWN( IK_LControl );
			Bool altDown   = RIM_IS_KEY_DOWN( IK_Alt );

			if ( ctrlDown )
			{
				Float mod = data * (altDown
										? 1.f
										: shiftDown
											? 0.01f : 0.1f);
				m_stampGhost.Get()->SetScale( m_stampGhost.Get()->GetScale() + Vector::ONES * mod );
			}
			else
			{
				EulerAngles rotation = m_stampGhost.Get()->GetRotation();
				m_yaw += data * (altDown
										? 25.f
										: shiftDown
											? 1.f : 10.f);

				Float maxNormalSnapAngle = m_stampDefinitions[ m_selectedStamp ]->m_maxNormalSnapAngle;
				if ( maxNormalSnapAngle > 0.f )
				{
					rotation = SnapToNormal( m_stampPointNormal, maxNormalSnapAngle, m_yaw );
				}
				else
				{
					rotation.Yaw = m_yaw;
				}

				m_stampGhost.Get()->SetRotation( rotation );
			}

			m_stampGhost.Get()->ForceUpdateBoundsNode();
			m_stampBoundingBox = m_stampGhost.Get()->CalcBoundingBox() - m_stampGhost.Get()->GetWorldPosition();

			return true;
		}
	}
	else
	if ( m_btnEraser->GetValue() )
	{
		if ( key == IK_LeftMouse )
		{
			m_erasing = action == IACT_Press;
			EraseStamps( ! m_erasing );
			return true;
		}
		if ( action == IACT_Axis && key == IK_MouseZ )
		{
			Bool shiftDown = RIM_IS_KEY_DOWN( IK_LShift );
			Bool altDown   = RIM_IS_KEY_DOWN( IK_Alt );

			Float mod = data * (altDown
				? 1.f
				: shiftDown
				? 0.01f : 0.1f);
			m_eraserRadius += mod;
			return true;
		}
		if ( m_erasing && action == IACT_Axis )
		{
			return true;
		}
	}

	

	return false;
}

Bool CEdGardener::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	Float  yaw   = m_yaw;
	Vector front = EulerAngles::YawToVector( yaw );
	
	Vector center;
	Float  radius;
	if ( m_stampGhost.Get() )
	{
		center = m_stampGhost.Get()->GetWorldPosition() + m_stampBoundingBox.CalcCenter() + Vector::EZ * 0.1f;
		radius = Max( m_stampBoundingBox.Max.X - m_stampBoundingBox.Min.X, m_stampBoundingBox.Max.Y - m_stampBoundingBox.Min.Y ) * 0.5f;

		frame->AddDebugLine( center, center + front * radius, Color::RED );
	}
	else
	if ( m_btnEraser->GetValue() )
	{
		center = m_stampPoint + Vector::EZ * 0.1f;
		radius = m_eraserRadius;
	}
	else
	{
		return false;
	}
	
	DebugVertex vertices[ 16 ];
	Uint16		indices[ 32 ];
	for ( Uint32 i = 0; i < 16; ++i )
	{
		vertices[ i ].Set( center + front * radius, Color::RED );
		
		yaw += 360.f / 16.f;
		front = EulerAngles::YawToVector( yaw );

		indices[ i*2 + 0 ] = i;
		indices[ i*2 + 1 ] = ( i + 1 ) % 16;
	}

	frame->AddDebugIndexedLines( vertices, 16, indices, 32 );

	if ( m_stampGhost.Get() )
	{
		frame->AddDebugText( center + front * ( radius * 1.3f ), ToString( m_selectedStamp ), true, Color::RED );
	}

	return true;
}

void CEdGardener::UpdateStampName( Uint32 stampIdx )
{
	CEdGardenerStamp * stamp = m_stampDefinitions[ stampIdx ];

	if ( stamp->m_objectTemplate.Get() != NULL )
	{
		m_meshNames[ stampIdx ]->SetLabel( stamp->m_objectTemplate->GetFile()->GetFileName().AsChar() );
	}
	else if ( stamp->m_objectMesh.Get() != NULL )
	{
		m_meshNames[ stampIdx ]->SetLabel( stamp->m_objectMesh->GetFile()->GetFileName().AsChar() );
	}
	else
	{
		m_meshNames[ stampIdx ]->SetLabel( TXT("[no mesh defined]") );
	}

	m_meshNames[ stampIdx ]->GetParent()->Layout();
}

void CEdGardener::UpdateStampGhost()
{
	if ( m_stampGhost.Get() )
	{
		m_stampGhost.Get()->Destroy();
		m_stampGhost = NULL;
	}

	if ( m_selectedStamp < 0 )
	{
		return;
	}

	CEdGardenerStamp * stamp = m_stampDefinitions[ m_selectedStamp ];

	EntitySpawnInfo spawnInfo;
	spawnInfo.m_spawnPosition		= m_stampPoint;
	spawnInfo.m_spawnScale			= Vector::ONES * ( stamp->m_scaleMean + GEngine->GetRandomNumberGenerator().Get< Float >( -stamp->m_scaleVariance , stamp->m_scaleVariance ) );
	spawnInfo.m_spawnRotation.Yaw	= stamp->m_yawMean + GEngine->GetRandomNumberGenerator().Get< Float >( -stamp->m_yawVariance , stamp->m_yawVariance );
	m_yaw = spawnInfo.m_spawnRotation.Yaw;

	if ( stamp->m_objectTemplate.Get() != NULL )
	{
		spawnInfo.m_template = stamp->m_objectTemplate;

		m_stampGhost = m_viewport->GetWorld()->GetDynamicLayer()->CreateEntitySync( spawnInfo );
	}
	else if ( stamp->m_objectMesh.Get() != NULL )
	{
		m_stampGhost = m_viewport->GetWorld()->GetDynamicLayer()->CreateEntitySync( spawnInfo );

		CMeshComponent * component = Cast< CMeshComponent >( m_stampGhost.Get()->CreateComponent( CMeshComponent::GetStaticClass(), SComponentSpawnInfo() ) );
		component->SetResource( stamp->m_objectMesh.Get() );
	}

	if ( m_stampGhost.Get() )
	{
		m_viewport->GetWorld()->DelayedActions();
		//m_stampGhost->ForceUpdateBounds();
		m_stampBoundingBox = m_stampGhost.Get()->CalcBoundingBox() - m_stampGhost.Get()->GetWorldPosition();
	}
}

void CEdGardener::SelectStamp( Int32 stampIdx )
{
	m_btnEraser->SetValue( false );

	if ( m_selectedStamp != stampIdx )
	{
		m_selectedStamp = stampIdx;
		if ( m_selectedStamp == -1 )
		{
			m_properties->Get().SetNoObject();
		}

		for ( Uint32 i = 0; i < m_toggleButtons.Size(); ++i )
		{
			if ( i != stampIdx )
			{
				m_toggleButtons[ i ]->SetValue( false );
			}
			else
			{
				m_toggleButtons[ i ]->SetValue( true );
				m_properties->Get().SetObject( m_stampDefinitions[ i ] );
				m_viewport->GetWorld()->GetSelectionManager()->DeselectAll();
			}
		}
	}

	UpdateStampGhost();
}

void CEdGardener::PlaceStamp()
{
	CEntity *stampGhost = m_stampGhost.Get();

	if ( stampGhost == NULL )
		return;

	// Get selected layer
	CLayer *layer = wxTheFrame->GetSceneExplorer()->GetActiveLayer();
	if ( layer == NULL )
	{
		wxMessageBox( TXT("Cannot stamp, no layer activated!"), TXT("Gardener"), wxOK | wxCENTRE, m_viewport );
		return;
	}

	if ( ! layer->MarkModified() )
	{
		return;
	}

	CEdGardenerStamp * stamp = m_stampDefinitions[ m_selectedStamp ];

	EntitySpawnInfo spawnInfo;
	spawnInfo.m_spawnScale    = stampGhost->GetScale();
	spawnInfo.m_spawnRotation = stampGhost->GetRotation();
	spawnInfo.m_spawnPosition = stampGhost->GetPosition();
	spawnInfo.m_entityFlags = EF_CreatedByGardenerTool;

	CEntity * entity = NULL;
	if ( stamp->m_objectTemplate.Get() != NULL )
	{
		spawnInfo.m_template = stamp->m_objectTemplate;

		entity = layer->CreateEntitySync( spawnInfo );
	}
	else if ( stamp->m_objectMesh.Get() != NULL )
	{
		entity = layer->CreateEntitySync( spawnInfo );

		CClass* meshClass = stamp->m_addCollisions ? ClassID< CStaticMeshComponent >() : ClassID< CMeshComponent >();
		CMeshComponent * component = Cast< CMeshComponent >( entity->CreateComponent( meshClass, SComponentSpawnInfo() ) );

		component->SetCastingShadows( stamp->m_castShadows );
		component->SetLocalWindSimulation( stamp->m_localWindSimulation );
		component->SetResource( stamp->m_objectMesh.Get() );
	}

	if ( entity != NULL )
	{
		CUndoCreateDestroy::CreateStep( m_viewport->GetUndoManager(), entity, true );
		CUndoCreateDestroy::FinishStep( m_viewport->GetUndoManager() );
	
		if ( m_btnRandomStamp->GetValue() )
		{
			Uint32 nextStampIdx = GEngine->GetRandomNumberGenerator().Get< Uint32 >( 10 );
			while ( m_stampDefinitions[ nextStampIdx ]->m_objectTemplate.Get() == NULL && m_stampDefinitions[ nextStampIdx ]->m_objectMesh.Get() == NULL )
			{
				nextStampIdx = GEngine->GetRandomNumberGenerator().Get< Uint32 >( 10 );
			}
			SelectStamp( nextStampIdx );
		}
		else
		{
			UpdateStampGhost();
		}
	}
}

void CEdGardener::EraseStamps( Bool finish )
{
	// Get selected layer
	CLayer *layer = wxTheFrame->GetSceneExplorer()->GetActiveLayer();
	if ( layer == NULL )
	{
		//wxMessageBox( TXT("Cannot stamp, no layer activated!"), TXT("Gardener"), wxOK | wxCENTRE, m_viewport );
		return;
	}

	if ( ! layer->MarkModified() )
	{
		return;
	}

	Bool erased = false;
	TDynArray< CEntity* > entities;
	layer->GetEntities( entities );
	for ( Uint32 i = 0; i < entities.Size(); ++i )
	{
		CEntity * entity = entities[ i ];
		if ( entity->CheckEntityFlag( EF_CreatedByGardenerTool ) )
		{
			//if ( entity->CalcBoundingBox().Distance( m_stampPoint ) <= m_eraserRadius )
			if ( ( entity->GetWorldPosition() - m_stampPoint ).SquareMag2() <= m_eraserRadius * m_eraserRadius )
			{
				//entity->Destroy();
				m_erased = true;
				CUndoCreateDestroy::CreateStep( m_viewport->GetUndoManager(), entity, false );
			}
		}
	}

	if ( finish && m_erased )
	{
		CUndoCreateDestroy::FinishStep( m_viewport->GetUndoManager() );
		m_erased = false;
	}
}

//////////////////////////////////////////////////////////////////////////
// Drag & Drop
//////////////////////////////////////////////////////////////////////////
Bool CGardenerDropTarget::OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources )
{
	for ( Uint32 i = 0; i < resources.Size(); ++i )
	{
		if ( resources[ i ]->IsA< CMesh >() )
		{
			m_gardener->GetStamp( m_stampIdx )->m_objectMesh     = Cast< CMesh >( resources[ i ] );
			m_gardener->GetStamp( m_stampIdx )->m_objectTemplate = NULL;
			m_gardener->UpdateStampName( m_stampIdx );
			m_gardener->SelectStamp( -1 );
			m_gardener->SelectStamp( m_stampIdx );
			return true;
		}
		else if ( resources[ i ]->IsA< CEntityTemplate >() )
		{
			m_gardener->GetStamp( m_stampIdx )->m_objectMesh     = NULL;
			m_gardener->GetStamp( m_stampIdx )->m_objectTemplate = Cast< CEntityTemplate >( resources[ i ] );
			m_gardener->UpdateStampName( m_stampIdx );
			m_gardener->SelectStamp( -1 );
			m_gardener->SelectStamp( m_stampIdx );
			return true;
		}
	}
	return false;
}

wxDragResult CGardenerDropTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
	const TDynArray< CResource* > & resources = this->GetDraggedResources();

	for ( Uint32 i = 0; i < resources.Size(); ++i )
	{
		if ( resources[ i ]->IsA< CMesh >() || resources[ i ]->IsA< CEntityTemplate >() )
		{
			return wxDragCopy;
		}
	}

	return wxDragNone;
}