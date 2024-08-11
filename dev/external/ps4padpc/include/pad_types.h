/*
 * SCE CONFIDENTIAL
 * Pad Library for PC Games 1.4.1
 * Copyright (C) 2014 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#ifndef _SCE_LIBPAD_TYPES_H
#define  _SCE_LIBPAD_TYPES_H


/* header file */
#include <stdint.h>


/*
 * Retrun value OK (by sceerror.h)
 */
#if !defined(SCE_OK)
#define SCE_OK							0
#endif  /* !defined(SCE_OK) */


/*
 * type for UserID
 * 
 * Type definition of UserID (by user_service_defs.h)
 */
typedef int32_t SceUserServiceUserId;


/*
 * Maximum login users at the same time (by user_service.h)
 */
#define SCE_USER_SERVICE_MAX_LOGIN_USERS	(4)


/*
 * User Id definition (by user_service.h)
 */
#define SCE_USER_SERVICE_STATIC_USER_ID_1	(1)
#define SCE_USER_SERVICE_STATIC_USER_ID_2	(2)
#define SCE_USER_SERVICE_STATIC_USER_ID_3	(3)
#define SCE_USER_SERVICE_STATIC_USER_ID_4	(4)


/*
 * Quaternion type (by scetypes.h)
 */
/**
 * @e @brief 32 bits float quaternion type @ee
 * @j @brief 32ビット浮動小数点数クォータニオン型 @ej
 */
typedef struct SceFQuaternion {
	float x, y, z, w;
} SceFQuaternion;


/*
 * Vector type (by scetypes.h)
 */
/**
 * @e @brief 32 bits integer 2D vector type @ee
 * @j @brief 32ビット整数型2次元ベクトル @ej
 */
typedef struct SceFVector3 {
	float x, y, z;
} SceFVector3;



#endif /* _SCE_LIBPAD_TYPES_H */
