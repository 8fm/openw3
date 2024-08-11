/*																										/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CIDInterlocutorComponent;
struct SIDBaseLine;

enum EHudChoicePosition
{
	CHOICE_Left = 0,
	CHOICE_Right,
	CHOICE_Up,
	CHOICE_Down,

	CHOICE_Max
};

BEGIN_ENUM_RTTI( EHudChoicePosition )
	ENUM_OPTION( CHOICE_Left )
	ENUM_OPTION( CHOICE_Right )
	ENUM_OPTION( CHOICE_Up )
	ENUM_OPTION( CHOICE_Down )
END_ENUM_RTTI()


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
enum EDialogDisplayMode
{
	DDM_ActiveDialog	= 0	,
	DDM_IncomingCall		,
	DDM_ActiveCall			,
	DDM_SMSReceived			,
	DDM_OutgoingCall		,
};
BEGIN_ENUM_RTTI( EDialogDisplayMode );
	ENUM_OPTION( DDM_ActiveDialog );
	ENUM_OPTION( DDM_IncomingCall );
	ENUM_OPTION( DDM_ActiveCall );
	ENUM_OPTION( DDM_SMSReceived );
	ENUM_OPTION( DDM_OutgoingCall );
END_ENUM_RTTI();

class IDialogHud : public IInputListener
{
public:
	virtual void PlayDialogLine		( CIDInterlocutorComponent* interlocutor, const SIDBaseLine& line ) = 0;
	virtual void EndDialogLine		( CIDInterlocutorComponent* interlocutor ) = 0;
	virtual void ClearDialogLines	( )	= 0;
	virtual void AddChoice			( const SIDOption& choice ) = 0;
	virtual void RemoveChoice		( const SIDOption& choice ) = 0;
	virtual void RemoveAllChoices	( ) = 0;
	virtual void SetMode			( EDialogDisplayMode displayMode )	= 0;
	virtual void OnInterrupted		( ) = 0;

	// IInputListener
	virtual Bool OnGameInputEvent	( const SInputAction & action ) = 0;

protected:
	virtual void UpdateInput		( );
	virtual void OnChoiceSelected	( EHudChoicePosition pos ) const = 0;
};

