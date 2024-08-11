/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialEditor.h"
#include "shortcutsEditor.h"
#include "shaderView.h"
#include "assetBrowser.h"
#include "..\..\common\matcompiler\materialCompiler.h"
#include "..\..\common\matcompiler\stringPrinter.h"
#include "..\..\common\matcompiler\materialShaderCompiler.h"
#include "..\..\common\matcompiler\materialPixelShaderCompiler.h"
#include "..\..\common\matcompiler\materialVertexShaderCompiler.h"
#include "../../common/core/dependencyMapper.h"
#include "../../common/engine/renderFragment.h"
#include "../../common/engine/drawableComponent.h"
#include "../../common/engine/materialBlockCompiler.h"

// Event table
BEGIN_EVENT_TABLE( CEdMaterialEditor, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "editCut" ), CEdMaterialEditor::OnEditCut )
	EVT_MENU( XRCID( "editCopy" ), CEdMaterialEditor::OnEditCopy )
	EVT_MENU( XRCID( "editPaste" ), CEdMaterialEditor::OnEditPaste )
	EVT_MENU( XRCID( "editDelete" ), CEdMaterialEditor::OnEditDelete )
	EVT_MENU( XRCID( "editUndo" ), CEdMaterialEditor::OnEditUndo )
	EVT_MENU( XRCID( "editRedo" ), CEdMaterialEditor::OnEditRedo )
	EVT_MENU( XRCID( "materialSave" ), CEdMaterialEditor::OnSave )
	EVT_MENU( XRCID( "materialShaderView"), CEdMaterialEditor::OnShaderView )
	EVT_BUTTON( XRCID( "btnUpdateProxies"), CEdMaterialEditor::OnUpdateProxies )
END_EVENT_TABLE()

//RED_DEFINE_STATIC_NAME( FileReloadToConfirm )
//RED_DEFINE_STATIC_NAME( FileReloadConfirm )
//RED_DEFINE_STATIC_NAME( FileReload )

CEdMaterialEditor::CEdMaterialEditor( wxWindow* parent, CMaterialGraph* graph )
	: wxSmartLayoutPanel( parent, TXT("MaterialEditor"), false )
	, m_graph( graph )
{
	GetOriginalFrame()->Bind( wxEVT_ACTIVATE, &CEdMaterialEditor::OnActivate, this );

	// Keep reference to resource as long as editor is opened
	m_graph->AddToRootSet();

	// Set title
	String title = GetTitle().wc_str();
	SetTitle( String::Printf( TEXT("%s - %s"), title.AsChar(), m_graph->GetFriendlyName().AsChar() ).AsChar() );

	// Set icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( _T("IMG_MATERIALS") ) );
	SetIcon( iconSmall );

	// Create rendering panel
	{
		wxPanel* rp = XRCCTRL( *this, "PreviewPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_preview = new CEdMaterialPreviewPanel( rp );
		m_preview->RefreshPreviewVisibility( m_graph != nullptr && m_graph->HasAnyBlockParameters() );
		m_preview->SetMaterial( m_graph );
		sizer1->Add( m_preview, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Undo manager
	{
		m_undoManager = new CEdUndoManager( GetOriginalFrame() );
		m_undoManager->AddToRootSet();
		m_undoManager->SetMenuItems( GetMenuBar()->FindItem( XRCID( "editUndo" ) ), GetMenuBar()->FindItem( XRCID( "editRedo" ) ) );
	}

	// Create properties
	{
		wxPanel* rp = XRCCTRL( *this, "PropertiesPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		m_properties = new CEdPropertiesBrowserWithStatusbar( rp, settings, m_undoManager );
		m_properties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdMaterialEditor::OnPropertiesChanged ), NULL, this );
		sizer1->Add( m_properties, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Graph editor
	{
		wxPanel* rp = XRCCTRL( *this, "GraphPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_graphEditor = new CEdMaterialGraphEditor( rp );
		sizer1->Add( m_graphEditor, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Code editor
	{
		wxPanel* cvp = XRCCTRL( *this, "CodeViewPanel", wxPanel );
		m_blockGeneratedCodeViewPS = XRCCTRL( *this, "PSCodeView1", wxTextCtrl );
		m_fullShaderViewPS = XRCCTRL( *this, "PSCodeView2", wxTextCtrl );
		m_blockGeneratedCodeViewVS = XRCCTRL( *this, "VSCodeView1", wxTextCtrl );
		m_fullShaderViewVS = XRCCTRL( *this, "VSCodeView2", wxTextCtrl );
		m_shaderPassChoice = XRCCTRL( *this, "shaderPassChoice", wxChoice );

		static_assert( RP_Max == 11, "RenderPasses changed, list below needs to be updated" );

		m_shaderPassChoice->Append( "RP_HitProxies"					);
		m_shaderPassChoice->Append( "RP_NoLighting"					);
		m_shaderPassChoice->Append( "RP_ShadowDepthSolid"			);
		m_shaderPassChoice->Append( "RP_ShadowDepthMasked"			);
		m_shaderPassChoice->Append( "RP_Emissive"					);
		m_shaderPassChoice->Append( "RP_GBuffer"					);
		m_shaderPassChoice->Append( "RP_RefractionDelta"			);
		m_shaderPassChoice->Append( "RP_ReflectionMask"				);
		m_shaderPassChoice->Append( "RP_ForwardLightingSolid"		);
		m_shaderPassChoice->Append( "RP_ForwardLightingTransparent"	);
		m_shaderPassChoice->Append( "RP_HiResShadowMask"			);

		m_shaderPassChoice->SetSelection( 5 );

		cvp->Layout();
	}

	// Bind properties editor
	//m_properties->SetHook( this );

	// Bind graph
	m_graphEditor->SetGraph( m_graph );
	m_graphEditor->SetHook( this );
	m_graphEditor->SetUndoManager( m_undoManager );

	// This is probably one of the oldest not working functionalities 
	// in the system so I'm keeping this here as a 
	// reminder of how things are actually done in this company
	// ANCIENT CODE START

	// Select shader
	//m_preview->setShader( m_Shader );

	// ANCIENT CODE END

	SEvents::GetInstance().RegisterListener( CNAME( FileReload ), this );
	SEvents::GetInstance().RegisterListener( CNAME( FileReloadConfirm ), this );

	// Update and finalize layout
	GetParent()->Layout();
	Layout();
	LoadOptionsFromConfig();
	Show();
}

CEdMaterialEditor::~CEdMaterialEditor()
{
	SaveOptionsToConfig();

	m_undoManager->RemoveFromRootSet();
	m_undoManager->Discard();

	m_graph->RemoveFromRootSet();

	SEvents::GetInstance().UnregisterListener( this );
}

void CEdMaterialEditor::SaveOptionsToConfig()
{
	SaveLayout(TXT("/Frames/MaterialEditor"));
}

void CEdMaterialEditor::LoadOptionsFromConfig()
{
	CEdShortcutsEditor::Load(*this->GetMenuBar(), GetOriginalLabel());
	// Load layout after the shortcuts (duplicate menu after the shortcuts loading)
	LoadLayout(TXT("/Frames/MaterialEditor"));
}

void CEdMaterialEditor::OnGraphStructureModified( IGraphContainer* graph )
{
	if ( m_preview )
	{
		m_preview->RefreshPreviewVisibility( m_graph != nullptr && m_graph->HasAnyBlockParameters() );
	}
}

void CEdMaterialEditor::OnGraphSelectionChanged()
{
	m_blockGeneratedCodeViewPS->Clear();
	m_fullShaderViewPS->Clear();
	m_blockGeneratedCodeViewVS->Clear();
	m_fullShaderViewVS->Clear();

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	// Grab objects
	TDynArray< CGraphBlock* > blocks;
	m_graphEditor->GetSelectedBlocks( blocks );

	// Select them at properties browser
	if ( blocks.Size() )
	{
		if (blocks.Size() == 1 && blocks[0]->IsA< CMaterialBlock >() )
		{
			CRenderCamera compilationCamera;
			RenderingContext rcontext(compilationCamera);
			rcontext.m_pass = (ERenderingPass) m_shaderPassChoice->GetSelection();

			MaterialRenderingContext context(rcontext);
			context.m_vertexFactory = m_graph->IsTerrainMaterial() ? MVF_TesselatedTerrain : MVF_MeshStatic;
			context.m_selected = false;

			CHLSLMaterialCompiler hlslCompiler( context, PLATFORM_PC );
			hlslCompiler.GenerateDefaultCode();
			CMaterialBlockCompiler* blockCompiler = new CMaterialBlockCompiler( m_graph, &hlslCompiler);
			CodeChunk chunk = ((CMaterialBlock*)blocks[0])->Compile(*blockCompiler, MSH_PixelShader);
			
			// Sort interpolators (so they don't depend on the graph structure)
			hlslCompiler.SortAndOutputInterpolators();

			const AnsiChar* codeStringPS = hlslCompiler.m_pixelShader->m_code.AsChar();
			m_blockGeneratedCodeViewPS->AppendText( wxString(codeStringPS) );

			const AnsiChar* codeStringVS = hlslCompiler.m_vertexShader->m_code.AsChar();
			m_blockGeneratedCodeViewVS->AppendText( wxString(codeStringVS) );

			hlslCompiler.CreateDataConnections();

			CStringPrinter psCode;
			hlslCompiler.m_pixelShader->GetFinalCode(psCode);
			const AnsiChar* fullShaderStringPS = psCode.AsChar();
			m_fullShaderViewPS->AppendText( wxString(fullShaderStringPS) );

			CStringPrinter vsCode;
			hlslCompiler.m_vertexShader->GetFinalCode(vsCode);
			const AnsiChar* fullShaderStringVS = vsCode.AsChar();
			m_fullShaderViewVS->AppendText( wxString(fullShaderStringVS) );
		}

		// Show blocks properties
		m_properties->Get().SetObjects( (TDynArray< CObject* >&) blocks );
	}
	else
	{
		CRenderCamera compilationCamera;
		RenderingContext rcontext(compilationCamera);
		rcontext.m_pass = (ERenderingPass) m_shaderPassChoice->GetSelection();

		MaterialRenderingContext context(rcontext);
		context.m_vertexFactory = m_graph->IsTerrainMaterial() ? MVF_TesselatedTerrain : MVF_MeshStatic;
		context.m_selected = false;

		CHLSLMaterialCompiler hlslCompiler( context, PLATFORM_PC );

		hlslCompiler.GenerateDefaultCode();

		CMaterialBlockCompiler* blockCompiler = new CMaterialBlockCompiler( m_graph, &hlslCompiler);
		blockCompiler->CompileRoots();
		const AnsiChar* codeStringPS = hlslCompiler.m_pixelShader->m_code.AsChar();
		m_blockGeneratedCodeViewPS->AppendText( wxString(codeStringPS) );
		const AnsiChar* codeStringVS = hlslCompiler.m_vertexShader->m_code.AsChar();
		m_blockGeneratedCodeViewVS->AppendText( wxString(codeStringVS) );

		// Sort interpolators (so they don't depend on the graph structure)
		hlslCompiler.SortAndOutputInterpolators();

		hlslCompiler.CreateDataConnections();

		CStringPrinter psCode;
		hlslCompiler.m_pixelShader->GetFinalCode(psCode);
		const AnsiChar* fullShaderStringPS = psCode.AsChar();
		m_fullShaderViewPS->AppendText( wxString(fullShaderStringPS) );

		CStringPrinter vsCode;
		hlslCompiler.m_vertexShader->GetFinalCode(vsCode);
		const AnsiChar* fullShaderStringVS = vsCode.AsChar();
		m_fullShaderViewVS->AppendText( wxString(fullShaderStringVS) );

		// Show material properties
		m_properties->Get().SetObject( m_graph );
	}
#endif // NO_RUNTIME_MATERIAL_COMPILATION
}

void CEdMaterialEditor::OnPropertiesChanged( wxCommandEvent& event )
{
	m_graphEditor->Repaint( false );
}

void CEdMaterialEditor::OnSave( wxCommandEvent& event )
{
	m_graph->Save();
}

void CEdMaterialEditor::OnEditCopy( wxCommandEvent& event )
{
	m_graphEditor->CopySelection();
}

void CEdMaterialEditor::OnEditCut( wxCommandEvent& event )
{
	m_graphEditor->CutSelection();
}

void CEdMaterialEditor::OnEditPaste( wxCommandEvent& event )
{
	m_graphEditor->Paste( NULL );
}

void CEdMaterialEditor::OnEditDelete( wxCommandEvent& event )
{
	m_graphEditor->DeleteSelection();
}

void CEdMaterialEditor::OnEditUndo( wxCommandEvent& event )
{
	m_undoManager->Undo();
}

void CEdMaterialEditor::OnEditRedo( wxCommandEvent& event )
{
	m_undoManager->Redo();
}

void CEdMaterialEditor::OnShaderView (wxCommandEvent& event )
{
	//get shader source code, and display it in read-only frame:
	CShaderView* shaderView = new CShaderView(this, m_graph);
	shaderView->Show();
}

void CEdMaterialEditor::OnUpdateProxies ( wxCommandEvent& event )
{
	if ( m_graph )
	{
		m_graph->ForceRecompilation();
	}

	CDrawableComponent::RecreateProxiesOfRenderableComponents();
}

void CEdMaterialEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( FileReloadConfirm ) )
	{
		CResource* res = GetEventData< CResource* >( data );
		if ( res == m_graph )
		{
			SEvents::GetInstance().QueueEvent( CNAME( FileReloadToConfirm ), CreateEventData( CReloadFileInfo( res, NULL, GetTitle().wc_str() ) ) );
		}
	}
	else if ( name == CNAME( FileReload ) )
	{
		const CReloadFileInfo& reloadInfo = GetEventData< CReloadFileInfo >( data );

		if ( reloadInfo.m_newResource->IsA<CMaterialGraph>() )
		{
			CMaterialGraph* oldMatGraph = (CMaterialGraph*)(reloadInfo.m_oldResource);
			CMaterialGraph* newMatGraph = (CMaterialGraph*)(reloadInfo.m_newResource);
			if ( oldMatGraph == m_graph )
			{
				m_graph = newMatGraph;
				m_graph->AddToRootSet();

				m_preview->Reload();

				m_graphEditor->SetGraph( m_graph );

				OnGraphSelectionChanged();

				wxTheFrame->GetAssetBrowser()->OnEditorReload(m_graph, this);
			}
		}
	}
}

void CEdMaterialEditor::OnActivate( wxActivateEvent& event )
{
	if ( event.GetActive() )
	{
		if ( wxTheFrame )
		{
			wxTheFrame->SetUndoHistoryFrameManager( m_undoManager, String( GetTitle().c_str() ) );
		}
	}

	event.Skip();
}
