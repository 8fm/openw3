#include "build.h"

#include "actorSpeechQueue.h"

struct CActorSpeechDataWrapper
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Gameplay );

	Uint32							m_speechId;
	StringAnsi						m_soundEventName;
	Bool							m_sync;
	Int32							m_modeFlags;
	Float							m_progress;
	IStorySceneDisplayInterface*	m_sceneDisplay;

	CActorSpeechDataWrapper*		m_next;

	CActorSpeechDataWrapper( Uint32 speechId, const StringAnsi& soundEventName, Bool sync, Int32 modeFlags );	

	void Init( ActorSpeechData& speechData ){ speechData.Init( m_speechId, m_soundEventName, m_sync, m_modeFlags ); }

};

CActorSpeechDataWrapper::CActorSpeechDataWrapper( Uint32 speechId, const StringAnsi& soundEventName, Bool sync, Int32 modeFlags )
	: m_speechId( speechId )
	, m_soundEventName( soundEventName )
	, m_sync( sync )
	, m_modeFlags( modeFlags )
	, m_progress( 0.f )
	, m_sceneDisplay( nullptr )
	, m_next( nullptr )
{}

CActorSpeechQueue::CActorSpeechQueue()
	: m_queueHead( nullptr )	
{

}

CActorSpeechQueue::~CActorSpeechQueue()
{
	Cleanup();
}

void  CActorSpeechQueue::AddLine( Uint32 speechId, const StringAnsi& soundEventName, Bool sync, Int32 modeFlags )
{
	CActorSpeechDataWrapper* newLine = new CActorSpeechDataWrapper( speechId, soundEventName, sync, modeFlags );
	newLine->m_next = m_queueHead;
	m_queueHead = newLine;
}

void CActorSpeechQueue::NextLineData( ActorSpeechData& data )
{	
	if( m_queueHead )
	{
		m_queueHead->Init( data );
		CActorSpeechDataWrapper* prevHead = m_queueHead;
		m_queueHead = m_queueHead->m_next;
		delete prevHead;
	}	
}

void CActorSpeechQueue::Cleanup()
{
	ActorSpeechData data( 0, StringAnsi::EMPTY, false, 0 );
	while( m_queueHead )
	{
		CActorSpeechDataWrapper* prevHead = m_queueHead;
		m_queueHead = m_queueHead->m_next;
		delete prevHead;
	}
}