#pragma once

#include "..\..\common\game\minigame.h"
#include "r4GwintManager.h"

enum EGwintDifficultyMode
{
	EGDM_Easy = 0,
	EGDM_Medium,
	EGDM_Hard,
};

BEGIN_ENUM_RTTI( EGwintDifficultyMode );
	ENUM_OPTION( EGDM_Easy );
	ENUM_OPTION( EGDM_Medium );
	ENUM_OPTION( EGDM_Hard );
END_ENUM_RTTI();

enum EGwintAggressionMode
{
	EGAM_Defensive = 0,
	EGAM_Normal,
	EGAM_Aggressive,
	EGAM_VeryAggressive,
	EGAM_AllIHave,
};

BEGIN_ENUM_RTTI( EGwintAggressionMode );
	ENUM_OPTION( EGAM_Defensive );
	ENUM_OPTION( EGAM_Normal );
	ENUM_OPTION( EGAM_Aggressive );
	ENUM_OPTION( EGAM_VeryAggressive );
	ENUM_OPTION( EGAM_AllIHave );
END_ENUM_RTTI();

class CGwintMenuInitData : public CObject
{
	DECLARE_ENGINE_CLASS( CGwintMenuInitData, CObject, 0 );

private:
	CName							m_deckName;
	EGwintDifficultyMode			m_difficulty;
	EGwintAggressionMode			m_aggression;
	Bool							m_allowMultipleMatches;
	eGwintFaction					m_forceFaction;

public:
	CGwintMenuInitData();
	void SetParams( const CName& deckName, EGwintDifficultyMode difficulty, EGwintAggressionMode aggression, Bool allowMultipleMatches );
};

BEGIN_CLASS_RTTI( CGwintMenuInitData );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_deckName, TXT("Name of a deck for AI player") )
	PROPERTY_EDIT( m_difficulty, TXT("Difficulty of AI player") )
	PROPERTY_EDIT( m_aggression, TXT("Aggression of AI player") )
	PROPERTY_EDIT( m_allowMultipleMatches, TXT("Allow multiple matches") )
	PROPERTY_EDIT( m_forceFaction, TXT("Force faction (neutral means no force)"))
END_CLASS_RTTI();

class CGwintMinigameInstanceData : public CMinigameInstanceData
{
public:
	CGwintMinigameInstanceData();

	void DisableGameSaving();
	void EnableGameSaving();

private:
	CGameSaveLock					m_saveLock;
};

class CGwintMinigame : public CMinigame
{
	DECLARE_ENGINE_CLASS( CGwintMinigame, CMinigame, 0 );
private:
	CName							m_deckName;
	EGwintDifficultyMode			m_difficulty;
	EGwintAggressionMode			m_aggression;
	Bool							m_allowMultipleMatches;	
	eGwintFaction					m_forceFaction;

public:
	CGwintMinigame();
	~CGwintMinigame();

	virtual void OnStartGame( CMinigameInstanceData* _data ) const override;
	virtual void OnEndGame( CMinigameInstanceData* _data ) const override;
	virtual Bool OnIsFinished( CMinigameInstanceData* _data ) const override;

	virtual CMinigameInstanceData* InitInstanceData() const override
	{
		return new CGwintMinigameInstanceData();
	}
};

BEGIN_CLASS_RTTI( CGwintMinigame );
	PARENT_CLASS( CMinigame );
	PROPERTY_EDIT( m_deckName, TXT("Name of a deck for AI player") )
	PROPERTY_EDIT( m_difficulty, TXT("Difficulty of AI player") )
	PROPERTY_EDIT( m_aggression, TXT("Aggression of AI player") )
	PROPERTY_EDIT( m_allowMultipleMatches, TXT("Allow multiple matches") )
	PROPERTY_EDIT( m_forceFaction, TXT("Force faction (neutral means no force)"))
END_CLASS_RTTI();
