/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CJobActionBase;
class CJobTreeNode;

struct SJobAnimation
{
	String	m_category;
	CName	m_animation;

	RED_INLINE Bool IsEqual( const SJobAnimation& rhs ) const
	{
		return m_animation == rhs.m_animation && m_category == rhs.m_category;
	}
};

enum EJobMovementMode : CEnum::TValueType
{
	JMM_Walk,
	JMM_Run,
	JMM_CustomSpeed
};

BEGIN_ENUM_RTTI( EJobMovementMode );
	ENUM_OPTION( JMM_Walk );
	ENUM_OPTION( JMM_Run );
	ENUM_OPTION( JMM_CustomSpeed );
END_ENUM_RTTI();

struct SJobTreeSettings
{
	DECLARE_RTTI_STRUCT( SJobTreeSettings );

	Bool	m_leftRemoveAtEnd;
	Bool	m_rightRemoveAtEnd;
	Bool	m_leftDropOnBreak;
	Bool	m_rightDropOnBreak;
	Bool	m_ignoreHardReactions;
	Bool	m_needsPrecision;	
	Bool	m_isConscious;
	Int32	m_jobTreeType;
	Float   m_globalBreakingBlendOutTime;
	Bool	m_forceKeepIKactive;
	THandle< CJobTree > m_altJobTreeRes; //alternative jobtree in case this one blocks robed npc's

	SJobTreeSettings()
		: m_leftRemoveAtEnd( false )
		, m_rightRemoveAtEnd( false )
		, m_leftDropOnBreak( true )
		, m_rightDropOnBreak( true )
		, m_ignoreHardReactions( false )
		, m_needsPrecision( false )
		, m_isConscious( true )
		, m_altJobTreeRes( NULL )
		, m_jobTreeType( 0 )
		, m_globalBreakingBlendOutTime(0.5f)
		, m_forceKeepIKactive( false )
	{}
};

BEGIN_CLASS_RTTI( SJobTreeSettings );
	PROPERTY_EDIT( m_leftRemoveAtEnd,				TXT("Should left item be removed after work") );
	PROPERTY_EDIT( m_leftDropOnBreak,				TXT("Drop left item on break") );
	PROPERTY_EDIT( m_rightRemoveAtEnd,				TXT("Should right item be removed after work") );
	PROPERTY_EDIT( m_rightDropOnBreak,				TXT("Drop right item on break") );
	PROPERTY_EDIT( m_ignoreHardReactions,			TXT("If hard reactions should be ignored") );
	PROPERTY_EDIT( m_needsPrecision,				TXT("Needs precise placement before starting job") );
	PROPERTY_EDIT( m_isConscious,					TXT("If NPC is consiuos") );
	PROPERTY_EDIT( m_altJobTreeRes,					TXT("Alternative JobTree resource for robed npc's") );
	PROPERTY_EDIT( m_globalBreakingBlendOutTime,	TXT("Blend time used when JobTree breaks played animation( and fast stop anim isn't definied )") );
	PROPERTY_EDIT( m_forceKeepIKactive,				TXT("Keep IK active during work") );
	PROPERTY_CUSTOM_EDIT( m_jobTreeType	,			TXT("Job tree type"), TXT("ScriptedEnum_EJobTreeType") );
END_CLASS_RTTI();

class CJobTree : public CResource
{
	friend class CEdJobTreeEditor;
	DECLARE_ENGINE_RESOURCE_CLASS( CJobTree, CResource, "w2job", "Job tree" );

private:
	CJobTreeNode*		m_jobTreeRootNode;
	EJobMovementMode	m_movementMode;
	Float				m_customMovementSpeed;
	SJobTreeSettings	m_settings;

public:
	CJobTree() 
		: m_jobTreeRootNode( NULL )
		, m_movementMode( JMM_Walk )
		, m_customMovementSpeed( 1.0f )
	{}

	CJobTreeNode* GetRootNode() { return m_jobTreeRootNode; }	

	void SetRootNode( CJobTreeNode* node ) { m_jobTreeRootNode = node; }

	EJobMovementMode GetMovementMode() const { return m_movementMode; }
	void SetMovementMode( EJobMovementMode mode ) { m_movementMode = mode; }

	void SetSettings( const SJobTreeSettings& settings ) { m_settings = settings; }
	const SJobTreeSettings& GetSettings() { return m_settings; }

	Float	GetCustomSpeed() const { return m_customMovementSpeed; }
	void	SetCustomSpeed( Float speed ) { m_customMovementSpeed = speed; }

	// Enum used animations
	void EnumUsedAnimations( TDynArray< SJobAnimation >& anims ) const;

	//! Applies the exit settings to an NPC that's been executing the tree
	void ApplyJobTreeExitSettings( CNewNPC* npc, Bool fast, SJobTreeExecutionContext& context ) const;

	Bool KeepIKActive() const { return m_settings.m_forceKeepIKactive; }

private:

	// Enum used animations
	void EnumUsedAnimationsForNode( const CJobTreeNode* node, TDynArray< SJobAnimation >& anims ) const;
	void EnumUsedAnimationsForAction( const CJobActionBase* action, TDynArray< SJobAnimation >& anims ) const;

};

BEGIN_CLASS_RTTI(CJobTree);
	PARENT_CLASS(CResource);
	PROPERTY( m_jobTreeRootNode );
	PROPERTY( m_movementMode );
	PROPERTY( m_customMovementSpeed );
	PROPERTY( m_settings );
END_CLASS_RTTI();
