#pragma once

class CQuestLookAtBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestLookAtBlock, CQuestGraphBlock, 0 )

private:
	// lookout data
	CName				m_actor;
	CName				m_target;
	Float				m_duration;

	Bool				m_enabled;
	EDialogLookAtType	m_type;
	Bool				m_instant;

	Bool				m_canCloseEyes;
	Bool				m_forceCloseEyes;

	Float				m_speed;
	ELookAtLevel		m_level;

	Float				m_range;
	Float				m_gameplayRange;
	Bool				m_limitDeact;

	Vector				m_staticPoint;

	Float				m_headRotationRatio;
	Float				m_eyesLookAtConvergenceWeight;
	Bool				m_eyesLookAtIsAdditive;
	Float				m_eyesLookAtDampScale;	

public:
	CQuestLookAtBlock() { m_name = TXT("Look at"); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Slanted; }
	virtual Color GetTitleColor() const { return Color( 1, 90, 15 ); }
	virtual Color GetClientColor() const { return Color( 1, 90, 15 ); }
	virtual String GetBlockCategory() const { return TXT( "Gameplay" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;	

private:
	void OnActivateImpl( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	void PerformLookAt( CNode* lookingActor, CNode* m_lookAtTarget ) const;
	void PerformLookAtDynamic( CActor* lookingActor, CNode* m_lookAtTarget ) const;
	void PerformLookAtStatic( CActor* lookingActor, CNode* m_lookAtTarget ) const;
	void PerformLookAtStaticPoint( CActor* lookingActor ) const;
};

BEGIN_CLASS_RTTI( CQuestLookAtBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_actor, TXT( "Actor" ) )
	PROPERTY_EDIT( m_target, TXT( "Target" ) )
	PROPERTY_EDIT( m_enabled, TXT( "Enabled or disable look at" ) )
	PROPERTY_EDIT( m_type, TXT( "Look at type" ) )
	PROPERTY_EDIT( m_duration, TXT( "Duration" ) )		
	PROPERTY_EDIT( m_canCloseEyes, TXT("Close eyes") )
	PROPERTY_EDIT( m_forceCloseEyes, TXT("Close eyes") )
	PROPERTY_EDIT( m_speed, TXT( "Look at speed. Zero is default speed" ) )
	PROPERTY_EDIT( m_level, TXT( "Look at level" ) )
	PROPERTY_EDIT( m_range, TXT( "Look at range (0-360)" ) )
	PROPERTY_EDIT( m_gameplayRange, TXT( "Look at range to use during gameplay (0-360)" ) )
	PROPERTY_EDIT( m_limitDeact, TXT( "Auto limit deactivation" ) )
	PROPERTY_EDIT( m_instant, TXT( "Instant" ) )
	PROPERTY_EDIT( m_staticPoint, TXT( "Static point for DLT_StaticPoinit look at" ) )
	PROPERTY_EDIT_RANGE( m_headRotationRatio, TXT( "Turn head by a fraction of whole lookat angle " ) , 0.f , 1.f )
	PROPERTY_CUSTOM_EDIT_RANGE( m_eyesLookAtConvergenceWeight, TXT("Use me!"), TXT("Slider"), 0.f, 1.f );
PROPERTY_EDIT( m_eyesLookAtIsAdditive, TXT("") );
PROPERTY_EDIT_RANGE( m_eyesLookAtDampScale, TXT("higher value means slower look at"), 0.001f, FLT_MAX );
END_CLASS_RTTI()