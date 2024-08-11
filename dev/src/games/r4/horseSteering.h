#pragma once
#include "../../common/game/moveSteeringTask.h"
#include "../../common/game/generalMoveSteeringTask.h"

///////////////////////////////////////////////////////////////////////////////
// CMoveSTRuberBand
class CMoveSTRuberBand : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTRuberBand, IMoveSteeringTask, 0 );

protected:
	Float			m_halfRange;
	SSimpleCurve	m_tensionCurve;
	Float			m_minAllowedSpeed;
public:
	CMoveSTRuberBand();

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CMoveSTRuberBand );
	PARENT_CLASS( IMoveSteeringTask );
	PROPERTY_EDIT( m_halfRange, TXT("The [ 0, 1 ] of the curve corresponds to [ -halfRange, halfRange ]") )
	PROPERTY_EDIT( m_tensionCurve, TXT("Tension of the ruber band between [ 0, 1 ] that translates to [ -halfRange, halfRange ]") )
	PROPERTY_EDIT( m_minAllowedSpeed, TXT("Minimum speed the rubberband can set") );
END_CLASS_RTTI();




///////////////////////////////////////////////////////////////////////////////
// CMoveSTHorse
class CMoveSTHorse : public IMoveSteeringTask
{
	DECLARE_ENGINE_CLASS( CMoveSTHorse, IMoveSteeringTask, 0 );

protected:
	Float			m_horseSlowWalkMult;
	Float			m_horseWalkMult;
	Float			m_horseTrotMult;
	Float			m_horseGallopMult;
	Float			m_horseCanterMult;
public:
	CMoveSTHorse();

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;

	String GetTaskName() const override;
};

BEGIN_CLASS_RTTI( CMoveSTHorse );
PARENT_CLASS( IMoveSteeringTask );
	PROPERTY_EDIT( m_horseSlowWalkMult, TXT("Horse slow walk mult on speed [Hack]") )
	PROPERTY_EDIT( m_horseWalkMult, TXT("Horse walk mult on speed [Hack]") )
	PROPERTY_EDIT( m_horseTrotMult, TXT("Horse trot mult on speed [Hack]") )
	PROPERTY_EDIT( m_horseGallopMult, TXT("Horse gallop mult on speed [Hack]") )
	PROPERTY_EDIT( m_horseCanterMult, TXT("Horse canter mult on speed [Hack]") )
END_CLASS_RTTI();
