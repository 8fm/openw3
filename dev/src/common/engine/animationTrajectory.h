
#pragma once

struct AnimationTrajectoryData
{
	TDynArray< Vector >	m_pointsLS;
	TDynArray< Vector > m_rotLS;
	TDynArray< Vector >	m_pointsMS;
	TDynArray< Vector >	m_rotMS;
	TDynArray< Vector >	m_pointsLSO;
	TDynArray< Vector >	m_pointsMSO;
	TDynArray< Float >	m_weights;
	Vector				m_defaultOffset;
	Uint32				m_syncFrame;

public:
	AnimationTrajectoryData() : m_syncFrame( 0 ), m_defaultOffset( 1.f, 0.f, 0.f ) {}

	void Serialize( IFile& file )
	{
		file << m_pointsLS;
		file << m_rotLS;
		file << m_pointsMS;
		file << m_rotMS;
		file << m_weights;
		file << m_pointsLSO;
		file << m_pointsMSO;
		file << m_defaultOffset;
		file << m_syncFrame;
	}

public:
	Float GetSyncPointTime() const;

	Float GetDuration() const;

	Vector GetPointLS( Float time ) const;

	Vector GetPointWS( Float time, const Matrix& localToWorld ) const;

	void GetPointsWS( Float time, const Matrix& localToWorld, TDynArray< Vector >& points ) const;

	Bool FindKeys( Float time, Int32& frameA, Int32& frameB, Float& progress ) const;

private:
	void CalcKeys( Int32& frameA, Int32& frameB, Float& progress, Float time, Uint32 keysNum ) const;
	void CalcKeyPoints( Vector& pointM, Vector& pointMLS, Float time, const TDynArray< Vector >& LS, const TDynArray< Vector >& MS ) const;
	Vector CalcPoint( Float time, const Matrix& localToWorld, const TDynArray< Vector >& LS, const TDynArray< Vector >& MS ) const;
	void CalcPoints( Float time, const Matrix& localToWorld, TDynArray< Vector >& points, const TDynArray< Vector >& LS, const TDynArray< Vector >& MS ) const;
};

//////////////////////////////////////////////////////////////////////////

//dex++
class AnimationTrajectoryModifier_Null : public Red::System::NonCopyable
//dex--
{
public:
	struct Setup
	{
		Vector	m_pointWS;
		Matrix	m_localToWorld;
		Vector	m_dataSyncPointMS;
		Vector	m_currSyncPointMS;
		Float	m_duration;
	};

	const AnimationTrajectoryData& m_input;

public:
	AnimationTrajectoryModifier_Null( const AnimationTrajectoryData& input );

	void DoModifications( const Setup& setup, AnimationTrajectoryData& output ) const;
};

//dex++
class AnimationTrajectoryModifier_OnePoint : public Red::System::NonCopyable
//dex--
{
public:
	struct Setup
	{
		Vector	m_pointWS;
		Matrix	m_localToWorld;
		Vector	m_dataSyncPointMS;
		Vector	m_currSyncPointMS;
		Float	m_duration;
	};

	const AnimationTrajectoryData& m_input;

public:
	AnimationTrajectoryModifier_OnePoint( const AnimationTrajectoryData& input );

	void DoModifications( const Setup& setup, AnimationTrajectoryData& output ) const;
};

//dex++
class AnimationTrajectoryModifier_Blend2 : public Red::System::NonCopyable
//dex--
{
public:
	struct Setup
	{
		Vector	m_pointWS;
		Matrix	m_localToWorld;
		Vector	m_dataSyncPointMS;
		Vector	m_currSyncPointMS;
		Float	m_duration;
	};

	const AnimationTrajectoryData& m_inputA;
	const AnimationTrajectoryData& m_inputB;

public:
	AnimationTrajectoryModifier_Blend2( const AnimationTrajectoryData& inputA, const AnimationTrajectoryData& inputB );

	void DoModifications( const Setup& setup, AnimationTrajectoryData& output ) const;
};

//////////////////////////////////////////////////////////////////////////

#include "animationTrajectory.inl"
