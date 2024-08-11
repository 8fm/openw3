#include "build.h"
#include "idLine.h"
#include "idConnector.h"
#include "../../common/game/actorSpeech.h"

// This is here ONLY for backwards compatibility reasons. 
// SIDLine is deprecated and will be removed after content resave. PLEASE DO NOT USE.
#if VER_IDLINES_REFACTOR > VER_MINIMAL
	IMPLEMENT_ENGINE_CLASS( SIDLine )
#endif

IMPLEMENT_ENGINE_CLASS( SIDBaseLine )
IMPLEMENT_ENGINE_CLASS( SIDTextLine )

#define ACTOR_SPEECH_DATA_CONSTRUCTOR_FOR_LINE_INSTANCE ActorSpeechData						\
( 																							\
	line.m_text.GetIndex(),																	\
	StringAnsi::EMPTY,																		\
	false,																					\
	( GGame->GetGameplayConfig().m_idUseNewVoicePipeline ? ASM_DialogVoice : ASM_Voice )	\
	| ( speaker->HasSceneMimic() ? ASM_Lipsync : 0 )											\
)

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
SIDLineInstance::SIDLineInstance( ISceneActorInterface* speaker, Bool active, Uint32 dialogInstance, const SIDTextLine& line )
	: m_active( active )
	, m_dialogInstance( dialogInstance )
	, m_line( line )
	, m_speaker( speaker )
	, m_receiver( line.m_receiver )
	, m_timeToDie( 0.f )
	, m_interruptReceiver( line.m_interruptReceiver )
	, m_speech(	speaker, ACTOR_SPEECH_DATA_CONSTRUCTOR_FOR_LINE_INSTANCE )
{
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
SIDLineInstance::SIDLineInstance( ISceneActorInterface* speaker, Bool active, Uint32 dialogInstance, const SIDConnectorLine& line )
	: m_active( active )
	, m_dialogInstance( dialogInstance )
	, m_line( line )
	, m_speaker( speaker )
	, m_receiver( CNAME( Default ) )
	, m_timeToDie( 0.f )
	, m_interruptReceiver( false )
	, m_speech(	speaker, ACTOR_SPEECH_DATA_CONSTRUCTOR_FOR_LINE_INSTANCE )
{
}


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
SIDLineInstance::~SIDLineInstance()
{
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void SIDLineInstance::Update( Float timeDelta )
{
	if ( m_active )
	{
		m_speech.Update( timeDelta );
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void SIDLineInstance::StopPlaying()
{
	RED_LOG( Dialog, TXT("Stopping dialog voice of %s, text: %s"), m_speaker->GetSceneActorVoiceTag().AsString().AsChar(), m_line.m_text.GetString().AsChar() );
	m_speech.Cancel();
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
Bool SIDLineInstance::UpdateNeedsToDie( Float timeDelta )
{
	if ( m_timeToDie <= 0.0f )
	{
		return false;
	}

	m_timeToDie -= timeDelta;
	return ( m_timeToDie <= 0.0f );
}

//------------------------------------------------------------------------------------------------------------------
// this is needed to prevent GC from deleting lipsync animation from actorspeech
//------------------------------------------------------------------------------------------------------------------
IFile& operator<<( IFile &file, SIDLineInstance &line )
{
	file << line.m_speech;

	return file;
}