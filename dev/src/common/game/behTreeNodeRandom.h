#pragma once

#include "behTreeNodeComposite.h"


////////////////////////////////////////////////////////////////////////
// Probability random selector
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeProbabilitySelectorInstance;

class CBehTreeNodeProbabilitySelectorDefinition : public IBehTreeNodeCompositeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeProbabilitySelectorDefinition, IBehTreeNodeCompositeDefinition, CBehTreeNodeProbabilitySelectorInstance, SelectRandom );
protected:
	static const Uint32 MAX_CHILDS = 6;

	Bool					m_testAvailability;
	Uint8					m_probability0;
	Uint8					m_probability1;
	Uint8					m_probability2;
	Uint8					m_probability3;
	Uint8					m_probability4;
	Uint8					m_probability5;

	IBehTreeNodeCompositeInstance* SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
	void CustomPostInstanceSpawn( IBehTreeNodeCompositeInstance* instance ) const;
public:
	CBehTreeNodeProbabilitySelectorDefinition()
		: m_testAvailability( false )
		, m_probability0( 25 )
		, m_probability1( 25 )
		, m_probability2( 25 )
		, m_probability3( 25 )
		, m_probability4( 25 )
		, m_probability5( 25 )											{}

	Bool CanAddChild() const override;

	void AddChild( IBehTreeNodeDefinition* node ) override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeProbabilitySelectorDefinition );
	PARENT_CLASS( IBehTreeNodeCompositeDefinition );	
	PROPERTY_EDIT( m_testAvailability, TXT("Should selected children be tested for their availability") );
	PROPERTY_EDIT_RANGE( m_probability0, TXT("Child 0 probability"), 0, 100 );
	PROPERTY_EDIT_RANGE( m_probability1, TXT("Child 1 probability"), 0, 100 );
	PROPERTY_EDIT_RANGE( m_probability2, TXT("Child 2 probability"), 0, 100 );
	PROPERTY_EDIT_RANGE( m_probability3, TXT("Child 3 probability"), 0, 100 );
	PROPERTY_EDIT_RANGE( m_probability4, TXT("Child 4 probability"), 0, 100 );
	PROPERTY_EDIT_RANGE( m_probability5, TXT("Child 5 probability"), 0, 100 );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Instance
////////////////////////////////////////////////////////////////////////

class CBehTreeNodeProbabilitySelectorInstance : public IBehTreeNodeCompositeInstance
{
	typedef IBehTreeNodeCompositeInstance Super;
protected:
	Bool					m_testAvailability;
	Uint8					m_probability0;
	Uint8					m_probability1;
	Uint8					m_probability2;
	Uint8					m_probability3;
	Uint8					m_probability4;
	Uint8					m_probability5;
	Uint16					m_overallProbability;
public:
	typedef CBehTreeNodeProbabilitySelectorDefinition Definition;

	CBehTreeNodeProbabilitySelectorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );

	//! Internal function run by definition after probabilities are all set
	void CalculateOverallProbabilityInternal();

	Bool Activate() override;
private:
	void SelectChild( Int16 random );
	void SelectChild( Int16 random, Bool* choices );
};