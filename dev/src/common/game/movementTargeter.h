/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "movementObject.h"


class CMovingAgentComponent;
struct SMoveLocomotionGoal;
class CPathComponent;
class CPathPointLock;

///////////////////////////////////////////////////////////////////////////////

class IMovementTargeter
{
public:
	virtual ~IMovementTargeter() {}

	virtual void Release();

	// Serialization support
	virtual void OnSerialize( IFile& file );

	virtual void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) = 0;
	virtual Bool IsFinished() const;

	virtual void GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame );
	virtual void GenerateDebugPage( TDynArray< String >& debugLines ) const;

protected:
	// ------------------------------------------------------------------------
	// Steering API
	// ------------------------------------------------------------------------
	
	// Seek the specified position. Returns a velocity vector that will allow it to reach
	// the position.
	Vector Seek( const CMovingAgentComponent& host, const Vector& pos ) const;

	// Moves away from the specified position.
	Vector Flee( const CMovingAgentComponent& host, const Vector& pos ) const;

	// Pursue the specified agent. Returns a velocity vector that will allow it to catch up
	// with the pursued agent.
	Vector Pursue( const CMovingAgentComponent& host, const CMovingAgentComponent& pursuedAgent ) const;

	// Calculates a desired orientation that will allow the agent to face the specified target position
	Float FaceTarget( const CMovingAgentComponent& host, const Vector& pos ) const;
};

///////////////////////////////////////////////////////////////////////////////

// A target API exposed to scripts
class CMoveTRGScript : public CObject, public IMovementTargeter
{
	DECLARE_ENGINE_CLASS( CMoveTRGScript, CObject, 0 );

private:
	mutable THandle< CMovingAgentComponent >		m_agent;
	mutable Float									m_timeDelta;

public:
	CMoveTRGScript();
	virtual ~CMoveTRGScript();
	// ------------------------------------------------------------------------
	// IMovementTargeter implementation
	// ------------------------------------------------------------------------
	virtual void Release();
	virtual void OnSerialize( IFile& file );
	virtual void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta );
	virtual void GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) {}
	virtual void GenerateDebugPage( TDynArray< String >& debugLines ) const {}

private:
	// ------------------------------------------------------------------------
	// Scripting support
	// ------------------------------------------------------------------------
	void funcSetHeadingGoal( CScriptStackFrame& stack, void* result );
	void funcSetOrientationGoal( CScriptStackFrame& stack, void* result );
	void funcSetSpeedGoal( CScriptStackFrame& stack, void* result );
	void funcSetMaxWaitTime( CScriptStackFrame& stack, void* result );
	void funcMatchDirectionWithOrientation( CScriptStackFrame& stack, void* result );
	void funcSetFulfilled( CScriptStackFrame& stack, void* result );

	void funcSeek( CScriptStackFrame& stack, void* result );
	void funcFlee( CScriptStackFrame& stack, void* result );
	void funcPursue( CScriptStackFrame& stack, void* result );
	void funcFaceTarget( CScriptStackFrame& stack, void* result );
};
BEGIN_CLASS_RTTI( CMoveTRGScript );
	PARENT_CLASS( CObject );
	PROPERTY_NOSERIALIZE( m_agent );
	PROPERTY_NOSERIALIZE( m_timeDelta );
	NATIVE_FUNCTION( "SetHeadingGoal", funcSetHeadingGoal );
	NATIVE_FUNCTION( "SetOrientationGoal", funcSetOrientationGoal );
	NATIVE_FUNCTION( "SetSpeedGoal", funcSetSpeedGoal );
	NATIVE_FUNCTION( "SetMaxWaitTime", funcSetMaxWaitTime );
	NATIVE_FUNCTION( "MatchDirectionWithOrientation", funcMatchDirectionWithOrientation );
	NATIVE_FUNCTION( "SetFulfilled", funcSetFulfilled );
	NATIVE_FUNCTION( "Seek", funcSeek );
	NATIVE_FUNCTION( "Flee", funcFlee );
	NATIVE_FUNCTION( "Pursue", funcPursue );
	NATIVE_FUNCTION( "FaceTarget", funcFaceTarget );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class IMoveTRGManaged : public IMovementTargeter, public IMovementObject
{
public:
	virtual ~IMoveTRGManaged() {}

	virtual void Release() { IMovementObject::Release(); }

	virtual void OnSerialize( IFile& file ) {}
};

///////////////////////////////////////////////////////////////////////////////

class CStaticMovementTargeter : public IMoveTRGManaged
{
public:
	enum EType
	{
		SMT_None,
		SMT_Dynamic,
		SMT_Static
	};

private:
	const CMovingAgentComponent&		m_agent;

	THandle< CNode >					m_dynamicRotationTarget;
	Vector								m_staticRotationTarget;
	EType								m_rotationTargetType;
	Bool								m_clamping;

	// runtime calculation results
	Bool								m_hasOrientation;
	Vector								m_orientationTrgPos;

public:
	CStaticMovementTargeter( const CMovingAgentComponent& agent );
	~CStaticMovementTargeter();

	void Update();

	void SetRotation( const THandle< CNode >& target, Bool clamping );

	void SetRotation( const Vector& target, Bool clamping );

	void ClearRotation();

	RED_INLINE Bool IsRotationTargetSet() const { return m_rotationTargetType != SMT_None; }

	// ------------------------------------------------------------------------
	// IMovementTargeter implementation
	// ------------------------------------------------------------------------
	void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta );

	void GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame );
	virtual void GenerateDebugPage( TDynArray< String >& debugLines ) const;

private:
	Bool CalcTarget( Vector& pos ) const;
};

///////////////////////////////////////////////////////////////////////////////
