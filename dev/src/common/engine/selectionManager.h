/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/events.h"

// Forward declarations
class CEntity;
class CComponent;
class CNode;
class CLayer;
class CEntityGroup;

/// Select mode
enum ESelectMode
{
	SM_ActiveLayer,
	SM_MultiLayer,
};

struct ISelectionFilter
{
public:
	virtual ~ISelectionFilter() {};

	virtual Bool FilterNode( CNode *entity ) = 0;
};

/// Global selection manager
class CSelectionManager
#ifndef NO_EDITOR_EVENT_SYSTEM
:	public IEdEventListener
#endif
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );
private:
    Int32  m_transactionsInProgress;
    Bool m_scheduleNodeSelectionChangedEvent;
    Bool m_scheduleLayerSelectionChangedEvent;

public:
	// Selection granularity
	enum ESelectionGranularity
	{
		SG_Components,		//!< Component fine-grain selection
		SG_Entities,		//!< Entity selection
	};

	class CSelectionTransaction
	{
		CSelectionManager*	m_manager;
		Uint32				m_id;
		static Uint32		sm_lastUsedId;

	public:
		CSelectionTransaction( CSelectionManager& manager );
		~CSelectionTransaction();
	};

    struct SSelectionEventData
    {
        CWorld *m_world;
        Uint32    m_transactionId;

        SSelectionEventData( CWorld *world, Uint32 transactionId = 0 )
            : m_world        ( world )
            , m_transactionId( transactionId )
        {}
    };

protected:
	TDynArray< CNode* >			m_selection;					//!< Selected nodes ( all )
	THandle< ISerializable >	m_layerSelection;				//!< Selected layer or layer group
	TDynArray< CNode* >			m_roots;						//!< Selection roots
	ESelectionGranularity		m_granularity;					//!< Selection granularity
	CNode*						m_pivot;						//!< Selection pivot
	Vector						m_pivotOffset;					//!< Selection pivot offset
	Vector						m_pivotPosition;				//!< Pivot position
	CWorld*						m_world;						//!< Owner
	ESelectMode					m_selectMode;					//!< Selection mode
	CLayerInfo*					m_activeLayer;					//!< Active layer
	Bool						m_isPivotUpdateScheduled;		//!< Is pivot update scheduled
	Bool						m_blockEvent;

    void SendNodeSelectionChangedEvent ( Uint32 transactionId = 0 );
    void SendLayerSelectionChangedEvent( Uint32 transactionId = 0 );

public:
	CSelectionManager( CWorld* world );
	~CSelectionManager();
	
#ifndef NO_EDITOR_EVENT_SYSTEM
	void DispatchEditorEvent( const CName& name, IEdEventData* data );
#endif

	// Set selection mode
	void SetSelectMode( ESelectMode selectMode );

	// Get selection mode
	ESelectMode GetSelectMode() const;

	// Set selection granularity
	void SetGranularity( ESelectionGranularity granularity );

	// Get selection granularity
	ESelectionGranularity GetGranularity() const { return m_granularity; }

	// Get selection pivot
	CNode* GetPivot() const;

	// Get selection pivot position
	Vector GetPivotPosition() const;

	// Get selection pivot rotation
	EulerAngles GetPivotRotation() const;

	// Get selection pivot offset
	Vector GetPivotOffset() const;

	// Get selection pivot local-to-world matrix
	Matrix GetPivotLocalToWorldMatrix() const;

	// Set an alternative pivot position
	void SetPivotPosition( const Vector& position );

	// Set a pivot offset
	void SetPivotOffset( const Vector& offset );

	// Refresh pivot with offset
	void RefreshPivot();

	// Set active layer
	void SetActiveLayer( CLayerInfo* activeLayer );

	// Get active layer
	CLayerInfo* GetActiveLayer() const;
	
	// Set selection state for node
	void SetSelection( CNode* node, Bool selected );

	// Select all components
	void SelectAll();

	// Gather all node from loaded layers
	void GatherAllNodes( TDynArray< CNode* >& nodeCollection );

	// Deselect all components
	void DeselectAll();

	// Invert selection for all node on the scene
	void InvertSelection();

	// Select all entities created from the same entity templates
	void SelectAllWithTheSameEntityTemplate();

	// Select all entities with the same tags
	void SelectByTags( const TagList& tagList );

	// Unselect all node from layer
	void DeselectOnLayer( CLayer *layer );

	// Destroy all selected nodes
	void DestroySelectedNodes();

	// Create a duplicate of the selection and select the duplicates
	TDynArray< CEntity* > DuplicateSelectedEntities();

	// Returns the number of selected objects
	Uint32 GetSelectionCount() const { return m_selection.Size(); }

	// Returns the number of selected entities
	Uint32 GetEntitiesSelectionCount() const;

	// Get selected nodes
	void GetSelectedNodes( TDynArray< CNode* >& nodes ) const;
	TDynArray< CNode* > GetSelectedNodes() const;
    
    // Get selected nodes
    void GetSelectedRoots( TDynArray< CNode* >& nodes ) const;
	TDynArray< CNode* > GetSelectedRoots() const;

	// Get selected entities (calculated from list of selected components)
	void GetSelectedEntities( TDynArray< CEntity* >& entities ) const;
	TDynArray< CEntity* > GetSelectedEntities() const;

	// Filter selected nodes
	void FilterSelection( ISelectionFilter *filter );

	// Get selected layer
	RED_INLINE ISerializable* GetSelectedLayer() const { return m_layerSelection.Get(); }

	// Get selected components
	void GetSelectedComponentsFiltered( CClass* filterClass, TDynArray< CComponent* >& components ) const;

	// Select node
	void Select( CNode* node, Bool ignoreGroups = false );

	// Select nodes (faster if you have many nodes)
	void Select( TDynArray< CNode* > nodes, Bool ignoreGroups = false );

	// Selects layer or layer group
	void SelectLayer( ISerializable* layer );

	// Select nodes from layer
	void SelectFromLayer( CLayer* layer );

	// Deselect single component
	void Deselect( CNode* node );

	// Deselect multiple nodes
	void Deselect( const TDynArray< CNode* >& nodes );

	// try to mark all selected objects as modified; 
	// returns true if all of them could be be marked so
	Bool ModifySelection();

	// Checks if given entity is a part of any group on the entire layer
	CEntityGroup* CheckIfPartOfAnyGroup( CEntity* entity );

protected:
	// Update selection pivot
	void UpdatePivot();

	// Get list of nodes to select/deselect based on current granularity and clicked node
	void GetNodeList( CNode* baseNode, TDynArray< CNode* >& nodes, TDynArray< CNode* >& roots, Bool ignoreGroups );

	// Checks if given entity is a part of given group or its child groups
	Bool CheckIfPartOfGroup( CEntity* entity, CEntityGroup* group );

	// Extracts entities from given group and its child groups
	void ExtractGroupNodes( CEntityGroup* group, TDynArray< CNode* >& nodes );
};
