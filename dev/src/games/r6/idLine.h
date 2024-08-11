#pragma once

#include "build.h"


//------------------------------------------------------------------------------------------------------------------
// Dialog line
//------------------------------------------------------------------------------------------------------------------
#ifndef NO_EDITOR
	struct SIDLineStub
	{
		CName	m_speaker;
		CName	m_receiver;
		String	m_text;
		String	m_comment;

		RED_INLINE Bool MatchesLine( const SIDTextLine& line ) const;
	};
#endif

struct SIDBaseLine
{
	DECLARE_RTTI_STRUCT( SIDBaseLine );

	LocalizedString	m_text;
#ifndef NO_EDITOR
	String			m_exampleVoiceoverFile;
#endif

protected:	// constructor is protected to prevent constructing base struct
	SIDBaseLine()
		: m_text()
	{
	}

	// Don't add new constructors.
};

BEGIN_NODEFAULT_CLASS_RTTI( SIDBaseLine );
END_CLASS_RTTI();

struct SIDTextLine : public SIDBaseLine
{
	DECLARE_RTTI_STRUCT( SIDTextLine );

	#ifndef NO_EDITOR
		String		m_comment;
	#endif
	CName			m_speaker;
	CName			m_receiver;
	Bool			m_interruptReceiver;


	RED_INLINE Bool operator==( const SIDTextLine& line ) const
	{
		return ( line.m_speaker == m_speaker ) && ( line.m_receiver == m_receiver ) && ( line.m_text == m_text );
	}

	SIDTextLine()
		: m_speaker( CName::NONE )
		, m_receiver( CName::NONE )	
		, m_interruptReceiver( false )
	{
	}

	// Don't add new constructors.

	#ifndef NO_EDITOR
		RED_INLINE SIDTextLine& operator=( const SIDLineStub& other )
		{
			R6_ASSERT( GIsEditor );
			m_text.SetString( other.m_text );
			m_comment = other.m_comment;
			m_speaker = other.m_speaker;
			m_receiver = other.m_receiver;
			return *this;
		}
	#endif
};

BEGIN_NODEFAULT_CLASS_RTTI( SIDTextLine )
	PROPERTY_CUSTOM_EDIT( m_text, TXT("Line content"), TXT("LocalizedStringEditor") );
#ifndef NO_EDITOR
	PROPERTY_RO_NOT_COOKED( m_exampleVoiceoverFile, TXT("") );
	PROPERTY_EDIT_NOT_COOKED( m_comment, TXT("") );
#endif
	PROPERTY_CUSTOM_EDIT( m_speaker, TXT("InterlocutorID that speaks the line"), TXT("InterlocutorIDList") )
	PROPERTY_CUSTOM_EDIT( m_receiver, TXT("InterlocutorID of whom the line is adressed"), TXT("InterlocutorIDList") )
	PROPERTY_EDIT( m_interruptReceiver, TXT("If true, it will try to interrupt the receiver so he stops the line in a short time") )
END_CLASS_RTTI()

#ifndef NO_EDITOR
	RED_INLINE Bool SIDLineStub::MatchesLine( const SIDTextLine& line ) const 
	{ 
		return line.m_speaker == m_speaker && line.m_receiver == m_receiver && line.m_text.GetString() == m_text && line.m_comment == m_comment; 
	} 
#endif

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
enum EIDLineState
{
	DILS_Queued					,
	DILS_Playing				,
	DILS_Completed				,
	DILS_CancelledOrNotPlayed	
};

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class SIDLineInstance : public Red::System::NonCopyable
{
private:
	Bool					m_active;
	Uint32					m_dialogInstance;
	const SIDBaseLine&		m_line;
	ISceneActorInterface*	m_speaker;
	CName					m_receiver;
	Float					m_timeToDie;
	CActorSpeech			m_speech;	

	Bool					m_interruptReceiver	: 1;

public:
	SIDLineInstance( ISceneActorInterface* speaker, Bool active, Uint32 dialogInstance, const SIDTextLine& line );
	SIDLineInstance( ISceneActorInterface* speaker, Bool active, Uint32 dialogInstance, const SIDConnectorLine& line );
	~SIDLineInstance();

	void Update( Float timeDelta );
	void StopPlaying();

	friend IFile& operator<<( IFile &file, SIDLineInstance &voicesetPlayer );

	RED_INLINE Bool LineMatches ( const SIDBaseLine& line, Uint32 dialogInstance ) const
	{
		return ( dialogInstance == m_dialogInstance ) && ( &line == &m_line );
	}

	RED_INLINE void				SetActive	( Bool active )			{ m_active	= active;				}
	RED_INLINE Bool				IsActive	( )				const	{ return m_active;					}
	RED_INLINE Uint32				GetDialogId	( )				const	{ return m_dialogInstance;			}
	RED_INLINE const SIDBaseLine&	GetLine		( )				const	{ return m_line;					}
	RED_INLINE Float				GetDuration	( )				const 	{ return m_speech.GetDuration();	}
	RED_INLINE Float				GetProgress	( )				const 	{ return m_speech.GetProgress();	}
	RED_INLINE void				SetTimeToDie( Float time )		 	{ m_timeToDie	= time;				}
	RED_INLINE Bool				ShouldInterruptReceiver()	const	{ return m_interruptReceiver;		}
	RED_INLINE CName				GetReceiver ( )				const	{ return m_receiver;				}

	//! Updates the time to die and returns true if it has to die
	Bool	UpdateNeedsToDie( Float timeDelta );
};

//------------------------------------------------------------------------------------------------------------------
// This is here ONLY for backwards compatibility reasons. 
// SIDLine is deprecated and will be removed after content resave. PLEASE DO NOT USE.
//------------------------------------------------------------------------------------------------------------------
#if VER_IDLINES_REFACTOR > VER_MINIMAL
	struct SIDLine
	{
		DECLARE_RTTI_STRUCT( SIDLine );

		CName			m_speaker;
		CName			m_receiver;
		Bool			m_interruptReceiver;
		LocalizedString	m_text;
	#ifndef NO_EDITOR
		String			m_voiceoverFile;
	#endif

		SIDLine()
			: m_speaker( CName::NONE )
			, m_receiver( CName::NONE )	
			, m_interruptReceiver( false )
			, m_text()
		{
		}
	};

	BEGIN_NODEFAULT_CLASS_RTTI( SIDLine )
		PROPERTY( m_speaker )
		PROPERTY( m_receiver )
		PROPERTY( m_interruptReceiver )
		PROPERTY( m_text )
#ifndef NO_EDITOR
		PROPERTY( m_voiceoverFile )
#endif

	END_CLASS_RTTI()
#endif

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
