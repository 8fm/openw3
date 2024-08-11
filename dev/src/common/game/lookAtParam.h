
#pragma once

class CLookAtStaticParam : public CEntityTemplateParam
{
	DECLARE_ENGINE_CLASS( CLookAtStaticParam, CEntityTemplateParam, 0 );

protected:
	ELookAtLevel	m_maxLookAtLevel;
	Float			m_maxHorAngle;
	Float			m_maxVerAngle;
	Float			m_firstWeight;
	Float			m_secWeight;
	Float			m_responsiveness;

public:
	CLookAtStaticParam();

public:
	ELookAtLevel	GetLevel() const;
	Float			GetMaxAngleVer() const;
	Float			GetMaxAngleHor() const;
	Float			GetResponsiveness() const;
	Float			GetWeight( Uint32 segmentNum ) const;
};

BEGIN_CLASS_RTTI( CLookAtStaticParam );
	PARENT_CLASS( CEntityTemplateParam );
	PROPERTY_EDIT( m_maxLookAtLevel, TXT("Max look at level for actor") );
	PROPERTY_EDIT( m_maxHorAngle, TXT("Max horizontal angle [deg]") );
	PROPERTY_EDIT( m_maxVerAngle, TXT("Max vertical angle [deg]") );
	PROPERTY_EDIT( m_secWeight, TXT("Head weight") );
	PROPERTY_EDIT( m_firstWeight, TXT("Torso weight") );
	PROPERTY_EDIT( m_responsiveness, TXT("Responsiveness") );
END_CLASS_RTTI();
