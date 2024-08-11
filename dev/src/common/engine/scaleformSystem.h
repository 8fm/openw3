/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#ifdef USE_SCALEFORM

#include "../redSystem/utility.h"
#include "flashFontConfig.h"
#include "scaleformLoader.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class IScaleformPlayer;

//////////////////////////////////////////////////////////////////////////
// CScaleformSystem
//////////////////////////////////////////////////////////////////////////
class CScaleformSystem : private Red::System::NonCopyable
{
private:
	typedef Red::Threads::CMutex CMutex;
	typedef Red::Threads::CScopedLock< CMutex > CScopedLock;

private:
	static CScaleformSystem*			s_instance;
	TDynArray< IScaleformPlayer* >		m_players;
	SF::Ptr< CScaleformLoader >			m_systemLoader;

private:
	CFlashFontConfig					m_fontConfig;
	Bool								m_isInitialized;

private:
	CScaleformSystem();
	~CScaleformSystem();

public:
	Bool Init( IViewport* viewport );
	void Shutdown();
	void Destroy();

public:
	static CScaleformSystem* StaticInstance();

public:
	Bool RegisterScaleformPlayer( IScaleformPlayer* player );
	Bool UnregisterScaleformPlayer( IScaleformPlayer* player );

public:
	SF::Ptr< CScaleformLoader > GetLoader() const { return m_systemLoader; }

public:
	void OnLanguageChange();
};

#endif // USE_SCALEFORM;