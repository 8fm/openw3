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

//////////////////////////////////////////////////////////////////////
//
// AkEffectsMgr.h
//
// Windows implementation of the effects manager.
// The effects manager takes care of allocating, deallocating and 
// storing the effect instances. 
// The initial effect parameters are kept at the node level. They are
// modified in run-time through RTPC, but the renderer sees them as
// anonymous blocks of data.
// The effects manager lives as a singleton in the AudioLib and supports
// multithreading.
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_EFFECTS_MGR_H_
#define _AK_EFFECTS_MGR_H_

#include <AK/SoundEngine/Common/IAkPlugin.h>

#include "AkKeyArray.h"
#include <AK/MotionEngine/Common/IAkMotionMixBus.h>
class CAkPBI;
class IAkSoftwareCodec;

//-----------------------------------------------------------------------------
// Name: Class CAkEffectsMgr
// Desc: Implementation of the effects manager.
//-----------------------------------------------------------------------------
class CAkEffectsMgr
{
public:

    // Effect record definitions.
    //-------------------------------------
    //
    struct EffectTypeRecord
    {
        AkCreatePluginCallback pCreateFunc;           // Plugin creation function address.
        AkCreateParamCallback pCreateParamFunc;       // Plugin parameter object creation function address.
    };

	struct CodecTypeRecord
    {
        AkCreateFileSourceCallback pFileCreateFunc;       // File source.
        AkCreateBankSourceCallback pBankCreateFunc;       // Bank source.
    };

    // Initialise/Terminate.
    static AKRESULT Init();
    static AKRESULT Term();

    // Registration-time.
    // Source plug-ins plus effect plug-ins
    static AKRESULT RegisterPlugin( 
		AkPluginType in_eType,
		AkUInt32 in_ulCompanyID,                    // Company identifier (as declared in plugin description XML)
		AkUInt32 in_ulPluginID,                     // Plugin identifier (as declared in plugin description XML)
        AkCreatePluginCallback in_pCreateFunc,       // Pointer to the effect's Create() function.
        AkCreateParamCallback in_pCreateParamFunc    // Pointer to the effect param's Create() function.
		);
	
    // Codec plug-ins
    static AKRESULT RegisterCodec( 
		AkUInt32 in_ulCompanyID,						// Company identifier (as declared in XML)
		AkUInt32 in_ulPluginID,							// Plugin identifier (as declared in XML)
        AkCreateFileSourceCallback in_pFileCreateFunc,  // File source.
        AkCreateBankSourceCallback in_pBankCreateFunc   // Bank source.
		);

    //
    // Run-time API.
    // 
    // Returns a pointer to the instance of an effect given the ID. Allocates it if needed.
    static AKRESULT AllocParams( 
		AK::IAkPluginMemAlloc * in_pAllocator,
		AkPluginID in_EffectTypeID,				// Effect type ID.
        AK::IAkPluginParam * & out_pEffectParam	// Effect param instance.
        );

    // Creates an effect instance given the FX ID. 
    static AKRESULT Alloc( 
		AK::IAkPluginMemAlloc * in_pAllocator,
		AkPluginID in_EffectTypeID,             // Effect type ID.
        AK::IAkPlugin* & out_pEffect            // Effect instance.
        );

	// Creates a codec (bank or file) instance given the Codec ID. 
    static IAkSoftwareCodec * AllocCodec( 
		CAkPBI * in_pCtx,						// Source context.
		AkUInt32 in_uSrcType,
		AkCodecID in_CodecID					// Codec type ID.
        );

#ifdef AK_MOTION
	static AKRESULT RegisterFeedbackBus(
		AkUInt32 in_ulCompanyID,
		AkUInt32 in_iBusPluginID,						// Plugin identifier (as declared in XML)
		AkCreatePluginCallback in_pCreateMixNodeFunc);		// Object creation callback

	static AKRESULT AllocFeedbackBus(
		AkUInt32 in_ulCompanyID,
		AkUInt32 in_iPlugin,							// Bus plugin ID
		AkPlatformInitSettings * io_pPDSettings,
		AkUInt8 in_iPlayer, 							// The player ID
		IAkMotionMixBus*& out_pMixNode, 				// Output bus pointer
		void * pDevice = NULL);
#endif // AK_MOTION

    inline static AkUInt32 GetMergedID( AkUInt32 in_type, AkUInt32 in_companyID, AkUInt32 in_PluginID )
    {
        return ( in_type & AkPluginTypeMask ) 
		    + ( in_companyID << 4 ) 
		    + ( in_PluginID << ( 4 + 12 ) );
    }

private:
    
    // Singleton functions.
    CAkEffectsMgr();                          // Constructor.
    // Destructor (made public to please the memory manager).

private:
    // Factory
	typedef CAkKeyArray<AkPluginID, EffectTypeRecord> FXList;
    static FXList m_RegisteredFXList;  // list keyed by the plugin unique type ID.

	typedef CAkKeyArray<AkCodecID, CodecTypeRecord> CodecList;
    static CodecList m_RegisteredCodecList;  // list keyed by the codec unique type ID.

#ifdef AK_MOTION
	typedef CAkKeyArray<AkPluginID, AkCreatePluginCallback> FeedbackBusList;
	static FeedbackBusList m_RegisteredFeedbackBus;
#endif // AK_MOTION
};

#endif //_AK_EFFECTS_MGR_H_
