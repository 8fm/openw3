/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "collisionMemToolModel.h"

using namespace CollisionMem;

class CCollisionMemUsageTool : public wxSmartLayoutPanel
{
	DECLARE_EVENT_TABLE();

	struct MemNodeInfo
	{
		wxTreeItemId	nodeId; 
		Uint32			memSize;
	};

	struct TreeEntry
	{
		wxTreeItemId nodeId;
		CMesh *					pMesh;
		CStaticMeshComponent *	pSMC;
	};

	typedef TDynArray< MemNodeInfo >	TMemNodeInfoArray;
	typedef	TDynArray< TreeEntry >		TTreeEntryArray;

public:

	enum EMemUnits
	{
		MU_Bytes = 0,
		MU_KBytes,
		MU_MBytes,
		MU_Total
	};

private:

	CCollisionMemToolModel	m_Model;					//!< This widget's model

	TTreeEntryArray			m_TreeEntryArray;			//!< Meshes associated with nodes
	TMemNodeInfoArray		m_MemNodeInfoArray;			//!< Raw memory data associated with each node

	wxTreeListCtrl *		m_pCollisionObjectsTree;	//!< TreeListCtrl used to show collision memory size

	wxMenuBar *				m_pMenuBar;
	wxMenu *				m_pUnitMenu;

	const Uint32				m_BMenuItemId;
	const Uint32				m_KBMenuItemId;
	const Uint32				m_MBMenuItemId;

	wxChoice *				m_pMemUnitsCB;
	wxStaticText *			m_pTotalMemLabel;

	EMemUnits				m_MemUnits;					//!< Currently displayed memory units

public:

	CCollisionMemUsageTool	( wxWindow * parent );

private:

	// Initialization
	void			InitializeTreeViewRes	();
	void			InitializeToolsRes		();

public:

	void			ReReadAndRefreshAll		();

	// Getters and setters
	void			SetMemUnits				( EMemUnits units );
	EMemUnits		GetMemUnits				() const;

private:

	//Model
	void			ReCreateModel			();

	// View
	void			UpdateView				( bool bRecreateView );
	void			UpdateMemView			( bool bFreeze);
	void			ReCreateView			();

	// View - memory information display helpers
	void			UpdateNodeMemInfo		( wxTreeItemId nodeId, Uint32 memSize );
	void			AddMeshToTreeCache		( wxTreeItemId nodeId, CMesh * pMesh );
	void			AddSMCToTreeCache		( wxTreeItemId nodeId, CStaticMeshComponent * pSMC );

	//View - low level tree building tools
	void			TreeFreeze				();
	void			TreeThaw				();

	wxTreeItemId	CreateRootNode			();
	wxTreeItemId	AppendNode				( wxTreeItemId parentNodeId, const wxString & name, Int32 imageId = -1 );

	void			SetNodeText				( wxTreeItemId nodeId, Uint32 column, const wxString & text );

	void			SetCollisionMemSize		( wxTreeItemId nodeId, Uint32 memSize );
	void			SetBold					( wxTreeItemId nodeId, Bool bBold );
	void			SetItalic				( wxTreeItemId nodeId, Bool bItalic );
	void			SetTotalMemory			( Uint32 memSize );

	// Convenience methods
	wxString		GetLeafNodeEntityName	( CStaticMeshComponent * pStaticMeshComponent ) const;
	wxString		GetLeafNodeLayerName	( CStaticMeshComponent * pStaticMeshComponent ) const;
	wxString		GetObjectPrintableName	( ISerializable * pObject ) const;

	String			FormattedMemoryMessage	( Uint32 size, EMemUnits units ) const;

	String			AsString				( const Vector & v ) const;

	// Event handlers
private:

	void			OnSelectMemUnits		( EMemUnits units );

protected:

	void			OnSelectB				( wxCommandEvent & event );
	void			OnSelectKB				( wxCommandEvent & event );
	void			OnSelectMB				( wxCommandEvent & event );

	void			OnMemUnitChanged		( wxCommandEvent & event );
	void			OnRefreshContent		( wxCommandEvent & event );
	void			OnTreeItemActivated		( wxTreeEvent & event );

};
