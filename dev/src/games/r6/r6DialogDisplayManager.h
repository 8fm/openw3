#pragma once

#include "r6SystemOrder.h"
#include "idHud.h"

class CR6DialogPage;
class CR6HudModule;
class CR6DialogBlockChoice;
struct SIDOption;


//------------------------------------------------------------------------------------------------------------------
// Displays dialog text
//------------------------------------------------------------------------------------------------------------------
class CR6DialogDisplay : public IGameSystem, public IDialogHud
{	
	DECLARE_R6_GAME_SYSTEM( CR6DialogDisplay, GSR6_DialogDisplay );

#ifndef NO_DEBUG_PAGES
	friend class CDebugPageInteractiveDialogs; 
#endif

protected:
	struct SPlayingLine
	{
		CIDInterlocutorComponent*	m_speaker;
		const SIDBaseLine*			m_line;
		Bool						m_needsToDisplay;

		SPlayingLine()
			: m_speaker( NULL )
			, m_line( NULL )
			, m_needsToDisplay( true )
		{
		}																			

		SPlayingLine( CIDInterlocutorComponent* speaker, const SIDBaseLine* line )
			: m_speaker( speaker )
			, m_line( line )
			, m_needsToDisplay( true )
		{
		}

		RED_INLINE Bool ShouldPlay() const { return m_speaker && m_line && m_needsToDisplay; };
	};

	TDynArray< SPlayingLine >		m_playingLines; 
	const SIDOption*				m_shownChoices[ CHOICE_Max ];
	CR6HudModule*					m_hudDialogueModule;
	Float							m_clearLineCooldownCur;
	Float							m_clearLineCooldownTotal;
	Uint32							m_selectedChoices;

	Bool							m_active;

	//------------------------------------------------------------------------------------------------------------------
	// Methods
	//------------------------------------------------------------------------------------------------------------------
public:
	CR6DialogDisplay();

	// IGameSystem interface
	void					OnGameStart					( const CGameInfo& gameInfo );
	void					OnGameEnd					( const CGameInfo& gameInfo );	
	void					OnWorldStart				( const CGameInfo& gameInfo );
	void					OnWorldEnd					( const CGameInfo& gameInfo );
	
	void					Tick						( Float timeDelta );
	void					OnGenerateDebugFragments	( CRenderFrame* frame );

	// Script Interface
	void					funcSetActive				( CScriptStackFrame& stack, void* result );

	// IDialogHud
	virtual void			PlayDialogLine				( CIDInterlocutorComponent* interlocutor, const SIDBaseLine& line );
	virtual void			EndDialogLine				( CIDInterlocutorComponent* interlocutor );
	virtual void			ClearDialogLines			( );

	virtual void			SetMode						( EDialogDisplayMode comunicatorMode );
	virtual void			DialogLineDisplayClear		( );
	virtual void			OnFocusedTopicInterrupted	( );

	virtual void			OnInterrupted				( );
	virtual void			OnDialogEnded				( );

	virtual void			AddChoice					( const SIDOption& choice );
	virtual void			RemoveChoice				( const SIDOption& choice );
	virtual void			RemoveAllChoices			( );

	// IInputListener
	virtual Bool			OnGameInputEvent			( const SInputAction & action );

protected:
	Bool					RemoveChoiceInternal		( EHudChoicePosition position );
	virtual void			UpdateInput					( );
	virtual void			OnChoiceSelected			( EHudChoicePosition pos ) const;
	SPlayingLine&			GetLineByInterlocutor		( CIDInterlocutorComponent* speaker );
	void					DeleteLineByInterlocutor	( CIDInterlocutorComponent* speaker );

private:
	Bool					InitHudModule				( );
};


BEGIN_CLASS_RTTI( CR6DialogDisplay )
	PARENT_CLASS( IGameSystem )	
	PROPERTY_NAME( m_active, TXT("i_ActiveB") );	
	NATIVE_FUNCTION( "I_SetActive", funcSetActive )	;
END_CLASS_RTTI()