
#pragma once

struct SAnimSliderTarget
{
	Vector				m_vec;
	THandle< CNode >	m_node;
	Bool				m_isTypeStatic; // static = vec, dynamic = node
	Bool				m_useRot;
	Bool				m_useTrans;

	SAnimSliderTarget();

	void Set( const CNode* node, Bool trans = true, Bool rot = true );
	void Set( const Vector& vec, Float yaw );
	void Set( const Vector& vec );
	void Set( Float yaw );

	Bool Get( const Matrix& defaultVal, Matrix& target ) const;

	Bool IsRotationSet() const;
	Bool IsTranslationSet() const;
};

RED_DECLARE_NAME( SlideToTarget );
RED_DECLARE_NAME( RotateEvent );
RED_DECLARE_NAME( SlideToTargetScaleEnd );

struct SAnimSliderSettings
{
	enum ERotationPolicy
	{
		RP_Delta,
		RP_Speed,
	};

	Float			m_blendIn;
	Float			m_blendOut;

	CName			m_translationEventName;
	CName			m_rotationEventName;
	CName			m_scaleEndPointEventNameA;
	CName			m_scaleEndPointEventNameB;

	CName			m_slotName;

	Bool			m_applyTranslation;
	Bool			m_applyRotation;

	ERotationPolicy	m_rotationPolicy;
	Bool			m_useRotationScaling;

	Bool			m_rotateAfterEventTime;
	Bool			m_translateAfterEventTime;

	Float			m_maxSlideTranslation;
	Float			m_maxSlideTranslationPerFrame;

	SAnimSliderSettings() 
		: m_blendIn( 0.2f ), m_blendOut( 0.2f ), m_rotationPolicy( RP_Speed ), m_useRotationScaling( true )
		, m_translationEventName( CNAME( SlideToTarget ) ), m_rotationEventName( CNAME( RotateEvent ) )
		, m_slotName( CNAME( EXP_SLOT ) )
		, m_applyTranslation( true ), m_applyRotation( true )
		, m_rotateAfterEventTime( false ), m_translateAfterEventTime( false )
		, m_scaleEndPointEventNameA( CNAME( SlideToTargetScaleEnd ) ), m_scaleEndPointEventNameB( CNAME( SlideToTargetScaleEnd ) )
		, m_maxSlideTranslation( 15.f ), m_maxSlideTranslationPerFrame( 3.f )
	{}
};

class AnimSlider
{
	const CSkeletalAnimationSetEntry*	m_animation;
	SAnimSliderSettings					m_settings;

	enum ERotationSide
	{
		RS_None,
		RS_Left,
		RS_Right,
	};

	enum ERotationDebugState
	{
		RDS_None,
		RDS_Motion,
		RDS_Delta,
		RDS_Speed
	};

	struct RotationSetup
	{
		Float		m_start;
		Float		m_end;
		Float		m_speed;

		RotationSetup() {}
		RotationSetup( Float start, Float end, Float speed ): m_start( start ), m_end( end ), m_speed( speed ) {}
	};

	Float			m_translationMinScale;
	Float			m_translationMaxScale;
	Float			m_translationMaxAcceptableScale;
	Bool			m_translationCancel[3];

	Float			m_rotationMinScale;
	Float			m_rotationMaxScale;
	Bool			m_rotationCancel;

	Bool			m_calcInWS;

	Bool			m_cancel;	// <= TODO

	Vector			m_translationScale;
	Vector			m_translationOffset;
	Bool			m_translationOffsetNotZero[3];

	Float			m_translationStart;
	Float			m_translationEnd;
	Bool			m_hasTranslation[3];
	Int32				m_copyTranslation[3];

	Float			m_rotationScale;
	Float			m_rotationOffset;
	Bool			m_rotationOffsetNotZero;
	ERotationDebugState m_lastRotationState;

	TDynArray< RotationSetup > m_rotations;
	Bool			m_hasRotation;

	Float			m_timer;
	Float			m_timeStart;
	Float			m_scaleEndTime;

	Matrix			m_curr;
	Matrix			m_dest;
	Matrix			m_ref;

	Bool			m_firstUpdate;
	Matrix			m_firstPosition;
	Bool			m_firstTargetFlag;
	Matrix			m_firstTarget;
	Float			m_firstTargetYaw;
	TDynArray< Matrix > m_debugPoints;
	Vector			m_debugLastDeltaTransWS;
	Float			m_debugLastDeltaRotWS;

	Matrix			m_target;
	Bool			m_targetUseRot;
	Bool			m_targetUseTrans;
	Bool			m_targetChanged;
	Bool			m_currChanged;

public:
	AnimSlider();

	Bool Init( const CSkeletalAnimationSetEntry* animation, const SAnimSliderSettings& settings );

	void SetTarget( const Matrix& dest );
	void SetTargetPosition( const Vector& destPosition );
	void SetTargetRotation( Float destRotation );

	void SetCurrentPosition( const Matrix& l2w );

	Bool UpdateWS( const Matrix& curr, Float dt, Vector& motionTransDeltaWS, Float& motionRotDeltaWS );
	Bool UpdateAtWS( const Matrix& curr, Float startTime, Float endTime, Vector& motionTransDeltaWS, Float& motionRotDeltaWS, const Vector * destPoint );

	Bool UpdateMS( const Matrix& curr, Float dt, Vector& motionTransDeltaWS, Float& motionRotDeltaWS );
	Bool UpdateAtMS( const Matrix& curr, Float startTime, Float endTime, Vector& motionTransDeltaWS, Float& motionRotDeltaWS );

	void GenerateDebugFragments( CRenderFrame* frame );

private:
	void InternalUpdate();

	void CalculateScaleAndOffset();
	void CalculateDebugPoints();

	Float GetDuration() const;

	void CalcMovementBetween( Float start, Float end, Vector& motionTrans, Float& motionRot, Float& motionZ );
	void CalcMovementBetweenMS( Float start, Float end, Vector& motionTransWS, Float& motionRotWS, Float& motionMSZ );

	void GetMovementWS( Float time, Vector& motionTransWS, Float& motionRotWS, Float& motionMSZ );

	void ApplyScale( Float time, Vector& translation, Float& rotation );

	void GetMovementDeltaWS( Float time, Vector& motionTransDeltaWS, Float& motionRotDeltaWS, const Vector* pointDest = NULL );
	void GetMovementDeltaMS( Float time, Vector& motionTransDeltaMS, Float& motionRotDeltaMS );

	void FindTimesForEvent( const CSkeletalAnimationSetEntry* animation, const CName& eventName, Float& start, Float& end ) const;
	void FindTimesAndSpeedForRotationEvent( const CSkeletalAnimationSetEntry* animation, const CName& eventName, TDynArray< RotationSetup > & m_rotations ) const;
	Bool FindTimeForEvent( const CSkeletalAnimationSetEntry* animation, const CName& eventName, Float& time ) const;

	Float DecodeRotationEnum( Int32 enumValue ) const;
};

//////////////////////////////////////////////////////////////////////////

class AnimationSlider2
{
	typedef TPair< Float, Float > TEventTime;

	struct Point
	{
		Vector			m_position;
		EulerAngles		m_rotation;
	};

private:
	SAnimSliderTarget			m_target;

	Float						m_timer;
	Float						m_startTime;
	Float						m_endTime;
	Float						m_duration;

	Point						m_startPoint;
	Matrix						m_startMat;
	Bool						m_startWasSet;

	Bool						m_rotate;
	Bool						m_translate;

	TDynArray< TEventTime >		m_translationEventTimes;
	TDynArray< TEventTime >		m_rotationEventTime;
	TDynArray< Float >			m_rotatonEventSpeeds;

public:
	AnimationSlider2();
	~AnimationSlider2();

	Bool Init( const CSkeletalAnimationSetEntry* animation, const SAnimSliderSettings& settings );

	void SetTarget( const SAnimSliderTarget& target );
	void SetTarget( const CNode* dest );
	void SetTarget( const Matrix& dest );
	void SetTargetPosition( const Vector& destPosition );
	void SetTargetRotation( Float destRotation );

	void GenerateDebugFragments( CRenderFrame* frame );

	void SetCurrentPosition( const Matrix& mat ) { if ( !m_startWasSet ) Start( mat ); }

public:
	void Start( const Matrix& startingPoint );

	Bool Update( const Matrix& currWS, Float dt, Matrix& transformWSOut );
	Bool UpdateAt( const Matrix& currWS, Float time, Matrix& transformWSOut );
	Bool Update( const Matrix& currWS, Float dt, Vector& deltaTransWS, Float& deltaRotWS );
	Bool UpdateAt( const Matrix& currWS, Float time, Vector& deltaTransWS, Float& deltaRotWS );

	Bool IsFinished() const;

private:
	Float DecodeRotationEnum( Int32 enumValue ) const;

	void ConvertMatrixToPoint( const Matrix& in, Point& out ) const;
	void ConvertPointToMatrix( const Point& in, Matrix& out ) const;

	void InterpolateTranslation( const Point& pointA, const Point& pointB, Point& out, Float weight ) const;
	void InterpolateRotation( const Point& pointA, const Point& pointB, Point& out, Float weight ) const;
};

