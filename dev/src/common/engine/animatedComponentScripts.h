
#pragma once

struct SAnimatedComponentSyncSettings
{
	DECLARE_RTTI_STRUCT( SAnimatedComponentSyncSettings ); 

	CName	m_instanceName;
	Bool	m_syncAllInstances;
	Bool	m_syncEngineValueSpeed;

	SAnimatedComponentSyncSettings() : m_syncAllInstances( true ), m_syncEngineValueSpeed( false ) {}
};

BEGIN_CLASS_RTTI( SAnimatedComponentSyncSettings );
	PROPERTY_EDIT( m_instanceName, TXT("") );
	PROPERTY_EDIT( m_syncAllInstances, TXT("") );
	PROPERTY_EDIT( m_syncEngineValueSpeed, TXT("") );
END_CLASS_RTTI();

struct SAnimatedComponentSlotAnimationSettings
{
	DECLARE_RTTI_SIMPLE_CLASS( SAnimatedComponentSlotAnimationSettings ); 

	Float	m_blendIn;
	Float	m_blendOut;

	SAnimatedComponentSlotAnimationSettings() : m_blendIn( 0.2f ), m_blendOut( 0.2f ) {}
};

BEGIN_CLASS_RTTI( SAnimatedComponentSlotAnimationSettings );
	PROPERTY_EDIT( m_blendIn, TXT("") );
	PROPERTY_EDIT( m_blendOut, TXT("") );
END_CLASS_RTTI();
