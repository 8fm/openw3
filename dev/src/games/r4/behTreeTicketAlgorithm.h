#pragma once

#include "../../common/game/behTreeNode.h"
#include "../../common/game/behTreeTask.h"

class IBehTreeTicketAlgorithmInstance;
class CBehTreeSpawnContext;
class CBehTreeTicketData;

////////////////////////////////////////////////////////////////////////////
// Base definition of ticket algorithm - object that
// calculates importance for tickets.
////////////////////////////////////////////////////////////////////////////
class IBehTreeTicketAlgorithmDefinition : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IBehTreeTicketAlgorithmDefinition, CObject );
protected:

public:
	virtual IBehTreeTicketAlgorithmInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeTicketAlgorithmDefinition )
	PARENT_CLASS( CObject )
END_CLASS_RTTI();

class IBehTreeTicketAlgorithmInstance
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Gameplay );

public:
	typedef IBehTreeTicketAlgorithmDefinition Definition;

	IBehTreeTicketAlgorithmInstance( const Definition& definition, CBehTreeInstance* owner, CBehTreeSpawnContext& context ) {}

	virtual ~IBehTreeTicketAlgorithmInstance()								{}

	virtual Float CalculateTicketImportance( IBehTreeNodeInstance* node, const CBehTreeTicketData& ticket, Uint16& count ) = 0;
};

////////////////////////////////////////////////////////////////////////////
// Base building blocks that modifies algorithms
////////////////////////////////////////////////////////////////////////////
class IBehTreeTicketAlgorithmDecoratorDefinition : public IBehTreeTicketAlgorithmDefinition
{
	friend class IBehTreeTicketAlgorithmDecoratorInstance;

	DECLARE_ENGINE_ABSTRACT_CLASS( IBehTreeTicketAlgorithmDecoratorDefinition, IBehTreeTicketAlgorithmDefinition );
protected:
	IBehTreeTicketAlgorithmDefinition*							m_baseAlgorithm;
public:
	IBehTreeTicketAlgorithmDecoratorDefinition()
		: m_baseAlgorithm( NULL )											{}
};
BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeTicketAlgorithmDecoratorDefinition );
	PARENT_CLASS( IBehTreeTicketAlgorithmDefinition );
	PROPERTY_INLINED( m_baseAlgorithm, TXT("Base algorithm") );
END_CLASS_RTTI();

class IBehTreeTicketAlgorithmDecoratorInstance : public IBehTreeTicketAlgorithmInstance
{
protected:
	IBehTreeTicketAlgorithmInstance*							m_baseAlgorithm;
public:
	typedef IBehTreeTicketAlgorithmDecoratorDefinition Definition;
	IBehTreeTicketAlgorithmDecoratorInstance( const Definition& definition, CBehTreeInstance* owner, CBehTreeSpawnContext& context );
	~IBehTreeTicketAlgorithmDecoratorInstance();
};

class CBehTreeTicketAlgorithmRandomizeDefinition : public IBehTreeTicketAlgorithmDecoratorDefinition
{
	friend class CBehTreeTicketAlgorithmRandomizeInstance;
	
	DECLARE_ENGINE_CLASS( CBehTreeTicketAlgorithmRandomizeDefinition, IBehTreeTicketAlgorithmDecoratorDefinition, 0 );
protected:
	CBehTreeValFloat											m_randMin;
	CBehTreeValFloat											m_randMax;
public:
	CBehTreeTicketAlgorithmRandomizeDefinition();

	IBehTreeTicketAlgorithmInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;
};
BEGIN_CLASS_RTTI( CBehTreeTicketAlgorithmRandomizeDefinition );
	PARENT_CLASS( IBehTreeTicketAlgorithmDecoratorDefinition );
	PROPERTY_EDIT( m_randMin, TXT("Minimal importance multiplier") );
	PROPERTY_EDIT( m_randMax, TXT("Maximal importance multiplier") );
END_CLASS_RTTI();


class CBehTreeTicketAlgorithmRandomizeInstance : public IBehTreeTicketAlgorithmDecoratorInstance
{
protected:
	Float														m_randMin;
	Float														m_randMax;
public:
	typedef CBehTreeTicketAlgorithmRandomizeDefinition Definition;
	CBehTreeTicketAlgorithmRandomizeInstance( const Definition& definition, CBehTreeInstance* owner, CBehTreeSpawnContext& context );

	Float CalculateTicketImportance( IBehTreeNodeInstance* node, const CBehTreeTicketData& ticket, Uint16& count ) override;
};

////////////////////////////////////////////////////////////////////////////
// Base building blocks that operate on lists of algorithms
////////////////////////////////////////////////////////////////////////////
class IBehTreeTicketAlgorithmListDefinition : public IBehTreeTicketAlgorithmDefinition
{
	friend class IBehTreeTicketAlgorithmListInstance;

	DECLARE_ENGINE_ABSTRACT_CLASS( IBehTreeTicketAlgorithmListDefinition, IBehTreeTicketAlgorithmDefinition );
protected:
	TDynArray< IBehTreeTicketAlgorithmDefinition* >				m_list;
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeTicketAlgorithmListDefinition );
	PARENT_CLASS( IBehTreeTicketAlgorithmDefinition );
	PROPERTY_INLINED( m_list, TXT("List of subalgorithms") );
END_CLASS_RTTI();

class IBehTreeTicketAlgorithmListInstance : public IBehTreeTicketAlgorithmInstance
{
protected:
	TDynArray< IBehTreeTicketAlgorithmInstance* >				m_list;
public:
	typedef IBehTreeTicketAlgorithmListDefinition Definition;

	IBehTreeTicketAlgorithmListInstance( const Definition& definition, CBehTreeInstance* owner, CBehTreeSpawnContext& context );

	virtual ~IBehTreeTicketAlgorithmListInstance();
};
////////////////////////////////////////////////////////////////////////////
// Sum algorithms list
////////////////////////////////////////////////////////////////////////////
class CBehTreeTicketAlgorithmListSumDefinition : public IBehTreeTicketAlgorithmListDefinition
{
	friend class IBehTreeTicketAlgorithmListSumInstance;

	DECLARE_ENGINE_CLASS( CBehTreeTicketAlgorithmListSumDefinition, IBehTreeTicketAlgorithmListDefinition, 0 );
public:
	IBehTreeTicketAlgorithmInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeTicketAlgorithmListSumDefinition );
	PARENT_CLASS( IBehTreeTicketAlgorithmListDefinition );
END_CLASS_RTTI();

class CBehTreeTicketAlgorithmListSumInstance : public IBehTreeTicketAlgorithmListInstance
{
public:
	typedef CBehTreeTicketAlgorithmListSumDefinition Definition;

	CBehTreeTicketAlgorithmListSumInstance( const Definition& definition, CBehTreeInstance* owner, CBehTreeSpawnContext& context )
		: IBehTreeTicketAlgorithmListInstance( definition, owner, context )	{}

	Float CalculateTicketImportance( IBehTreeNodeInstance* node, const CBehTreeTicketData& ticket, Uint16& count ) override;
};



////////////////////////////////////////////////////////////////////////////
// Premade native ticketing algorithms
////////////////////////////////////////////////////////////////////////////
class CBehTreeConstantTicketAlgorithmDefinition : public IBehTreeTicketAlgorithmDefinition
{
	friend class CBehTreeConstantTicketAlgorithmInstance;
	DECLARE_ENGINE_CLASS( CBehTreeConstantTicketAlgorithmDefinition, IBehTreeTicketAlgorithmDefinition, 0 )
protected:
	CBehTreeValFloat				m_importance;
public:
	IBehTreeTicketAlgorithmInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeConstantTicketAlgorithmDefinition )
	PARENT_CLASS( IBehTreeTicketAlgorithmDefinition )
	PROPERTY_EDIT( m_importance, TXT("Constant importance") )
END_CLASS_RTTI();

class CBehTreeConstantTicketAlgorithmInstance : public IBehTreeTicketAlgorithmInstance
{
protected:
	Float							m_importance;
public:
	typedef CBehTreeConstantTicketAlgorithmDefinition Definition;

	CBehTreeConstantTicketAlgorithmInstance( const Definition& definition, CBehTreeInstance* owner, CBehTreeSpawnContext& context )
		: IBehTreeTicketAlgorithmInstance( definition, owner, context )
		, m_importance( definition.m_importance.GetVal( context ) )			{}

	Float CalculateTicketImportance( IBehTreeNodeInstance* node, const CBehTreeTicketData& ticket, Uint16& count ) override;
};


class CBehTreeIfActiveTicketAlgorithmDefinition : public CBehTreeConstantTicketAlgorithmDefinition
{
	friend class CBehTreeIfActiveTicketAlgorithmInstance;
	DECLARE_ENGINE_CLASS( CBehTreeIfActiveTicketAlgorithmDefinition, CBehTreeConstantTicketAlgorithmDefinition, 0 )
public:
	IBehTreeTicketAlgorithmInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeIfActiveTicketAlgorithmDefinition )
	PARENT_CLASS( CBehTreeConstantTicketAlgorithmDefinition )
END_CLASS_RTTI();

class CBehTreeIfActiveTicketAlgorithmInstance : public CBehTreeConstantTicketAlgorithmInstance
{
protected:
	Float							m_importance;
public:
	typedef CBehTreeConstantTicketAlgorithmDefinition Definition;

	CBehTreeIfActiveTicketAlgorithmInstance( const Definition& definition, CBehTreeInstance* owner, CBehTreeSpawnContext& context )
		: CBehTreeConstantTicketAlgorithmInstance( definition, owner, context ) {}

	Float CalculateTicketImportance( IBehTreeNodeInstance* node, const CBehTreeTicketData& ticket, Uint16& count ) override;
};


struct CBehTreeDistanceBasedTicketAlgorithmField
{
	DECLARE_RTTI_STRUCT( CBehTreeDistanceBasedTicketAlgorithmField );

public:
	CBehTreeValFloat			m_distance;
	Float						m_importance;
};

BEGIN_CLASS_RTTI( CBehTreeDistanceBasedTicketAlgorithmField )
	PROPERTY_EDIT( m_distance, TXT("Distance") )
	PROPERTY_EDIT( m_importance, TXT("Importance") )
END_CLASS_RTTI();

class CBehTreeDistanceBasedTicketAlgorithmDefinition : public IBehTreeTicketAlgorithmDefinition
{
	friend class CBehTreeDistanceBasedTicketAlgorithmInstance;
	DECLARE_ENGINE_CLASS( CBehTreeDistanceBasedTicketAlgorithmDefinition, IBehTreeTicketAlgorithmDefinition, 0 )
protected:
	TDynArray< CBehTreeDistanceBasedTicketAlgorithmField >		m_distanceToImportance;
	CBehTreeValFloat											m_importanceMultiplier;
public:
	CBehTreeDistanceBasedTicketAlgorithmDefinition()
		: m_importanceMultiplier( 1.f )										{}

	IBehTreeTicketAlgorithmInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeDistanceBasedTicketAlgorithmDefinition )
	PARENT_CLASS( IBehTreeTicketAlgorithmDefinition )
	PROPERTY_EDIT( m_distanceToImportance, TXT("Constant importance") )
	PROPERTY_EDIT( m_importanceMultiplier, TXT("Base multiplier to importances") )
END_CLASS_RTTI();

class CBehTreeDistanceBasedTicketAlgorithmInstance : public IBehTreeTicketAlgorithmInstance
{
protected:
	TDynArray< Vector2 >			m_distanceToImportance;
public:
	typedef CBehTreeDistanceBasedTicketAlgorithmDefinition Definition;

	CBehTreeDistanceBasedTicketAlgorithmInstance( const Definition& definition, CBehTreeInstance* owner, CBehTreeSpawnContext& context );

	Float CalculateTicketImportance( IBehTreeNodeInstance* node, const CBehTreeTicketData& ticket, Uint16& count ) override;
};


class CBehTreeTimeBasedTicketAlgorithmDefinition : public IBehTreeTicketAlgorithmDefinition
{
	friend class CBehTreeTimeBasedTicketAlgorithmInstance;
	DECLARE_ENGINE_CLASS( CBehTreeTimeBasedTicketAlgorithmDefinition, IBehTreeTicketAlgorithmDefinition, 0 )
protected:
	CBehTreeValFloat											m_importanceMultiplier;
public:
	CBehTreeTimeBasedTicketAlgorithmDefinition()
		: m_importanceMultiplier( 1.f )										{}

	IBehTreeTicketAlgorithmInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeTimeBasedTicketAlgorithmDefinition )
	PARENT_CLASS( IBehTreeTicketAlgorithmDefinition )
	PROPERTY_EDIT( m_importanceMultiplier, TXT("Base multiplier to importances") )
END_CLASS_RTTI();

class CBehTreeTimeBasedTicketAlgorithmInstance : public IBehTreeTicketAlgorithmInstance
{
protected:
	Float														m_importanceMultiplier;
public:
	typedef CBehTreeTimeBasedTicketAlgorithmDefinition Definition;

	CBehTreeTimeBasedTicketAlgorithmInstance( const Definition& definition, CBehTreeInstance* owner, CBehTreeSpawnContext& context );

	Float CalculateTicketImportance( IBehTreeNodeInstance* node, const CBehTreeTicketData& ticket, Uint16& count ) override;
};