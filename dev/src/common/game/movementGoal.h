/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

///////////////////////////////////////////////////////////////////////////////

class IMovementTargeter;
class CMovingAgentComponent;

#define MAX_MOVEMENT_WAIT_TIME			1.0f

///////////////////////////////////////////////////////////////////////////////

// A helper structure describing a local movement goal
struct SMoveLocomotionGoal
{
	DECLARE_RTTI_STRUCT( SMoveLocomotionGoal )

public:
	struct BehNotification
	{
		CName		m_name;
		Float		m_timeout;
		BehNotification( const CName& name = CName::NONE , Float timeout = 0.0f )
			: m_name( name )
			, m_timeout( timeout )
		{}

		Bool operator==( const BehNotification& rhs ) const 
		{
			return m_name == rhs.m_name;
		}
	};

	TDynArray< BehNotification >					m_expectedBehNotification;

protected:

	struct SCustomFlagValue
	{
		struct TypedPtr
		{
			void*			m_ptr;
			CClass*			m_class;
		};
		struct SimpleVec
		{
			Float			m_v[4];
		};
		enum EType
		{
			ECFT_None,
			ECFT_Float,
			ECFT_Int,
			ECFT_Vector,
			ECFT_Pointer,
			ECFT_Bool,
		};
		
		EType			m_type;
		union
		{
			SimpleVec	m_vec;
			TypedPtr	m_obj;
			Float		m_float;
			Int32		m_int;
			Bool		m_bool;
		};
	};
	

	// goals
	Bool											m_isFulfilled;
	Bool											m_hasHeading;
	Bool											m_hasOrientation;
	Bool											m_hasSpeed;
	Bool											m_hasGoalPosition;
	Bool											m_hasGoalTolerance;
	Bool											m_hasDistanceToGoal;
	Bool											m_hasDestinationPosition;
	Bool											m_hasDistanceToDestination;
	Bool											m_orientationGoalAlwaysSet;
	Bool											m_orientationClamping;
	Bool											m_matchMoveDirectionWithOrientation;
	Bool											m_matchOrientationWithHeading;
	Bool											m_snapToMinimalSpeed;									// if set to true, steering will be supposed to increase speed to match some minimal value

	Vector2											m_headingToGoal;
	Vector											m_goalPosition;
	Float											m_goalTolerance;
	Float											m_distanceToGoal;
	Vector											m_destinationPosition;
	Float											m_distanceToDestination;
	Float											m_orientation;
	Float											m_maxWaitTime;
	Float											m_desiredSpeed;
	Float											m_headingImportanceMultiplier;

	// recalculated steering values
	Float											m_orientationDiff;
	Float											m_rotationToGoal;
	THandle< CNode >								m_goalTarget;

	THashMap< CName, SCustomFlagValue >				m_flags;												// more general structure to store any custom-made flag to communicate locomotion goal

public:

	SMoveLocomotionGoal();

	Bool IsSet() const;

	Bool CanBeProcessed() const;

	void Clear();

	void SetHeadingGoal( const CMovingAgentComponent& agent, const Vector2& heading, Bool normalize = true );

	void SetGoalPosition( const Vector& position );

	void SetGoalTolerance( Float toleranceRadius );

	void SetDistanceToGoal( Float distance );

	void SetDestinationPosition( const Vector& position );

	void SetDistanceToDestination( Float distance );

	void SetOrientationGoal( const CMovingAgentComponent& agent, Float orientation, Bool alwaysSet = false, Bool clamped = true );

	void MatchMoveDirectionWithOrientation( Bool enable )					{ m_matchMoveDirectionWithOrientation = enable; }

	void MatchOrientationWithHeading( Bool enable = true )					{ m_matchOrientationWithHeading = enable; }

	void SnapToMinimalSpeed( Bool snap = true )								{ m_snapToMinimalSpeed = snap; }

	void SetHeadingImportanceMultiplier( Float mult )						{ m_headingImportanceMultiplier = mult; }

	void SetSpeedGoal( Float speed );

	void SetMaxWaitTime( Float time );

	void SetFulfilled( Bool val );

	void SetGoalTargetNode( CNode* target )									{ m_goalTarget = target; }
	void SetGoalTargetNode( const THandle< CNode >& target )				{ m_goalTarget = target; }

	// --------------------------------------------------------------------
	// Accessors
	// --------------------------------------------------------------------
	RED_INLINE Bool IsHeadingGoalSet() const								{ return m_hasHeading; }

	RED_INLINE Bool IsOrientationGoalSet() const							{ return m_hasOrientation; }

	RED_INLINE Bool IsSpeedGoalSet() const									{ return m_hasSpeed; }

	RED_INLINE Bool IsGoalPositionSet() const								{ return m_hasGoalPosition; }
	
	RED_INLINE Bool IsGoalToleranceSet() const								{ return m_hasGoalTolerance; }

	RED_INLINE Bool IsDistanceToGoalSet() const								{ return m_hasDistanceToGoal; }

	RED_INLINE Bool IsDestinationPositionSet() const						{ return m_hasDestinationPosition; }

	RED_INLINE Bool IsDistanceToDestinationSet() const						{ return m_hasDistanceToDestination; }

	RED_INLINE const Vector2& GetHeadingToGoal() const						{ ASSERT( m_hasHeading ); return m_headingToGoal; }

	RED_INLINE const Vector& GetGoalPosition() const						{ ASSERT( m_hasGoalPosition ); return m_goalPosition; }

	RED_INLINE Float GetGoalTolerance() const								{ ASSERT( m_hasGoalTolerance); return m_goalTolerance; }

	RED_INLINE Float GetDistanceToGoal() const								{ ASSERT( m_hasDistanceToGoal ); return m_distanceToGoal; }

	RED_INLINE const Vector& GetDestinationPosition() const					{ ASSERT( m_hasDestinationPosition ); return m_destinationPosition; }

	RED_INLINE Float GetDistanceToDestination() const						{ ASSERT( m_hasDistanceToDestination ); return m_distanceToDestination; }

	RED_INLINE Vector2 GetVelocity() const									{ ASSERT( m_hasHeading ); return m_hasSpeed ? m_headingToGoal * m_desiredSpeed : m_headingToGoal; }
		
	RED_INLINE Float GetOrientationDiff() const								{ ASSERT( m_hasOrientation ); return m_orientationDiff; }

	RED_INLINE Float GetOrientation() const									{ ASSERT( m_hasOrientation ); return m_orientation; }

	RED_INLINE Bool ClampOrientation() const								{ return m_orientationClamping; }

	RED_INLINE Float GetDesiredSpeed() const								{ ASSERT( m_hasSpeed ); return m_desiredSpeed; }

	RED_INLINE Float GetRotationToGoal() const								{ ASSERT( m_hasHeading ); return m_rotationToGoal; }

	RED_INLINE Float GetMaxWaitTime() const									{ return m_maxWaitTime; }

	RED_INLINE Bool ShouldMatchMoveDirectionWithOrientation() const			{ return m_hasOrientation && m_matchMoveDirectionWithOrientation; }

	RED_INLINE Bool ShouldMatchOrientationWithHeading() const				{ return m_matchOrientationWithHeading; }

	RED_INLINE Bool ShouldSnapToMinimalSpeed() const						{ return m_snapToMinimalSpeed; }

	RED_INLINE Float GetHeadingImportanceMultiplier() const					{ return m_headingImportanceMultiplier; }

	RED_INLINE CNode* GetGoalTargetNode() const								{ return m_goalTarget.Get(); }

	// --------------------------------------------------------------------
	// Goal custom flags
	// --------------------------------------------------------------------
	void SetFlag( CName flag );
	void SetFlag( CName flag, Float val );
	void SetFlag( CName flag, Int32 val );
	void SetFlag( CName flag, Bool val );
	void SetFlag( CName flag, const Vector& val );
	void SetFlag( CName flag, void* obj, CClass* ptrClass );
	template < class T >
	void TSetFlag( CName flag, T* obj )										{ SetFlag( flag, obj, T::GetStaticClass() ); }

	Bool HasFlag( CName flag ) const;

	Bool GetFlag( CName flag, Float& outVal ) const;
	Bool GetFlag( CName flag, Int32& outVal ) const;
	Bool GetFlag( CName flag, Bool& outVal ) const;
	Bool GetFlag( CName flag, Vector& outVal ) const;
	Bool GetFlag( CName flag, void*& outObj, CClass* ptrClass ) const;
	template < class T >
	Bool TGetFlag( CName flag, T*& outObj ) const							{ return GetFlag( flag, reinterpret_cast< void*& >( outObj ), T::GetStaticClass() ); }

	// --------------------------------------------------------------------
	// Changes introduced to the goal during analysis
	// --------------------------------------------------------------------
	void AddNotification( const CName& notificationName, Float timeout );
};

BEGIN_NODEFAULT_CLASS_RTTI( SMoveLocomotionGoal );
END_CLASS_RTTI();


