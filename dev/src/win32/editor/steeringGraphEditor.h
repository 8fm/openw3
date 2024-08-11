/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "treeEditor.h"
#include "classHierarchyMapper.h"

class CEdSteeringEditor;
class IMoveSteeringNode;

/// Graph editor for behavior tree
class CEdSteeringGraphEditor : public CEdTreeEditor, public IEdEventListener
{
	DECLARE_EVENT_TABLE()

protected:
	CClassHierarchyMapper					m_blockClasses;			//!< Block types that may be spawned
	CClassHierarchyMapper					m_taskClasses;			//!< Task classes
	CClassHierarchyMapper					m_conditionClasses;		//!< Condition classes
	TDynArray< CClass* >					m_rlSelectorClasses;	//!< RL selector classes
	CMoveSteeringBehavior*					m_steering;				//!< Steering to edit
	CEdSteeringEditor*						m_editor;				//!< Tree editor
	THandle< IMoveSteeringNode >			m_contextMenuItem;
	static IMoveSteeringNode*				s_copiedBranch;			//!< Copied branch
	static Int32								s_numEditors;			//!< Number of editors

	THashSet< IMoveSteeringNode* >			m_activeNodes;

	static const Int32			OFFSET_X_MANY = 50;
	static const Int32			OFFSET_Y_MANY = 50;
	static const Int32			OFFSET_X_SINGLE = 0;
	static const Int32			OFFSET_Y_SINGLE = 50;

public:
	CEdSteeringGraphEditor( wxWindow * parent, IHook * hook );
	~CEdSteeringGraphEditor();

	void SetEditor( CEdSteeringEditor* editor ) { m_editor = editor; }

	//! Set steering to edit
	void SetSteering( CMoveSteeringBehavior* steering );

	//! Delete selection
	virtual void DeleteSelection() override { DeleteSelectedObjects(); }

	//! Force layout update
	virtual void ForceLayoutUpdate() override;

	// IEdEventListener
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data ) override;

protected:
	//! Open context menu
	virtual void OnOpenContextMenu() override;

	//! Get block friendly name
	virtual String GetBlockName( IScriptable & block ) const override;

	//! Get block friendly name
	virtual String GetBlockComment( IScriptable & block ) const override;

	//! Is this block locked
	virtual Bool IsLocked( IScriptable & block ) const override;

	//! Draw block links
	virtual void DrawBlockLinks( LayoutInfo & layout ) override;

	//! Paint
	virtual void PaintCanvas( Int32 width, Int32 height ) override;

	//! Fill node layout
	void FillNodeLayout( IMoveSteeringNode * node, LayoutInfo * parentLayout );

	//! Load block position from somewhere
	virtual Bool LoadBlockPosition( IScriptable * block, wxPoint & pos ) override;

	//! Save block position to somewhere
	virtual Bool SaveBlockPosition( IScriptable * block, const wxPoint & pos ) override;

	//! Delete selected objects
	void DeleteSelectedObjects( bool askUser = true );

	//! Move selected blocks override
//	virtual void MoveSelectedBlocks( wxPoint totalOffset, Bool alternate );

	//! Called when move ended
	virtual void OnMoveEnded() override;

protected:
	//! Is block in disabled branch
	Bool IsNodeInDisabledBranch( IMoveSteeringNode * node ) const;

	//! Check out file
	void OnCheckOut( wxCommandEvent& event );

	//! On block spawn
	void OnSpawnBlock( wxCommandEvent& event );
	void SpawnBlock( CClass* blockClass );

	//! When a condition is set on a condition node
	void OnSetCondition( wxCommandEvent& event );

	//! When a task is set on a task node
	void OnSetTask( wxCommandEvent& event );

	//! On copy branch
	void OnCopyBranch( wxCommandEvent& event );

	//! On cut branch
	void OnCutBranch( wxCommandEvent& event );

	//! On copy branch
	void OnPasteBranch( wxCommandEvent& event );

	//! On auto-layout
	void OnApplyDefaultLayout( wxCommandEvent& event );

	//! Get canvas color
	virtual wxColour GetCanvasColor() const override { return STEERING_EDITOR_BACKGROUND; }	

	//! Fill task classes
	void FillTaskClasses();

	//! Fill condition classes
	void FillConditionClasses();

	void ShowGameActiveInfo();

	//! Show must checkout info
	void ShowMustCheckoutInfo();

	//! marks the steering behavior resource as modified
	void MarkStructureModified();

private:
	void CopyActiveItem();
};
