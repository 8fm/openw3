#pragma once

#include "behTreeNodeStrafing.h"

class CBehTreeStrafingAlgorithmDefinition;
class CBehTreeStrafingAlgorithmInstance;
class CBehTreeStrafingAlgorithmListInstance;
class CBehTreeStrafingAlgorithmFastSurroundInstance;
class CBehTreeStrafingAlgorithmNeverBackDownInstance;

////////////////////////////////////////////////////////////////////////////
// Custom algorithms definitions
////////////////////////////////////////////////////////////////////////////
class CBehTreeStrafingAlgorithmDefinition : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CBehTreeStrafingAlgorithmDefinition, CObject );
public:
	typedef CBehTreeStrafingAlgorithmInstance Instance;

	virtual CBehTreeStrafingAlgorithmInstance* SpawnInstance( CBehTreeNodeStrafingInstance* node, CBehTreeSpawnContext& context ) const;
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehTreeStrafingAlgorithmDefinition );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

class CBehTreeStrafingAlgorithmListDefinition : public CBehTreeStrafingAlgorithmDefinition
{
	DECLARE_ENGINE_CLASS( CBehTreeStrafingAlgorithmListDefinition, CBehTreeStrafingAlgorithmDefinition, 0 );

	friend class CBehTreeStrafingAlgorithmListInstance;
public:
	typedef CBehTreeStrafingAlgorithmListInstance Instance;

	CBehTreeStrafingAlgorithmInstance* SpawnInstance( CBehTreeNodeStrafingInstance* node, CBehTreeSpawnContext& context ) const override;
protected:
	TDynArray< CBehTreeStrafingAlgorithmDefinition* >			m_list;
};

BEGIN_CLASS_RTTI( CBehTreeStrafingAlgorithmListDefinition );
	PARENT_CLASS( CBehTreeStrafingAlgorithmDefinition );
	PROPERTY_INLINED( m_list, TXT("Algorithms list") );
END_CLASS_RTTI();

class CBehTreeStrafingAlgorithmFastSurroundDefinition : public CBehTreeStrafingAlgorithmDefinition
{
	DECLARE_ENGINE_CLASS( CBehTreeStrafingAlgorithmFastSurroundDefinition, CBehTreeStrafingAlgorithmDefinition, 0 );

	friend class CBehTreeStrafingAlgorithmFastSurroundInstance;
protected:
	CBehTreeValFloat				m_usageDelay;
	CBehTreeValFloat				m_distanceToActivate;
	CBehTreeValFloat				m_speedMinToActivate;
	CBehTreeValFloat				m_distanceToBreak;
	CBehTreeValFloat				m_verticalHeadingLimitToBreak;
	CBehTreeValFloat				m_speedMinLimitToBreak;
	CBehTreeValEMoveType			m_surroundMoveType;
public:
	typedef CBehTreeStrafingAlgorithmFastSurroundInstance Instance;

	CBehTreeStrafingAlgorithmFastSurroundDefinition()
		: m_usageDelay( 6.f )
		, m_distanceToActivate( 5.f )
		, m_speedMinToActivate( 0.3f )
		, m_distanceToBreak( 1.0f )
		, m_verticalHeadingLimitToBreak( -1.f )
		, m_speedMinLimitToBreak( 0.5f )								{}

	CBehTreeStrafingAlgorithmInstance* SpawnInstance( CBehTreeNodeStrafingInstance* node, CBehTreeSpawnContext& context ) const override;
};
BEGIN_CLASS_RTTI( CBehTreeStrafingAlgorithmFastSurroundDefinition );
	PARENT_CLASS( CBehTreeStrafingAlgorithmDefinition );
	PROPERTY_EDIT( m_usageDelay, TXT("How often 'fast surround' behavior can be used. Note that this value is additionaly randomized each time.") );
	PROPERTY_EDIT( m_distanceToActivate, TXT("2 dimentional straight distance to desired place that activates behavior.") );
	PROPERTY_EDIT( m_speedMinToActivate, TXT("Minimal starting velocity needed to activate fast surround.") );
	PROPERTY_EDIT( m_distanceToBreak, TXT("2 dimentional straight distance to desired place that activates behavior.") );
	PROPERTY_EDIT( m_verticalHeadingLimitToBreak, TXT("If 'keep distance' heading value is below, fast surround is disabled.") );
	PROPERTY_EDIT( m_speedMinLimitToBreak, TXT("If velocity length is below this value, fast surround would also get disabled.") );
	PROPERTY_EDIT( m_surroundMoveType, TXT("Move type of surrounding behavior") );
END_CLASS_RTTI();

class CBehTreeStrafingAlgorithmNeverBackDownDefinition : public CBehTreeStrafingAlgorithmDefinition
{
	DECLARE_ENGINE_CLASS( CBehTreeStrafingAlgorithmNeverBackDownDefinition, CBehTreeStrafingAlgorithmDefinition, 0 );

	friend class CBehTreeStrafingAlgorithmNeverBackDownInstance;
public:
	typedef CBehTreeStrafingAlgorithmNeverBackDownInstance Instance;

	CBehTreeStrafingAlgorithmInstance* SpawnInstance( CBehTreeNodeStrafingInstance* node, CBehTreeSpawnContext& context ) const override;
};
BEGIN_CLASS_RTTI( CBehTreeStrafingAlgorithmNeverBackDownDefinition );
	PARENT_CLASS( CBehTreeStrafingAlgorithmDefinition );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////////
// Custom strafing additive algorithms instances implementations
////////////////////////////////////////////////////////////////////////////
class CBehTreeStrafingAlgorithmInstance
{
protected:
	CBehTreeNodeStrafingInstance*			m_node;

	// methods that directly access m_node using class friendship
	void LockOrientation( Bool b )											{ m_node->m_lockOrientation = b; }
	void SetMoveType( EMoveType moveType );
	void SetDefaultMoveType();

	Float GetDesiredRange() const											{ return m_node->m_currDesiredDistance; }
	Float GetCurrentTargetDistance() const									{ return m_node->m_currentTargetDistance; }
	Float GetCurrentDesiredAngleDistance() const							{ return m_node->m_desiredAngleDistance; }
	const CCombatDataPtr& GetCombatTarget() const							{ return m_node->m_targetData; }
	EMoveType GetDefaultMoveType() const									{ return m_node->GetDefaultMoveType(); }
	const Vector2& GetCurrentWorldHeading() const							{ return m_node->m_currentWorldHeading; }
	
public:
	typedef CBehTreeStrafingAlgorithmDefinition Definition;

	CBehTreeStrafingAlgorithmInstance( const Definition* def, CBehTreeNodeStrafingInstance* node, CBehTreeSpawnContext& context )
		: m_node( node )													{}
	virtual ~CBehTreeStrafingAlgorithmInstance()							{}

	virtual void Activate();
	virtual void Deactivate();
	virtual void UpdateHeading( Vector2& inoutHeading );
};

class CBehTreeStrafingAlgorithmListInstance : public CBehTreeStrafingAlgorithmInstance
{
	typedef CBehTreeStrafingAlgorithmInstance Super;
protected:
	TDynArray< CBehTreeStrafingAlgorithmInstance* >		m_list;
public:
	typedef CBehTreeStrafingAlgorithmListDefinition Definition;

	CBehTreeStrafingAlgorithmListInstance( const Definition* def, CBehTreeNodeStrafingInstance* node, CBehTreeSpawnContext& context );

	~CBehTreeStrafingAlgorithmListInstance();
	void Activate() override;
	void Deactivate() override;
	void UpdateHeading( Vector2& inoutHeading ) override;
};

class CBehTreeStrafingAlgorithmFastSurroundInstance : public CBehTreeStrafingAlgorithmInstance
{
	typedef CBehTreeStrafingAlgorithmInstance Super;
protected:
	// algorithm input data
	Float							m_usageDelay;
	Float							m_distanceToActivateSq;
	Float							m_speedMinToActivateSq;
	Float							m_distanceToBreakSq;
	Float							m_verticalHeadingLimitToBreak;
	Float							m_speedMinLimitToBreakSq;

	EMoveType						m_surroundMoveType;

	// runtime data
	Bool							m_isFastSurrounding;
	Bool							m_hasLowVelocity;
	Float							m_nextActivationDelay;
	Float							m_lowVelocityBreakDelay;
	Float							m_isFastSurroundingSince;
	


	void BreakFastSurround();
	void StartFastSurround();
public:
	typedef CBehTreeStrafingAlgorithmFastSurroundDefinition Definition;

	CBehTreeStrafingAlgorithmFastSurroundInstance( const Definition* def, CBehTreeNodeStrafingInstance* node, CBehTreeSpawnContext& context );

	void Activate() override;
	void UpdateHeading( Vector2& inoutHeading ) override;
};

class CBehTreeStrafingAlgorithmNeverBackDownInstance : public CBehTreeStrafingAlgorithmInstance
{
	typedef CBehTreeStrafingAlgorithmInstance Super;
protected:
public:
	typedef CBehTreeStrafingAlgorithmNeverBackDownDefinition Definition;

	CBehTreeStrafingAlgorithmNeverBackDownInstance( const Definition* def, CBehTreeNodeStrafingInstance* node, CBehTreeSpawnContext& context )
		: Super( def, node, context )										{}

	void UpdateHeading( Vector2& inoutHeading ) override;
};