/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_IO_COMMON_H
#define RED_IO_COMMON_H
#pragma once

#include "redIOPlatform.h"

REDIO_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
// Globals.
//////////////////////////////////////////////////////////////////////////
// Don't increase these unless you know the implementation details. The
// underlying OS has certain resource limits which these values were
// chosen for.
const Uint32 REDIO_MAX_PATH_LENGTH	= 256;	
const Uint32 REDIO_MAX_ASYNC_OPS	= 64;
const Uint32 REDIO_MAX_FILE_HANDLES	= 192;
const Uint32 REDIO_MAX_DIR_HANDLES	= 256;

//////////////////////////////////////////////////////////////////////////
// EOpenFlag
//////////////////////////////////////////////////////////////////////////
enum EOpenFlag
{
	eOpenFlag_Read					= FLAG(0),
	eOpenFlag_Write					= FLAG(1),
	eOpenFlag_Append				= FLAG(2),
	eOpenFlag_Create				= FLAG(3),
	eOpenFlag_Truncate				= FLAG(4),
	eOpenFlag_Async					= FLAG(5),
	eOpenFlag_ReadWrite				= eOpenFlag_Read | eOpenFlag_Write,
	eOpenFlag_ReadWriteNew			= eOpenFlag_ReadWrite | eOpenFlag_Create | eOpenFlag_Truncate,
	eOpenFlag_WriteNew				= eOpenFlag_Write | eOpenFlag_Create | eOpenFlag_Truncate,
};

//////////////////////////////////////////////////////////////////////////
// EAsyncFlag
//////////////////////////////////////////////////////////////////////////
enum EAsyncFlag
{
	eAsyncFlag_None							= 0,
	eAsyncFlag_TryCloseFileWhenNotUsed		= FLAG(0),
};

//////////////////////////////////////////////////////////////////////////
// EAsyncPriority
//////////////////////////////////////////////////////////////////////////
enum EAsyncPriority
{
	eAsyncPriority_Critical,
	eAsyncPriority_VeryHigh,
	eAsyncPriority_High,
	eAsyncPriority_Normal,
	eAsyncPriority_Low,
	eAsyncPriority_Count,
};

//////////////////////////////////////////////////////////////////////////
// ECallbackResult
//////////////////////////////////////////////////////////////////////////
enum ECallbackRequest
{
	eCallbackRequest_Finish,
	eCallbackRequest_More,
	eCallbackRequest_Defer,
};

//////////////////////////////////////////////////////////////////////////
// EAsyncResult
//////////////////////////////////////////////////////////////////////////
enum EAsyncResult
{			
	eAsyncResult_Canceled				= -2, //!< Operation was canceled
	eAsyncResult_Error					= -1, //!< Error performing async operation
	eAsyncResult_Success				=  0, //!< Completed async operation successfully
	eAsyncResult_Pending				=  1, //!< Pending result for async operation
	eAsyncResult_Undeferred				=  2, //!< Trying again after a deferral

};

//////////////////////////////////////////////////////////////////////////
// ESeekOrigin
//////////////////////////////////////////////////////////////////////////
enum ESeekOrigin
{
	eSeekOrigin_Set			= 0,
	eSeekOrigin_Current		= 1,
	eSeekOrigin_End			= 2,
};

REDIO_NAMESPACE_END

#endif // RED_IO_COMMON_H