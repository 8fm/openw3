/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once


///////////////////////////////////////////////////////////////////////////////
class CStoryPhaseListener : public Red::System::NonCopyable
{
public:
	CStoryPhaseListener( TDynArray< THandle< CCommunity > >& spawnsets );
	virtual ~CStoryPhaseListener();

protected:
	const TDynArray< THandle< CCommunity > >& GetAttachedSpawnsets() { return m_spawnsets; }

private:
	TDynArray< THandle< CCommunity > >&		m_spawnsets;
};
///////////////////////////////////////////////////////////////////////////////
