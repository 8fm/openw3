/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2005-2013 CRI Middleware Co., Ltd.
 *
 * Library  : CRI Movie
 * Module   : Library User's Header
 * File     : cri_movie_core.h
 * Date     : 2013-09-27
 * Version  : (see CRIMVPLY_VER)
 *
 ****************************************************************************/
/*!
 *	\file		cri_movie_core.h
 */
#ifndef	CRI_MOVIE_CORE_H_INCLUDED		/* Re-definition prevention */
#define	CRI_MOVIE_CORE_H_INCLUDED

/*	Version No.	*/
#define	CRIMVPLY_VER		"3.40"
#define	CRIMVPLY_NAME		"criMvPly"

/***************************************************************************
 *      Include file
 ***************************************************************************/
#include <cri_xpt.h>
#include <cri_sj.h>
#include <cri_heap.h>

/***************************************************************************
 *      MACRO CONSTANT
 ***************************************************************************/

/*EN
 * \brief	Audio OFF setting of Audio Track
 * \ingroup MDL_MV_OPTION
 * \sa CriMvEasyPlayer::SetAudioTrack()
 */
/*JP
 * \brief	�I�[�f�B�I�Đ�OFF�̎w��l
 * \ingroup MDL_MV_OPTION
 * \sa CriMvEasyPlayer::SetAudioTrack()
 */
#define CRIMV_AUDIO_TRACK_OFF		(-1)

/*EN
 * \brief	Default setting of Audio Track
 * \ingroup MDL_MV_OPTION
 * \sa CriMvEasyPlayer::SetAudioTrack()
 */
/*JP
 * \brief	�I�[�f�B�I�`���l���̃f�t�H���g�l
 * \ingroup MDL_MV_OPTION
 * \sa CriMvEasyPlayer::SetAudioTrack()
 */
#define CRIMV_AUDIO_TRACK_AUTO	(100)

/*EN
 * \brief	Maximum number of PCM tracks in one audio stream
 * \ingroup MDL_MV_OPTION
 */
/*JP
 * \brief	�I�[�f�B�I�f�[�^���̍ő�PCM�g���b�N��
 * \ingroup MDL_MV_OPTION
 */
#define CRIMV_PCM_BUFFER_MAX		(8)

/*EN
 * \brief	Subtitle OFF setting
 * \ingroup MDL_MV_OPTION
 * \sa CriMvEasyPlayer::SetSubtitleChannel()
 */
/*JP
 * \brief	�����Đ�OFF�̎w��l
 * \ingroup MDL_MV_OPTION
 * \sa CriMvEasyPlayer::SetSubtitleChannel()
 */
#define CRIMV_SUBTITLE_CHANNEL_OFF	(-1)

/*EN
 * \brief	Maximum number of video tracks in a movie file
 * \ingroup MDL_MV_OPTION
 */
/*JP
 * \brief	���[�r�t�@�C�����̍ő�r�f�I�X�g���[����
 * \ingroup MDL_MV_OPTION
 */
#define CRIMV_MAX_VIDEO_NUM			(1)

/*EN
 * \brief	Maximum number of audio tracks in a movie file
 * \ingroup MDL_MV_OPTION
 */
/*JP
 * \brief	���[�r�t�@�C�����̍ő�I�[�f�B�I�X�g���[����
 * \ingroup MDL_MV_OPTION
 */
#define CRIMV_MAX_AUDIO_NUM			(32)

/*EN
 * \brief	Maximum number of alpha tracks in a movie file
 * \ingroup MDL_MV_OPTION
 */
/*JP
 * \brief	���[�r�t�@�C�����̍ő�A���t�@�X�g���[����
 * \ingroup MDL_MV_OPTION
 */
#define CRIMV_MAX_ALPHA_NUM			(1)

#if defined(XPT_TGT_PC)
/*EN
 * \brief	Number of extra threads for multicore decoding
 * \ingroup MDL_MV_OPTION
 * The number of additional decoding threads that CRI Movie library internally creates.
 * The threads are intended to run on multiple processors in parallel.
 * \sa CriMvEasyPlayer::SetUsableProcessors_PC()
 */
/*JP
 * \brief	�}���`�R�A�f�R�[�h�p�̒ǉ��̃f�R�[�h�X���b�h��
 * \ingroup MDL_MV_OPTION
 * CRI Movie���C�u�����������ō쐬����ǉ��̃f�R�[�h�̐��ł��B�����̃X���b�h�́A�}���`�R�APC���
 * �f�R�[�h��������񕪎U�����邽�߂ɍ���܂��B
 * \sa CriMvEasyPlayer::SetUsableProcessors_PC()
 */
#define CRIMV_NUM_EXT_DECTHREAD_PC		(3)

/*EN
 * \brief	Default affnity mask of a thread
 * \ingroup MDL_MV_OPTION
 * \sa CriMvEasyPlayer::SetUsableProcessors_PC()
 */
/*JP
 * \brief	�X���b�h�A�t�B�j�e�B�}�X�N�̃f�t�H���g�ݒ�l
 * \ingroup MDL_MV_OPTION
 * \sa CriMvEasyPlayer::SetUsableProcessors_PC()
 */
#define CRIMV_DEFAULT_AFFNITY_MASK_PC		(0xFFFFFFFF)


/*EN
 * \brief	Default priority of a thread
 * \ingroup MDL_MV_OPTION
 * \sa CriMvEasyPlayer::SetUsableProcessors_PC()
 */
/*JP
 * \brief �X���b�h�̃f�t�H���g�D��x
 * \ingroup MDL_MV_OPTION
 * \sa CriMvEasyPlayer::SetUsableProcessors_PC()
 */
#define CRIMV_DEFAULT_THREAD_PRIORITY_PC		(0x8000000)
#endif

/*EN
 * \brief	Default value of the read buffer size
 * \ingroup MDL_MV_OPTION
 * \sa CriMvEasyPlayer::SetReadBufferSize()
 */
/*JP
 * \brief	���[�h�o�b�t�@�T�C�Y���f�t�H���g�l
 * \ingroup MDL_MV_OPTION
 * \sa CriMvEasyPlayer::SetReadBufferSize()
 */
#define CRIMV_READ_BUFFER_SIZE_AUTO	(-1)


/***************************************************************************
 *      Library Spec Defenition
 ***************************************************************************/

/* <These definitions are for CRI internal use> */

/* 2007-09-06:URUSHI �I�[�f�B�I�����̃T�u���W���[����                   *
 * MvPly�̃I�[�f�B�I����������CriMvPlyAmng�Ƃ����V���ȃN���X�ɐ؂�o���B*
 * CriMvPlyAmng�̓f�}���`�v���N�T����S�g���b�N�f�[�^���󂯎���āA     *
 * �g���b�N���ƂɊ���U��ꂽAdec�ɏ�����n���܂��B                     *
 * �ړI�͈ȉ��̓��                                                     *
 * 1) �I�[�f�B�I�g���b�N�̓��I�ؑ�                                      *
 * 2) ���[�v�Đ��ł̈قȂ�`�u�ڂ̓���                                  */

#define NUM_MAX_ADEC  (2)	/* Adec�̍ő吔 */
/* ADEC�̃C���f�b�N�X��` */
/* ����index���g����CriMvPlyAmng����Adec���R���g���[�����Ă��������B */
#define MAIN_ADEC_IDX (0)		// ���C���g���b�N�p
#define SUB_ADEC_IDX  (1)		// �{�C�X�g���b�N�p

/* �I�[�f�B�I�̓��I�ؑ֋@�\���T�|�[�g���邩 */
//#define ENABLE_DYNAMIC_AUDIO_SWITCH

#if defined(ENABLE_DYNAMIC_AUDIO_SWITCH)
	#define CRIMVPLYAMNG_TRACK_OFF (512)			// �؂�ւ���g���b�N�ԍ��̃f�t�H���g�l�i�ؑւn�e�e�j

	/* �g���b�N�̓��I�ؑւ̂��߂̏�Ԓ�` */
	typedef enum _crimvplyamng_track_state {
		CRIMVPLYAMNG_TRACK_STATE_FIXED = (1),      // �f�t�H���g
		CRIMVPLYAMNG_TRACK_STATE_PREP_SWITCHING,   // ���[�U���ؑւ𖽗߂��A�ؑւ̏����i�K�i��������̂��߂̊���Ԃ��Z�b�g�j
		CRIMVPLYAMNG_TRACK_STATE_SWITCHING         // �ؑ֌��Ɛ�̎���������s�Ȃ��Đؑւ��s�Ȃ����
	} CriMvPlyAmngTrackState;
#endif

/* �A���Đ����A2�ڈȍ~�̃w�b�_���擾�ł���悤�ɂ��邽�� */
#define CRIMVPLY_HEAD_CONTAINER_NUM  (2)

/* �ēǂݍ���臒l�̃f�t�H���g�l */
#define CRIMV_DEFAULT_RELOAD_THRESHOLD		(0.8f)		// 0.8[sec]

/* �Đ��������̒����t���[�����f�t�H���g�l */
#define CRIMV_DEFAULT_NUM_FRAMES_FOR_PREP	(-1) /* �f�t�H���g�F�s�g�p (�t���[���v�[�������̗p) */

/* CriMvPly�����ŃL�[�v����f���Q�[�g�X���b�h�ݒ�p�̔z�� */
#if defined(XPT_TGT_PC)
	#define CRIMV_DLGTHREAD_NUM		(64)
#elif defined(XPT_TGT_XBOX360)
	#define CRIMV_DLGTHREAD_NUM		(6 - 1)		// 6 HW thread - ���[�U�A�C�h���X���b�h
#elif defined(XPT_TGT_VITA)
	#define CRIMV_DLGTHREAD_NUM		(2)
#elif defined(XPT_TGT_WIIU)
	#define CRIMV_DLGTHREAD_NUM		(2)
#endif

/* �������[�N�̈�̊m�ۂ�CRI Heap���g�p���Ȃ� */
#define CRIMV_REMOVE_CRIHEAP

/* �@��ŗL�t���[����� */
#define CRIMV_FRAME_DETAILS_NUM		(2)

/* CriVavfios �Ŏw�肷��O���t�@�C���̃p�X�̏�� */
#if	defined(XPT_TGT_IOS)
	#define CRIMVPLY_VAVFIOS_MAX_FILEPATH	(256)
#endif

/***************************************************************************
 *      Process MACRO
 ***************************************************************************/
#define criMv_SetDefaultHandleConfig(p_config)	\
{\
	(p_config)->readbuffer_size = 0;\
}

/***************************************************************************
 *      Enum declaration
 ***************************************************************************/
/*EN
 * \brief	Speaker index of CRI Movie PCM output
 * \ingroup MDL_MV_OPTION
 */
/*JP
 * \brief	CRI Movie ��PCM�o�͂̃X�s�[�J�[�z�u
 * \ingroup MDL_MV_OPTION
 */
typedef enum {
	CRIMV_PCM_BUFFER_L		 = 0,	/*EN< The LEFT channel of CRI Movie output 					*/
									/*JP< CRI Movie �I�[�f�B�I�o�͂� LEFT �`�����l�� 			*/
	CRIMV_PCM_BUFFER_R		 = 1,	/*EN< The RIGHT channel of CRI Movie output 				*/
									/*JP< CRI Movie �I�[�f�B�I�o�͂� RIGHT �`�����l�� 			*/
	CRIMV_PCM_BUFFER_LS		 = 2,	/*EN< The Surround LEFT channel of CRI Movie output 		*/
									/*JP< CRI Movie �I�[�f�B�I�o�͂� Surround LEFT �`�����l�� 	*/
	CRIMV_PCM_BUFFER_RS		 = 3,	/*EN< The Surround RIGHT channel of CRI Movie output 		*/
									/*JP< CRI Movie �I�[�f�B�I�o�͂� Surround RIGHT �`�����l�� 	*/
	CRIMV_PCM_BUFFER_C		 = 4,	/*EN< The CENTER channel of CRI Movie output 				*/
									/*JP< CRI Movie �I�[�f�B�I�o�͂� CENTER �`�����l�� 			*/
	CRIMV_PCM_BUFFER_LFE	 = 5,	/*EN< The LFE channel of CRI Movie output 					*/
									/*JP< CRI Movie �I�[�f�B�I�o�͂� LFE �`�����l�� 			*/
	CRIMV_PCM_BUFFER_EXT1	 = 6,	/*EN< The EXT1(Rear Left) channel of CRI Movie output 		*/
									/*JP< CRI Movie �I�[�f�B�I�o�͂� EXT1(Rear Left) �`�����l�� */
	CRIMV_PCM_BUFFER_EXT2	 = 7,	/*EN< The EXT2(Rear Right) channel of CRI Movie output 		*/
									/*JP< CRI Movie �I�[�f�B�I�o�͂� EXT2(Rear Right) �`�����l�� */

	/* Keep enum 4bytes */
	CRIMV_PCM_BUFFER_MAKE_ENUM_SINT32 = 0x7FFFFFFF
} CriMvPcmBufferIndex;


/*EN
 * \brief	Composite mode of alpha movie
 * \ingroup MDL_MV_OPTION
 */
/*JP
 * \brief	�A���t�@���[�r�̍������[�h
 * \ingroup MDL_MV_OPTION
 */
typedef enum {
	CRIMV_COMPO_OPAQ		 = 0,	/*EN< Opacity, no alpha value								*/
									/*JP< �s�����A�A���t�@���Ȃ�								*/
	CRIMV_COMPO_ALPHFULL	 = 1,	/*EN< Full alpha blending (8bits-alpha data) 				*/
									/*JP< �t��Alpha�����i�A���t�@�p�f�[�^��8�r�b�g)				*/
	CRIMV_COMPO_ALPH3STEP	 = 2,	/*EN< 3 Step Alpha											*/
									/*JP< 3�l�A���t�@											*/
	CRIMV_COMPO_ALPH32BIT	 = 3,	/*EN< Full alpha blending (32bits color + alpha data)		*/
									/*JP< �t��Alpha�A�i�J���[�ƃA���t�@�f�[�^��32�r�b�g�j		*/
	CRIMV_COMPO_ALPH1BIT	 = 4,	/*EN< Alpha blending (24bits color + 1->8bits alpha)        */
									/*JP< �t��Alpha�A�i�J���[�ƃA���t�@�f�[�^��32bit�A�l��2�l�j */
	CRIMV_COMPO_ALPH2BIT	 = 5,	/*EN< Alpha blending (24bits color + 2->8bits alpha)        */
									/*JP< �t��Alpha�A�i�J���[�ƃA���t�@�f�[�^��32bit�A�l��4�l�j */
	CRIMV_COMPO_ALPH3BIT	 = 6,	/*EN< Alpha blending (24bits color + 3->8bits alpha)        */
									/*JP< �t��Alpha�A�i�J���[�ƃA���t�@�f�[�^��32bit�A�l��8�l�j */
	CRIMV_COMPO_ALPH4BIT	 = 7,	/*EN< Alpha blending (24bits color + 4->8bits alpha)        */
									/*JP< �t��Alpha�A�i�J���[�ƃA���t�@�f�[�^��32bit�A�l��16�l�j*/

	/* Keep enum 4bytes */
	CRIMV_COMPO_MAKE_ENUM_SINT32 = 0x7FFFFFFF
} CriMvAlphaType;


/*EN
 * \brief	Result of the last video frame retrieval
 * \ingroup MDL_MV_OPTION
 */
/*JP
 * \brief	�O��̃r�f�I�t���[���擾�̌���
 * \ingroup MDL_MV_OPTION
 */
typedef enum {
	CRIMV_LASTFRAME_OK				= 0,	/*EN< Succeeded													*/
											/*JP< �擾����													*/
	CRIMV_LASTFRAME_TIME_EARLY		= 1,	/*EN< Failed. The frame is not yet the time to draw				*/
											/*JP< �擾���s�B�t���[���\���������Đ����ԂɒB���Ă��Ȃ�����    */
	CRIMV_LASTFRAME_DECODE_DELAY    = 2,	/*EN< Failed. The frame to draw is not decoded yet				*/
											/*JP< �擾���s�B�r�f�I�t���[���̃f�R�[�h���Ԃɍ���Ȃ�����	    */
	CRIMV_LASTFRAME_DISCARDED       = 3,	/*EN< Failed. The video frame is discarded by app				*/
											/*JP< �擾���s�B�A�v���ɂ���Ĕj�����ꂽ						*/
	//CRIMV_LASTFRAME_NO_INPUT_DATA = 4,	//Not supported yet
	//CRIMV_LASTFRAME_SKIPPED       = 5,	//Not supported yet
	//CRIMV_LASTFRAME_DEMUX_STUCKED = 6,	//Not supported yet

	/* Keep enum 4bytes */
	CRIMV_LASTFRAME_MAKE_ENUM_SINT32 = 0x7FFFFFFF
} CriMvLastFrameResult;

#if defined(XPT_TGT_PS3PPU)
/*EN
 * \brief	Graphic Environment type for PS3
 * \ingroup MDL_MV_OPTION
 */
/*JP
 * \brief	PS3�̃O���t�B�b�N��
 * \ingroup MDL_MV_OPTION
 */
typedef enum {
	CRIMV_GRAPHIC_ENV_GCM	= 0,	/*EN< GCM. (or same ARGB 32bit texture format of GCM)			*/
									/*JP< GCM�� (�܂��̓e�N�X�`���t�H�[�}�b�g��GCM�Ɠ�����)		*/
	CRIMV_GRAPHIC_ENV_PSGL	= 1,	/*EN< PSGL. (or same ARGB 32bit texture format of PSGL)			*/
									/*JP< PSGL�� (�܂��̓e�N�X�`���t�H�[�}�b�g��PSGL�Ɠ�����)	*/

	/* Keep enum 4bytes */
	CRIMV_GRAPHIC_ENV_MAKE_ENUM_SINT32 = 0x7FFFFFFF
} CriMvGraphicEnv;
#endif


/* CRI Movie Interanl handle status
 * 
 * The possible legal states of a CriMvPly handle.  Over the course of your application
 * the CriMvPly handle will walk through the following states in roughly the following
 * order.  In states that are waiting states, you can request the
 * CriMvPly handle to advance to the next state by calling the criMvPly_IncrementState()
 * function.  You can check the status of a valid CriMvPly handle at any time
 * by calling criMvPly_GetStatus().
 *
 * It is not possible to jump from a state to an arbitrary other state.  Normal play
 * proceeds from the CRIMVPLY_STATUS_STOP state through the CRIMV_PLY_STATUS_PLAYEND
 * state in that order.  Premature termination can be requested with the criMvPly_Stop()
 * function.
 *
 * \sa criMvPly_IncrementState(), criMvPly_GetStatus(), criMvPly_Stop()
 */
typedef enum {
	CRIMVPLY_STATUS_STOP			= 0,	/* Standstill.  No processing is occurring.
											 * CriMvPly handles are created into this state.
											 */
											/* ��~�� */
	CRIMVPLY_STATUS_DECHDR			= 1,	/* The CriMvPly structure is now parsing the header
											 * of the file, including information on height and width
											 * of the video stream.
											 */
											/* �w�b�_��͒� */
	CRIMVPLY_STATUS_WAIT_PREP		= 2,	/* The header has been decoded and criMvPly_GetStreamingParameters()
											 * will now provide valid values.  Typically you will call
											 * criMvPly_AllocateWorkBuffer() with this information at this point.
											 */
											/* PREP��Ԃւ�IncrementState�҂�<BR>
											   AllocateWorkBuffer���Ă��玟�ւ������� */
	CRIMVPLY_STATUS_PREP			= 3,	/* Transition to this state to acknowledge to the
											 * CriMvPly handle that you have allocated your work buffers. */
											/* �Đ������� */
	CRIMVPLY_STATUS_WAIT_PLAYING	= 4,	/* The audio and video decoders are now ready to begin playback.*/
											/* PLAYING��Ԃւ�IncrementState�҂�<BR>
											   ���̏�ԂŊ��Ƀr�f�I�ƃI�[�f�B�I�̃f�R�[�h���ʂ͎擾�ł���B*/
	CRIMVPLY_STATUS_PLAYING			= 5,	/* The decoders are currently decoding and playing output. */
											/* �Đ��� */
	CRIMVPLY_STATUS_WAIT_PLAYEND	= 6,	/* The library is waiting for you to acknowledge the end of the movie.  You
											 * have informed the CriMvPly structure that an end-of-file condition exists,
											 * but final frames of video and audio may still be pending in your application. */
											/* PLAYEND��Ԃւ�IncrementState�҂� */
	CRIMVPLY_STATUS_PLAYEND			= 7,	/* You have acknowledged the end of the movie.  Teardown can occur at this point. */
											/* �Đ��I�� */
	CRIMVPLY_STATUS_STOP_PROCESSING	= 8,	/* A request to stop has been received by the CriMvPly structure,
											 *    that is, you have called criMvPly_Stop(), and a stop is now pending. */
											/* ��~������ */
	CRIMVPLY_STATUS_WAIT_STOP		= 9,	/* The CriMvPly handle has acknowledged the stop request and
											 * you may now call criMvPly_IncrementState() to transition to
											 * the CRIMVPLY_STATUS_STOP state. */
											/* STOP��Ԃւ�IncrementState�҂� */
	CRIMVPLY_STATUS_ERROR			= 10,	/* An error has occurred. */
											/* �G���[ */

	/* Keep enum 4bytes */
	CRIMVPLY_STATUS_MAKE_ENUM_SINT32 = 0x7FFFFFFF
} CriMvPlyStatus;


/* Sofdec2 */
typedef enum {
	CRIMVPLY_META_FLAG_OFF  = 0x0000,
	CRIMVPLY_META_FLAG_CUE  = 0x0001,
	CRIMVPLY_META_FLAG_SEEK = 0x0002,
	CRIMVPLY_META_FLAG_ALL  = CRIMVPLY_META_FLAG_CUE + CRIMVPLY_META_FLAG_SEEK,

	/* Keep enum 4bytes */
	CRIMVPLY_META_FLAG_MAKE_ENUM_SINT32 = 0x7FFFFFFF
} CriMvMetaFlag;

/* Color Conversion*/
typedef enum{
	CRIMV_COLORSPACE_CONVERSION_TYPE_ITU_R_BT601_LIMITED	= 0,
	CRIMV_COLORSPACE_CONVERSION_TYPE_ITU_R_BT601_FULLRANGE	= 1,

	/* Keep enum 4bytes */
	CRIMV_COLORSPACE_CONVERSION_TYPE_ENUM_SINT32 			= 0x7FFFFFFF
}CriMvColorSpaceConversionType;

typedef void *(*CriMvMetaDataWorkMallocFunc)(void *obj, CriUint32 size);
typedef void (*CriMvMetaDataWorkFreeFunc)(void *obj, void *mem);

/* OUTER_FRAMEPOOL_WORK */
#define CriMvFramepoolWorkMallocFunc   CriMvMetaDataWorkMallocFunc
#define CriMvFramepoolWorkFreeFunc     CriMvMetaDataWorkFreeFunc

/***************************************************************************
 *      Data type declaration
 ***************************************************************************/

/*EN
 * \brief Audio parameters
 * \ingroup MDL_MV_INFO
 * 
 * \sa CriMvStreamingParameters, CriMvWaveInfo
 */
/*JP 
 * \brief �I�[�f�B�I�p�����[�^
 * \ingroup MDL_MV_INFO
 * 
 * �I�[�f�B�I�X�g���[���̃p�����[�^
 * \sa CriMvStreamingParameters, CriMvWaveInfo
 */
typedef struct {
	CriUint32		sampling_rate;		/*EN< Sampling rate */
										/*JP< �T���v�����O���g�� */
	CriUint32		num_channel;		/*EN< Number of channels. Monaural = 1, Stereo = 2 */
										/*JP< �I�[�f�B�I�`���l���� */
	CriUint32		total_samples;		/*EN< Total number of samples */
										/*JP< ���T���v���� */
	CriUint32		output_buffer_samples;	/*EN< Output wave buffer size */
											/*JP< �T�E���h�o�̓o�b�t�@�̃T���v���� */
	CriUint32		codec_type;			/*EN< Codec type */
										/*JP< �R�[�f�b�N��� */

} CriMvAudioParameters;

/*EN
 * \brief Video Parameters
 * \ingroup MDL_MV_INFO
 * \sa CriMvStreamingParameters
 */
/*JP 
 * \brief �r�f�I�p�����[�^
 * \ingroup MDL_MV_INFO
 * �r�f�I�X�g���[���̃p�����[�^
 * \sa CriMvStreamingParameters
 */
typedef struct {
	CriUint32		max_width;			/*EN< Maximum video width for stream. (multiple of 8) */
										/*JP< ���[�r�ő啝�i�W�̔{���j */
	CriUint32		max_height;			/*EN< Maximum video height for stream. (multiple of 8)*/
										/*JP< ���[�r�ő卂���i�W�̔{���j */
	CriUint32		disp_width;			/*EN< Width of the image to draw. */
										/*JP< �\���������f���̉��s�N�Z�����i���[����j */
	CriUint32		disp_height;		/*EN< Height of the image to draw. */
										/*JP< �\���������f���̏c�s�N�Z�����i��[����j */
	CriUint32		num_frame_pool;		/*EN< Number of frame pools required for stream */
										/*JP< �t���[���v�[���� */
	CriUint32		framerate;			/*EN< Frame rate per second [x1000]. */
										/*JP< �t���[�����[�g[x1000] */
	CriUint32		framerate_n;		/*EN< Frame rate (in rational as numerator). framerate_n/framerate_d = framerate */ /* UTODO: �ϐ��� */
										/*JP< �t���[�����[�g�̕��q(�L�����`��)�Bframerate_n/framerate_d = framerate */
	CriUint32		framerate_d;		/*EN< Frame rate (in rational as denominator). */
										/*JP< �t���[�����[�g�̕���(�L�����`��)�B */
	CriUint32		total_frames;		/*EN< Total number of video frames */
										/*JP< ���t���[���� */

	CriUint32		material_width;		/*EN< Width of the video source resolustion before encoding. */
										/*JP< �G���R�[�h�O�̃r�f�I�f�ނ̉��s�N�Z���� */
	CriUint32		material_height;	/*EN< Height of the video source resolustion before encoding. */
										/*JP< �G���R�[�h�O�̃r�f�I�f�ނ̏c�s�N�Z���� */
	CriUint32		screen_width;		/*EN< Screen width set by encoding and cropping. 
										 *    This parameter is only available when you encoded the movie with "Widescreen TV Support" option. 
										 *    Normally this value is 0. */
										/*JP< �G���R�[�h���Ɏw�肵���X�N���[�����B
										 *    ���̒l�̓G���R�[�h���Ɂu���C�h�e���r�x���@�\�v���g�p�����ꍇ�̂ݗL���ɂȂ�܂��B
										 *    �ʏ�͂O�ł��B */

	CriUint32		codec_type;			/*EN< Video Codec Type. If you encoded the movie for PS2, this value is 2.
										 *    Normally this value is 1 or 0(no info). 
										 *    If the codec_type is 1, the CRI Movie for ONLY PS2 can play the movie file. */
										/*JP< �r�f�I�R�[�f�b�N��ʁBPS2�p�ɃG���R�[�h�����ꍇ 2�ɂȂ�܂��B
										 *    �ʏ�� 1�܂��� 0(��񖳂�)�ł��B
										 *    �R�[�f�b�N��ʂ��Q�̃��[�r�́APS2�Ń��C�u�����Łu�̂݁v�Đ��\�ł��B */
	CriUint32		codec_dc_option;	/*EN< Video Codec DC Option. If you encoded the movie for PS2, this value is 10.
										 *    Normally this value is 11 or 0(no info). 
										 *    If the codec_type is 11, the CRI Movie for PS2 can NOT play the movie file. */
										/*JP< �r�f�I�R�[�f�b�N��DC�I�v�V������ʁBPS2�p�ɃG���R�[�h�����ꍇ10�ɂȂ�܂��B
										 *    �ʏ��11�܂��� 0(��񖳂�)�ł��B
										 *    �R�[�f�b�NDC�I�v�V������11�̃��[�r�́APS2�Ń��C�u�����u�ł́v�Đ��ł��܂���B */
	CriUint32	color_conversion_type;	/*EN< Color space converion type. Fullrange or Limited.*/
										/*JP< �F�ϊ��^�C�v�B */
	CriSint32	capacity_of_picsize;	/*EN< Capacity size of video pictures. */
										/*JP< �s�N�`���T�C�Y����l */
	CriUint32	average_bitrate;		/*EN< Average bitrate. */
										/*JP< ���σr�b�g���[�g */
} CriMvVideoParameters;

/*EN
 * \brief Alpha Parameters
 * \ingroup MDL_MV_INFO
 * \sa CriMvStreamingParameters
 */
/*JP 
 * \brief �A���t�@�p�����[�^
 * \ingroup MDL_MV_INFO
 * �A���t�@�X�g���[���̃p�����[�^
 * \sa CriMvStreamingParameters
 */
typedef struct {
	CriUint32		max_width;			/*EN< Maximum alpha width for stream */
										/*JP< �A���t�@�t���[���̍ő啝 */
	CriUint32		max_height;			/*EN< Maximum alpha height for stream */
										/*JP< �A���t�@�t���[���̍ő卂�� */
	CriUint32		disp_width;			/*EN< valid alpha width */
										/*JP< �A���t�@�t���[���̎��L���� */
	CriUint32		disp_height;		/*EN< valid alpha height */
										/*JP< �A���t�@�t���[���̎��L������ */
	CriUint32		framerate;			/*EN< Frame rate per second [x1000]. */
										/*JP< �A���t�@�̃t���[�����[�g[x1000] */
	CriUint32		framerate_n;		/*EN< Frame rate (in rational as numerator). framerate_n/framerate_d = framerate */ /* UTODO: �ϐ��� */
										/*JP< �t���[�����[�g�̕��q(�L�����`��)�Bframerate_n/framerate_d = framerate */
	CriUint32		framerate_d;		/*EN< Frame rate (in rational as denominator). */
										/*JP< �t���[�����[�g�̕���(�L�����`��)�B */
	CriUint32		total_frames;		/*EN< Total number of alpha frames */
										/*JP< ���t���[���� */
	CriMvAlphaType	alpha_type;			/*EN< Alpha Composite Type. */
										/*JP< �A���t�@������ʁB */
	CriUint32		codec_type;			/*EN< Internal use only. Do not access this */
										/*JP< ���C�u���������g�p�ϐ� */
	CriUint32	color_conversion_type;	/*EN< Color space converion type. Fullrange or Limited.*/
										/*JP< �F�ϊ��^�C�v�B */
	CriSint32	capacity_of_picsize;	/*EN< Capacity size of video pictures. */
										/*JP< �s�N�`���T�C�Y����l */
	CriUint32	average_bitrate;		/*EN< Average bitrate. */
										/*JP< ���σr�b�g���[�g */
} CriMvAlphaParameters;


/*EN
 * \brief Streaming Parameters
 * \ingroup MDL_MV_INFO
 * This structure includes streaming parameters and playing parameters.
 * \sa CriMvEasyPlayer::GetMovieInfo()
 */
/*JP
 * \brief �X�g���[�~���O�Đ��p�����[�^
 * \ingroup MDL_MV_INFO
 * �X�g���[�~���O�Đ��p�����[�^�B<br>
 * �X�g���[�����̂̏��ƁA�Đ��̂��߂ɕK�v�ȃp�����[�^�̗������܂�ł���B
 * \sa CriMvEasyPlayer::GetMovieInfo()
 */
typedef struct {
	/* Stream */
	CriUint32		is_playable;		/*EN< Flag of the movie file is playable or not. 1 is playable. 0 is not playable.*/
										/*JP< �Đ��\�t���O�i1: �Đ��\�A0: �Đ��s�j */
	CriFloat32		buffering_time;		/*EN< Amount of time to buffer in the stream, in seconds */
										/*JP< �ǂݍ��݃f�[�^�̃o�b�t�@�����O���ԁB�P��[sec]�B */
	CriUint32		max_bitrate;		/*EN< Maximum bits per second for stream. This value includes video and audio both. */
										/*JP< �ő�r�b�g���[�g(�G�Ɖ��̍��v) */
	CriUint32		max_chunk_size;		/*EN< Maximum chunk size of incoming stream (USF) file */
										/*JP< �ő�USF�`�����N�T�C�Y */
	CriUint32		min_buffer_size;	/*EN< Minimum buffer size for reading */
										/*JP< �Œ���K�v�ȓǂݍ��݃o�b�t�@�T�C�Y�B<br> �I�[�f�B�I�ƃr�f�I�̍��v */
	CriSint32		read_buffer_size;	/*EN< Input buffer size for reading data */
										/*JP< ���[�h�o�b�t�@�T�C�Y */
	/* Video */
	CriUint32		num_video;			/*EN< Number of simultaneous video streams */
										/*JP< �r�f�I�f�R�[�_�̐��B���݂�1�Œ�B*/
	CriMvVideoParameters	video_prm[CRIMV_MAX_VIDEO_NUM];		/*EN< Video parameters see CriMvVideoParameters struct for details */
																/*JP< �r�f�I�p�����[�^ */
	/* Audio */
	CriUint32		num_audio;			/*EN< Number of simultaneous audio streams */
										/*JP< �I�[�f�B�I�f�R�[�_�̐��B���݂�1�Œ�B*/
	CriSint32		track_play_audio;		/*EN< Track of audio playback. */
											/*JP< �Đ�����I�[�f�B�I�`���l���ԍ��B-1�w��ōĐ������B */
	CriMvAudioParameters	audio_prm[CRIMV_MAX_AUDIO_NUM];		/*EN< Audio parameters see CriMvAudioParameters struct for details */
																/*JP< �I�[�f�B�I�p�����[�^ */
	/* Subtitle */
	CriUint32		num_subtitle;			/*EN< Number of subtitles */
											/*JP< �����`���l���� */
	CriSint32		channel_play_subtitle;	/*EN< Channel for playing subtitles */
											/*JP< �Đ����鎚���`���l���ԍ� */
	CriUint32		max_subtitle_size;		/*EN< Maximum size of subtitle data */
											/*JP< �����f�[�^�̍ő�T�C�Y*/

	/* Composite mode */
	CriUint32		num_alpha;				/*EN< Number of alpha channels (current spec allows only one) */
											/*JP< �A���t�@�f�R�[�_�̐��B���݂�1�Œ�B */
	CriMvAlphaParameters	alpha_prm[CRIMV_MAX_ALPHA_NUM]; /*EN< Alpha parameters see CriMvAlphaParameters struct for details */
															/*JP< �A���t�@�p�����[�^ */

	CriBool			seekinfo_flag;			/*EN< Flag of the movie file inclues seek info */
											/*JP< �V�[�N���t���O */
	CriUint32		format_ver;				/*EN< Format version */
											/*JP< �t�H�[�}�b�g�o�[�W���� */
} CriMvStreamingParameters;


/*EN 
 * \brief Input Buffer Information
 * \ingroup MDL_MV_INFO
 * \sa CriMvEasyPlayer::GetInputBufferInfo(),
 *     CriMvEasyPlayer::SetReloadThresholdTime(), CriMvEasyPlayer::SetBufferingTime()
 */
/*JP 
 * \brief ���̓o�b�t�@���
 * \ingroup MDL_MV_INFO
 * \sa CriMvEasyPlayer::GetInputBufferInfo(),
 *     CriMvEasyPlayer::SetReloadThresholdTime(), CriMvEasyPlayer::SetBufferingTime()
 */
typedef struct {
	CriUint32		buffer_size;		/*EN< Input buffer size [byte] */
										/*JP< ���̓o�b�t�@�T�C�Y[byte] */
	CriUint32		data_size;			/*EN< Data size in input buffer[byte] */
										/*JP< ���̓o�b�t�@�ɂ���f�[�^�T�C�Y[byte] */
	CriUint32		reload_threshold;	/*EN< Re-load threshold. When data size is less than re-load threshold, next read is requested. */
										/*JP< �ēǂݍ���臒l[byte]�B�f�[�^�T�C�Y�����̒l�ȉ��ɂȂ�Ɠǂݍ��݂��s���܂��B */
} CriMvInputBufferInfo;


// TEMP: for internal use
typedef struct {
	CriUint8	*imageptr;
	CriUint32	bufsize;		// [Byte]
	CriUint32	line_pitch;		// [Byte]
	CriUint32	line_size;		// [Byte]
	CriUint32	num_lines;
} CriMvImageBufferInfo;

/*EN 
 * \brief Video Frame Information
 * \ingroup MDL_MV_INFO
 * \sa CriMvEasyPlayer::GetFrameOnTimeAs32bitARGB(), CriMvEasyPlayer::GetFrameOnTimeAsYUVBuffers,
 *     CriMvEasyPlayer::GetFrameOnTimeAsYUV422(), CriMvEasyPlayer::DiscardNextFrame()
 */
/*JP 
 * \brief �r�f�I�t���[�����
 * \ingroup MDL_MV_INFO
 * �r�f�I�t���[�����
 * \sa CriMvEasyPlayer::GetFrameOnTimeAs32bitARGB(), CriMvEasyPlayer::GetFrameOnTimeAsYUVBuffers,
 *     CriMvEasyPlayer::GetFrameOnTimeAsYUV422(), CriMvEasyPlayer::DiscardNextFrame()
 */
typedef struct {
	CriUint8		*imageptr;		/*EN< Pointer to image data */
									/*JP< �摜�f�[�^�̃|�C���^ */
	CriSint32		frame_id;		/*EN< Frame ID ot the playback */
									/*JP< �t���[������ID�i���[�v�^�A���Đ����͒ʎZ�j */
	CriUint32		width;			/*EN< Width of movie frame [pixel] (multiple of 8) */
									/*JP< ���[�r�̉���[pixel] (�W�̔{��) */
	CriUint32		height;			/*EN< Height of movie frame [pixel] (multiple of 8) */
									/*JP< ���[�r�̍���[pixel] (�W�̔{��) */
	CriUint32		pitch;			/*EN< Pitch of movie frame [byte]*/
									/*JP< ���[�r�̃s�b�`[byte] */
	CriUint32		disp_width;		/*EN< Width of the image to draw. */
									/*JP< �\���������f���̉��s�N�Z�����i���[����j */
	CriUint32		disp_height;	/*EN< Height of the image to draw. */
									/*JP< �\���������f���̏c�s�N�Z�����i��[����j */
	CriUint32		framerate;		/*EN< Frames per second times 1000 */
									/*JP< �t���[�����[�g��1000�{�̒l */
	CriUint32		framerate_n;	/*EN< Frame rate (in rational as numerator). framerate_n/framerate_d = framerate */ /* UTODO: �ϐ��� */
									/*JP< �t���[�����[�g�̕��q(�L�����`��)�Bframerate_n/framerate_d = framerate */
	CriUint32		framerate_d;	/*EN< Frame rate (in rational as denominator). */
									/*JP< �t���[�����[�g�̕���(�L�����`��)�B */
	CriUint64		time;			/*EN< Frame time ('time / tunit' indicates time in seconds) */
									/*JP< �����Btime / tunit �ŕb��\���B */
	CriUint64		tunit;			/*EN< Unit of time measurement */
									/*JP< �����P�� */
	CriUint32		cnt_concatenated_movie;		/*EN< Number of concatenated movie data */
												/*JP< ���[�r�̘A���� */
	CriSint32		frame_id_per_data;	/*EN< Frame ID of the movie data */
										/*JP< ���[�r�f�[�^���Ƃ̃t���[���ԍ� */

	CriBool			csc_flag;	/*EN< This is temporary variable. Please don't access. */
								/*JP< �e�X�g���̕ϐ��ł��B�A�N�Z�X���Ȃ��ł��������B */

	CriMvAlphaType		alpha_type;		/*EN< Composite mode */
										/*JP< �A���t�@�̍������[�h*/

	void			*details_ptr[CRIMV_FRAME_DETAILS_NUM];	// for internal use

	CriSint32				num_images;			// TEMP: for internal use
	CriMvImageBufferInfo	image_info[4];		// TEMP: for internal use

	CriUint32	color_conversion_type;	/*EN< Color space converion type. Fullrange or Limited.*/
										/*JP< �F�ϊ��^�C�v�B */
	CriUint32	total_frames_per_data;	/* EN< Total frames of the movie data*/
											/*JP< ���[�r�f�[�^�P�ʂ̑��t���[���� */
} CriMvFrameInfo;

/*EN 
 * \brief Subtitle Information
 * \ingroup MDL_MV_INFO
 * \sa CriMvEasyPlayer::GetSubtitleOnTime()
 */
/*JP 
 * \brief �������
 * \ingroup MDL_MV_INFO
 * \sa CriMvEasyPlayer::GetSubtitleOnTime()
 */
typedef struct {
	CriUint8		*dataptr;		/*EN< Pointer to subtitle data */
									/*JP< �����f�[�^�̃|�C���^ */
	CriUint32		data_size;		/*EN< Size of subtitle data */
									/*JP< �����f�[�^�T�C�Y */
	CriSint32		channel_no;		/*EN< Channel number of subtitle data */
									/*JP< �����f�[�^�̃`���l���ԍ� */
	CriUint64		time_unit;		/*EN< Unit of time measurement */
									/*JP< �����P�� */
	CriUint64		in_time;		/*EN< Display start time */
									/*JP< �\���J�n����*/
	CriUint64		duration_time;	/*EN< Display duration time */
									/*JP< �\����������  */
	CriUint32		cnt_concatenated_movie;		/*EN< Number of concatenated movie data */
												/*JP< ���[�r�̘A���� */
	CriUint64		in_time_per_data;	/*EN< Display start time per movie data*/
										/*JP< ���[�r�f�[�^���Ƃɕ\���J�n����*/
} CriMvSubtitleInfo;

/*EN
 * \brief Event Point Info
 * \ingroup MDL_MV_INFO
 * Event point info is the each timing info was embeded to movie data as cue point info.
 * \sa CriMvEasyPlayer::GetCuePointInfo()
 */
/*JP
 * \brief �C�x���g�|�C���g���
 * \ingroup MDL_MV_INFO
 * �L���[�|�C���g�@�\�Ń��[�r�f�[�^�ɖ��ߍ��܂ꂽ�X�̃^�C�~���O���ł��B
 * \sa CriMvEasyPlayer::GetCuePointInfo()
 */
typedef struct {
	CriChar8		*cue_name;			/*EN< The name string of event point. Char code depends on cue point text. */
										/*JP< �C�x���g�|�C���g���B�����R�[�h�̓L���[�|�C���g���e�L�X�g�ɏ]���܂��B */
	CriUint32		size_name;			/*EN< The data size of name string */
										/*JP< �C�x���g�|�C���g���̃f�[�^�T�C�Y */
	CriUint64		time;				/*EN< Timer counter */
										/*JP< �^�C�}�J�E���g */
	CriUint64		tunit;				/*EN< Counter per 1 second. "count / unit" indicates the timer on the second time scale. */
										/*JP< �P�b������̃^�C�}�J�E���g�l�Bcount �� unit �ŕb�P�ʂ̎����ƂȂ�܂��B */
	CriSint32		type;				/*EN< Event point type */
										/*JP< �C�x���g�|�C���g��� */
	CriChar8		*param_string;		/*EN< The string of user parameters. Char code depends on cue point text. */
										/*JP< ���[�U�p�����[�^������B�����R�[�h�̓L���[�|�C���g���e�L�X�g�ɏ]���܂��B */
	CriUint32		size_param;			/*EN< The data size of user parameters string */
										/*JP< ���[�U�p�����[�^������̃f�[�^�T�C�Y */
	CriUint32		cnt_callback;		/*EN< The counter of calling cue point callback. */
										/*JP< �L���[�|�C���g�R�[���o�b�N�̌Ăяo���J�E���^ */
} CriMvEventPoint;

/*EN
 * \brief Cue Point Info
 * \ingroup MDL_MV_INFO
 * Cue point info includes the number of event points and the list.
 * \sa CriMvEasyPlayer::GetCuePointInfo()
 */
/*JP
 * \brief �L���[�|�C���g���
 * \ingroup MDL_MV_INFO
 * �L���[�|�C���g���́A�C�x���g�|�C���g�̌��ƈꗗ�ł��B<br>
 * \sa CriMvEasyPlayer::GetCuePointInfo()
 */
typedef struct {
	CriUint32			num_eventpoint;		/*EN< The number of event points */
											/*JP< �C�x���g�|�C���g�� */
	CriMvEventPoint		*eventtable;		/*EN< The list of event points */
											/*JP< �C�x���g�|�C���g�ꗗ */
} CriMvCuePointInfo;

/*EN
 * \brief YUV Texture Buffer Parameters
 * \ingroup MDL_MV_INFO
 * The output buffer parameters for CriMvEasyPlayer::GetFrameOnTimeAsYUVBuffers().
 * CriMvEasyPlayer::GetFrameOnTimeAsYUVBuffers() outputs data for pixel shader.<BR>
 * If an application doesn't play alpha movie, CRI Movie library doesn't use alpha buffer parameters.<BR>
 * \sa CriMvEasyPlayer::GetFrameOnTimeAsYUVBuffers()
 */
/*JP
 * \brief YUV�ʃo�b�t�@���
 * \ingroup MDL_MV_INFO
 * CriMvEasyPlayer::GetFrameOnTimeAsYUVBuffers() �̏o�̓o�b�t�@���ł��B<br>
 * CriMvEasyPlayer::GetFrameOnTimeAsYUVBuffers() ��Pixel Shader �����̃f�R�[�h���ʂ��o�͂��܂��B<br>
 * �A���t�@���[�r�Đ����s��Ȃ��ꍇ�i�s�����̒ʏ�Đ��j�́AAlpha�e�N�X�`���֘A�̃p�����[�^�͎g�p���܂���B<BR>
 * \sa CriMvEasyPlayer::GetFrameOnTimeAsYUVBuffers()
 */
typedef struct {
	CriUint8	*y_imagebuf;	/*EN< Pointer to the buffer of Y texture */
								/*JP< Y�e�N�X�`���̃o�b�t�@�|�C���^ */
	CriUint32	y_bufsize;		/*EN< Size of the buffer of Y texture [byte] */
								/*JP< Y�e�N�X�`���̃o�b�t�@�T�C�Y[byte] */
	CriUint32	y_pitch;		/*EN< Pitch of the buffer of Y texture [byte] */
								/*JP< Y�e�N�X�`���̃s�b�`[byte] */
	CriUint8	*u_imagebuf;	/*EN< Pointer to the buffer of U texture */
								/*JP< U�e�N�X�`���̃o�b�t�@�|�C���^ */
	CriUint32	u_bufsize;		/*EN< Size of the buffer of U texture [byte] */
								/*JP< U�e�N�X�`���̃o�b�t�@�T�C�Y[byte] */
	CriUint32	u_pitch;		/*EN< Pitch of the buffer of U texture [byte] */
								/*JP< U�e�N�X�`���̃s�b�`[byte] */
	CriUint8	*v_imagebuf;	/*EN< Pointer to the buffer of V texture */
								/*JP< V�e�N�X�`���̃o�b�t�@�|�C���^ */
	CriUint32	v_bufsize;		/*EN< Size of the buffer of V texture [byte] */
								/*JP< V�e�N�X�`���̃o�b�t�@�T�C�Y[byte] */
	CriUint32	v_pitch;		/*EN< Pitch of the buffer of V texture [byte] */
								/*JP< V�e�N�X�`���̃s�b�`[byte] */
	CriUint8	*a_imagebuf;	/*EN< Pointer to the buffer of Alpha texture */
								/*JP< Alpha�e�N�X�`���̃o�b�t�@�|�C���^ */
	CriUint32	a_bufsize;		/*EN< Size of the buffer of Alpha texture [byte] */
								/*JP< Alpha�e�N�X�`���̃o�b�t�@�T�C�Y[byte] */
	CriUint32	a_pitch;		/*EN< Pitch of the buffer of Alpha texture [byte] */
								/*JP< Alpha�e�N�X�`���̃s�b�`[byte] */
} CriMvYuvBuffers;


/*EN
 * \brief Playback Information
 * \ingroup MDL_MV_INFO
 * The output playback information of CriMvEasyPlayer::GetPlaybackInfo().
 * These parameters represents current movie playback information such as decode delay of movie data and 
 * interval of video frames retrieval.
 * \sa CriMvEasyPlayer::GetPlaybackInfo()
 */
/*JP
 * \brief �Đ����
 * \ingroup MDL_MV_INFO
 * CriMvEasyPlayer::GetPlaybackInfo() �̏o�͍Đ����ł��B<br>
 * �t���[���̎擾�Ԋu��f�R�[�h�̒x���Ȃǂ̌��ݍĐ����Ă��郀�[�r�̍Đ�����\���܂��B<br>
 * \sa CriMvEasyPlayer::GetPlaybackInfo()
 */
typedef struct {
	CriUint64  cnt_app_loop;						/*EN< Loop count of application. Precisely, this is a number of calls of CriMvEasyPlayer::Update(). The count up will start after app is able to acquire the first video frame */
													/*JP< �A�v���P�[�V�����̃��[�v�J�E���g�B��̓I�ɂ� CriMvEasyPlayer::Update() �̌Ăяo���񐔂ɂȂ�܂��B�ŏ��̃t���[�����擾�\�ɂȂ�ƃJ�E���g���n�܂�܂��B*/
	CriUint64  cnt_frame_interval[4];				/*EN< Interval of video frames retrieval. These values are count up when CriMvEasyPlayer::IsNextFrameOnTime() returns TRUE.
													 *
													 * The interval of video frames retrieval indicates a number of the loop count when your application calls CriMvEasyPlayer::IsNextFrameOnTime() in the main loop. 
													 * In case that the application waits for vertical retrace, 1 interval equals about 16.7 msec.
													 * The index of array represents the count of intervals as follows:
													 * <table>
													 * <TR><TH> Index <TH> Interval of video frames retrieval
													 * <TR><TD> 0 <TD> Every main loop
													 * <TR><TD> 1 <TD> 2 main loops
													 * <TR><TD> 2 <TD> 3 main loops
													 * <TR><TD> 3 <TD> 4 or more main loops
													 * </table>													
													 * With these values, you can check if the application gets video frames with appropriate intervals. Please use the values as measuring playback smoothness
													 *
													 * In order to use these values, the application must meet the following conditions:
													 * - The main loop should work periodically and stably (Ideally sync with vertical retrace)
													 * - The application should call CriMvEasyPlayer::IsNextFrameOnTime() once in everly main loop
													 * 
													 * For example, if the application runs at 59.94fps by waiting for vertical retrace and a framerate of playing movie file is 29.97fps,
													 * only cnt_frame_interval[1] should be increased.
													 */
													/*JP< �t���[���̎擾�Ԋu�B�����̒l�́A CriMvEasyPlayer::IsNextFrameOnTime() ��TRUE��Ԃ������ɃJ�E���g�A�b�v����܂��B<BR>
													 *
													 *  �t���[���̎擾�Ԋu�Ƃ́A�A�v���P�[�V���������C�����[�v���Ńt���[���擾�֐���ǂ񂾎��̃��[�v�̉񐔂��Ӗ����܂��B
													 *  ���C�����[�v��VSync�Ɠ������Ă���ꍇ�́A1 Interval = ��16.7msec�Ƃ������ƂɂȂ�܂��B
													 *  �z��̃C���f�b�N�X�́A�ȉ��̂悤�Ɏ擾�Ԋu��\���܂��B<BR>
													 * <table>
													 * <TR><TH> �C���f�b�N�X <TH> �t���[���̎擾�Ԋu
													 * <TR><TD> 0 <TD> �����C�����[�v
													 * <TR><TD> 1 <TD> 2 ���C�����[�v
													 * <TR><TD> 2 <TD> 3 ���C�����[�v
													 * <TR><TD> 3 <TD> 4 ���C�����[�v�ȏ�
													 * </table>
													 * �����̒l�����邱�ƂŁA�A�v�����������Ԋu�Ńt���[�����擾�ł����̂��ǂ������`�F�b�N���邱�Ƃ��ł��܂��B���[�r�����炩�ɍĐ��ł��Ă��邩�̖ڈ��ɂ��Ă��������B<BR>
													 *
													 * �������O��Ƃ��āA�ȉ��̏������A�v�����������Ă���K�v������܂��B
													 * - �A�v����VSync�ȂǁA���̎����ň��肵�ē��삵�Ă���
													 * - ���C�����[�v���Ŗ��� CriMvEasyPlayer::IsNextFrameOnTime() ���Ăяo��
													 *
													 * ��L�̏������ɂ����āA�Ⴆ�΃A�v����59.94fps�œ��삵�Ă����ԂŁA�t���[�����[�g��29.97fps�̃��[�r���Đ������ꍇ�Acnt_frame_interval[1]�݂̂�������������
													 * �������Ԋu�Ńt���[���̎擾���o�������ƂɂȂ�܂��B
													 */
	CriUint64  cnt_time_early;						/*EN< A count of how many times CriMvEasyPlayer::IsNextFrameOnTime() returns FALSE due to the determination if it is the time to provide the next video frame */
													/*JP< CriMvEasyPlayer::IsNextFrameOnTime() ���A�t���[���\����������ɂ��FALSE��Ԃ����񐔁B*/
	CriUint64  cnt_decode_delay;					/*EN< A count of how many times CriMvEasyPlayer::IsNextFrameOnTime() returns FALSE due to the delay of decoding movie data */
													/*JP< CriMvEasyPlayer::IsNextFrameOnTime()���A�r�f�I�t���[���̃f�R�[�h�x���ɂ��FALSE��Ԃ����� */
	CriFloat32 time_max_delay;						/*EN< Maximum delay time [msec] of the actual time a video frame retrieved against the original time should be retrieved */
													/*JP< �r�f�I�t���[�����擾�������ۂ̎����ƁA�{���\�����ׂ������Ƃ̍ő�x������ [msec]�B */
	CriFloat32 time_average_delay;					/*EN< Average delay time [msec] of the actual time a video frame retrieved against the original time should be retrieved */
													/*JP< �r�f�I�t���[�����擾�������ۂ̎����ƁA�{���\�����ׂ������Ƃ̕��ϒx������ [msec]�B */
} CriMvPlaybackInfo;

#if defined(XPT_TGT_PS3PPU)
/*EN
 * \brief Parameters of SPURS and PPU for decoding
 * \ingroup MDL_MV_BASIC
 * 
 * \sa CriMv::SetupSpursParameters_PS3(), CriMv::CalcSpursWorkSize_PS3()
 */
/*JP
 * \brief �f�R�[�h�Ɏg��SPURS�����PPU�̃p�����[�^
 * \ingroup MDL_MV_BASIC
 * 
 * \sa CriMv::SetupSpursParameters_PS3(), CriMv::CalcSpursWorkSize_PS3()
 */
typedef	struct {
	void	*spurs_handler;			/*EN< SPURS handler	*/
									/*JP< SPURS�n���h��	*/
	void	*spurs_work;			/*EN< SPURS work area. The size is spurs_worksize. The alignment is 128 byte. */
									/*JP< SPURS�p���[�N�o�b�t�@�B�o�b�t�@�T�C�Y�� spurs_worksize ��128�o�C�g���E�B */
	CriSint32	spurs_worksize;			/*EN< SPURS work size. This size is calculated by CriMv::CalcSpursWorkSize_PS3 function. */
										/*JP< SPURS�p���[�N�T�C�Y�BCriMv::CalcSpursWorkSize_PS3 �֐��Ŏ擾�����l�B */
	CriSint32	spurs_max_contention;	/*EN< SPURS max contention	*/
										/*JP< SPURS �Ń��[�r�f�R�[�h�p�Ɏg��SPU�̍ő吔	*/
	CriUint8	*spurs_task_priority;	/*EN< SPURS task priority x 8	*/
										/*JP< SPURS �̃^�X�N�v���C�I���e�B�z��B�z��v�f�͂W�B	*/

	CriUint32	ppu_num;				/*EN< The number of PPU for decoding (0-2) */
										/*JP< The number of PPU for decoding (0-2) */
	CriSint32	ppu_thread_prio;		/*EN< PPU Thread Priority. This priority is used for decoding thread in the case of ppu_num equal 2. */
										/*JP< PPU Thread Priority. ���̒l�� ppu_num ��2���w�肵���ꍇ�ɍ쐬����X���b�h�Ɏg����B	*/
} CriMvProcessorParameters_PS3;

// [NOT SUPPORT on normal library]
// for SPU Thread
typedef	struct {
	CriUint32	ppu_num;		/* The number of PPU for decoding (0-2) */
	CriSint32	ppu_prio;		/* PPU Thread Priority */
	CriUint32	spu_num;		/* The number of SPU for decoding (0-6) */
	CriSint32	spu_grp_prio;	/* SPU Thread Group Priority */
} CriMvSpuThreadParameters_PS3;

#endif

#if defined(XPT_TGT_XBOX360)
/*EN
 * \brief Parameters of Xbox360 processors for decoding
 * \ingroup MDL_MV_BASIC
 * 
 * \sa CriMvEasyPlayer::SetUsableProcessors_XBOX360()
 */
/*JP
 * \brief �f�R�[�h�Ɏg��Xbox360�v���Z�b�T�̃p�����[�^
 * \ingroup MDL_MV_BASIC
 * 
 * \sa CriMvEasyPlayer::SetUsableProcessors_XBOX360()
 */
typedef	struct {
	CriBool	processor0_flag;	/*EN< Processor 0 (Core 0, Thread 0) usable flag  */
								/*JP< �v���Z�b�T0 (�R�A0�X���b�h0) �g�p�\�t���O */
	CriBool	processor1_flag;	/*EN< Processor 1 (Core 0, Thread 1) usable flag  */
								/*JP< �v���Z�b�T1 (�R�A0�X���b�h1) �g�p�\�t���O */
	CriBool	processor2_flag;	/*EN< Processor 2 (Core 1, Thread 0) usable flag  */
								/*JP< �v���Z�b�T2 (�R�A1�X���b�h0) �g�p�\�t���O */
	CriBool	processor3_flag;	/*EN< Processor 3 (Core 1, Thread 1) usable flag  */
								/*JP< �v���Z�b�T3 (�R�A1�X���b�h1) �g�p�\�t���O */
	CriBool	processor4_flag;	/*EN< Processor 4 (Core 2, Thread 0) usable flag  */
								/*JP< �v���Z�b�T4 (�R�A2�X���b�h0) �g�p�\�t���O */
	CriBool	processor5_flag;	/*EN< Processor 5 (Core 2, Thread 1) usable flag  */
								/*JP< �v���Z�b�T5 (�R�A2�X���b�h1) �g�p�\�t���O */
	CriSint32 thread_priority;	/*EN< Priority of decoding threads on the active processors */
								/*JP< �e�v���Z�b�T��Ńf�R�[�h�������s���X���b�h�̗D��x */
} CriMvProcessorParameters_XBOX360;
#endif

#if defined(XPT_TGT_VITA)
/*EN
 * \brief AVC Decoder Parameters 
 * \ingroup MDL_MV_BASIC
 * 
 * \sa CriMv::SetupAvcDecoderParameters_VITA()
 */
/*JP
 * \brief AVC�f�R�[�_�p�����[�^
 * \ingroup MDL_MV_BASIC
 * 
 * \sa CriMv::SetupAvcDecoderParameters_VITA()
 */
typedef	struct {
	CriUint32 horizontal;		/*EN< Maximum width for decoding (in pixel)  */
								/*JP< �ő�f�R�[�h�摜�̉��� (�P�ʁF�s�N�Z��) */
	CriUint32 vertical;			/*EN< Maximum height for decoding (in pixel)  */
								/*JP< �ő�f�R�[�h�摜�̍��� (�P�ʁF�s�N�Z��) */
	CriUint32 n_ref_frames;		/*EN< Maximum reference frames on decoding (default:3) */
								/*JP< �f�R�[�h���̍ő�Q�Ɖ摜�̖��� */
	CriUint32 n_decoders;		/*EN< Maximum number of avc decoders (max:1) */
								/*JP< �����Ɏg�p����AVC�ŃR�[�_�̍ő吔 (1�Œ�) */
	
} CriMvAvcDecoderParameters_VITA;
#endif

/*--------------------------------------------------------------------------*/
/*		<Library Internal Use Only>											*/
/*--------------------------------------------------------------------------*/
typedef enum {
	CRIMV_PCM_FORMAT_SINT16	 	= 0,
	CRIMV_PCM_FORMAT_FLOAT32	= 1,

	/* Keep enum 4bytes */
	CRIMV_PCM_FORMAT__MAKE_ENUM_SINT32 = 0x7FFFFFFF
} CriMvPcmFormat;
/*
 * \brief 16bit wave data information
 * \ingroup MDL_MV_INFO
 * Information about a 16-bit waveform.
 */
/* 16bit Waveform ��� */
typedef struct {
	CriUint32		num_channel;	/* Number of Channels. monaural = 1, stereo = 2 */
									/* Number of Channels. monaural = 1, stereo = 2 */
	CriUint32		num_samples;	/* Number of sample */
									/* �T���v���� */
	CriUint32		sampling_rate;	/* Sampling rate */
									/* �T���v�����O���g�� */
} CriMvWaveInfo;

/* �I�[�f�B�I�w�b�_ */
typedef struct {
	/* �X�g���[�~���O�p�����[�^�Ƌ��� */
	CriUint32		sampling_rate;
	CriUint32		num_channel;
	CriUint32		total_samples;
	CriUint32		codec_type;

	CriUint32		metadata_count;
	CriUint32		metadata_size;

	/* �w�b�_�ŗL */
	CriUint32		a_input_xsize;
} CriMvPlyAudioHeader;

/* �r�f�I�w�b�_ */
typedef struct {
	/* �w�b�_�ŗL */
	CriUint32		width;
	CriUint32		height;
	CriUint32		disp_width;
	CriUint32		disp_height;
	CriUint32		framerate_n;
	CriUint32		framerate_d;
	CriUint32		total_frames;

	CriUint32		material_width;		/* width of video original source. 0 means no info. */
	CriUint32		material_height;	/* height of video original source. 0 means no info. */
	CriUint32		screen_width;		/* screen width for Wii */

	CriUint32		codec_type;
	CriUint32		codec_dc_option;	/* 11 or 10 */

	CriUint32		metadata_count;
	CriUint32		metadata_size;

	CriUint32		pre_padding;
	CriUint32		color_conversion_type;
	CriSint32		max_picture_size;
	CriSint32		average_bitrate;
} CriMvPlyVideoHeader;

/* �T�u�^�C�g���w�b�_ */
typedef struct {
	CriBool			is_subtitle_data;
	CriUint32		num_channel;
	CriUint64		time_unit;
	CriUint32		max_subtitle_size;
} CriMvPlySubtitleHeader;

/* �L���[�|�C���g�w�b�_ */
typedef struct {
	CriBool			is_cuepoint_data;
	CriUint32		metadata_count;
	CriUint32		metadata_size;
	CriUint32		num_eventpoint;
	CriUint64		time_unit;
} CriMvPlyCuePointHeader;

/* �A���t�@�w�b�_ */
typedef struct {
	/* �w�b�_�ŗL */
	CriUint32		width;
	CriUint32		height;
	CriUint32		disp_width;
	CriUint32		disp_height;
	CriUint32		framerate_n;
	CriUint32		framerate_d;
	CriUint32		total_frames;

	CriMvAlphaType	alpha_type;
	CriUint32		codec_type;

	CriUint32		metadata_count;
	CriUint32		metadata_size;

	CriUint32		pre_padding;
	CriUint32		color_conversion_type;
	CriSint32		max_picture_size;
	CriSint32		average_bitrate;
} CriMvPlyAlphaHeader;

/* �A���t�@�݂̂̃t���[����� */
typedef struct {
	CriUint8		*imageptr;		/*EN< Pointer to image data */
									/*JP< �摜�f�[�^�̃|�C���^ */
	CriSint32		frame_id;		/*EN< Frame ID */
									/*JP< �t���[������ID */
	CriUint32		width;			/*EN< Width of movie frame [pixel] */
									/*JP< ���[�r�̉���[pixel] */
	CriUint32		height;			/*EN< Height of movie frame [pixel] */
									/*JP< ���[�r�̍���[pixel] */
	CriUint32		disp_width;		/*EN< Width of image [pixel] */
									/*JP< �L���ȉf���̉���[pixel] */
	CriUint32		disp_height;	/*EN< Height of image [pixel] */
									/*JP< �L���ȉf���̍���[pixel] */
	CriUint32		pitch;			/*EN< Pitch of movie frame [byte]*/
									/*JP< ���[�r�̃s�b�`[byte] */
	CriUint64		time;			/*EN< Frame time ('time / tunit' indicates time in seconds) */
									/*JP< �����Btime / tunit �ŕb��\���B */
	CriUint64		tunit;			/*EN< Unit of time measurement */
									/*JP< �����P�� */
	CriSint32		frame_id_per_data;	/*EN< Frame ID of the movie data */
										/*JP< ���[�r�f�[�^���Ƃ̃t���[���ԍ� */
	CriMvAlphaType	alpha_type;		/*EN< Composite mode */
									/*JP< �A���t�@�̍������[�h*/
	void			*detail_ptr;	/* TEMP: for internal use */
	CriUint32	color_conversion_type;	/*EN< Color space converion type. Fullrange or Limited.*/
										/*JP< �F�ϊ��^�C�v�B */
} CriMvAlphaFrameInfo;

// �����Ǘ��p�B���[�r�������[�U�ɓn�����͂���Ƃقړ������낤���B
/* <Library Internal Use Only> Information of USF File */
typedef struct {
	CriBool					is_usf_file;
	CriUint32				max_chunk_size;
	CriUint32				min_buffer_size;
	CriUint32				bitrate;
	CriUint32				format_version;
	/* Video */
	CriUint32				num_video;
	CriMvPlyVideoHeader		videohead[CRIMV_MAX_VIDEO_NUM];
	/* Audio */
	CriUint32				num_audio;
	CriMvPlyAudioHeader		audiohead[CRIMV_MAX_AUDIO_NUM];
	/* Subtitle */
	CriUint32				num_subtitle;
	CriMvPlySubtitleHeader	subtitlehead;
	/* Alpha */
	CriUint32				num_alpha;
	CriMvPlyAlphaHeader		alphahead[CRIMV_MAX_ALPHA_NUM];
	/* CuePoint */
	CriUint32				num_cuepoint;
	CriMvPlyCuePointHeader	cuepointhead;
} CriMvPlyHeaderInfo;

/* <Library Internal Use Only> Video Elementary Stream */
typedef struct {
	CriUint32		fcid;
	CriSint32		track_no;	// �`�����N�̃`���l���ԍ�
	void			*vdec;
} CriMvPlyVideo;

typedef struct {
	CriUint32		fcid;
	CriSint32		track_no;	// �`�����N�̃`���l���ԍ�
	void			*dec;
} CriMvPlyAlpha;

/* ���[�r�w�b�_���Ǘ����邽�߂̍\���� */
typedef struct {
	CriMvPlyHeaderInfo	info;
	CriBool				write_new_head_flag;			// CRID���������Ď��̃w�b�_���������ޏ������ł������H
	CriUint32			num_remaining_adec_head;		// �K�v�Ȏc��̃I�[�f�B�I�w�b�_�̐�
	CriUint32			num_remaining_vdec_head;		// �K�v�Ȏc��̃r�f�I�̃w�b�_�̐�
	CriUint32			num_remaining_subtitle_head;	// �K�v�Ȏc��̎����̃w�b�_�̐�
	CriUint32			num_remaining_alpha_head;		// �K�v�Ȏc��̃A���t�@�̃w�b�_��
	CriUint32			num_remaining_cuepoint_head;	// �K�v�Ȏc��̃L���[�|�C���g�̃w�b�_��
	/* 2010-08-19: TEMP: CONCAT_KAI: Don't refer this member. */
	CriUint64			accumulated_tcount;
} CriMvHeaderInfoContainer;

typedef struct {
	CriBool		is_play_audio;
	CriUint32	fcid;
	CriUint32	track_no;							// �`�����N�̃`���l���ԍ�
	void		*adec;								// ���ۂ̃I�[�f�B�I�R�[�f�b�N
	CriUint32	num_channel;						// �f�[�^�̃`���l����
	CriUint32	sampling_rate;						// �T���v�����O���g��
	CriUint32	output_buffer_samples;
	CriSj		sji;								// UNI
	CriSj		sjo[CRIMV_PCM_BUFFER_MAX];			// RBF
	CriUint32	sjo_bufsize[CRIMV_PCM_BUFFER_MAX];
	CriBool		term_supply;						// �f�[�^�����I���ʒm�t���O
	CriBool		is_working;							// �R���e���c�`�����N������

#if defined(ENABLE_DYNAMIC_AUDIO_SWITCH)
	CriUint32	next_track_no;							// ���[�U���w�肵���ؑ֐�̃g���b�N�ԍ�
	CriUint32	last_track_switch_time;					// �ؑ֌��̃g���b�N�̍Ō�Ƀ`�����N���Ƃ�������
	CriUint32	last_track_switch_tunit;				// ��L�����̒P�� (in Hz?)
	CriMvPlyAmngTrackState	switch_state;				// �g���b�N�ؑւɂ����
#endif
} *CriMvPlyAdec, CriMvPlyAdecObj;

typedef struct {

	CriMvPlyAdec	mvply_adecs[NUM_MAX_ADEC];
	CriSint32		num_adecs;
	CriUint32		size_smpl;
	CriSj			sji;
	CriUint32		chunk_num_per_server;

	CriBool (*cbfunc_nofify_found_header)(void *usrobj, CriChunk *ckc, CriUint8 chno);
	void *usrobj_nofify_found_header;

} *CriMvPlyAmng, CriMvPlyAmngObj;


/* �V�[�N�u���b�N��� */
typedef struct {
	CriSint32   top_frame_id;
} CriMvSeekBlockInfo;

/* �X�g���[�}�p��� */
typedef struct {
	CriUint32	max_chunk_size;
	CriUint32	average_bitrate;
} CriMvStreamerInfo;


/* �n���h���쐬�p�R���t�B�O�\���� */
typedef struct {
	CriUint32	readbuffer_size;
} CriMvHandleConfig;


/*JP CRI Movie �n���h�� */
/*EN
 * A handle for a single movie.  If multiple movies are to be played simultaneously,
 * create a CriMvPly handle for each movie.
 * 
 * \sa criMvPly_Create(), criMvPly_Destroy() */
typedef struct {
	/*** Member variable is <Library Internal Use Only> ***/
	CriBool			used;
	CriMvPlyStatus	stat;
	CriBool			request_stop;
	CriBool			restrain_supply;
	CriBool			term_supply;
	void			*cs_work;
	void			*cshn;
	/* USF Header */

	CriMvHeaderInfoContainer	headinfo_container[CRIMVPLY_HEAD_CONTAINER_NUM];
	Uint16						cur_dechead_idx;
	CriUint32					cnt_dechead;		/* �w�b�_��͂��ƂɍX�V */
	CriUint32					cnt_concat;			/* GetFrame�ōX�V */

	CriMvPlyHeaderInfo	headinfo;
	CriUint32			num_headck;			/* �w�b�_��͏��������`�����N�� */
	CriFloat32			def_buffering_time;
	CriUint32			def_max_stream;
	CriUint32			def_sound_output_buffer_samples;
	CriSint32			def_track_play_audio;		/* -1 �ŃI�[�f�B�I�Đ����� */

#if	defined(XPT_TGT_XBOX360) || defined(XPT_TGT_PC) || defined(XPT_TGT_VITA) || defined(XPT_TGT_WIIU)
	CriSint32			def_num_dlg_threads;
	CriUint32			def_processor_mask;
	CriUint32			def_affinity_masks[CRIMV_DLGTHREAD_NUM];
	CriSint32			def_thread_priorities[CRIMV_DLGTHREAD_NUM];
#endif

	CriBool			is_prepare_work;
	CriMvStreamingParameters	stmprm;		/* �X�g���[�~���O�p�����[�^�̋L�^ */
	/* Demultiplexer */
	CriSint32		inputtype;			/* �X�g���[�~���O�����������H�����������j�r�i�Đ� */
	CriBool			is_usf_data;		/* ���̓t�@�C����USF�t�@�C�����H */
	void			*demux;				/* USF�f�}���`�v���N�T�n���h��		*/
	CriUint32		max_demuxout;		/* �f�}���`�v���N�T�o�͂̍ő��ʐ�	*/
	CriUint32		num_demuxout;		/* �f�}���`�v���N�T�o�͂ɐݒ�ς݂̎�ʐ�	*/
	CriSj			headanaly_in_sj;	// RBF
	CriSj			headanaly_out_sj;	// UNI
	CriSj			read_sj;			// RBF
	CriChunk		readck;

	CriSj			memplay_sj;			// UNI (for memory playback)
	CriChunk		movie_on_mem;		/* �������w��̃��[�r�f�[�^�L���p�i�P�j */
	CriUint32		offset_content;		/* �������w��擪�f�[�^�̃R���e���c�{�̂܂ł̃T�C�Y */

	/* === �n���h���쐬���Ɋm�� === */
	CriHeap			heap_gen;
	/* �w�b�_��͗p�̓ǂݍ��ݗ̈� */
	CriUint32		headanaly_bufsize;
	/* �n���h�������������͍ŏ���10kbyte�m�ۂ��Ďg���܂킷�B��̓I�ɂ̓w�b�_��͗p�B */
	CriHeap			local_heap;			/* �n���h��������pHeap */
	CriSint32		local_bufsize;		/* �n���h��������pHeap�p�̃o�b�t�@�T�C�Y */
	CriUint8		*local_bufptr;		/* �n���h��������pHeap�p�̃o�b�t�@�|�C���^ */
	/* === ���^���[�N�o�b�t�@ (�w�b�_��͎��Ɋm��) === */
	CriHeap			heap_meta;
	/* === ���[�N�o�b�t�@�쐬���Ɋm�� === */
	CriHeap			heap_core;
	/* �ǂݍ��݃o�b�t�@ */
	CriUint32		size_readbuf_main;
	CriUint32		size_readbuf_ext;
	/* Video Decoder */
	CriMvPlyVideo	video;
	CriUint32		framerate_n;
	CriUint32		framerate_d;
	/* Audio Decoder */
	CriMvPlyAmng	audio_mngr;
	CriMvPcmFormat	pcmfmt;
	CriUint32		size_smpl;
	CriHeap			heap_audio2;
	/* Subtitle */
	CriSj			sjo_subtitle;
	CriSint32		concat_subtitle_cnt;	/* �����̘A�������� */
	CriSint32		ch_subtitle;	/* �����̘A�������� */
	/* Alpha */
	CriMvPlyAlpha	alpha;

	/* �܂�Ԃ��`�����N�Ή��p�i�g�����ǂ����Ɋ֌W�Ȃ��ϐ������͒�`����j */
	CriUint32		bufsize_read_main;	/* ����SJ�̃o�b�t�@�{�̃T�C�Y */
	CriUint32		bufsize_read_ext;	/* ����SJ�̂̂肵��T�C�Y */
	CriUint8		*read_sj_bufptr;	/* ����RBSJ�̐擪�o�b�t�@�A�h���X */

	/* �X�g���[�~���O�p�����[�^�ɓ����Ƃ�����i���A�������H */
	CriSint32		seek_frame_id;			/* �V�[�N�������t���[��ID�iGOP�̓r���̉\������j */
	CriSint32		video_gop_top_id;		/* �V�[�N��̃r�f�IGOP�擪�t���[��ID : 0�ȉ��ŃV�[�N���� */
	CriSint32		alpha_gop_top_id;		/* �V�[�N��̃A���t�@GOP�擪�t���[��ID : 0�ȉ��ŃV�[�N���� */
	CriBool			seek_video_prep_flag;	/* �V�[�N�Đ��̃r�f�I���������t���O�iGOP�r���܂Ői�񂾂��H�j */
	CriBool			seek_alpha_prep_flag;	/* �V�[�N�Đ��̃A���t�@���������t���O�iGOP�r���܂Ői�񂾂��H�j */
	CriBool			seek_audio_prep_flag;	/* �V�[�N�Đ��̃I�[�f�B�I���������t���O�i�V�[�N�w�莞���܂Ŏ̂Ă����H�j */

	CriSint32		dechdr_stage;		/* DECHDR�̐i�݋ */
	CriSint32		sji_meta_bufsize;	/* ���^�f�[�^�p���̓o�b�t�@�T�C�Y */
	CriSj			sji_meta;			/* ���^�f�[�^�p����SJ */
	CriUint32		cnt_meta_ck;		/* ���^�f�[�^�p����SJ */
	void			*video_seektbl_ptr;
	CriUint32		video_seektbl_size;
	CriSint32		video_gop_num;
	void			*alpha_seektbl_ptr;
	CriUint32		alpha_seektbl_size;
	CriSint32		alpha_gop_num;

	void			*audio_header_ptr[CRIMV_MAX_AUDIO_NUM];
	CriUint16		audio_header_size[CRIMV_MAX_AUDIO_NUM];

	void					*cuepoint_meta_ptr;
	CriUint32				cuepoint_meta_size;
	CriMvCuePointInfo		cuepoint_info;

	CriMvInputBufferInfo	ibuf_info;
	CriFloat32				reload_sec_threshold;

	CriSint32		num_frames_for_prep;

	/* For Sofdec2 */
	CriHeap		heap_playback;
	CriMvMetaDataWorkMallocFunc cbfunc_meta_alloc;
	CriMvMetaDataWorkFreeFunc cbfunc_meta_free;
	void* usrobj_meta_data;	
	void* meta_data_work_allocated;		/* ���[�U�A���P�[�^�Ŋm�ۂ��ꂽ���^�f�[�^���[�N */
	void* event_table_work_allocated;	/* ���[�U�A���P�[�^�Ŋm�ۂ��ꂽ�C�x���g�e�[�u�� */
	CriMvStreamerInfo streamer_info;

	/* OUTER_FRAMEPOOL_WORK */
	CriMvFramepoolWorkMallocFunc cbfunc_framepool_alloc;
	CriMvFramepoolWorkFreeFunc   cbfunc_framepool_free;
	void* usrobj_framepool;
	void* framepool_work_allocated;	/* ���[�U�A���P�[�^�Ŋm�ۂ��ꂽ�t���[���v�[�����[�N�i����K�v�j */
	void* framepool_work_set;			/* ���ڃo�b�t�@�w�肳�ꂽ�t���[���v�[�����[�N�i����s�v�j */

	/* For Debug */
	volatile CriUint8 end_sequence_info;
	CriUint64Adr	header_ptr;
	CriSint64		header_size;
	CriUint64Adr	body_ptr;
	CriSint64		body_size;

	CriBool			sync_flag;

	CriBool			error_flag;

#if	defined(XPT_TGT_IOS)
	CriChar8		vavfios_filepath[CRIMVPLY_VAVFIOS_MAX_FILEPATH];
#endif

	/* For Tools */
	void			*extended_mvinfo_config;

	/* �n���h���쐬�R���t�B�O�֘A */
	CriBool				use_hn_config_flag;		/* �n���h���쐬�R���t�B�O�w�肪���������ǂ��� */
	CriMvHandleConfig	hn_config;

} *CriMvPly, CriMvPlyObj;

/***************************************************************************
 *      Function Declaration
 ***************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* ���C�u���������� */
/*
 *  \brief		���C�u�����̏�����
 *	\param		�Ȃ�
 *	\return		�Ȃ�
 *	\par ����:
 *	���C�u�����̏��������s���܂��B<BR>
 *	������A���ŏ����������ꍇ�́A�ŏ��̂P��̂ݏ��������������s���܂��B
 */
/*
 *  \ingroup MODULE_INIT
 *  \brief		Initialize library
 *
 *  This function initializes the CRI Movie library, including internal audio,
 *  streaming and video subsystems.  This function must be 
 *  called before any other function in this library will work properly.  
 *  This function initializes only the first time it is called; if it is
 *  called again, it simply increments an internal counter and returns; it 
 *  does not re-initialize, nor does it create an error condition.
 *
 *  Therefore, it is safe to call criMvPly_Initialize() and criMvPly_Finalize()
 *  at the beginning and end, respectively, within each of the independent
 *  modules in your program.  If you match these functions call for call,
 *  only the first criMvPly_Initialize() function and the last criMvPly_Finalize()
 *  functions should have any effect.
 *
 *  \sa criMvPly_Finalize()
 */
void CRIAPI criMvPly_Initialize(void);

/* ���C�u�����I�� */
/*
 *  \brief		���C�u�����I��
 *	\param		�Ȃ�
 *	\return		�Ȃ�
 *	\par ����:
 *	���C�u�����̏I���������s���܂��B<BR>
 *	�����񏉊��������Ă����ꍇ�́A�����񐔂����I�����������s���Ă��������B
 */
/*
 * \ingroup MODULE_INIT
 *  \brief		Finalize library
 *	
 *	This function deallocates any resources in use by the library.
 *  It should be called the same number of times that the criMvPly_Initialize() 
 *  function is called.  If the criMvPly_Initialize() function is called <i>n</i>
 *  times, on the <i>n</i>th time the criMvPly_Finalize() function is called,
 *  criMvPly_Finalize() releases any resources allocated by CRI Movie.
 *
 *  Therefore, it is safe to call criMvPly_Initialize() and criMvPly_Finalize()
 *  at the beginning and end, respectively, within each of the independent
 *  modules in your program.  If you match these functions call for call,
 *  only the first criMvPly_Initialize() function and the last criMvPly_Finalize()
 *  functions should have any effect.
 *
 *  \sa criMvPly_Initialize()
 */
void CRIAPI criMvPly_Finalize(void);

/* �n���h���쐬 */
/*
 *  \brief		�n���h���쐬
 *  \param		heap : �������m�ۂɎg�p����Heap�n���h��
 *	\return		CriMvPly�n���h��
 *	\par ����:
 *	CRI Movie �n���h�����쐬���܂��B<BR>
 *	�K�v�ȃn���h���Ǘ��̈��Heap�n���h�����g���Ď����I�Ɋm�ۂ��܂��B<BR>
 *	�쐬����̃n���h����Ԃ�STOP��Ԃł��B
 */
/*
 *  \ingroup MODULE_INIT
 *  \brief		Create a handle
 *  \param		heap A valid CriHeap handle
 *	\return		A valid CriMvPly handle, or NULL if the handle cannot be allocated
 *	
 *  This function creates a CriMvPly handle in the CRIMVPLY_STATUS_STOP state.
 * Memory for the handle is allocated from the CriHeap structure that you provide.
 * Any memory allocation failure during this function results in a text error message to output, and the program hangs.
 * Make sure to initialize and create your heap with criHeap_Initialize() and
 * criHeap_Create() before calling this function.
 *
 * \sa CriMvPly, CriMvPlyStatus, criHeap_Initialize(), criHeap_Create()
 */
CriMvPly CRIAPI criMvPly_Create(CriHeap heap);

/* �R���t�B�O�w��̃n���h���쐬 */
/* config ��NULL�w��̏ꍇ�̓R���t�B�O�w�薳���Ɠ��l */
CriMvPly CRIAPI criMvPly_CreateWithConfig(CriHeap heap, CriMvHandleConfig *config);

/* �n���h���j�� */
/*
 *  \brief		�n���h���j��
 *  \param		mvply : CRI Movie �n���h��
 *	\return		�Ȃ�
 *	\par ����:
 *	CRI Movie �n���h���̔j�����s���܂��B<BR>
 *	�n���h���쐬���Ɉ����Ŏw�肵��Heap�n���h�����g���āA�n���h���Ǘ��̈��������܂��B<BR>
 *	���[�N�o�b�t�@���m�ۂ����܂܂̏ꍇ�́A���[�N�o�b�t�@�m�ێ��Ɏw�肵��Heap�n���h�����g���ă��[�N�o�b�t�@��������܂��B<BR>
 *	�n���h���̔j���́A�n���h����Ԃ�STOP��PLAYEND�̎��ɂ������s�ł��܂���B
 */
/*
 *  \ingroup MODULE_INIT
 *  \brief		Destroy a handle
 *  \param		mvply A valid CriMvPly handle to be destroyed
 *	
 *	This function destroys the CriMvPly handle previously created
 *  with criMvPly_Create().
 *
 *  You can only destroy the handle if it is in either the CRIMVPLY_STATUS_STOP or the
 *  the CRIMVPLY_STATUS_PLAYEND states.  Attempting to destroy the handle in
 *  any other state will produce an error message.  You can check the status of
 * the handle at any time with criMvPly_GetStatus().
 *
 *  Any work buffers allocated via criMvPly_AllocateWorkBuffer(), if they are
 * still associated with the handle, are freed when criMvPly_Destroy() is called.
 *
 * \sa CriMvPly, CriMvPlyStatus, criMvPly_GetStatus(), criMvPly_AllocateWorkBuffer()
 */
void CRIAPI criMvPly_Destroy(CriMvPly mvply);

/* �X�g���[�~���O�p�����[�^�̎擾 */
/*
 *  \brief		�X�g���[�~���O�p�����[�^�̎擾
 *  \param		mvply : CRI Movie �n���h��
 *  \param		stmprm : �X�g���[�~���O�p�����[�^
 *	\return		�Ȃ�
 *	\par ����:
 *	�w�b�_��͂̌��ʂ����ƂɁA���[�r�Đ��ɕK�v�ȃX�g���[�~���O�p�����[�^���擾���܂��B<BR>
 *	�n���h����Ԃ�WAIT_PREP�ɂȂ�Ǝ擾�ł���悤�ɂȂ�܂��B<BR>
 *	���̃p�����[�^�����Ƃ�criMvPly_AllocateWorkBuffer�֐����Ăяo�����Ƃ��ł��܂��B<BR>
 *	�K�v�Ȃ�΂��̃p�����[�^�̒l��ύX���āA�Ⴆ�Ή����胀�[�r�ŉ����Đ����Ȃ��A�Ƃ��������Ƃ��\�ł��B<BR>
 */
/*
 *  \ingroup MODULE_BUFFER
 *  \brief		Get streaming parameters
 *  \param		mvply A valid CriMvPly handle
 *  \param		stmprm An empty CriMvStreamingParameters structure to be filled with data
 *	
 * This function permits you to get detailed information about the stream and
 * dynamically allocate resources  just before the video and audio sequence
 * begins playback.
 *
 * This function does nothing if the current state of the CriMvPly handle is 
 * CRIMVPLY_STATUS_DECHDR or CRIMVPLY_STATUS_STOP.  The only useful state in
 * which to call criMvPly_GetStreamingParameters() is the CRIMVPLY_WAIT_PREP
 * status.  When the CriMvPly handle is in the CRIMVPLY_WAIT_PREP status,
 * calling this function will cause the CriMvStreamingParameters field to be
 * filled with data.
 *
 * Some of the CriMvStreamingParameters, such as buffering time and the 
 * maximum number of files to read, are copied from the CriMvPly structure.
 * However, maximum bitrate, video size, audio stream rate and channel
 * info are calculated from the incoming stream.
 *
 * After calling criMvPly_GetStreamingParameters(), you can programmatically 
 * override any of the fields in the CriMvStreamingParameters struct yourself
 * before calling criMvPly_AllocateWorkBuffer() with it.  For example, you might
 * need to read a stream containing both audio and video, but only output the
 * video from the stream.  In this case you could allocate trivial audio buffers 
 * for output by modifying the CriMvStreamingParameters struct accordingly after
 * calling this function.
 *
 * \sa criMvPly_AllocateWorkBuffer(), CriMvPly, CriMvStreamingParameters
 */
 void CRIAPI criMvPly_GetStreamingParameters(CriMvPly mvply, CriMvStreamingParameters *stmprm);

/* ���[�N�o�b�t�@�̊m�� */
/*
 *  \brief		���[�N�o�b�t�@�̊m��
 *  \param		mvply : CRI Movie �n���h��
 *  \param		heap : 
 *  \param		stmprm : �X�g���[�~���O�p�����[�^
 *	\return		�Ȃ�
 *	\par ����:
 *	������Heap�n���h�����g���āA�ǂݍ��݃o�b�t�@��r�f�I�^�I�[�f�B�I�̃��[�N�o�b�t�@���m�ۂ��܂��B<BR>
 *	���̊֐����Ăяo���\�Ȃ̂́ASTOP��Ԃ�WAIT_PREP��Ԃ̎��݂̂ł��B<BR>
 *	�����n���h���ɑ΂��ĂQ�x�Ăяo���ƁA�P�x�ڂ̃��[�N�o�b�t�@��S�ĉ�����Ă���A���߂ă��[�N�o�b�t�@���m�ۂ��܂��B<BR>
 *	criMvPly_Start�֐�������Ƀ��[�N�o�b�t�@���m�ۂ��Ă������Ƃ��ł��܂��B
 */
/*
 *  \ingroup MODULE_BUFFER
 *  \brief		Allocate internal streaming work buffers
 *  \param		mvply A CriMvPly handle
 *  \param		heap A CriHeap handle
 *  \param		stmprm An initialized CriMvStreamingParameters structure
 * 
 * This function allocates internal streaming buffers for the CriMvPly movie
 * player from the CriHeap.  The amount of memory required is based
 * on the maximum bitrate of the stream, the requested buffering time, the
 * maximum chunk size, and the height and width of the incoming video frame.
 * However, a small amount of memory is allocated for the
 * video and audio decoders from the CriHeap as well.
 *
 * The CriHeap handle passed as a parameter to this function need not 
 * be the same CriHeap handle you passed to the criMvPly_Create() function.
 * You may prefer to use either one or two heaps.
 *
 * This function can be called only if the CriMvPly handle is in the
 * CRIMVPLY_STATUS_STOP or the CRIMVPLY_STATUS_WAIT_PREP status.  Calling
 * this function any other time will produce an error message.
 * 
 * If this function is called twice without calling criMvPly_FreeWorkBuffer(),
 * it releases the previously allocated buffers before allocating them again.
 *
 * This function must be called sometime before criMvPly_Start().
 *
 * \if ps2
 * \par PS2 only:
 * The PS2 implementation of this function additionally allocates buffers for
 * internal DMA tags.  If these allocations fail due to lack of memory, the
 * library will hang.
 * \endif
 *
 * \sa CriMvPly, CriHeap, CriMvPlyStatus, CriMvStreamingParameters, criMvPly_FreeWorkBuffer(),
 * criMvPly_Start()
 */
CriBool CRIAPI criMvPly_AllocateWorkBuffer(CriMvPly mvply, CriHeap heap, CriMvStreamingParameters *stmprm);

/* ���[�N�o�b�t�@�̉�� */
/*
 *  \brief		���[�N�o�b�t�@�̉��
 *  \param		mvply : CRI Movie �n���h��
 *	\return		�Ȃ�
 *	\par ����:
 *	criMvPly_AllocateWorkBuffer�֐��Ŋm�ۂ������[�N�o�b�t�@��S�ĉ�����܂��B<BR>
 *	���̊֐����Ăяo���\�Ȃ̂́ASTOP�^WAIT_PREP�^PLAYEND��Ԃ̎��݂̂ł��B<BR>
 *	CRI Movie Ver.0.60 �ł͖������ł��B
 */
/*
 * \ingroup MODULE_BUFFER
 *  \brief		Release streaming work buffer
 *  \param		mvply A valid CriMvPly handle
 *	
 * This function releases streaming work buffers allocated from the CriHeap
 * previously associated with criMvPly_AllocateWorkBuffer().
 * This function should only be called at CRIMVPLY_STATUS_STOP,
 * CRIMVPLY_STATUS_WAIT_PREP or CRIMVPLY_STATUS_PLAYEND states.  However, this
 * function does not verify the current stream status before releasing
 * all the buffers; it merely deallocates them.  Expect interesting crashes
 * if you call this function while playing a movie.
 * 
 * You can verify the current CriMvPly handle status with criMvPly_GetStatus() if
 * necessary.
 *
 */
void CRIAPI criMvPly_FreeWorkBuffer(CriMvPly mvply);

/* �Đ�����I�[�f�B�I�`���l���̐ݒ� */
/*
 *  \brief		�Đ�����I�[�f�B�I�`���l���̐ݒ�
 *  \param		mvply : CRI Movie �n���h��
 *  \param		ch : �I�[�f�B�I�`���l���ԍ�
 *	\return		�Ȃ�
 *	\par ����:
 *	CriMvStreamingParameters�\���̂̃����o track_play_audio�̃f�t�H���g�l��ݒ肵�܂��B<BR>
 *	criMvPly_GetStreamingParameters�֐���CriMvStreamingParameters�\���̂��擾�����Ƃ��ɂ��̒l���i�[����܂��B<BR>
 *	�����ݒ肵�Ă��Ȃ��ꍇ�Abuffering_time�ɂ�0�������܂��B
 *	-1���w�肷��ƃI�[�f�B�I���Đ����Ȃ��ݒ�ɂȂ�܂��B
 */
void CRIAPI criMvPly_SetAudioTrack(CriMvPly mvply, CriSint32 track);

/* �o�b�t�@�����O����(�P��[sec])�̐ݒ� */
/*
 *  \brief		�o�b�t�@�����O����(�P��[sec])�̐ݒ�
 *  \param		mvply : CRI Movie �n���h��
 *  \param		time : �o�b�t�@�����O����
 *	\return		�Ȃ�
 *	\par ����:
 *	CriMvStreamingParameters�\���̂̃����obuffering_time�̃f�t�H���g�l��ݒ肵�܂��B<BR>
 *	criMvPly_GetStreamingParameters�֐��� CriMvStreamingParameters�\���̂��擾�����Ƃ��ɂ��̒l���i�[����܂��B<BR>
 *	�����ݒ肵�Ă��Ȃ��ꍇ�Abuffering_time�ɂ�1.0�b�������Ă��܂��B
 */
/*
 * \ingroup MODULE_BUFFER
 *  \brief		Set default buffering time (unit[sec])
 *  \param		mvply A valid CriMvPly handle
 *  \param		time Buffering time in seconds
 *	
 * This function tells the CriMvPly handle how much time of the stream to buffer in
 * memory.  Buffering is necessary to cover seeks, error retries, latency and
 * other various hiccups in most data sources.
 * 
 * This value is stored in the buffering_time field of the CriMvStreamingParameters
 * struct.  It is set to a default of 1.0 seconds when the CriMvPly handle is created.
 * This is typically safe for most DVD type file systems.
 *
 * \sa CriMvPly, CriMvStreamingParameters
 */
//void CRIAPI criMvPly_SetBufferingTime(CriMvPly mvply, CriFloat32 time);

/* �����ǂݍ��݃t�@�C�����̐ݒ� */
/*
 *  \brief		�����ǂݍ��݃t�@�C�����̐ݒ�
 *  \param		mvply : CRI Movie �n���h��
 *  \param		max_stm : �����ǂݍ��݃t�@�C����
 *	\return		�Ȃ�
 *	\par ����:
 *	CriMvStreamingParameters�\���̂̃����omax_simultaneous_read_files�̃f�t�H���g�l��ݒ肵�܂��B<BR>
 *	criMvPly_GetStreamingParameters�֐��� CriMvStreamingParameters�\���̂��擾�����Ƃ��ɂ��̒l���i�[����܂��B<BR>
 *	�����ݒ肵�Ă��Ȃ��ꍇ�Amax_simultaneous_read_files�ɂ�1�������Ă��܂��B
 */
/*
 * \ingroup MODULE_BUFFER
 *  \brief		Set  maximum number of simultaneous streams
 *  \param		mvply A valid CriMvPly handle
 *  \param		max_stm The maximum number of simultaneous streams
 *	
 *	This function sets the default value of the "max_simultaneous_read_files" field of
 * the CriMvStreamingParameters struct.  Currently, setting this value has no effect.
 */
//void CRIAPI criMvPly_SetMaxSimultaneousStreams(CriMvPly mvply, CriUint32 max_stm);

/* �T�E���h�o�̓o�b�t�@�T���v�����̐ݒ� */
/*
 *  \brief		GetWave16�ŗv������ő�T���v�����̐ݒ�
 *  \param		mvply : CRI Movie �n���h��
 *  \param		max_smpl : �ő�T���v����
 *	\return		�Ȃ�
 *	\par ����:
 *	CriMvAudioParameters�\���̂̃����ooutput_buffer_samples�̃f�t�H���g�l��ݒ肵�܂��B<BR>
 *	criMvPly_GetStreamingParameters�֐��� CriMvStreamingParameters�\���̂��擾�����Ƃ��ɂ��̒l���i�[����܂��B<BR>
 *	�����ݒ肵�Ă��Ȃ��ꍇ�Aoutput_buffer_samples�ɂ�16*1024�������Ă��܂��B
 */
/*
 *  \ingroup MODULE_AUDIO
 *  \brief		Set default sound output buffer samples
 *  \param		mvply A valid CriMvPly handle
 *  \param		smpls : sound output buffer samples
 *	
 *	This function sets the default value of the "output_buffer_samples" field in the
 * CriMvAudioParameters struct.  The default value is 16384, which is set when
 * criMvPly_Create() is called.
 *
 *  This function only has an effect if it is called before the
 * criMvPly_AllocateWorkBuffer() function is called, since this is when the audio
 * output buffer is allocated.
 *
 * \sa CriMvAudioParameters, criMvPly_Create(), criMvPly_AllocateWorkBuffer(),
 * criMvPly_GetWave16()
 */
//void criMvPly_SetMaxSamplesOfGetWave16(CriMvPly mvply, CriUint32 max_smpl);
void CRIAPI criMvPly_SetSoundOutputBufferSamples(CriMvPly mvply, CriUint32 smpls);

/* �n���h����Ԃ̎擾 */
/*
 *  \brief		�n���h����Ԃ̎擾
 *  \param		mvply : CRI Movie �n���h��
 *	\return		�n���h�����
 *	\par ����:
 *	�n���h����Ԃ��擾���܂��B
 */
/*
 *  \ingroup MODULE_STATE
 *  \brief		Get the handle status
 *  \param		mvply A valid CriMvPly handle
 *	\return		One of the CriMvPlyStatus enum values
 *	
 *	This function gets the current status of the CRI Movie handle.  Check the
 * following link for possible return values.
 *
 * \sa CriMvPlyStatus
 */
CriMvPlyStatus CRIAPI criMvPly_GetStatus(CriMvPly mvply);

/* WAIT��Ԃ��玟�̏�Ԃւ̑J�ڒʒm */
/*
 *  \brief		WAIT��Ԃ��玟�̏�Ԃւ̑J�ڒʒm
 *  \param		mvply : CRI Movie �n���h��
 *	\return		�Ȃ�
 *	\par ����:
 *	�n���h����Ԃ�WAIT_**** ��Ԃ��玟�̏�ԂɑJ�ڂ����܂��B��Ԃɉ����Ď��̂悤�Ɏg�p���܂��B<BR>
 *	�EWAIT_PREP ��� : criMvPly_AllocateWorkBuffer�֐��Ń��[�N���m�ۂ����������Ăяo���Ă��������B<BR>
 *	�EWAIT_PLAYING ��� : �r�f�I�t���[���A�I�[�f�B�I�f�[�^���擾���ĕ\���E�o�͂̏������ł�����A
 *							�o�͂��J�n���āA�{�֐����Ăяo���Ă��������B<BR>
 *	�EWAIT_PLAYEND ��� : �Ō�̃r�f�I�t���[���̕\���A�Ō�̃I�[�f�B�I�f�[�^�̏o�͂��I��������Ăяo���Ă��������B<BR>
 *	�EWAIT_STOP ��� : �r�f�I��I�[�f�B�I�̏o�͂���~���Ă�������ԂɂȂ�����A�Ăяo���Ă��������B<BR>
 *	�{�֐����Ăяo���Ɗe��Ԃ͑����Ɏ��̏�ԂɑJ�ڂ��܂��B<BR>
 *	�{�֐��� WAIT_**** �ȊO�̏�ԂŌĂяo���Ă��A��Ԃ͉����ς��܂���B
 */
/*
 *  \ingroup MODULE_STATE
 *  \brief		Notify transition from WAIT status
 *  \param		mvply A valid CriMvPly handle
 *	
 *	This function notifies the CriMvPly handle that your application is ready to
 * go from the current WAIT state to the next state.  There are exactly four states
 * in which it is appropriate to call this function:
 * 
 * - CRIMVPLY_STATUS_WAIT_PREP After your application has allocated buffers with
 *		criMvPly_AllocateWorkBuffer()
 * - CRIMVPLY_STATUS_WAIT_PLAYING After your application has prerolled stream data (if
 * 		necessary)
 * - CRIMVPLY_STATUS_WAIT_PLAYEND After your application has displayed the last frames of audio and
 *		video from the stream
 * - CRIMVPLY_STATUS_WAIT_STOP After your application suspends playback from the stream
 *
 * You can check the current status of the CriMvPly handle by calling the criMvPly_GetStatus()
 * function.  This function has no effect if called in states other than those listed above.
 *
 * \sa CriMvPly, CriMvPlyStatus
 */
void CRIAPI criMvPly_IncrementState(CriMvPly mvply);

/* ��Ԃ̍X�V */
/*
 *  \brief		CriMvPly���W���[���̃T�[�o�֐�
 *  \param		mvply : CRI Movie �n���h��
 *	\return		�Ȃ�
 *	\par ����:
 * ��Ƀf�}���`�v���N�T�����̃f�[�^�̍X�V���s���܂��B<br>
 *	�{�֐��̓A�v���P�[�V�����̃��C���X���b�h���łŖ���Ăяo���悤�ɂ��Ă��������B<BR>
 */
void CRIAPI criMvPly_Update(CriMvPly mvply);

/* �Đ��J�n */
/*
 *  \brief		�Đ��J�n
 *  \param		mvply : CRI Movie �n���h��
 *	\return		�Đ��J�n�ł����ꍇ��CRI_TRUE, ���s�����ꍇ��CRI_FALSE
 *	\par ����:
 *	�Đ��̂��߂̏������J�n���܂��B<BR>
 *	�{�֐��ďo����A�n���h����Ԃ�DECHDR�ɑJ�ڂ��܂��B<BR>
 */
/*
 *  \ingroup MODULE_STATE
 *  \brief		Start of playback processing
 *  \param		mvply A valid CriMvPly handle
 *	
 *  This function initiates playback processing.  This function should be called
 * after the CriMvPly handle is created and the data source has been opened,
 * but before the work buffers are allocated with criMvPly_AllocateWorkBuffer().
 * This function sets the current status of the CriMvPly handle to 
 * CRIMVPLY_STATUS_DECHDR, which prepares it to decode the header information
 * from the data source.
 *
 * \sa CriMvPly, CriMvPlyStatus, criMvPly_AllocateWorkBuffer()
 */
CriBool CRIAPI criMvPly_Start(CriMvPly mvply);

/* �Đ���~���N�G�X�g(�������A) */
/*
 *  \brief		�Đ���~���N�G�X�g(�������A)
 *  \param		mvply : CRI Movie �n���h��
 *	\return		�Ȃ�
 *	\par ����:
 *	�Đ���~�̃��N�G�X�g�𔭍s���đ������A���܂��B<BR>
 *	�{�֐��ďo����A�n���h����Ԃ�STOP_PROCESSING��ԂɑJ�ڂ��܂��B<BR>
 *	��~�̂��߂̏������I���ƁA�n���h����Ԃ�WAIT_STOP�ɑJ�ڂ��܂��B<BR>
 *	WAIT_STOP��ԂɂȂ�����AcriMvPly_IncrementState�֐���STOP��ԂɑJ�ڂ����āA
 *	�A�v���P�[�V�����̒�~�������s���Ă��������B
 */
/*
 *  \ingroup MODULE_STATE
 *  \brief		Non-blocking request to stop playback
 *  \param		mvply A currently playing CriMvPly handle
 *	
 *	This function records a request to terminate playback.  Termination of 
 * playback is not synchronous to this function; this function sets the current
 * state of the CriMvPly handle to CRIMVPLY_STATUS_STOP_PROCESSING.  After
 * movie processing is halted, the state of the handle transitions to
 * CRIMWPLY_STATUS_WAIT_STOP.
 *
 *	This function is useful for prematurely terminating a movie, e.g. "press
 * X to skip this movie".
 *
 * Video frames will keep being delivered until you detect a CRIMVPLY_STATUS_WAIT_STOP
 * state in the CriMvPly handle, and then call criMvPly_IncrementState to transition
 * back to the CRIMVPLY_STATUS_STOP state.
 *
 * \note Pausing is not accomplished through this function.  The system clock, including
 * whether or not to pause or advance frames, is controlled entirely through user
 * code.  So the effect of "pausing" a CriMvPly handle can be accomplished by simply
 * not updating your system clock as long as your pause is in effect.
 *
 * \sa CriMvPly, CriMvPlyStatus
 */
void CRIAPI criMvPly_Stop(CriMvPly mvply);

/* �T�[�o����(�n���h���w��) */
/*
 *  \brief		�T�[�o����(�n���h���w��)
 *  \param		mvply : CRI Movie �n���h��
 *	\return		�Ȃ�
 *	\par ����:
 *	CRI Movie �n���h�����w�肵�ăT�[�o���������s���܂��B<br>
 *	�eWAIT_**** ��Ԃւ̏�ԑJ�ڂ̓T�[�o�֐����Ŏ��s����܂��B
 */
/*
 *  \ingroup MODULE_VIDEO
 *  \brief		Execute heartbeat functions for a handle
 *  \param		mvply A valid CriMvPly handle
 *	
 *	This function executes heartbeat functions for the specified CriMvPly handle only,
 * including handoff and parsing of input buffers and audio decoding.  Additionally,
 * it checks for buffer situations in which the CriMvPly handle should transition to 
 * one of the four WAIT states of CriMvPlyStatus, and it makes these transitions if
 * necessary.
 *
 * However, video decoding does NOT occur in criMvPly_Execute().
 * 
 * Expect that criMvPly_Execute() will take a relatively low CPU load. Typically,
 * this function should be called on every vertical blank.  However, it may be called
 * more frequently in a CriMvPly wait state, in conjunction with criMvPly_IncrementState(),
 * in order to "force" a transition into the next state without waiting for 
 * another vertical blank.  This type of transition is not generically
 * necessary.
 * 
 * \sa CriMvPly, CriMvPlyStatus, criMvPly_ExecuteAll()
 */
void CRIAPI criMvPly_Execute(CriMvPly mvply);

/* �t�@�C���ǂݍ��݃o�b�t�@�̋󂫃`�����N�擾 */
/*
 *  \brief		�t�@�C���ǂݍ��݃o�b�t�@�̋󂫃`�����N�擾
 *  \param		mvply : CRI Movie �n���h��
 *  \param		ck : �`�����N
 *	\return		�Ȃ�
 *	\par ����:
 *	�t�@�C���ǂݍ��݃o�b�t�@�̋󂫗̈���擾���܂��B<BR>
 *  �擾�����󂫗̈�i�`�����N�ƌĂт܂��j�́A�f�[�^�������݂�ʒm���邳����
 *  ���̂܂܎g�p���܂��̂ŁA�A�v���P�[�V�����ŋL�����Ă��������B<BR>
 *	�P�x�Ɏ擾�ł���`�����N�͂P�݂̂ł��B<BR>
 *  �`�����N���擾�ł������ۂ��́A�`�����N�̃T�C�Y�Ŕ���ł��܂��B<BR>
 *	�f�[�^�̏������݂��I�������AcriMvPly_PutInputChunk�֐��ŏ������݃T�C�Y��ʒm���Ă��������B<BR>
 */
/*
 *  \ingroup MODULE_SUPPLY
 *  \brief		Get a free chunk from file reading buffer
 *  \param		mvply A valid CriMvPly handle
 *  \param		ck A CriChunk structure to be filled with data by this function
 *
 *  This function selects an empty internal buffer for your data source to read its data into.
 * An area of this type is referred to as a "chunk."
 * After calling this function, the ck->data and ck->size
 * fields will provide a valid pointer and size, respectively, that your data 
 * source should copy its data into.
 * If no buffers can internally be allocated, this function will return 0 as the
 * ck->size field.  If this occurs, your program should choke input until a free
 * buffer can be allocated.
 * Typical data sources are native file reading, sequential memory access, or
 * playback from a network source.
 * The ck->size field is dynamically calculated when criMvPly_AllocateWorkBuffer() 
 * is called; it is calculated based on expected data rate and video resolutions 
 * embedded in the stream file.
 *
 * After you receive a valid chunk from criMvPly_GetInputChunk(),
 * you can fill the provided chunk up to the ck->size limit.  After you 
 * fill the chunk with valid data, call the criMvPly_PutInputChunk() function
 * to queue the data for processing. The functions criMvPly_GetInputChunk() and
 * criMvPly_PutInputChunk() should be called in equal pairs; calling these
 * functions out of order will produce odd results.  No dynamic chunk reordering
 * is permitted; serial calls with out-of-order chunks will be
 * flagged at run-time as an error.
 *
 * This function will only return a valid chunk if the CriMvPly handle is in
 * one of two playback states: CRIMVPLY_STATUS_PLAY or CRIMVPLY_STATUS_DECHDR.
 * You can verify the current playback state with criMvPly_GetStatus().
 * 
 * \sa criMvPly_GetStatus(), criMvPly_PutInputChunk(), criMvPly_AllocateWorkBuffer(), 
 * CriChunk, CriMvPlyStatus
 */
void CRIAPI criMvPly_GetInputChunk(CriMvPly mvply, CriChunk *ck);

/* �t�@�C���ǂݍ��݃o�b�t�@�ւ̃f�[�^�������ݒʒm */
/*
 *  \brief		�t�@�C���ǂݍ��݃o�b�t�@�ւ̃f�[�^�������ݒʒm
 *  \param		mvply : CRI Movie �n���h��
 *  \param		ck : 
 *  \param		inputsize : 
 *	\return		�Ȃ�
 *	\par ����:
 *	criMvPly_GetInputChunk�֐��Ŏ擾�����󂫗̈�i�`�����N�ƌĂт܂��j��
 *  �f�[�^���������ݏI�������A����inputsize�Ƀf�[�^�T�C�Y�����Ė{�֐����Ăяo���Ă��������B<BR>
 *  ���̍ہA�`�����N�� criMvPly_GetInputChunk�֐��Ŏ擾�������̂Ɠ����`�����N��K���w�肵�Ă��������B
 */
/*
 *  \ingroup MODULE_SUPPLY
 *  \brief		Put a data chunk into file read buffer
 *  \param		mvply A valid CriMvPly handle
 *  \param		ck A CriChunk structure containing source data
 *  \param		inputsize The number of bytes actually supplied
 *
 *  This function informs the CriMvPly handle that the CriChunk structure
 * now contains valid data from the data source.  Typically, you would call
 * this function after your asynchronous file read reports that the buffer is 
 * full of data.
 *
 * The inputsize field should contain the number of bytes actually provided.
 * This value can be less than or equal to ck->size.  In an end-of-file condition,
 * be sure to supply the actual number of bytes remaining in the file, and not
 * merely the size of the input buffer, to criMvPly_PutInputChunk().
 * Do not modify the contents of the CriChunk after calling this function;
 * instead, call criMvPly_GetInputChunk() to get a new chunk for further input.
 *
 * This function invalidates the CriChunk provided if the function is called
 * while the CriMvPly handle is in the CRIMVPLY_STATUS_STOP, the 
 * CRIMVPLY_STATUS_WAIT_PREP, or the CRIMVPLY_STATUS_STOP_PROCESSING state.
 * You can verify the current playback state with criMvPly_GetStatus().
 *
 * After you receive a valid chunk from criMvPly_GetInputChunk(),
 * you can fill the provided chunk up to the ck->size limit.  After you call
 * fill the chunk with valid data, call the criMvPly_PutInputChunk() function
 * to queue the data for processing. The function criMvPly_GetInputChunk() and
 * criMvPly_PutInputChunk() should be called in equal pairs; calling these
 * functions out of order will produce odd results.  No dynamic chunk reordering
 * is permitted; serial calls with out-of-order chunks will be
 * flagged at run-time as an error.
 *
 * After putting the final chunk of data in the stream, call
 * criMvPly_TerminateSupply() to indicate that an end-of-file condition exists.
 * 
 *  \sa criMvPly_GetStatus(), criMvPly_GetInputChunk(), criMvPly_AllocateWorkBuffer(), 
 * criMvPly_TerminateSupply(), CriChunk, CriMvPlyStatus
 */
void CRIAPI criMvPly_PutInputChunk(CriMvPly mvply, CriChunk *ck, CriUint32 inputsize);

/* �t�@�C���ǂݍ��ݏI���̒ʒm */
/*
 *  \brief		�t�@�C���ǂݍ��ݏI���̒ʒm
 *  \param		mvply : CRI Movie �n���h��
 *	\return		�Ȃ�
 *	\par ����:
 *	�Đ��������S�Ẵf�[�^��ǂݍ���ŁA criMvPly_PutInputChunk�֐��Œʒm���I�������A
 *	�{�֐��Ńt�@�C���ǂݍ��ݏI���̒ʒm��K���s���Ă��������B<BR>
 *	�I����ʒm���ꂽ���_�œǂݍ��݃o�b�t�@�ɏ������܂ꂽ�S�Ẵf�[�^���f�R�[�h���I���ƁA
 *	�n���h����Ԃ�WAIT_PLAYEND�ɑJ�ڂ��܂��B<BR>
 *	�{�֐����Ăяo���Ȃ�����AWAIT_PLAYEND��ԂɂȂ邱�Ƃ͂���܂���B
 */
/*
 *  \ingroup MODULE_SUPPLY
 *  \brief		Notify end of reading data
 *  \param		mvply A valid CriMvPly structure
 *	
 * After you put all the source data into the CriMvPly handle with
 * criMvPly_PutInputChunk(), indicate the end of the movie file by calling
 * criMvPly_TerminateSupply().  After calling this function, the CriMvPly
 * handle's status is changed by the library to CRIMVPLY_STATUS_WAIT_PLAYEND
 * and the library completes processing of whatever frames it has internally
 * buffered. 
 *
 * If you do not call this function, the CriMvPly handle will never transition
 * to the CRIMVPLY_STATUS_WAIT_PLAYEND state, making teardown impossible.
 *
 * \sa criMvPly_PutInputChunk(), CriMvPly, CriMvPlyStatus
 */
void CRIAPI criMvPly_TerminateSupply(CriMvPly mvply);

#if 0//defined(XPT_TGT_EE)
/* RGB32�t�H�[�}�b�g�̃r�f�I�t���[���̎擾 */
/*
 * \if ps2
 *  \brief		RGB32�t�H�[�}�b�g�̃r�f�I�t���[���̎擾
 *  \param		mvply : CRI Movie �n���h��
 *  \param		imagebuf : �r�f�I�t���[���o�b�t�@
 *  \param		bufsize : �o�b�t�@�T�C�Y
 *  \param		frameinfo : �t���[�����
 *	\return		�擾�ł����ꍇ��TRUE, �ł��Ȃ������ꍇ��FALSE
 *	\par ����:
 *	�����Ŏw�肵���o�b�t�@�ɁAPS2�̃}�N���u���b�N����RGBA32�t�H�[�}�b�g�Ńt���[�����擾���܂��B<BR>
 *	����CriMvFrameInfo�\���̂ɂ́A�擾�����t���[���ɂ��Ă̏�񂪊i�[����܂��B<BR>
 *	�t���[�����擾�ł���̂́A�n���h����Ԃ�WAIT_PLAYING�^PLAYING�̎��݂̂ł��B<BR>
 *	����ȊO�̏�ԂŌĂяo���A�܂��͓��̓f�[�^�s���̏ꍇ�ɂ́A�{�֐��̓t���[���̎擾�Ɏ��s���A�����ɕ��A���܂��B<BR>
 *	�t���[�����擾�ł��Ȃ������ꍇ�́A�֐��l��FALSE���Ԃ�܂��B<BR>
 *	���ۂ̃r�f�I�f�R�[�h�������{�֐����œ������߁A�t���[���擾�ł���ꍇ�ɂ́A�����̏d���֐��ƂȂ�܂��B
 * \endif
 */
/*
 * \if ps2
 *  \ingroup MODULE_VIDEO
 *  \brief		Get a video frame in PS2 macroblock RGB32 format
 *  \param		mvply A valid CriMvPly handle
 *  \param		imagebuf a pointer to the video buffer in memory to receive the frame
 *  \param		bufsize video buffer size in bytes
 *  \param		frameinfo Information about the decoded frame
 *	\return		returns true if a frame has been copied into the buffer, false otherwise
 *	
 *  This function finds and decodes the current frame, if any, to the imagebuf buffer.
 *  The format of this buffer is specific to the PS2.  This function will only return
 * a valid frame if the CriMvPly handle is in the CRIMVPLY_STATUS_WAIT_PLAYING or the
 * CRIMVPLY_STATUS_PLAYING state.  
 *
 * This function is an EE-intensive activity, and the amount of time required
 * is variable, depending on the complexity and size of the video frame being decoded.
 * Therefore the preferred method of calling this function is in a low priority thread,
 * separate from your I/O, buffer management, and criMvPly_Execute() routines.
 * 
 * If this function returns true, the preferred display time of the video frame is calculated
 * as:
 *
 * \code
 * frameinfo.time / frameinfo.tunit 
 * \endcode
 * 
 * Humans notice audio stuttering much more readily than a dropped video frame during
 * a video decode process.  If the frame reported by criMvPly_GetFrameRGBA32_PS2()
 * arrives after your system clock says the frame should be displayed, you should
 * simply drop the frame without bothering to DMA it to video memory.
 * 
 * Here is an example showing how to drop frames in this case:
 *
 * \dontinclude crimvt01_simple_playback_ps2.c
 * \skip Get video frame
 * \until *tutor_update_video_frame_on_display_time*
 * 
 * It is not preferred, but it is possible, to call this function in a single-threaded
 * playback model.  In this case, it is important to allocate more heap space
 * and service the CriMvPly handle with criMvPly_Execute() or criMvPly_ExecuteAll()
 * frequently, as well as criMvPly_GetWave16() frequently.  This helps to cover
 * for the case where other I/O needs to occur when a frame is currently being decoded by
 * criMvPly_GetFrameRGBA32_PS2().
 *
 * \image html crimvply_getframergba32_ps2.png The DMA reordering step on PS2
 * 
 * In order to get acceptable performance on the PS2, a macroblock reordering step
 * must take place during the DMA transfer from EE RAM to video RAM.
 * \endif
 */
CriBool CRIAPI criMvPly_GetFrameRGBA32_PS2(CriMvPly mvply, CriUint8 *imagebuf, CriUint32 bufsize, CriMvFrameInfo *frameinfo);

CriBool CRIAPI criMvPly_DecodeFrameRGBA32_PS2(CriMvPly mvply, CriUint8 *imagebuf, CriUint32 bufsize, CriMvFrameInfo *frameinfo);
#endif

#if defined(XPT_TGT_PC) || defined(XPT_TGT_XBOX360) || defined(XPT_TGT_WII) || defined(XPT_TGT_SH7269)  || defined(XPT_TGT_TRGP6K)
/*
 * \if pc
 *  \ingroup MODULE_VIDEO
 *  \brief		Get a video frame in YUV422 format
 *  \param		mvply A valid CriMvPly handle
 *  \param		imagebuf A pointer to the video buffer in memory to receive the frame
 *  \param      pitch Number of bytes in one row of the video buffer
 *  \param		bufsize Video buffer size in bytes
 *  \param		frameinfo Information about the decoded frame
 *	\return		returns true if a frame has been copied into the buffer, false otherwise
 *	
 *  This function finds and decodes the current frame, if any, to the imagebuf buffer.
 * This function will only return a valid frame if the CriMvPly handle is in the
 * CRIMVPLY_STATUS_WAIT_PLAYING or the CRIMVPLY_STATUS_PLAYING state.  
 *
 * This function is a CPU-intensive activity, and the amount of time required
 * is variable, depending on the complexity and size of the video frame being decoded.
 * Therefore the preferred method of calling this function is in a low priority thread,
 * separate from your I/O, buffer management, and criMvPly_Execute() routines.
 * 
 * If this function returns true, the preferred display time of the video frame is calculated
 * as:
 *
 * \code
 * frameinfo.time / frameinfo.tunit 
 * \endcode
 * 
 * It is not preferred, but it is possible, to call this function in a single-threaded
 * playback model.  In this case, it is important to allocate more heap space
 * and service the CriMvPly handle with criMvPly_Execute() or criMvPly_ExecuteAll()
 * frequently, as well as criMvPly_GetWave16() frequently.
 *
 * \endif
 */
CriBool CRIAPI criMvPly_GetFrameYUV422(CriMvPly mvply, CriUint8 *imagebuf, CriUint32 pitch, CriUint32 bufsize, CriMvFrameInfo *frameinfo);
#endif

/* 16bit WAVE�t�H�[�}�b�g�̃I�[�f�B�I�f�[�^�擾 */
/*
 *  \brief		16bit WAVE�t�H�[�}�b�g�̃I�[�f�B�I�f�[�^�擾
 *  \param		mvply : CRI Movie �n���h��
 *  \param		nch : �`���l����
 *  \param		waveptr : �I�[�f�B�I�f�[�^�o�b�t�@
 *  \param		wavesmpl : �v���T���v����(���o�b�t�@�T�C�Y)
 *  \param		waveinfo : 16bit Waveform ���
 *	\return		�擾�ł����T���v����
 *	\par ����:
 *	16bit��WAVE�t�H�[�}�b�g�ŃI�[�f�B�I�f�[�^���擾���܂��B����waveptr�ɂ́Anch���̃o�b�t�@�|�C���^���i�[����
 *	�z����w�肵�Ă��������B<BR>
 *	����CriMvWaveInfo�\���̂ɂ́A�擾�����I�[�f�B�I�f�[�^�ɂ��Ă̏�񂪊i�[����܂��B<BR>
 *	���̓f�[�^�s���Ȃǂŗv�����ꂽ�T���v�����̃f�R�[�h���ł��Ȃ��ꍇ������܂��B<BR>
 *	(�������@�\) �{�֐��̏������d���Ȃ��Ăł��A�Ȃ�ׂ��v�����ꂽ�����̃I�[�f�B�I�f�[�^���擾���郂�[�h�B
 */
/*
 *  \ingroup MODULE_AUDIO
 *  \brief		Get 16bit wave audio data
 *  \param		mvply A currently playing CriMvPly handle
 *  \param		nch The number of audio channels to get in this call
 *  \param		waveptr An array of audio data buffers to copy audio data into
 *  \param		wavesmpl The number of requested wave data samples (must be less than buffer size)
 *  \param		waveinfo A structure filled by this function with info about this wave
 *	\return		The number of wave data samples actually copied into the buffer
 *	
 *  This function copies currently decoding audio data into your output buffer
 * for you to send to the audio output.  The output format is a sixteen-bit PCM 
 * format.  The data provided is "current", e.g. you should try to minimize latency.
 * while delivering the audio data to the output device.
 * 
 * This function will return an empty audio buffer if the input to the CriMvPly
 * handle is starving for data, or if criMvPly_Execute() or criMvPly_ExecuteAll()
 * has not been called recently.
 *
 * Note that this function actually copies data.  However, the expected bandwidth
 * for moving audio data in memory is minimal -- about 176400 bytes per second for
 * a stereo stream, which is typically a fraction of 1% of the bandwidth available 
 * on modern game systems.
 *
 * For debugging tips on stuttering, see \ref crim_section_stuttering .
 */
CriUint32 CRIAPI criMvPly_GetWave16(CriMvPly mvply, CriUint32 nch, CriSint16 *waveptr[],
								  CriUint32 wavesmpl, CriMvWaveInfo *waveinfo);
CriUint32 CRIAPI criMvPly_GetWave32(CriMvPly mvply, CriUint32 nch, CriFloat32 *waveptr[],
								  CriUint32 wavesmpl, CriMvWaveInfo *waveinfo);


#if defined(XPT_TGT_EE)
/* �X�N���b�`�p�b�hRAM�g�p�ݒ� */
/* �X�N���b�`�p�b�hRAM�̎g�p�ݒ�(�n���h���쐬�O�ɌĂяo������) */
/*
 *  \brief		�X�N���b�`�p�b�hRAM�g�p�ݒ�
 *  \param		sw : 
 *	\return		�Ȃ�
 *	\par ����:
 *	�X�N���b�`�p�b�hRAM�̎g�p�ݒ���s���܂��B<BR>
 *  �f�t�H���g��OFF�ł��B
 */
/*
 * \if ps2
 *  \ingroup MODULE_INIT
 *  \brief		Enable or disable PS2 scratch pad (SPRAM) usage
 *  \param		sw : ON to enable scratch pad use, OFF to disable
 *	
 *  This function determines whether the video decode step uses the PS2
 * SPRAM memory area for its work.  The performance of this library is 
 * increased by around 30% when using SPRAM; however, this use might
 * conflict with graphics engines that depend on exclusive access to SPRAM.
 * However, on the PS2, the video decode step occurs synchronously to the 
 * criMvPly_GetFrameRGBA32_PS2() function, so you can take appropriate external
 * locking measures to intelligently synchronize SPRAM utilization.
 * The value set by criMvPly_SetUseScratchPadRAM_PS2() is internally checked
 * exactly once, during the criMvPly_Create() step; calling this function
 * after criMvPly_Create() has no effect.  The default setting for this
 * function is OFF. 
 * \endif
 */
void CRIAPI criMvPly_SetUseScratchPadRAM_PS2(CriBool sw);
#endif


/* ����������̍Đ��J�n */
/*
 *  \brief		����������̍Đ��J�n
 *  \param		mvply : CRI Movie �n���h��
 *  \param		memptr : ��������̃��[�r�f�[�^�̐擪�A�h���X
 *  \param		memsize : ��������̃��[�r�f�[�^�̃T�C�Y
 *	\return		�Ȃ�
 *	\par ����:
 *	����������̃��[�r�Đ����J�n���܂��B<BR>
 *  �{�֐��̌Ăяo���O�ɁA���炩���߃��[�r�f�[�^�̑S�Ă���������ɓǂݍ���ł����Ă��������B
 */
/*
 *  \ingroup MODULE_STATE
 *  \brief		Start playback from movie file on memory
 *  \param		mvply A currently playing CriMvPly handle
 *  \param		memptr A address of movie file
 *  \param		memsize The size of movie file
 *
 *  Start playback from memory.<BR>
 *  Please read movie file to memory before playback.
 */
void CRIAPI criMvPly_StartMemory(CriMvPly mvply, CriUint8* memptr, CriUint32 memsize);


/*
 * YUV�ʃo�b�t�@�ւ̃t���[���擾
 */
/*
 * \if xbox360
 *  \ingroup MODULE_VIDEO
 *  \brief		Get a video frame to Y,U,V independently texture buffers
 *  \param		mvply A valid CriMvPly handle
 *  \param		yuvbuffers Information about Y,U,V independently texture buffers
 *  \param		frameinfo Information about the decoded frame
 *	\return		returns true if a frame has been copied into the buffer, false otherwise
 *	
 *  This function finds and decodes the current frame, if any, to the Y,U,V texture buffers.
 * This function will only return a valid frame if the CriMvPly handle is in the
 * CRIMVPLY_STATUS_WAIT_PLAYING or the CRIMVPLY_STATUS_PLAYING state.  
 * \endif
 */
CriBool CRIAPI criMvPly_GetFrameYUVBuffers(CriMvPly mvply, CriMvYuvBuffers *yuvbuffers, CriMvFrameInfo *frameinfo);


/*
 * �r�f�I�̃f�R�[�h
 */
/*
 *  \ingroup MODULE_VIDEO
 *  \param		mvply The movie player handle
 *	\return		returns the number of decoded frames
 * 
 */
CriUint32 CRIAPI criMvPly_DecodeVideo(CriMvPly mvply);

/*
 * �w�b�_�̃f�R�[�h
 */
/*
 *
 *  \ingroup MODULE_VIDEO
 *  \param		mvply The movie player handle
 *
 */
void CRIAPI criMvPly_DecodeHeader(CriMvPly mvply);

/*
 * �f�R�[�h�X�L�b�v�w��
 *	\par ����:
 * ���̊֐������s�����񐔂����A���̌�̃f�R�[�h���Ɏ����I�ɂP��B�s�N�`�����X�L�b�v����B<br>
 * �X�L�b�v�w�����o�������̃t���[������́A���ۂ̃X�L�b�v�����s����Ă��Ȃ��Ă�
 * �\�������̓X�L�b�v�������̂Ƃ��ĕ␳�����B
 */
/*
 *  \ingroup MODULE_VIDEO
 *  \param		mvply The movie player handle
 *	\return		returns the number of decoded frames
 * 
 * After calling SkipFrame function, Decoding function skip  B-picture. 
 * To avoid that application judge continuous wrong skip, After calling
 * SkipFrame function, next frame time will be adjusted.
 */
void CRIAPI criMvPly_SkipFrame(CriMvPly mvply);


#if 0
/*
 * YUVA8�t�H�[�}�b�g�̃t���[���擾
 */
/*
 * \if ps3
 *  \ingroup MODULE_VIDEO
 *  \brief		Get a video frame in YUVA8 format.
 *  \param		mvply A valid CriMvPly handle
 *  \param		imagebuf A pointer to the video buffer in memory to receive the frame
 *  \param      pitch Number of bytes in one row of the video buffer
 *  \param		bufsize Video buffer size in bytes
 *  \param		frameinfo Information about the decoded frame
 *	\return		returns true if a frame has been copied into the buffer, false otherwise
 *	
 *  This function finds and decodes the current frame, if any, to the imagebuf buffer.
 * This function will only return a valid frame if the CriMvPly handle is in the
 * CRIMVPLY_STATUS_WAIT_PLAYING or the CRIMVPLY_STATUS_PLAYING state.  
 * \endif
 */
CriBool CRIAPI criMvPly_GetFrameYUVA8_PS3(CriMvPly mvply, CriUint8 *imagebuf, CriUint32 pitch, CriUint32 bufsize, CriMvFrameInfo *frameinfo);
#endif

/*
 * ARGB8888�t�H�[�}�b�g�Ńt���[���擾
 */
/*
 * \if ps3
 *  \ingroup MODULE_VIDEO
 *  \brief		Get a video frame in ARGB8888 format.
 *  \param		mvply A valid CriMvPly handle
 *  \param		imagebuf A pointer to the video buffer in memory to receive the frame
 *  \param      pitch Number of bytes in one row of the video buffer
 *  \param		bufsize Video buffer size in bytes
 *  \param		frameinfo Information about the decoded frame
 *	\return		returns true if a frame has been copied into the buffer, false otherwise
 *	
 *  This function finds and decodes the current frame, if any, to the imagebuf buffer.
 * This function will only return a valid frame if the CriMvPly handle is in the
 * CRIMVPLY_STATUS_WAIT_PLAYING or the CRIMVPLY_STATUS_PLAYING state.  
 * \endif
 */
CriBool CRIAPI criMvPly_GetFrameARGB8888(CriMvPly mvply, CriUint8 *imagebuf, CriUint32 pitch, CriUint32 bufsize, CriMvFrameInfo *frameinfo);
void CRIAPI criMvPly_InitializeFrameARGB8888(void);


#if defined(XPT_TGT_IPHONE) || defined(XPT_TGT_WINMO) || defined(XPT_TGT_ANDROID) || defined(XPT_TGT_CQSH2A) || defined(XPT_TGT_ACRODEA) || defined(XPT_TGT_NACL) || defined(XPT_TGT_SH7269)  || defined(XPT_TGT_TRGP6K)
/*
 * RGB565�t�H�[�}�b�g�Ńt���[���擾
 */
/*
 *  \ingroup MODULE_VIDEO
 *  \brief		Get a video frame in RGB565 format
 */
CriBool CRIAPI criMvPly_GetFrameRGB565(CriMvPly mvply, CriUint8 *imagebuf, CriUint32 pitch, CriUint32 bufsize, CriMvFrameInfo *frameinfo);
void CRIAPI criMvPly_InitializeFrameRGB565(void);
#endif
	
/*
 * ���̃t���[���̏�񂾂��擾����
 */
/*
 *  \ingroup MODULE_VIDEO
 *  \brief		Get a information of next video frame (without actual video frame).
 */
CriUint8* CRIAPI criMvPly_GetNextFrameInfo(CriMvPly mvply, CriMvFrameInfo *frameinfo);

/*
 * ���̃t���[�����̂Ă�
 */
CriBool CRIAPI criMvPly_DiscardNextFrame(CriMvPly mvply, CriMvFrameInfo *frameinfo);

/*
 * �Đ������������(PREP����WAIT_PLAYING)�ɂȂ�܂łɒ�������t���[�����̎w��
 * ���̊֐����Ăяo���Ȃ���΁A�����t���[���� = �t���[���v�[����
 */
void CRIAPI criMvPly_SetNumberOfFramesForPrep(CriMvPly mvply, CriSint32 nframes);

/*
 *  For Debug use.
 */
void CRIAPI criMvPly_SetSeekPosition(CriMvPly mvply, CriSint32 seek_frame_id, CriSint32 video_gop_top_id);
void CRIAPI criMvPly_SetSeekAlphaPosition(CriMvPly mvply, CriSint32 alpha_gop_top_id);
void CRIAPI criMvPly_CalcSeekPosition(CriMvPly mvply, void *seektbl_ptr, CriUint32 seektbl_size, Sint32 frame_id, Uint64 *offset, Sint32 *gop_top_id);

/* for specific use */
/* �{�f�B�A�h���X�̐ݒ� */
void CRIAPI criMvPly_SetBodyData(CriMvPly mvply, const CriUint64Adr body_ptr, CriSint64 body_size);

/* ����SJ����уo�b�t�@�T�C�Y�̎擾�i�o�b�t�@�T�C�Y�A�����[�h�T�C�Y��NULL�w��ŏȗ��j */
CriSj CRIAPI criMvPly_GetInputSj(CriMvPly mvply, CriUint32 *buffer_size, CriUint32 *reload_threshold);

/* ���O���^�C�v�w��ɂ��C�x���g�|�C���g���̎擾 */
//Bool criMvPly_SearchEventPointByName(CriMvPly mvply, Char8 *cue_name, Sint32 type, CriMvEventPoint *eventinfo);
/* �C�x���g�|�C���g��񂩂�t���[��ID�ւ̕ϊ� */
//Sint32 criMvPly_CalcFrameIdFromCuePoint(CriMvPly mvply, CriMvEventPoint *eventinfo);

CriBool CRIAPI criMvPly_AttachSubAudio(CriMvPly mvply, CriHeap heap, CriUint32 track);
CriUint32 CRIAPI criMvPly_GetSubAudioWave16(CriMvPly mvply, CriUint32 nch, CriSint16 *waveptr[], CriUint32 wavesmpl, CriMvWaveInfo *waveinfo);
CriUint32 CRIAPI criMvPly_GetSubAudioWave32(CriMvPly mvply, CriUint32 nch, CriFloat32 *waveptr[], CriUint32 wavesmpl, CriMvWaveInfo *waveinfo);
void CRIAPI criMvPly_DetachSubAudio(CriMvPly mvply);

void CRIAPI criMvPly_GetSubtitle(CriMvPly mvply, CriUint8 *bufptr, CriUint32 bufsize, CriMvSubtitleInfo *info);
void CRIAPI criMvPly_GetNextSubtitleInfo(CriMvPly mvply, CriMvSubtitleInfo *info);

/* ���̓o�b�t�@�̃f�[�^�ʂ�����[byte] */
CriUint32 CRIAPI criMvPly_PeekInputBufferData(CriMvPly mvply);
/* ��������̃��[�r�����SJ�ɒǉ����� */
void CRIAPI criMvPly_AddInputMemory(CriMvPly mvply, CriUint8* memptr, CriUint32 memsize);


/* �擾�ł���I�[�f�B�I�f�[�^�̃T���v�����𒲂ׂ� */
CriUint32 CRIAPI criMvPly_GetDataSizeMainAudio(CriMvPly mvply, CriUint32 nch);
/* �擾�ł���I�[�f�B�I�f�[�^�̃T���v�����𒲂ׂ� */
CriUint32 CRIAPI criMvPly_GetDataSizeSubAudio(CriMvPly mvply, CriUint32 nch);

/* ���C���̃I�[�f�B�I�̍Đ����I�����Ă��邩�ǂ����𒲂ׂ� */
CriBool CRIAPI criMvPly_IsEndMainAudioPlayback(CriMvPly mvply);
/* �T�u�̃I�[�f�B�I�̍Đ����I�����Ă��邩�ǂ����𒲂ׂ� */
CriBool CRIAPI criMvPly_IsEndSubAudioPlayback(CriMvPly mvply);

/* ���C���I�[�f�B�I�����������ǂ����𒲂ׂ� (�f�R�[�h�����o�̓o�b�t�@��������) */
CriBool CRIAPI criMvPly_IsActiveMainAudioPlayback(CriMvPly mvply);

/* ���[�N�m�ۑO�ɐݒ�ύX���邱�� */
void CRIAPI criMvPly_SetPcmFormat(CriMvPly mvply, CriMvPcmFormat pcmfmt);

#if defined(XPT_TGT_PC)
/* [PC] �}���`�v���Z�b�T�̎w�� */
void CRIAPI criMvPly_SetProcessorParameters_PC(CriMvPly mvply, Sint32 thread_num, Uint32 *affinity_masks, Sint32 *priorities);
#endif

#if defined(XPT_TGT_WIIU)
/* <TEMP> */
void criMvPly_SetMultiCoreDecode_WIIU(CriBool sw);
#endif

#if defined(XPT_TGT_XBOX360)
/* [Xbox360] �}���`�v���Z�b�T�̎w�� */
void CRIAPI criMvPly_SetProcessorParameters_XBOX360(CriMvPly mvply, Sint32 thread_num, CriUint32 processor_mask, CriSint32 *priorities);
#endif

#if defined(XPT_TGT_PS3PPU)
/* [PS3] SPURS�̎w�� */
void CRIAPI criMvPly_SetupSpursParameters_PS3(CriMvProcessorParameters_PS3 *processor_param);
	/* [PS3] SPU�X���b�h�ɂ��}���`�v���Z�b�T�̎w�� */
void CRIAPI criMvPly_SetupSpuThreadParameters_PS3(CriMvSpuThreadParameters_PS3 *spu_thread_param);
#endif

#if defined(XPT_TGT_PS3PPU)
void CRIAPI criMvPly_SetGraphicEnv(CriMvGraphicEnv env);
CriMvGraphicEnv CRIAPI criMvPly_GetGraphicEnv(void);
#endif

/* �t���[���v�[�����̎擾 */
void CRIAPI criMvPly_GetFramePoolInfo(CriMvPly mvply, CriSint32 *num_input, CriUint32* num_data, CriUint32* num_ref, CriUint32* num_hold, CriUint32* num_free);

/* �Đ����ł������`���l����؂�ւ��� */
void CRIAPI criMvPly_SetSubtitleChannel(CriMvPly mvply, CriSint32 chno);

/* �|�C���^�����擾���ăt���[���v�[�����̃o�b�t�@�����b�N���� */
CriBool CRIAPI criMvPly_LockFrameBuffer(CriMvPly mvply, CriMvYuvBuffers *yuvbuffers, CriMvFrameInfo *frameinfo);
/* ���b�N���Ă����t���[���v�[����������� */
CriBool CRIAPI criMvPly_UnlockFrameBuffer(CriMvPly mvply, CriMvFrameInfo *frameinfo);

/*
 * For Sofdec2
 */
CriMvPly CRIAPI criMvPly_CreateWithWork(void *work, CriSint32 size, CriMvHandleConfig *config);
CriSint32 CRIAPI criMvPly_CalcHandleWorkSize(CriMvHandleConfig *config);
CriSint32 CRIAPI criMvPly_CalcPlaybackWorkSize(CriMvPly mvply, CriMvStreamingParameters *stmprm);
CriBool CRIAPI criMvPly_AllocateWorkBufferWithWork(CriMvPly mvply, void *work ,Sint32 work_size, CriMvStreamingParameters *stmprm);
void CRIAPI criMvPly_SetMetaDataWorkAllocator(CriMvPly mvply, CriMvMetaDataWorkMallocFunc allocfunc, CriMvMetaDataWorkFreeFunc freefunc, void *usrobj, CriMvMetaFlag meta_flag);
CriSint32 criMvPly_CalcSubAudioWorkSize(CriMvPly mvply, const CriMvAudioParameters *aprm);
CriBool criMvPly_CopyFrameYUVBuffers(CriMvPly mvply, CriMvYuvBuffers *yuvbuffers, 
								  const CriMvFrameInfo *frameinfo,const CriMvAlphaFrameInfo *alpha_frameinfo);
CriBool criMvPly_CopyFrameARGB8888Buffer(CriMvPly mvply,  CriUint8 *dst_buf, CriUint32 dst_pitch, CriUint32 dst_bufsize, 
									  const CriMvYuvBuffers *src_bufs, const CriMvFrameInfo *src_vinf, const CriMvAlphaFrameInfo *src_ainf);
#if defined(XPT_TGT_IPHONE) || defined(XPT_TGT_WINMO) || defined(XPT_TGT_ANDROID) || defined(XPT_TGT_CQSH2A) || defined(XPT_TGT_ACRODEA) || defined(XPT_TGT_NACL) || defined(XPT_TGT_SH7269)  || defined(XPT_TGT_TRGP6K) || defined(XPT_TGT_TRGP6K)
CriBool criMvPly_CopyFrameRGB565Buffer(CriMvPly mvply,  CriUint8 *dst_buf, CriUint32 dst_pitch, CriUint32 dst_bufsize, 
									  const CriMvYuvBuffers *src_bufs, const CriMvFrameInfo *src_vinf, const CriMvAlphaFrameInfo *src_ainf);
#endif
CriBool criMvPly_LockAlphaFrameBuffer(CriMvPly mvply, CriMvYuvBuffers *yuvbuffers, CriMvAlphaFrameInfo *alpha_frameinfo);
CriBool criMvPly_UnlockAlphaFrameBuffer(CriMvPly mvply, CriMvAlphaFrameInfo *alpha_frameinfo);
CriSint32 criMvPly_GetNumPictureData(CriMvPly mvply);
const CriMvPlyHeaderInfo* criMvPly_GetCurrentStreamInfo(CriMvPly mvply);
/* for debug */
CriBool CRIAPI criMvPly_GetAlphaFrame(CriMvPly mvply, CriUint8 *imagebuf, CriUint32 pitch, CriMvAlphaFrameInfo *alpha_frameinfo);

/* OUTER_FRAMEPOOL_WORK */
/* �t���[���v�[���p���[�N�v�Z�B�n���h����NULL�w��OK�B */
CriSint32 criMvPly_CalcFramepoolWorkSize(CriMvPly mvply, const CriMvStreamingParameters *stmprm);
/* �t���[���v�[���p���[�N�ݒ� */
void criMvPly_SetFramepoolWork(CriMvPly mvply, void *work, CriSint32 work_size);
/* �t���[���v�[���p���[�N�A���P�[�^�ݒ� */
void criMvPly_SetFramepoolWorkAllocator(CriMvPly mvply, CriMvFramepoolWorkMallocFunc allocfunc, CriMvFramepoolWorkFreeFunc freefunc, void *usrobj);

/* �V�[�N�u���b�N���̎擾 */
/* <����>
 * - seektbl_ptr : UTF�A�h���X
 * - seektbl_size : UTF�T�C�Y
 * - num_seekblock : �o�͔z��̗v�f��
 * <�o��>
 * - blockinfo : �V�[�N�u���b�N���z��ւ̃|�C���^�inum_seekblock���̗̈���m�ۂ��ēn�����Ɓj
 */
void criMvPly_GetSeekBlockInfo(CriMvPly mvply, void *seektbl_ptr, CriUint32 seektbl_size, CriSint32 num_seekblock, CriMvSeekBlockInfo *blockinfo);

/* �t���[�����[�g�̋����w�� */
void criMvPly_SetVideoFramerate(CriMvPly mvply, CriUint32 framerate_n, CriUint32 framerate_d);

/* �����L���̐ݒ� */
void criMvPly_SetSyncFlag(CriMvPly mvply, CriBool sync_flag);

/* �Đ��\���̖₢���킹 */
CriBool criMvPly_IsPlayable(CriMvPly mvply, const CriMvStreamingParameters *stmprm);

#ifdef __cplusplus
}
#endif

#endif	/* CRI_MOVIE_CORE_H_INCLUDED */
