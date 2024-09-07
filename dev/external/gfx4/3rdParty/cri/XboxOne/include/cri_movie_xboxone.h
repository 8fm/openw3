/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2013 CRI Middleware Co., Ltd.
 *
 * Library  : CRI Movie
 * Module   : Library User's Header
 * File     : cri_movie_xboxone.h
 * Date     : 2013-11-25
 *
 ****************************************************************************/
/*!
 *	\file		cri_movie_xboxone.h
 */
#ifndef	CRI_MOVIE_XBOXONE_H_INCLUDED		/* Re-definition prevention */
#define	CRI_MOVIE_XBOXONE_H_INCLUDED

/***************************************************************************
 *      Include file
 ***************************************************************************/
#include <windows.h>
#include <objbase.h>

#include <cri_xpt.h>
#include <cri_movie.h>

/***************************************************************************
 *      Variable Declaration
 ***************************************************************************/
namespace CriMv {
	extern CriError ErrorContainer;
}

/***************************************************************************
 *      Prototype Functions
 ***************************************************************************/
namespace CriMv {
	/*EN
	 * \ingroup MODULE_OPTION
	 * \brief		Set processor parameters for decoding (Library Global)
	 *
 	 *  \param		num_threads		Number of additional threads for load distribution in decoding (Maximum 5 threads)
	 *  \param		affinity_mask 	Pointer to an array of thread affinity masks for each thread specified with num_threads.
	 *  \param		priorities		Pointer to an array of thread priority for each thread specified with num_threads.
	 *  \param		err				Optional error code
	 * 
	 * This function sets the processor parameters for decoding as default setting of library global. 
	 * Use it when you want to change processors or thread priority for decoding load distribution.
	 * 
	 * If this function is called, it must be called before library initialization.
	 * 
	 * On initialization, CRI Movie prepares 5 worker threads for distributed decoding.<br>
	 *
	 * \a num_threads specifies how many worker threads CRI Movie should use.<br>
	 * \a affinity_mask is an array of affinity masks for the worker threads.  This array must have
	 * \a num_threads many elements. 
	 * \a priorities is an array of thread priority for the worker threads. This array must have
	 * \a num_threads many elements. 
	 * The format of \a affinity_mask and \a priorities is same as for the value passed to the OS standard API.<br>
     *
	 * If this function is not called, 5 distributed decoding threads will run in parallel by default.
	 * Also, processor assignment of the decoding threads is handled by the operating system, and their 
	 * priority will be THREAD_PRIORITY_BELOW_NORMAL.
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�f�R�[�h�����Ɏg���ǉ��v���Z�b�T�ݒ�i���C�u�����S�́j
	 *  \param		num_threads		���ו��U�f�R�[�h�p�Ɏg�p����ǉ��X���b�h�̐� (�ő�T�j
	 *  \param		affinity_masks	�X���b�h�A�t�B�j�e�B�}�X�N�̔z��ւ̃|�C���^�Bnum_threads�Ŏw�肵���X���b�h���Ƃ̃}�X�N�l�B
	 *  \param		priorities		�ǉ��X���b�h�̗D��x�z��ւ̃|�C���^�B
	 *  \param		err				�G���[���i�ȗ��j
	 * 
	 * �f�R�[�h�����𕪎U���ď������邽�߂̃v���Z�b�T�����C�u�����S�̂Ɏw�肵�܂��B
	 * �f�R�[�h�����ɍs���v���Z�b�T��X���b�h�D��x��ύX�������ꍇ�Ɏg�p���Ă��������B
	 * �{�֐��̓��C�u�����������O�ɌĂяo���K�v������܂��B
	 * 
	 * CRI Movie�͏������̍ۂɂT�̕��U�f�R�[�h�p�̃��[�J�[�X���b�h��p�ӂ��܂��B
	 * num_threads�����ŁA���̂����̂����̃X���b�h�����ۂɎg�p���邩���w��ł��܂��B
	 * �A�v���P�[�V�������疾���I�Ƀv���Z�b�T���蓖�Ă��s�������ꍇ�A�X�̃X���b�h�ɑ΂���
	 * �A�t�B�j�e�B�}�X�N�ƃX���b�h�D��x��ݒ肵�Ă��������B
	 * �A�t�B�j�e�B�}�X�N�ƃX���b�h�D��x�̒l�́AOS�W���֐��̈����Ɠ��������ł��B
	 * 
	 * ���̊֐����Ă΂Ȃ������ꍇ�A�T�̃X���b�h�ŕ���f�R�[�h���s���܂��B
	 * �f�R�[�h�X���b�h�̃v���Z�b�T�͊��蓖�Ă͑S��OS�C���ŁA�D��x��THREAD_PRIORITY_BELOW_NORMAL�ɂȂ�܂��B
	 */
	void SetUsableProcessors_XBOXONE(
						CriSint32 num_threads, 
						const DWORD_PTR *affinity_masks, 
						const int *priorities,
						CriError &err = CriMv::ErrorContainer);
}



#endif	/* CRI_MOVIE_XBOXONE_H_INCLUDED */
