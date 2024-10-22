// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.

#include "ApexGeneralizedCubeTemplates.h"

namespace physx
{
namespace apex
{

ApexGeneralizedCubeTemplates::ApexGeneralizedCubeTemplates()
{
	// init basis
	physx::PxU32 p = 0;
	const physx::PxF32 d = 1.0f / (SUB_GRID_LEN - 1);
	for (physx::PxU32 xi = 0; xi < SUB_GRID_LEN; xi++)
	{
		const physx::PxF32 wx = d * xi;
		const physx::PxF32 mx = 1.0f - wx;
		for (physx::PxU32 yi = 0; yi < SUB_GRID_LEN; yi++)
		{
			const physx::PxF32 wy = d * yi;
			const physx::PxF32 my = 1.0f - wy;
			for (physx::PxU32 zi = 0; zi < SUB_GRID_LEN; zi++)
			{
				const physx::PxF32 wz = d * zi;
				const physx::PxF32 mz = 1.0f - wz;
				mBasis[p][0] = mx * my * mz;
				mBasis[p][1] = wx * my * mz;
				mBasis[p][2] = wx * wy * mz;
				mBasis[p][3] = mx * wy * mz;
				mBasis[p][4] = mx * my * wz;
				mBasis[p][5] = wx * my * wz;
				mBasis[p][6] = wx * wy * wz;
				mBasis[p][7] = mx * wy * wz;
				p++;
			}
		}
	}

	// default vertex positions
	mVertPos[ 0] = physx::PxVec3(0.5f, 0.0f, 0.0f);		// on edges
	mVertPos[ 1] = physx::PxVec3(1.0f, 0.5f, 0.0f);
	mVertPos[ 2] = physx::PxVec3(0.5f, 1.0f, 0.0f);
	mVertPos[ 3] = physx::PxVec3(0.0f, 0.5f, 0.0f);
	mVertPos[ 4] = physx::PxVec3(0.5f, 0.0f, 1.0f);
	mVertPos[ 5] = physx::PxVec3(1.0f, 0.5f, 1.0f);
	mVertPos[ 6] = physx::PxVec3(0.5f, 1.0f, 1.0f);
	mVertPos[ 7] = physx::PxVec3(0.0f, 0.5f, 1.0f);
	mVertPos[ 8] = physx::PxVec3(0.0f, 0.0f, 0.5f);
	mVertPos[ 9] = physx::PxVec3(1.0f, 0.0f, 0.5f);
	mVertPos[10] = physx::PxVec3(1.0f, 1.0f, 0.5f);
	mVertPos[11] = physx::PxVec3(0.0f, 1.0f, 0.5f);
	mVertPos[12] = physx::PxVec3(0.0f, 0.5f, 0.5f);		// on faces
	mVertPos[13] = physx::PxVec3(1.0f, 0.5f, 0.5f);
	mVertPos[14] = physx::PxVec3(0.5f, 0.0f, 0.5f);
	mVertPos[15] = physx::PxVec3(0.5f, 1.0f, 0.5f);
	mVertPos[16] = physx::PxVec3(0.5f, 0.5f, 0.0f);
	mVertPos[17] = physx::PxVec3(0.5f, 0.5f, 1.0f);
	mVertPos[18] = physx::PxVec3(0.5f, 0.5f, 0.5f);		// in center

	// init other data structures
	for (int i = 0; i < NUM_SUB_CELLS; i++)
	{
		mSubGrid[i].init();
	}

	for (int i = 0; i < NUM_CASES_3; i++)
	{
		mFirst3[i] = -1;
	}

	mLookupIndices3.clear();

	createLookupTable3();
}




void ApexGeneralizedCubeTemplates::getTriangles(const int groups[8], physx::Array<physx::PxI32> &indices)
{
	physx::PxI32 maxGroup = 0;
	for (physx::PxU32 i = 0; i < 8; i++)
	{
		if (groups[i] > maxGroup)
		{
			maxGroup = groups[i];
		}
	}

	//physx::Array<physx::PxI32> currentIndices;

	indices.clear();
	PX_ASSERT(maxGroup < 8);
	if (maxGroup <= 2 && !mLookupIndices3.empty())
	{
		physx::PxU32 code = 0;
		for (physx::PxI32 i = 7; i >= 0; i--)
		{
			code = code * 3 + groups[i];
		}

		physx::PxI32 first = mFirst3[code];
		if (first >= 0)
		{
			for (physx::PxU32 i = first; mLookupIndices3[i] >= 0; i++)
			{
				indices.pushBack(mLookupIndices3[i]);
				//currentIndices.pushBack(mLookupIndices3[i]);
			}
			return;
		}
	}

	setCellGroups(groups);
	splitDisconnectedGroups();
	findVertices();
	createTriangles(indices);
}




void ApexGeneralizedCubeTemplates::createLookupTable3()
{
	if (!mLookupIndices3.empty())
	{
		return;
	}

	physx::PxI32 groups[8];
	physx::Array<physx::PxI32> indices;
	indices.reserve(32);

	mLookupIndices3.clear();
	mLookupIndices3.reserve(169200);		// otherwise it crashes in release mode!

	for (physx::PxU32 i = 0; i < NUM_CASES_3; i++)
	{
		physx::PxU32 c = i;
		for (physx::PxU32 j = 0; j < 8; j++)
		{
			groups[j] = c % 3;
			c /= 3;
		}

		getTriangles(groups, indices);

		mFirst3[i] = mLookupIndices3.size();
		for (physx::PxU32 j = 0; j < indices.size(); j++)
		{
			mLookupIndices3.pushBack(indices[j]);
		}

		mLookupIndices3.pushBack(-1);	// marks end
	}
}



void ApexGeneralizedCubeTemplates::setCellGroups(const physx::PxI32 groups[8])
{
	physx::PxF32 groupWeights[8];
	physx::PxU32 p = 0;

	for (physx::PxU32 xi = 0; xi < SUB_GRID_LEN; xi++)
	{
		for (physx::PxU32 yi = 0; yi < SUB_GRID_LEN; yi++)
		{
			for (physx::PxU32 zi = 0; zi < SUB_GRID_LEN; zi++)
			{
				GenSubCell& c = mSubGrid[p];

				physx::PxF32* basis = mBasis[p];
				p++;
				c.init();

				memset(groupWeights, 0, sizeof(physx::PxF32) * 8);

				for (physx::PxU32 corner = 0; corner < 8; corner++)
				{
					groupWeights[groups[corner]] += basis[corner];
				}

				c.group = 0;
				physx::PxF32 maxWeight = groupWeights[0];
				for (physx::PxU32 group = 1; group < 8; group++)
				{
					if (groupWeights[group] > maxWeight)
					{
						maxWeight = groupWeights[group];
						c.group = group;
					}
				}
			}
		}
	}
}



void ApexGeneralizedCubeTemplates::splitDisconnectedGroups()
{
	// in certain cases groups are not connected sub spaces
	// in that case we want to create (more) new connected groups

	physx::Array<GenCoord> queue;
	//GenCoord c;

	const int n[6][3] =
	{
		{ -1, 0, 0},
		{ 1, 0, 0},
		{ 0, -1, 0},
		{ 0, 1, 0},
		{ 0, 0, -1},
		{ 0, 0, 1}
	};

	int newGroup = -1;
	for (physx::PxU32 xi = 0; xi < SUB_GRID_LEN; xi++)
	{
		for (physx::PxU32 yi = 0; yi < SUB_GRID_LEN; yi++)
		{
			for (physx::PxU32 zi = 0; zi < SUB_GRID_LEN; zi++)
			{
				GenSubCell& start = mSubGrid[cellNr(xi, yi, zi)];

				if (start.group < 0 || start.marked)
				{
					continue;
				}

				newGroup++;
				physx::PxI32 oldGroup = start.group;

				GenCoord c;
				c.xi = xi;
				c.yi = yi;
				c.zi = zi;

				queue.pushBack(c);
				while (!queue.empty())
				{
					c = queue[queue.size() - 1];
					queue.popBack();

					GenSubCell& cell = mSubGrid[cellNr(c.xi, c.yi, c.zi)];

					if (cell.marked)
					{
						continue;
					}

					cell.marked = true;
					cell.group = newGroup;

					for (physx::PxU32 i = 0; i < 6; i++)
					{
						GenCoord ci = c;
						ci.xi = c.xi + n[i][0];
						ci.yi = c.yi + n[i][1];
						ci.zi = c.zi + n[i][2];
						if (ci.xi < 0 || ci.xi >= SUB_GRID_LEN ||
						        ci.yi < 0 || ci.yi >= SUB_GRID_LEN ||
						        ci.zi < 0 || ci.zi >= SUB_GRID_LEN)
						{
							continue;
						}

						GenSubCell& celli = mSubGrid[cellNr(ci.xi, ci.yi, ci.zi)];

						if (celli.marked || celli.group != oldGroup)
						{
							continue;
						}

						queue.pushBack(ci);
					}
				}
			}
		}
	}
}



void ApexGeneralizedCubeTemplates::findVertices()
{
	//int i,j,xi,yi,zi;

	for (physx::PxU32 i = 0; i < 8; i++)
		for (physx::PxU32 j = 0; j < 8; j++)
		{
			mFirstPairVertex[i][j] = -1;
		}

	const int faces[6][4] =
	{
		{0, 4, 7, 3},
		{1, 2, 6, 5},
		{0, 1, 5, 4},
		{2, 3, 7, 6},
		{0, 3, 2, 1},
		{4, 5, 6, 7},
	};
	const int edges[12][6] =
	{
		{0, 1, 2, 3, 4, 5},
		{0, 1, 2, 3, 5, 6},
		{0, 1, 2, 3, 6, 7},
		{0, 1, 2, 3, 7, 4},
		{4, 5, 6, 7, 0, 1},
		{4, 5, 6, 7, 1, 2},
		{4, 5, 6, 7, 2, 3},
		{4, 5, 6, 7, 3, 0},
		{0, 1, 4, 5, 3, 7},
		{0, 1, 4, 5, 2, 6},
		{3, 2, 7, 6, 1, 5},
		{3, 2, 7, 6, 0, 4},
	};

	for (physx::PxI32 xi = 0; xi <= SUB_GRID_LEN; xi++)
	{
		for (physx::PxI32 yi = 0; yi <= SUB_GRID_LEN; yi++)
		{
			for (physx::PxI32 zi = 0; zi <= SUB_GRID_LEN; zi++)
			{
				physx::PxI32 groups[8];
				groups[0] = groupAt(xi - 1, yi - 1, zi - 1);
				groups[1] = groupAt(xi  , yi - 1, zi - 1);
				groups[2] = groupAt(xi  , yi  , zi - 1);
				groups[3] = groupAt(xi - 1, yi,   zi - 1);
				groups[4] = groupAt(xi - 1, yi - 1, zi);
				groups[5] = groupAt(xi  , yi - 1, zi);
				groups[6] = groupAt(xi  , yi,   zi);
				groups[7] = groupAt(xi - 1, yi,   zi);

				// edges test
				physx::PxU32 numEdges = 0;
				for (physx::PxU32 i = 0; i < 6; i++)
				{
					physx::PxI32 g0 = groups[faces[i][0]];
					physx::PxI32 g1 = groups[faces[i][1]];
					physx::PxI32 g2 = groups[faces[i][2]];
					physx::PxI32 g3 = groups[faces[i][3]];
					physx::PxU32 flips = 0;
					if (g0 != g1)
					{
						flips++;
					}
					if (g1 != g2)
					{
						flips++;
					}
					if (g2 != g3)
					{
						flips++;
					}
					if (g3 != g0)
					{
						flips++;
					}
					if (flips > 2)
					{
						numEdges++;
					}
				}

				physx::PxI32 vertexNr = -1;

				// edge vertex?
				if (numEdges > 1)
				{
					for (physx::PxU32 i = 0; i < 12; i++)
					{
						if (groups[edges[i][0]] < 0 &&
						        groups[edges[i][1]] < 0 &&
						        groups[edges[i][2]] < 0 &&
						        groups[edges[i][3]] < 0 &&
						        groups[edges[i][4]] < 0 &&
						        groups[edges[i][5]] < 0)
						{
							vertexNr = i;
						}
					}
				}

				// face vertex?
				if (vertexNr < 0 && numEdges > 2)
				{
					for (physx::PxU32 i = 0; i < 6; i++)
					{
						if (groups[faces[i][0]] < 0 &&
						        groups[faces[i][1]] < 0 &&
						        groups[faces[i][2]] < 0 &&
						        groups[faces[i][3]] < 0)
						{
							vertexNr = 12 + i;
						}
					}
				}

				// inner vertex
				if (vertexNr < 0 && numEdges > 2)
				{
					vertexNr = 18;
				}

				mVertexAt[xi][yi][zi] = vertexNr;

				if (vertexNr < 0)
				{
					continue;
				}

				for (physx::PxU32 i = 0; i < 8; i++)
				{
					if (groups[i] < 0)
					{
						continue;
					}

					for (physx::PxU32 j = 0; j < 8; j++)
					{
						if (groups[j] < 0)
						{
							continue;
						}

						if (vertexNr > mFirstPairVertex[groups[i]][groups[j]])
						{
							mFirstPairVertex[groups[i]][groups[j]] = vertexNr;
							mFirstPairCoord[groups[i]][groups[j]].init(xi, yi, zi);
						}
					}
				}
			}
		}
	}
}


void ApexGeneralizedCubeTemplates::createTriangles(physx::Array<physx::PxI32>& currentIndices)
{
	physx::Array<physx::PxI32> verts;
	physx::Array<GenCoord> queue;

	currentIndices.clear();

	for (physx::PxU32 group0 = 0; group0 < 8; group0++)
	{
		for (physx::PxU32 group1 = group0 + 1; group1 < 8; group1++)
		{
			physx::PxI32 first = mFirstPairVertex[group0][group1];

			if (first < 0)
			{
				continue;
			}

			// walk around the edge of the surface between group 0 and group 1
			verts.clear();

			GenCoord c;
			for (c.xi = 0; c.xi <= SUB_GRID_LEN; c.xi++)
				for (c.yi = 0; c.yi <= SUB_GRID_LEN; c.yi++)
					for (c.zi = 0; c.zi <= SUB_GRID_LEN; c.zi++)
					{
						mVertexMarked[c.xi][c.yi][c.zi] = false;
					}

			c = mFirstPairCoord[group0][group1];
			queue.pushBack(c);
			while (!queue.empty())
			{
				c = queue[queue.size() - 1];
				queue.popBack();

				if (vertexMarked(c))
				{
					continue;
				}

				markVertex(c);

				int vert = mVertexAt[c.xi][c.yi][c.zi];
				if (vert >= 0)
				{
					if (verts.size() == 1)
					{
						queue.clear();    // otherwise the search goes both ways around the border
					}
					if (verts.empty() || verts[verts.size() - 1] != vert)
					{
						verts.pushBack(vert);
					}
				}

				GenCoord next;
				next = c;
				next.xi--;
				if (!vertexMarked(next) && isEdge(next, 0, group0, group1))
				{
					queue.pushBack(next);
				}
				next = c;
				next.yi--;
				if (!vertexMarked(next) && isEdge(next, 1, group0, group1))
				{
					queue.pushBack(next);
				}
				next = c;
				next.zi--;
				if (!vertexMarked(next) && isEdge(next, 2, group0, group1))
				{
					queue.pushBack(next);
				}
				next = c;
				next.xi++;
				if (!vertexMarked(next) && isEdge(c, 0, group0, group1))
				{
					queue.pushBack(next);
				}
				next = c;
				next.yi++;
				if (!vertexMarked(next) && isEdge(c, 1, group0, group1))
				{
					queue.pushBack(next);
				}
				next = c;
				next.zi++;
				if (!vertexMarked(next) && isEdge(c, 2, group0, group1))
				{
					queue.pushBack(next);
				}
			}

			if (verts.size() < 3)
			{
				continue;
			}

			// create triangle fan
			if (verts[0] == verts[verts.size() - 1])
			{
				verts.popBack();
			}

			for (int i = 1; i + 1 < (int)verts.size(); i++)
			{
				// PH: Attemt to fix: This is a double fan where the initial value also appears somewhere in the middle
				if (verts[0] == verts[i] || verts[0] == verts[i + 1])
				{
					continue;
				}

				PX_ASSERT(verts[0] != verts[i]);
				PX_ASSERT(verts[0] != verts[i + 1]);
				PX_ASSERT(verts[i] != verts[i + 1]);
				currentIndices.pushBack(verts[0]);
				currentIndices.pushBack(verts[i]);
				currentIndices.pushBack(verts[i + 1]);
			}
		}
	}
}



bool ApexGeneralizedCubeTemplates::isEdge(const GenCoord& c, physx::PxI32 dim, physx::PxI32 group0, physx::PxI32 group1)
{
	if (dim < 0 || dim >= 3)
	{
		return false;
	}

	int g0, g1, g2, g3;
	if (dim == 0)
	{
		g0 = groupAt(c.xi, c.yi,   c.zi);
		g1 = groupAt(c.xi, c.yi - 1, c.zi);
		g2 = groupAt(c.xi, c.yi - 1, c.zi - 1);
		g3 = groupAt(c.xi, c.yi,   c.zi - 1);
	}
	else if (dim == 1)
	{
		g0 = groupAt(c.xi,   c.yi, c.zi);
		g1 = groupAt(c.xi,   c.yi, c.zi - 1);
		g2 = groupAt(c.xi - 1, c.yi, c.zi - 1);
		g3 = groupAt(c.xi - 1, c.yi, c.zi);
	}
	else
	{
		g0 = groupAt(c.xi,   c.yi,   c.zi);
		g1 = groupAt(c.xi - 1, c.yi,   c.zi);
		g2 = groupAt(c.xi - 1, c.yi - 1, c.zi);
		g3 = groupAt(c.xi,   c.yi - 1, c.zi);
	}

	physx::PxU32 flips = 0;
	if (g0 != g1)
	{
		flips++;
	}
	if (g1 != g2)
	{
		flips++;
	}
	if (g2 != g3)
	{
		flips++;
	}
	if (g3 != g0)
	{
		flips++;
	}

	if (flips <= 2)
	{
		return false;
	}

	if (group0 < 0 || group1 < 0)
	{
		return true;
	}

	if (g0 == group0 && g1 == group1)
	{
		return true;
	}
	if (g1 == group0 && g2 == group1)
	{
		return true;
	}
	if (g2 == group0 && g3 == group1)
	{
		return true;
	}
	if (g3 == group0 && g0 == group1)
	{
		return true;
	}

	if (g0 == group1 && g1 == group0)
	{
		return true;
	}
	if (g1 == group1 && g2 == group0)
	{
		return true;
	}
	if (g2 == group1 && g3 == group0)
	{
		return true;
	}
	if (g3 == group1 && g0 == group0)
	{
		return true;
	}

	return false;
}

}
} // end namespace physx::apex

