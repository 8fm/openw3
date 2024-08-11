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


/*	----------------------------------------------------------------------------------------
	PS3 implementation.
	Location: \Wwise\Communication\Remote\PS3
	Header location: \Wwise\Communication\Remote\CodeBase
	-----------------------------------------------------------------------------------------
*/


#include "Network.h"

#include <net.h>
#include <libnetctl.h>
#include <string.h>

#include "NetworkTypes.h"

namespace Network
{
	int g_NetMemId = 0;
	AKRESULT Init( AkMemPoolId in_pool, bool /*in_bInitSystemLib*/ )
	{
		//SceNetInitParam param;
		static char memory[16 * 1024];
		int ret;

		/*E libnet and libnetctl load */
		/*ret = sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
		if (ret < 0) {
			goto fail10;
		}*/

		/*E libnet */
		g_NetMemId = sceNetPoolCreate("AkNet", sizeof(memory), 0);
		//param.memory = memory;
		//param.size = sizeof(memory);
		//param.flags = 0;
		ret = sceNetInit(/*balary todo*/);		
		if (ret == SCE_NET_ERROR_EBUSY)
		{	
			//Already initialized.
			return AK_PartialSuccess;
		}

		if (ret < 0) {
			goto fail20;
		}

		/*E libnetctl */
		ret = sceNetCtlInit();
		if (ret < 0) {
			goto fail30;
		}

		return AK_Success;

	fail30:
		sceNetTerm();
	fail20:
		//sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
	fail10:
		return AK_Fail;
	}

	void Term(bool in_bTermSystemLib)
	{
		if (in_bTermSystemLib)
		{
			sceNetCtlTerm();

			/*E libnet */
			sceNetTerm();

			if (g_NetMemId)
			{
				sceNetPoolDestroy(g_NetMemId);
			}
		}
	}

	AkInt32 GetLastError()
	{
		return sce_net_errno;
	}

	void GetMachineName( char* out_pMachineName, AkInt32* io_pStringSize )
	{
		static char* pszUnnamed = { "Unnamed" };

		strncpy( out_pMachineName, pszUnnamed, *io_pStringSize );
		out_pMachineName[*io_pStringSize-1] = 0;
		*io_pStringSize = strlen( pszUnnamed );
	}

	bool SameEndianAsNetwork()
	{
		return sceNetHtons( 12345 ) == 12345;
	}
}
