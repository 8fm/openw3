#pragma once

#define PROBES_NUM 8

struct SPredictionInfo
{
	DECLARE_RTTI_STRUCT( SPredictionInfo );

	Float	m_distanceToCollision;
	Float	m_normalYaw;
	Float	m_turnAngle;
	Float	m_leftGroundLevel;
	Float	m_frontGroundLevel;
	Float	m_rightGroundLevel;
};

BEGIN_CLASS_RTTI( SPredictionInfo )
	PROPERTY( m_distanceToCollision )
	PROPERTY( m_normalYaw )
	PROPERTY( m_turnAngle )
	PROPERTY( m_leftGroundLevel )
	PROPERTY( m_frontGroundLevel )
	PROPERTY( m_rightGroundLevel )
END_CLASS_RTTI()

class CHorsePrediction : public CObject
{
	DECLARE_ENGINE_CLASS( CHorsePrediction, CObject, 0 );

private:
	static const Float	Z_OFFSET;
	static const Float	SWEEP_RAD;
	static const Float	PROBE_RAD;
	static const Vector OFFSETS[ PROBES_NUM ];

	enum EPrevTurn
	{
		PT_None,
		PT_Right,
		PT_Left
	} m_prevTurn;

	Float	m_hits[ PROBES_NUM ];

	THandle<CEntity>	m_horse;

#ifndef RED_FINAL_BUILD
	Vector	m_dbgProbes[ PROBES_NUM ];
	Vector	m_dbgSweep[2];
	Vector	m_dbgHeightTrace[3];
#endif

public:

#ifndef RED_FINAL_BUILD
	static TDynArray<CHorsePrediction*> m_dbgFragments;
	CHorsePrediction() { m_dbgFragments.PushBack( this ); }
	virtual ~CHorsePrediction() { m_dbgFragments.Remove( this ); }
#else
	CHorsePrediction() {}
#endif

	void GenerateDebugFragments( CRenderFrame* frame );

	SPredictionInfo CollectPredictionInfo( CNode* ent, Float testDistance, Float inputDir, Bool checkWater );

private:
	void funcCollectPredictionInfo( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CHorsePrediction );
	PARENT_CLASS( CObject );
	NATIVE_FUNCTION( "CollectPredictionInfo", funcCollectPredictionInfo );
END_CLASS_RTTI();