//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/SoundFrame/SF.h>

using namespace AK;
using namespace SoundFrame;

class ICarSimUI
{
public:
	enum ObjectType
	{
		Type_Event,
		Type_GameParameter,
		Type_AuxBus
	};

	// Tell the that the simulation settings have been changed
	// Usually when you load a simulation file or when you connected or disconnected from Wwise.
	virtual void OnSimulationSettingsChanged() = 0;

	// Called when an object notification is sent by the Sound frame
	virtual void OnNotif( ObjectType in_eObjectType, IClient::Notif in_eNotif, AkUniqueID in_objectID ) = 0;

	// Called each frame to tell the UI to update the simulation states
	virtual void UpdateFrame() = 0;
};