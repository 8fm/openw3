/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "classHierarchyMapper.h"

class CEdUndoManager;
class CEdEntityEditor;

/// Graph editor hook
class EntityGraphEditorHook
{
public:
	virtual ~EntityGraphEditorHook() {};

	//! Selection has changed
	virtual void OnGraphSelectionChanged()=0;

	//! Selection has been removed
	virtual void OnGraphSelectionDeleted()=0;
};

/// Graph editor for entity
class CEdEntityGraphEditor : public CEdCanvas, public ISavableToConfig
{
	DECLARE_EVENT_TABLE()

protected:
	/// Block layout info
	struct LayoutInfo
	{
		wxSize		m_windowSize;		//!< Absolute window size
		wxRect		m_connectorRect;	//!< Connector rect
		wxPoint		m_connectorSrc;		//!< Connector source
		wxPoint		m_connectorDest;	//!< Connector destination
	};

	/// Mouse action mode
	enum EMouseAction
	{
		MA_None,				//!< Nothing, free mouse
		MA_BackgroundScroll,	//!< Scrolling background
		MA_MovingWindows,		//!< Moving selected windows
		MA_SelectingWindows,	//!< Selecting multiple windows
		MA_DraggingLink,		//!< Dragging link
	};

protected:
	THashMap< CObject*, LayoutInfo >	m_layout;	//!< Layout info for blocks
	TDynArray< THandle< CObject > >		m_layoutObjects;

protected:
	CEdEntityEditor*					m_editor;				//!< Parent editor
	wxPoint								m_lastMousePos;			//!< Last position of mouse
	wxPoint								m_selectRectStart;		//!< Selection rect first corner
	wxPoint								m_selectRectEnd;		//!< Selection rect second corner
	wxPoint								m_autoScroll;			//!< Automatic background scroll
	Int32								m_moveTotal;			//!< Total move
	TDynArray< CClass* >				m_attachmentClasses;	//!< List of attachment classes
	TDynArray< THandle< CObject > >		m_selected;				//!< Selected objects
	EMouseAction						m_action;				//!< Action performed
	THandle< CObject >					m_activeItem;			//!< Active item
	Float								m_desiredScale;			//!< Desired scale
	EntityGraphEditorHook*				m_hook;					//!< Event hook
	CComponent*							m_srcComponent;			//!< Source link component
	CComponent*							m_destComponent;		//!< Destination link component
	CEntityTemplate*					m_template;				//!< Entity template
	Bool								m_showIncludes;
	CEdFileDialog						m_importDlg;
	CEdUndoManager*						m_undoManager;
	TStaticArray< CClass*, 5 >			m_recentlyUsedBlockClasses;
	Bool								m_componentClassHierarchyInitialized;
	CClassHierarchyMapper				m_componentClassHierarchy;
	THashSet< String >					m_hidden;
	Gdiplus::Bitmap*					m_importedIcon;
	Gdiplus::Bitmap*					m_appearanceIcon;
	Gdiplus::Bitmap*					m_streamedIcon;
	Gdiplus::Bitmap*					m_overridesIcon;

private:
	Int32 m_toggleShowIncludesStartingId;
	Int32 m_recentlyUsedBlockClassesStartingId;
	Int32 m_usedEffectsStartingId;

private:
	CComponent* CreateComponent( CResource* resource, CClass* c );
	
	//! Update newly created component
	void UpdateComponent( CComponent* component, Int32 graphPosX, Int32 graphPosY );

public:
	CEdEntityGraphEditor( wxWindow* parent, CEntity* entity, CEntityTemplate* entityTemplate, CEdUndoManager* undoManager, EntityGraphEditorHook* hook, CEdEntityEditor* editor );

	~CEdEntityGraphEditor();
	
	// get layout
	void GetBlockWidthHeight( CComponent* block, Int32 & outX, Int32 & outY );
	
	// Get entity
	CEntity* GetEntity() const;
	
	// Get selected objects
	void GetSelectedObjects( TDynArray< CObject* >& objects );
	void GetAllObjects( TDynArray< CObject* >& objects );
	
	// Force selection update
	void ForceSelectionUpdate();
	
	// Force layout update
	void ForceLayoutUpdate();
	
	// Delete selection
	void DeleteSelection();
	
	// Copy selection
	void CopySelection();
	
	// Cut selection
	void CutSelection();
	
	// Paste selection
	void PasteSelection();
	
	// Check if component is visible
	Bool IsComponentVisible(const CComponent &component);

protected:
	String GetComponentName(const CComponent &component);
	String GetFriendlyName(const CComponent &component);

private:
	void ScaleGraph( Float newScale );

public:
	//! Zoom extents
	void ZoomExtents( Bool extentsOfSelectionOnly = false );

	//! Paint the shit
	virtual void PaintCanvas( Int32 width, Int32 height );

	//! Mouse event
	virtual void MouseClick( wxMouseEvent& event );

	//! Mouse moved
	virtual void MouseMove( wxMouseEvent& event, wxPoint delta );

	void ClearLayout();
	void RemoveFromLayout( CComponent* component );

	void SelectComponents( const TDynArray< CComponent* >& components );
	void HideComponents( const TDynArray< CComponent* >& components );
	void IsolateComponents( const TDynArray< CComponent* >& components );
	void UnhideComponents();

protected:
	//! Update block layout
	void UpdateBlockLayout( CComponent* block );

	//! Draw block layout
	void DrawBlockLayout( CComponent* block );

	//! Draw socket connector
	void DrawConnector( const wxRect& rect, const wxColour& borderColor, const wxColour &baseColor );

	//! Draw block links
	void DrawBlockLinks( CComponent* block );

	//! Draw dragged link
	void DrawDraggedLink();

	//! Set background offset
	void SetBackgroundOffset( wxPoint offset );

	//! Scroll background offset
	void ScrollBackgroundOffset( wxPoint delta );

	//! Delete selected objects
	void DeleteSelectedObjects( bool askUser = true );

	//! Deselect all objects
	void DeselectAllObjects();

	//! Returns true if given object is selected
	Bool IsObjectSelected( CObject* object );

	//! Select object
	void SelectObject( CObject* block, Bool select );

	//! Move selected blocks
	void MoveSelectedBlocks( wxPoint totalOffset );

	//! Select objects from area
	void SelectObjectsFromArea( wxRect area );

	//! Open context menu
	void OpenContextMenu();

	//! Update active item
	void UpdateActiveItem( wxPoint mousePoint );
	
	//! Finalize link
	void FinalizeLink();

	//! Calculate link bezier points
	void CalculateBezierPoints( CComponent* src, CComponent* dest, wxPoint& a, wxPoint& b, wxPoint& c, wxPoint& d );

	//! Is attachment allowed
	Bool IsAttachmentAllowed( const CComponent* parent, const CComponent* child, const CClass* attachmentClass ) const;

	//! New attachment created
	void OnAttachmentCreated( IAttachment* attachment );

	void AddComponent( CComponent* component, const Vector& origin = Vector::ZEROS );
	void AddComponent( CComponent* component, const Matrix& localToWorld, const Vector& origin = Vector::ZEROS );

protected:
	wxString ConvertActorDialogLine( String& line ) const;

protected:
	void OnMouseLeftDblClick( wxMouseEvent &event );
	void OnMouseWheel( wxMouseEvent& event );
	void OnSpawnRecentBlock( wxCommandEvent& event );
	void OnSpawnAttachment( wxCommandEvent& event );
	void OnToggleShowIncludes( wxCommandEvent& event );
	void OnHideSelection( wxCommandEvent& event );
	void OnIsolateSelection( wxCommandEvent& event );
	void OnUnhideHiddenComponents( wxCommandEvent& event );
	void OnEditCopy( wxCommandEvent& event );
	void OnEditCut( wxCommandEvent& event );
	void OnEditPaste( wxCommandEvent& event );
	void OnChangeComponentClass( wxCommandEvent& event );
	void OnMergeMesh( wxCommandEvent& event );

	void GetUniqueOverriddenPropertiesForSelection( TDynArray< CName >& propertyNames );
	void OnShowComponentOverrides( wxCommandEvent& event );
	void OnResetOverriddenProperties( wxCommandEvent& event );

	void OnConvertMesh( wxCommandEvent& event );
	void OnConvertMovingAgent( wxCommandEvent& event );
	void OnConvertAnimComponentToSlotComponent( wxCommandEvent& event );
	void OnConvertSlotComponentToAnimComponent( wxCommandEvent& event );
	void OnConvertToSlot( wxCommandEvent& event );
	void OnEditComponentRelatedEffect( wxCommandEvent& event );
	void OnGenerateNavmesh( wxCommandEvent& event );
	void OnToggleNavmeshEdition( wxCommandEvent& event );
	void OnRemoveStreamingData( wxCommandEvent& event );
	void OnResetEntityData( wxCommandEvent& event );
	void OnInspectObject( wxCommandEvent& event );
	void OnSelectComponent( wxCommandEvent& event );
	void OnAlignComponents( wxCommandEvent& event );
	void OnAlignComponentsAll( wxCommandEvent& event );

protected:
	wxDragResult OnDragOver( wxCoord x, wxCoord y, wxDragResult def );
	Bool OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources );

protected:
	virtual wxColour GetCanvasColor() const { return ENTITY_EDITOR_BACKGROUND; }

	// ISaveableToConfig
	virtual void SaveOptionsToConfig();
	virtual void LoadOptionsFromConfig();

private:
	// helper functions
	void SpawnBlock( CClass* blockClass );
	void DrawStatistic();
	
};
