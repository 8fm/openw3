/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 330.001
 *  Copyright (C) 2008 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */

#ifndef __CELL_GCMUTIL_TYPE_H__
#define __CELL_GCMUTIL_TYPE_H__


#if defined(__cplusplus)
namespace CellGcmUtil{
#endif


struct Memory_t{
  	uint32_t offset;
  	void* addr;
  	uint32_t size;
  	uint8_t location;
    uint8_t  _padding0;
    uint16_t _padding1;

#if defined(__cplusplus)
	Memory_t(): offset(0), addr(0), size(0), location(0), _padding0(0), _padding1(0){}
#endif
};

struct FileList_t{
	uint32_t count;
	uint32_t size;
	char **files;

#if defined(__cplusplus)
	FileList_t(): count(0), size(0), files(0){}
#endif
};

struct Viewport_t{
	uint16_t x, y, width, height;
	float min, max;
	float scale[4], offset[4];
};

struct Shader_t{
	CGprogram program;
	void *ucode;
	uint32_t offset;
	bool is_vp;

	bool is_cgb;
	CellCgbProgram program_cgb;
	CellCgbVertexProgramConfiguration vertex_program_conf;
	CellCgbFragmentProgramConfiguration fragment_program_conf;

	Memory_t body;

#if defined(__cplusplus)
	Shader_t(): program(0), ucode(0), offset(0), is_vp(false), is_cgb(false){}
#endif
};

typedef struct _CellGcmUtilUnpackedTileInfo{
	uint8_t enabled;
	uint8_t location;
	uint32_t offset;
	uint32_t size;
	uint32_t pitch;
	uint8_t comp;
	uint16_t base;
	uint16_t limit;
	uint32_t bank;
}CellGcmUtilUnpackedTileInfo;

typedef struct _CellGcmUtilUnpackedZcullInfo{
	uint8_t enabled;
	uint32_t offset;
	uint32_t width;
	uint32_t height;
	uint32_t cullStart;
	uint32_t zFormat;
	uint32_t aaFormat;
	uint32_t zcullDir;
	uint32_t zcullFormat;
	uint32_t sFunc;
	uint32_t sRef;
	uint32_t sMask;
}CellGcmUtilUnpackedZcullInfo;



#if defined(__cplusplus)
} // namespace CellGcmUtil
#endif


#endif /* __CELL_GCMUTIL_TYPE_H__ */

