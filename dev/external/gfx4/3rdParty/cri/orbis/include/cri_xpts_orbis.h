/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 1998-2012 CRI Middleware Co., Ltd.
 *
 * Library  : CRI Middleware Library
 * Module   : CRI Common Header for PS4
 * File     : cri_xpts_ps4.h
 * Date     : 2012-10-04
 * Version  : 2.01
 *
 ****************************************************************************/

#if !defined(CRI_INCL_CRI_XPTS_PS4_H)
#define CRI_INCL_CRI_XPTS_PS4_H

/*****************************************************************************
 * ��{�f�[�^�^�錾
 *****************************************************************************/

#if !defined(_TYPEDEF_CriUint8)
#define _TYPEDEF_CriUint8
typedef unsigned char			CriUint8;		/* �����Ȃ��P�o�C�g���� */
#endif

#if !defined(_TYPEDEF_CriSint8)
#define _TYPEDEF_CriSint8
typedef signed char				CriSint8;		/* �������P�o�C�g���� */
#endif

#if !defined(_TYPEDEF_CriUint16)
#define _TYPEDEF_CriUint16
typedef unsigned short			CriUint16;		/* �����Ȃ��Q�o�C�g���� */
#endif

#if !defined(_TYPEDEF_CriSint16)
#define _TYPEDEF_CriSint16
typedef signed short			CriSint16;		/* �������Q�o�C�g���� */
#endif

#if !defined(_TYPEDEF_CriUint32)
#define _TYPEDEF_CriUint32
typedef unsigned int			CriUint32;		/* �����Ȃ��S�o�C�g���� */
#endif

#if !defined(_TYPEDEF_CriSint32)
#define _TYPEDEF_CriSint32
typedef signed int				CriSint32;		/* �������S�o�C�g���� */
#endif

#if !defined(_TYPEDEF_CriUint64)
#define _TYPEDEF_CriUint64
typedef unsigned long			CriUint64;		/* �����Ȃ��W�o�C�g���� */
#endif

#if !defined(_TYPEDEF_CriSint64)
#define _TYPEDEF_CriSint64
typedef signed long				CriSint64;		/* �������W�o�C�g���� */
#endif

#if !defined(_TYPEDEF_CriUint128)
#define _TYPEDEF_CriUint128
//typedef unsigned __int128		CriUint128;		/* �����Ȃ�16�o�C�g���� */
typedef struct {								/* �����Ȃ�16�o�C�g���� */
	CriUint64			h;						/* ���64�r�b�g */
	CriUint64			l;						/* ����64�r�b�g */
} CriUint128;
#endif

#if !defined(_TYPEDEF_CriSint128)
#define _TYPEDEF_CriSint128
//typedef signed __int128			CriSint128;		/* ������16�o�C�g���� */
typedef struct {								/* ������16�o�C�g���� */
	CriSint64	h;								/* ���64�r�b�g */
	CriUint64	l;								/* ����64�r�b�g */
} CriSint128;
#endif

#if !defined(_TYPEDEF_CriFloat16)
#define _TYPEDEF_CriFloat16
typedef signed short			CriFloat16;		/* �Q�o�C�g���� */
#endif

#if !defined(_TYPEDEF_CriFloat32)
#define _TYPEDEF_CriFloat32
typedef float					CriFloat32;		/* �S�o�C�g���� */
#endif

#if !defined(_TYPEDEF_CriFloat64)
#define _TYPEDEF_CriFloat64
typedef double					CriFloat64;		/* �W�o�C�g���� */
#endif

#if !defined(_TYPEDEF_CriFixed32)
#define _TYPEDEF_CriFixed32
typedef signed int				CriFixed32;		/* �Œ菬���_32�r�b�g */
#endif

#if !defined(_TYPEDEF_CriBool)
#define _TYPEDEF_CriBool
typedef CriSint32				CriBool;		/* �_���^�i�_���萔��l�ɂƂ�j */
#endif

#if !defined(_TYPEDEF_CriChar8)
#define _TYPEDEF_CriChar8
typedef char					CriChar8;		/* �����^ */
#endif

/*****************************************************************************
 * �|�C���^���i�[�\�Ȑ����^
 *****************************************************************************/
#if !defined(_TYPEDEF_CriSintPtr)
#define _TYPEDEF_CriSintPtr
typedef signed long			CriSintPtr; // 8 bytes pointer
#endif

#if !defined(_TYPEDEF_CriUintPtr)
#define _TYPEDEF_CriUintPtr
typedef unsigned long		CriUintPtr; // 8 bytes pointer
#endif

/*****************************************************************************
 * �Ăяo���K��
 *****************************************************************************/

#if !defined(CRIAPI)
#define CRIAPI
#endif

#endif	/* CRI_INCL_CRI_XPTS_PS4_H */

/* end of file */
