#include "build.h"
#include "horseSteering.h"
#include "../../common/game/movementCommandBuffer.h"
#include "../../common/game/movementGoal.h"
#include "w3GenericVehicle.h"

///////////////////////////////////////////////////////////////////////////////
// CMoveSTRuberBand
///////////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CMoveSTRuberBand );
CMoveSTRuberBand::CMoveSTRuberBand()
	: m_halfRange( 100.0f )
	, m_tensionCurve( SCT_Float ) 
	, m_minAllowedSpeed( 0.6f )
{
	m_tensionCurve.SetUseDayTime( false );
	m_tensionCurve.AddPoint( 0.0f, 1.5f );
	m_tensionCurve.AddPoint( 0.25f, 1.5f );
	m_tensionCurve.AddPoint( 0.30f, 1.2f );
	m_tensionCurve.AddPoint( 0.375f, 1.2f );
	m_tensionCurve.AddPoint( 0.40f, 1.0f );

	m_tensionCurve.AddPoint( 0.6f, 1.0f );
	m_tensionCurve.AddPoint( 0.625f, 0.75f );
	m_tensionCurve.AddPoint( 0.70f, 0.75f );
	m_tensionCurve.AddPoint( 0.75f, 0.5f );
	m_tensionCurve.AddPoint( 1.00f, 0.5f );
}
void CMoveSTRuberBand::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	Float speed = comm.GetSpeed();
	Float distanceToTarget;
	if ( comm.GetGoal().GetFlag( CNAME( RubberBandDistance ), distanceToTarget ) )
	{
		Float t = Clamp( distanceToTarget, -m_halfRange, m_halfRange );
		t		= t + m_halfRange; // moving into [ 0, 2*m_halfRange ]
		t		= t / (2 * m_halfRange); // moving into [ 0, 1 ]
		const Float tension = m_tensionCurve.GetFloatValue( t );
		speed *= tension;
		speed = Max( m_minAllowedSpeed, speed );
		comm.ModifySpeed( speed );
	}

}

String CMoveSTRuberBand::GetTaskName() const
{
	static const String TASKNAME( TXT( "Ruber band" ) );
	return TASKNAME;
}

///////////////////////////////////////////////////////////////////////////////
// CMoveSTHorse
///////////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CMoveSTHorse );
CMoveSTHorse::CMoveSTHorse()
	: m_horseSlowWalkMult( 0.2f )
	, m_horseWalkMult( 0.4f )
	, m_horseTrotMult( 0.6f )
	, m_horseGallopMult( 0.8f )
	, m_horseCanterMult( 1.f )
{
	
}
void CMoveSTHorse::CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const
{
	
/*	EHorseMoveType horseMoveType;
	if ( comm.GetGoal().GetFlag( CNAME( HorseMoveType ), *((Int32*)&horseMoveType) ) == false )
	{
		EMoveType moveType;
		Float unsused;
		comm.GetAgent().GetMoveType( moveType, unsused );
		if ( moveType == MT_Run )
		{
			horseMoveType = HMT_Gallop;
		}
		else
		{
			horseMoveType = HMT_Trot;
		}
	}
	Float speed		= comm.GetSpeed();


	Float speedMult = 1.0f;
	switch ( horseMoveType )
	{
	case HMT_SlowWalk:
		speedMult = m_horseSlowWalkMult;
		break;
	case HMT_Walk:
		speedMult = m_horseWalkMult;
		break;
	case HMT_Trot:
		speedMult = m_horseTrotMult;
		break;
	case HMT_Gallop:
		speedMult = m_horseGallopMult;
		break;
	case HMT_Canter:
		speedMult = m_horseCanterMult;
		break;
	}
	
	speed *= speedMult;
	comm.ModifySpeed( speed );
	comm.GetGoal().SnapToMinimalSpeed( true );*/
}

String CMoveSTHorse::GetTaskName() const
{
	static const String TASKNAME( TXT( "Horse steering" ) );
	return TASKNAME;
}