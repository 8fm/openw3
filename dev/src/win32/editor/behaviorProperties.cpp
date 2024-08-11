/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/behaviorGraphInstance.h"

#include "../../common/game/actor.h"
#include "../../common/game/inventoryComponent.h"

#include "behaviorEditor.h"
#include "behaviorProperties.h"

BEGIN_EVENT_TABLE( CEdBehaviorEditorProperties, CEdPropertiesPage )
END_EVENT_TABLE()

CEdBehaviorEditorProperties::CEdBehaviorEditorProperties( CEdBehaviorEditor* editor, const PropertiesPageSettings& settings, CEdUndoManager* undoManager )
	: CEdPropertiesPage( editor, settings, undoManager )
	, CEdBehaviorEditorPanel( editor )
{

}

wxAuiPaneInfo CEdBehaviorEditorProperties::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.CloseButton( false ).Left().LeftDockable( true ).MinSize(100,300).BestSize( 300, 600 ).Position( 2 );

	return info;
}

void CEdBehaviorEditorProperties::PropertyPreChange( IProperty* property, STypedObject object )
{
	GetEditor()->UseBehaviorInstance( false );

	CEdPropertiesPage::PropertyPreChange( property, object );
}

void CEdBehaviorEditorProperties::PropertyPostChange( IProperty* property, STypedObject object )
{
	GetEditor()->BehaviorGraphModified();

	CEdPropertiesPage::PropertyPostChange( property, object );
}

void CEdBehaviorEditorProperties::OnReset()
{
	SetNoObject();
}

void CEdBehaviorEditorProperties::OnDebug( Bool flag )
{
	if ( GetRootItem() )
	{
		wxCommandEvent event;
		GetRootItem()->OnExpandAll( event );
	}

	SetReadOnly( flag );
}

void CEdBehaviorEditorProperties::OnNodesSelect( const TDynArray< CGraphBlock* >& nodes )
{
	SetObjects( (TDynArray< CObject* >&)nodes );
}

void CEdBehaviorEditorProperties::OnNodesDeselect()
{
	SetObject( GetBehaviorGraph() );
}

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CEdInstancePropertiesPage, wxGrid )
END_EVENT_TABLE()

CEdInstancePropertiesPage::CEdInstancePropertiesPage( wxWindow* parent )
	: wxGrid( parent, -1 )
{
	CreateGrid( 10, 10 );

	EnableEditing( false );

	SetRowLabelSize( 0 );
	SetColLabelSize( 0 );

	EnableDragRowSize( false );
	EnableDragColMove( false );

	EnableScrolling( false, true );
}

void CEdInstancePropertiesPage::ClearPage()
{
	if ( GetNumberCols() > 0 ) DeleteCols( 0, GetNumberCols(), false );
	if ( GetNumberRows() > 0 ) DeleteRows( 0, GetNumberRows(), false );
}

void CEdInstancePropertiesPage::RefreshPage( CInstancePropertiesBuilder& properties )
{
	// Temp - very slow

	Freeze();
	ClearPage();
	Thaw();

	FillPage( properties );
}

void CEdInstancePropertiesPage::FillPage( CInstancePropertiesBuilder& properties )
{
	Freeze();

	if ( GetNumberCols() < 2 ) AppendCols( 2 );

	AppendRows( properties.m_data.Size() );

	wxFont boldFont = GetDefaultCellFont();
	boldFont.SetWeight( wxFONTWEIGHT_BOLD );

	for ( Uint32 i=0; i<properties.m_data.Size(); ++i )
	{
		const String& value = properties.m_data[i].m_second;

		if ( properties.m_data[i].m_first.Empty() )
		{
			String className = TXT(" ") + properties.m_data[i].m_second;

			SetCellSize( i, 0, 1, 2 );
			SetCellValue( i, 0, className.AsChar() );
			SetCellBackgroundColour( i, 0, *wxLIGHT_GREY );
			SetCellTextColour( i, 0, wxColour( 128, 128, 128 ) );
			SetCellFont( i, 0, boldFont );
		}
		else
		{
			const String& propNameStr = properties.m_data[i].m_first;
			const String& propVar = properties.m_data[i].m_second;

			String propName = TXT("   ") + propNameStr;

			SetCellValue( i, 0, propName.AsChar() );
			SetCellValue( i, 1, propVar.AsChar() );
		}
	}

	Fit();

	Thaw();
}

Bool CEdInstancePropertiesPage::IsClass( const CVariant& variant ) const
{
	return variant.GetRTTIType() == NULL ? true : false;
}

Bool CEdInstancePropertiesPage::IsArray( const CVariant& variant ) const
{
	return variant.IsArray();
}

Bool CEdInstancePropertiesPage::ParseVariant( const CVariant &variant, String &out ) const
{
	Bool ret = variant.ToString( out );
	out = TXT(" ") + out;
	return ret;
}

Bool CEdInstancePropertiesPage::ParseVariant( const CVariantArray &variant, String &out ) const
{
	/*return variant.ToString( out );

	String propName;
	if ( name.BeginsWith( TXT("i_") ) )
	{
		propName = name.RightString( name.GetLength() - 2 );
	}
	else
	{
		propName = name;
	}

	m_data.PushBack( tDesc( propName, valueStr ) );

	if ( variant.IsArray() )
	{
		CVariantArray theArray( variant );

		Uint32 size = theArray.Size();

		for ( Uint32 i=0; i<size; ++i )
		{
			CVariant elem;
			theArray.Get( i, elem );
			String desc = String::Printf( TXT("%d"),i );
			String valueStr;
			elem.ToString( valueStr );
			m_data.PushBack( tDesc( desc, valueStr ) );
		}
	}*/

	return false;
}

//////////////////////////////////////////////////////////////////////////

#define ID_CONNECT			6001

BEGIN_EVENT_TABLE( CEdBehaviorEditorRuntimeProperties, CEdBehaviorEditorSimplePanel )
END_EVENT_TABLE()

CEdBehaviorEditorRuntimeProperties::CEdBehaviorEditorRuntimeProperties( CEdBehaviorEditor* editor )
	: CEdBehaviorEditorSimplePanel( editor )
	, m_connected( false )
	, m_selectedNode( NULL )
{
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

	// Tool bar
	wxToolBar* toolbar = new wxToolBar( this, -1 );

	toolbar->AddTool( ID_CONNECT, wxT("Connect"), SEdResources::GetInstance().LoadBitmap(_T("IMG_CONNECT")), wxT("Connect"), wxITEM_CHECK );
	toolbar->Connect( ID_CONNECT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorEditorRuntimeProperties::OnConnect ), NULL, this );
	toolbar->Realize();

	sizer->Add( toolbar, 0, wxEXPAND|wxALL, 0 );

	// Properties page
	m_properties = new CEdInstancePropertiesPage( this );
	sizer->Add( m_properties, 1, wxEXPAND|wxALL, 0 );

	SetAutoLayout( true );
	SetSizer( sizer );

	sizer->Fit( this );

	Layout();
}

wxAuiPaneInfo CEdBehaviorEditorRuntimeProperties::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.CloseButton( true ).Left().LeftDockable( true ).MinSize(100,300).BestSize( 300, 600 ).Position( 3 );

	return info;
}

void CEdBehaviorEditorRuntimeProperties::OnConnect( wxCommandEvent& event )
{
	m_connected = event.IsChecked();
}

void CEdBehaviorEditorRuntimeProperties::OnReset()
{
	OnNodesDeselect();
}

void CEdBehaviorEditorRuntimeProperties::OnNodesSelect( const TDynArray< CGraphBlock* >& nodes )
{
	m_properties->ClearPage();

	m_selectedNode = NULL;

	if ( nodes.Size() > 0 )
	{
		m_selectedNode = SafeCast< CBehaviorGraphNode > ( nodes[0] );
		FillProperties();
	}
}

void CEdBehaviorEditorRuntimeProperties::OnNodesDeselect()
{
	m_selectedNode = NULL;
	m_properties->ClearPage();
	Refresh();
}

void CEdBehaviorEditorRuntimeProperties::OnTick( Float dt )
{
	if ( m_connected && m_selectedNode )
	{
		RefreshProperties();
	}
}

void CEdBehaviorEditorRuntimeProperties::FillProperties()
{
	if ( m_connected )
	{
		CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();

		InstanceBuffer& instanceBuffer = instance->GetInstanceBuffer();
		CInstancePropertiesBuilder builder;

		ASSERT( m_selectedNode );
		if ( m_selectedNode )
		{
			m_selectedNode->OnBuildInstanceProperites( instanceBuffer, builder );
		}

		m_properties->FillPage( builder );
	}
}

void CEdBehaviorEditorRuntimeProperties::RefreshProperties()
{
	if ( m_connected )
	{
		CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();

		InstanceBuffer& instanceBuffer = instance->GetInstanceBuffer();
		CInstancePropertiesBuilder builder;

		ASSERT( m_selectedNode );
		m_selectedNode->OnBuildInstanceProperites( instanceBuffer, builder );

		m_properties->RefreshPage( builder );
	}
}
