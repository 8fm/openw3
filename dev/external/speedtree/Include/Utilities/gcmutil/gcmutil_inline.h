/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 330.001
 *  Copyright (C) 2009 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */

#ifndef __CELL_GCMUTIL_INLINE_H__
#define __CELL_GCMUTIL_INLINE_H__


extern inline uint32_t cellGcmUtilGetAlign(uint32_t size, uint32_t alignment)
{
	return (size == 0? 0: (alignment == 0? size: (static_cast<uint32_t>((size - 1) / alignment) + 1) * alignment));
}

extern inline float cellGcmUtilToRadian(float degree){
	return 3.1415926535f * degree / 180.0f;
}
extern inline float cellGcmUtilToDegree(float radian){
	return radian * 180.0f / 3.1415926535f;
}


#endif /* __CELL_GCMUTIL_INLINE_H__ */

