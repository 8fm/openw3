/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class SFormationSlotConstraint;
class CFormationSlotInstance;
class CAISlotFormationLeaderData;

// DEFINITION

enum EFormationConstraintType
{
	EFormationConstraint_POSITION,
	EFormationConstraint_SPEED,
};


BEGIN_ENUM_RTTI( EFormationConstraintType )
	ENUM_OPTION( EFormationConstraint_POSITION )
	ENUM_OPTION( EFormationConstraint_SPEED )
END_ENUM_RTTI()


struct SFormationConstraintDefinition
{
	DECLARE_RTTI_STRUCT( SFormationConstraintDefinition )

protected:
	Bool								m_referenceRelativeIndex;
	Int32								m_referenceSlot;
	EFormationConstraintType			m_type;
	Vector2								m_value;
	Float								m_strength;
	Float								m_tolerance;

public:
	SFormationConstraintDefinition()
		: m_referenceRelativeIndex( false )
		, m_referenceSlot( 0 )
		, m_type( EFormationConstraint_POSITION )
		, m_value( 0.f, 0.f )
		, m_strength( 1.f )
		, m_tolerance( 1.f )
	{

	}

	void Instantiate( SFormationSlotConstraint& constraint, CAISlotFormationLeaderData& pattern, Uint32 currentSlotIndex ) const;
};

BEGIN_NODEFAULT_CLASS_RTTI( SFormationConstraintDefinition )
	PROPERTY_EDIT( m_referenceRelativeIndex, TXT("If set referenceSlot index is relative to current slot insteady of absoulte value") )
	PROPERTY_EDIT( m_referenceSlot, TXT("Which formation flost constraint reference (-1: leader)") )
	PROPERTY_EDIT( m_type, TXT("Constraint type") )
	PROPERTY_EDIT( m_value, TXT("Constraint value. Use 'X' for scalars") )
	PROPERTY_EDIT( m_strength, TXT("Constraint strength.") )
	PROPERTY_EDIT( m_tolerance, TXT("Constraint value tolerance (interpretation is constraint dependent)") )
	END_CLASS_RTTI()

class CFormationSlotDefinition
{
	DECLARE_RTTI_SIMPLE_CLASS( CFormationSlotDefinition )

protected:
	TDynArray< SFormationConstraintDefinition >		m_constraints;

public:
	void Instantiate( CFormationSlotInstance& instance, CAISlotFormationLeaderData& pattern, Uint32 currentSlotIndex, Uint32 currentSlotGeneration ) const;
};

BEGIN_NODEFAULT_CLASS_RTTI( CFormationSlotDefinition )
	PROPERTY_EDIT( m_constraints, TXT("") )
END_CLASS_RTTI()

// Formation pattern definition.
// We define some kind of tree, which models something like 
// Way to get nodes from it is through CParser object, because possibly formation pattern would produce nodes in infinity (which will be often the case).
class IFormationPatternNode : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IFormationPatternNode, CObject )
protected:
	struct SNodeParseInfo
	{
		union
		{
			const IFormationPatternNode*	m_node;
			Uint32							m_int;
			Float							m_float;
		};
	};
	typedef TDynArray< SNodeParseInfo > ParseStack;

	virtual void					StartParsing( ParseStack& stack ) const;
	virtual Bool					GetFormationSlots( TDynArray< CFormationSlotInstance >& slots, CAISlotFormationLeaderData& formation ) const;
	virtual IFormationPatternNode*	NextNode( ParseStack& stack ) const;
	virtual void					EndParsing( ParseStack& stack ) const;

public:
	// 
	class CParser
	{
	protected:
		TDynArray< SNodeParseInfo >				m_stack;
		THandle< IFormationPatternNode >		m_currentNode;			// NOTICE: assertion - current node was visited (StartParsing collected), but GetFormationSlots wasn't yet called on it.

	public:
		CParser();
		~CParser();

		void Initialize( IFormationPatternNode* def );
		Bool Parse( TDynArray< CFormationSlotInstance >& slots, CAISlotFormationLeaderData& formation );
	};

	IFormationPatternNode();
};

BEGIN_ABSTRACT_CLASS_RTTI( IFormationPatternNode )
	PARENT_CLASS( CObject )
	END_CLASS_RTTI()

class CFormationPatternCompositeNode : public IFormationPatternNode
{
	DECLARE_ENGINE_CLASS( CFormationPatternCompositeNode, IFormationPatternNode, 0 )
protected:
	TDynArray< THandle< IFormationPatternNode > >	m_childNodes;

	virtual void					StartParsing( ParseStack& stack ) const override;
	virtual IFormationPatternNode*	NextNode( ParseStack& stack ) const override;
	virtual void					EndParsing( ParseStack& stack ) const override;
};

BEGIN_CLASS_RTTI( CFormationPatternCompositeNode )
	PARENT_CLASS( IFormationPatternNode )
	PROPERTY_INLINED( m_childNodes, TXT("Child nodes") )
	END_CLASS_RTTI()

class CFormationPatternRepeatNode : public CFormationPatternCompositeNode
{
	DECLARE_ENGINE_CLASS( CFormationPatternRepeatNode, CFormationPatternCompositeNode, 0 )
protected:
	Int32											m_repeatCount;

	virtual void					StartParsing( ParseStack& stack ) const override;
	virtual IFormationPatternNode*	NextNode( ParseStack& stack ) const override;
	virtual void					EndParsing( ParseStack& stack ) const override;
public:
	CFormationPatternRepeatNode()
		: m_repeatCount( -1 )												{}
};

BEGIN_CLASS_RTTI( CFormationPatternRepeatNode )
	PARENT_CLASS( CFormationPatternCompositeNode )
	PROPERTY_EDIT( m_repeatCount, TXT("Node repeat count (or infinitie <= 0)") )
	END_CLASS_RTTI()

class CFormationPatternSlotsNode : public IFormationPatternNode
{
	DECLARE_ENGINE_CLASS( CFormationPatternSlotsNode, IFormationPatternNode, 0 )
protected:
	TDynArray< CFormationSlotDefinition >			m_slots;

	virtual Bool					GetFormationSlots( TDynArray< CFormationSlotInstance >& slots, CAISlotFormationLeaderData& formation ) const override;
};

BEGIN_CLASS_RTTI( CFormationPatternSlotsNode )
	PARENT_CLASS( IFormationPatternNode )
	PROPERTY_INLINED( m_slots, TXT("Slot nodes") )
END_CLASS_RTTI()