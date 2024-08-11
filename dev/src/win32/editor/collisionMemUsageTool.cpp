/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "worldTreeTraverser.h"
#include "collisionObjSizeCalc.h"
#include "../../common/engine/mesh.h"
#include "../../common/core/diskFile.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/layerGroup.h"
#include "../../common/engine/staticMeshComponent.h"
#include "../../common/engine/collisionShape.h"

// Image ids
const Int32 GWorldImageId				= 0;
const Int32 GLayerGroupImageId		= 1;
const Int32 GLayerInfoImageId			= 2;
const Int32 GEntityImageId			= 3;
const Int32 GComponentImageId			= 4;
const Int32 GResourceImageId			= 5;
const Int32 GCollisionShapeImageId	= 6;

// Column indices
const Uint32 GTreeColumnId	= 0;
const Uint32 GMultColumnId	= 1;
const Uint32 GLayerColumnId	= 2;
const Uint32 GMemoryColumnId	= 3;

// UI sizing
const Int32 GTreeColumnWidth		= 300;
const Int32 GMultColumnWidth		= 20;
const Int32 GLayerColumnWidth		= 120;
const Int32 GMemoryColumnWidth	= 120;

// String resources used by this widget - TreeList widget resources
const String GTreeColumnName	= TEXT( "Tree" );
const String GMultColumnName	= TEXT( "N" );
const String GLayerColumnName	= TEXT( "Layer" );
const String GMemoryColumnName	= TEXT( "Memory" );

// String resources used by this widget - global
const String GMeshesNodeName	= TEXT( "Unique meshes" );


BEGIN_EVENT_TABLE( CCollisionMemUsageTool, wxSmartLayoutPanel )
EVT_TREE_ITEM_ACTIVATED( XRCID("CollisionObjectsTree"), CCollisionMemUsageTool::OnTreeItemActivated  )
EVT_CHOICE( XRCID("memUnitsCB"), CCollisionMemUsageTool::OnMemUnitChanged )
EVT_TOOL( XRCID("toolRefresh"), CCollisionMemUsageTool::OnRefreshContent )
END_EVENT_TABLE()


using namespace CollisionMem;

//** *******************************
//
CCollisionMemUsageTool::CCollisionMemUsageTool	( wxWindow * parent ) 
	: m_pCollisionObjectsTree( NULL )
	, wxSmartLayoutPanel( parent, TEXT( "CollisionMemUsageTool" ), false )
	, m_BMenuItemId( XRCID( "menuItemB" ) )
	, m_KBMenuItemId( XRCID( "menuItemKB" ) )
	, m_MBMenuItemId( XRCID( "menuItemMB" ) )
{
	// Load designed frame from resource
	wxXmlResource::Get()->LoadPanel( this, parent, TEXT("CollisionMemUsageTool") );

	m_pMenuBar			= GetMenuBar();
	m_pUnitMenu			= m_pMenuBar->GetMenu( 0 );
	m_pMemUnitsCB		= XRCCTRL( *this, "memUnitsCB", wxChoice );
	m_pTotalMemLabel	= XRCCTRL( *this, "totalMemoryLabel", wxStaticText );

	Connect( XRCID( "menuItemB" ), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CCollisionMemUsageTool::OnSelectB ), NULL, this );
	Connect( XRCID( "menuItemKB" ), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CCollisionMemUsageTool::OnSelectKB ), NULL, this );
	Connect( XRCID( "menuItemMB" ), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CCollisionMemUsageTool::OnSelectMB ), NULL, this );

	SetMemUnits( MU_KBytes );

	InitializeTreeViewRes();
	InitializeToolsRes();

	Layout();

	ReReadAndRefreshAll();

	Show();
}

//** *******************************
//
void CCollisionMemUsageTool::InitializeTreeViewRes	()
{
	// Collision Objects Tree
	m_pCollisionObjectsTree = XRCCTRL( *this, "CollisionObjectsTree", wxTreeListCtrl );

	m_pCollisionObjectsTree->SetWindowStyle( m_pCollisionObjectsTree->GetWindowStyle() | wxTR_LINES_AT_ROOT | wxTR_EDIT_LABELS | wxTR_MULTIPLE );

	m_pCollisionObjectsTree->AddColumn( GTreeColumnName.AsChar(), GTreeColumnWidth );
	m_pCollisionObjectsTree->AddColumn( GMultColumnName.AsChar(), GMultColumnWidth );
	m_pCollisionObjectsTree->AddColumn( GLayerColumnName.AsChar(), GLayerColumnWidth );
	m_pCollisionObjectsTree->AddColumn( GMemoryColumnName.AsChar(), GMemoryColumnWidth );

	//Images
	wxImageList * pImages = new wxImageList( 16, 16, true, 2 );

	pImages->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_WORLD16") ) );
	pImages->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_EYE") ) );
	pImages->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_EYE_NO") ) );
	pImages->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_ENTITY") ) );
	pImages->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_COMPONENT") ) );
	pImages->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_RESOURCE_INTERNAL") ) );
	pImages->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_RESOURCE_EXTERNAL") ) );

	m_pCollisionObjectsTree->AssignImageList( pImages );
}

//** *******************************
//
void CCollisionMemUsageTool::InitializeToolsRes		()
{
	m_pMemUnitsCB->Freeze();
	m_pMemUnitsCB->Clear();

	m_pMemUnitsCB->AppendString( wxString( TEXT("Bytes" ) ) );
	m_pMemUnitsCB->AppendString( wxString( TEXT("KBytes" ) ) );
	m_pMemUnitsCB->AppendString( wxString( TEXT("MBytes" ) ) );

	m_pMemUnitsCB->SetSelection( CCollisionMemUsageTool::MU_KBytes );

	m_pMemUnitsCB->Thaw();
}

//** *******************************
//
void			CCollisionMemUsageTool::ReReadAndRefreshAll		()
{
	ReCreateModel	();
	UpdateView		( true );
}

//** *******************************
//
void		CCollisionMemUsageTool::SetMemUnits				( CCollisionMemUsageTool::EMemUnits units )
{
	ASSERT( units != MU_Total );

	m_MemUnits = units;
}

//** *******************************
//
CCollisionMemUsageTool::EMemUnits	CCollisionMemUsageTool::GetMemUnits	() const
{
	return m_MemUnits;
}

//** *******************************
//
void			CCollisionMemUsageTool::ReCreateModel		()
{
	CWorld * pWorld = GGame->GetActiveWorld();
	
	if( pWorld )
	{
		CWorldTreeTraverser traverser;

		traverser.TraverseWorldTree( pWorld, &m_Model );
	}
}

//** *******************************
//
void			CCollisionMemUsageTool::UpdateView				( bool bRecreateView )
{
	if( bRecreateView )
	{
		ReCreateView();
	}

	UpdateMemView( true );
}

//** *******************************
//
void			CCollisionMemUsageTool::UpdateMemView			( bool bFreeze )
{
	if( bFreeze )
	{
		TreeFreeze();
	}

	for( Uint32 i = 0; i < m_MemNodeInfoArray.Size(); ++i )
	{
		const MemNodeInfo & mni = m_MemNodeInfoArray[ i ];

		SetCollisionMemSize( mni.nodeId, mni.memSize );
	}

	SetTotalMemory( m_Model.GetTotalCollisionMemory() );

	if( bFreeze )
	{
		TreeThaw();
	}
}

//** *******************************
//
void			CCollisionMemUsageTool::ReCreateView			()
{
	TreeFreeze();

	wxTreeItemId rootNodeId = CreateRootNode();
	wxTreeItemId infoNodeId = AppendNode( rootNodeId, GMeshesNodeName.AsChar(), GWorldImageId );

	m_MemNodeInfoArray.Clear();
	m_TreeEntryArray.Clear();

	// Iterate over unique mesh entries
	const TUniqueMeshInfoArray & arr = m_Model.GetUniqueMeshArray();

	SetNodeText( infoNodeId, GMultColumnId, ToString( arr.Size() ).AsChar() );

	for( Uint32 i = 0; i < arr.Size(); ++i )
	{
		const UniqueMeshData & umd = arr[ i ];

		// Append mesh node
		wxTreeItemId meshNodeId = AppendNode( infoNodeId, GetObjectPrintableName( umd.pMesh ), GResourceImageId );

		UpdateNodeMemInfo	( meshNodeId, umd.collisionMemSize );
		AddMeshToTreeCache	( meshNodeId, umd.pMesh );
		SetNodeText			( meshNodeId, GMultColumnId, ToString( umd.arr.Size() ).AsChar() );

		// Iterate over distinct scales
		Uint32 meshMemSize			= CollisionObjMemCalc::CollisionSizeOf( umd.pMesh );
		Uint32 amortizedMemSize		= meshMemSize / umd.arr.Size();
		Uint32 amortizedMemReminder	= meshMemSize % umd.arr.Size();

		for( Uint32 j = 0; j < umd.arr.Size(); ++j )
		{
			const SingleScaleData & ssd = umd.arr[ j ];

			// Append scale node
			wxTreeItemId scaleNodeId = AppendNode( meshNodeId, AsString( ssd.scale ).AsChar(), GEntityImageId );
			SetNodeText	( scaleNodeId, GMultColumnId, ToString( ssd.arr.Size() ).AsChar() );

			Uint32 memSize = ssd.collisionMemSize + amortizedMemSize;

			if( j == 0 )
			{
				memSize += amortizedMemReminder;
			}

			UpdateNodeMemInfo( scaleNodeId, memSize );

			// Iterate over CStaticMeshComponent entries
			for( Uint32 k = 0; k < ssd.arr.Size(); ++k )
			{
				CStaticMeshComponent * pSMC = ssd.arr[ k ];

				// Append CStaticMeshComponent node
				wxTreeItemId smcNodeId = AppendNode( scaleNodeId, GetLeafNodeEntityName( pSMC ), GComponentImageId );
				AddSMCToTreeCache( smcNodeId, pSMC );
				SetNodeText( smcNodeId, GLayerColumnId, GetLeafNodeLayerName( pSMC ) );
			}
		}
	}

	Uint32 totalMemSize = m_Model.GetTotalCollisionMemory();

	// Dummy spacer node
	wxTreeItemId dummyNodeId	= AppendNode( rootNodeId, TEXT( "" ) );

	//Summary node
	wxTreeItemId summaryNodeId	= AppendNode( rootNodeId, TEXT( "Total memory used" ) );
	SetBold( summaryNodeId, true );

	UpdateNodeMemInfo( summaryNodeId, totalMemSize );

	// Reset total memory label
	SetTotalMemory( totalMemSize );

	TreeThaw();
}

//** *******************************
//
void			CCollisionMemUsageTool::UpdateNodeMemInfo		( wxTreeItemId nodeId, Uint32 memSize )
{
	MemNodeInfo mni;

	mni.nodeId	= nodeId;
	mni.memSize	= memSize;

	m_MemNodeInfoArray.PushBack( mni );
}

//** *******************************
//
void			CCollisionMemUsageTool::AddMeshToTreeCache		( wxTreeItemId nodeId, CMesh * pMesh )
{
	// Register static mesh component
	TreeEntry entry;

	entry.nodeId	= nodeId;
	entry.pMesh		= pMesh;
	entry.pSMC		= NULL;

	m_TreeEntryArray.PushBack( entry );
}

//** *******************************
//
void			CCollisionMemUsageTool::AddSMCToTreeCache		( wxTreeItemId nodeId, CStaticMeshComponent * pSMC )
{
	// Register CNode instance
	TreeEntry entry;

	entry.nodeId	= nodeId;
	entry.pMesh		= NULL;
	entry.pSMC		= pSMC;

	m_TreeEntryArray.PushBack( entry );
}

//** *******************************
//
void			CCollisionMemUsageTool::TreeFreeze				()
{
	m_pCollisionObjectsTree->Freeze();
}

//** *******************************
//
void			CCollisionMemUsageTool::TreeThaw				()
{
	m_pCollisionObjectsTree->Thaw();
}

//** *******************************
//
wxTreeItemId	CCollisionMemUsageTool::CreateRootNode			()
{
	//FIXME: really delete??
	m_pCollisionObjectsTree->DeleteRoot();

	return m_pCollisionObjectsTree->AddRoot( TEXT("") );
}

//** *******************************
//
wxTreeItemId	CCollisionMemUsageTool::AppendNode				( wxTreeItemId parentNodeId, const wxString & name, Int32 imageId )
{
	wxTreeItemId nodeId = m_pCollisionObjectsTree->AppendItem( parentNodeId, TEXT(""), imageId, imageId );

	SetNodeText( nodeId, GTreeColumnId, name );

	return nodeId;
}

//** *******************************
//
void			CCollisionMemUsageTool::SetNodeText				( wxTreeItemId nodeId, Uint32 column, const wxString & text )
{
	m_pCollisionObjectsTree->SetItemText( nodeId, column, text );
}

//** *******************************
//
void			CCollisionMemUsageTool::SetCollisionMemSize		( wxTreeItemId nodeId, Uint32 memSize )
{
	if( memSize > 0 )
	{
		String sizeStr = FormattedMemoryMessage( memSize, GetMemUnits() );

		m_pCollisionObjectsTree->SetItemText( nodeId, GMemoryColumnId, sizeStr.AsChar() );
	}
}

//** *******************************
//
void			CCollisionMemUsageTool::SetBold					( wxTreeItemId nodeId, Bool bBold )
{
	m_pCollisionObjectsTree->SetItemBold( nodeId, bBold );
}

//** *******************************
//
void			CCollisionMemUsageTool::SetTotalMemory			( Uint32 memSize )
{
	m_pTotalMemLabel->SetLabel( FormattedMemoryMessage( memSize, GetMemUnits() ).AsChar() );
}

//** *******************************
//
wxString		CCollisionMemUsageTool::GetLeafNodeEntityName	( CStaticMeshComponent * pStaticMeshComponent ) const
{
	CObject * pParent = pStaticMeshComponent->GetParent();
	CEntity * pEntity = SafeCast< CEntity >( pParent );

	return GetObjectPrintableName( pEntity );
}

//** *******************************
//
wxString		CCollisionMemUsageTool::GetLeafNodeLayerName	( CStaticMeshComponent * pStaticMeshComponent ) const
{
	CObject *		pParent	= pStaticMeshComponent->GetParent();
	CEntity *		pEntity	= SafeCast< CEntity >( pParent );
	CLayer	*		pLayer	= SafeCast< CLayer >( pEntity->GetLayer() );

	return GetObjectPrintableName( pLayer->GetLayerInfo() );
}

//** *******************************
//FIXME: superfluous checks
wxString		CCollisionMemUsageTool::GetObjectPrintableName	( ISerializable * pObject ) const
{
	if( pObject->IsA< CWorld >() )
	{
		return TEXT( "World" );
	}
	else if ( pObject->IsA< CLayerGroup >() )
	{
		CLayerGroup * pLG = Cast< CLayerGroup >( pObject );

		return pLG->GetName().AsChar();
	}
	else if( pObject->IsA< CLayerInfo >() )
	{
		CLayerInfo * pLI = Cast< CLayerInfo >( pObject );

		return pLI->GetShortName().AsChar();
	}
	else if( pObject->IsA< CEntity >() )
	{
		CEntity * pE = Cast< CEntity >( pObject );

		return pE->GetName().AsChar();
	}
	else if( pObject->IsA< CComponent >() )
	{
		CComponent * pC = Cast< CComponent >( pObject );

		return pC->GetName().AsChar();
	}
	else if( pObject->IsA< CResource >() )
	{
		CResource* pR = SafeCast< CResource >( pObject );

		String name;

		if ( pR->GetFile() )
		{
			name = pR->GetFile()->GetFileName();
		}
		else 
		{
			name = pR->GetImportFile();
		}

		if ( name.GetLength() == 0 )
		{
			name = TXT("<noname>");
		}

		return name.AsChar();
	}
	else if( pObject->IsA< ICollisionShape>() )
	{
		ICollisionShape * pICS = SafeCast< ICollisionShape >( pObject );

		String name = pICS->GetFriendlyName();

		return name.AsChar();
	}

	return TEXT( "invalid name" );
}

//** *******************************
//
String CCollisionMemUsageTool::FormattedMemoryMessage( Uint32 size, CCollisionMemUsageTool::EMemUnits units ) const
{
	//FIXME: implicit enumeration dependency
	String suffix[] = { TEXT( "B" ), TEXT( "KB" ), TEXT( "MB" ) };

	switch ( units )
	{
	case MU_MBytes:
		return String::Printf( TEXT( "%4.2f %s" ), Float( size ) / ( 1024.f * 1024.f ), suffix[ (Int32) units ].AsChar() );
	case MU_KBytes:
		return String::Printf( TEXT( "%6.2f %s" ), Float( size ) / 1024.f, suffix[ (Int32) units ].AsChar() );
	default: 
		;
	}

	return String::Printf( TEXT( "%d %s" ), size, suffix[ (Int32) units ].AsChar() );
}


//** *******************************
//
String			CCollisionMemUsageTool::AsString				( const Vector & v ) const
{
	return String::Printf( TEXT( "[%4.3f, %4.3f, %4.3f]"), v.X, v.Y, v.Z );
}

//** *******************************
//
void			CCollisionMemUsageTool::OnSelectMemUnits		( EMemUnits units )
{
	if( GetMemUnits() != units )
	{
		SetMemUnits( units );
		m_pMemUnitsCB->SetSelection( units );
		UpdateView( false );
	}
}

//** *******************************
//
void			CCollisionMemUsageTool::OnSelectB				( wxCommandEvent & event )
{
	OnSelectMemUnits( MU_Bytes );
}

//** *******************************
//
void			CCollisionMemUsageTool::OnSelectKB				( wxCommandEvent & event )
{
	OnSelectMemUnits( MU_KBytes );
}

//** *******************************
//
void			CCollisionMemUsageTool::OnSelectMB				( wxCommandEvent & event )
{
	OnSelectMemUnits( MU_MBytes );
}

//** *******************************
//
void			CCollisionMemUsageTool::OnMemUnitChanged		( wxCommandEvent & event )
{
	//FIXME: unsafe?
	SetMemUnits( ( EMemUnits ) event.GetSelection() );
	UpdateView( false );
}

//** *******************************
//
void			CCollisionMemUsageTool::OnRefreshContent		( wxCommandEvent & event )
{
	ReReadAndRefreshAll();
}

//** *******************************
// FIXME: register changes so that scene is reloaded properly during editor's changes
// FIXME: add data to node, so that there is no need to search for it here
void			CCollisionMemUsageTool::OnTreeItemActivated		( wxTreeEvent & event )
{
	//FIXME: this is bullshit - at this point there is no constraint assuring that this widgets represent
	//current world state
	CWorld * pWorld = GGame->GetActiveWorld();

	if ( !pWorld )
	{
		return;
	}

	wxTreeItemId nodeId = event.GetItem();

	for( Uint32 i = 0; i < m_TreeEntryArray.Size(); ++i )
	{
		if( m_TreeEntryArray[ i ].nodeId == nodeId )
		{
			if( m_TreeEntryArray[ i ].pMesh != NULL )
			{
				CMesh * pMesh = m_TreeEntryArray[ i ].pMesh;

				String resourcePath = pMesh->GetFile()->GetDepotPath();
				SEvents::GetInstance().DispatchEvent( CNAME( OpenAsset ), CreateEventData( resourcePath ) );
			}
			else
			{
				ASSERT( m_TreeEntryArray[ i ].pSMC != NULL );
				ASSERT( m_TreeEntryArray[ i ].pSMC->IsA< CNode >() );

				CNode * pNode = Cast< CNode >( m_TreeEntryArray[ i ].pSMC );

				wxTheFrame->GetWorldEditPanel()->LookAtNode( pNode );
			}
		}
	}
}
