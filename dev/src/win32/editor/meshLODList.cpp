/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "meshLODList.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/renderCommands.h"
#include "../../common/engine/materialDefinition.h"
#include "../../common/core/feedback.h"


enum ELODDataType
{
	LDT_LOD,
	LDT_Chunk
};

/// Class defined only for the properties browser...
class CEdMeshLODProperties : public CObject
{
	DECLARE_ENGINE_CLASS( CEdMeshLODProperties, CObject, 0 );

public:
	Int32	m_index;			//!< Internal LOD index
	Float	m_distance;			//!< LOD show distance
	Int32	m_numTriangles;		//!< Number of triangles in the LOD
	Int32	m_numVertices;		//!< Number of vertices in the LOD
	Int32	m_numChunks;		//!< Number of chunks in the LOD
	Int32	m_numMaterials;		//!< Number of materials in the LOD
};

BEGIN_CLASS_RTTI( CEdMeshLODProperties )
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_distance, TXT("LOD show distance") );
	PROPERTY_RO( m_numTriangles, TXT("Number of triangles in LOD") );
	PROPERTY_RO( m_numVertices, TXT("Number of vertices in LOD") );
	PROPERTY_RO( m_numChunks, TXT("Number of chunks in LOD") );
	PROPERTY_RO( m_numMaterials, TXT("Number of used materials in LOD") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CEdMeshLODProperties );

/// Class defined only for the properties browser...
class CEdMeshChunkProperties : public CObject
{
	DECLARE_ENGINE_CLASS( CEdMeshChunkProperties, CObject, 0 );
	NO_DEFAULT_CONSTRUCTOR( CEdMeshChunkProperties );

public:
	Int32	m_index;			//!< Internal Chunk index
	Int32	m_materialIndex;	//!< Index of material (for reference)
	Int32	m_numVertices;		//!< Number of vertices in this chunk
	Int32	m_numTriangles;		//!< Number of triangles in this chunk
	Bool	m_renderInScene;	//!< Render chunk in the scene
	Bool	m_renderInCascade1;	//!< Render chunk in the shadow cascade 1
	Bool	m_renderInCascade2;	//!< Render chunk in the shadow cascade 2
	Bool	m_renderInCascade3;	//!< Render chunk in the shadow cascade 3
	Bool	m_renderInCascade4;	//!< Render chunk in the shadow cascade 4
	Bool	m_renderInLocalShadows; //!< Render chunk in the local shadows
	Bool	m_useForShadowMesh; //!< Use this chunk in shadow mesh generation

public:
	CEdMeshChunkProperties( const Int32 index, const SMeshChunkPacked& chunk )
		: m_index( index )
		, m_materialIndex( chunk.m_materialID )
		, m_numVertices( chunk.m_numVertices )
		, m_numTriangles( chunk.m_numIndices / 3 )
		, m_renderInScene( (chunk.m_renderMask & MCR_Scene) != 0 )
		, m_renderInCascade1( (chunk.m_renderMask & MCR_Cascade1) != 0 )
		, m_renderInCascade2( (chunk.m_renderMask & MCR_Cascade2) != 0 )
		, m_renderInCascade3( (chunk.m_renderMask & MCR_Cascade3) != 0 )
		, m_renderInCascade4( (chunk.m_renderMask & MCR_Cascade4) != 0 )
		, m_renderInLocalShadows( (chunk.m_renderMask & MCR_LocalShadows) != 0 )
		, m_useForShadowMesh( chunk.m_useForShadowmesh )
	{
	}

	const Uint8 AssembleRenderMask() const
	{
		Uint8 ret = 0;
		if ( m_renderInScene ) ret |= MCR_Scene;
		if ( m_renderInCascade1 ) ret |= MCR_Cascade1;
		if ( m_renderInCascade2 ) ret |= MCR_Cascade2;
		if ( m_renderInCascade3 ) ret |= MCR_Cascade3;
		if ( m_renderInCascade4 ) ret |= MCR_Cascade4;
		if ( m_renderInLocalShadows ) ret |= MCR_LocalShadows;
		return ret;
	}
};

BEGIN_CLASS_RTTI( CEdMeshChunkProperties )
	PARENT_CLASS( CObject );
	PROPERTY_RO( m_materialIndex, TXT("Index of material (for reference)") );
	PROPERTY_RO( m_numVertices, TXT("Number of vertices in this chunk") );
	PROPERTY_RO( m_numTriangles, TXT("Number of triangles in this chunk") );
	PROPERTY_EDIT( m_renderInScene, TXT("Render chunk in the scene") );
	PROPERTY_EDIT( m_renderInCascade1, TXT("Render chunk in the shadow cascade 1") );
	PROPERTY_EDIT( m_renderInCascade2, TXT("Render chunk in the shadow cascade 2") );
	PROPERTY_EDIT( m_renderInCascade3, TXT("Render chunk in the shadow cascade 3") );
	PROPERTY_EDIT( m_renderInCascade4, TXT("Render chunk in the shadow cascade 4") );
	PROPERTY_EDIT( m_renderInLocalShadows, TXT("Render chunk in the local shadows") );
	PROPERTY_EDIT( m_useForShadowMesh, TXT("Use this chunk in shadowmesh generation") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CEdMeshChunkProperties );

class CLODData : public wxTreeItemData
{
public:
	CLODData( Uint32 type, CObject* properties, Uint32 index )
		: m_type( type ), m_properties( properties ), m_index( index )
		{}

	Uint32			m_type;
	CObject*		m_properties;
	Uint32			m_index;
};


CEdMeshLODList::CEdMeshLODList( wxWindow* parent, CMeshTypeResource* mesh, CEdUndoManager* undoManager )
	: m_parent( parent )
	, m_mesh( mesh )
{
	// Create properties in panel
	{
		wxPanel* rp = XRCCTRL( *parent, "LODPropertiesPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		m_properties = new CEdPropertiesBrowserWithStatusbar( rp, settings, undoManager );
		m_properties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdMeshLODList::OnLODPropertiesChanged ), nullptr, this );
		sizer1->Add( m_properties, 1, wxEXPAND|wxRIGHT|wxBOTTOM, 5 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	m_lodTree = XRCCTRL( *parent, "LODTree", wxTreeCtrl );
	ASSERT( m_lodTree );

	UpdateList( false );

	// Connect events
	m_parent->Bind( wxEVT_COMMAND_TREE_SEL_CHANGED, &CEdMeshLODList::OnLODSelected, this, XRCID("LODTree") );
};

CEdMeshLODList::~CEdMeshLODList()
{
	for ( Uint32 i=0; i<m_lodProperties.Size(); i++ )
	{
		m_lodProperties[i]->RemoveFromRootSet();
		m_lodProperties[i]->Discard();
	}

	for ( Uint32 i=0; i<m_chunkProperties.Size(); i++ )
	{
		m_chunkProperties[i]->RemoveFromRootSet();
		m_chunkProperties[i]->Discard();
	}
}

void CEdMeshLODList::UpdateCaptions()
{
	if ( m_lodTree->IsEnabled() )
	{
		for ( Uint32 i=0; i<m_lodTreeRoots.Size(); i++ )
		{
			wxTreeItemId rootItem = m_lodTreeRoots[i];
			CLODData* lodData = static_cast< CLODData* >( m_lodTree->GetItemData(rootItem) );

			CEdMeshLODProperties* meshLOD = Cast< CEdMeshLODProperties >( lodData->m_properties );
			if ( meshLOD )
			{
				Uint32 index = lodData->m_index;

				// The name of LOD
				String lodName = String::Printf( TXT("Mesh%i "), index );

				// Append some info
				const SMeshTypeResourceLODLevel& level = m_mesh->GetLODLevel( index );

				Uint32 chunks = m_mesh->CountLODChunks( index );

				// Format name
				if ( index == 0 )
				{
					lodName += String::Printf( TXT("(FIRST, %u chunk%s) "), chunks, chunks>1 ? TXT("s") : TXT("") );
				}
				else
				{
					lodName += String::Printf( TXT("(LOD%i at %1.2fm, %u chunk%s) "), index, level.m_distance, chunks, chunks>1 ? TXT("s") : TXT("") );
				}

				// Set the name
				m_lodTree->SetItemText( rootItem, lodName.AsChar() );
			}
		}
	}
}


namespace
{
	typedef TDynArray< Uint32 > TreeItemPath;

	struct TreeStateSnapshot
	{
		String m_top;
		TDynArray< String > m_selected;
		TDynArray< String > m_expanded;
	};

	struct PathBasedStamp
	{
		String operator()( const wxTreeCtrl& tree, wxTreeItemId item, const TreeItemPath& path )
		{
			String result;
			for ( Uint32 pos : path )
			{
				result += ToString( pos ) + L";";
			}
			return result;
		}
	};

	template < typename StampGen >
	void SaveTreeStateImpl( const wxTreeCtrl& tree, StampGen stampGen, wxTreeItemId parent, const TreeItemPath& basePath, TreeStateSnapshot& outResult )
	{
		wxTreeItemIdValue cookie;
		wxTreeItemId child = tree.GetFirstChild( parent, cookie );
		Uint32 idx = 0;
		while ( child.IsOk() )
		{
			TreeItemPath path = basePath;
			path.PushBack( idx );

			String stamp = stampGen( tree, child, path );

			if ( tree.IsSelected( child ) )
			{
				outResult.m_selected.PushBack( stamp );
			}

			if ( tree.IsExpanded( child ) )
			{
				outResult.m_expanded.PushBack( stamp );
			}

			if ( child == tree.GetFirstVisibleItem() )
			{
				outResult.m_top = stamp;
			}

			SaveTreeStateImpl( tree, stampGen, child, path, outResult );

			child = tree.GetNextChild( parent, cookie ); 
			++idx;
		}
	}

	template < typename StampGen >
	TreeStateSnapshot SaveTreeState( const wxTreeCtrl& tree, StampGen stampGen = PathBasedStamp() )
	{
		TreeStateSnapshot res;
		if ( !tree.IsEmpty() )
		{
			SaveTreeStateImpl( tree, stampGen, tree.GetRootItem(), TreeItemPath(), res );
		}
		return res;
	}

	template < typename StampGen >
	void RestoreTreeStateImpl( wxTreeCtrl& tree, StampGen stampGen, wxTreeItemId parent, const TreeItemPath& basePath, const TreeStateSnapshot& snapshot, wxTreeItemId& top )
	{
		wxTreeItemIdValue cookie;
		wxTreeItemId child = tree.GetFirstChild( parent, cookie );
		Uint32 idx = 0;
		while ( child.IsOk() )
		{
			TreeItemPath path = basePath;
			path.PushBack( idx );

			String stamp = stampGen( tree, child, path );

			if ( snapshot.m_selected.Exist( stamp ) )
			{
				tree.SelectItem( child );
			}

			if ( snapshot.m_expanded.Exist( stamp ) )
			{
				tree.Expand( child );
			}

			if ( stamp == snapshot.m_top )
			{
				top = child;
			}

			RestoreTreeStateImpl( tree, stampGen, child, path, snapshot, top );

			child = tree.GetNextChild( parent, cookie ); 
			++idx;
		}
	}

	template < typename StampGen >
	void RestoreTreeState( wxTreeCtrl& tree, const TreeStateSnapshot& savedSelection, StampGen stampGen = PathBasedStamp() )
	{
		if ( !tree.IsEmpty() )
		{
			tree.UnselectAll();
			wxTreeItemId top = tree.GetFirstVisibleItem();
			RestoreTreeStateImpl( tree, stampGen, tree.GetRootItem(), TreeItemPath(), savedSelection, top );
			tree.ScrollTo( top );
		}
	}
}

void CEdMeshLODList::DoUpdateList( Bool preserveExpansionState )
{
	// Store the expansion and selection state
	TreeStateSnapshot selection = SaveTreeState( *m_lodTree, PathBasedStamp() );

	// Begin update
	m_lodTree->Freeze();
	m_lodTree->DeleteAllItems();

	wxTreeItemId root = m_lodTree->AddRoot( m_mesh->GetFriendlyName().AsChar() );

	m_lodTreeRoots.Clear();

	// Add LOD levels from mesh
	for ( Uint32 lodIdx = 0; lodIdx < m_lodProperties.Size(); ++lodIdx )
	{
		CEdMeshLODProperties* meshLOD = m_lodProperties[ lodIdx ];

		// The name of LOD
		String lodName = String::Printf( TXT("LOD%i "), lodIdx );;

		CLODData* lodData = new CLODData( LDT_LOD, meshLOD, lodIdx );

		// Add entry
		wxTreeItemId addedItem = m_lodTree->AppendItem(m_lodTree->GetRootItem(), lodName.AsChar(), -1, -1, lodData );

		m_lodTreeRoots.PushBack( addedItem );

		if ( CMesh* asMesh = Cast< CMesh >( m_mesh ) )
		{
			const CMesh::LODLevel& lodLevel = asMesh->GetMeshLODLevels()[ lodIdx ];

			const auto& chunks = asMesh->GetChunks();

			for ( Uint32 lodChunkIndex=0; lodChunkIndex<lodLevel.m_chunks.Size(); lodChunkIndex++ )
			{
				Uint16 meshChunkIndex = lodLevel.m_chunks[lodChunkIndex];
				if ( meshChunkIndex >= chunks.Size() )
				{
					continue;
				}
				const auto& chunk = chunks[meshChunkIndex];

				String matName   = CreateChunkMaterialName( chunk.m_materialID );
				String chunkName = String::Printf( TXT("Chunk %u (%u) - I: %u V: %u [%s]"), lodChunkIndex, meshChunkIndex, chunk.m_numIndices, chunk.m_numVertices, matName.AsChar() );

				if ( chunk.m_vertexType == MVT_SkinnedMesh || chunk.m_vertexType == MVT_DestructionMesh )
				{
					chunkName += TXT(" (skinning)");
				}

				CEdMeshChunkProperties* chunkProps = m_chunkProperties[ meshChunkIndex ];
				CLODData* chunkData = new CLODData( LDT_Chunk, chunkProps, meshChunkIndex );

				m_lodTree->AppendItem( addedItem, chunkName.AsChar(), -1, -1, chunkData );
			}
		}
	}

	// No items, add default
	const Uint32 count = m_lodTree->GetCount();
	if ( count == 1 )
	{
		// Add default item
		m_lodTree->AppendItem(m_lodTree->GetRootItem(), TXT("(No LODs)") );
		m_lodTree->Enable( false );
	}
	else
	{
		m_lodTree->Enable( true );
		// Update the captions
		UpdateCaptions();
	}

	m_lodTree->Thaw();

	if ( preserveExpansionState )
	{
		RestoreTreeState( *m_lodTree, selection, PathBasedStamp() );
	}
	else
	{
		m_lodTree->ExpandAll();
	}
}

Int32 CEdMeshLODList::GetSelectedLODIndex() const
{
	wxArrayTreeItemIds selections;
	m_lodTree->GetSelections(selections);

	if (selections.size() == 1)
	{
		const wxTreeItemId& selected = selections[0];
		if (!selected.IsOk())
		{
			return -1;
		}

		CLODData* lodData = (CLODData*)m_lodTree->GetItemData(selected);
		if (!lodData)
		{
			return -1;
		}

		if (lodData->m_type == LDT_Chunk)
		{
			lodData = (CLODData*)m_lodTree->GetItemData(m_lodTree->GetItemParent(selected));
		}

		return lodData->m_index;
	}
	else
	{
		Int32 selectedLOD = -1;

		for ( Uint32 si = 0; si < selections.size(); ++si )
		{
			const wxTreeItemId& selected = selections[si];

			if (!selected.IsOk())
			{
				continue;
			}

			CLODData* lodData = (CLODData*)m_lodTree->GetItemData(selected);
			if (!lodData)
			{
				continue;
			}

			if (lodData->m_type == LDT_Chunk)
			{
				lodData = (CLODData*)m_lodTree->GetItemData(m_lodTree->GetItemParent(selected));
				if (selectedLOD == lodData->m_index || selectedLOD == -1)
				{
					selectedLOD = lodData->m_index;
				}
				else
				{
					//chunks from different LODs selected
					return -1;
				}
			}
		}

		return selectedLOD;
	}
}

void CEdMeshLODList::GetSelectedChunkIndices( TDynArray< Uint32 >& indices ) const
{
	wxArrayTreeItemIds selections;
	m_lodTree->GetSelections(selections);

	for ( Uint32 si = 0; si < selections.size(); ++si )
	{
		const wxTreeItemId& selected = selections[si];

		if (!selected.IsOk())
		{
			continue;
		}

		CLODData* lodData = (CLODData*)m_lodTree->GetItemData(selected);
		if (!lodData)
		{
			continue;
		}

		if (lodData->m_type == LDT_Chunk)
		{
			indices.PushBack( lodData->m_index );
		}
	}
}

void CEdMeshLODList::UpdateList( Bool preserveExpansionState )
{
	// Deselect
	m_properties->Get().SetNoObject();

	// Destroy old LOD infos
	for ( Uint32 i=0; i<m_lodProperties.Size(); i++ )
	{
		m_lodProperties[i]->RemoveFromRootSet();
		m_lodProperties[i]->Discard();
	}
	m_lodProperties.Clear();

	// Create the object for each LOD
	const Uint32 numLODs = m_mesh->GetNumLODLevels();
	for ( Uint32 i=0; i<numLODs; i++ )
	{
		// Add to list
		CEdMeshLODProperties* lodProps = new CEdMeshLODProperties;
		m_lodProperties.PushBack( lodProps );

		// Keep...
		lodProps->AddToRootSet();

		// Get the lod info
		const SMeshTypeResourceLODLevel& lodLevel = m_mesh->GetLODLevel( i );

		// Define properties
		lodProps->m_distance = lodLevel.m_distance;
		lodProps->m_numVertices = m_mesh->CountLODVertices( i );
		lodProps->m_numTriangles = m_mesh->CountLODTriangles( i );
		lodProps->m_numMaterials = m_mesh->CountLODMaterials( i );
		lodProps->m_numChunks = m_mesh->CountLODChunks( i );
		lodProps->m_index = i;
	}

	// Destroy old chunk infos
	for ( Uint32 i=0; i<m_chunkProperties.Size(); i++ )
	{
		m_chunkProperties[i]->RemoveFromRootSet();
		m_chunkProperties[i]->Discard();
	}
	m_chunkProperties.Clear();

	// Create the property wrappers for each chunk
	if ( m_mesh->IsA< CMesh >() )
	{
		CMesh* mesh = Cast< CMesh >( m_mesh );

		const Uint32 numChunks = mesh->GetChunks().Size();
		for ( Uint32 i=0; i<numChunks; i++ )
		{
			// Add to list
			CEdMeshChunkProperties* chunkProps = new CEdMeshChunkProperties( i, mesh->GetChunks()[i] );
			m_chunkProperties.PushBack( chunkProps );

			// Keep...
			chunkProps->AddToRootSet();
		}
	}

	// Update the list
	DoUpdateList( preserveExpansionState );
}

String CEdMeshLODList::CreateChunkMaterialName( Uint32 matId ) const
{
	ASSERT ( matId < m_mesh->GetMaterialNames().Size() && matId < m_mesh->GetMaterials().Size() );

	THandle< IMaterial > mat = m_mesh->GetMaterials()[ matId ];
	IMaterialDefinition* def = mat ? mat->GetMaterialDefinition() : nullptr;
	String matName    = m_mesh->GetMaterialNames()[ matId ];
	String shaderName = def ? def->GetShaderName() : TXT("<broken>");

	return String::Printf( TXT("%s | %s"), matName.AsChar(), shaderName.AsChar() );
}

void CEdMeshLODList::OnLODSelected( wxTreeEvent& event )
{
	wxArrayTreeItemIds selections;
	m_lodTree->GetSelections(selections);

	TDynArray< Uint32 > chunkIndices;

	for ( Uint32 si = 0; si < selections.size(); ++si )
	{
		const wxTreeItemId& selected = selections[si];

		if (!selected.IsOk())
		{
			continue;
		}

		CLODData* lodData = (CLODData*)m_lodTree->GetItemData(selected);
		if (!lodData)
		{
			continue;
		}
		else
		{
			if (lodData->m_type == LDT_Chunk)
			{
				if ( CMesh* asMesh = Cast< CMesh >( m_mesh ) )
				{
					chunkIndices.PushBack( lodData->m_index );
				}
			}
		}
	}

	if ( chunkIndices.Size() > 0 )
	{
		( new CRenderCommand_ToggleMeshChunkHighlight( Cast< CMesh >( m_mesh ), chunkIndices ) )->Commit();
	}
	else
	{
		( new CRenderCommand_ToggleMeshChunkHighlight( nullptr, -1 ) )->Commit();
	}

	if ( selections.size() == 1 )
	{
		const wxTreeItemId& selected = selections[0];

		if (selected.IsOk())
		{
			CLODData* lodData = (CLODData*)m_lodTree->GetItemData(selected);

			if (lodData)
			{
				m_properties->Get().SetObject( lodData->m_properties );
				m_properties->Refresh();
			}
			else
			{
				m_properties->Get().SetNoObject();
			}
		}
	}
	else
	{
		m_properties->Get().SetNoObject();
	}

	// Update parent
	wxTreeEvent fakeEvent( wxEVT_COMMAND_TREE_SEL_CHANGED );
	ProcessEvent( fakeEvent );
}

void CEdMeshLODList::OnLODPropertiesChanged( wxCommandEvent& event )
{
	wxArrayTreeItemIds selections;
	m_lodTree->GetSelections(selections);

	if ( selections.size() != 1 )
		return;

	const wxTreeItemId& selected = selections[0];
	if (!selected.IsOk())
		return;

	CLODData* lodData = (CLODData*)m_lodTree->GetItemData(selected);
	if ( !lodData )
		return;

	// chunk props ?
	CEdMeshChunkProperties* chunkProps = Cast< CEdMeshChunkProperties >( lodData->m_properties );
	if ( chunkProps && m_mesh->IsA< CMesh >() )
	{
		const Uint8 renderMask = chunkProps->AssembleRenderMask();

		CMesh* mesh = Cast< CMesh >( m_mesh );
		mesh->ForceRenderMaskOnChunk( chunkProps->m_index, renderMask, true );
		mesh->ForceShadowMeshFlagOnChunk( chunkProps->m_index, chunkProps->m_useForShadowMesh );
	}

	// lod props ?
	CEdMeshLODProperties* lodProps = Cast< CEdMeshLODProperties >( lodData->m_properties );
	if ( lodProps )
	{
		// Change LOD settings
		SMeshTypeResourceLODLevel lodLevelInfo;
		lodLevelInfo.m_distance = lodProps->m_distance;
		m_mesh->UpdateLODSettings( lodProps->m_index, lodLevelInfo );

		// Update instances
		CDrawableComponent::RecreateProxiesOfRenderableComponents();
	}

	// Update list
	UpdateCaptions();
}

