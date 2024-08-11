/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"

enum EFinisherSide
{
	FinisherLeft,
	FinisherRight
};

BEGIN_ENUM_RTTI( EFinisherSide )
	ENUM_OPTION( FinisherLeft )
	ENUM_OPTION( FinisherRight )
END_ENUM_RTTI()


class CR4FinisherDLC: public CObject
{
	DECLARE_ENGINE_CLASS( CR4FinisherDLC, CObject, 0 );
public: 
	CR4FinisherDLC();

protected:
	void funcIsFinisherForAnim( CScriptStackFrame& stack, void* result );

private:	
	CName m_finisherAnimName;
	CName m_woundName;
	EFinisherSide m_finisherSide;
	CName m_leftCameraAnimName;
	CName m_rightCameraAnimName;
	CName m_frontCameraAnimName;
	CName m_backCameraAnimName;
};

BEGIN_CLASS_RTTI( CR4FinisherDLC );
PARENT_CLASS( CObject );	
	PROPERTY_EDIT( m_finisherAnimName,		TXT("Finisher animation name") )
	PROPERTY_EDIT( m_woundName,				TXT("Wound name") )
	PROPERTY_EDIT( m_finisherSide,			TXT("Finisher animation side") )
	PROPERTY_EDIT( m_leftCameraAnimName,	TXT("Left side camera animation name") )
	PROPERTY_EDIT( m_rightCameraAnimName,	TXT("Right side camera animation name") )
	PROPERTY_EDIT( m_frontCameraAnimName,	TXT("Front camera animation name") )
	PROPERTY_EDIT( m_backCameraAnimName,	TXT("Back camera animation name") )
	NATIVE_FUNCTION( "IsFinisherForAnim", funcIsFinisherForAnim )
END_CLASS_RTTI();

class CR4FinishersDLCMounter : public IGameplayDLCMounter
{
	DECLARE_ENGINE_CLASS( CR4FinishersDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4FinishersDLCMounter();

	//! IGameplayDLCMounter
public:
	virtual bool OnCheckContentUsage() override;
	virtual void OnGameStarting() override;
	virtual void OnGameStarted() override;
	virtual void OnGameEnding() override;

#ifndef NO_EDITOR

	virtual void OnEditorStarted() override;
	virtual void OnEditorStopped() override;

	virtual bool DoAnalyze( CAnalyzerOutputList& outputList ) override;

#endif // !NO_EDITOR

private:
	void ActivatePhase1();
	void ActivatePhase2();
	void Deactivate();

	THandle< CSkeletalAnimationSet > m_customCameraAnimSet;
	Bool m_customCameraAnimSetLoaded;

	typedef TDynArray< CR4FinisherDLC* > TCR4FinishersDLC;
	TCR4FinishersDLC m_finishers;
};

BEGIN_CLASS_RTTI( CR4FinishersDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
	PROPERTY_EDIT( m_customCameraAnimSet, TXT("Animations used by the camera in finishers") )
	PROPERTY_INLINED( m_finishers, TXT("Finishers") )
END_CLASS_RTTI();
