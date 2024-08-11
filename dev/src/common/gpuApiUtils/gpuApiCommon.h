/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

// Runtime error handling
#if !defined( RED_FINAL_BUILD ) && !defined( LTCG ) && !defined( NO_GPU_ASSERTS )
#define GPUAPI_MUST_SUCCEED( expression )		do { HRESULT hRet = (expression); if ( !SUCCEEDED(hRet) ) { GPUAPI_HALT(  "Failed: " MACRO_TXT(#expression) ); } } while( 0 )
#else
	#define GPUAPI_MUST_SUCCEED( expression )		expression
#endif

