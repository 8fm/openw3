/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "decalEmitter.h"
#include "dynamicDecal.h"
#include "renderer.h"
#include "world.h"
#include "bitmapTexture.h"
#include "..\core\depot.h"
#include "renderCommands.h"

CLightweightDecalEmitter::CLightweightDecalEmitter()
{
	
}
	
void CLightweightDecalEmitter::Spawn( CWorld* world, const Vector& position, const Vector& front, const Vector& up, CBitmapTexture* footStepDecalDiff, CBitmapTexture* footStepDecalNorm, Uint8 atlasScaleS, Uint8 atlasScaleT, Uint8 atlasTile )
{
	IRenderScene* scene = world->GetRenderSceneEx();

	SDynamicDecalInitInfo footDecal;

	if( footStepDecalDiff ) 
	{
		footDecal.m_diffuseTexture = footStepDecalDiff->GetRenderResource();
	}
	if( footStepDecalNorm )
	{
		footDecal.m_normalTexture = footStepDecalNorm->GetRenderResource();
	}

	footDecal.m_width = 0.25;
	footDecal.m_height = 0.40;
	footDecal.m_doubleSided = false;
	footDecal.m_origin = position;
	footDecal.m_dirUp = up;
	footDecal.m_dirFront = front;
	footDecal.m_fadeTime = 15.0f;
	footDecal.m_timeToLive = 20.0f;
	footDecal.m_farZ = 0.5;
	footDecal.m_nearZ = -0.1f;

	footDecal.SetAtlasVector(atlasScaleS, atlasScaleT, atlasTile);

	IRenderResource* decal = GRender->CreateDynamicDecal( footDecal );
	if ( decal != nullptr )
	{
		( new CRenderCommand_AddDynamicDecalToScene( scene, decal, true ) )->Commit();
		decal->Release();
	}
}