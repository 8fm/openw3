#pragma once



enum EDamageType
{
	DT_Cut,
	DT_Fire,
	DT_Bullet,

	DT_Size
};

BEGIN_ENUM_RTTI( EDamageType );
	ENUM_OPTION( DT_Cut );
	ENUM_OPTION( DT_Fire );
	ENUM_OPTION( DT_Bullet );	
	ENUM_OPTION( DT_Size );
END_ENUM_RTTI();


struct SR6DamageData
{
	DECLARE_RTTI_STRUCT( SR6DamageData );

	Float		m_damage;
	CName		m_bodyPartName;
	Vector		m_hitDirection;
	EDamageType	m_damageType;
};


BEGIN_CLASS_RTTI( SR6DamageData )
	PROPERTY_NAME( m_damage			, TXT("i_damageF")  );
	PROPERTY_NAME( m_bodyPartName	, TXT("i_bodyPartNameN") );
	PROPERTY_NAME( m_hitDirection	, TXT("i_hitDirectionV") );
	PROPERTY_NAME( m_damageType		, TXT("i_damageTypeE") );
END_CLASS_RTTI()