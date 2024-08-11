/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderObject.h"


class IRenderResource;
class IMaterial;
class CEntity;
class CEntityTemplate;


// Request and lock streaming on a set of textures. The textures in the request will have highest priority for streaming, and not
// unstreamed unless absolutely necessary. After submitting, the lock remains in effect as long as the request object exists.
class IRenderTextureStreamRequest : public IRenderObject
{
public:
	virtual ~IRenderTextureStreamRequest() {}

	// These can be called up until the request is submitted through render command. After submission, the request must not be modified.
	virtual void AddRenderTexture( IRenderResource* texture ) = 0;

	// HACK : Can also add a mesh to the request. The request will hold a reference to the mesh, so it won't get destroyed early.
	virtual void AddRenderMesh( IRenderResource* mesh ) = 0;

	void AddMaterial( IMaterial* material );
	void AddEntity( const CEntity* entity, Bool includeAppearance = true );
	void AddEntityTemplate( CEntityTemplate* templ );
	void AddEntityAppearance( CEntityTemplate* templ, CName appearance );

	// Check if the textures from this request are ready. Safe to call at any time.
	// HACK : Also only ready after no mesh buffers are loading.
	virtual Bool IsReady() const = 0;
};
