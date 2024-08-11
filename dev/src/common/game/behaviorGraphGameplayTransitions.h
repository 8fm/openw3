/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

///////////////////////////////////////////////////////////////////////////////

class CChangeMovementDirectionTransitionCondition;
class CChangeFacingDirectionTransitionCondition;

///////////////////////////////////////////////////////////////////////////////

class CChangeMovementDirectionTransitionCondition : public IBehaviorStateTransitionCondition
{
	DECLARE_ENGINE_CLASS( CChangeMovementDirectionTransitionCondition, IBehaviorStateTransitionCondition, 0 );

protected:
	Float m_angleDiffThreshold; // maximal difference of angles between two
	Float m_startCheckingAfterTime; // minimal time needed to make this transition available
	CName m_requestedMovementDirectionWSVariableName; // requested movement direction (float variable)
	CName m_currentMovementDirectionMSInternalVariableName; // current movement direction (internal float variable), as given by DirectionalMovement node

protected:
	TInstanceVar< Float > i_timeActive;
	TInstanceVar< Bool > i_requiresChange;
	TInstanceVar< Bool > i_requiresUpdate;

public:
	CChangeMovementDirectionTransitionCondition();

	virtual Bool Check( CBehaviorGraphInstance& instance ) const;
	virtual Bool Test( CBehaviorGraphInstance& instance ) const { return Check( instance ); }
	virtual void Reset( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnStartBlockActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void CopyDataFrom(const IBehaviorStateTransitionCondition* transition);

	virtual void GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance = nullptr ) const { captions.PushBack( ( getCaptionTests ? GetCaptionTest( instance ) : TXT("") ) + TXT("Change Movement Direction") ); }

	virtual void GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const;

private:
	void UpdateRequiresChange( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CChangeMovementDirectionTransitionCondition )
	PARENT_CLASS( IBehaviorStateTransitionCondition );
	PROPERTY_EDIT( m_angleDiffThreshold, TXT("Maximal difference of angles between two") );	
	PROPERTY_EDIT( m_startCheckingAfterTime, TXT("Minimal time needed to make this transition available") );	
	PROPERTY_CUSTOM_EDIT( m_requestedMovementDirectionWSVariableName, TXT("Requested movement direction variable name"), TXT("BehaviorVariableSelection") );
	PROPERTY_CUSTOM_EDIT( m_currentMovementDirectionMSInternalVariableName, TXT("Current movement direction internal variable name"), TXT("BehaviorInternalVariableSelection") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

enum EChangeFacingDirectionSide
{
	CFDS_None,
	CFDS_Left,
	CFDS_Right,
	CFDS_Any,
};

BEGIN_ENUM_RTTI( EChangeFacingDirectionSide );
	ENUM_OPTION( CFDS_None );
	ENUM_OPTION( CFDS_Left );
	ENUM_OPTION( CFDS_Right );
	ENUM_OPTION( CFDS_Any );
END_ENUM_RTTI();

class CChangeFacingDirectionTransitionCondition : public IBehaviorStateTransitionCondition
{
	DECLARE_ENGINE_CLASS( CChangeFacingDirectionTransitionCondition, IBehaviorStateTransitionCondition, 0 );

protected:
	EChangeFacingDirectionSide m_side; // to which side?
	Float m_angleDiffMin; // minimal difference of angles to trigger condition
	Float m_angleDiffMax; // maximal difference of angles to trigger condition
	Float m_startCheckingAfterTime; // minimal time needed to make this transition available
	CName m_requestedFacingDirectionWSVariableName; // requested facing direction (float variable)
	Bool m_dontChange; // if you want to know that you dont want to change

protected:
	TInstanceVar< Float > i_timeActive;
	TInstanceVar< Bool > i_requiresChange;
	TInstanceVar< Bool > i_requiresUpdate;

public:
	CChangeFacingDirectionTransitionCondition();

	virtual Bool Check( CBehaviorGraphInstance& instance ) const;
	virtual Bool Test( CBehaviorGraphInstance& instance ) const { return Check( instance ); }
	virtual void Reset( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnStartBlockActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void CopyDataFrom(const IBehaviorStateTransitionCondition* transition);

	virtual void GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance = nullptr ) const;

	virtual void GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const;

private:
	void UpdateRequiresChange( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CChangeFacingDirectionTransitionCondition )
	PARENT_CLASS( IBehaviorStateTransitionCondition );
	PROPERTY_EDIT( m_side, TXT("To which side?") );
	PROPERTY_EDIT( m_angleDiffMin, TXT("Minimal difference of angles to trigger condition") );
	PROPERTY_EDIT( m_angleDiffMax, TXT("Maximal difference of angles to trigger condition") );
	PROPERTY_EDIT( m_startCheckingAfterTime, TXT("Minimal time needed to make this transition available") );	
	PROPERTY_CUSTOM_EDIT( m_requestedFacingDirectionWSVariableName, TXT("Requested facing direction variable name"), TXT("BehaviorVariableSelection") );
	PROPERTY_EDIT( m_dontChange, TXT("If you want to know that you dont want to change") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CIsMovingForwardTransitionCondition : public IBehaviorStateTransitionCondition
{
	DECLARE_ENGINE_CLASS( CIsMovingForwardTransitionCondition, IBehaviorStateTransitionCondition, 0 );

protected:
	Float m_maxOffAngle;
	Bool m_notMovingForward;

protected:
	TInstanceVar< Bool > i_movingForward;
	TInstanceVar< Bool > i_requiresUpdate;

public:
	CIsMovingForwardTransitionCondition();

	virtual Bool Check( CBehaviorGraphInstance& instance ) const;
	virtual Bool Test( CBehaviorGraphInstance& instance ) const { return Check( instance ); }
	virtual void Reset( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnStartBlockActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void CopyDataFrom(const IBehaviorStateTransitionCondition* transition);

	virtual void GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance = nullptr ) const;

private:
	void UpdateMovingForward( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CIsMovingForwardTransitionCondition )
	PARENT_CLASS( IBehaviorStateTransitionCondition );
	PROPERTY_EDIT( m_maxOffAngle, TXT("Max difference between movement and current rotation") );
	PROPERTY_EDIT( m_notMovingForward, TXT("Check if character is not moving forward") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CBehaviorGraphStateTransitionFinalStepNode : public CBehaviorGraphStateTransitionBlendNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphStateTransitionFinalStepNode, CBehaviorGraphStateTransitionBlendNode, "State machine.Transitions", "Final step transition (blend)" );

	CName m_locationAdjustmentVar;
	CName m_adjustmentActiveVar;

public:
	CBehaviorGraphStateTransitionFinalStepNode();

	virtual String GetCaption() const;

public:
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphStateTransitionFinalStepNode );
	PARENT_CLASS( CBehaviorGraphStateTransitionBlendNode );
	PROPERTY_EDIT( m_locationAdjustmentVar, TXT( "Movement Adjustment location variable" ) );
	PROPERTY_EDIT( m_adjustmentActiveVar, TXT( "Is Movement Adjustment active" ) );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
