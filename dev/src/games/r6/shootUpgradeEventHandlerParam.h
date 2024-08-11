#pragma once


#include "baseUpgradeEventHandlerParam.h"


class CShootUpgradeHandlerParam : public CUpgradeEventHandlerParam
{
	DECLARE_ENGINE_CLASS( CShootUpgradeHandlerParam, CUpgradeEventHandlerParam, 0 );

private:
	Vector				m_shootDirection;
	THandle< CEntity >	m_shootingTarget;
public:
	RED_INLINE const Vector& GetShootDirection(){ return m_shootDirection; }
	RED_INLINE void SetShootDirection( const Vector& val ){ m_shootDirection = val; }
	RED_INLINE THandle< CEntity > GetShootingTarget( ){ return m_shootingTarget; }
};

BEGIN_CLASS_RTTI( CShootUpgradeHandlerParam );
	PARENT_CLASS( CUpgradeEventHandlerParam );
	PROPERTY_EDIT_NAME( m_shootDirection, TXT("i_shootDirection"), TXT("Direction of shoot") );
	PROPERTY_NAME( m_shootingTarget, TXT("i_shootingTarget") );
END_CLASS_RTTI();
