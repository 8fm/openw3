/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace CookerSpeedTreeSupport
{
	struct SRTInfo
	{
		Bool	m_isGrass;
		Vector	m_diagonalExtents;
	};

	void GetTreeCollisionShapes( const CSRTBaseTree* tree, Bool& grass, TDynArray< Sphere >& outShapes );
	Bool GetTreeInfo( const CSRTBaseTree* tree, SRTInfo& outInfo );
};