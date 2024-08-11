#include "build.h"

#include "MusicSystem.h"
#include "game.h"


void CMusicSystem::Reset( )
{
}

void CMusicSystem::Tick( Float delta )
{
	PC_SCOPE_PIX( CMusicSystemTick )

 	if( !m_enabled )
		return;


#ifndef RED_FINAL_BUILD
	Float gameTime = GGame->GetEngineTime();

	TDynArray<SMusicSyncDebugInfo> tempList;

	for (auto & debugInfo : m_syncInfos)
	{
		//Let the debug persest for 1/2 second after it's finished so we can see fast-triggerng events
		if(gameTime < debugInfo.nextEventTime - debugInfo.timeOffset + 0.5f)
		{
			tempList.PushBack(debugInfo);
		}
	}

	m_syncInfos.Clear();

	for(auto info : tempList)
	{
		m_syncInfos.PushBack(info);
	}
#endif

}

void CMusicSystem::NotifyBeat(Float duration)
{
	if(GGame)
	{
		Float gameTime = GGame->GetEngineTime();
		m_currentBeatDuration = duration;
		m_lastBeatTime = gameTime;
		m_beatsToNextBar--;
		m_beatsToNextGrid--;
	}
}

void CMusicSystem::NotifyBar(Uint32 beatsRemaining)
{
	if(GGame)
	{
		Float gameTime = GGame->GetEngineTime();
		//Because wwise sends the beat notification before the bar, we need to add one beat
		//to account for the subsequent -- in the NotifyBeat call
		m_beatsToNextBar = beatsRemaining + 1;
		m_lastBarTime = gameTime;
		m_currentBeatsPerBar = beatsRemaining;
	}
	
}

void CMusicSystem::NotifyGrid(Uint32 beatsRemaining)
{
	if(GGame)
	{
		Float gameTime = GGame->GetEngineTime();
		m_beatsToNextGrid = beatsRemaining; 
		m_lastGridTime = gameTime;
		m_currentBeatsPerGrid = beatsRemaining;
	}
}

static Bool AreClose(Float x, Float y, Float epsilon)
{
	return (x >= y && x - epsilon <= y) || (y >= x && y - epsilon <= x); 
}

Float CMusicSystem::GetTimeToNextValidEventBeforeTime(Uint32 typeFlags, Float timeLimit, Float timeOffset /*= 0.f*/)
{
	static const Float maxTimeVariance = 0.1f;

	if(!m_enabled )
		return 0.f;

	Float gameTime = GGame->GetEngineTime() - maxTimeVariance;
	Float soonestTime = 0.f;

	switch (typeFlags)
	{
	case EMusicResponseEventType::Bar:
		soonestTime = GetNextBarTime(gameTime, timeOffset);
		break;
	case EMusicResponseEventType::Beat:
		soonestTime = GetNextBeatTime(gameTime, timeOffset);
		break;
	case EMusicResponseEventType::Grid:
		soonestTime = GetNextGridTime(gameTime, timeOffset);
		break;
	default:
		break;
	}

	if(soonestTime > timeLimit)
	{
		soonestTime = 0.f;
	}

	return soonestTime;
}

void CMusicSystem::GenerateEditorFragments(CRenderFrame* frame)
{
#ifndef RED_FINAL_BUILD

	if(!m_enableDebug)
	{
		return;
	}

	Float gameTime = GGame->GetEngineTime();

	Uint32 ySize = 50;
	Uint32 xSize = 5;
	Uint32 yPos = 175;
	Uint32 xZero = 300; 
	Float timelineScale = 200.f;

	frame->AddDebugGradientRect(xZero, yPos, xSize, ySize*3, Color::RED, Color::WHITE);

	Float maxFutureTime = 7.f;

	Uint32 timelineSize = 5;

	Uint32 timelinePostition = xZero;

	Color timelineColour = Color::GRAY;

	while(timelinePostition < xZero + maxFutureTime*timelineScale)
	{
		frame->AddDebugRect(timelinePostition, yPos - timelineSize, (Uint32)timelineScale, timelineSize, timelineColour);
		timelinePostition += (Uint32)timelineScale;
		timelineColour = timelineColour == Color::GRAY ? Color::WHITE : Color::GRAY;
	}

	Float currentTime = m_lastBeatTime;
	Uint32 xPos;

	if(currentTime <= 0.f ||  currentTime + maxFutureTime < gameTime)
	{
		return;
	}

	//Add a small delta each time to move us onto the next beat/bar/grid
	while(currentTime < gameTime + maxFutureTime)
	{
		xPos = xZero + Uint32((currentTime - gameTime)*timelineScale);
		frame->AddDebugGradientRect(xPos, yPos, xSize, ySize, Color::BLUE, Color::WHITE);
		currentTime = GetNextBeatTime(currentTime) + 0.01f;
	}

	xPos = xZero + Uint32((currentTime - gameTime)*timelineScale);
	frame->AddDebugGradientRect(xPos, yPos, xSize, ySize, Color::BLUE, Color::WHITE);

	yPos += ySize;
	currentTime = m_lastBeatTime;
	
	xPos = xZero + Uint32((m_lastBarTime - gameTime)*timelineScale);
	frame->AddDebugGradientRect(xPos, yPos, xSize, ySize, Color::GREEN, Color::WHITE);

	while(currentTime < gameTime + maxFutureTime)
	{
		currentTime = GetNextBarTime(currentTime) +0.01f;
		xPos = xZero + Uint32((currentTime - gameTime)*timelineScale);
		frame->AddDebugGradientRect(xPos, yPos, xSize, ySize, Color::GREEN, Color::WHITE);
	}

	yPos += ySize;
	currentTime = m_lastBeatTime;

	xPos = xZero + Uint32((m_lastGridTime - gameTime)*timelineScale);
	frame->AddDebugGradientRect(xPos, yPos, xSize, ySize, Color::YELLOW, Color::WHITE);

	while(currentTime < gameTime + maxFutureTime)
	{
		currentTime = GetNextGridTime(currentTime) + 0.01f;
		xPos = xZero + Uint32((currentTime - gameTime)*timelineScale);
		frame->AddDebugGradientRect(xPos, yPos, xSize, ySize, Color::YELLOW, Color::WHITE);
	}

	for(auto &info : m_syncInfos)
	{
		yPos += ySize;
		Color col, col2;
		switch (info.type)
		{
		case EMusicResponseEventType::Beat:
			col = Color::BLUE;
			break;
		case EMusicResponseEventType::Bar:
			col = Color::GREEN;
			break;
		case EMusicResponseEventType::Grid:
			col = Color::YELLOW;
			break;
		default:
			break;
		}

		col2 = info.hasSynched ? Color::RED : Color::WHITE;

		//If we haven't received an condition check recently,  the AI has probably lost interest in this sync
		if(!info.hasSynched &&(gameTime - info.lastPollTime > 0.2f))
		{
			col = Color::GRAY;
		}

		xPos = xZero + Uint32((info.nextEventTime - gameTime)*timelineScale);
		Uint32 timeOffsetSize = Max((Uint32)(-1.f*info.timeOffset*timelineScale), xSize);
		frame->AddDebugGradientRect(xPos, yPos, timeOffsetSize, ySize/2, col, col2);
		frame->AddDebugScreenText(xPos, yPos + ySize/2, info.debugName.AsString());
		if(!info.eventName.Empty())
		{
			Uint32 nameOffset = xPos + info.debugName.AsString().Size() * 7;
			Uint32 eventNamePos = Max(xPos + timeOffsetSize, nameOffset) + 5;
			frame->AddDebugScreenText(eventNamePos , yPos + ySize/2, info.eventName);
		}
	}
#endif

}

Float CMusicSystem::GetNextBarTime(Float timeNow, Float timeOffset)
{
	Uint32 beatsRemaining = m_beatsToNextBar;
	Float currentTime = m_lastBeatTime + timeOffset;
	while(currentTime < timeNow || beatsRemaining != m_currentBeatsPerBar)
	{
		currentTime+= m_currentBeatDuration;
		if(beatsRemaining > 0)
		{
			beatsRemaining--;
		}
		if(beatsRemaining == 0)
		{
			beatsRemaining += m_currentBeatsPerBar;
		}
	}
	
	return currentTime;
}


Float CMusicSystem::GetNextGridTime(Float timeNow, Float timeOffset)
{
	Uint32 beatsRemaining = m_beatsToNextGrid;
	Float currentTime = m_lastBeatTime + timeOffset;
	while(currentTime < timeNow || beatsRemaining != m_currentBeatsPerGrid)
	{
		currentTime+= m_currentBeatDuration;
		if(beatsRemaining > 0)
		{
			beatsRemaining--;
		}
		if(beatsRemaining == 0)
		{
			beatsRemaining += m_currentBeatsPerGrid;
		}
	}

	return currentTime;
}

Float CMusicSystem::GetNextBeatTime(Float timeNow, Float timeOffset)
{
	Float currentTime = m_lastBeatTime + timeOffset;

	while(currentTime < timeNow)
	{
		currentTime+= m_currentBeatDuration;
	}

	return currentTime;
}

void CMusicSystem::EnableDebug(Bool enable)
{
#ifndef RED_FINAL_BUILD
	m_enableDebug = enable;
#endif
}

#ifndef RED_FINAL_BUILD
void CMusicSystem::RegisterPendingSync(SMusicSyncDebugInfo &info)
{
	for(auto &debugInfo : m_syncInfos)
	{
		if(debugInfo.uniqueId == info.uniqueId && !debugInfo.hasSynched)
		{
			debugInfo = info;
			debugInfo.lastPollTime = GGame->GetEngineTime();
			return;
		}
	}

	m_syncInfos.PushBack(info);
}

void CMusicSystem::RegisterSync(Uint64 uniqueId)
{
	for(auto &debugInfo : m_syncInfos)
	{
		if(debugInfo.uniqueId == uniqueId)
		{
			debugInfo.hasSynched = true;
			debugInfo.lastPollTime = GGame->GetEngineTime();
		}
	}
}


#endif
