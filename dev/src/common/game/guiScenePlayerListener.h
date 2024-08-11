/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CEntity;

class IGuiScenePlayerListener
{
public:
	virtual void OnGuiSceneEntitySpawned( const THandle< CEntity >& spawnedEntity )=0;
	virtual void OnGuiSceneEntityDestroyed()=0;
//	virtual void OnGuiSceneError()=0;

protected:
	virtual ~IGuiScenePlayerListener() {}
};