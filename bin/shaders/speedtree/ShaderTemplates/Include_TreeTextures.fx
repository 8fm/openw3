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

#ifndef ST_INCLUDE_TREE_TEXTURES
#define ST_INCLUDE_TREE_TEXTURES


///////////////////////////////////////////////////////////////////////
//	Texture/material bank used for geometry types
//
//	In OpenGL/GLSL, changes in these sampler names (or additions) should be followed by 
// a change to BindTextureSamplers() in Shaders_inl.h in the OpenGL render library.

DeclareTexture(DiffuseMap, 0);
DeclareTexture(NormalMap, 1);
DeclareTexture(DetailDiffuseMap, 2);
DeclareTexture(DetailNormalMap, 3);
DeclareTexture(SpecularMaskMap, 4);
DeclareTexture(TransmissionMaskMap, 5);
DeclareTexture(AuxAtlas1, 6);
DeclareTexture(AuxAtlas2, 7);
DeclareTexture(NoiseMap, 8);
DeclareTexture(PerlinNoiseKernel, 9);
DeclareTexture(ImageBasedAmbientLighting, 10);
//LAVA++
// For tree fading, bind our own dissolve pattern. This ensures a consistent dissolve experience
// between meshes and trees, and also we can be sure that it's been bound (ST might not bind its
// own noise-dissolve when not needed).
DeclareTexture(DissolveMap, 11);
DeclareTexture(MipNoiseTexture, 12); // Adding grain to the billboards, to make them less blobby
// LAVA--


#endif // ST_INCLUDE_TREE_TEXTURES
