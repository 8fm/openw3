/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "formationPattern.h"

#include "formationMemberDataSlot.h"
#include "formationSlot.h"

///////////////////////////////////////////////////////////////////////////////
// RTTI stuff
///////////////////////////////////////////////////////////////////////////////
IMPLEMENT_RTTI_ENUM( EFormationConstraintType )

IMPLEMENT_ENGINE_CLASS( SFormationConstraintDefinition )
IMPLEMENT_ENGINE_CLASS( CFormationSlotDefinition )
IMPLEMENT_ENGINE_CLASS( IFormationPatternNode )
IMPLEMENT_ENGINE_CLASS( CFormationPatternCompositeNode )
IMPLEMENT_ENGINE_CLASS( CFormationPatternRepeatNode )
IMPLEMENT_ENGINE_CLASS( CFormationPatternSlotsNode )

///////////////////////////////////////////////////////////////////////////////
// SFormationConstraintDefinition
void SFormationConstraintDefinition::Instantiate( SFormationSlotConstraint& constraint, CAISlotFormationLeaderData& pattern, Uint32 currentSlotIndex ) const
{
	constraint.m_referenceSlot = m_referenceRelativeIndex ? Max( Int32(m_referenceSlot + currentSlotIndex), -1 ) : m_referenceSlot;
	constraint.m_type = m_type;
	constraint.m_value = m_value;
	constraint.m_strength = Max( m_strength, 0.1f );
	constraint.m_tolerance = m_tolerance;
}

///////////////////////////////////////////////////////////////////////////////
// CFormationSlotDefinition
void CFormationSlotDefinition::Instantiate( CFormationSlotInstance& instance, CAISlotFormationLeaderData& pattern, Uint32 currentSlotIndex, Uint32 currentSlotGeneration ) const
{
	instance.m_slotGeneration = currentSlotGeneration;

	instance.m_constraints.Resize( m_constraints.Size() );
	for ( Uint32 i = 0, n = m_constraints.Size(); i != n; ++i )
	{
		m_constraints[ i ].Instantiate( instance.m_constraints[ i ], pattern, currentSlotIndex );
	}
}

///////////////////////////////////////////////////////////////////////////////
// IFormationPatternNode::CParser
///////////////////////////////////////////////////////////////////////////////
IFormationPatternNode::CParser::CParser()
	: m_currentNode( NULL )
{

}
IFormationPatternNode::CParser::~CParser()
{

}

void IFormationPatternNode::CParser::Initialize( IFormationPatternNode* def )
{
	m_currentNode = def;
	SNodeParseInfo info;
	info.m_node = NULL;
	m_stack.PushBack( info );
	def->StartParsing( m_stack );
}

Bool IFormationPatternNode::CParser::Parse( TDynArray< CFormationSlotInstance >& slots, CAISlotFormationLeaderData& formation )
{
	const IFormationPatternNode* currentNode = m_currentNode.Get();

	while ( currentNode )
	{
		// try to collect slots from current node
		Bool collectedSlots = currentNode->GetFormationSlots( slots, formation );

		// progress parser to next node
		const IFormationPatternNode* nextNode = currentNode->NextNode( m_stack );

		// if we have nodes on that tree level
		while ( !nextNode )
		{
			// go back to parent node
			currentNode->EndParsing( m_stack );
			SNodeParseInfo info = m_stack.Back();
			m_stack.PopBackFast();
			currentNode = info.m_node;
			if ( !currentNode )
			{
				// basically we hit eot. Don't break function now cause its possible we collected some slots
				break;
			}

			// try to get new parent node child
			nextNode = currentNode->NextNode( m_stack );
		}

		if ( nextNode )
		{
			SNodeParseInfo info;
			info.m_node = currentNode;											// push 'return' node
			m_stack.PushBack( info );
			currentNode = nextNode;
			nextNode->StartParsing( m_stack );
			
		}

		if ( collectedSlots)
		{
			m_currentNode = currentNode;										// possibly null
			return true;
		}
	}

	// eot
	m_currentNode = nullptr;
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// IFormationPatternNode
///////////////////////////////////////////////////////////////////////////////
IFormationPatternNode::IFormationPatternNode()
{

}

void IFormationPatternNode::StartParsing( ParseStack& stack ) const
{
}
Bool IFormationPatternNode::GetFormationSlots( TDynArray< CFormationSlotInstance >& slots, CAISlotFormationLeaderData& formation ) const
{
	return false;
}
IFormationPatternNode* IFormationPatternNode::NextNode( ParseStack& stack ) const
{
	return NULL;
}
void IFormationPatternNode::EndParsing( ParseStack& stack ) const
{

}


///////////////////////////////////////////////////////////////////////////////
// CFormationPatternCompositeNode
///////////////////////////////////////////////////////////////////////////////
void CFormationPatternCompositeNode::StartParsing( ParseStack& stack ) const
{
	// push iteration index
	SNodeParseInfo info;
	info.m_int = 0;
	stack.PushBack( info );
}
IFormationPatternNode* CFormationPatternCompositeNode::NextNode( ParseStack& stack ) const
{
	// iterate
	Uint32 index = stack.Back().m_int;
	if ( index >= m_childNodes.Size() )
	{
		return NULL;
	}
	++stack.Back().m_int;
	return m_childNodes[ index ].Get();
}
void CFormationPatternCompositeNode::EndParsing( ParseStack& stack ) const
{
	stack.PopBackFast();
}

///////////////////////////////////////////////////////////////////////////////
// CFormationPatternRepeatNode
///////////////////////////////////////////////////////////////////////////////
void CFormationPatternRepeatNode::StartParsing( ParseStack& stack ) const
{
	// NOTICE: A trick here. We store loops count on stack b4 subclass data - so subclass can work, not knowing of our existance.

	// Push loops count
	SNodeParseInfo info;
	info.m_int = 0;
	stack.PushBack( info );

	// Call superclass method to push its iteration index
	TBaseClass::StartParsing( stack );
}
IFormationPatternNode* CFormationPatternRepeatNode::NextNode( ParseStack& stack ) const
{
	IFormationPatternNode* nextNode = TBaseClass::NextNode( stack );

	if ( !nextNode )
	{
		if ( m_repeatCount > 0 )
		{
			Int32 loopsCount = ++stack[ stack.Size() -2 ].m_int;
			if ( loopsCount >= m_repeatCount )
			{
				return NULL;
			}
		}
		
		stack.Back().m_int = 0;
		
		// NOTICE: Looks like duplicated code, but basically we need to assure that if TBaseClass::NextNode returns NULL
		// initially our function will also fail to prevent execution stall.
		nextNode = TBaseClass::NextNode( stack );
	}

	return nextNode;
}
void CFormationPatternRepeatNode::EndParsing( ParseStack& stack ) const
{
	// superclass cleanup
	TBaseClass::EndParsing( stack );

	// ma cleanup
	stack.PopBackFast();
}

///////////////////////////////////////////////////////////////////////////////
// CFormationPatternSlotsNode
///////////////////////////////////////////////////////////////////////////////
Bool CFormationPatternSlotsNode::GetFormationSlots( TDynArray< CFormationSlotInstance >& slots, CAISlotFormationLeaderData& formation ) const
{
	if ( m_slots.Empty() )
	{
		return false;
	}

	Uint32 generationIdx = formation.GetNextGeneration();
	Uint32 defIndex = 0;
	Uint32 baseIndex = slots.Size();
	slots.Grow( m_slots.Size() );
	for ( Uint32 idx = baseIndex, n = slots.Size(); idx < n; ++idx )
	{
		m_slots[ defIndex ].Instantiate( slots[ idx ], formation, idx, generationIdx );
		defIndex++;
	}

	return true;
}