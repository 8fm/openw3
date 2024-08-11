#pragma once

#include "..\..\common\game\minigame.h"

class CFistfightMinigameInstanceData : public CMinigameInstanceData
{
public:
	TDynArray< CName >							m_activeActors;
	TDynArray< THandle< CNewNPC > >				m_enemies;
	TDynArray< THandle< CDeniedAreaSaveable > >	m_area;
	EMinigameState								m_requestedGameEnd;
	EngineTime									m_gameEndRequestTime;
	Bool										m_wasFadeCalled;

public:
	CFistfightMinigameInstanceData();

	void RequestEndGame( EMinigameState state );
	Bool HasGameEndRequest() const;
	Bool WaitForGameEnd();
	Bool WaitForFadeStart();
	void DoFade();
	void DoBlacksceen();
};

struct CFistfightOpponent
{
	DECLARE_RTTI_SIMPLE_CLASS( CFistfightOpponent )
	
	CName	m_npcTag;
	CName	m_startingPosTag;
};

BEGIN_CLASS_RTTI( CFistfightOpponent )
	PROPERTY_EDIT( m_npcTag, TXT("Tag of an opponent npc ") )
	PROPERTY_EDIT( m_startingPosTag, TXT("Tag of trajectory this enemy will start in ") )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CFistfightMinigame : public CMinigame
{
	DECLARE_ENGINE_CLASS( CFistfightMinigame, CMinigame, 0 );

	TDynArray< CFistfightOpponent > m_enemies;

	CName	m_fightAreaTag;
	CName	m_playerPosTag;
	Bool	m_toTheDeath;
	Bool	m_endsWithBlackscreen;

public:
	static const Float FADE_DURATION;
	static const Float END_GAME_DELAY;
	static const Float TIME_OFFSET;

public:
	CFistfightMinigame();

	void PrepareArea( CFistfightMinigameInstanceData* data ) const;
	void ReleaseArea( CFistfightMinigameInstanceData* data ) const;

	CActor* AddActor( CFistfightMinigameInstanceData* data, CName actorTag, CName startPos  ) const;
	void RemoveActor( CFistfightMinigameInstanceData* data, CActor* actor ) const;

	virtual void OnStartGame( CMinigameInstanceData* _data ) const override;
	virtual void OnEndGame( CMinigameInstanceData* _data ) const override;
	virtual Bool OnIsFinished( CMinigameInstanceData* _data ) const override;

	virtual CMinigameInstanceData* InitInstanceData() const override
	{
		return new CFistfightMinigameInstanceData();
	}
};

BEGIN_CLASS_RTTI( CFistfightMinigame );
	PARENT_CLASS( CMinigame );
	PROPERTY_EDIT( m_fightAreaTag, TXT("Tag of denied area components that close the fight area") )
	PROPERTY_EDIT( m_playerPosTag, TXT("Tag of trajectory player will start in") )
	PROPERTY_EDIT( m_toTheDeath, TXT("") )
	PROPERTY_EDIT( m_endsWithBlackscreen, TXT("") )
	PROPERTY_EDIT( m_enemies, TXT("Oponents definitions") )
END_CLASS_RTTI();
