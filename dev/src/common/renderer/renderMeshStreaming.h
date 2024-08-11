/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Stats + global control for mesh streaming
class CRenderMeshStreaming
{
public:
	CRenderMeshStreaming();

	// buffer counting
	void ReportBufferPending( const Uint32 size );
	void ReportBufferLoaded( const Uint32 size );

	// next frame ticked
	void NextFrame();

	// get mesh streaming stats
	void GetStats( Uint32& outNumBuffers, Uint32& outBufferSize ) const;

	// are we loading meshes ? right now ?
	const Bool IsLoading() const;

private:
	Red::Threads::CAtomic< Int32 >		m_numBuffersPending;
	Red::Threads::CAtomic< Int32 >		m_sizeBuffersPending;

	Uint32								m_lastFrameBuffersCount;
	Uint32								m_lastFrameBuffersSize;
};