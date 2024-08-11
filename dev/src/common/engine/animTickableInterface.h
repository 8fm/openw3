
#pragma once

enum EAsyncAnimPriority
{
	AAP_High,		// Npcs
	AAP_Normal,		// Background objects - cutscenes
	AAP_Low,		// Background objects - animated entities, flags, tents ...	
	AAP_Last,		// Marker
};

class IAnimAsyncTickable
{
public:
	virtual EAsyncAnimPriority GetPriority() const = 0;
	virtual Box GetBox() const = 0;

	virtual void DoAsyncTick( Float dt ) = 0;
};

class IAnimSyncTickable
{
public:
	virtual void DoSyncTick( Float dt ) = 0;
};
