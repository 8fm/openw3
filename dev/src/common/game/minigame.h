/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

RED_DECLARE_NAME( Minigame );

#ifndef NO_LOG

#define MINIGAME_LOG( format, ... )		RED_LOG( Minigame, format, ## __VA_ARGS__ )
#define MINIGAME_WARN( format, ... )	RED_LOG( Minigame, format, ## __VA_ARGS__ )
#define MINIGAME_ERROR( format, ... )	RED_LOG( Minigame, format, ## __VA_ARGS__ )

#else

#define MINIGAME_LOG( format, ... )		
#define MINIGAME_WARN( format, ... )	
#define MINIGAME_ERROR( format, ... )	

#endif

enum EMinigameState
{
	EMS_None			= FLAG( 1 ),
	EMS_Init			= FLAG( 2 ),
	EMS_Started			= FLAG( 3 ),
	EMS_End_PlayerWon	= FLAG( 4 ),
	EMS_End_PlayerLost  = FLAG( 5 ),
	EMS_End_Error		= FLAG( 6 ),
	EMS_Wait_PlayerLost = FLAG( 7 ),
	EMS_End				= EMS_End_PlayerWon | EMS_End_PlayerLost | EMS_End_Error
};

BEGIN_ENUM_RTTI( EMinigameState );
	ENUM_OPTION( EMS_None );
	ENUM_OPTION( EMS_Init );
	ENUM_OPTION( EMS_Started );
	ENUM_OPTION( EMS_End_PlayerWon );
	ENUM_OPTION( EMS_End_PlayerLost );
	ENUM_OPTION( EMS_End_Error );
	ENUM_OPTION( EMS_Wait_PlayerLost );
	ENUM_OPTION( EMS_End );
END_ENUM_RTTI();

class CMinigameInstanceData
{
public:

	CMinigameInstanceData() : m_state( EMS_None )
	{}

	EMinigameState					m_state;

	Bool IsStarted() const { return (m_state & EMS_Started) != 0; }
	Bool IsFinished() const { return (m_state & EMS_End) != 0; }
	Bool HasPlayerWon() const { return (m_state & EMS_End_PlayerWon) != 0; }
};



/// Minigame base
class CMinigame : public CObject
{
	DECLARE_ENGINE_CLASS( CMinigame, CObject, 0 );

public:

	CMinigame();
	virtual ~CMinigame();

	CMinigameInstanceData* StartGame() const
	{
		CMinigameInstanceData* data = InitInstanceData();
		data->m_state = EMS_Init;
		OnStartGame( data );
		return data;
	}

	void StartGame( CMinigameInstanceData* data ) const
	{
		OnStartGame( data );
	}

	Bool IsFinished( CMinigameInstanceData* data ) const
	{
		if ( data )
		{
			return OnIsFinished( data );
		}
		return true;
	}

	Bool HasStarted( CMinigameInstanceData* data ) const
	{
		if ( data )
		{
			return data->m_state != EMS_Init;
		}
		return true;
	}

	void EndGame( CMinigameInstanceData* data ) const
	{		
		if ( data )
		{
			OnEndGame( data );
			delete data;
		}
	}

protected:
	virtual CMinigameInstanceData* InitInstanceData() const
	{
		return new CMinigameInstanceData();
	}
	virtual void OnStartGame( CMinigameInstanceData* ) const = 0;
	virtual void OnEndGame( CMinigameInstanceData* ) const = 0;
	virtual Bool OnIsFinished( CMinigameInstanceData* ) const = 0;
	
};

BEGIN_ABSTRACT_CLASS_RTTI( CMinigame );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////