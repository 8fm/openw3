/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

#pragma once

#include <ajm.h>
#include <AK/Tools/Common/AkListBare.h>

class CAkSrcBankAt9;
class CAkSrcFileAt9;

class CAkACPManager
{
public:
	CAkACPManager();
	~CAkACPManager();

	AKRESULT Init();
	void Term();
	
	AKRESULT Register(CAkSrcBankAt9* pACPSrc);
	AKRESULT Register(CAkSrcFileAt9* pACPSrc);
	AKRESULT Unregister(CAkSrcBankAt9* pACPSrc);
	AKRESULT Unregister(CAkSrcFileAt9* pACPSrc);

	AKRESULT Update();

private:
	SceAjmContextId m_AjmContextId;
	int m_SoundsToInitialize;
	
	AkListBare<CAkSrcBankAt9, AkListBareNextItem<CAkSrcBankAt9>, AkCountPolicyWithCount> m_ACPBankSrc;
	AkListBare<CAkSrcFileAt9, AkListBareNextItem<CAkSrcFileAt9>, AkCountPolicyWithCount> m_ACPFileSrc;
	uint8_t* m_pAcpBatchBuffer;
};