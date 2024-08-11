/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "soundEmitter.h"

//////////////////////////////////////////////////////////////////////////

enum EAmbientSoundType
{
	EAST_Area,
	EAST_FX
};

// Interface for ambient sound components
class IAmbientSound 
{
public:
	virtual ~IAmbientSound() {}

	virtual Bool Play( ) = 0;
	virtual void Stop() = 0;
	virtual Bool IsPlaying() const = 0;
	virtual Bool HasFailed() const = 0;

	virtual Float GetMaxDistance() const = 0;
	virtual const Vector& GetSoundPosition() const = 0;
	virtual void UpdateSoundPosition( const Vector& listenerPosition ) {}

	// Gets reverb definition assigned to given ambient
	virtual const StringAnsi& GetReverbName() { return StringAnsi::EMPTY; }

	virtual EAmbientSoundType GetType() const = 0;
};

//////////////////////////////////////////////////////////////////////////

// Class responsible for all ambient sounds mangling
class CSoundAmbientManager : public Red::System::NonCopyable, public CSoundEmitter
{
	friend class CSoundSystem;
protected:
	// Activator for ambient area triggers
	ITriggerActivator*			m_ambientActivator;

	TDynArray< THandle< CSoundAmbientAreaComponent > > m_currentAmbients;

	enum ESoundAmbientManagerState
	{
		ESAMS_Inactive,
		ESAMS_Inited,
		ESAMS_FirstRun,
		ESAMS_Active
	};

	THandle< CSoundAmbientAreaComponent > m_listenerCurrentAmbientArea;
	THandle< CSoundAmbientAreaComponent > m_listenerCurrentMusicArea;

	TDynArray< SSoundGameParameterValue > m_currentParameters;

public:
	CSoundAmbientManager();
	virtual ~CSoundAmbientManager();

	void Init();
	void Shutdown();
	void Reset();

	// Update ambient manager, a valid global listener position is required
	void Tick( const Vector& listenerPosition, Float dt );

	// Update sounds triggered on world attach, restart them if they have stopped
	void UpdateWorldSounds();

	//Call when an ambient area is detatched to handle any cleanup e.g. ambient parameters
	void DetachAmbientArea(const CSoundAmbientAreaComponent * areaComponent);
	
	// Updates all area components to latest priority setting
	void UpdateAmbientPriorities();

private:
	void UpdateParameter( CSoundAmbientAreaComponent* area, CSoundAmbientAreaComponent* newArea, const char* parameterName, float value, float newValue );

#ifndef RED_FINAL_BUILD
	virtual StringAnsi GetSoundObjectName() const  { return "SOUND_AMBIENT_MANAGER"; }
#endif

};
