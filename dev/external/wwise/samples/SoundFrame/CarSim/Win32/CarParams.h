//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

struct CarParams
{
	double m_dblWeight;				// Kg
	double m_dblTorque;				// Newton-meter ( constant value but Should be a table torque / RPM)
	double m_dblDragCoefficient;	// Cd in metrics units
	double m_dblFrontSurface;		// square meters
	double m_dblRollingResistence;
	size_t m_nNumOfGear;
	std::vector<double> m_gearRatios;
	double m_dblDifferentialRatio;
	double m_dblWheelRadius;		// Meters
	double m_dblMinRPM;
	double m_dblRPMCutOff;
};

namespace CarParam
{
	// Default value
	static const double s_dblDefaultWeight( 1470.0 );		// Kg
	static const double s_dblDefaultTorque( 370.0 );		// Newton-meter ( constant value but Should be a table torque / RPM)
	static const double s_dblDefaultDragCoefficient( 0.29 );	// Cx in metrics units
	static const double s_dblDefaultFrontSurface( 1.5 );		// square meters
	static const double s_dblDefaultRollingResistence( 0.05 );
	static const size_t s_nDefaultNumOfGear( 6 );
	// Default gears ratio
	static const double s_dblDefaultGear1( 3.82 );
	static const double s_dblDefaultGear2( 2.05 );
	static const double s_dblDefaultGear3( 1.41 );
	static const double s_dblDefaultGear4( 1.12 );
	static const double s_dblDefaultGear5( 0.92 );
	static const double s_dblDefaultGear6( 0.75 );
	static const double s_dblDefaultGear7( 0.65 );

	static const double s_dblDefaultDifferentialRatio( 3.44 );
	static const double s_dblDefaultWheelRadius( 0.23 );	// 18 inches tires ~ radius of 23 cm
	static const double s_dblDefaultMinRPM( 1000 );
	static const double s_dblDefaultRPMCutOff( 7500 );

	// Value range ( mainly for UI purpose )
	static const double s_dblMinWeight( 1.0 );
	static const double s_dblMaxWeight( 100000.0 );	// Should be enought for any car ....

	static const double s_dblMinTorque( 1.0 );
	static const double s_dblMaxTorque( 10000.0 );

	static const double s_dblMinDragCoefficient( 0.0 );
	static const double s_dblMaxDragCoefficient( 1000.0 );

	static const double s_dblMinFrontSurface( 0.0 );
	static const double s_dblMaxFrontSurface( 1000.0 );

	static const double s_dblMinRollingResistence( 0.0 );
	static const double s_dblMaxRollingResistence( 100.0 );

	static const size_t s_nMinNumOfGear( 1 );
	static const size_t s_nMaxNumOfGear( 7 );

	static const double s_dblMinGearRatio( 0.01 );
	static const double s_dblMaxGearRatio( 5.00 );

	static const double s_dblMinDifferentialRatio( 0.01 );
	static const double s_dblMaxDifferentialRatio( 100.0 );

	static const double s_dblMinWheelRadius( 0.01 );
	static const double s_dblMaxWheelRadius( 100.0 );

	static const double s_dblMinRPM( 0.0 );
	static const double s_dblMaxRPM( 100000.0 );
}