
#pragma once

#include "expManager.h"

class ExpPlayer;

class ExpTraverser
{
	CEntity*	m_owner;
	ExpPlayer*	m_player;

	CName		m_inputHor;
	CName		m_inputVer;

	CName		m_inputX;
	CName		m_inputY;
	CName		m_inputA;
	CName		m_inputB;

public:
	ExpTraverser( const SExplorationQueryToken& token, ExpManager* dir, CEntity* ent );
	~ExpTraverser();

	void Update( Float dt );

	Bool IsRunning() const;

	void Release();

public:
	void GenerateDebugFragments( CRenderFrame* frame );
};
