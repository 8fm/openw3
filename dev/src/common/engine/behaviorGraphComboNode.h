/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

#include "behaviorGraphStateNode.h"
#include "behaviorGraphTransitionForce.h"

class CBehaviorGraphAnimationNode;

enum EAttackDirection
{
	AD_Front = 0,
	AD_Left = 1,
	AD_Right = 2,
	AD_Back = 3,
	AD_Last = 4
};

BEGIN_ENUM_RTTI( EAttackDirection );
	ENUM_OPTION( AD_Front )
	ENUM_OPTION( AD_Left )
	ENUM_OPTION( AD_Right )
	ENUM_OPTION( AD_Back )
END_ENUM_RTTI();

enum EAttackDistance
{
	ADIST_Small = 0,
	ADIST_Medium = 1,
	ADIST_Large = 2,
	ADIST_Last = 3,
	ADIST_None = 4,
};

BEGIN_ENUM_RTTI( EAttackDistance );
	ENUM_OPTION( ADIST_Small )
	ENUM_OPTION( ADIST_Medium )
	ENUM_OPTION( ADIST_Large )
	ENUM_OPTION( ADIST_None )
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorComboAnimation
{
	DECLARE_RTTI_STRUCT( SBehaviorComboAnimation )

	SBehaviorComboAnimation() : m_weight( 1.f ) {}

	CName	m_animationAttack;
	CName	m_animationParry;
	Float	m_weight;
};

BEGIN_CLASS_RTTI( SBehaviorComboAnimation );
	PROPERTY_CUSTOM_EDIT( m_animationAttack, TXT("Attack animation name"), TXT("BehaviorAnimSelection") );
	PROPERTY_CUSTOM_EDIT( m_animationParry, TXT("Parry animation name"), TXT("BehaviorAnimSelection") );
	PROPERTY_EDIT( m_weight, TXT( "Weight" ) );
END_CLASS_RTTI();

struct SBehaviorComboDistance
{
	DECLARE_RTTI_STRUCT( SBehaviorComboDistance )

	SBehaviorComboDistance()
	{
		SBehaviorComboAnimation anim;
		m_animations.PushBack( anim );
	}

	TDynArray< SBehaviorComboAnimation > m_animations;
};

BEGIN_CLASS_RTTI( SBehaviorComboDistance );
	PROPERTY_EDIT( m_animations, TXT( "Animations" ) );
END_CLASS_RTTI();

struct SBehaviorComboDirection
{
	DECLARE_RTTI_STRUCT( SBehaviorComboDirection )

	SBehaviorComboDistance	m_distSmall;
	SBehaviorComboDistance	m_distMedium;
	SBehaviorComboDistance	m_distLarge;
};

BEGIN_CLASS_RTTI( SBehaviorComboDirection );
	PROPERTY_EDIT( m_distSmall, TXT( "Distance small" ) );
	PROPERTY_EDIT( m_distMedium, TXT( "Distance medium" ) );
	PROPERTY_EDIT( m_distLarge, TXT( "Distance large" ) );
END_CLASS_RTTI();

struct SBehaviorComboLevel
{
	DECLARE_RTTI_STRUCT( SBehaviorComboLevel )

	SBehaviorComboDirection		m_dirFront;
	SBehaviorComboDirection		m_dirBack;
	SBehaviorComboDirection		m_dirLeft;
	SBehaviorComboDirection		m_dirRight;

	CName						m_abilityRequired;
};

BEGIN_CLASS_RTTI( SBehaviorComboLevel );
	PROPERTY_EDIT( m_dirFront, TXT( "Front" ) );
	PROPERTY_EDIT( m_dirBack, TXT( "Back" ) );
	PROPERTY_EDIT( m_dirLeft, TXT( "Left" ) );
	PROPERTY_EDIT( m_dirRight, TXT( "Right" ) );
	PROPERTY_CUSTOM_EDIT( m_abilityRequired, TXT( "Ability required" ), TXT("AbilitySelection") );
END_CLASS_RTTI();

struct SBehaviorComboWay
{
	DECLARE_RTTI_STRUCT( SBehaviorComboWay )

	TDynArray< SBehaviorComboLevel >	m_levels;
};

BEGIN_CLASS_RTTI( SBehaviorComboWay );
	PROPERTY_EDIT( m_levels, TXT( "Combo levels" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorComboAttack
{
	DECLARE_RTTI_STRUCT( SBehaviorComboAttack )

	SBehaviorComboAttack() { Reset(); }

	Float				m_attackHitTime;
	Float				m_parryHitTime;

	Float				m_attackHitLevel;
	Float				m_parryHitLevel;

	//Vector				m_attackHitTimeTable;
	//Vector				m_parryHitTimeTable;

	//Vector				m_attackHitLevelTable;
	//Vector				m_parryHitLevelTable;

	Float				m_attackHitTime1,m_attackHitTime2,m_attackHitTime3;
	Float				m_parryHitTime1,m_parryHitTime2,m_parryHitTime3;
	Float				m_attackHitLevel1,m_attackHitLevel2,m_attackHitLevel3;
	Float				m_parryHitLevel1,m_parryHitLevel2,m_parryHitLevel3;

	Int32					m_level;
	Int32					m_type;
	EAttackDirection	m_direction;
	EAttackDistance		m_distance;

	Float				m_attackTime;
	Float				m_parryTime;

	CName				m_attackAnimation;
	CName				m_parryAnimation;

	Bool IsValid() const	{ return m_attackAnimation.Empty() == false; }
	void Reset()			{ m_attackAnimation = CName::NONE; m_parryAnimation = CName::NONE; }
};

BEGIN_CLASS_RTTI( SBehaviorComboAttack );
	PROPERTY( m_level );
	PROPERTY( m_type );
	PROPERTY( m_direction );
	PROPERTY( m_distance );
	PROPERTY( m_attackTime );
	PROPERTY( m_parryTime );
	PROPERTY( m_attackAnimation );
	PROPERTY( m_parryAnimation );
	
	PROPERTY( m_attackHitTime );
	PROPERTY( m_parryHitTime );
	PROPERTY( m_attackHitLevel );
	PROPERTY( m_parryHitLevel );

	PROPERTY( m_attackHitTime1 );
	PROPERTY( m_parryHitTime1 );
	PROPERTY( m_attackHitLevel1 );
	PROPERTY( m_parryHitLevel1 );

	PROPERTY( m_attackHitTime2 );
	PROPERTY( m_parryHitTime2 );
	PROPERTY( m_attackHitLevel2 );
	PROPERTY( m_parryHitLevel2 );

	PROPERTY( m_attackHitTime3 );
	PROPERTY( m_parryHitTime3 );
	PROPERTY( m_attackHitLevel3 );
	PROPERTY( m_parryHitLevel3 );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

// A combo state in a behavior state machine
class CBehaviorGraphComboStateNode	: public CBehaviorGraphStateNode
									, public IBehaviorGraphProperty
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphComboStateNode, CBehaviorGraphStateNode, "State machine", "State combo" );

public:
	struct SComboDef
	{
		Int32					m_way;
		EAttackDirection	m_dir;
		EAttackDistance		m_dist;
	};

	static const Int32						ROOT_IDLE_STATE = -1;
	static const Int32						ROOT_WORK_STATE = -2;

	static const Uint32						MAX_COMBO_PARTS = 3;

protected:
	TDynArray< SBehaviorComboWay >			m_comboWays;
	Float									m_cooldown;
	Float									m_blendForAnim;
	Float									m_blendInternal;
	Uint32									m_maxLevel;
	Bool									m_isConnected;

	String									m_idleAnimation;
	CName									m_comboEvent;
	CName									m_finishedEvent;

	CName									m_varComboWay;
	CName									m_varComboDist;
	CName									m_varComboDir;

protected:
	TInstanceVar< Uint32 >					i_level;
	TInstanceVar< Bool >					i_running;
	TInstanceVar< Float >					i_internalRootTimer;
	TInstanceVar< Int32 >						i_rootState;
	TInstanceVar< Float >					i_cooldownTimer;
	TInstanceVar< Float >					i_cooldownDuration;
	TInstanceVar< Float >					i_comboTimer;
	TInstanceVar< SBehaviorComboAttack >	i_comboNextAttack;
	TInstanceVar< Uint32 >					i_comboEvent;
	TInstanceVar< Bool >					i_hasVarComboWay;
	TInstanceVar< Bool >					i_hasVarComboDist;
	TInstanceVar< Bool >					i_hasVarComboDir;

protected:
	CBehaviorGraphAnimationNode*			m_slotA;
	CBehaviorGraphAnimationNode*			m_slotB;

public:
	CBehaviorGraphComboStateNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! created in editor
	virtual void OnSpawned( const GraphBlockSpawnInfo& info );

	//! Get block caption
	virtual String GetCaption() const;

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape() const;

	//! Get block border color
	virtual Color GetBorderColor() const;

	//! Get client area color
	virtual Color GetClientColor() const;

	//! rendering depth
	virtual EGraphBlockDepthGroup GetBlockDepthGroup() const;

#endif
public:
	//! IBehaviorGraphProperty implementation
	virtual CBehaviorGraph* GetParentGraph();

public:
	//! Update stage
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	//! Sample stage
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const; 

	//! Called on activation of node
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

	//! Called on deactivation of node
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	//! Reset stage to default state
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	//! Process event
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	//! Process force event
	virtual Bool ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	//! Push activation alpha through graph
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	//! Cache connections
	virtual void CacheConnections();

public:
	Bool IsConnected() const;
	Bool ReadyToStart( CBehaviorGraphInstance& instance ) const;
	Bool IsExternalBlending( CBehaviorGraphInstance& instance ) const;

	void InjectNextAnimation( CBehaviorGraphInstance& instance, const CName& animationAttack, const CName& animationParry ) const;

	void ProcessCooldown( CBehaviorGraphInstance& instance, Float timeDelta ) const;
	
	Float GetCooldownDuration( CBehaviorGraphInstance& instance ) const;
	Float GetCooldownProgress( CBehaviorGraphInstance& instance ) const;

	void SetCooldownDuration( CBehaviorGraphInstance& instance, Float duration ) const;

	virtual void CollectUsedAnimations( TDynArray< CName >& anims ) const;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetComboType() const;
	virtual String GetComboDesc( CBehaviorGraphInstance& instance ) const;
#endif

	Bool IsRunning( CBehaviorGraphInstance& instance ) const;

public:
	void SetComboLevel( CBehaviorGraphInstance& instance, Uint32 level ) const;
	Uint32 GetComboLevel( CBehaviorGraphInstance& instance ) const;
	
	Uint32 GetMaxComboLevel() const;

	void IncComboLevel( CBehaviorGraphInstance& instance ) const;
	void ResetComboLevel( CBehaviorGraphInstance& instance ) const;

protected:
	virtual void StartCombo( CBehaviorGraphInstance& instance ) const;
	virtual void StopCombo( CBehaviorGraphInstance& instance ) const;

	virtual Bool CanStartNextAttack( CBehaviorGraphInstance& instance ) const;
	virtual Bool CurrAttackFinished( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsAttackParried( CBehaviorGraphInstance& instance, SBehaviorComboAttack& attack ) const;
	virtual void OnStartNextAttack( CBehaviorGraphInstance& instance ) const;

	virtual Bool ProcessComboEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual void ProcessMainPose( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& pose ) const;

	virtual Bool HasCachedAllValues( CBehaviorGraphInstance& instance ) const;
	virtual Bool AutoStart() const;

public:
	virtual Bool CanProcessEvent( CBehaviorGraphInstance& instance ) const;

protected:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	CBehaviorGraphAnimationNode* CreateSlot();
#endif

	Bool FireFinishEvent( CBehaviorGraphInstance& instance ) const;
	void PlayNextAnimation( CBehaviorGraphInstance& instance, const CName& animation, Float restTime ) const;
	Bool SelectDefaultAnimation( CBehaviorGraphInstance& instance, const CName& animName ) const;

	void StartPlayingRoot( CBehaviorGraphInstance& instance, Bool force ) const;
	void StopPlayingRoot( CBehaviorGraphInstance& instance ) const;
	Bool IsPlayingRoot( CBehaviorGraphInstance& instance ) const;
	Bool IsRootBlending( CBehaviorGraphInstance& instance ) const;
	Float GetWeightForRoot( CBehaviorGraphInstance& instance ) const;
	void UpdateRoot( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	Bool HasNextAttack( CBehaviorGraphInstance& instance ) const;
	const SBehaviorComboAttack& GetNextAttack( CBehaviorGraphInstance& instance ) const;
	SBehaviorComboAttack ResetNextAttack( CBehaviorGraphInstance& instance ) const;
	Bool SelectNextAttack( CBehaviorGraphInstance& instance, const Uint32 currLevel, const SComboDef& def ) const;
	void StartNextAttack( CBehaviorGraphInstance& instance, Float restTime ) const;

	Float UpdateSteps( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	void ResetSlots( CBehaviorGraphInstance& instance ) const;

	Bool FillComboDefinition( CBehaviorGraphInstance& instance, SComboDef& def ) const;
	Bool HasAbility( const CName& ability, CBehaviorGraphInstance& instance ) const;
	
	Float GetAnimBlendWeight( CBehaviorGraphInstance& instance ) const;
	Bool GetHitDataFromAnim( const CSkeletalAnimationSetEntry* anim, Vector& hitTimes, Vector& hitLevels ) const;

	Bool GetHitDataFromAnim( const CSkeletalAnimationSetEntry* anim, 
		Float& hitTimes0, Float& hitTimes1, Float& hitTimes2, Float& hitTimes3, 
		Float& hitLevels0, Float& hitLevels1, Float& hitLevels2, Float& hitLevels3 ) const;

	void SortEvents( TDynArray< const CExtAnimHitEvent* >& events ) const;
	const CSkeletalAnimationSetEntry* FindAnimation( CBehaviorGraphInstance& instance, const CName& animName ) const;

	Uint32							GetNextComboLevel( const SBehaviorComboWay& way, Uint32 level ) const;
	const SBehaviorComboDirection&	GetComboDir( const SBehaviorComboLevel& level, EAttackDirection dir ) const;
	const SBehaviorComboDistance&	GetComboDist( const SBehaviorComboDirection& dir, EAttackDistance dist ) const;
	const SBehaviorComboAnimation	GetComboAnim( const SBehaviorComboDistance& dist ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphComboStateNode );
	PARENT_CLASS( CBehaviorGraphStateNode );
	PROPERTY( m_isConnected );
	PROPERTY_EDIT( m_comboWays, TXT("Combo ways") );
	PROPERTY_EDIT( m_cooldown, TXT("Cooldown") );
	PROPERTY_EDIT( m_blendForAnim, TXT("Blend for combo animations") );
	PROPERTY_EDIT( m_blendInternal, TXT("Internal blend duration") );
	PROPERTY_EDIT( m_maxLevel, TXT("Max level for combo") );
	PROPERTY_CUSTOM_EDIT( m_comboEvent, TXT("Combo event"), TXT("BehaviorEventEdition") );
	PROPERTY_CUSTOM_EDIT( m_finishedEvent, TXT("Combo finished event"), TXT("BehaviorEventEdition") );
	PROPERTY_CUSTOM_EDIT( m_varComboWay, TXT("Combo way variable name"), TXT("BehaviorVariableSelection") );
	PROPERTY_CUSTOM_EDIT( m_varComboDist, TXT("Combo distance variable name"), TXT("BehaviorVariableSelection") );
	PROPERTY_CUSTOM_EDIT( m_varComboDir, TXT("Combo direction variable name"), TXT("BehaviorVariableSelection") );
	PROPERTY( m_slotA );
	PROPERTY( m_slotB );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphOffensiveComboStateNode : public CBehaviorGraphComboStateNode
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphOffensiveComboStateNode, CBehaviorGraphComboStateNode, "State machine.State Combo", "Offensive" );

protected:
	CName							m_allowAttackEvent;

protected:
	TInstanceVar< Bool >			i_allowAttack;
	TInstanceVar< Bool >			i_comboEventTime;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const; 

public:
	virtual void StartCombo( CBehaviorGraphInstance& instance ) const;
	virtual void StopCombo( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetComboType() const;
	virtual String GetComboDesc( CBehaviorGraphInstance& instance ) const;
#endif

	Bool IsComboEventOccurred( CBehaviorGraphInstance& instance ) const;
	Bool IsAllowAttackEventOccurred( CBehaviorGraphInstance& instance ) const;

protected:
	virtual Bool ProcessComboEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual void ProcessMainPose( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& pose ) const;
	virtual Bool CanStartNextAttack( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsAttackParried( CBehaviorGraphInstance& instance, SBehaviorComboAttack& attack ) const; 

	Bool ComboEventOccurred( const SBehaviorGraphOutput& pose ) const;
	Bool AllowAttackEventOccurred( const SBehaviorGraphOutput& pose ) const;

public:
	virtual Bool CanProcessEvent( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphOffensiveComboStateNode );
	PARENT_CLASS( CBehaviorGraphComboStateNode );
	PROPERTY_EDIT( m_allowAttackEvent, TXT("Allow attack event name") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphDefensiveComboStateNode : public CBehaviorGraphComboStateNode
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphDefensiveComboStateNode, CBehaviorGraphComboStateNode, "State machine.State Combo", "Defensive" );	

protected:
	CName						m_varHitTime;
	CName						m_varLevel;
	CName						m_varParry;
	TDynArray< CName >			m_defaultHits;

protected:
	TInstanceVar< Bool >			i_hasVarHitTime;
	TInstanceVar< Bool >			i_hasVarLevel;
	TInstanceVar< Bool >			i_hasVarParry;
	TInstanceVar< Float >		i_timeToNextAttack;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

protected:
	virtual void StartCombo( CBehaviorGraphInstance& instance ) const;
	virtual void StopCombo( CBehaviorGraphInstance& instance ) const;

	virtual Bool ProcessComboEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual Bool HasCachedAllValues( CBehaviorGraphInstance& instance ) const;
	virtual Bool CanStartNextAttack( CBehaviorGraphInstance& instance ) const;
	virtual Bool CurrAttackFinished( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsAttackParried( CBehaviorGraphInstance& instance, SBehaviorComboAttack& attack ) const; 

	virtual Bool AutoStart() const;

public:
	virtual Bool CanProcessEvent( CBehaviorGraphInstance& instance ) const;

protected:
	Uint32 GetLevelFromVariable( CBehaviorGraphInstance& instance ) const;
	Bool IsHit( CBehaviorGraphInstance& instance ) const;
	Float GetAttackHitTime( CBehaviorGraphInstance& instance ) const;
	Float GetBlockHitTime( CBehaviorGraphInstance& instance, Bool isHit ) const;
	void SetTimeToNextAttack( CBehaviorGraphInstance& instance, Float time ) const;
	void ResetTimeToNextAttack( CBehaviorGraphInstance& instance ) const;

	Bool HasDefaultHitAnimation( CBehaviorGraphInstance& instance ) const;
	Bool SelectDefaultHitAnimation( CBehaviorGraphInstance& instance ) const;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetComboType() const;
	virtual String GetComboDesc( CBehaviorGraphInstance& instance ) const;
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphDefensiveComboStateNode );
	PARENT_CLASS( CBehaviorGraphComboStateNode );
	PROPERTY_CUSTOM_EDIT( m_varHitTime, TXT("Hit time variable name"), TXT("BehaviorVariableSelection") );
	PROPERTY_CUSTOM_EDIT( m_varLevel, TXT("Hit time variable name"), TXT("BehaviorVariableSelection") );
	PROPERTY_CUSTOM_EDIT( m_varParry, TXT("Hit time variable name"), TXT("BehaviorVariableSelection") );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_defaultHits, TXT("Default hit animations"), TXT("BehaviorAnimSelection") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorComboAnim
{
	DECLARE_RTTI_STRUCT( SBehaviorComboAnim )

	CName	m_animationAttack;
	CName	m_animationParry;
	Int32		m_id;
};

BEGIN_CLASS_RTTI( SBehaviorComboAnim );
	PROPERTY_CUSTOM_EDIT( m_animationAttack, TXT("Attack animation name"), TXT("BehaviorAnimSelection") );
	PROPERTY_CUSTOM_EDIT( m_animationParry, TXT("Parry animation name"), TXT("BehaviorAnimSelection") );
	PROPERTY_EDIT( m_id, TXT( "ID" ) );
END_CLASS_RTTI();

struct SBehaviorComboElem
{
	DECLARE_RTTI_STRUCT( SBehaviorComboElem )

	CName							m_enum;
	TDynArray< SBehaviorComboAnim >	m_animations;
};

BEGIN_CLASS_RTTI( SBehaviorComboElem );
	PROPERTY_EDIT( m_animations, TXT("Combo animations") );
	PROPERTY_EDIT( m_enum, TXT( "Enum id" ) );
END_CLASS_RTTI();

class CBehaviorGraphSimpleDefensiveComboStateNode : public CBehaviorGraphComboStateNode
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphSimpleDefensiveComboStateNode, CBehaviorGraphComboStateNode, "State machine.State Combo", "Hit" );	

protected:
	CName							m_varHitTime;
	CName							m_varHitLevel;
	CName							m_varElemEnum;
	CName							m_varParry;
	
	CName							m_enum;

	TDynArray< SBehaviorComboElem >	m_animElems;

protected:
	TInstanceVar< Bool >				i_hasVarHitTime;
	TInstanceVar< Bool >				i_hasVarHitLevel;
	TInstanceVar< Bool >				i_hasVarHit;
	TInstanceVar< Bool >				i_hasVarParry;

	TInstanceVar< Float >			i_timeToNextAttack;
	TInstanceVar< Int32 >				i_attackType;
	TInstanceVar< Bool >			i_attackHitFlag;
	TInstanceVar< Vector >			i_attackTimesTable;
	TInstanceVar< Vector >			i_attackLevelTable;

	TInstanceVar< TDynArray< Int32 > > i_animationMap;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

protected:
	virtual void StartCombo( CBehaviorGraphInstance& instance ) const;
	virtual void StopCombo( CBehaviorGraphInstance& instance ) const;

	virtual Bool ProcessComboEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual Bool HasCachedAllValues( CBehaviorGraphInstance& instance ) const;
	virtual Bool CanStartNextAttack( CBehaviorGraphInstance& instance ) const;
	virtual Bool CurrAttackFinished( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsAttackParried( CBehaviorGraphInstance& instance, SBehaviorComboAttack& attack ) const;
	virtual void OnStartNextAttack( CBehaviorGraphInstance& instance ) const;

	virtual Bool AutoStart() const;

public:
	virtual Bool CanProcessEvent( CBehaviorGraphInstance& instance ) const;

protected:
	void FillAnimationIndexTable( CBehaviorGraphInstance& instance ) const;
	Bool InternalSelectNextAttackPart( CBehaviorGraphInstance& instance ) const;
	Bool SelectNextAttack( CBehaviorGraphInstance& instance, Int32 attackType, Int32 attackId ) const;
	Int32 GetAttackHit( CBehaviorGraphInstance& instance ) const;
	Bool IsHit( CBehaviorGraphInstance& instance ) const;
	Vector GetAttackHitTime( CBehaviorGraphInstance& instance ) const;
	Vector GetAttackHitLevel( CBehaviorGraphInstance& instance ) const;
	Float GetBlockHitTime( CBehaviorGraphInstance& instance, Bool isHit ) const;
	void SetTimeToNextAttack( CBehaviorGraphInstance& instance, Float time ) const;
	void ResetTimeToNextAttack( CBehaviorGraphInstance& instance ) const;
	void ShiftAttackTable( Vector& table, const Float defValue ) const;
	Bool HasNextAttackPart( CBehaviorGraphInstance& instance ) const;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetComboType() const;
	virtual String GetComboDesc( CBehaviorGraphInstance& instance ) const;
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphSimpleDefensiveComboStateNode );
	PARENT_CLASS( CBehaviorGraphComboStateNode );
	PROPERTY_CUSTOM_EDIT( m_varHitTime, TXT("Hit time variable name - vector"), TXT("BehaviorVectorVariableSelection") );
	PROPERTY_CUSTOM_EDIT( m_varHitLevel, TXT("Hit level variable name - vector"), TXT("BehaviorVectorVariableSelection") );
	PROPERTY_CUSTOM_EDIT( m_varElemEnum, TXT("Hit name - float"), TXT("BehaviorVariableSelection") );
	PROPERTY_CUSTOM_EDIT( m_varParry, TXT("Hit time variable name - float"), TXT("BehaviorVariableSelection") );
	PROPERTY_CUSTOM_EDIT( m_enum, TXT("Enum"), TXT("EnumList") );
	PROPERTY_EDIT( m_animElems, TXT("Animations") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class IBehaviorGraphComboModifier : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IBehaviorGraphComboModifier, CObject );

	virtual void Modify( CBehaviorGraphInstance& instance, CBehaviorGraphComboStateNode* combo ) const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehaviorGraphComboModifier );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

class CBehaviorGraphComboLevelModifier : public IBehaviorGraphComboModifier
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphComboLevelModifier, IBehaviorGraphComboModifier, 0 );

public:
	virtual void Modify( CBehaviorGraphInstance& instance, CBehaviorGraphComboStateNode* combo ) const;

private:
	Uint32 m_level;
};

BEGIN_CLASS_RTTI( CBehaviorGraphComboLevelModifier )
	PARENT_CLASS( IBehaviorGraphComboModifier );
	PROPERTY_EDIT( m_level, TXT("Set level") );
END_CLASS_RTTI();

class CBehaviorGraphComboStartingAnimationModifier : public IBehaviorGraphComboModifier
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphComboStartingAnimationModifier, IBehaviorGraphComboModifier, 0 );

public:
	virtual void Modify( CBehaviorGraphInstance& instance, CBehaviorGraphComboStateNode* combo ) const;

private:
	CName m_animationAttack;
	CName m_animationParry;
};

BEGIN_CLASS_RTTI( CBehaviorGraphComboStartingAnimationModifier )
	PARENT_CLASS( IBehaviorGraphComboModifier );
	PROPERTY_CUSTOM_EDIT( m_animationAttack, TXT("Attack animation name"), TXT("BehaviorAnimSelection") );
	PROPERTY_CUSTOM_EDIT( m_animationParry, TXT("Parry animation name"), TXT("BehaviorAnimSelection") );
END_CLASS_RTTI();

class CBehaviorGraphComboCooldownModifier : public IBehaviorGraphComboModifier
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphComboCooldownModifier, IBehaviorGraphComboModifier, 0 );

public:
	virtual void Modify( CBehaviorGraphInstance& instance, CBehaviorGraphComboStateNode* combo ) const;

private:
	Float m_cooldown;
};

BEGIN_CLASS_RTTI( CBehaviorGraphComboCooldownModifier )
	PARENT_CLASS( IBehaviorGraphComboModifier );
	PROPERTY_EDIT( m_cooldown, TXT("Set cooldown") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphComboTransitionInterface : public CObject
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphComboTransitionInterface, CObject, 0 );

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Get block border color
	Color GetBorderColor( Bool enabled ) const;

	//! Get client area color
	Color GetClientColor( Bool enabled ) const;

#endif

	//! can process event
	Bool CanProcessEvent( CBehaviorGraphInstance& instance ) const;

	//! process event
	Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	//! process force event
	Bool ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	//! called on update of start block
	void OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	//! Check if the transition condition is satisfied
	Bool CheckTransitionCondition( CBehaviorGraphInstance& instance ) const;

	//! Test if the transition condition is satisfied
	Bool TestTransitionCondition( CBehaviorGraphInstance& instance ) const;

	//! called on activation of block
	void OnActivated( CBehaviorGraphInstance& instance ) const;

	//! called on deactivation of block
	void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	//! Get combo node
	CBehaviorGraphComboStateNode* GetCombo() const;

	//! Steer to combo
	Bool SteerToCombo() const;

	//! Apply combo modifications
	void ApplyComboModifications( CBehaviorGraphInstance& instance ) const;

	TDynArray< IBehaviorGraphComboModifier* > m_modifiers;
};

BEGIN_CLASS_RTTI( CBehaviorGraphComboTransitionInterface )
	PARENT_CLASS( CObject );
	PROPERTY_INLINED( m_modifiers, TXT("Combo's modifires") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphComboTransitionNode : public CBehaviorGraphStateTransitionBlendNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphComboTransitionNode, CBehaviorGraphStateTransitionBlendNode, "State machine", "Combo transition" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnSpawned( const GraphBlockSpawnInfo& info );

	//! Get block border color
	virtual Color GetBorderColor() const;

	//! Get client area color
	virtual Color GetClientColor() const;

#endif

	//! process event
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	//! process force event
	virtual Bool ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	//! called on update of start block
	virtual void OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	//! Check if the transition condition is satisfied
	virtual Bool CheckTransitionCondition( CBehaviorGraphInstance& instance ) const;

	//! Test if the transition condition is satisfied
	virtual Bool TestTransitionCondition( CBehaviorGraphInstance& instance ) const;

	//! called on activation of block
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

	//! called on deactivation of block
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual Bool IsManualCreationAllowed() const { return false; }

protected:
	CBehaviorGraphComboTransitionInterface* m_comboInterface;
};

BEGIN_CLASS_RTTI( CBehaviorGraphComboTransitionNode );
	PARENT_CLASS( CBehaviorGraphStateTransitionBlendNode );
	PROPERTY_INLINED( m_comboInterface, TXT("Combo") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphGlobalComboTransitionNode : public CBehaviorGraphStateTransitionGlobalBlendNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphGlobalComboTransitionNode, CBehaviorGraphStateTransitionGlobalBlendNode, "State machine", "Combo global transition" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnSpawned( const GraphBlockSpawnInfo& info );

	//! Get block border color
	virtual Color GetBorderColor() const;

	//! Get client area color
	virtual Color GetClientColor() const;

#endif

	//! process event
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	//! process force event
	virtual Bool ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	//! called on update of start block
	virtual void OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	//! Check if the transition condition is satisfied
	virtual Bool CheckTransitionCondition( CBehaviorGraphInstance& instance ) const;

	//! Test if the transition condition is satisfied
	virtual Bool TestTransitionCondition( CBehaviorGraphInstance& instance ) const;

	//! called on activation of block
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

	//! called on deactivation of block
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

protected:
	CBehaviorGraphComboTransitionInterface* m_comboInterface;
};

BEGIN_CLASS_RTTI( CBehaviorGraphGlobalComboTransitionNode );
	PARENT_CLASS( CBehaviorGraphStateTransitionGlobalBlendNode );
	PROPERTY_INLINED( m_comboInterface, TXT("Combo") );
END_CLASS_RTTI();
