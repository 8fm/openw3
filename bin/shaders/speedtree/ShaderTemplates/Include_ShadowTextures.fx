///////////////////////////////////////////////////////////////////////
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com

#ifndef ST_INCLUDE_SHADOW_TEXTURES
#define ST_INCLUDE_SHADOW_TEXTURES


///////////////////////////////////////////////////////////////////////
//	Shadow map samplers
//
//	Not all of these shadow map samplers may be used (controlled by ST_SHADOWS_ENABLED and ST_SHADOWS_NUM_MAPS macros)

DeclareShadowMapTexture(ShadowMap0, 11);
DeclareShadowMapTexture(ShadowMap1, 12);
DeclareShadowMapTexture(ShadowMap2, 13);
DeclareShadowMapTexture(ShadowMap3, 14);

#endif // ST_INCLUDE_SHADOW_TEXTURES

