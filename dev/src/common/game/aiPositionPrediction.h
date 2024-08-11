/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once


struct SAIPositionPrediction
{
	DECLARE_RTTI_STRUCT( SAIPositionPrediction )

	SAIPositionPrediction( Float time = 0.f )
		: m_inTime( time )
		, m_outIsHandled( false )														{}

	Float					m_inTime;
	Bool					m_outIsHandled;
	Vector					m_outPosition;

	static CName			EventName();
};

BEGIN_NODEFAULT_CLASS_RTTI( SAIPositionPrediction )
END_CLASS_RTTI()
