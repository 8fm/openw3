/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdBehaviorEditor;
class CEdBehaviorGraphEditor;
class CBehaviorDebugVisualizer;

#include "behaviorEditorPanel.h"
#include "graphEditor.h"
#include "../../common/engine/graphContainer.h"

enum EBehaviorEditorEffectType
{
	BEET_INTERPOLATE_FLOAT
};

class CBehaviorEditorEffect
{
private:
	Bool						m_isToRemove;
protected:
	EBehaviorEditorEffectType	m_type;
public:
	CBehaviorEditorEffect() : m_isToRemove( false ) {}
	virtual void OnTick( Float delta, CEdBehaviorGraphEditor* editor ) = 0;
	void Remove() { m_isToRemove = true; }
	Bool IsToRemove() const { return m_isToRemove; }
	EBehaviorEditorEffectType GetType() const { return m_type; }
};

class CBehaviorEditorEffectInterpolateFloat : public CBehaviorEditorEffect
{
	CName		m_variableName;
	Float		m_accumulator;
	Float		m_accumulationSpeed;
public:
	CBehaviorEditorEffectInterpolateFloat( CName variableName = CName::NONE, CEdBehaviorGraphEditor* editor = NULL, Float accumulationSpeed = 1.0f );
	virtual void OnTick( Float delta, CEdBehaviorGraphEditor* editor );
	Float GetAccumulationSpeed() const { return m_accumulationSpeed; }
	void SetAccumulationSpeed( Float accumulationSpeed ) { m_accumulationSpeed = accumulationSpeed; }
	const CName& GetVariableName() const { return m_variableName; }
};

class CEdBehaviorGraphEditor : public CEdGraphEditor,
							   public IGraphContainer,
							   public CEdBehaviorEditorPanel
{
	DECLARE_EVENT_TABLE()

protected:
	CBehaviorGraphContainerNode*	m_currentRoot;
	Vector							m_offset;
	BehaviorBackground				m_background;

	THashMap< CBehaviorGraphNode*, CBehaviorDebugVisualizer* >	m_debugVisualizers;
	Bool													m_debugDisplayBlocksActivation;
	Bool													m_displayConditions;
	Bool													m_displayConditionsAlternate;
	Bool													m_displayConditionTests;

	TSet< CBehaviorEditorEffect* >			m_effects;
	THandle< CBehaviorGraphStateNode >		m_handleToCopyAllTransitionsGraphState;
public:
	CEdBehaviorGraphEditor( CEdBehaviorEditor* editor );
	~CEdBehaviorGraphEditor();

	static wxString		GetPanelNameStatic()	{ return wxT("Graph"); }

	virtual wxString	GetPanelName() const	{ return wxT("Graph"); }
	virtual wxString	GetPanelCaption() const { return wxT("Graph"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual wxWindow*	GetPanelWindow()		{ return this; }

	virtual void		OnReset()				{ ResetGraph(); }
	virtual void		OnDebug( Bool flag )	{ SetReadOnly( flag ); }
	virtual void		OnUnloadEntity()		{ RemoveAllVisualizers(); }
	virtual void		OnPanelClose()			{ ResetGraph(); }
	virtual void		OnTick( Float dt );

	const wxWindow*		GetPanelWindow() const { return this; }

public:
	void AddEffect( CBehaviorEditorEffect* effect );
	void RemoveEffect( CBehaviorEditorEffect* effect );

public: // CDropTarget
	virtual wxDragResult OnDragOver( wxCoord x, wxCoord y, wxDragResult def );
	virtual Bool OnDropText( wxCoord x, wxCoord y, String &text );

public:
	virtual void SaveSession( CConfigurationManager &config, const String& path );
	virtual void RestoreSession( CConfigurationManager &config, const String& path );

public:
	CEdUndoManager* GetUndoManager() const { return m_undoManager; }

public:
	//! Set graph read only
	void SetReadOnly( Bool flag );

	//! Reset behavior graph editor
	void ResetGraph();

	//! Calculate inner area size for given block
	virtual void CalcBlockInnerArea( CGraphBlock* block, wxSize& innerArea );

	//! Draw inner area for given block, rect is given in client space, clip rect is set
	virtual void DrawBlockInnerArea( CGraphBlock* block, const wxRect& rect );

	//! set new root node, also zoom out to see whole graph if wanted (zoomExtents)
	void SetRootNode( CBehaviorGraphContainerNode* rootNode, Bool zoomExtents = true );

	//! get current root node
	CBehaviorGraphContainerNode* GetRootNode() const;

	//! Set conditions displaying
	void SetDisplayConditions( Bool flag );

	//! Get conditions displaying
	Bool GetDisplayConditions() const { return m_displayConditions; }

	//! Set condition tests displaying
	void SetDisplayConditionTests( Bool flag );

	//! Get condition tests displaying
	Bool GetDisplayConditionTests() const { return m_displayConditionTests; }
		
	//! Is block activated
	virtual Bool IsBlockActivated( CGraphBlock* block ) const;

	//! Get blocks activation alpha
	virtual Float GetBlockActivationAlpha( CGraphBlock* block ) const;

	//! Get layer
	virtual SGraphLayer* GetLayer( Uint32 num );

	//! Get layer num
	virtual Uint32 GetLayerNum() const;

	//! Focus graph on block
	void FocusOnBehaviorNode( CBehaviorGraphNode* node );

public:
	//! Paint the shit
	void PaintCanvas( Int32 width, Int32 height );

	//! proceed up one level
	void DisplayUpperLevel();

	//! proceed one level down, inside given block
	void DisplayBlockContent( CBehaviorGraphNode *node );

	//! display blocks activations
	void DisplayBlockActivations( Bool flag );

public:
	//! Get object that owns the graph
	virtual CObject *GraphGetOwner();

	//! Get list of blocks
	virtual TDynArray< CGraphBlock* >& GraphGetBlocks() { return m_currentRoot->GetConnectedChildren(); }
	virtual const TDynArray< CGraphBlock* >& GraphGetBlocks() const { return m_currentRoot->GetConnectedChildren(); }

	//! Graph structure has been modified
	virtual Bool PrepareForGraphStructureModified();
	virtual void GraphStructureModified();

	//! Does this graph supports given class
	virtual Bool GraphSupportsBlockClass( CClass *blockClass ) const;

	//!! Get background offset
	virtual Vector GraphGetBackgroundOffset() const;

	//! Set background offset
	virtual void GraphSetBackgroundOffset( const Vector& offset );

	//! Create block and at it to graph
	virtual CGraphBlock* GraphCreateBlock( const GraphBlockSpawnInfo& info );

	//! Can remove block?
	virtual Bool GraphCanRemoveBlock( CGraphBlock *block ) const;

	//! Remove block from the graph, does not delete object
	virtual Bool GraphRemoveBlock( CGraphBlock *block );

	//! Paste blocks
	virtual Bool GraphPasteBlocks( const TDynArray< Uint8 >& data, TDynArray< CGraphBlock* >& pastedBlocks, Bool relativeSpawn, const Vector& spawnPosition );

	//
	virtual Bool GraphPasteBlocks( const TDynArray< Uint8 >& data, TDynArray< CGraphBlock* >& pastedBlocks, Bool relativeSpawn, const Vector& spawnPosition, CBehaviorGraphContainerNode *parent );

	//! init menu
	virtual void InitClipboardMenu( wxMenu& menu );

	//! init menu
	virtual void InitCollapseToStageMenu( wxMenu& menu );

	//! Fill context menu for Empty Selection
	virtual void InitLinkedDefaultContextMenu( wxMenu& menu );

	//! init context menu for block
	virtual void InitLinkedBlockContextMenu( CGraphBlock *block, wxMenu &menu );

	//! init context menu for socket
	virtual void InitLinkedSocketContextMenu( CGraphSocket *socket, wxMenu &menu );

	//! connect two given sockets
	virtual void ConnectSockets( CGraphSocket* srcSocket, CGraphSocket* destSocket );

	void PasteOnCenter();

public:
	virtual Bool IsDraggedByClickOnInnerArea( wxMouseEvent& event, CGraphBlock* block );

	Bool MouseClickAndMoveActions( wxMouseEvent& event );

	void MouseClick( wxMouseEvent& event );

	void MouseMove( wxMouseEvent& event, wxPoint delta );

	void OnMouseLeftDblClick( wxMouseEvent &event );

	void OnKeyDown( wxKeyEvent &event );

	void OnKeyUp( wxKeyEvent &event );

	void OnSetDefaultState( wxCommandEvent& event );

	void OnSetStateActive( wxCommandEvent& event );

	void OnSetDefaultStateMachine( wxCommandEvent& event );

	void OnGoToTransitionDestState( wxCommandEvent& event );

	void OnGoToTransitionSrcState( wxCommandEvent& event );

	void OnGoToPointedtState( wxCommandEvent& event );

	void OnAddInput( wxCommandEvent& event );

	void OnAddInputWithWeight( wxCommandEvent& event );

	void OnAddRandomInput( wxCommandEvent& event );

	void OnRemoveInput( wxCommandEvent& event );

	void OnRemoveContainerInput( wxCommandEvent& event );

	void OnAddContainerAnimInput( wxCommandEvent& event );

	void OnAddContainerValueInput( wxCommandEvent& event );

	void OnAddContainerVectorValueInput( wxCommandEvent& event );

	void OnAddContainerMimicInput( wxCommandEvent& event );

	void OnAddSwitchInput( wxCommandEvent& event );

	void OnRemoveSwitchInput( wxCommandEvent& event );

	void OnRefreshEnumInput( wxCommandEvent& event );

	void OnAddDebugVisualizer( wxCommandEvent& event );

	void OnChangeDebugVisualizer( wxCommandEvent& event );

	void OnDebugVisualizerToggleBonesAxis( wxCommandEvent& event );

	void OnDebugVisualizerToggleBonesName( wxCommandEvent& event );

	void OnDebugVisualizerToggleBox( wxCommandEvent& event );

	void OnDebugVisualizerChooseHelpers( wxCommandEvent& event );

	void OnDebugVisualizerBoneStyle( wxCommandEvent& event );

	void OnRemoveDebugVisualizer( wxCommandEvent& event );

	void OnCollapseToStage( wxCommandEvent& event );

	void OnCreateTransition( wxCommandEvent& event );

	void OnPasteTransition( wxCommandEvent& event );

	void OnCopyTransitionCondition( wxCommandEvent& event );

	void OnPasteTransitionCondition( wxCommandEvent& event );

	void OnConvertTransitionCondition( wxCommandEvent& event );

	void OnCopyAllTransitions( wxCommandEvent& event );

	void OnPasteAllTransitions( wxCommandEvent& event );

	void RemoveAllVisualizers();

	void CalcSocketLayoutCenter( const TDynArray< CGraphSocket* > &sockets, const wxRect &clientRect, BlockLayoutInfo& layout );

	virtual void DrawBlockLayout( CGraphBlock* block );
	virtual void DrawBlockBackground( CGraphBlock* block, BlockLayoutInfo* layout, const SGraphBlockLayoutHelperData& data );
	virtual void DrawBlockSocketsCaptions( CGraphBlock* block, BlockLayoutInfo* layout, const SGraphBlockLayoutHelperData& data );

	void DrawBlockLinks( CGraphBlock* block );

	void DrawTransitionLink( CGraphBlock *startBlock, CGraphBlock *midBlock, CGraphBlock *endBlock, const wxColour &color, Float width );

	void OnBreakAllLinks( wxCommandEvent& event );

	void OnBreakLink( wxCommandEvent& event );

	void OnPasteHere( wxCommandEvent &event );

	void OnEditTrackNames( wxCommandEvent &event );

	void OnGoToSnapshot( wxCommandEvent &event );

	void OnAddGlobalTransition( wxCommandEvent& event );

	void OnAddGlobalComboTransition( wxCommandEvent& event );

	void OnCreateDefaultTransition( wxCommandEvent& event );

	void OnConvertGlobalWithStreamingTransitionCondition( wxCommandEvent& event );

protected:

	virtual wxColor GetCanvasColor() const { return BEHAVIOR_EDITOR_BACKGROUND; }

	virtual void AdjustBlockColors( CGraphBlock* block, Color& borderColor, Float& borderWidth );

	void SetTransitionColor( CGraphBlock* startBlock, CGraphBlock* endBlock, Color& linkColor );

	void GetGraphSnaphots(TDynArray<String>& snapshots);

	void DispatchEditorEvent( const CName& systemEvent, void* data );

	void CreateGlobalTransition( CClass* blockClass );

	virtual wxSize GetGraphArea() const;

	virtual const Char * ClipboardChannelName() const { return TXT("CEdBehaviorGraphEditor") ; } 
};

//////////////////////////////////////////////////////////////////////////

class CEdBehaviorGraphLayerEditor : public CEdBehaviorEditorSimplePanel
{
	DECLARE_EVENT_TABLE()

protected:
	wxListCtrl*		m_listLayer;
	wxListCtrl*		m_listVisible;
	wxListCtrl*		m_listFreeze;

	wxImageList*	m_imageList;

public:
	CEdBehaviorGraphLayerEditor( CEdBehaviorEditor* editor );

	virtual wxString	GetPanelName() const	{ return wxT("Layers"); }
	virtual wxString	GetPanelCaption() const { return wxT("Layers"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual void		OnReset();
	virtual void		OnNodesSelect( const TDynArray< CGraphBlock* >& nodes );
	virtual void		OnNodesDeselect();

protected:
	void OnLabelEditEnd( wxListEvent& event );
	void OnItemClickLayer( wxListEvent& event );
	void OnItemClickVisible( wxListEvent& event );
	void OnItemClickFreeze( wxListEvent& event );
	void OnAddNodes( wxCommandEvent& event );
	void OnRemoveNodes( wxCommandEvent& event );
	void OnClearLayer( wxCommandEvent& event );

protected:
	void CreateImageList();
	void FillList();
	void UpdateList();

	void GetSelectedLayers( TDynArray< Uint32 >& layers );
};
