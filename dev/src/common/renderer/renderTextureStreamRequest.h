/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../engine/renderTextureStreamRequest.h"


class IRenderProxyBase;
class CRenderTextureBase;
class CRenderMesh;


class CRenderTextureStreamRequest : public IRenderTextureStreamRequest
{
private:
	THashSet< CRenderTextureBase* >			m_textures;				// The textures in this request.
	THashSet< CRenderTextureBase* >			m_pendingTextures;		// The textures that we're still waiting on.

	mutable Red::Threads::CAtomic< Bool >	m_isReady;				// Mutable so we can switch to a ready state on timeout.
	Bool									m_lockTextures;

	CTimeCounter							m_timeout;				// Track how long since we were started, in case of timeout.


	// HACK : Texture Stream Request is holding on to some meshes too :)
	THashSet< CRenderMesh* >				m_meshes;


	Red::Threads::CAtomic< Bool >			m_isCancelled;

public:
	CRenderTextureStreamRequest( Bool lockTextures );
	virtual ~CRenderTextureStreamRequest();

	virtual void AddRenderTexture( IRenderResource* texture ) override;
	virtual void AddRenderMesh( IRenderResource* mesh ) override;

	virtual Bool IsReady() const override;

	// Start the request. This must not be called during or after Cancel.
	void Start();

	// Cancel the request. Release locks on textures, and set Ready state.
	// Must not call before or during Start. Otherwise, or if Start will not be called, safe to call from anywhere.
	void Cancel();

	// Return true if the request is completed.
	Bool OnTextureStreamed( CRenderTextureBase* texture );
	// If a texture was canceled, we'll stop waiting for it. Return true if the request is completed.
	Bool OnTextureCanceled( CRenderTextureBase* texture );
};
