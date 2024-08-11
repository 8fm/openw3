/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "jobTreeLeaf.h"
#include "jobTreeNode.h"
#include "../../common/engine/skeletalAnimationContainer.h"

IMPLEMENT_RTTI_ENUM( EJobTreeNodeSelectionMode );
IMPLEMENT_ENGINE_CLASS( SJobTreeExecutionContext );
IMPLEMENT_ENGINE_CLASS( CJobTreeNode );

//////////////////////////////////////////////////////////////////////////
void SJobTreeExecutionContext::EnterNewTransitionalState( const CJobTreeNode* treeNode )
{
	m_transitionState.PushBack( SJobTreeTransitionState() );
}

void SJobTreeExecutionContext::ExitTransitionalState()
{
	m_transitionState.PopBack();
}

void SJobTreeExecutionContext::AccumulateNewAction( const CJobActionBase* jobAction )
{
	if ( !jobAction )
	{
		return;
	}

	if ( m_trackExecutionPosition && m_animatedComponent )
	{
		m_currentTranslation = m_nextTranslation;
		m_currentRotation = m_nextRotation;

		Vector translation;
		Float rotation;
		jobAction->GetMotionExtraction( m_animatedComponent, translation, rotation, &m_currentActionDuration );
		
		if ( m_currentRotation )
		{
			translation.AsVector2() = MathUtils::GeometryUtils::Rotate2D( translation.AsVector2(), DEG2RAD( m_currentRotation ) );
		}

		m_nextTranslation += translation;
		m_nextRotation += rotation;
	}
}

//////////////////////////////////////////////////////////////////////////

const CJobActionBase* CJobTreeNode::GetNextAction( SJobTreeExecutionContext& context ) const
{
	const CJobActionBase* action = GetNextActionInternal( context );
	if ( action )
	{
		context.AccumulateNewAction( action );
	}
	return action;
}

const CJobActionBase* CJobTreeNode::GetNextExitAction( SJobTreeExecutionContext& context ) const
{
	const CJobActionBase* action = GetNextExitActionInternal( context );
	if ( action )
	{
		context.AccumulateNewAction( action );
	}
	return action;
}

const CJobActionBase* CJobTreeNode::GetNextForcedExitAction( SJobTreeExecutionContext& context ) const
{
	const CJobActionBase* action = GetNextForcedExitActionInternal( context );
	if ( action )
	{
		context.AccumulateNewAction( action );
	}
	return action;
}

void CJobTreeNode::InitExecutionContext( SJobTreeExecutionContext& outContext, CAnimatedComponent* animComp, Bool skipEntryAnimations ) const
{
	outContext.m_animatedComponent = animComp;
	if ( skipEntryAnimations )
	{
		const CJobActionBase* action = GetNextAction( outContext );
		if ( action && outContext.IsCurrentActionApproach() )
		{
			//action->GetMotionExtraction( animComp, outContext.m_currentTranslation, outContext.m_currentRotation );
			outContext.m_skippedItemEventsAction = Cast< CJobAction >( action );
		}
	}
}

const CJobActionBase* CJobTreeNode::GetNextActionInternal( SJobTreeExecutionContext& context ) const
{
	struct Guard
	{
		SJobTreeExecutionContext& m_c;
		Guard( SJobTreeExecutionContext& c )
			: m_c( c ) {}
		~Guard()
		{
			m_c.m_justGotStarted = false;
		}
	} g( context );

	context.m_isCurrentActionApproach = false;
	context.m_isCurrentActionLeave = false;
	context.m_isLeaf = false;

	Bool justGotStarted = context.m_justGotStarted;

	// if we enter this node for the first time, add it to the transition track
	if ( context.m_transitionState.Size() == context.m_currentLevel )
	{
		context.EnterNewTransitionalState( this );
	}
	SJobTreeTransitionState* state = &context.m_transitionState[context.m_currentLevel];
	if ( state->m_actionType == JAT_ON_ENTER )
	{
		// initialize transition state for this tree level
		state->m_actionType = JAT_CHILD;
		state->m_currentLoop = 1;
		// Fill valid nodes array so we won't randomize using nodes with no categories required
		for ( JobNodeIterator it = m_childNodes.Begin(); it!=m_childNodes.End(); ++it )
		{
			if ( (*it) && (*it)->MeetsAnyCategory( context.m_currentCategories ) )
			{
				state->m_validChildNodes.PushBack( *it );
			}
		}
		state->m_currentNodeIndex = 0;
 
		// if random selection enabled, adjust node iterator
		if ( m_selectionMode == SM_RANDOM && state->m_validChildNodes.Size()>0 )
		{
			Int32 cChild =  0;			
			if( state->m_validChildNodes.Size() > 1 )
			{
				cChild = GEngine->GetRandomNumberGenerator().Get< Int32 >( state->m_validChildNodes.Size() );
				if( cChild == context.m_lastRandomChoice  )
				{
					cChild = ( cChild + 1 ) % state->m_validChildNodes.Size();
				}				
			}
			context.m_lastRandomChoice = cChild;
			state->m_currentNodeIndex += cChild;
		}	

		if ( m_leftItem != CNAME( Any ) ) 
		{
			context.m_leftItem = m_leftItem;
		}
		if ( m_rightItem != CNAME( Any ) ) 
		{
			context.m_rightItem = m_rightItem;
		}

		// return onEnterAction if not null
		if ( m_onEnterAction ) 
		{
			if( !m_onEnterAction->GetIgnoreIfItemMounted() || !context.m_workerInventoryComponent || !context.m_workerInventoryComponent->IsItemHeld( m_onEnterAction->GetIgnoreIfItemMounted() ) )
			{			
				state->m_actionType = JAT_CHILD;

				// Check if current node is leaf node. Tis information will be used for optimization.
				if ( !m_onLeaveAction && m_childNodes.Empty() )
				{
					context.m_isLeaf = true;
				}

				if ( justGotStarted )
				{
					context.m_isCurrentActionApproach = true;
				}

				return m_onEnterAction;
			}
			else
			{
				context.m_skippedEtryAnimItem = m_onEnterAction->GetIgnoreIfItemMounted();
			}
		}	
	}
	if ( state->m_actionType == JAT_CHILD )
	{
		ASSERT( context.m_currentLevel < context.m_transitionState.Size() );
		ASSERT( m_iterations >= 0 );
		const CJobActionBase * action = NULL;
		Bool isAlreadyLooped = false;

		// search child nodes for the next action
		while ( !action && state->m_currentNodeIndex < state->m_validChildNodes.Size() )
		{
			// try to get the action recursively
			++context.m_currentLevel;
			action = state->m_validChildNodes[state->m_currentNodeIndex]->GetNextActionInternal( context );
			--context.m_currentLevel;

			if ( context.m_isCurrentActionLeave && m_onLeaveAction )
			{
				// this is not top-level leave action yet!
				context.m_isCurrentActionLeave = false;
			}

			// if there are no more action in the current child, proceed to the next
			if ( !action )
			{
				state = &context.m_transitionState[context.m_currentLevel];
				if ( m_selectionMode == SM_SEQUENCE )
				{
					++(state->m_currentNodeIndex);
					if ( state->m_currentNodeIndex == state->m_validChildNodes.Size() )
					{
						if ( ( ++state->m_currentLoop <= m_iterations || m_iterations == 0 || ( m_looped && context.m_loopingJob ) ) && !isAlreadyLooped ) 
						{
							state->m_currentNodeIndex = 0;
							isAlreadyLooped = true;
						}
					}
				}
				else // if randomized
				{
					++(state->m_currentLoop);
					// check if we didn't exceed max iterations for this node
					if ( state->m_currentLoop <= m_iterations || m_iterations == 0 || ( m_looped && context.m_loopingJob ) )
					{
						++state->m_currentNodeIndex;
						if ( ( state->m_currentNodeIndex + 1 ) > state->m_validChildNodes.Size() && !isAlreadyLooped)
						{
							state->m_currentNodeIndex = 0;
							isAlreadyLooped = true;
							continue;
						}						
					}	
					else
					{
						state->m_currentNodeIndex = state->m_validChildNodes.Size();
					}										
				}
			}
			// if action found, just return it
			else
			{
				return action;
			}
		}
		// we ran out of children, proceed to the ON_LEAVE action
		state->m_actionType = JAT_ON_LEAVE;
	}
	if ( state->m_actionType == JAT_ON_LEAVE )
	{
		// update action type state
		state->m_actionType = JAT_DONE;

		// return onLeaveAction
		if ( m_onLeaveAction )
		{
			if( !m_onLeaveAction->GetIgnoreIfItemMounted() 
				|| !context.m_workerInventoryComponent 
				|| ! ( context.m_workerInventoryComponent->IsItemHeld( m_onLeaveAction->GetIgnoreIfItemMounted() ) && context.m_skippedEtryAnimItem == m_onLeaveAction->GetIgnoreIfItemMounted()  ) 
			)
			{
				if ( m_leftItem != CNAME( Any ) ) 
				{
					context.m_leftItem = m_leftItem;
				}
				if ( m_rightItem != CNAME( Any ) ) 
				{
					context.m_rightItem = m_rightItem;
				}

				// check if we are last leave action
				context.m_isCurrentActionLeave = true;

				return m_onLeaveAction;
			}
		}
	}
	if ( state->m_actionType == JAT_DONE )
	{
		// the node is done definitely, pop back the transition state stack
		context.ExitTransitionalState();
	}

	return nullptr;
}

const CJobActionBase* CJobTreeNode::GetNextExitActionInternal( SJobTreeExecutionContext& context ) const
{
	context.m_isLeaf = false;
	context.m_justGotStarted = false;
	context.m_isCurrentActionApproach = false;
	const CJobActionBase* leaveAction = NULL;
	if ( context.m_transitionState.Size() == 0 )
	{
		return NULL;
	}
	if ( context.m_transitionState.Size()-1 > context.m_currentLevel )
	{
		++context.m_currentLevel;
		auto& nextActionNode = context.m_transitionState[context.m_currentLevel-1].m_validChildNodes[ context.m_transitionState[context.m_currentLevel-1].m_currentNodeIndex ];
		leaveAction = nextActionNode->GetNextExitActionInternal( context );
		--context.m_currentLevel;
		if ( leaveAction )
		{
			if ( m_onLeaveAction )
			{
				context.m_isCurrentActionLeave = false;
			}

			return leaveAction;
		}
	}
	context.ExitTransitionalState();

	context.m_isCurrentActionLeave = true;
	return m_onLeaveAction;
}

const CJobActionBase* CJobTreeNode::GetNextForcedExitActionInternal( SJobTreeExecutionContext& context ) const
{
	context.m_isLeaf = false;
	context.m_justGotStarted = false;
	context.m_isCurrentActionApproach = false;
	const CJobActionBase* forcedLeaveAction = NULL;
	if ( context.m_transitionState.Size() == 0 )
	{
		return NULL;
	}
	if ( context.m_transitionState.Size()-1 > context.m_currentLevel )
	{
		++context.m_currentLevel;

		auto& childNode = context.m_transitionState[context.m_currentLevel-1].m_validChildNodes[ context.m_transitionState[context.m_currentLevel-1].m_currentNodeIndex ];

		forcedLeaveAction = childNode->GetNextForcedExitActionInternal( context );
		--context.m_currentLevel;
		if ( forcedLeaveAction )
		{
			if ( m_onFastLeaveAction )
			{
				context.m_isCurrentActionLeave = false;
			}
			return forcedLeaveAction;
		}
	}

	context.m_isCurrentActionLeave = true;
	context.ExitTransitionalState();
	return m_onFastLeaveAction;
}

Bool CJobTreeNode::HasAnim( const String &animName ) const
{
	return HasAnim( this, animName );
}


Bool CJobTreeNode::MeetsAnyCategory( const TDynArray< CName >& categories ) const
{
	if ( m_validCategories.Empty() )
	{
		return false;
	}
	if ( m_childNodes.Empty() )
	{
		// If it is a leaf, use it ( categories apply to child nodes selection, not the on leave/ on enter )
		return true;
	}
	for ( Uint32 i=0; i<categories.Size(); ++i )
	{
		if ( m_validCategories.Exist( categories[i] ) )
		{
			return true;
		}
	}
	return false;
}

Bool CJobTreeNode::HasAnim( const CJobTreeNode *node, const String &animName ) const
{
	if ( (m_onEnterAction && m_onEnterAction->GetAnimName().AsString().BeginsWith( animName )) ||
		 (m_onLeaveAction && m_onLeaveAction->GetAnimName().AsString().BeginsWith( animName )) )
	{
		return true;
	}

	if ( node->HasChildren() )
	{
		for ( TDynArray< CJobTreeNode* >::const_iterator childNode = node->m_childNodes.Begin();
			  childNode != node->m_childNodes.End();
			  ++childNode )
		{
			if ( HasAnim( *childNode, animName ) )
			{
				return true;
			}
		}
	}

	return false;
}

void CJobTreeNode::CollectWorkingPlacesNames( const CJobTreeNode *node, TDynArray< CName >& placesNames /* out */ ) const
{
	if ( node->m_onEnterAction && node->m_onEnterAction->GetPlace() != CName::NONE )
	{
		placesNames.PushBackUnique( node->m_onEnterAction->GetPlace() );
	}
	if ( node->m_onLeaveAction && node->m_onLeaveAction->GetPlace() != CName::NONE )
	{
		placesNames.PushBackUnique( node->m_onLeaveAction->GetPlace() );
	}

	if ( node->HasChildren() )
	{
		for ( TDynArray< CJobTreeNode* >::const_iterator childNode = node->m_childNodes.Begin(); childNode != node->m_childNodes.End(); ++childNode )
		{
			CollectWorkingPlacesNames( *childNode, placesNames );
		}
	}
}

Bool CJobTreeNode::Validate( TDynArray< String >& log ) const
{
	Bool result = true;
	// Check if there are valid categories specified
	if ( m_validCategories.Empty() && !m_childNodes.Empty() )
	{
		log.PushBack( String::Printf( TXT("Node has no valid categories specified") ) );
		result = false;
	}
	if ( m_validCategories.Exist( CName::NONE ) )
	{
		log.PushBack( String::Printf( TXT("Node has 'None' category specified as valid") ) );
		result = false;
	}

	// Dive into child nodes
	for ( Uint32 i=0; i<m_childNodes.Size(); ++i )
	{
		if ( !m_childNodes[i]->Validate( log ) )
		{
			result = false;
		}
	}

	return result;
}

void CJobTreeNode::CollectValidCategories( TDynArray< CName >& categories ) const
{
	categories.PushBack( m_validCategories );

	for ( Uint32 i=0; i<m_childNodes.Size(); ++i )
	{
		m_childNodes[i]->CollectValidCategories( categories );
	}
}

const Uint32 MAX_ITERATIONS = 99;

Bool CJobTreeNode::HasEndlessLoop() const
{
	if ( m_iterations == 0 || m_iterations > MAX_ITERATIONS )
	{
		return true;
	}

	for ( TDynArray< CJobTreeNode* >::const_iterator childNode = m_childNodes.Begin(); childNode != m_childNodes.End();	++childNode )
	{
		if ( (*childNode)->HasEndlessLoop() )
		{
			return true;
		}
	}
	
	return false;
}

void CJobTreeNode::GetItemsRequiredByNode( CName& itemLeft, CName& itemRight ) const
{
	// Ask parent nodes for their items, and return them if this node accepts "Any"
	CJobTreeNode* parentNode = Cast< CJobTreeNode >( GetParent() );

	CName parentItemLeft = CNAME( Any );
	CName parentItemRight = CNAME( Any );

	if ( parentNode )
	{	
		parentNode->GetItemsRequiredByNode( parentItemLeft, parentItemRight );
	}

	if ( m_leftItem != CNAME( Any ) )
	{
		// this node specifies left item, return this one
		itemLeft = m_leftItem;
	}
	else
	{
		// this node doesn't specify left item ( it allows any item ), return the one from parent ( may be "Any" too )
		itemLeft = parentItemLeft;
	}

	if ( m_rightItem != CNAME( Any ) )
	{
		// this node specifies right item, return this one
		itemRight = m_rightItem;
	}
	else
	{
		// this node doesn't specify right item ( it allows any item ), return the one from parent ( may be "Any" too )
		itemRight = parentItemRight;
	}

}
