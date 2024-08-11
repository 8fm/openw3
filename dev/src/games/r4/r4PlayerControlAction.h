#pragma once

#include "../../common/game/actorAction.h"

class CR4LocomotionDirectController;

class CR4PlayerControlAction : public ActorAction, public CMoveLocomotion::IController
{
	typedef ActorAction Super;
protected:
	THandle< CR4LocomotionDirectController >			m_controller;
public:
	CR4PlayerControlAction( CR4Player* player );
	~CR4PlayerControlAction();

	////////////////////////////////////////////////////////////////////
	// ActorAction life cycle
	Bool Start( const THandle< CR4LocomotionDirectController >& playerController );
	Bool Update( Float timeDelta ) override;
	void Stop() override;

	virtual Bool CanUseLookAt() const override { return true; }

	String GetDescription() const override;
	void OnGCSerialize( IFile& file ) override;
	////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////
	// CMoveLocomotion::IController implementation
	void OnSegmentFinished( EMoveStatus status ) override;
	void OnControllerAttached() override;
	void OnControllerDetached() override;
	////////////////////////////////////////////////////////////////////
};