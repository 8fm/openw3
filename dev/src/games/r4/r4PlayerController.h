#pragma once

class CR4LocomotionDirectController : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CR4LocomotionDirectController, CObject );
protected:
	THandle< CMovingAgentComponent >			m_agent;

	Float										m_moveSpeed;
	Float										m_moveRotation;

	virtual void UpdateLocomotion() = 0;
public:
	CR4LocomotionDirectController();
	~CR4LocomotionDirectController();

	void Update();

	virtual Bool Activate();
	virtual void Deactivate();

	void SetMovingAgent( CMovingAgentComponent* agent )					{ m_agent = agent; }
};

BEGIN_ABSTRACT_CLASS_RTTI( CR4LocomotionDirectController );
	PARENT_CLASS( CObject );
	PROPERTY( m_agent );
	PROPERTY( m_moveSpeed );
	PROPERTY( m_moveRotation );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////


class CR4LocomotionDirectControllerScript : public CR4LocomotionDirectController
{
	DECLARE_ENGINE_CLASS( CR4LocomotionDirectControllerScript, CR4LocomotionDirectController, 0 );
protected:
	void UpdateLocomotion() override;
public:
	CR4LocomotionDirectControllerScript();
	~CR4LocomotionDirectControllerScript();

	Bool Activate() override;
	void Deactivate() override;
};

BEGIN_CLASS_RTTI( CR4LocomotionDirectControllerScript );
	PARENT_CLASS( CR4LocomotionDirectController );
END_CLASS_RTTI();
