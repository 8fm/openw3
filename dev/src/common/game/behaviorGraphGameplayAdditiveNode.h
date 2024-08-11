
#pragma once

struct SGameplayAdditiveAnimRuntimeData
{
	DECLARE_RTTI_STRUCT( SGameplayAdditiveAnimRuntimeData )

	Float						m_prevTime;
	Float						m_currTime;
	Float						m_weight;
	Float						m_speed;
	CSkeletalAnimationSetEntry* m_animation;
	Int32							m_index;
	Float						m_loop;
	Float						m_blendTimer;

	SGameplayAdditiveAnimRuntimeData() : m_prevTime( 0.f ), m_currTime( 0.f ), m_weight( 1.f ), m_speed( 1.f ), m_animation( NULL ), m_index( -1 ), m_loop( 0.f ), m_blendTimer( 0.f ) {}

	void Reset()
	{
		m_prevTime = 0.f;
		m_currTime = 0.f;
		m_weight = 1.f;
		m_speed = 1.f;
		m_animation = NULL;
		m_index = -1;
		m_loop = 0.f;
		m_blendTimer = 0.f;
	}

	Bool IsPlaying() const
	{
		return m_index != -1 && m_animation;
	}

	void Update( Float dt )
	{
		m_prevTime = m_currTime;
		m_currTime += m_speed * dt;

		ASSERT( m_currTime <= m_animation->GetDuration() );
	}

	void Sync( const CSyncInfo& info )
	{
		m_prevTime = info.m_prevTime + m_loop;
		m_currTime = info.m_currTime + m_loop;
	}

	void AddLoopAndSync( const CSyncInfo& info )
	{
		m_loop += info.m_totalTime;

		Sync( info );
	}

	Bool WillBeEnd( Float dt ) const
	{
		return m_currTime + m_speed * dt > m_animation->GetDuration();
	}

	Bool IsFinished() const
	{
		return m_animation ? m_currTime > m_animation->GetDuration() : true;
	}
};

BEGIN_CLASS_RTTI( SGameplayAdditiveAnimRuntimeData );
END_CLASS_RTTI();

struct SGameplayAdditiveAnimation
{
	DECLARE_RTTI_STRUCT( SGameplayAdditiveAnimation )

	CName	m_animationName;
	Float	m_delay;
	Float	m_cooldown;
	Float	m_chance;
	Bool	m_onlyOnce;

	Float	m_weightRangeMin;
	Float	m_weightRangeMax;

	Float	m_speedRangeMin;
	Float	m_speedRangeMax;

	Bool	m_useWeightRange;
	Bool	m_useSpeedRange;

	SGameplayAdditiveAnimation() : m_delay( 0.f ), m_cooldown( 0.f ), m_chance( 1.f ), m_useWeightRange( false ), m_weightRangeMin( 0.9f ), m_weightRangeMax( 1.1f ), m_useSpeedRange( false ), m_speedRangeMin( 0.9f ), m_speedRangeMax( 1.1f ), m_onlyOnce( false ) {}
};

BEGIN_CLASS_RTTI( SGameplayAdditiveAnimation );
	PROPERTY_CUSTOM_EDIT( m_animationName, TXT(""), TXT("BehaviorAnimSelection") );
	PROPERTY_EDIT( m_delay, TXT("") );
	PROPERTY_EDIT( m_cooldown, TXT("") );
	PROPERTY_EDIT( m_chance, TXT("") );
	PROPERTY_EDIT( m_onlyOnce, TXT("") );
	PROPERTY_EDIT( m_useWeightRange, TXT("") );
	PROPERTY_EDIT_RANGE( m_weightRangeMin, TXT(""), 0.0, 1.f );
	PROPERTY_EDIT_RANGE( m_weightRangeMax, TXT(""), 0.0, 1.f );
	PROPERTY_EDIT( m_useSpeedRange, TXT("") );
	PROPERTY_EDIT_RANGE( m_speedRangeMin, TXT(""), 0.0, 10.f );
	PROPERTY_EDIT_RANGE( m_speedRangeMax, TXT(""), 0.0, 10.f );
END_CLASS_RTTI();

struct SGameplayAdditiveLevel
{
	DECLARE_RTTI_STRUCT( SGameplayAdditiveLevel )

	TDynArray< SGameplayAdditiveAnimation >		m_animations;
	Bool										m_useLevel;
	Bool										m_synchronize;

	SGameplayAdditiveLevel() : m_useLevel( true ), m_synchronize( false ) {}
};

BEGIN_CLASS_RTTI( SGameplayAdditiveLevel );
	PROPERTY_EDIT( m_useLevel, TXT("") );
	PROPERTY_EDIT( m_synchronize, TXT("") );
	PROPERTY_EDIT( m_animations, TXT("Animations") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphGameplayAdditiveNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphGameplayAdditiveNode, CBehaviorGraphNode, "Animation", "Gameplay additive" );

protected:
	SGameplayAdditiveLevel		m_level_0;
	SGameplayAdditiveLevel		m_level_1;
	Bool						m_gatherEvents;

protected:
	TInstanceVar< Float >									i_actTime;
	TInstanceVar< Bool >									i_firstUpdate;
	TInstanceVar< Float >									i_timeDelta;

	TInstanceVar< TDynArray< CSkeletalAnimationSetEntry* > > i_animsForLevel0;
	TInstanceVar< TDynArray< CSkeletalAnimationSetEntry* > > i_animsForLevel1;

	TInstanceVar< TDynArray< Float > >						i_cooldownsForLevel0;
	TInstanceVar< TDynArray< Float > >						i_cooldownsForLevel1;

	TInstanceVar< SGameplayAdditiveAnimRuntimeData >		i_animLevel0;
	TInstanceVar< SGameplayAdditiveAnimRuntimeData >		i_animLevel1;

protected:
	CBehaviorGraphNode*			m_cachedInputNode;

public:
	CBehaviorGraphGameplayAdditiveNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return TXT("Gameplay additive"); }

	void GetAnimProgress( CBehaviorGraphInstance& instance, Float& p1, Float& p2 ) const;
	void GetAnimNames( CBehaviorGraphInstance& instance, CName& n1, CName& n2 ) const;
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void CollectAnimationUsageData( const CBehaviorGraphInstance& instance, TDynArray<SBehaviorUsedAnimationData>& collectorArray ) const;
#endif

protected:
	void UpdateCooldowns( CBehaviorGraphInstance& instance, Float dt ) const;
	void UpdateCooldowns( TDynArray< Float >& arr, Float dt ) const;

	void AddAdditivePose( const CBehaviorGraphInstance& instance, SGameplayAdditiveAnimRuntimeData& animData, SBehaviorGraphOutput& temp, SBehaviorSampleContext& context, SBehaviorGraphOutput& output ) const;

	void FindAnims( CBehaviorGraphInstance& instance ) const;

	void FillAnimsForLevel( CSkeletalAnimationContainer* cont, const SGameplayAdditiveLevel& level, TDynArray< CSkeletalAnimationSetEntry* >& anims ) const;
	void FillCooldownsForLevel( const SGameplayAdditiveLevel& level, TDynArray< Float >& cooldowns ) const;
	void ResetCooldownsForLevelOnDeact( const SGameplayAdditiveLevel& level, TDynArray< Float >& cooldowns ) const;

	void UpdateLevel( CBehaviorGraphInstance& instance, const SGameplayAdditiveLevel& level, SGameplayAdditiveAnimRuntimeData& animData, const TDynArray< CSkeletalAnimationSetEntry* >& anims, TDynArray< Float >& cooldowns, Float actTime, Float timeDelta ) const;
	void SelectNextAnim( CBehaviorGraphInstance& instance, const SGameplayAdditiveLevel& level, SGameplayAdditiveAnimRuntimeData& animData, const TDynArray< CSkeletalAnimationSetEntry* >& anims, TDynArray< Float >& cooldowns, Float actTime ) const;

	void InternalReset( CBehaviorGraphInstance& instance ) const;

public:
	virtual void OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphGameplayAdditiveNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_EDIT( m_level_0, TXT("") );
	PROPERTY_EDIT( m_level_1, TXT("") );
	PROPERTY_EDIT( m_gatherEvents, TXT("") );
	PROPERTY( m_cachedInputNode );
END_CLASS_RTTI();

