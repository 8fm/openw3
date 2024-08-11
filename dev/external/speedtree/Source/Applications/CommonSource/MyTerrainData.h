///////////////////////////////////////////////////////////////////////  
//  MyTerrainData.h
//
//	All source files prefixed with "My" indicate an example implementation, 
//	meant to detail what an application might (and, in most cases, should)
//	do when interfacing with the SpeedTree SDK.  These files constitute 
//	the bulk of the SpeedTree Reference Application and are not part
//	of the official SDK, but merely example or "reference" implementations.
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
//      http://www.idvinc.com


///////////////////////////////////////////////////////////////////////  
//  Preprocessor

#pragma once
#include "Core/Core.h"
#include "Core/Vector.h"
#include "Core/String.h"
#include "Forest/Forest.h"
#include "MyCmdLineOptions.h"
#include "My2dInterpolator.h"
#include "Utilities/Utility.h"


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{
	///////////////////////////////////////////////////////////////////////  
	//	Forward references

	struct SUserConfig;


	///////////////////////////////////////////////////////////////////////  
	//	Class CMyTerrainData
	//
	//	This class serves as merely an example way to feed data into the
	//	terrain engine housed in the SpeedTree SDK. CMyTerrainData is not
	//	part of the SpeedTree SDK.  Your application is	free to feed data 
	//	to the terrain engine in any way.
	//
	//	The ambient occlusion component is a good example of how this class
	//	is for more illustrative purposes.  It serves to demonstrate how
	//	effective darkening the terrain vertices can be in visually grounding
	//	billboards in the distance.  Our example implementation has not been
	//	optimized for memory or speed.  Normally these values would be baked
	//	in with a world builder instead of generated on the fly.

	class CMyTerrainData
	{
	public:
											CMyTerrainData( );
	virtual									~CMyTerrainData( );			

			// set up
			void							Clear(void);
			st_bool							Init(const CMyConfigFile& cConfigFile);

			// load a terrain data directory. it must include a terrain.txt file
			st_bool							IsLoaded(void) const;

			// the uniform size of the area to which the heightmap will be applied
			// these values are used when generating normals and when converting (x, y) on height lookups
			const Vec3&						GetSize(void) const					{ return m_vSize; }
			st_int32						GetNumTiles(void) const				{ return m_nTiles; }

			// load individual TGA texture maps
			st_bool							LoadHeight(const char* pFilename, EChannels eChannels = CHANNEL_ALPHA, st_int32 nTiles = 0);
			st_bool							ResampleHeight(const CMyConfigFile& cConfigFile);
			st_bool							SaveOBJ(const char* pFilename);
			
			// generate normals for the currently loaded height data
			void							GenerateNormals(void);

			// compute the slope for the currently loaded height data
			void							ComputeSlope(st_float32 fSmoothing = 0.0f);

			// queries
			void							GetHeightRange(st_float32& fMin, st_float32& fMax) const;
			st_float32						GetHeight(st_float32 x, st_float32 y) const;
			Vec3							GetNormal(st_float32 x, st_float32 y) const;
			st_float32						GetSlope(st_float32 x, st_float32 y) const;
			st_float32						GetSmoothHeight(st_float32 x, st_float32 y, st_float32 fDistance) const;

			// tree query, returns false if placement fails placement parameters
			st_bool							GetHeightFromPlacementParameters(st_float32& fHeight, st_float32 x, st_float32 y, 
																			 st_float32 fMinHeight, st_float32 fMaxHeight,
																			 st_float32 fMinSlope = 0.0f, st_float32 fMaxSlope = 1.0f) const;

			// tree dimming
			void							ComputeAmbientOcclusion(const CMyConfigFile& cConfigFile, const CMyInstancesContainer& sInstances);
			st_float32						GetAmbientOcclusion(st_float32 x, st_float32 y) const;

	private:
			Vec3							m_vSize;
			st_float32						m_fHeightScalar;
			st_float32						m_fMinHeight;
			st_float32						m_fMaxHeight;
			st_float32						m_afAOBounds[4];
			st_int32						m_nTiles;

			struct SNormalData
			{
				Vec3						m_vUpper;
				Vec3						m_vLower;
			};

			CMy2dInterpolator<st_float32>	m_cHeightData;
			CMy2dInterpolator<SNormalData>	m_cNormalData;
			CMy2dInterpolator<st_float32>	m_cSlopeData;
			CMy2dInterpolator<st_uchar>		m_cAOData;
	};

	// include inline functions
	#include "MyTerrainData_inl.h"

} // end namespace SpeedTree


