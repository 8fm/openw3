//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Car.h"

namespace
{
	static const double s_PI = 3.14159;
	static const double s_airDensity = 1.205;	// Kg/m^3
}

Car::Car()
: m_dblThrottle( 0.0 )
, m_dblSpeed( 0.0 )
, m_dblLoad( 0.0 )
, m_nCurrentGear( 0 )
{
	SetDefaultCarParam();
}

Car::~Car()
{}

void Car::SetCarParams( const CarParams& in_rCarParam )
{
	m_carParams = in_rCarParam;
}

const CarParams& Car::GetCarParam() const
{
	return m_carParams;
}

CarParams& Car::GetCarParam()
{
	return m_carParams;
}

void Car::SetDefaultCarParam()
{
	// Default Car Param ...
	m_carParams.m_dblWeight = CarParam::s_dblDefaultWeight;
	m_carParams.m_dblTorque = CarParam::s_dblDefaultTorque;
	m_carParams.m_dblDragCoefficient = CarParam::s_dblDefaultDragCoefficient;
	m_carParams.m_dblFrontSurface = CarParam::s_dblDefaultFrontSurface;
	m_carParams.m_dblRollingResistence = CarParam::s_dblDefaultRollingResistence;
	m_carParams.m_nNumOfGear = CarParam::s_nDefaultNumOfGear;
	m_carParams.m_gearRatios.resize(CarParam::s_nMaxNumOfGear);	
	m_carParams.m_gearRatios[0] = CarParam::s_dblDefaultGear1;
	m_carParams.m_gearRatios[1] = CarParam::s_dblDefaultGear2;
	m_carParams.m_gearRatios[2] = CarParam::s_dblDefaultGear3;
	m_carParams.m_gearRatios[3] = CarParam::s_dblDefaultGear4;
	m_carParams.m_gearRatios[4] = CarParam::s_dblDefaultGear5;
	m_carParams.m_gearRatios[5] = CarParam::s_dblDefaultGear6;
	m_carParams.m_gearRatios[6] = CarParam::s_dblDefaultGear7;
	m_carParams.m_dblDifferentialRatio = CarParam::s_dblDefaultDifferentialRatio;
	m_carParams.m_dblWheelRadius = CarParam::s_dblDefaultWheelRadius;
	m_carParams.m_dblMinRPM = CarParam::s_dblDefaultMinRPM;
	m_carParams.m_dblRPMCutOff = CarParam::s_dblDefaultRPMCutOff;

	// Adjust current gear
	m_nCurrentGear = min( m_nCurrentGear, m_carParams.m_nNumOfGear - 1 );
}

double Car::GetCarEngineForce() const
{
	double dblEngineTorque = m_dblThrottle * m_carParams.m_dblTorque;
	if( GetRPM() > m_carParams.m_dblRPMCutOff && dblEngineTorque > 0.0 )
		dblEngineTorque = 0.0; // Direct cuf off RPM better to have a progressive curve

	return ( dblEngineTorque * m_carParams.m_gearRatios[m_nCurrentGear] * m_carParams.m_dblDifferentialRatio ) / m_carParams.m_dblWheelRadius;
}

double Car::GetDragForce() const
{
	return (-0.5)* s_airDensity * m_carParams.m_dblDragCoefficient * m_carParams.m_dblFrontSurface * m_dblSpeed * m_dblSpeed;
}

double Car::GetFrictionForce() const
{
	return - m_carParams.m_dblRollingResistence * m_dblSpeed;
}

double Car::GetRPM() const
{
	const double dblRPS = ( m_dblSpeed / ( ( 2 * s_PI ) * m_carParams.m_dblWheelRadius ) ) * m_carParams.m_gearRatios[m_nCurrentGear] * m_carParams.m_dblDifferentialRatio;

	return max( m_carParams.m_dblMinRPM, dblRPS * 60 );
}

void Car::SetThrottle( const double& in_rThrottle )
{
	m_dblThrottle = min( 1.0, max( -1.0, in_rThrottle ) );
}

double Car::GetThrottle() const
{
	return m_dblThrottle;
}

void Car::SetSpeed( const double& in_rSpeed )
{
	m_dblSpeed = in_rSpeed;
}

double Car::GetSpeed() const
{
	return m_dblSpeed;
}

void Car::SetLoad( const double& in_rLoad )
{
	m_dblLoad = min( 1.0, max( -1.0, in_rLoad ) );
}

double Car::GetLoad() const
{
	return m_dblLoad;
}

void Car::SetCurrentGear( size_t in_currentGear )
{
	if( in_currentGear >= 0 && in_currentGear < m_carParams.m_nNumOfGear )
		m_nCurrentGear = in_currentGear;
}

size_t Car::GetCurrentGear() const
{
	return m_nCurrentGear;
}

void Car::ResetSimulationData()
{
	m_dblThrottle = 0.0;
	m_dblSpeed = 0.0;
	m_dblLoad = 0.0;
	m_nCurrentGear = 0;
}