/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "redIOPlatform.h"
#include "redIOCommon.h"

REDIO_NAMESPACE_BEGIN
//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
struct SAsyncReadToken;

/////////////////////////////////////////////////////////////////////////
// FAsyncOpCallback
//////////////////////////////////////////////////////////////////////////
typedef ECallbackRequest (*FAsyncOpCallback)( 
	SAsyncReadToken& asyncReadToken,	//!< The async token passed into BeginRead.
	EAsyncResult asyncResult,			//!< The async result for this operation.
	Uint32 numberOfBytesTransferred );	//!< Valid if eAsyncResult_Success.

//////////////////////////////////////////////////////////////////////////
// SAsyncReadToken
//////////////////////////////////////////////////////////////////////////
struct SAsyncReadToken
{
public:
	FAsyncOpCallback			m_callback;				//!< User callback when IO completes, is canceled, or an error occurs
	void*						m_userData;				//!< User data
	void*						m_buffer;				//!< Buffer to read into
	Int64						m_offset;				//!< File offset to read from
	Uint32						m_numberOfBytesToRead;	//!< Number of bytes to read from the file

public:
	SAsyncReadToken()
		: m_callback( nullptr )
		, m_userData( nullptr )
		, m_buffer( nullptr )
		, m_offset( 0 )
		, m_numberOfBytesToRead( 0 )
	{}
};

REDIO_NAMESPACE_END
