/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "2daProperties.h"

class CInventoryComponent;
class CJobAction;
class CJobActionBase;
class CJobForceOutAction;

enum EJobTreeNodeSelectionMode
{
	SM_RANDOM,
	SM_SEQUENCE
};

BEGIN_ENUM_RTTI( EJobTreeNodeSelectionMode );
	ENUM_OPTION( SM_RANDOM );
	ENUM_OPTION( SM_SEQUENCE );
END_ENUM_RTTI();


class CJobTreeNode;

typedef TDynArray<CJobTreeNode*>::const_iterator JobNodeIterator;

enum EJobActionType
{
	JAT_ON_ENTER,
	JAT_CHILD,
	JAT_ON_LEAVE,
	JAT_DONE
};

struct SJobTreeTransitionState
{
	Uint32							m_currentNodeIndex;
	EJobActionType					m_actionType; 
	Uint32							m_currentLoop;
	TDynArray< CJobTreeNode* >		m_validChildNodes;

	SJobTreeTransitionState(): m_currentNodeIndex((Uint32)-1), m_currentLoop(0), m_actionType( JAT_ON_ENTER ) {}
};

struct SJobTreeExecutionContext
{
	DECLARE_RTTI_STRUCT( SJobTreeExecutionContext );

protected:

public:
	TDynArray<SJobTreeTransitionState>	m_transitionState;
	Uint32								m_currentLevel;
	Int32								m_lastRandomChoice;

	const CJobAction*					m_skippedItemEventsAction;
	CInventoryComponent*				m_workerInventoryComponent;
	CAnimatedComponent*					m_animatedComponent; // cached animated component ptr
	TDynArray<CName>					m_currentCategories;

	Vector								m_currentTranslation; //local
	Vector								m_nextTranslation;

	Float								m_currentRotation;
	Float								m_nextRotation;

	Float								m_currentActionDuration; // WARNING: its set only if 'm_trackExecutionPosition' is set

	CName								m_leftItem;
	CName								m_rightItem;
	CName								m_skippedEtryAnimItem;

	Bool								m_trackExecutionPosition;
	Bool								m_isLeaf;
	Bool								m_loopingJob;
	Bool								m_justGotStarted;
	Bool								m_isCurrentActionApproach;
	Bool								m_isCurrentActionLeave;

	SJobTreeExecutionContext() 
		: m_currentLevel( 0 )
		, m_lastRandomChoice ( -1 )
		, m_skippedItemEventsAction( nullptr )
		, m_workerInventoryComponent( nullptr )
		, m_animatedComponent( nullptr )
		, m_currentTranslation( 0.0f, 0.0f, 0.0f )
		, m_nextTranslation( 0.f, 0.f, 0.f )
		, m_currentRotation( 0.f )
		, m_nextRotation( 0.f )
		, m_currentActionDuration( 0.f )
		, m_leftItem( CNAME( Any ) )
		, m_rightItem( CNAME( Any ) )
		, m_skippedEtryAnimItem( CName::NONE )
		, m_trackExecutionPosition( true )
		, m_isLeaf( false )
		, m_loopingJob( false )
		, m_justGotStarted( true )
		, m_isCurrentActionApproach( false )
		, m_isCurrentActionLeave( false )
	{}

	void Reset( Bool clearCategories = true )
	{
		m_transitionState.ClearFast();
		m_currentLevel = 0;
		if ( clearCategories )
		{
			m_currentCategories.ClearFast();
		}
		m_currentTranslation.SetZeros();
		m_nextTranslation.SetZeros();
		m_currentRotation			= 0.f;
		m_nextRotation				= 0.f;
		m_currentActionDuration		= 0.f;
		m_isLeaf					= false;
		m_justGotStarted			= true;
		m_isCurrentActionApproach	= false;
		m_isCurrentActionLeave		= false;
		m_workerInventoryComponent	= nullptr;		
	}

	Uint32			GetTransitionLevel() const										{ return m_transitionState.Size(); }
	Bool			IsCurrentActionApproach() const									{ return m_isCurrentActionApproach; }
	Bool			IsCurrentActionLeave() const									{ return m_isCurrentActionLeave; }

	void			EnterNewTransitionalState( const CJobTreeNode* treeNode );
	void			ExitTransitionalState();
	void			AccumulateNewAction( const CJobActionBase* jobAction );
};

BEGIN_CLASS_RTTI( SJobTreeExecutionContext );
END_CLASS_RTTI();


class CJobTreeNode : public CObject
#ifndef NO_EDITOR
	, public CActionPointCategories2dPropertyOwner
#endif
{
	friend class CEdJobTreeEditor;
	friend class CJobTree;
	friend class CUndoJobEditorNodeExistance;
	friend class CUndoJobEditorNodeMove;
	friend class CUndoJobActionExistance;
	DECLARE_ENGINE_CLASS( CJobTreeNode, CObject, 0 );

private:
	CJobAction*					m_onEnterAction;		//!< Action returned before first child
	CJobAction*					m_onLeaveAction;		//!< Action returned after last child
	CJobForceOutAction*			m_onFastLeaveAction;	//!< Action to perform on forced job break
	TDynArray<CJobTreeNode*>	m_childNodes;			//!< child nodes
	TDynArray<CName>			m_validCategories;		//!< action categories valid for this node
	EJobTreeNodeSelectionMode	m_selectionMode;		//!< child node selection mode
	CName						m_leftItem;				//!< Item supposed to be in the left hand
	CName						m_rightItem;			//!< Item supposed to be in the right hand
	Bool						m_looped;				//!< Sequence will be looped infinitely (see CSimpleActionPointSelector::m_keepActionPointOnceSelected

protected:
	Uint32						m_iterations;			//!< number of randomization loops

	//! Get the next action node that suits the context (which means also the stage of job tree execution)
	const CJobActionBase* GetNextActionInternal( SJobTreeExecutionContext& context ) const;

	//! Get the next appropriate OnLeave action, if NULL returned - the job tree has ended
	const CJobActionBase* GetNextExitActionInternal( SJobTreeExecutionContext& context ) const;

	//! Get the force out action
	const CJobActionBase* GetNextForcedExitActionInternal( SJobTreeExecutionContext& context ) const;

public:
	CJobTreeNode() 
		: m_onEnterAction( NULL )
		, m_onLeaveAction( NULL )
		, m_onFastLeaveAction( NULL ) 
		, m_selectionMode( SM_SEQUENCE )
		, m_iterations( 1 )
		, m_leftItem( CNAME( Any ) )
		, m_rightItem( CNAME( Any ) )
		, m_looped( false )
	{}

	//! Initializes an execution context
	void InitExecutionContext( SJobTreeExecutionContext& outContext, CAnimatedComponent* animComp, Bool skipEntryAnimations ) const;

	//! Get the next action node that suits the context (which means also the stage of job tree execution)
	const CJobActionBase* GetNextAction( SJobTreeExecutionContext& context ) const;

	//! Get the next appropriate OnLeave action, if NULL returned - the job tree has ended
	const CJobActionBase* GetNextExitAction( SJobTreeExecutionContext& context ) const;

	//! Get the force out action
	const CJobActionBase* GetNextForcedExitAction( SJobTreeExecutionContext& context ) const;

	//! Returns true if job tree has animation name that starts with 'beginAnimName'
	Bool HasAnim( const String &beginAnimName ) const;

	//! Check if node has any of the categories specified
	Bool MeetsAnyCategory( const TDynArray< CName >& categories ) const;

	//! Check if tree contains endless loops
	Bool HasEndlessLoop() const;

	//! Get categories from this node
	const TDynArray<CName>& GetCategories() { return m_validCategories; }

	//! Recursive collection of the working places
	void CollectWorkingPlacesNames( const CJobTreeNode *node, TDynArray< CName >& placesNames /* out */ ) const;

	//! Recursively validate node
	Bool Validate( TDynArray< String >& log ) const;

	//! Get items required by this node, considering parent nodes
	void GetItemsRequiredByNode( CName& itemLeft, CName& itemRight ) const;

	//! Collect all valid categories from this node and childs
	void CollectValidCategories( TDynArray< CName >& categories ) const;

private:
	const CJobAction* GetOnEnterAction() const { return m_onEnterAction; }
	const CJobAction* GetOnLeaveAction() const { return m_onLeaveAction; }
	Bool HasChildren() const { return m_childNodes.Size()>0; }
	Bool HasAnim( const CJobTreeNode *node, const String &beginAnimName ) const;

private:
	void funcGetName( CScriptStackFrame& stack, void* result );
	void funcHasAnim( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CJobTreeNode );
	PARENT_CLASS( CObject );
	PROPERTY( m_onEnterAction );
	PROPERTY( m_onLeaveAction );
	PROPERTY( m_onFastLeaveAction );
	PROPERTY( m_childNodes );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_validCategories, TXT("Action categories that fit this node"), TXT("2daValueSelection") );
	PROPERTY_EDIT( m_selectionMode, TXT( "Child node selection method" ) );
	PROPERTY_EDIT( m_iterations, TXT( "Number of randomized selections, 0 means infinity" ) );
	PROPERTY_CUSTOM_EDIT( m_leftItem, TXT( "Item supposed to be in the left hand" ), TXT( "ChooseItem" ) );
	PROPERTY_CUSTOM_EDIT( m_rightItem, TXT( "Item supposed to be in the right hand" ), TXT( "ChooseItem" ) );
	PROPERTY_EDIT( m_looped, TXT("Loop this sequence infinitely") );
END_CLASS_RTTI();

