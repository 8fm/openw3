#pragma once

#include "behTreeCustomMoveData.h"
#include "behTreeNodeCondition.h"

class CBehTreeNodeConditionDistanceToCombatTargetInstance;
class CBehTreeNodeConditionDistanceToActionTargetInstance;
class CBehTreeNodeConditionDistanceToNamedTargetInstance;
class CBehTreeNodeConditionDistanceToCustomTargetInstance;
class CBehTreeNodeConditionDistanceToTaggedInstance;
////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionDistanceToCombatTargetDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionDistanceToCombatTargetDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionDistanceToCombatTargetInstance, ConditionDistanceToCombatTarget );
protected:	
	Bool									m_checkRotation;
	Float									m_rotationTolerance;
	CBehTreeValFloat						m_minDistance;
	CBehTreeValFloat						m_maxDistance;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionDistanceToCombatTargetDefinition() : 
		CBehTreeNodeConditionDefinition()
		  , m_checkRotation( false )
		  , m_rotationTolerance( 0.0f )
		  , m_minDistance( 0.0f )
		  , m_maxDistance ( 10.0f )										{}
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionDistanceToCombatTargetDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);	
	PROPERTY_EDIT( m_minDistance, TXT("Lower, accepted distance range"));
	PROPERTY_EDIT( m_maxDistance, TXT("Upper, accepted distance range"));
	PROPERTY_EDIT( m_checkRotation, TXT("Optional rotation towards the target check"));
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeConditionDistanceToCombatTargetInstance : public CBehTreeNodeConditionInstance
{
protected:
	Bool								m_checkRotation;
	Float								m_rotationTolerance;
	Float								m_minDistanceSq;
	Float								m_maxDistanceSq;

	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionDistanceToCombatTargetDefinition Definition;

	CBehTreeNodeConditionDistanceToCombatTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
};



////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionDistanceToActionTargetDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionDistanceToActionTargetDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionDistanceToActionTargetInstance, ConditionDistanceToActionTarget );
protected:	
	Bool								m_checkRotation;
	Float								m_rotationTolerance;
	CBehTreeValFloat					m_minDistance;
	CBehTreeValFloat					m_maxDistance;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionDistanceToActionTargetDefinition() : 
	  CBehTreeNodeConditionDefinition()
		  , m_checkRotation( false )
		  , m_rotationTolerance( 0.0f )
		  , m_minDistance( 0.0f )
		  , m_maxDistance ( 10.0f )										{}
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionDistanceToActionTargetDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);	
	PROPERTY_EDIT( m_minDistance, TXT("Lower, accepted distance range"));
	PROPERTY_EDIT( m_maxDistance, TXT("Upper, accepted distance range"));
	PROPERTY_EDIT( m_checkRotation, TXT("Optional rotation towards the target check"));
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeConditionDistanceToActionTargetInstance : public CBehTreeNodeConditionInstance
{
protected:
	Bool								m_checkRotation;
	Float								m_rotationTolerance;
	Float								m_minDistanceSq;
	Float								m_maxDistanceSq;

	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionDistanceToActionTargetDefinition Definition;

	CBehTreeNodeConditionDistanceToActionTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
};



////////////////////////////////////////////////////////////////////////
// Named Target Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionDistanceToNamedTargetDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionDistanceToNamedTargetDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionDistanceToNamedTargetInstance, ConditionDistanceToNamedTarget );
protected:	
	Bool								m_checkRotation;
	Float								m_rotationTolerance;
	CBehTreeValFloat					m_minDistance;
	CBehTreeValFloat					m_maxDistance;
	CBehTreeValCName					m_targetName;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionDistanceToNamedTargetDefinition() : 
		CBehTreeNodeConditionDefinition()
		, m_checkRotation( false )
		, m_rotationTolerance( 0.0f )
		, m_minDistance( 0.0f )
		, m_maxDistance ( 10.0f )										{}

	String GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionDistanceToNamedTargetDefinition);
PARENT_CLASS(CBehTreeNodeConditionDefinition);	
PROPERTY_EDIT( m_minDistance, TXT("Lower, accepted distance range"));
PROPERTY_EDIT( m_maxDistance, TXT("Upper, accepted distance range"));
PROPERTY_EDIT( m_checkRotation, TXT("Optional rotation towards the target check"));
PROPERTY_EDIT( m_targetName		, TXT( "" ) );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeConditionDistanceToNamedTargetInstance : public CBehTreeNodeConditionInstance
{
protected:
	Bool								m_checkRotation;
	Float								m_rotationTolerance;
	Float								m_minDistanceSq;
	Float								m_maxDistanceSq;
	CName								m_targetName;

	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionDistanceToNamedTargetDefinition Definition;

	CBehTreeNodeConditionDistanceToNamedTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
};



////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionDistanceToCustomTargetDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionDistanceToCustomTargetDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionDistanceToCustomTargetInstance, ConditionDistanceToCustomTarget );
protected:	
	Bool							m_checkRotation;
	Float							m_rotationTolerance;
	CBehTreeValFloat				m_minDistance;
	CBehTreeValFloat				m_maxDistance;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionDistanceToCustomTargetDefinition() : 
		CBehTreeNodeConditionDefinition()
		, m_checkRotation( false )
		, m_rotationTolerance( 0.0f )
		, m_minDistance( 0.0f )
		, m_maxDistance ( 10.0f )										{}
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionDistanceToCustomTargetDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);	
	PROPERTY_EDIT( m_minDistance, TXT("Lower, accepted distance range"));
	PROPERTY_EDIT( m_maxDistance, TXT("Upper, accepted distance range"));
	PROPERTY_EDIT( m_checkRotation, TXT("Optional rotation towards the target check"));
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeConditionDistanceToCustomTargetInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	Bool							m_checkRotation;
	Float							m_rotationTolerance;
	Float							m_minDistanceSq;
	Float							m_maxDistanceSq;
	CBehTreeCustomMoveDataPtr		m_customTarget;

	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionDistanceToCustomTargetDefinition Definition;

	CBehTreeNodeConditionDistanceToCustomTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
};


////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionDistanceToTaggedDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionDistanceToTaggedDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionDistanceToTaggedInstance, ConditionDistanceToTagged );
protected:	
	Bool								m_checkRotation;
	Float								m_rotationTolerance;
	CBehTreeValFloat					m_minDistance;
	CBehTreeValFloat					m_maxDistance;
	CBehTreeValCName					m_tag;
	Bool								m_allowActivationWhenNoTarget;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionDistanceToTaggedDefinition() : 
		CBehTreeNodeConditionDefinition()
		, m_checkRotation( false )
		, m_rotationTolerance( 0.0f )
		, m_minDistance( 0.0f )
		, m_maxDistance ( 10.0f )	
		, m_allowActivationWhenNoTarget( false ){}
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionDistanceToTaggedDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);	
	PROPERTY_EDIT( m_minDistance, TXT("Lower, accepted distance range"));
	PROPERTY_EDIT( m_maxDistance, TXT("Upper, accepted distance range"));
	PROPERTY_EDIT( m_checkRotation, TXT("Optional rotation towards the target check"));
	PROPERTY_EDIT( m_tag, TXT("Tag") );
	PROPERTY_EDIT( m_allowActivationWhenNoTarget, TXT("If flag is set to true goal will activate even if no target is found") );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeConditionDistanceToTaggedInstance : public CBehTreeNodeConditionInstance
{
protected:
	Bool								m_checkRotation;
	Float								m_rotationTolerance;
	Float								m_minDistanceSq;
	Float								m_maxDistanceSq;
	CName								m_tag;
	Bool								m_allowActivationWhenNoTarget;

	Bool ConditionCheck() override;

	CNode* GetTarget();
public:
	typedef CBehTreeNodeConditionDistanceToTaggedDefinition Definition;

	CBehTreeNodeConditionDistanceToTaggedInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
};