
#pragma once

// Temp shit for testing if everything is ok
class TCrBehaviorNodeController
{
	CAnimatedComponent*		m_component;

	Bool						m_hasVarHandPosL;
	Bool						m_hasVarHandPosR;

	Bool						m_hasVarHandL;
	Bool						m_hasVarHandR;

public:
	TCrBehaviorNodeController();
	~TCrBehaviorNodeController();

	void Init( CAnimatedComponent* component );
	void Deinit();

public:
	void SetEffector_HandL( const Vector& positionWS );
	void SetEffector_HandR( const Vector& positionWS );

	void SetEffectorActive_HandL( Float weight );
	void SetEffectorActive_HandR( Float weight );

	void SetWeaponOffset_HandL( Float weight );
	void SetWeaponOffset_HandR( Float weight );
};
