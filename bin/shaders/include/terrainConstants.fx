#define PATCH_RES	32
#define TESS_BLOCK_RES 16

#define NUM_TESS_BLOCKS_PER_PATCH (PATCH_RES/TESS_BLOCK_RES)

// Want to be able to use this CB under different registers in different shaders so that we don't have the same data passed around through VSC_Whatever or PSC_BlahBlah.
#define DEFINE_TERRAIN_PARAMS_CB( cbReg )		START_CB( TerrainParams, cbReg )			\
												struct STerrainParams						\
												{											\
													float	lowestElevation;				\
													float	highestElevation;				\
													float	mipmapLevelRes;					\
													float	numLevels;						\
													float	terrainEdgeLength;				\
													float	fullResolution;					\
													float	interVertexSpace;				\
													float	colormapStartingIndex;			\
													float	screenSpaceErrorThresholdNear;	\
													float	screenSpaceErrorThresholdFar;	\
													float	quadTreeNodeSize;				\
												} TerrainParams;						\
												END_CB
