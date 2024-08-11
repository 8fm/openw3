/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdMeshLODProperties;
class CEdMeshChunkProperties;

/// List of mesh LODs
class CEdMeshLODList : public wxEvtHandler
{
protected:
	wxWindow*								m_parent;			//!< Parent frame
	CMeshTypeResource*						m_mesh;				//!< Edited mesh
	CEdPropertiesBrowserWithStatusbar*		m_properties;		//!< Properties
	wxTreeCtrl*								m_lodTree;			//!< LOD tree
	TDynArray< wxTreeItemId >				m_lodTreeRoots;		//!< LOD levels
	TDynArray< CEdMeshLODProperties* >		m_lodProperties;	//!< LOD properties object
	TDynArray< CEdMeshChunkProperties* >	m_chunkProperties;	//!< Chunk properties object

public:
	CEdMeshLODList( wxWindow* parent, CMeshTypeResource* mesh, CEdUndoManager* undoManager );
	~CEdMeshLODList();

	//! Get selected LOD index. -1 if no LOD is selected.
	Int32 GetSelectedLODIndex() const;

	//! Get selected LOD index
	void GetSelectedChunkIndices( TDynArray< Uint32 >& indices ) const;

	//! Update LOD list
	void UpdateList( Bool preserveExpansionState = true );

	//!
	String CreateChunkMaterialName( Uint32 matId ) const;

protected:
	//! Update LOD list
	void DoUpdateList( Bool preserveExpansionState );

	//! Update list captions
	void UpdateCaptions();

	//! Events
	void OnLODSelected( wxTreeEvent& event );
	void OnLODPropertiesChanged( wxCommandEvent& event );
};
