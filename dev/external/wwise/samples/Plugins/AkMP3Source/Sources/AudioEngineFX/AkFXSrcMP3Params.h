//////////////////////////////////////////////////////////////////////
//
// AkFXSrcMP3Params.h
//
// Allows for audio source to come from an external input.
// 
// Note: Target output format is currently determined by the source itself.
// 
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKFXSRC_MP3PARAMS_H_
#define _AKFXSRC_MP3PARAMS_H_

#include <AK/Plugin/AkMP3SourceFactory.h>
#include <AK/Tools/Common/AkAssert.h>
#include <math.h>

const AkPluginParamID AK_SRCMP3_FXPARAM_TOKEN_ID	= 0;

struct AkFXSrcMP3Params
{
	AkUInt32 uToken;
};


//-----------------------------------------------------------------------------
// Name: class CAkFXSrcMP3Params
// Desc: Sample implementation the audio input source shared parameters.
//-----------------------------------------------------------------------------
class CAkFxSrcMP3Params : public AK::IAkPluginParam
{
public:

	// Allow effect to call accessor functions for retrieving parameter values.
	friend class CAkFXSrcMP3;

    // Constructor.
    CAkFxSrcMP3Params();
	
	// Copy constructor.
    CAkFxSrcMP3Params( const CAkFxSrcMP3Params & in_rCopy );

	// Destructor.
    ~CAkFxSrcMP3Params();

    // Create parameter object duplicate.
	AK::IAkPluginParam * Clone( AK::IAkPluginMemAlloc * in_pAllocator );

    // Initialization.
    AKRESULT Init( AK::IAkPluginMemAlloc *	in_pAllocator,		// Memory allocator.						    
                   const void *				in_pParamsBlock,	// Pointer to param block.
                   AkUInt32					in_ulBlockSize		// Sise of the parm block.
                   );
   
	// Termination.
	AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

    // Set all parameters at once.
    AKRESULT SetParamsBlock( const void * in_pParamsBlock, 
                             AkUInt32 in_ulBlockSize
                             );

    // Update one parameter.
    AKRESULT SetParam( AkPluginParamID in_ParamID,
                       const void * in_pValue, 
                       AkUInt32 in_ulParamSize
                       );

private:

    AkUInt32	GetToken();

private:

    // Parameter structure.
    AkFXSrcMP3Params m_Params;
};

// Safely retrieve input.
inline AkUInt32 CAkFxSrcMP3Params::GetToken()
{
	return m_Params.uToken;
}

#endif // _AKFXSRC_MP3PARAMS_H_
