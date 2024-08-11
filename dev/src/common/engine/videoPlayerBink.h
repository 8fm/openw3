/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "videoPlayer.h"

#ifdef USE_BINK_VIDEO
# include "../../../external/Bink2SDKWindows/include/bink.h"
# ifdef _WIN64
#  pragma comment( lib, "../../../external/Bink2SDKWindows/lib/bink2w64.lib" )
# else
#  pragma comment( lib, "../../../external/Bink2SDKWindows/lib/bink2w32.lib" )
# endif

struct SVideoParamsBink : public SVideoParams
{
	//TDynArray< Uint32 >		m_audioTracks;

	SVideoParamsBink( const SVideoParams& videoParams );
};

class CVideoPlayerBink : public CRenderVideoPlayer
{
private:
	typedef CRenderVideoPlayer Base;
	
public:
	CVideoPlayerBink();
	~CVideoPlayerBink();

public:
	virtual void StartVideo( const SVideoParams& videoParams );
};

#endif // USE_BINK_VIDEO
