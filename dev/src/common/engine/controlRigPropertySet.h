
#pragma once

#include "controlRigIncludes.h"

class TCrPropertySet : public CObject
{
	DECLARE_ENGINE_CLASS( TCrPropertySet, CObject, 0 );

public:
	Float	m_shoulderWeight;
	Float	m_shoulderLimitUpDeg;
	Float	m_shoulderLimitDownDeg;
	Float	m_shoulderLimitLeftDeg;
	Float	m_shoulderLimitRightDeg;

public:
	TCrPropertySet();
};

BEGIN_CLASS_RTTI( TCrPropertySet )
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_shoulderWeight, String::EMPTY );
	PROPERTY_EDIT( m_shoulderLimitUpDeg, String::EMPTY );
	PROPERTY_EDIT( m_shoulderLimitDownDeg, String::EMPTY );
	PROPERTY_EDIT( m_shoulderLimitLeftDeg, String::EMPTY );
	PROPERTY_EDIT( m_shoulderLimitRightDeg, String::EMPTY );
END_CLASS_RTTI();
