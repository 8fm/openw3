#pragma once

#include "actorSpeech.h"

struct CActorSpeechDataWrapper;


class CActorSpeechQueue
{	
private:
	CActorSpeechDataWrapper*	m_queueHead;	

public:
	CActorSpeechQueue();
	~CActorSpeechQueue();
	
	void AddLine( Uint32 speechId, const StringAnsi& soundEventName, Bool sync, Int32 modeFlags );
	Bool HasNextLine( ) const { return m_queueHead != nullptr;}
	void NextLineData( ActorSpeechData& data );
	void Cleanup();
};