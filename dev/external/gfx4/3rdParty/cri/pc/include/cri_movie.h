/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2006-2013 CRI Middleware Co., Ltd.
 *
 * Library  : CRI Movie
 * Module   : Library User's Header
 * File     : cri_movie.h
 * Date     : 2013-11-27
 * Version  : (see CRIMOVIE_VER)
 *
 ****************************************************************************/
/*!
 *	\file		cri_movie.h
 */
#ifndef	CRI_MOVIE_H_INCLUDED		/* Re-definition prevention */
#define	CRI_MOVIE_H_INCLUDED

/*	Version No.	*/
#define	CRIMOVIE_VER		"3.50"
#define	CRIMOVIE_NAME		"CRI Movie"

/***************************************************************************
 *      Include file
 ***************************************************************************/
#include <cri_xpt.h>
#include <cri_heap.h>
#include <cri_allocator.h>
#include <cri_sj.h>
#include <cri_error.h>
#include <cri_movie_core.h>

/***************************************************************************
 *      MACRO CONSTANT
 ***************************************************************************/
/*EN
 * \brief	Maximum length of a filename that can be opened by EasyPlayer
 * \ingroup MDL_MV_OPTION
 * \sa CriMvEasyPlayer::SetFile()
 */
/*JP
 * \brief	EasyPlayer�Ɏw��\�ȃt�@�C�����̍ő咷��
 * \ingroup MDL_MV_OPTION
 * \sa CriMvEasyPlayer::SetFile()
 */
#define CRIMV_MAX_FILE_NAME			(256)

/*EN
 * \brief	Default audio track setting used by AttachSubAudioInterface(), ReplaceCenterVoice()
 * \ingroup MDL_MV_OPTION
 * \sa CriMvEasyPlayer::ReplaceCenterVoice(), CriMvEasyPlayer::AttachSubAudioInterface()
 */
/*JP
 * \brief	�T�u�I�[�f�B�I�i�܂��̓Z���^�[�{�C�X�j�̃f�t�H���g�l
 * \ingroup MDL_MV_OPTION
 * \sa CriMvEasyPlayer::ReplaceCenterVoice(), CriMvEasyPlayer::AttachSubAudioInterface()
 */
#define CRIMV_CENTER_VOICE_OFF		(-1)


/***************************************************************************
 *      Variable Declaration
 ***************************************************************************/
namespace CriMv {
	extern CriError ErrorContainer;
}

/***************************************************************************
 *      Prototype Functions
 ***************************************************************************/
/*EN
 * \brief CRI Movie Namespace
 * \ingroup MDL_MV_BASIC
 * \par
 * Namespace for all CRI Movie methods, constants, and declarations
 */
/*JP
 * \brief CRI Movie Namespace
 * \ingroup MDL_MV_BASIC
 * \par
 * Namespace for all CRI Movie methods, constants, and declarations
 */
namespace CriMv {
	
	 /*EN
     * \brief   Returns the version number and build information of CRI Movie
     * \param   none
     * \return  A string constant
     *
     * Returns the version number and build information of CRI Movie as a constant
     * string, in the form 
     *
     *  "CRI Movie/{PLATFORM} {VERSION} Build:{BUILD DATE}"
     */
    /*JP
     * \brief	CRI Movie�̃o�[�W�����ԍ���r���h����Ԃ��܂��B
	 * \return	���C�u������񕶎���
	 *
     */
	const CriChar8* CRIAPI GetLibraryVersionString(void);

	/*EN
	 * \brief	Initialize the CRI Movie library
	 * \param	err				Optional error code
	 * 
	 * Does one-time initialization of the CRI Movie library.<BR>
	 * This function must be successfully called before calling CriMvEasyPlayer::Create().<BR>
	 * <BR>
	 * In general, all CRI Movie APIs are available after calling CriMv::Initialize(), 
	 * until CriMv::Finalize() is called.  However, there are some methods that set options
	 * for the library as a whole which must be called before CriMv::Initialize():<br>
	 * <BR>
	 * It is safe to call CriMv::Initialize() more than once, as long as your application calls
	 * CriMv::Finalize() once for each call to CriMv::Initialize().  Multiple calls will not
	 * have an effect on the library, other than to increment or decrement an internal counter.
	 * 
	 * \sa CriMv::Finalize()
	 */
	/*JP
	 * \brief	CRI Movie���C�u�����̏�����
	 *  \param	err				�G���[���i�ȗ��j
	 * 
	 * CRI Movie ���C�u���������������܂��B<br>
	 * CriMvEasyPlayer::Create �֐�������ɌĂяo���Ă��������B<br>
	 * <BR>
	 * �����Ƃ��đS�Ă� CRI Movie ���C�u�����֐��͏�������A�I���֐��Ăяo���܂ł̊Ԃɂ̂ݎg�p���܂��B<BR>
	 * �������A�������̐ݒ�֐��͏������֐�������ɌĂяo���K�v��������̂�����܂��B
	 * �ڍׂ͊e�ݒ�֐��̐������Q�Ƃ��Ă��������B
	 * <BR>
	 * �������֐��𕡐���Ăяo�����ꍇ�A�Q��ڈȍ~�̌Ăяo���ł͌Ăяo���񐔂��L�^���邾���ōď������͍s���܂���B<BR>
	 * ���̏ꍇ�A�������I���������s���ɂ͓����񐔂����I���֐����Ăяo���K�v������܂��B
	 * �������֐��ƏI���֐��͕K���΂ŌĂяo���悤�Ɏ������Ă��������B<BR>
	 * 
	 * \sa CriMv::Finalize()
	 */
	void CRIAPI Initialize(CriError &err = CriMv::ErrorContainer);

	/* MEMO: 
	 * If an application calls this function instead of CriMv::Initialize(), 
	 * an application need to call CriMv::SetupMovieHandleWork() before CriMv::InitializeMana().
	 */
	void CRIAPI InitializeMana(CriError &err = CriMv::ErrorContainer);

	/*EN
	 * \brief	Initialize 32bit ARGB frame conversion
	 * 
	 * This function initializes 32bit ARGB frame conversion.<BR>
	 * When an application uses CriMvEasyPlayer::GetFrameOnTimeAs32bitARGB(), 
	 * please call this function after CriMv::Initialize().<BR>
	 * 
	 * \sa CriMvEasyPlayer::GetFrameOnTimeAs32bitARGB()
	 */
	/*JP
	 * \brief	32bitARGB�p�t���[���ϊ��̏�����
	 * 
	 * 32bitARGB�p�t���[���ϊ����������������܂��B<br>
	 * CriMvEasyPlayer::GetFrameOnTimeAs32bitARGB() ���g�p����ꍇ�� CRI Movie ���C�u������
	 * ��������ɕK���Ăяo���Ă��������B<br>
	 * 
	 * \sa CriMvEasyPlayer::GetFrameOnTimeAs32bitARGB()
	 */
	void CRIAPI InitializeFrame32bitARGB(void);

#if defined(XPT_TGT_IPHONE) || defined(XPT_TGT_WINMO) || defined(XPT_TGT_ANDROID) || defined(XPT_TGT_CQSH2A) || defined(XPT_TGT_ACRODEA) || defined(XPT_TGT_NACL) || defined(XPT_TGT_SH7269) || defined(XPT_TGT_PC) || defined(XPT_TGT_TRGP6K)
	void CRIAPI InitializeFrameRGB565(void);
#endif

	/*EN
	 * \brief	De-initializes the CRI Movie library
	 *  \param	err				Optional error code 
	 * 
	 * This function finalize whole CRI Movie library.<BR>
	 * An application needs to destroy all CriMvEasyPlayer handles and decoding threads before callign CriMv::Finalize().<BR>
	 * <BR>
	 * In principle, all CRI Movie library APIs are enabled after CriMv::Initialize() until CriMv::Finalize().<BR>
	 * But there are some APIs for parameter setting, which need to be called after CriMv::Finalize().
	 * For details, refer to each explanations of setting APIs.
	 * <BR>
	 * \remarks
	 * CriMv::Finalize() must be called once for each call to CriMv::Initialize().  When the
	 * internal initialization count reaches 0, the library will be finalized.
	 * 
	 * \sa CriMv::Initialize()
	 */
	/*JP
	 * \brief	CRI Movie���C�u�����̏I��
	 *  \param	err				�G���[���i�ȗ��j
	 * 
	 * CRI Movie ���C�u�������I�����܂��B<br>
	 * ���̊֐����Ăяo���O�ɁA�S�Ă� CriMvEasyPlayer �n���h������уf�R�[�h�X���b�h��j�����Ă��������B<br>
	 * <BR>
	 * �����Ƃ��đS�Ă� CRI Movie ���C�u�����֐��͏�������A�I���֐��Ăяo���܂ł̊Ԃɂ̂ݎg�p���܂��B<BR>
	 * �������A�������̐ݒ�֐��͏I���֐�������ɌĂяo���K�v��������̂�����܂��B
	 * �ڍׂ͊e�ݒ�֐��̐������Q�Ƃ��Ă��������B
	 * <BR>
	 * �������֐��𕡐���Ăяo�����ꍇ�A�������I���������s���ɂ͓����񐔂����I���֐����Ăяo���K�v������܂��B
	 * �������֐��ƏI���֐��͕K���΂ŌĂяo���悤�Ɏ������Ă��������B<BR>
	 * 
	 * \sa CriMv::Initialize()
	 */
	void CRIAPI Finalize(CriError &err = CriMv::ErrorContainer);

#if !defined(XPT_TGT_EE)
	/*EN
	 * \brief	Calculates the work buffer size needed for the given number of movie handles
	 *  \param	max_num			Maximum number of movie handles desired
	 *  \param	err				Optional error code 
	 *  \return					The necessary work buffer size, in bytes
	 * 
	 * Each active movie handle requires an internal work buffer.  If your application
	 * needs to have multiple movies open at one time, it should determine how many handles 
	 * it needs, allocate a buffer of the size returned by this function, and pass it to
	 * CriMv::SetupMovieHandleWork().<br>
	 *
	 * Each CriMvEasy object uses a movie handle.  Alpha movie playback uses two handles.
	 * If you need to play aplha movies, be sure to double the number of handles requested.
	 * 
	 * \remarks
	 * The number of movie handles must be set <b>before</b> calling CriMv::Initialize().
	 * 
	 * \sa CriMv::SetupMovieHandleWork()
	 */
	/*JP
	 * \brief	�}���`�n���h���p���[�N�o�b�t�@�T�C�Y�̌v�Z
	 *  \param	max_num			�ő�n���h�����i�s�������[�r�̍Đ����j
	 *  \param	err				�G���[���i�ȗ��j
	 *  \return					���[�N�T�C�Y
	 * 
	 * �����Ɏg�p���� CriMvEasyPlayer �n���h���̍ő吔�𑝉�������ꍇ�ɕK�v�ȃ��[�N�o�b�t�@
	 * �T�C�Y���v�Z���܂��B
	 * 
	 * �A���t�@���[�r���Đ�����ƃn���h���������Q����܂��B
	 * ���������̃A���t�@���[�r�Đ����s�������ꍇ�́A�ő�n���h�����͔{�ɂ��Ďw�肵�Ă��������B
	 * 
	 * \sa CriMv::SetupMovieHandleWork()
	 */
	CriUint32 CRIAPI CalcMovieHandleWork(CriUint32 max_num, CriError &err = CriMv::ErrorContainer);

	/*EN
	 * \brief	Increases the default number of available movie handles
	 *  \param	max_num			Maximium number of movie handles
	 *  \param	workbuf			Work buffer
	 *  \param	worksize		Size of work buffer
	 *  \param	err				Optional error code 
	 *
	 * Call this function if your application needs to increase the maximum number of open movies
	 * beyond the default.  Note that playing an alpha movie requires a second handle.<br>
	 *
	 * The default number of movie handles depends on the platform.<br>
	 *
	 * The work buffer must be allocated based on the size returned by CriMv::CalcMovieHandleWork().<br>
	 * 
	 * \remarks
	 * This function must be called <b>before</b> calling CriMv::Initialize().
	 * 
	 * \sa CriMv::CalcMovieHandleWork()
	 */
	/*JP
	 * \brief	�}���`�n���h���p���[�N�o�b�t�@�̐ݒ�
	 *  \param	max_num			�ő�n���h�����i�s�������[�r�̍Đ����j
	 *  \param	workbuf			���[�N�o�b�t�@�A�h���X
	 *  \param	worksize		���[�N�o�b�t�@�T�C�Y
	 *  \param	err				�G���[���i�ȗ��j
	 * 
	 * �����Ɏg�p���� CriMvEasyPlayer �n���h���̍ő吔�𑝉������邽�߂̃��[�N�o�b�t�@��ݒ肵�܂��B
	 * �Ȃ��A���[�N�o�b�t�@���w�肵�Ȃ��ꍇ�̃n���h��������͋@��ɂ���ĈقȂ�܂��B
	 * 
	 * ���[�N�o�b�t�@�̐ݒ�́A CriMv::Initialize() �̌Ăяo��<B>��</B>�Ɏ��s���Ă��������B
	 * 
	 * \sa CriMv::CalcMovieHandleWork()
	 */
	void CRIAPI SetupMovieHandleWork(CriUint32 max_num, void *workbuf, CriUint32 worksize, CriError &err = CriMv::ErrorContainer);

	/*EN
	 * \brief	Get max number of movie handles
	 *  \param	err				Optional error code 
	 *  \return					Max number of movie handles you set by CriMv::SetupMovieHandleWork().
	 *
	 * Returns the maximum number of movie handles that are available to CRI Movie.<br>
	 *
	 * Note that this is not necessarily equal to the number of movies that can be
	 * opened at a time.  In general, each movie will use one handle; however,
	 * alpha channel movies uses two handles.
	 * 
	 * \sa CriMv::SetupMovieHandleWork(), CriMv::CalcMovieHandleWork()
	 */
	/*JP
	 * \brief	�ő�n���h�����̎擾
	 *  \param	err				�G���[���i�ȗ��j
	 *  \return					CriMv::SetupMovieHandleWork() �Őݒ肵���ő�n���h����
	 * 
	 * CriMv::CalcMovieHandleWork() �ōő�n���h�����𑝉��������ꍇ�ɁA
	 * �ݒ肵���ő�n���h�������擾���܂��B
	 * 
	 * \sa CriMv::SetupMovieHandleWork(), CriMv::CalcMovieHandleWork()
	 */
	CriUint32 CRIAPI GetMaxNumberOfHandles(CriError &err = CriMv::ErrorContainer);
#endif

	/* For Sofdec2 */
	CriSint32 CRIAPI CalcHandleWorkSize(CriMvHandleConfig *config, CriError &err = CriMv::ErrorContainer);
	void CRIAPI SetDelayDestroySubmodules(CriBool sw);
}

/***************************************************************************
 *      CLASS
 ***************************************************************************/
/*EN
 * \brief	File Reading Interface Class for EasyPlayer
 * \ingroup MDL_IF_READER
 * \par
 * You can implement your own streaming filesystem for movie playback by deriving
 * from this class.<br>
 *
 * Pass an instance of this subclass to CriMvEasyPlayer::Create().<br>
 *
 * This class is designed for asynchronous operation. All functions are pure virtual.
 *
 * \sa CriMvEasyPlayer::Create()
 */
/*JP
 * \brief	�t�@�C���ǂݍ��݃C���^�t�F�[�X for EasyPlayer
 * \ingroup MDL_IF_READER
 * 
 * ���̃N���X���`���邱�ƂŁA���O�̃t�@�C���V�X�e�����g����EasyPlayer��
 * �X�g���[�~���O�Đ����\�ɂȂ�܂��B<br>
 * �S�Ă̊֐��͏������z�֐��Ƃ��Ē�`����Ă���̂ŁA�S�Ă̊֐���K���������Ă��������B
 *
 * \sa CriMvEasyPlayer::Create()
 */
class CriMvFileReaderInterface
{
public:
	/*EN Status of an asynchronous operation */
	/*JP �񓯊������X�e�[�^�X */
	enum AsyncStatus {
		ASYNC_STATUS_STOP,			/*EN< No action */
									/*JP< �������Ă��Ȃ���ԁB*/
		ASYNC_STATUS_BUSY,			/*EN< Currently processing */
									/*JP< ������ */
		ASYNC_STATUS_COMPLETE,		/*EN< Processing completed */
									/*JP< �����I�� */
		ASYNC_STATUS_ERROR,			/*EN< An error occured */
									/*JP< �G���[ */

		/* Keep enum 4bytes */
		ASYNC_STATUS_MAKE_ENUM_SINT32 = 0x7FFFFFFF
	};
	/*EN Offset values for Seek() */
	/*JP �V�[�N�J�n�ʒu */
	enum SeekOrigin {
		SEEK_FROM_BEGIN,			/*EN< Start of file */
									/*JP< �t�@�C���擪 */
		SEEK_FROM_CURRENT,			/*EN< Current position in file */
									/*JP< �t�@�C���̌��݈ʒu */
		SEEK_FROM_END,				/*EN< End of file */
									/*JP< �t�@�C���I�[ */

		/* Keep enum 4bytes */
		SEEK_FROM_MAKE_ENUM_SINT32 = 0x7FFFFFFF
	};
	/*EN
	 * \brief	Opens a file asynchronously
	 * \param	fname		Name of file to open
	 *
	 * Initiates a file open request, and returns immediately.
	 *
	 * To determine success or failure, call CriMvFileReaderInterface::GetOpenStatus().<br>
	 *
	 * \remarks
	 * CRI Movie may call CriMvFileReaderInterface::Read() and CriMvFileReaderInterface::Close()
	 * before CriMvFileReaderInterface::GetOpenStatus() returns ASYNC_STATUS_COMPLETE.
	 * 
	 * \sa CriMvFileReaderInterface::GetOpenStatus()
	 */
	/*JP
	 * \brief	�t�@�C�����ɂ��t�@�C���I�[�v��(�������A)
	 * \param	fname �t�@�C�����̕�����
	 * 
	 * �t�@�C�����w��Ńt�@�C���̃I�[�v���v�����o���܂��B<br>
	 * ���̊֐��͑������A�̊֐��Ƃ��ČĂяo����܂��B<br>
	 * �I�[�v���������I��������ǂ����� CriMvFileReaderInterface::GetOpenStatus�֐� 
	 * �Ń`�F�b�N�ł���悤�ɂ��Ă��������B<br>
	 * CRI Movie ���C�u�����̓I�[�v���������I���i�� CriMvFileReaderInterface::GetOpenStatus�֐���
	 * ASYNC_STATUS_COMPLETE��Ԃ��j�O�ɁA���[�h�A�N���[�Y�̗v�����Ăяo���\��������܂��B
	 * 
	 * \sa CriMvFileReaderInterface::GetOpenStatus()
	 */
	/* pure */ virtual void Open(CriChar8 *fname)=0;

	/*EN
	 * \brief	Closes a file asynchronously
	 *
	 * Initiates a file close request, and returns immediately.<br>
	 *
	 * To determine success or failure, call CriMvFileReaderInterface::GetCloseStatus().<br>
	 * 
	 * \sa CriMvFileReaderInterface::GetCloseStatus()
	 */
	/*JP
	 * \brief	�t�@�C���̃N���[�Y(�������A)
	 * 
	 * �I�[�v���ς݂̃t�@�C���̃N���[�Y�v�����o���܂��B<br>
	 * ���̊֐��͑������A�̊֐��Ƃ��ČĂяo����܂��B<br>
	 * �N���[�Y�������I��������ǂ����� CriMvFileReaderInterface::GetCloseStatus() 
	 * �Ń`�F�b�N�ł���悤�ɂ��Ă��������B
	 * 
	 * \sa CriMvFileReaderInterface::GetCloseStatus()
	 */
	/* pure */ virtual void Close(void)=0;

	/*EN
	 * \brief	Reads from a file
	 * \param	buffer		Buffer to read into
	 * \param	req_size	Size of the buffer
	 * 
	 * Initiates a file read request, and returns immediately.<br>
	 *
	 * The buffer must be available and writable until the read request completes.<br>
	 *
	 * To determine success or failure, call CriMvFileReaderInterface::GetReadStatus().<br>
	 *
	 * To determine the number of bytes actually read, call CriMvFileReaderInterface::GetReadSize()
	 * after CriMvFileReaderInterface::GetReadStatus() has returned ASYNC_STATUS_COMPLETE.
	 * 
	 * \remarks
	 * CRI Movie may call CriMvFileReaderInterface::Close() before 
	 * CriMvFileReaderInterface::GetReadStatus() returns ASYNC_STATUS_COMPLETE.
	 * 
	 * \sa CriMvFileReaderInterface::GetReadStatus(), CriMvFileReaderInterface::GetReadSize()
	 */
	/*JP
	 * \brief	�ǂݍ��ݗv���i�������A�j
	 * \param	buffer		�����o���o�b�t�@�̃|�C���^�B�ǂݍ��ݗv���T�C�Y�𖞂��������̃o�b�t�@���m�ۂ��Ă����K�v������܂��B
	 * \param	req_size	�ǂݍ��ݗv���T�C�Y�B�P�ʂ̓o�C�g�P�ʂł��B
	 * 
	 * �t�@�C���̓ǂݍ��ݗv�����o���܂��B<br>
	 * ���̊֐��͑������A�̊֐��Ƃ��ČĂяo����܂��B<br>
	 * ���[�h�������I��������ǂ����� CriMvFileReaderInterface::GetReadStatus() 
	 * �Ń`�F�b�N�ł���悤�ɂ��Ă��������B<br>
	 * CRI Movie ���C�u�����̓��[�h�������I���i�� CriMvFileReaderInterface::GetReadStatus()��
	 * ASYNC_STATUS_COMPLETE��Ԃ��j�O�ɁA�N���[�Y�v�����Ăяo���\��������܂��B<br>
	 * ���̊֐��͓ǂݍ��񂾃T�C�Y��Ԃ��܂���B<br>
	 * �ǂݍ��ݍς݃T�C�Y�́A CriMvFileReaderInterface::GetReadStatus()�� ASYNC_STATUS_COMPLETE��
	 * �Ԃ������Ƃ� CriMvFileReaderInterface::GetReadSize()�ŕԂ��悤�Ɏ������Ă��������B
	 * 
	 * \sa CriMvFileReaderInterface::GetReadStatus(), CriMvFileReaderInterface::GetReadSize()
	 */
	/* pure */ virtual void Read(CriUint8 *buffer, CriSint64 req_size)=0;

	/*EN
	 * \brief   Gets asynchronous status of a call to CriMvFileReaderInterface::Open()
	 *
	 * \return  Status of the call
	 *
	 * While the Open() call is in process, this will return ASYNC_STATUS_BUSY.<br>
	 * If the call fails, this will return ASYNC_STATUS_ERROR.<br>
	 * On success, this will return ASYNC_STATUS_COMPLETE.<br>
	 * 
	 * \sa CriMvFileReaderInterface::Open()
	 */
	/*JP
	 * \brief	�I�[�v���R�}���h�̏�Ԏ擾
	 *  \return		�I�[�v���R�}���h�̏�����ԁB
	 * 
	 * CriMvFileReaderInterface::Open�֐��̏�����Ԃ��擾���܂��B
	 * 
	 * \sa CriMvFileReaderInterface::Open()
	 */
	/* pure */ virtual CriMvFileReaderInterface::AsyncStatus GetOpenStatus(void)=0;

	/*EN
	 * \brief   Gets asynchronous status of a call to CriMvFileReaderInterface::Close()
	 *
	 * \return  Status of the call
	 *
	 * While the Close() call is in process, this will return ASYNC_STATUS_BUSY.<br>
	 * If the call fails, this will return ASYNC_STATUS_ERROR.<br>
	 * On success, this will return ASYNC_STATUS_COMPLETE.<br>
	 * 
	 * \sa CriMvFileReaderInterface::Close()
	 */
	/*JP
	 * \brief	�N���[�Y�R�}���h�̏�Ԏ擾
	 *  \return		�N���[�Y�R�}���h�̏�����ԁB
	 * 
	 * CriMvFileReaderInterface::Close�֐��̏�����Ԃ��擾���܂��B
	 * 
	 * \sa CriMvFileReaderInterface::Close()
	 */
	/* pure */ virtual CriMvFileReaderInterface::AsyncStatus GetCloseStatus(void)=0;

	/*EN
	 * \brief   Gets asynchronous status of a call to CriMvFileReaderInterface::Read()
	 *
	 * \return  Status of the call
	 *
	 * While the Read() call is in process, this will return ASYNC_STATUS_BUSY.<br>
	 * If the call fails, this will return ASYNC_STATUS_ERROR.<br>
	 * On success, this will return ASYNC_STATUS_COMPLETE.<br>
	 * 
	 * \sa CriMvFileReaderInterface::Read()
	 */
	/*JP
	 * \brief	���[�h�R�}���h�̏�Ԏ擾
	 *  \return		���[�h�R�}���h�̏�����ԁB
	 * 
	 * CriMvFileReaderInterface::Read�֐��̏�����Ԃ��擾���܂��B
	 * 
	 * \sa CriMvFileReaderInterface::Read()
	 */
	/* pure */ virtual CriMvFileReaderInterface::AsyncStatus GetReadStatus(void)=0;

	/*EN
	 * \brief   Gets the number of bytes read by a successful call to CriMvFileReaderInterface::Read()
	 *
	 * \return  Number of bytes read
	 *
	 * Once CriMvFileReaderInterface::GetReadStatus() returns ASYNC_STATUS_COMPLETE, this 
	 * function can be called to determine the number of bytes read by 
	 * CriMvFileReaderInterface::Read().<br>
	 *
	 * If no call to Read() has been made, this function will return 0.<br>
	 *
	 * If called multiple times after completing the read, the same value will be returned each time.
	 * 
	 * \sa CriMvFileReaderInterface::Read(), CriMvFileReaderInterface::GetReadStatus()
	 */
	/*JP
	 * \brief		�O��ǂݍ��ݗv���ɑ΂���ǂݍ��݊����T�C�Y
	 *  \return		�ǂݍ��݊����T�C�Y�B�P�ʂ�Byte�B
	 * 
	 * �O��̓ǂݍ��ݗv���ɑ΂��ēǂݍ��݊��������T�C�Y��Ԃ��܂��B
	 * �܂��ǂݍ��݂��P�x���v������Ă��Ȃ��ꍇ�͂O��Ԃ��܂��B
	 * �ǂݍ��݊�����ɌJ��Ԃ����̊֐����Ăяo���ꂽ�ꍇ�́A���ׂē����l��Ԃ��܂��B
	 * 
	 * \sa CriMvFileReaderInterface::Read(), CriMvFileReaderInterface::GetReadStatus()
	 */
	/* pure */ virtual CriSint64 GetReadSize(void)=0;

	/*EN
	 * \brief   Seeks to a new position in the file
	 *
	 * \param   size        Number of bytes to seek relative to \a offset
	 * \param   offset      Starting position of seek
	 * \return  The offset, in bytes, from the previous file position.
	 *
	 * If \a offset is <b>SEEK_FROM_BEGIN</b>, seeking will start from the beginning of the file.<br>
	 * If \a offset is <b>SEEK_FROM_CURRENT</b>, seeking will start from the current file position.<br>
	 * If \a offset is <b>SEEK_FROM_END</b>, seeking will start from the end of the file.<br>
	 *
	 * \sa CriMvFileReaderInterface::SeekOrigin
	 */
	/*JP
	 * \brief	�V�[�N
	 *  \param	size		�V�[�N�T�C�Y
	 *  \param	offset		�V�[�N�̊J�n�ʒu
	 *  \return		���ۂɃV�[�N���������BByte�P�ʁB
	 *
	 * \sa CriMvFileReaderInterface::SeekOrigin
	 *
	 */
	/* pure */ virtual CriSint64 Seek(CriSint64 size, CriMvFileReaderInterface::SeekOrigin offset)=0;

	/*EN
	 * \brief   Gets the file size
	 *
	 * \return  File size, in bytes
	 *
	 * This function can safely be called once CriMvFileReaderInterface::Open() has completed
	 * successfully.
	 * 
	 * \sa CriMvFileReaderInterface::Open(), CriMvFileReaderInterface::GetOpenStatus()
	 */
	/*JP
	 * \brief		�t�@�C���T�C�Y�̎擾
	 *  \return		�t�@�C���T�C�Y[byte].
	 * 
	 * ���̊֐��̓t�@�C���I�[�v���̏I����ɌĂяo����܂��B
	 * 
	 * \sa CriMvFileReaderInterface::Open(), CriMvFileReaderInterface::GetOpenStatus()
	 */
	/* pure */ virtual CriSint64 GetFileSize(void)=0;

protected:
	virtual ~CriMvFileReaderInterface(void) {}
};

/*EN
 * \brief	Sound Interface Class for EasyPlayer
 * \ingroup MDL_IF_SOUND
 * 
 * \par
 * A class derived from CriMvSoundInterface is required in order to play sound in CRI Movie.
 * Pass an instance of this subclass to CriMvEasyPlayer::Create().<br>
 *
 * If you do not need audio output, you can pass NULL instead.  However, if you do,
 * you can not use a movie timer of type MVEASY_TIMER_AUDIO.  See CriMvEasyPlayer::SetMasterTimer()
 * for more details.<br>
 *
 * Sound data must be provided in either 32 or 16 bit PCM format.
 *
 * All functions are pure virtual.
 *
 * \sa CriMvEasyPlayer::Create(), CriMvEasyPlayer::SetMasterTimer(), CriMvEasyPlayer::TimerType
 */
/*JP
 * \brief	�T�E���h�o�̓C���^�t�F�[�X
 * \ingroup MDL_IF_SOUND
 *
 * \sa CriMvEasyPlayer::Create(), CriMvEasyPlayer::SetMasterTimer(), CriMvEasyPlayer::TimerType
 */
class CriMvSoundInterface
{
public:
	/*EN
	 * \brief		The status of the Sound Module
	 * 
	 * This is the return value of CriMvSoundInterface::GetStatus().<br>
	 *
	 * After CriMvSoundInterface::Start() is called, the state transitions to MVEASY_SOUND_STATUS_EXEC.<br>
	 * While the state is MVEASY_SOUND_STATUS_EXEC, the sound module calls the callback function
	 * to retrieve sound data as needed.<br>
	 * When the EasyPlayer is stopped, or transits to MVEASY_STATUS_PLAYEND, 
	 * CRI Movie will call CriMvSoundInterface::Stop().  Then, CRI Movie waits for 
	 * CriMvSoundInterface::GetStatus() to return MVEASY_SOUND_STATUS_STOP, 
	 * and calls CriMvSoundInterface::DestroyOutput().
	 * 
	 * \sa CriMvSoundInterface::GetStatus(), CriMvSoundInterface::Start(), 
	 *     CriMvSoundInterface::Stop(), CriMvSoundInterface::DestroyOutput()
	 */
	/*JP
	 * \brief		�T�E���h���W���[���̏��
	 * 
	 * �T�E���h���W���[���̏�Ԃ�\���񋓌^�ł��B<br>
	 * CriMvEasyPlayer::GetStatus() �̊֐��l�ł��B<br>
	 * CriMvSoundInterface::Start() ���Ăяo������MVEASY_SOUND_STATUS_EXEC��ԂɂȂ�܂��B<br>
	 * MVEASY_SOUND_STATUS_EXEC��Ԃ̊Ԃ́A�T�E���h�o�̓��W���[���̓R�[���o�b�N�֐����Ăяo���܂��B<br>
	 * CRI Movie ���C�u�����͍Đ��I���܂��͍Đ���~�w�����󂯂��ꍇ�A�܂� CriMvSoundInterface::Stop()���Ăяo���܂��B<br>
	 * ���̌�ASTOP��ԂɂȂ�̂�҂��Ă��� CriMvSoundInterface::DestroyOutput()���Ăяo���܂��B
	 * 
	 * \sa CriMvSoundInterface::GetStatus(), CriMvSoundInterface::Start(), 
	 *     CriMvSoundInterface::Stop(), CriMvSoundInterface::DestroyOutput()
	 */
	enum Status {
		MVEASY_SOUND_STATUS_STOP,		/*EN< No sound processing is happening. */
										/*JP< CRI Movie �̃T�E���h�o�͂����Ă��Ȃ���� */
		MVEASY_SOUND_STATUS_EXEC,		/*EN< Sound data is being retrieved and processed. */
										/*JP< CRI Movie �̃T�E���h�o�͒� */
		MVEASY_SOUND_STATUS_ERROR,		/*EN< An error has occurred.  */
										/*JP< �G���[��� */

		/* Keep enum 4bytes */
		MVEASY_SOUND_STATUS_MAKE_ENUM_SINT32 = 0x7FFFFFFF
	};

	/*EN
	 * \brief		PCM audio data format
	 * 
	 * CRI Movie only operates on PCM encoded data.  Two formats are supported: 32 bit floating
	 * point values, and 16 bit integer values.
	 *
	 * Return one of these values from CriMvSoundInterface::GetPcmFormat().
	 * According to the returned format, CRI Movie calls a SetCallback for the specified PCM format.
	 *
	 * \sa CriMvSoundInterface::GetPcmFormat(), CriMvSoundInterface::SetCallbackGetFloat32PcmData(), 
	 *     CriMvSoundInterface::SetCallbackGetSint16PcmData()
	 */
	/*JP
	 * \brief		PCM�f�[�^�t�H�[�}�b�g
	 * 
	 * PCM�̏o�̓t�H�[�}�b�g��PcmFormat�^�Œ�`���ꂽ�����ꂩ�łȂ���΂����܂���B<br>
	 * EasyPlayer�� CriMvSoundInterface::GetPcmFormat()�Ŏ擾�ł���f�[�^�^�̂ݎg�p���܂��B<br>
	 * �A�v���P�[�V�����͂��̃N���X�̑S�Ă̊֐����������Ȃ���΂����Ȃ��̂ŁA�g��Ȃ��t�H�[�}�b�g
	 * �̃R�[���o�b�N�o�^�֐��̓J���֐��Ƃ��Ď������Ă��������B
	 * 
	 * \sa CriMvSoundInterface::GetPcmFormat(), CriMvSoundInterface::SetCallbackGetFloat32PcmData(), 
	 *     CriMvSoundInterface::SetCallbackGetSint16PcmData()
	 */
	enum PcmFormat {
		MVEASY_PCM_FLOAT32,		/*EN< PCM data is in 32 bit floating point format. */
								/*JP< 32bit ���������^��PCM�t�H�[�}�b�g */
		MVEASY_PCM_SINT16,		/*EN< PCM data is in 16 bit integer format. */
								/*JP< 16bit �����^��PCM�t�H�[�}�b�g */

		/* Keep enum 4bytes */
		MVEASY_PCM_MAKE_ENUM_SINT32 = 0x7FFFFFFF
	};

	/*EN
	 * \brief		Creates a sound output module
	 *  \param		heap		Handle to a CriHeap object
	 *  \param		channel		Number of sound channels (1 = monaural, 2 = stereo, 6 = 5.1ch)
	 *  \param		samplerate	Sample rate of audio data (ex. 48000 = 48k)
	 *  \return		TRUE if the sound module was created successfully
	 *  \return     FALSE if there was an error
	 * 
	 * Creates a CRI Movie sound output module and prepares it for use.  Memory for the module 
	 * is taken from the provided CriHeap object.<br>
	 *
	 * CRI Movie calls this method once it has determined that a movie has an active audio track
	 * and has analyzed the number of channels (mono, stereo, 5.1ch, etc.) and the sample rate.
	 * 
	 */
	/*JP
	 * \brief		�T�E���h�o�͂̍쐬
	 *  \param		heap		�������n���h��
	 *  \param		channel		�o�͂���T�E���h�̃`���l���� (1=monaural, 2=stereo, 6=5.1ch)
	 *  \param		samplerate	�T���v�����O���[�g (ex. 48k = 48000)
	 *  \return		�쐬���ʁB�����̏ꍇ��TRUE�A���s�̏ꍇ��FALSE���Ԃ�܂��B
	 * 
	 * �T�E���h�o�͂��쐬���܂��B<br>
	 * ���̊֐��� CRI Movie ���Đ�����T�E���h�����肵�����ƂɁA���̃T�E���h�̃`���l������
	 * �T���v�����O���[�g�������Ƃ��Ď��s����܂��B
	 * 
	 */
	/* pure */ virtual CriBool CreateOutput(CriHeap heap, CriUint32 channel, CriUint32 samplerate)=0;

	/*EN
	 * \brief       Destroys the sound output module
	 *
	 * Deletes the sound output module that was created by 
	 * CriMvSoundInterface::CreateOutput().<br>
	 *
	 * CRI Movie calls this method once CriMvSoundInterface::GetStatus() returns MVEASY_SOUND_STATUS_STOP.
	 * 
	 */
	/*JP
	 * \brief		�T�E���h�o�͂̔j��
	 * 
	 * �T�E���h�o�͂�j�����܂��B<br>
	 * ���̊֐��̓T�E���h�o�͂� MVEASY_SOUND_STATUS_STOP ��ԂɂȂ�����ɌĂяo����܂��B
	 * 
	 */
	/* pure */ virtual void DestroyOutput(void)=0;

	/*EN
	 * \brief       Gets the PCM format of audio data
	 * \return      The type of PCM format supported by this CriMvSoundInterface instance
	 *
	 * CRI Movie supports audio data in one of two PCM formats: 32 bit floating point (MVEASY_PCM_FLOAT32)
	 * or 16 bit integer (MVEASY_PCM_SINT16).  EasyPlayer uses the return value from this method to 
	 * determine which format is being used, and will call the appropriate callback function to retrieve
	 * data samples.
	 * 
	 * \sa CriMvSoundInterface::PcmFormat
	 */
	/*JP
	 * \brief		PCM�f�[�^�t�H�[�}�b�g�̎擾
	 *  \return		CriMvSoundInterface ���g�p����PCM�t�H�[�}�b�g��Ԃ��܂��B
	 * 
	 * EasyPlayer�͂��̊֐��ɂ���āA�o�͂���PCM�t�H�[�}�b�g�𔻒f���܂��B
	 * 
	 * \sa CriMvSoundInterface::PcmFormat
	 */
	/* pure */ virtual PcmFormat GetPcmFormat(void)=0;

	/*EN
	 * \brief		Sets the callback function for retrieving 32 bit floating point PCM sound samples
	 *  \param	func	Function that is called when CriMvSoundInterface gets PCM data (32bit float)
	 *  \param	obj		Pointer to user-specifed data, passed as the first argument to the callback
	 * 
	 * Sets a function that CriMvSoundInterface will call when it needs audio data in 32 bit floating point 
	 * PCM format.  This callback function takes 4 arguments:<br>
	 *
	 * - \a obj:        The user-specified \a obj parameter passed to SetCallbackGetFloat32PcmData().<br>
	 * - \a nch:        The number of audio channels.  Mono is 1, stereo is 2, 5.1 channel is 6.<br>
	 * - \a pcmbuf:     An array of buffers to hold the returned PCM data.  There must be one element of 
	 *                  this array for each channel.<br>
	 * - \a req_nsmpl:  The number of samples requested.  Each buffer must be large enough to hold this
	 *                  many samples (i.e. \a nch * \a req_nsmpl).
	 * 
	 * \remarks
	 * The maximum number of channels is CRIMV_PCM_BUFFER_MAX
	 *
	 * \sa CriMvSoundInterface::SetCallbackGetSint16PcmData()
	 */
	/*JP
	 * \brief		32bit�`����PCM�f�[�^���擾����R�[���o�b�N�֐��̓o�^
	 *  \param	func	CriMvSoundInterface ��PCM�f�[�^��v������ۂɌĂт����R�[���o�b�N�֐�
	 *  \param	obj		�R�[���o�b�N�֐������s����ۂɑ������Ɏw�肷��I�u�W�F�N�g�ϐ�
	 * 
	 * CriMvSoundInterface ��EasyPlayer��PCM�f�[�^��v������ۂɌĂяo���R�[���o�b�N�֐���o�^���܂��B
	 * �R�[���o�b�N�֐��͂S�̈����������Ă��܂��B<BR>
	 * - "obj" �̓R�[���o�b�N�֐����Ŏg�p����I�u�W�F�N�g�ł��B
	 *   �R�[���o�b�N�֐����Ăяo���ۂ́A�֐��o�^���Ɏw�肳�ꂽobj��K�����̈����ɓ���Ă��������B<BR>
	 * - "nch" �� CriMvSoundInterface ���v������I�[�f�B�I�̃`���l�����ł��B���m�����Ȃ�1�B�X�e���I�Ȃ�2�B5.1ch�Ȃ�6�ƂȂ�܂��B<BR>
	 * - "pcmbuf" ��PCM�f�[�^���i�[���邽�߂̃o�b�t�@�|�C���^�z��ł��B<br>
	 *   �o�b�t�@�̎��̂� CriMvSoundInterface �ŏ������Ă��������B�o�b�t�@�̐���"nch"�Ɠ����łȂ���΂����܂���B<BR>
	 * - "req_nsmpl" �� CriMvSoundInterface ���v������PCM�f�[�^�̍ő�T���v�����ł��B<br>
	 *   "pcmbuf"�Ŏw�肵���e�o�b�t�@���̂ɂ́A���̃T���v�������������܂�Ă����v�Ȃ����̗̈��K���������Ă��������B<BR>
	 * 
	 * �o�^���ꂽ�R�[���o�b�N�֐����Ăяo���^�C�~���O�� CriMvSoundInterface �̔C�ӂƂȂ�܂��B
	 * 
	 * \sa CriMvSoundInterface::SetCallbackGetSint16PcmData()
	 */
	/* pure */ virtual void SetCallbackGetFloat32PcmData(CriUint32 (*func)(void *obj, CriUint32 nch, CriFloat32 *pcmbuf[], CriUint32 req_nsmpl), void *obj)=0;

	/*EN
	 * \brief       Sets the callback function for retrieving 16 bit integer PCM sound samples
	 * \param       func            Function that will be called when 16 bit integer PCM data is needed
	 * \param       obj             Pointer to user-specifed data, passed as the first argument to the callback
	 *
	 * Sets a function that CriMvSoundInterface will call when it needs audio data in 16 bit integer
	 * PCM format.  This callback function takes 4 arguments:<br>
	 *
	 * - \a obj:        The user-specified \a obj parameter passed to SetCallbackGetSint16PcmData().<br>
	 * - \a nch:        The number of audio channels.  Mono is 1, stereo is 2, 5.1 channel is 6.<br>
	 * - \a pcmbuf:     An array of buffers to hold the returned PCM data.  There must be one element of 
	 *                  this array for each channel.<br>
	 * - \a req_nsmpl:  The number of samples requested.  Each buffer must be large enough to hold this
	 *                  many samples (i.e. \a nch * \a req_nsmpl).
	 *
	 * \remarks
	 * The maximum number of channels is CRIMV_PCM_BUFFER_MAX (currently 8)
	 * 
	 * \sa CriMvSoundInterface::SetCallbackGetFloat32PcmData()
	 */
	/*JP
	 * \brief		16bit�`����PCM�f�[�^���擾����R�[���o�b�N�֐��̓o�^
	 *  \param	func	CriMvSoundInterface ��PCM�f�[�^��v������ۂɌĂт����R�[���o�b�N�֐�
	 *  \param	obj		�R�[���o�b�N�֐������s����ۂɑ������Ɏw�肷��I�u�W�F�N�g�ϐ�
	 * 
	 * PCM�t�H�[�}�b�g���Ⴄ�ȊO�́A CriMvSoundInterface::SetCallbackGetFloat32PcmData() �Ɠ����ł��B
	 * 
	 * \sa CriMvSoundInterface::SetCallbackGetFloat32PcmData()
	 */
	/* pure */ virtual void SetCallbackGetSint16PcmData(CriUint32 (*func)(void *obj, CriUint32 nch, CriSint16 *pcmbuf[], CriUint32 req_nsmpl), void *obj)=0;

	/*EN
	 * \brief       Starts sound output
	 *
	 * CRI Movie will call this method when it needs to start playing sound.  The PCM data callback function
	 * has to be called after this function until movie playback is finished.<br>
	 *
	 * Your sound module should begin incrementing its playback time when this method is called.
	 *
	 * \remarks
	 * On success, this should set the status to CriMvSoundInterface::MVEASY_SOUND_STATUS_EXEC.
	 */
	/*JP
	 * \brief		�T�E���h�o�͂̊J�n
	 * 
	 * �T�E���h�o�͂��J�n���܂��BPCM�f�[�^�擾�p�R�[���o�b�N�֐��́A�{�֐��̌ďo���ォ����s���Ă��������B
	 */
	/* pure */ virtual void Start(void)=0;

	/*EN
	 * \brief       Stops sound output
	 *
	 * CRI Movie will call this when the movie playback stops, or the status changes to MVEASY_STATUS_PLAYEND.<br>
	 *
	 * When EasyPlayer wants to pause and restart playback, it will call CriMvSoundInterface::Pause()
	 * instead of this method.<br>
	 *
	 * Your sound module should set its playback time to 0 when this method is called.
	 * 
	 * \remarks
	 * This should set the status to CriMvSoundInterface::MVEASY_SOUND_STATUS_STOP.
	 * 
	 * \sa CriMvSoundInterface::Pause(), CriMvSoundInterface::Start()
	 */
	/*JP
	 * \brief		�T�E���h�o�͂̒�~
	 * 
	 * �T�E���h�o�͂��~���܂��B�ĊJ�ł���悤�ɂ���K�v�͂���܂���B<br>
	 * EasyPlayer���ĊJ�������s�������ꍇ�́A�{�֐��ł͂Ȃ��A CriMvSoundInterface::Pause()���Ăяo���܂��B<br>
	 * CriMvSoundInterface::Stop() �ďo����́A�R���[�o�b�N�֐����Ă΂Ȃ��悤�Ɏ������Ă��������B
	 * 
	 * \sa CriMvSoundInterface::Pause(), CriMvSoundInterface::Stop()
	 */
	/* pure */ virtual void Stop(void)=0;

	/*EN
	 * \brief       Gets status of sound module
	 * \return      The module status
	 *
	 * This must return one of the enumerated values in CriMvSoundInterface::Status.<br>
	 *
	 * While this method returns CriMvSoundInterface::MVEASY_SOUND_STATUS_EXEC, EasyPlayer will call the
	 * PCM data callback.<br>
	 * When a movie has finished playing and this method returns CriMvSoundInterface::MVEASY_SOUND_STATUS_STOP, 
	 * EasyPlayer will call CriMvSoundInterface::DestroyOutput().
	 * 
	 * \sa  CriMvSoundInterface::Status
	 */
	/*JP
	 * \brief		�T�E���h���W���[���̏�Ԏ擾
	 * 
	 * �T�E���h���W���[���̏�Ԃ��擾���܂��B
	 * 
	 * \sa  CriMvSoundInterface::Status
	 */
	/* pure */ virtual Status GetStatus(void)=0;

	/*EN
	 * \brief       Pauses or resumes sound output
	 * \param       sw          Pause or resume playback.
	 *
	 * If \a sw is 1 (ON), output will be paused.<br>
	 * If \a sw is 0 (OFF), output will be resumed.<br>
	 *
	 * Temporarily pauses or resumes sound output.<br>
	 *
	 * When you pause sound output, you must pause your playback timer as well.
	 */
	/*JP
	 * \brief		�T�E���h�o�͂̈ꎞ��~�܂��͍ĊJ
	 *  \param	sw		�|�[�Y�X�C�b�`�B�|�[�YON�̏ꍇ��1�A�|�[�YOFF(���W���[��)�̏ꍇ��0���w�肵�܂��B
	 * 
	 * �{�֐��̓���͈����Ɉˑ����܂��B<br>
	 * ���� sw ��ON(1)�Ȃ�A�ꎞ��~�B���� sw ��OFF(0)�Ȃ�T�E���h�o�͍ĊJ�ł��B
	 */
	/* pure */ virtual void Pause(CriBool sw)=0;

	/*EN
	 * \brief       Gets the time, in seconds, that sound has been playing
	 * \param       count       Playback time counter
	 * \param       unit        Counter increment per second
	 *
	 * CRI Movie calls this method periodically for some damn reason.<br>
	 * <br>
	 * The time, in seconds, is specified by \a count / \a unit.<br>
	 *
	 * For example, if \a count was 500 and \a unit was 1000, that would be 0.5 seconds.<br>
	 *
	 * \remarks
	 * Before Start() is called, and after Stop() is called, \a count should be 0.
	 */
	/*JP
	 * \brief		�Đ������̎擾
	 *  \param	count	�^�C�}�J�E���g
	 *  \param	unit	�P�b������̃^�C�}�J�E���g�l�Bcount �� unit �ŕb�P�ʂ̎����ƂȂ�܂��B
	 * 
	 * �^�C�}�������擾���܂��B������count��unit�̓�̕ϐ��ŕ\�����܂��B<br>
	 * count �� unit �ŕb�P�ʂ̎����ƂȂ�悤�Ȓl��Ԃ��܂��B<br>
	 * �Đ��J�n�O�i CriMvSoundInterface::Start()�Ăяo���O�j�����
	 * �Đ���~��i CriMvSoundInterface::Stop()�Ăяo����j�́A�����O�i�^�C�}�J�E���g���O�j��Ԃ��܂��B
	 */
	/* pure */ virtual void GetTime(CriUint64 &count, CriUint64 &unit)=0;		// sec = count / unit.

protected:
	virtual ~CriMvSoundInterface(void) {}
};

/*EN
 * \brief	System Timer Interface Class for EasyPlayer
 * \ingroup MDL_IF_TIMER
 *
 * If you want to synchronize video frames with something other than the audio track, or if you
 * need to play a movie that does not have an audio track, you will need to pass an instance of
 * a class derived from CriMvSystemTimerInterface to CriMvEasyPlayer::Create().<br>
 *
 * If you do not need any special timer facilities, you can pass NULL to CriMvEasyPlayer::Create() instead.<br>
 *
 * All functions are pure virtual.
 *
 * \sa CriMvEasyPlayer::Create(), CriMvEasyPlayer::SyncMasterTimer()
 */
/*JP
 * \brief	�V�X�e���^�C�}�[�C���^�t�F�[�X for EasyPlayer
 * \ingroup MDL_IF_TIMER
 * 
 * �V�X�e���^�C�}�[�͉��������[�r�Đ����ɁA�r�f�I�t���[���̑��o�^�C�~���O�𒲐����邽�߂Ɏg�p����܂��B<br>
 * ���̃N���X���`���邱�ƂŁA���O�̃^�C�}�V�X�e�����g����EasyPlayer�̃X�g���[�~���O�Đ����\�ɂȂ�܂��B<br>
 * �S�Ă̊֐��͏������z�֐��Ƃ��Ē�`����Ă���̂ŁA�S�Ă̊֐���K���������Ă��������B
 * 
 * \sa CriMvEasyPlayer::Create(), CriMvEasyPlayer::SyncMasterTimer()
 */
class CriMvSystemTimerInterface
{
public:
	/*EN
	 * \brief       Starts timer and resets it to 0
	 *
	 * When this method is called, your internal counter should initialize itself to 0 and start
	 * normal operation.
	 */
	/*JP
	 * \brief		�^�C�}�J�n
	 * 
	 * �^�C�}�̃J�E���g���J�n���܂��B���̊֐����Ă΂ꂽ���������O�ƂȂ�܂��B
	 */
	/* pure */ virtual void Start(void)=0;

	/*EN
	 * \brief       Stops the timer and resets it to 0
	 *
	 * When this method is called, your internal counter should stop incrementing, and re-initialize
	 * itself to 0.  After this method has been called, CriMvSystemTimerInterface::GetTime() must
	 * return a time of 0 seconds.
	 */
	/*JP
	 * \brief		�^�C�}��~
	 * 
	 * �^�C�}�̃J�E���g���~���܂��B���̊֐����Ă΂ꂽ���ƂɁA���̃^�C�}���ĊJ���邱�Ƃ͂���܂���B
	 */
	/* pure */ virtual void Stop(void)=0;

	/*EN
	 * \brief       Pauses or resumes the timer
	 *
	 * \param       sw          Pause or resume timer operation.
	 *
	 * If \a sw is 1 (ON), the timer will be paused.<br>
	 * If \a sw is 0 (OFF), the timer will be resumed.<br>
	 *
	 * Temporarily pauses or resumes the timer.<br>
	 *
	 * When you pause the timer, you must maintain the previous value of the counter.
	 */
	/*JP
	 * \brief		�^�C�}�̈ꎞ��~�܂��͍ĊJ
	 *  \param	sw		�|�[�Y�X�C�b�`�BON(1)�Ȃ�ꎞ��~�AOFF(0)�Ȃ�ĊJ�B
	 * 
	 * �{�֐��̓���͈����Ɉˑ����܂��B<br>
	 * ���� sw ��ON(1)�Ȃ�A�ꎞ��~�B���� sw ��OFF(0)�Ȃ�^�C�}�J�E���g�ĊJ�ł��B
	 */
	/* pure */ virtual void Pause(CriBool sw)=0;

	/*EN
	 * \brief       Gets the time, in seconds, that the timer has been running
	 *
	 * \param       count       Timer counter
	 * \param       unit        Counter increment per second
	 *
	 * CRI Movie calls this method periodically to synchronize video playback with the
	 * movie's internal framerate.<br>
	 * <br>
	 * The time, in seconds, is specified by \a count / \a unit.<br>
	 *
	 * For example, if \a count was 500 and \a unit was 1000, that would be 0.5 seconds.<br>
	 *
	 * \remarks
	 * Before Start() is called, and after Stop() is called, \a count should be 0.
	 */
	/*JP
	 * \brief		�o�ߎ����̎擾
	 *  \param	count	�^�C�}�J�E���g
	 *  \param	unit	�P�b������̃^�C�}�J�E���g�l�Bcount �� unit �ŕb�P�ʂ̎����ƂȂ�܂��B
	 * 
	 * �^�C�}�������擾���܂��B������count��unit�̓�̕ϐ��ŕ\�����܂��B<br>
	 * count �� unit �ŕb�P�ʂ̎����ƂȂ�悤�Ȓl��Ԃ��܂��B<br>
	 * �Đ��J�n�O�i CriMvSystemTimerInterface::Start()�Ăяo���O�j�����
	 * �Đ���~��i CriMvSystemTimerInterface::Stop()�Ăяo����j�́A�����O�i�^�C�}�J�E���g���O�j��Ԃ��܂��B
	 */
	/* pure */ virtual void GetTime(CriUint64 &count, CriUint64 &unit)=0;

protected:
	virtual ~CriMvSystemTimerInterface(void) {}
};


/*EN
 * \brief	EasyPlayer Interface class for CRI Movie
 * \ingroup MDL_EASY_PLAYER
 */
/*JP
 * \brief	EasyPlayer�C���^�t�F�[�X
 * \ingroup MDL_EASY_PLAYER
 */
class CriMvEasyPlayer : public CriAllocator
{
public:
	/*EN
	 * \brief       The possible states an EasyPlayer handle can be in.
	 *
	 * An EasyPlayer handle takes on various states, depending on where it is in the decoding process.
	 * You can check the status of a valid EasyPlayer handle at any time by calling CriMvEasyPlayer::GetStatus().
	 *
	 * An EasyPlayer handle is created in the <b>MVEASY_STATUS_STOP</b> state.  During movie playback, the status
	 * transitions through various states from <b>MVEASY_STATUS_STOP</b> to <b>MVEASY_STATUS_PLAYEND</b>.
	 *
	 * An application does not need to check all states.  At a minimum, it only needs to check for 
	 * <b>MVEASY_STATUS_STOP</b>, <b>MVEASY_STATUS_PLAYING</b>, <b>MVEASY_STATUS_PLAYEND</b>, 
	 * and <b>MVEASY_STATUS_ERROR</b>.
	 *
	 * If an application calls CriMvEasyPlayer::DecodeHeader(), the status of the EasyPlayer handle will change to
	 * <b>MVEASY_STATUS_WAIT_PREP</b> when CRI Movie has finished analyzing the movie information.  The EasyPlayer 
	 * handle will remain in this state until the application calls CriMvEasyPlayer::Prepare() or 
	 * CriMvEasyPlayer::Start().  Once the state has changed to <b>MVEASY_STATUS_WAIT_PREP</b>, information
	 * about the movie can be retrieved by calling CriMvEasyPlayer::GetMovieInfo().
	 *
	 * If an application calls CriMvEasyPlayer::Prepare(), the EasyPlayer handle status will change to 
	 * <b>MVEASY_STATUS_READY</b> once CRI Movie has finished buffering enough input and decoded output for playback.
	 * The handle will remain in this state until CriMvEasyPlayer::Start() is called.  This allows the application
	 * to better control playback timing, since a movie can immediately start playing.
	 *
	 * Once CriMvEasyPlayer::Start() is called and the movie is actively playing, the status will alternate
	 * between <b>MVEASY_STATUS_PLAYING</b> and <b>MVEASY_STATUS_PREP</b>, as CRI Movie plays back frames and decodes
	 * new ones.
	 *
	 * When the movie has finished playing normally, the status of the EasyPlayer handle will automatically change
	 * to <b>MVEASY_STATUS_PLAYEND</b>.  If the movie is in looping mode, however, once the movie reaches the 
	 * end, it will start playing from the beginning and the status will not change to <b>MVEASY_STATUS_PLAYEND</b>.
	 *
	 * When CriMvEasyPlayer::Stop() is called, the status of the handle will change to <b>MVEASY_STATUS_STOP</b> 
	 * once it has finished any decoding and playback that is in progress.  This does not happen immediately,
	 * but will take a few cycles.
	 *
	 * If there are any problems during playback, for instance insufficient memory or invalid input data, the
	 * status will change to <b>MVEASY_STATUS_ERROR</b>.  When the handle is in state <b>MVEASY_STATUS_ERROR</b>,
	 * the application must call CriMvEasyPlayer::Stop() and wait until the state changes to <b>MVEASY_STATUS_STOP</b>
	 * before doing anything else with the handle.
	 *
	 * Once the state is <b>MVEASY_STATUS_STOP</b> or <b>MVEASY_STATUS_PLAYEND</b>, the handle can be deleted by
	 * calling CriMvEasyPlayer::Destroy().
	 *
	 * \attention
	 * As of CRI Movie version 2.00, the handling of the <b>MVEASY_STATUS_ERROR</b> state has changed.  Previously,
	 * a handle could be destroyed when it was in the <b>MVEASY_STATUS_ERROR</b> state.  Now, an application 
	 * must call CriMvEasyPlayer::Stop() and wait for the <b>MVEASY_STATUS_STOP</b> state before destroying
	 * the handle.
	 *
	 * \sa CriMvEasyPlayer::GetStatus(), CriMvEasyPlayer::Start(), CriMvEasyPlayer::DecodeHeader(), 
	 *     CriMvEasyPlayer::Prepare(), CriMvEasyPlayer::GetMovieInfo(), CriMvEasyPlayer::Stop(), 
	 *     CriMvEasyPlayer::Destroy()
	 */
	/*JP
	 * \brief		EasyPlayer�n���h�����
	 * 
	 * EasyPlayer �̃n���h����Ԃł��B
	 * �n���h����Ԃ� CriMvEasyPlayer::GetStatus() �ł��ł��擾���邱�Ƃ��o���܂��B<br>
	 * �n���h���쐬����� MVEASY_STATUS_STOP ��Ԃł��B
	 * 
	 * �n���h����Ԃ� MVEASY_STATUS_STOP ���� MVEASY_STATUS_PLAYEND �܂ŏ��ɑJ�ڂ��Ă����܂��B<br>
	 * �A�v���P�[�V���������[�r���Đ�����ɂ������āA�K���������̑S�Ă̏�Ԃ��`�F�b�N����K�v�͂���܂���B<br>
	 * �Œ���AMVEASY_STATUS_STOP, MVEASY_STATUS_PLAYING, MVEASY_STATUS_PLAYEND, MVEASY_STATUS_ERROR ����
	 * �`�F�b�N����΁A���[�r�̍Đ����s�����Ƃ��ł��܂��B
	 * 
	 * EasyPlayer �n���h���쐬��A���[�r�̉𑜓x�Ȃǂ����Ɋm�肵�Ă���ꍇ�́A�A�v���P�[�V������
	 * CriMvEasyPlayer::Start() �𒼐ڌĂяo�����Ƃ��ł��܂��B���̏ꍇ�A�n���h����Ԃ͎����I��
	 * MVEASY_STATUS_PLAYEND �܂őJ�ڂ��Ă����܂��B
	 * 
	 * �ŏ��� CriMvEasyPlayer::DecodeHeader() ���Ăяo�����ꍇ�́A�w�b�_��͂��I������ƃn���h����Ԃ�
	 * MVEASY_STATUS_WAIT_PREP �ƂȂ�A�A�v���P�[�V�������� CriMvEasyPlayer::Prepare() �܂���
	 * CriMvEasyPlayer::Start() ���Ă΂��܂őҋ@���܂��B
	 * 
	 * MVEASY_STATUS_WAIT_PREP��Ԉȍ~�A CriMvEasyPlayer::GetMovieInfo() �Ń��[�r�����擾���邱�Ƃ��ł��܂��B<br>
	 * CriMvEasyPlayer::Prepare() ���Ăяo�����ꍇ�́A�w�b�_��͂���уf�[�^�̃o�b�t�@�����O���I���ƁA
	 * �n���h����Ԃ� MVEASY_STATUS_READY �ƂȂ�A�A�v���P�[�V�������� CriMvEasyPlayer::Start() ��
	 * �Ă΂��܂őҋ@���܂��B����ɂ���čĐ��J�n�̃^�C�~���O�𒲐����邱�Ƃ��ł��܂��B
	 * 
	 * �Đ����I������Ǝ����I�� MVEASY_STATUS_PLAYEND �ɂȂ�܂��B
	 * 
	 * CriMvEasyPlayer::Stop() ���Ăяo�����ꍇ�́A�f�R�[�_�̒�~�������I��������Ƃ� MVEASY_STATUS_STOP
	 * ��ԂɂȂ�܂��B CriMvEasyPlayer::Stop() �I������ɒ�~��ԂɂȂ�Ƃ͌���܂���B
	 * 
	 * �������s����f�[�^�G���[�Ȃǉ��炩�̖�肪���������ꍇ�� MVEASY_STATUS_ERROR ��ԂƂȂ�܂��B<BR>
	 * MVEASY_STATUS_ERROR ��ԂɂȂ����ꍇ�� CriMvEasyPlayer::Stop() ���Ăяo���ăn���h����Ԃ�
	 * MVEASY_STATUS_STOP ��ԂɑJ�ڂ����Ă��������B<BR>
	 * 
	 * CriMvEasyPlayer::Destroy() �� MVEASY_STATUS_STOP, MVEASY_STATUS_PLAYEND ��
	 * �����ꂩ�̏�Ԃ̎��̂݌Ăяo�����Ƃ��ł��܂��B
	 * 
	 * \attention
	 * CRI Movie Ver.2.00 �� MVEASY_STATUS_ERROR ��Ԃɂ��Ă̎d�l���ύX�ɂȂ�܂����B<BR>
	 * MVEASY_STATUS_ERROR ��ԂŃn���h���j�����o���Ȃ��Ȃ�A CriMvEasyPlay::Stop() ���Ăяo���K�v������܂��B
	 * 
	 * \sa CriMvEasyPlayer::GetStatus(), CriMvEasyPlayer::Start(), CriMvEasyPlayer::DecodeHeader(), 
	 *     CriMvEasyPlayer::Prepare(), CriMvEasyPlayer::GetMovieInfo(), CriMvEasyPlayer::Stop(), 
	 *     CriMvEasyPlayer::Destroy()
	 */
	enum Status {
		MVEASY_STATUS_STOP,			/*EN< Standstill.  No processing is happening.
									 *    EasyPlayer handles are created in this state. */
									/*JP< ��~�� */
		MVEASY_STATUS_DECHDR,		/*EN< The EasyPlayer handle is now parsing the movie header, 
									 *    including information about the width and height of the video stream. */
									/*JP< �w�b�_��͒� */
		MVEASY_STATUS_WAIT_PREP,	/*EN< The EasyPlayer handle is a waiting for the work buffer to be allocated. */
									/*JP< �o�b�t�@�����O�J�n�ҋ@�� */
		MVEASY_STATUS_PREP,			/*EN< The EasyPlayer handle is now buffering video and audio data. */
									/*JP< �Đ������� */
		MVEASY_STATUS_READY,		/*EN< Ready to start playback. */
									/*JP< �Đ��ҋ@ */
		MVEASY_STATUS_PLAYING,		/*EN< The decoders are currently decoding and playing output. */
									/*JP< �Đ��� */
		MVEASY_STATUS_PLAYEND,		/*EN< The end of the movie has been reached. */
									/*JP< �Đ��I�� */
		MVEASY_STATUS_ERROR,		/*EN< An error has occurred. */
									/*JP< �G���[ */

		/* Keep enum 4bytes */
		MVEASY_STATUS_MAKE_ENUM_SINT32 = 0x7FFFFFFF
	};

	/*EN
	 * \brief		Supported timer types; used to synchronize video frames.
	 */
	/*JP
	 * \brief		�^�C�}���
	 */
	enum TimerType {
		MVEASY_TIMER_NONE,		/*EN< No synchronization.  The output is available as soon as
								 *    each frame is decoded. */
								/*JP< �r�f�I�t���[���͎������������܂���B�f�R�[�h���I������t���[��
								 *    �͂����Ɏ擾���邱�Ƃ��ł��܂��B */
		MVEASY_TIMER_SYSTEM,	/*EN< Video frames synchronize to the system timer.<br>
								 *    You must provide an instance of CriMvSystemTimerInterface to
								 *    CriMvEasyPlayer::Create().                                                     */
								/*JP< �r�f�I�t���[���̓V�X�e�������ɓ������܂��B�V�X�e�������̓A�v���P�[�V����
								 *    �� CriMvSystemTimerInterface �Ƃ���CriMvEasy�n���h���ɐݒ肷��K�v������܂��B */
		MVEASY_TIMER_AUDIO,		/*EN< Video frames synchronize with the movie's audio data.<br>
								 *    You must provide an instance of CriMvSoundInterface to CriMvEasyPlayer::Create(). <br> 
								 *    If the movie does not have audio, video frames will synchronize with the system timer. */
								/*JP< �r�f�I�t���[���̓��[�r�̃I�[�f�B�I�����ɓ������܂��B
								 *    �A�v���P�[�V������ GetTime�֐����܂� CriMvSoundInterface ��CriMvEasy
								 *    �n���h���ɐݒ肷��K�v������܂��B���������[�r�f�[�^�ɃI�[�f�B�I���܂܂��
								 *    ���Ȃ��ꍇ�́A�r�f�I�̓V�X�e�������ɓ������܂��B */

		/* Keep enum 4bytes */
		MVEASY_TIMER_MAKE_ENUM_SINT32 = 0x7FFFFFFF
	};

	/*EN
	 * \ingroup     MODULE_INIT
	 *
	 * \brief       Creates an EasyPlayer handle
	 *
	 * \param       heap        Handle to a CriHeap object
	 * \param       freader     File input interface
	 * \param       stimer      System timer interface
	 * \param       sound       Sound module interface
	 * \param       err         Optional error code
	 *
	 * \return      A valid CriMvEasyPlayer handle, or NULL if the handle cannot be allocated
	 *
	 * Creates and initialize a new EasyPlayer handle.  Its status is initially MVEASY_STATUS_STOP.
	 * Memory for the handle is allocated from the provided CriHeap object.
	 *
	 * If memory allocation fails, this function will return NULL.  Be sure to initialize and create
	 * your heap with criHeap_Initialize() and criHeap_Create() before calling this function.
	 *
	 * \remarks
	 * CriMv::Initialize() must be called <b>before</b> calling this function.
	 * 
	 */
	/*JP
	 * \ingroup MODULE_INIT
	 * \brief		EasyPlayer�n���h���̍쐬
	 *  \param		heap	CriHeap�n���h��
	 *  \param		freader	�t�@�C���ǂݍ��݃C���^�t�F�[�X
	 *  \param		stimer	�V�X�e���^�C�}�C���^�t�F�[�X
	 *  \param		sound	�T�E���h�C���^�t�F�[�X
	 *  \param		err		�G���[���
	 *  \return		CriMvEasy�n���h����Ԃ��܂��B�G���[�����������ꍇ�́ANULL��Ԃ��܂��B
	 *
	 * �{�֐��� CriMv::Initialize() �Ăяo������<B>��</B>�Ɏ��s���Ă��������B<BR>
	 * �n���h���쐬��̓n���h����Ԃ�MVEASY_STATUS_STOP�ƂȂ�܂��B<br>
	 * �n���h���m�ۂɕK�v�ȃ������͑S�āA�����œn���ꂽ CriHeap ���g���Ċm�ۂ���܂��B<br>
	 * �������s���ȂǂŃG���[�����������ꍇ�́A�{�֐���NULL��Ԃ��܂��B
	 * 
	 */
	static CriMvEasyPlayer* CRIAPI Create(CriHeap heap, 
								   CriMvFileReaderInterface *freader, 
								   CriMvSystemTimerInterface *stimer, 
								   CriMvSoundInterface *sound, 
								   CriError &err=CriMv::ErrorContainer);
	/*EN
	 * \ingroup MODULE_INIT
	 * \brief		Destroy a handle
	 *  \param		err		Optional error code 
	 *
	 * \ingroup     MODULE_INIT
	 *
	 * \brief       Destroys an EasyPlayer handle
	 * \param       err         Optional error code
	 *
	 * Destroys an EasyPlayer handle previously created with CriMvEasyPlayer::Create(), and frees its resources.
	 *
	 * An EasyPlayer handle can only be destroyed when it is in the MVEASY_STATUS_STOP or MVEASY_STATUS_PLAYEND state.
	 * Attempting to destroy a handle when it is in any other state will cause an error.
	 *
	 * Any work buffers allocated via CriHeap, if still associated with the handle, are freed by this call.
	 *
	 * \sa CriMvEasyPlayer::Create(), CriMvEasyPlayer::Status, CriMvEasyPlayer::GetStatus()
	 */
	/*JP
	 * \ingroup MODULE_INIT
	 * \brief		EasyPlayer�n���h���̔j��
	 *  \param		err		�G���[���i�ȗ��j
	 *
	 * CriMvEasyPlayer::Create()�ō쐬����EasyPlayer�n���h����j�����܂��B
	 *
	 * �n���h����Ԃ� MVEASY_STATUS_STOP �A MVEASY_STATUS_PLAYEND �̎��ɂ̂݃n���h����j�����邱�Ƃ��ł��܂��B<br>
	 * ����ȊO�̏�ԂŌĂяo�����ꍇ�́A�G���[�ɂȂ�܂��B<br>
	 * 
	 * �n���h����Ԃ� MVEASY_STATUS_ERROR �������ꍇ�́ACriMvEasyPlayer::Stop() ���Ăяo����
	 * MVEASY_STATUS_STOP ��ԂɂȂ��Ă���n���h���j�����Ă��������B<BR>
	 * �n���h����Ԃ� CriMvEasyPlayer::GetStatus() �Ŋm�F���邱�Ƃ��ł��܂��B
	 *
	 * �n���h���쐬���Ɏw�肵��CriHeap�ɂ���Ċm�ۂ��ꂽ�������Ŗ�����̑S�ẮA
	 * ���̊֐��̌Ăяo���ɂ���ĉ������܂��B
	 *
	 * \sa CriMvEasyPlayer::Create(), CriMvEasyPlayer::Status, CriMvEasyPlayer::GetStatus()
	 */
	void Destroy(CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup     MODULE_BASIC
	 *
	 * \brief       Returns status of an EasyPlayer handle.
	 * \param       err         Optional error code
	 * \return      Handle status
	 *
	 * Returns the current status of an EasyPlayer handle.  The status will be one of the values 
	 * defined by CriMvEasyPlayer::Status.
	 *
	 * \sa CriMvEasyPlayer::Status
	 */
	/*JP
	 * \ingroup MODULE_BASIC
	 * \brief		�n���h����Ԃ̎擾
	 *  \param		err		�G���[���i�ȗ��j
	 *  \return		�n���h����� CriMvEasyPlayer::Status
	 *
	 *  �n���h����Ԃ��擾���܂��B
	 *
	 * \sa CriMvEasyPlayer::Status
	 */
	Status GetStatus(CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup     MODULE_BASIC
	 *
	 * \brief       Executes heatbeat functions for an EasyPlayer handle
	 * \param       err         Optional error code
	 *
	 * Performs regular processing for an EasyPlayer handle, including handoff and parsing of input
	 * buffers, and audio decoding.  Additionally, it checks for situations where the handle state
	 * should change to one of the values defined by CriMvEasyPlayer::Status, and changes state as
	 * necessary.
	 *
	 * This function and CriMvEasyPlayer::ExecuteDecode() should be called periodically during movie 
	 * playback or when waiting for CriMvEasyPlayer::DecodeHeader() to complete.  It takes a relatively 
	 * low CPU load, and should typically be called on every vertical blank.
	 *
	 * \remarks
	 * Note that this function does not perform any video decoding.  Decoding is done in 
	 * CriMvEasyPlayer::ExecuteDecode().
	 * 
	 * \sa CriMvEasyPlayer::Status, CriMvEasyPlayer::ExecuteDecode(), CriMvEasyPlayer::DecodeHeader()
	 */
	/*JP
	 * \ingroup MODULE_BASIC
	 * \brief		EasyPlayer�T�[�o�֐�
	 *  \param		err		�G���[���i�ȗ��j
	 *
	 * ���[�r�̃w�b�_��͂���̓o�b�t�@����A�I�[�f�B�I�f�R�[�h�����s���܂��B<br>
	 * EasyPlayer �n���h���̏�ԑJ�ڂ����̊֐��ōs���܂��B<br>
	 * ���̊֐��̓r�f�I�̃f�R�[�h�͍s���܂���B���̂���CPU���ׂ͂��܂荂���Ȃ�܂���B<br>
	 * �{�֐��̓A�v���P�[�V�����̃��C�����[�v�Ŗ���Ăяo���悤�ɂ��Ă��������B
	 * 
	 * \sa CriMvEasyPlayer::Status
	 */
	void Update(CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup     MODULE_BASIC
	 * 
	 * \brief       Synchronizes the master timer of an EasyPlayer handle.
	 * \param       err         Optional error code
	 *
	 * Synchronizes the timing of video frames to the master timer used by this EasyPlayer handle.
	 * If the master timer is of type MVEASY_TIMER_AUDIO and the movie does not have audio,
	 * EasyPlayer will use the system timer that was set when the handle was created.
	 *	 
	 * \sa CriMvEasyPlayer::TimerType, CriMvEasyPlayer::SetMasterTimer(), CriMvSystemTimerInterface
	 */
	/*JP
	 * \ingroup MODULE_BASIC
	 * \brief		�}�X�^�^�C�}�ւ̓���
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * ���[�r�Đ��������}�X�^�^�C�}�ɓ��������܂��B<br>
	 * �}�X�^�^�C�}�� CriMvEasyPlayer::SetMasterTimer() �ɂ���Ďw�肳�ꂽ�^�C�}���g���܂��B<br>
	 * �^�C�}��ʂƂ��� MVEASY_TIMER_AUDIO ���w�肳��Ă��āA�Đ����郀�[�r�ɃI�[�f�B�I��
	 * �܂܂�Ă��Ȃ��ꍇ�́A�n���h���쐬���̃V�X�e���^�C�}���g�p���܂��B
	 * 
	 * ���̊֐��́A�n���h���쐬���Ɏw�肵���V�X�e���^�C�}�C���^�t�F�[�X��
	 * CriMvSystemTimerInterface::GetTime() ���Ăяo���܂��B
	 * 
	 * \sa CriMvEasyPlayer::TimerType, CriMvEasyPlayer::SetMasterTimer(), CriMvSystemTimerInterface
	 */
	void SyncMasterTimer(CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup     MODULE_BASIC
	 *
	 * \brief       Decodes a video frame
	 * \param       err         Optional error code
	 * \return      \a TRUE     If the movie is currently in a playback state
	 * \return      \a FALSE    If the movie is not playing
	 *
	 * Performs the actual video decoding.  Each call to this function decodes a single frame.
	 * It does not return until the frame has been decoded.
	 *
	 * Video decoding can impose a very heavy CPU load, so calling this function from an application's
	 * main thread may cause your application to miss the vertical blank interval.  To avoid this situation,
	 * ExecuteDecode() can be called from a separate, lower-priority thread.  See the <b>Mutithreaded Decoding</b>
	 * tutorial for a full explanation.
	 *
	 * The return value describes the movie playback state.  If TRUE, the movie is currently playing
	 * (or decoding the movie header) or ready to play.  If FALSE, playback has not yet been started, 
	 * or playback has finished.
	 *
	 * If a decoding thread is used, an application must wait until this function returns FALSE before destroying 
	 * the thread.  Otherwise, the handle state will not transition to MVEASY_STATUS_STOP or MVEASY_STATUS_PLAYEND,
	 * and the EasyPlayer handle cannot be destroyed.
	 * 
	 * \attention
	 * ExecuteDecode() is the only CRI Movie function that is safe to call from a separate thread.
	 * No other CRI Movie functions should be considered to be thead-safe.
	 * 
	 */
	/*JP
	 * \ingroup MODULE_BASIC
	 * \brief		�r�f�I�f�R�[�h
	 *  \param		err		�G���[���i�ȗ��j
	 *  \return				���[�r�Đ�����TRUE �A�Đ��I���܂���~���FALSE��Ԃ��܂��B
	 * 
	 * �r�f�I�f�[�^�̃f�R�[�h���s���܂��B<br>
	 * �{�֐��͂P�s�N�`�����̃f�R�[�h���I���܂ŏI�����܂���B<br>
	 * �s�N�`���f�R�[�h�͕��ׂ̍��������Ȃ̂ŁA�A�v���P�[�V�����̃��C���X���b�h����Ăяo���Ə�����������������\��������܂��B<br>
	 * ���̏ꍇ�́A���C���X���b�h�����D��x�̒Ⴂ�ʃX���b�h����Ăяo���悤�ɂ��Ă��������B
	 * 
	 * �{�֐��̕Ԃ�l�́A���[�r�Đ��̎��s�����ǂ�����\���Ă��܂��B<BR>
	 * �f�R�[�h�p�X���b�h���I������ꍇ�́A�Ԃ�l��FALSE�ɂȂ�̂�҂��Ȃ���΂����܂���B<BR>
	 * �Ԃ�l��TRUE�̊ԂɃf�R�[�h�X���b�h���I�����Ă��܂��ƁA�n���h���̏�Ԃ� MVEASY_STATUS_STOP ��
	 *  MVEASY_STATUS_PLAYEND �ɑJ�ڂł����A�n���h���j�����o���Ȃ��Ȃ�܂��B<BR>
	 * 
	 */
	CriBool ExecuteDecode(CriError &err=CriMv::ErrorContainer);

	/* �Đ����� */
	/*EN
	 * \ingroup MODULE_CONTROL
	 *
	 * \brief       Sets the name of the movie file to play
	 * \param       fname       Name of the movie file
	 * \param       err         Optional error code
	 *
	 * Sets the name of the movie file to play, but does not open the file.  The length of the filename
	 * (including path) is limited to CRIMV_MAX_FILE_NAME characters.  EasyPlayer copies this 
	 * string to internal memory, so a temporary variable can be used.
	 *
	 * If an application plays the same movie repeatedly, it only needs to call this function once.
	 *
	 * Multiple calls to this function will overwrite previous values.  Calling CriMvEasyPlayer::SetData() 
	 * will clear any filename set by this function.
	 *
	 * \remarks
	 * This function can only be called when the handle status is MVEASY_STATUS_STOP or MVEASY_STATUS_PLAYEND,
	 * or from the file request callback.  See CriMvEasyPlayer::SetFileRequestCallback() for details.
	 *
	 * \remarks
	 * Calling this function does not open the file.  The file is opened by a call to CriMvEasyPlayer::Update().
	 *
	 * \sa CriMvEasyPlayer::SetFileRequestCallback(), CriMvEasyPlayer::SetData()
	 */
	/*JP
	 * \ingroup     MODULE_CONTROL
	 *
	 * \brief       �Đ��t�@�C���̎w��
	 * \param       fname       ���[�r�t�@�C���p�X
	 * \param		err			�G���[���i�ȗ��j
	 *
	 * �Đ����郀�[�r�̃t�@�C���p�X��ݒ肵�܂��B�t�@�C���p�X�̍ő咷�� CRIMV_MAX_FILE_NAME �o�C�g�ł��B<br>
	 * EasyPlayer�͓����ł��̃t�@�C���p�X���R�s�[����̂ŁA�����œn����������͔j�����Ă����܂��܂���B
	 *
	 * �����t�@�C�����J��Ԃ��Đ�����ꍇ�́A���̊֐����ēx�Ăяo���K�v�͂���܂���B
	 *
	 * ���̊֐����Ăяo��������ɂ�����x���̊֐����Ăяo���ƁA�O��̃t�@�C�����͐V�����t�@�C�����ɏ㏑������܂��B
	 * CriMvEasyPlayer::SetData()�֐����Ăяo�����ꍇ�́A���O�ɐݒ肵���t�@�C����񂪃N���A����܂��B
	 *
	 * \para ���l1�F
	 * ���̊֐��̓n���h����Ԃ� MVEASY_STATUS_STOP �������� MVEASY_STATUS_PLAYEND���̂݌Ăяo���\�ł��B
	 * �܂��̓t�@�C���v���R�[���o�b�N�֐����ł����̊֐����Ăяo�����Ƃ��ł��܂��B�ڍׂ� CriMvEasyPlayer::SetFileRequestCallback()�֐���
	 * �Q�Ƃ��Ă��������B
	 *
	 * \para ���l2�F
	 * ���̊֐��̓����ł̓t�@�C���̃I�[�v���v���͂��܂���B�t�@�C���̃I�[�v�������� CriMvEasyPlayer::Update() �֐��̒��ōs���܂��B
	 *
	 * \sa CriMvEasyPlayer::SetFileRequestCallback(), CriMvEasyPlayer::SetData(), CriMvEasyPlayer::SetFileRange()
	 */
	void SetFile(CriChar8 *fname, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup     MODULE_CONTROL
	 *
	 * \brief       Sets in-memory movie data 
	 * \param       dataptr     Pointer to movie data
	 * \param       datasize    Size of data, in bytes
	 * \param       err         Optional error code
	 *
	 * Sets the in-memory data buffer for this EasyPlayer handle to point to the provided buffer,
	 * but does not attempt to read that buffer.  After this call, the memory pointed to by \a dataptr
	 * belongs to CRI Movie and must remain valid until playback is complete.
	 *
	 * If an application plays the same movie data repeatedly, it only needs to call this function once.
	 *
	 * Multiple calls to this function will overwrite previous values.  Calling CriMvEasyPlayer::SetFile() 
	 * will clear any pointer set by this function.
	 *
	 * \remarks
	 * This function can only be called when the handle status is MVEASY_STATUS_STOP or MVEASY_STATUS_PLAYEND,
	 * or from the file request callback.  See CriMvEasyPlayer::SetFileRequestCallback() for details.
	 *
	 * \remarks
	 * Calling this function does not attempt to reference the memory.  The memory read is initiated by a call 
	 * to CriMvEasyPlayer::DecodeHeader(), CriMvEasyPlayer::Prepare() or CriMvEasyPlayer::Start().
	 * 
	 * \sa CriMvEasyPlayer::SetFileRequestCallback(), CriMvEasyPlayer::SetFile(), CriMvEasyPlayer::SetFileRange()
	 */
	/*JP
	 * \ingroup MODULE_CONTROL
	 * \brief		��������f�[�^�̎w��
	 *  \param		dataptr		�f�[�^�|�C���^
	 *  \param		datasize	�f�[�^�T�C�Y
	 *  \param		err			�G���[���i�ȗ��j
	 * 
	 * ���̊֐���EasyPlayer�̃n���h����Ԃ�MVEASY_STATUS_STOP��MVEASY_STATUS_PLAYEND�̎��ɌĂяo���Ă��������B<br>
	 * �܂��́A�t�@�C���v���R�[���o�b�N�̓����ŌĂяo�����Ƃ��ł��܂��B<BR>
	 * 
	 * �{�֐����J��Ԃ��Ăяo�����ꍇ�́A���������͏㏑������܂��B<BR>
	 * CriMvEasyPlayer::SetFile()���Ăяo�����ꍇ�́A�{�֐��Ŏw�肵�����������̓n���h���������������܂��B
	 * 
	 * �����n���h���œ������[�r�f�[�^���J��Ԃ��Đ�����ꍇ�́A�{�֐��̌Ăяo���͏ȗ����邱�Ƃ��ł��܂��B
	 * 
	 * �w�肳�ꂽ�������̈�Ɏ��ۂɃA�N�Z�X����̂́A CriMvEasyPlayer::DecodeHeader(), CriMvEasyPlayer::Prepare(), 
	 * CriMvEasyPlayer::Start() �̂����ꂩ���Ăяo���ꂽ���ȍ~�ł��B<br>
	 * 
	 * \sa CriMvEasyPlayer::SetFileRequestCallback(), CriMvEasyPlayer::SetFile(), CriMvEasyPlayer::SetFileRange()
	 */
	void SetData(CriUint8 *dataptr, CriUint32 datasize, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_CONTROL
	 * \brief		Set a pack file and pass offset and range of movie file to an EasyPlayer handle
	 *  \param		fname	File name string pointer of the pack file
	 *  \param		offset	offset in byte to movie data in the packing file [in bytes]
	 *  \param		range	size of movie data from the offset in the packing file [in bytes]
	 *  \param		err		Optional error code 
	 * 
	 * Sets the name of the pack file that contains the movie file to play.
	 * EasyPlayer tries to read data from 'offset' through 'range' in the pack file as the movie data.
	 * Passing a negative value as range, EasyPlayer reads data until the end of the pack file.
	 * 
	 * The length of file name is limited until CRIMV_MAX_FILE_NAME.
	 * EasyPlayer handle copies the file name string into the handle.
	 * You can use a temporary variable as the file name string.
	 * 
	 * If an application plays the same movie repeatedly, it only needs to call this function once.
	 *
	 * Multiple calls to this function will overwrite previous values.  Calling CriMvEasyPlayer::SetData() 
	 * will clear any filename set by this function.
	 * 
	 * \remarks
	 * This function can only be called when the handle status is MVEASY_STATUS_STOP or MVEASY_STATUS_PLAYEND,
	 * or from the file request callback.  See CriMvEasyPlayer::SetFileRequestCallback() for details.
	 *
	 * \remarks
	 * Calling this function does not open the file.  The file is opened by a call to CriMvEasyPlayer::Update().
	 * 
	 * \sa CriMvEasyPlayer::SetFileRequestCallback(), CriMvEasyPlayer::SetData(), CriMvEasyPlayer::SetFile()
	 */
	/*JP
	 * \ingroup     MODULE_CONTROL
	 *
	 * \brief       �Đ����������[�r�t�@�C�����܂ރp�b�N�t�@�C���̎w��
	 * \param		fname	�p�b�N�t�@�C���� (�p�X���܂�)
	 * \param		offset	�p�b�N�t�@�C�����̃��[�r�f�[�^�܂ł̃I�t�Z�b�g (�P��: �o�C�g) 
	 * \param		range	�p�b�N�t�@�C�����̃��[�r�f�[�^�̃T�C�Y (�P�ʁF�o�C�g)
	 * \param		err		�G���[���i�ȗ��j
	 *
	 * �Đ����郀�[�r���܂ރp�b�N�t�@�C�����w�肵�܂��B�����Ŏw�肵�� offset �ʒu���� range �T�C�Y���܂ł��p�b�N�t�@�C�����Ɋ܂܂�郀�[�r�f�[�^�݂Ȃ��܂��B
     * range�ɕ��l����͂���ƃp�b�N�t�@�C���̏I�[�܂ł����[�r�Ƃ��ēǂݍ��݂܂��B
	 *
	 * �p�b�N�t�@�C���̃t�@�C���p�X�̍ő咷�� CRIMV_MAX_FILE_NAME �o�C�g�ł��B
	 * EasyPlayer�͓����ł��̃t�@�C���p�X���R�s�[����̂ŁA�����œn����������͔j�����Ă����܂��܂���B
	 *
	 * �����t�@�C�����J��Ԃ��Đ�����ꍇ�́A���̊֐����ēx�Ăяo���K�v�͂���܂���B
	 *
	 * ���̊֐����Ăяo��������ɁA������x���̊֐����Ăяo���ƁA�O��̃t�@�C�����͐V�����t�@�C�����ɏ㏑������܂��B
	 * CriMvEasyPlayer::SetData()�֐����Ăяo�����ꍇ�́A���O�ɐݒ肵���t�@�C����񂪃N���A����܂��B
	 *
	 * \para ���l1�F
	 * ���̊֐��̓n���h����Ԃ� MVEASY_STATUS_STOP �������� MVEASY_STATUS_PLAYEND���̂݌Ăяo���\�ł��B
	 * �܂��̓t�@�C���v���R�[���o�b�N�֐����ł����̊֐����Ăяo�����Ƃ��ł��܂��B�ڍׂ� CriMvEasyPlayer::SetFileRequestCallback()�֐���
	 * �Q�Ƃ��Ă��������B
	 *
	 * \para ���l2�F
	 * ���̊֐��̓����ł̓t�@�C���̃I�[�v���v���͂��܂���B�t�@�C���̃I�[�v�������� CriMvEasyPlayer::Update() �֐��̒��ōs���܂��B
	 *
	 * \sa CriMvEasyPlayer::SetFileRequestCallback(), CriMvEasyPlayer::SetData(), CriMvEasyPlayer::SetFile()
	 */
	void SetFileRange(CriChar8 *fname, CriUint64 offset, CriSint64 range, CriError &err=CriMv::ErrorContainer);

	/* �O��̃��[�r�f�[�^��������x�o�^����i�t�@�C���v���R�[���o�b�N�֐��ł̂݌ĂԂ��Ɓj */
	void SetPreviousDataAgain(CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup     MODULE_CONTROL
	 *
	 * \brief       Reads movie header and analyzes movie information
	 * \param       err         Optional error code
	 *
	 * This function opens the movie file, reads the header, and starts to analyze the movie data.
	 * It does not start movie playback.
	 *
	 * When this function is called, the handle status changes to MVEASY_STATUS_DECHDR.  Once EasyPlayer
	 * has finished reading the header and analyzing the movie, the status changes to MVEASY_STATUS_WAIT_PREP.
	 * When the status is MVEASY_STATUS_WAIT_PREP, CriMvEasyPlayer::GetMovieInfo() can be successfully called.
	 * 
	 * To continue playback, call CriMvEasyPlayer::Prepare() or CriMvEasyPlayer::Start() when the handle status 
	 * is MVEASY_STATUS_WAIT_PREP.
	 *
	 * \remarks
	 * This function can only be called when the handle status is MVEASY_STATUS_STOP or MVEASY_STATUS_PLAYEND.
	 *
	 * \remarks
	 * You must either call CriMvEasyPlayer::SetFile() or CriMvEasyPlayer::SetData(), or provide a file request 
	 * callback with CriMvEasyPlayer::SetFileRequestCallback(), before calling this function.
	 *
	 * \remarks
	 * Once this function has been called, CriMvEasyPlayer::ExecuteDecode() and CriMvEasyPlayer::Update() must be 
	 * called periodically in order for this function to have any effect.  Otherwise, the handle status will 
	 * never change.
	 *
	 * \sa CriMvEasyPlayer::GetStatus(), CriMvEasyPlayer::Prepare(), CriMvEasyPlayer::Start(), 
	 *     CriMvEasyPlayer::SetFileRequestCallback(),
	 *     CriMvEasyPlayer::ExecuteDecode(), CriMvEasyPlayer::Update()	 
	 */
	/*JP
	 * \ingroup MODULE_CONTROL
	 * \brief		���[�r�w�b�_���
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * ���[�r�̍Đ��͊J�n�����A�w�b�_��͂̂ݍs���đҋ@���邽�߂̊֐��ł��B<br>
	 * ���̊֐����g�p���ăw�b�_��͂����O�ɍς܂��邱�Ƃɂ��A�Đ��J�n�O�Ƀ��[�r�̉𑜓x��I�[�f�B�I�̏���
	 * ���邱�Ƃ��ł��܂��B<br>
	 * �{�֐����Ăяo���ƁAEasyPlayer�̃n���h����Ԃ�MVEASY_STATUS_STOP �� MVEASY_STATUS_DECHDR �ƑJ�ڂ��Ă����A
	 * �w�b�_��͂����������MVEASY_STATUS_WAIT_PREP�ƂȂ�܂��B<br>
	 * ���[�r�����擾����ɂ́A�n���h����Ԃ�MVEASY_STATUS_WAIT_PREP�ɂȂ������Ƃ� CriMvEasyPlayer::GetMovieInfo()
	 * �����s���Ă��������B<br>
	 * 
	 * �n���h����Ԃ�MVEASY_STATUS_WAIT_PREP�̎��ɁA CriMvEasyPlayer::Prepare() �� CriMvEasyPlayer::Start() ��
	 * �ĂԂ��ƂōĐ������𑱂��邱�Ƃ��ł��܂��B<br>
	 * 
	 * �{�֐��� EasyPlayer�̃n���h����Ԃ�MVEASY_STATUS_STOP��MVEASY_STATUS_PLAYEND�̎��ɌĂяo���Ă��������B
	 * 
	 * �{�֐����Ăяo���O�� CriMvEasyPlayer::SetFile() �� CriMvEasyPlayer::SetData() �Ń��[�r�f�[�^���w�肵�Ă��������B<br>
	 * �������A�t�@�C���v���R�[���o�b�N�֐���o�^���Ă���ꍇ�͎��O�̃��[�r�f�[�^�ݒ�͏ȗ����邱�Ƃ����܂��B
	 * 
	 * \sa CriMvEasyPlayer::GetStatus(), CriMvEasyPlayer::Prepare(), CriMvEasyPlayer::Start(), CriMvEasyPlayer::SetFileRequestCallback()
	 */
	void DecodeHeader(CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup     MODULE_CONTROL
	 *
	 * \brief       Reads movie header, analyzes movie information, and buffers data.
	 * \param       err         Optional error code
	 *
	 * This function opens the movie file and gets it ready for immediate playback, by reading and analyzing
	 * the header and decoding and buffering video data.  It allows a movie to start playing immediately when
	 * CriMvEasyPlayer::Start() is called, without an initial delay.
	 *
	 * When this function is called, the handle status changes to MVEASY_STATUS_DECHDR.  Once EasyPlayer
	 * has finished reading and analyzing the movie data, the status changes to MVEASY_STATUS_PREP and EasyPlayer
	 * begins decoding video data.  When the initial video data has been decoded and buffered, the status changes to
	 * MVEASY_STATUS_READY.
	 *
	 * The amount of data that is buffered, in seconds, is based on the value set by CriMvEasyPlayer::SetBufferingTime().
	 * By default, this is 1 second.
	 *
	 * When the status is MVEASY_STATUS_READY, movie playback will start imediately when CriMvEasyPlayer::Start()
	 * is called.
	 *
	 * \remarks
	 * Unless you  CriMvEasyPlayer::DecodeHeader() first, this function can only be called when the handle status
	 * is MVEASY_STATUS_STOP or MVEASY_STATUS_PLAYEND.
	 *
	 * \remarks
	 * You must either call CriMvEasyPlayer::SetFile() or CriMvEasyPlayer::SetData(), or provide a file request 
	 * callback with CriMvEasyPlayer::SetFileRequestCallback(), before calling this function.
	 *
	 * \remarks
	 * Once this function has been called, CriMvEasyPlayer::ExecuteDecode() and CriMvEasyPlayer::Update() must be 
	 * called periodically in order for this function to have any effect.  Otherwise, the handle status will 
	 * never change.
	 * 
	 * \sa CriMvEasyPlayer::GetStatus(), CriMvEasyPlayer::DecodeHeader(), CriMvEasyPlayer::Start(),
	 *     CriMvEasyPlayer::SetBufferingTime(), CriMvInputBufferInfo
	 */
	/*JP
	 * \ingroup MODULE_CONTROL
	 * \brief		�Đ������i�w�b�_��͂ƃo�b�t�@�����O�j
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * ���[�r�̍Đ��͊J�n�����A�w�b�_��͂ƍĐ������݂̂��s���đҋ@���邽�߂̊֐��ł��B<br>
	 * ���̊֐����g�p���čĐ����������O�ɍς܂��邱�Ƃɂ��A���[�r�Đ��J�n�̃^�C�~���O���ׂ������䂷�邱�Ƃ��ł��܂��B<br>
	 * �i�Đ����������ōĐ��J�n�֐����Ăяo�����ꍇ�́A���ۂɍĐ����n�܂�܂łɃ^�C�����O���������܂��B�j<br>
	 * �{�֐����Ăяo���ƁAEasyPlayer�̃n���h����Ԃ�MVEASY_STATUS_STOP �� MVEASY_STATUS_DECHDR �� MVEASY_STATUS_PREP �ƑJ�ڂ��Ă����A
	 * �Đ����������������MVEASY_STATUS_READY�ƂȂ�܂��B
	 * 
	 * �n���h����Ԃ�MVEASY_STATUS_READY�̎��ɁA CriMvEasyPlayer::Start() ���ĂԂ��ƂōĐ����J�n���邱�Ƃ��ł��܂��B
	 * 
	 * CriMvEasyPlayer::DecodeHeader() �̌Ăяo�������ł��̊֐����Ăяo���ꍇ�́ACriMvEasyPlayer�̃n���h����Ԃ�
	 * MVEASY_STATUS_STOP��MVEASY_STATUS_PLAYEND �łȂ���΂����܂���B
	 * 
	 * �Đ��J�n�O�ɂ� CriMvEasyPlayer::SetFile() �� CriMvEasyPlayer::SetData() �Ń��[�r�f�[�^���w�肵�Ă��������B<br>
	 * �������A�t�@�C���v���R�[���o�b�N�֐���o�^���Ă���ꍇ�͎��O�̃��[�r�f�[�^�ݒ�͏ȗ����邱�Ƃ����܂��B
	 * 
	 * \sa CriMvEasyPlayer::GetStatus(), CriMvEasyPlayer::DecodeHeader(), CriMvEasyPlayer::Start(),
	 *     CriMvEasyPlayer::SetBufferingTime(), CriMvInputBufferInfo
	 */
	void Prepare(CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup     MODULE_CONTROL
	 *
	 * \brief       Starts movie playback
	 * \param       err         Optional error code
	 *
	 * Opens the movie and starts playback.  If CriMvEasyPlayer::Prepare() was not called, there will be a
	 * delay while the library reads the movie header and buffers the initial data.  If CriMvEasyPlayer::Prepare()
	 * was called and the handle status is MVEASY_STATUS_READY, playback will start as soon as this function is called.
	 *
	 * \remarks
	 * If you call this function without calling CriMvEasyPlayer::DecodeHeader() or CriMvEasyPlayer::Prepare() 
	 * first, this function can only be called when the handle status is MVEASY_STATUS_STOP, MVEASY_STATUS_PLAYEND,
	 * or MVEASY_STATUS_READY.
	 *
	 * \remarks
	 * You must either call CriMvEasyPlayer::SetFile() or CriMvEasyPlayer::SetData(), or provide a file request 
	 * callback with CriMvEasyPlayer::SetFileRequestCallback(), before calling this function.
	 *
	 * \remarks
	 * Once this function has been called, CriMvEasyPlayer::ExecuteDecode() and CriMvEasyPlayer::Update() must be 
	 * called periodically in order for this function to have any effect.  Otherwise, the movie will not play and
	 * the handle status will never change.
	 * 
	 * \sa CriMvEasyPlayer::GetStatus(), CriMvEasyPlayer::DecodeHeader(), CriMvEasyPlayer::Prepare()
	 */
	/*JP
	 * \ingroup MODULE_CONTROL
	 * \brief		�Đ��J�n
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * ���[�r�̍Đ����J�n���܂��B<br>
	 * CriMvEasyPlayer::Prepare()���Ă΂��ɁA�{�֐����Ăяo�����ꍇ�́A���[�r�̉�͂ƍĐ��̏������s�����߁A
	 * ���ۂɃ��[�r�̍Đ����n�܂�܂łɃ^�C�����O���������܂��B<br>
	 * CriMvEasyPlayer::Prepare()���ɌĂяo���āA�n���h����Ԃ�MVEASY_STATUS_READY�ɂȂ��Ă���΁A
	 * ���̊֐����Ăяo���Ă����ɍĐ����n�܂�܂��B
	 * 
	 * CriMvEasyPlayer::DecodeHeader() �܂��� CriMvEasyPlayer::Prepare() �̌Ăяo�������ł��̊֐����Ăяo���ꍇ�́A
	 * CriMvEasyPlayer�̃n���h����Ԃ� MVEASY_STATUS_STOP��MVEASY_STATUS_PLAYEND �łȂ���΂����܂���B
	 * 
	 * �Đ��J�n�O�ɂ� CriMvEasyPlayer::SetFile() �� CriMvEasyPlayer::SetData() �Ń��[�r�f�[�^���w�肵�Ă��������B<br>
	 * �������A�t�@�C���v���R�[���o�b�N�֐���o�^���Ă���ꍇ�͎��O�̃��[�r�f�[�^�ݒ�͏ȗ����邱�Ƃ����܂��B
	 * 
	 * \sa CriMvEasyPlayer::GetStatus(), CriMvEasyPlayer::DecodeHeader(), CriMvEasyPlayer::Prepare()
	 */
	void Start(CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup     MODULE_CONTROL
	 *
	 * \brief       Stops movie playback; resets a handle that is in an ERROR state,
	 * \param       err         Optional error code
	 *
	 * Tells the movie to stop playing and returns immediately.  Note that this does not actually stop playback;
	 * an application must continue to call CriMvEasyPlayer::Update() and CriMvEasyPlayer::ExecuteDecode() until
	 * the status changes to MVEASY_STATUS_STOP.
	 *
	 * Once the handle is in the MVEASY_STATUS_STOP state, it can be reused for a different movie.
	 *
	 * If the handle state is MVEASY_STATUS_ERROR, an application must call this function and wait for the status
	 * to change to MVEASY_STATUS_STOP before destroying or reusing the player handle.
	 *
	 * In principle, this function does not reset any EasyPlayer handle parameters that were explicitly set via
	 * any of the settings APIs, so an application can replay the same movie by simply calling CriMvEasyPlayer::Start()
	 * once the state has changed to MVEASY_STATUS_STOP.  However, calling this function will reset the following
	 * parameters:
	 *
	 *  - The pause state (see CriMvEasyPlayer::Pause()) will be reset to OFF (not paused).
	 *  - If a file request callback has been set (via CriMvEasyPlayer::SetFileRequestCallback()), the movie
	 *    data information will be reset.
	 *
	 * See the description of CriMvEasyPlayer::ResetAllParameters() for a comparison of the parameters that are 
	 * affected by that function and by this function.
	 *
	 * \remarks
	 * This function may call CriMvSoundInterface::Stop() and CriMvFileReaderInterface::Close().  For an EasyPlayer 
	 * handle to change to MVEASY_STATUS_STOP, the sound and file interfaces need to transition to their STOP states.
	 * In the case of CriMvSoundInterface, this means that CriMvSoundInterface::GetStatus() will return 
	 * MVEASY_SOUND_STATUS_STOP.  For CriMvFileReaderInterface, this means that CriMvFileReaderInterface::GetCloseStatus()
	 * will return ASYNC_STATUS_COMPLETE.
	 *
	 * \sa CriMvEasyPlayer::Status, CriMvEasyPlayer::GetStatus(), CriMvEasyPlayer::ResetAllParameters()
	 */
	/*JP
	 * \ingroup MODULE_CONTROL
	 * \brief		�Đ���~�^�G���[��Ԃ���̕��A
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * ���[�r�Đ���~�̗v�����o���܂��B�{�֐��͑������A�֐��ł��B�{�֐����őS�Ă̒�~���������s�����킯�ł͂���܂���B<br>
	 * �{�֐��ďo����A�Đ���Ԃ� MVEASY_STATUS_STOP �Ȃ�܂ł͒ʏ�̃��C�����[�v�����𓮂����Ă��������B<br>
	 * ��̓I�ɂ� CriMvEasyPlayer::Update(), CriMvEasyPlayer::ExecuteDecode() ���ʏ�ʂ�Ăяo�����K�v������܂��B
	 * 
	 * �Đ���Ԃ� MVEASY_STATUS_ERROR �ɂȂ����ꍇ�́A�{�֐����Ăяo���� MVEASY_STATUS_STOP ��҂��Ă��������B<BR>
	 * 
	 * for���[�v�Ȃǂɂ�郍�[�J�����[�v�ŏ�ԕύX�҂������Ă� MVEASY_STATUS_STOP �ɂ͂Ȃ�܂���B<br>
	 * 
	 * �{�֐����Ăяo���Ă��A�A�v���P�[�V�������Đ��n���h���ɐݒ肵���e��p�����[�^�͌����Ƃ��ă��Z�b�g����܂���B<BR>
	 * MVEASY_STATUS_STOP ��ԂɂȂ������ƁA������x�Đ����J�n����ƑO��Ɠ����p�����[�^�ōĐ����s�����Ƃ��ł��܂��B<BR>
	 * ��O�I�ɖ{�֐��Ń��Z�b�g�����p�����[�^�͈ȉ��̂��̂�����܂��B
	 * - CriMvEasyPlayer::Pause() �ɂ��|�[�Y��Ԃ́AOFF�Ƀ��Z�b�g����܂��B
	 * - �t�@�C���v���R�[���o�b�N�֐��̓o�^������ꍇ�A���[�r�t�@�C�����i�܂��̓������j�̏��̓��Z�b�g����܂��B
	 * 
	 * ���Z�b�g�����p�����[�^�ꗗ�� CriMvEasyPlayer::ResetAllParameters() �̐������Q�Ƃ��Ă��������B
	 * 
	 * �{�֐��͕K�v�ɉ����� CriMvSoundInterface::Stop() ����� CriMvFileReaderInterface::Close() ���Ăяo���܂��B<br>
	 * EasyPlayer �n���h���� MVEASY_STATUS_STOP ��ԂɂȂ邽�߂ɂ́A�e�C���^�t�F�[�X����~��ԂɂȂ�Ȃ���΂����܂���B<br>
	 * �T�E���h�C���^�t�F�[�X�̏ꍇ�A CriMvSoundInterface::GetStatus()�� MVEASY_SOUND_STATUS_STOP ��Ԃ����ƁB<br>
	 * �t�@�C���ǂݍ��݃C���^�t�F�[�X�̏ꍇ�A CriMvFileReaderInterface::GetCloseStatus() ���AASYNC_STATUS_COMPLETE 
	 * ��Ԃ��Ȃ���΂����܂���B
	 * 
	 * \sa CriMvEasyPlayer::Status, CriMvEasyPlayer::GetStatus(), CriMvEasyPlayer::ResetAllParameters()
	 */
	void Stop(CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup     MODULE_CONTROL
	 *
	 * \brief       Pauses or resumes movie playback
	 * \param       sw          Pause state.  ON (1) will pause playback, OFF (0) will resume it.
	 * \param       err         Optional error code
	 *
	 * Pauses or resumes movie playback, depending on the argument.  If \a sw is ON (1), playback will be paused.
	 * If \a sw is OFF (0), playback will be resumed.
	 *
	 * \remarks
	 * This function will call CriMvSoundInterface::Pause() and CriMvSystemTimerInterface::Pause() with the
	 * provided argument.
	 *
	 * \remarks
	 * Calling  CriMvEasyPlayer::Stop() or CriMvEasyPlayer::ResetAllParameters() will set the pause state to OFF.
	 *
	 * \sa CriMvSoundInterface::Pause(), CriMvSystemTimerInterface::Pause() 
	 */
	/*JP
	 * \ingroup MODULE_CONTROL
	 * \brief		�Đ��̈ꎞ��~�܂��͍ĊJ
	 *  \param		sw		�|�[�Y�X�C�b�`�B�|�[�YON�̏ꍇ��1�A�|�[�YOFF(���W���[��)�̏ꍇ��0���w�肵�܂��B
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * �{�֐��̓���͈����Ɉˑ����܂��B<br>
	 * ���� sw ��ON(1)�Ȃ�A�ꎞ��~�B���� sw ��OFF(0)�Ȃ�Đ��ĊJ�ł��B
	 * 
	 * CriMvEasyPlayer::Stop() �܂��� CriMvEasyPlayer::ResetAllParameters ���Ăяo���ƃ|�[�Y��Ԃ�OFF�Ƀ��Z�b�g����܂��B
	 * 
	 * ���̊֐��� CriMvSoundInterface::Pause() �� CriMvSystemTimerInterface::Pause() �𓯂������ŌĂяo���܂��B
	 * 
	 * \sa CriMvSoundInterface::Pause(), CriMvSystemTimerInterface::Pause() 
	 */
	void Pause(CriBool sw, CriError &err=CriMv::ErrorContainer);

	CriBool IsPaused(CriError &err=CriMv::ErrorContainer);

	/* �I�v�V�����ݒ�^�擾 */
	/*EN
	 * \ingroup MODULE_OPTION
	 * 
	 * \brief       Sets the type of timer used for video synchronization
	 * \param       type        Type of timer to use
	 * \param       err         Optional error code
	 *
	 * In order to display video frames at the proper rate, CRI Movie uses a timer to determine when the
	 * next frame should be shown.  For a movie with an audio track, you would typically use the
	 * MVEASY_TIMER_AUDIO timer type.  For a movie with no audio, the MVEASY_TIMER_SYSTEM type timer
	 * should be used.
	 *
	 * The default is the timer type that was passed to CriMvEasyPlayer::Create().  If this value
	 * was NULL, a system timer (MVEASY_TIMER_SYSTEM) will be used.
	 *
	 * \remarks
	 * If an audio timer (MVEASY_TIMER_AUDIO) is wanted, it must be created and passed to
	 * CriMvEasyPlayer::Create().
	 *
	 * \remarks
	 * If the movie does not have an audio track, CRI Movie will use a MVEASY_TIMER_SYSTEM regardless of
	 * the value set by this function.
	 *
	 * \sa CriMvEasyPlayer::GetMasterTimer(), CriMvEasyPlayer::Create(), TimerType
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�}�X�^�^�C�}��ʂ̎w��
	 *  \param		type	�}�X�^�^�C�}���
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * �r�f�I�t���[���̎����Ǘ��Ɏg�p����^�C�}��ʂ��w�肵�܂��B<br>
	 * �f�t�H���g�̓n���h���쐬���Ɏw�肷��V�X�e���^�C�}�ł��B<br>
	 * �r�f�I�t���[���̕\���^�C�~���O���I�[�f�B�I�̎����Ɠ������������Ƃ��̓I�[�f�B�I�^�C�}���w�肵�Ă��������B<br>
	 * �I�[�f�B�I�^�C�}���w�肵���ꍇ�ł��A�Đ����郀�[�r�ɃI�[�f�B�I���܂܂�Ă��Ȃ��ꍇ�̓V�X�e���^�C�}�����ƂȂ�܂��B
	 * 
	 * \sa CriMvEasyPlayer::GetMasterTimer(), CriMvEasyPlayer::Create()
	 */
	void SetMasterTimer(TimerType type, CriError &err=CriMv::ErrorContainer);		// default is SYSTEM

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Returns the type of timer currently being used by the EasyPlayer handle
	 * \param       err         Optional error code
	 * \return      The current timer type
	 *
	 * Returns the current type of timer used to synchronize video frames.  This value can be changed
	 * by calling CriMvEasyPlayer::SetMasterTimer().  Otherwise, the timer type is set when the handle 
	 * is created.
	 *
	 * \sa CriMvEasyPlayer::SetMasterTimer(), CriMvEasyPlayer::Create(), TimerType
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�}�X�^�^�C�}��ʂ̎擾
	 *  \param		err		�G���[���i�ȗ��j
	 *  \return 	���ݐݒ肳��Ă���}�X�^�^�C�}���
	 * 
	 * ���ݐݒ肳��Ă���}�X�^�^�C�}��ʂ��擾���܂��B
	 * 
	 * \sa CriMvEasyPlayer::SetMasterTimer()
	 */
	TimerType GetMasterTimer(CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Sets the number of internal video buffers
	 * \param       npools      The number of buffers to use; must be greater than 0
	 * \param       err         Optional error code
	 *
	 * CRI Movie uses internal memory, or frame pools, to buffer decoded frames before display.
	 * More frame pools can help smooth out playback under high CPU loads.
	 *
	 * \remarks
	 * By default, the number of pools is 1.  To change the value, this function must be called
	 * before starting playback (with either CriMvEasyPlayer::Prepare() or CriMvEasyPlayer::Start()).
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�����r�f�I�o�b�t�@�i�t���[���v�[���j���̎w��
	 *  \param		npools		�����r�f�I�o�b�t�@���i�Œ�ł��P�j
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * EasyPlayer�n���h�������̃r�f�I�o�b�t�@�����w�肵�܂��B<br>
	 * ���̓����r�f�I�o�b�t�@�̓f�R�[�h���ʂ�~���Ă������߂̂��̂ŁA�t���[���v�[���ƌĂт܂��B<br>
	 * �t���[���v�[���������قǐ�s���ăr�f�I�f�R�[�h��i�߂邱�Ƃ��ł��邽�߁A�f�R�[�h��
	 * ���וϓ����傫��������A�f�R�[�h�Ɏg�p�ł���CPU���Ԃ̕ϓ����傫���ꍇ�ɂ��X���[�Y�ȍĐ���
	 * �s���₷���Ȃ�܂��B<br>
	 * �f�t�H���g�̃t���[���v�[�����͂P�ł��B<br>
	 * �t���[���v�[������ύX�������ꍇ�́A�Đ��J�n�O( CriMvEasyPlayer::Prepare()�܂��� CriMvEasyPlayer::Start())��
	 * �{�֐������s���Ă��������B
	 */
	void SetNumberOfFramePools(CriUint32 npools, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Sets the amount of movie data that will be buffered, in seconds
	 * \param       sec         Buffering time, in seconds
	 * \param       err         Optional error code
	 *
	 * CRI Movie buffers enough raw data from disk to allow for smooth playback and to reduce disk reads.
	 * The buffer size is based on the bitrate of the movie, and other movie parameters.
	 *
	 * By default, this buffer will be large enough to hold 1 second worth of playback.
	 *
	 * To determine the current buffering time, look at the \a buffering_time field of the
	 * CriMvStreamingParameters structure, which is retrieved by calling CriMvEasyPlayer::GetMovieInfo().
	 *
	 * \remarks
	 * If this function is called, it must be called before starting playback (with either 
	 * CriMvEasyPlayer::Prepare() or CriMvEasyPlayer::Start()).
	 *
	 * \remarks
	 * Passing \a 0.0 as the value of \a sec will reset the buffering time to the default value.
	 *
	 * \remarks
	 * If an application calls CriMvEasyPlayer::SetStreamingParameters() for a handle, this function
	 * can not be used with that handle.
	 *
	 * \remarks
	 * The value set by this function, along with the value set by CriMvEasyPlayer::SetReloadThresholdTime(),
	 * determine how often data is read from disk.  See the description of 
	 * CriMvEasyPlayer::SetReloadThresholdTime() for details.
	 *
	 * \sa CriMvEasyPlayer::SetReloadThresholdTime(), CriMvEasyPlayer::GetInputBufferInfo(),
	 *     CriMvStreamingParameters
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		���̓f�[�^�̃o�b�t�@�����O���Ԃ̎w��
	 *  \param		sec		�o�b�t�@�����O���ԁB�P�ʂ͕b�B
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * �X�g���[�~���O�Đ��Ńo�b�t�@�����O������̓f�[�^�̗ʂ�b�P�ʂ̎��ԂŎw�肵�܂��B<br>
	 * EasyPlayer�́A�o�b�t�@�����O���Ԃƃ��[�r�̃r�b�g���[�g������ǂݍ��݃o�b�t�@�̃T�C�Y�����肵�܂��B
	 * 
	 * �f�t�H���g�̃o�b�t�@�����O���Ԃ́A�Đ��J�n���_�ŃA�v���P�[�V�������쐬�ς݂�EasyPlayer�n���h����
	 * �Ɉˑ����Č��܂�܂��BEasyPlayer�n���h���P�ɂ��P�b�̃o�b�t�@�����O���Ԃ��m�ۂ��܂��B�������A�v��
	 * �P�[�V�������R��EasyPlayer�n���h�����쐬���Ă����ꍇ�A�o�b�t�@�����O���Ԃ͂R�b�ƂȂ�܂��B
	 * 
	 * EasyPlayer�n���h�������b���̃o�b�t�@�����O���ԂɂȂ��Ă��邩�� CriMvEasyPlayer::GetMovieInfo 
	 * �֐��Ŏ擾���� CriMvStreamingParameters �\���̂̕ϐ� buffering_time �Ŋm�F�ł��܂��B
	 * 
	 * �{�֐��̌Ăяo���́A CriMvEasyPlayer::Prepare �֐��܂��� CriMvEasyPlayer::Start �֐��̑O�܂łɎ��s���Ă��������B
	 * 
	 * �o�b�t�@�����O���Ԃ� 0.0f ���w�肵���ꍇ�A�o�b�t�@�����O���Ԃ̓��C�u�����̃f�t�H���g�l�ƂȂ�܂��B<br>
	 * �܂��A�A�v���P�[�V������ CriMvEasyPlayer::SetStreamingParameters �֐����Ăяo�����ꍇ�͖{�֐���
	 * �ݒ肵���l�����A CriMvEasyPlayer::SetStreamingParameters �֐��̎w�肪�D�悳��܂��B
	 * 
	 * \sa CriMvEasyPlayer::SetReloadThresholdTime(), CriMvEasyPlayer::GetInputBufferInfo()
	 */
	void SetBufferingTime(CriFloat32 sec, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Determines how often the movie data buffer is refilled from disk
	 * \param       sec         Number of seconds of playback time to buffer
	 * \param       err         Optional error code
	 *
	 * EasyPlayer buffers raw data from disk to allow for smooth playback.  How often it refills 
	 * its buffers is determined by this function.  When the amount of data remaining, in seconds,
	 * falls below this value, EasyPlayer will call into the FileReader module for this handle in 
	 * order to read more data.
	 *
	 * SetBufferingTime() sets a "low water mark" for the data buffer.  For instance, if an application
	 * sets the buffer size to 4 seconds (with CriMvEasyPlayer::SetBufferingTime()), and sets the reload
	 * threshold to 1 second, then CRI Movie will initially fill the buffer with 4 seconds worth of 
	 * data.  After 3 seconds worth of data have been decoded and consumed, there will be less than 
	 * \a reload \a threshold seconds of data remaining, and CRI Movie will refill the buffer.
	 * 
	 * \remarks
	 * The default value for \a sec is 0.8s.
	 *
	 * \remarks
	 * If this function is called, it must be called before starting playback (with either 
	 * CriMvEasyPlayer::Prepare() or CriMvEasyPlayer::Start()).
	 *
	 * \sa CriMvEasyPlayer::SetBufferingTime(), CriMvEasyPlayer::GetInputBufferInfo(), CriMvFileReaderInterface
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�ēǂݍ���臒l�̎��Ԏw��
	 *  \param		sec		���Ԏw��ɂ��ēǂݍ���臒l�B�P�ʂ͕b�B
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * EasyPlayer�n���h���́A���̓o�b�t�@���̃f�[�^���ēǂݍ���臒l�ȉ��ɂȂ������Ɏ��̃f�[�^�ǂݍ��݂����s���܂��B
	 * �ēǂݍ���臒l�͖{�֐��ɂ��w�莞�Ԃƃ��[�r�f�[�^�̃r�b�g���[�g�ɂ���Ď����I�Ɍv�Z����܂��B
	 * �ēǂݍ���臒l�͎���[�b]�Ŏw�肵�܂��B�f�t�H���g�l��0.8�b�ł��B
	 * 
	 * ���[�r�Đ����Ƀf�[�^�𗠓ǂ݂���ꍇ�ȂǂɃV�[�N�񐔂����炷���߂�臒l�ݒ�𗘗p���邱�Ƃ��ł��܂��B
	 * �Ⴆ�΁A�o�b�t�@�����O���Ԃ�2�b�A�ēǂݍ���臒l��1�b�ɐݒ肷��ƁA���[�r�f�[�^�̓ǂݍ��݂͖�1�b��1��̎��s�ɂȂ�܂��B
	 * �������邱�ƂŁA��1�b�̊Ԃ̓f�[�^�̓ǂݍ��݂�A���I�ɍs�����Ƃ��ł��܂��B
	 * 
	 * ���[�r���Đ����Ȃ��烆�[�U�f�[�^�̓ǂݍ��݂��s���ꍇ�A���[�U�f�[�^�̓ǂݍ��݂͖{�֐��Ŏw�肵�����Ԉȓ��ɓǂݍ���
	 * �������I���悤�ɂ��Ă��������B�T�C�Y�̑傫�ȃf�[�^�͕����ɕ������ēǂݍ��ނȂǂ̑Ώ����K�v�ɂȂ�܂��B
	 * �{�֐��Ŏw�肵�����Ԉȓ��Ƀ��[�U�f�[�^�̓ǂݍ��݂��I���Ȃ������ꍇ�A���[�r�f�[�^���͊����ă��[�r�Đ����؂�܂��B
	 * 
	 * �{�֐��̌Ăяo���́A CriMvEasyPlayer::Prepare �֐��܂��� CriMvEasyPlayer::Start �֐��̑O�܂łɎ��s���Ă��������B
	 * 
	 * ���[�r�Đ����̓��̓o�b�t�@�̃f�[�^�ʂ�ēǂݍ���臒l�̃T�C�Y�́ACriMvEasyPlayer::GetInputBufferInfo �Ŏ擾�\�ł��B
	 * 
	 * \sa CriMvEasyPlayer::SetBufferingTime(), CriMvEasyPlayer::GetInputBufferInfo()
	 */
	void SetReloadThresholdTime(CriFloat32 sec, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Retrieves input data buffering settings
	 * \param       ibuf_info       Input buffer information structure
	 * \param       err             Optional error code
	 *
	 * Populates the passed CriMvInputBufferInfo structure with the values of the settings
	 * for the raw input buffer size, the reload interval, and the amount of data currently buffered.
	 *
	 * \remarks
	 * This function can be called once the handle status has transitioned to MVEASY_STATUS_WAIT_PREP.
	 *
	 * \sa CriMvInputBufferInfo, CriMvEasyPlayer::SetBufferingTime(), CriMvEasyPlayer::SetReloadThresholdTime()
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		���̓o�b�t�@���̎擾
	 *  \param		ibuf_info	���̓o�b�t�@���
	 *  \param		err			�G���[���i�ȗ��j
	 * 
	 * ���̓o�b�t�@��� CriMvInputBufferInfo ���擾���܂��B<br>
	 * ���̓o�b�t�@����EasyPlayer�n���h���̏�Ԃ� MVEASY_STATUS_WAIT_PREP �ȍ~�ɂȂ������Ǝ擾�ł��܂��B<br>
	 * 
	 * \sa CriMvInputBufferInfo, CriMvEasyPlayer::SetBufferingTime(), CriMvEasyPlayer::SetReloadThresholdTime()
	 */
	void GetInputBufferInfo(CriMvInputBufferInfo &ibuf_info, CriError &err=CriMv::ErrorContainer);

	/*EN
	 *
	 * \brief       Sets the maximum bitrate EasyPlayer will assume for movie data
	 * \param       max_bitrate     Maximum bitrate, in bits/second
	 * \param       err             Optional error code
	 *
	 * EasyPlayer determines the size of its input data buffer by the movie's bitrate, the buffering time,
	 * and other movie parameters.  In normal usage, an application should not need to call this function.
	 * However, it can be useful when doing concatenated playback of several movies sequentially.
	 *
	 * If the bitrate of the first movie is smaller or larger than the next movie, EasyPlayer can choose
	 * a buffer size that will be appropriate for the first movie, but either too small for the next 
	 * (causing excessive disk reads or playback stuttering) or too large (using more memory than necessary).
	 * 
	 * Passing a value of 0 for \a max_bitrate will cause the handle to revert to its default behavior
	 * for determining maximum bitrate.
	 *
	 * \remarks
	 * The value set by this function will not be reflected in the \a max_bitrate field of the 
	 * CriMvStreamingParameters structure, which will contain the actual value as stored in the movie's
	 * header.
	 *
	 * \remarks
	 * If this function is called, it must be called before starting playback (with either 
	 * CriMvEasyPlayer::Prepare() or CriMvEasyPlayer::Start()).
	 *
	 * \remarks
	 * If an application calls CriMvEasyPlayer::SetStreamingParameters() for a handle, this function
	 * can not be used with that handle.
	 *
	 * \remarks
	 * For details about concatenated playback, see the description of CriMvEasyPlayer::SetFileRequestCallback().
	 *
	 * \sa CriMvEasyPlayer::SetBufferingTime(), CriMvEasyPlayer::SetReloadThresholdTime(), 
	 *     CriMvEasyPlayer::SetFileRequestCallback()
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�ő�r�b�g���[�g�̎w��
	 *  \param		max_bitrate		�ő�r�b�g���[�g(bit per second)
	 *  \param		err				�G���[���(�ȗ���)
	 * 
	 * ���[�r�f�[�^�̍ő�r�b�g���[�g���w�肵�܂��B�ő�r�b�g���[�g�̓X�g���[���Đ��p�Ɋm�ۂ���o�b�t�@�T�C�Y�ɉe�����܂��B<br>
	 * 
	 * �P���Đ����͖{�֐����Ăяo���K�v�͂���܂���BEasyPlayer�n���h���������I�ɍő�r�b�g���[�g���擾���ĕK�v�Ȃ�����
	 * �ǂݍ��݃o�b�t�@���m�ۂ��܂��B<br>
	 * 
	 * �A���Đ����ɁA�擪�̃��[�r�t�@�C���̃r�b�g���[�g���㑱�̃��[�r�t�@�C���Ɣ�ׂċɒ[�ɏ������ꍇ�ɂ́A�{�֐����g�p����
	 * �����I�ɍő�r�b�g���[�g��傫���w�肵�Ă��������B<br>
	 * 
	 * �{�֐��Őݒ肵���ő�r�b�g���[�g�́ACriMvEasyPlayer::GetMovieInfo �֐��Ŏ擾���郀�[�r���ɂ͔��f����܂���B
	 * CriMvEasyPlayer::GetMovieInfo �֐��Ŏ擾�ł���̂̓��[�r�f�[�^�̖{���̏��ł��B<br>
	 * 
	 * �{�֐��̌Ăяo���́A CriMvEasyPlayer::Prepare �֐��܂��� CriMvEasyPlayer::Start �֐��̑O�܂łɎ��s���Ă��������B<br>
	 * 
	 * �ő�r�b�g���[�g�� 0���w�肵���ꍇ�A�ő�r�b�g���[�g�̓��[�r�f�[�^�̎��l�ƂȂ�܂��B<br>
	 * �܂��A�A�v���P�[�V������ CriMvEasyPlayer::SetStreamingParameters �֐����Ăяo�����ꍇ�͖{�֐���
	 * �ݒ肵���l�����A CriMvEasyPlayer::SetStreamingParameters �֐��̎w�肪�D�悳��܂��B
	 *
	 * \sa CriMvEasyPlayer::SetBufferingTime(), CriMvEasyPlayer::SetReloadThresholdTime(), 
	 *     CriMvEasyPlayer::SetFileRequestCallback()	
	*/
	void SetMaxBitrate(CriUint32 max_bitrate, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Sets the audio playback track
	 * \param       track       Audio track number
	 * \param       err         Optional error code
	 *
	 * If a movie has multiple audio tracks (for instance, English and Spanish versions), this function
	 * will determine which track plays.  By default, the first audio track is used.
	 *
	 * To determine the number of audio tracks in the movie, call CriMvEasyPlayer::GetMovieInfo() and 
	 * look at the \a num_audio field of the CriMvStreamingParameters structure.
	 *
	 * If the movie does not have any audio, this function has no effect.
	 *
	 * \remarks
	 * To use the default setting, set \a track to CRIMV_AUDIO_TRACK_AUTO.
	 *
	 * \remarks
	 * To turn off audio altogether, set \a track to CRIMV_AUDIO_TRACK_OFF.
	 * 
	 * \sa CriMvEasyPlayer::GetMovieInfo()
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�Đ�����I�[�f�B�I�g���b�N�̎w��
	 *  \param		track	�Đ�����I�[�f�B�I�g���b�N
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * ���[�r�������̃I�[�f�B�I�g���b�N�������Ă���ꍇ�ɁA�Đ�����I�[�f�B�I���w�肵�܂��B<br>
	 * �Đ��J�n�O( CriMvEasyPlayer::Prepare()�܂��� CriMvEasyPlayer::Start())�ɖ{�֐������s���Ă��������B
	 * 
	 * �{�֐������s���Ȃ������ꍇ�́A�����Ƃ��Ⴂ�ԍ��̃I�[�f�B�I�g���b�N���Đ����܂��B<br>
	 * CriMvEasyPlayer::DecodeHeader()�� CriMvEasyPlayer::GetMovieInfo()���g�����ƂŁA�ǂ̃`���l����
	 * �ǂ�ȃI�[�f�B�I�������Ă��邩���Đ��J�n�O�ɒm�邱�Ƃ��ł��܂��B
	 * 
	 * �f�[�^�����݂��Ȃ��g���b�N�ԍ����w�肵���ꍇ�́A�I�[�f�B�I�͍Đ�����܂���B
	 * 
	 * �g���b�N�ԍ��Ƃ���CRIMV_AUDIO_TRACK_OFF���w�肷��ƁA�Ⴆ���[�r�ɃI�[�f�B�I���܂܂�Ă�����
	 * ���Ă��I�[�f�B�I�͍Đ����܂���B
	 * 
	 * �܂��A�f�t�H���g�ݒ�i�����Ƃ��Ⴂ�`���l���̃I�[�f�B�I���Đ�����j�ɂ������ꍇ�́A
	 * �`���l���Ƃ���CRIMV_AUDIO_TRACK_AUTO���w�肵�Ă��������B
	 * 
	 * \sa CriMvEasyPlayer::GetMovieInfo(), CriMvEasyPlayer::DecodeHeader()
	 */
	void SetAudioTrack(CriSint32 track, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Turns movie looping on or off
	 * \param       sw          Looping value
	 * \param       err         Optional error code
	 *
	 * If \a sw is 1 (ON), the movie will loop continuously.
	 * If \a sw is 0 (OFF), the movie will play normally.
	 *
	 * In normal usage, a movie will play once, with the handle status changing to MVEASY_STATUS_PLAYEND
	 * when it completes.  When looping is ON, this behavior changes.  When the movie reaches the
	 * end, it will immediately start playing again from the beginning, and the status will continue to
	 * toggle between MVEASY_STATUS_PLAYING and MVEASY_STATUS_PREP.
	 *
	 * If looping is ON, and the playback is from a file, EasyPlayer will call CriMvFileReaderInterface::Seek()
	 * as necessary to reset the file pointer.
	 *
	 * \remarks
	 * If looping is ON, and an application sets it to OFF while the movie is playing, playback might
	 * not stop at the end of the movie.  In that case, playback will end after the next loop iteration.
	 *
	 * \sa CriMvEasyPlayer::GetLoopFlag(), CriMvFileReaderInterface::Seek()
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		���[�v�Đ��t���O�̎w��
	 *  \param		sw		���[�v�X�C�b�`�BON�̏ꍇ�̓��[�v����AOFF�̏ꍇ�̓��[�v�����ɂȂ�܂��B
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * ���[�v�Đ��̗L����ݒ肵�܂��B�f�t�H���g�̓��[�vOFF�ł��B<br>
	 * ���[�v�Đ�ON�ɂ����ꍇ�́A���[�r�̏I�[�܂ōĐ����Ă��n���h����Ԃ�MVEASY_STATUS_PLAYEND�ɂȂ炸�A
	 * ���[�r�̐擪����Đ����J��Ԃ��܂��B<br>
	 * �t�@�C�����w��ōĐ����Ă���ꍇ�́A�Ō�܂œǂݍ��񂾂��� CriMvFileReaderInterface::Seek()���g����
	 * �ǂݍ��݈ʒu���t�@�C���̐擪�ɖ߂��܂��B
	 * 
	 * ���[�v�Đ�OFF�ɐݒ肵���ꍇ�́A���̂Ƃ��ǂݍ���ł������[�r�̏I�[�܂ōĐ�����ƁA
	 * �n���h����Ԃ�MVEASY_STATUS_PLAYEND�ɑJ�ڂ��܂��B<br>
	 * �Đ����Ƀ��[�vOFF�ɂ����ꍇ�A�^�C�~���O�ɂ���ẮA�Đ����̃��[�r�I�[�ŏI��炸�A���̌J��Ԃ�
	 * �Đ��܂Ŏ��s����܂��B
	 * 
	 * ���݂̃��[�v�ݒ���擾����ɂ� CriMvEasyPlayer::GetLoopFlag()���g���Ă��������B
	 *
	 * \sa CriMvEasyPlayer::GetLoopFlag(), CriMvFileReaderInterface::Seek()
	 */
	void SetLoopFlag(CriBool sw, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Retrieves the value of the looping flag
	 * \return      The current looping setting
	 *
	 * By default, looping is OFF and playback will stop when it reaches the end.  You can 
	 * change this behavior by calling CriMvEasyPlayer::SetLoopFlag().
	 *
	 * \sa CriMvEasyPlayer::SetLoopFlag()
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		���[�v�Đ��t���O�̎擾
	 *  \param		err		�G���[���i�ȗ��j
	 *  \return		���݂̃��[�v�Đ��ݒ�
	 * 
	 * ���݂̃��[�v�ݒ���擾���܂��B
	 * ���[�v�ݒ�� CriMvEasyPlayer::SetLoopFlag() �ŕύX���邱�Ƃ��ł��܂��B
	 * 
	 * \sa CriMvEasyPlayer::SetLoopFlag()
	 */
	CriBool GetLoopFlag(CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Gets the amount of time that a movie has been playing
	 * \param       count       Number of timer units since the movie started playing
	 * \param       unit        Number of timer units per second
	 * \param       err         Optional error code
	 *
	 * This function retrieves the absolute time that has elapsed since a movie started playing.
	 * The time value is returned in two parts - a counter with an arbitrary interval, and the 
	 * number of timer ticks per second.  To determine the playing time in seconds, divide
	 * \a count by \a unit.
	 *
	 * Before playback has started, and after it has stopped, this function will return a \a count value of 0.
	 *
	 * The value retrieved is the value of the master timer for the handle, not the time of the current frame itself.
	 * To get the video frame time, check the CriMvFrameInfo structure once you have retrieved the frame.
	 *
	 * \remarks
	 * Note that this function provides an absolute playback time - it does <b>not</b> wrap to 0
	 * when the movie loops.
	 *
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�Đ������̎擾
	 *  \param	count	�^�C�}�J�E���g
	 *  \param	unit	�P�b������̃^�C�}�J�E���g�l�Bcount �� unit �ŕb�P�ʂ̎����ƂȂ�܂��B
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * �^�C�}�������擾���܂��B������count��unit�̓�̕ϐ��ŕ\�����܂��B<br>
	 * count �� unit �ŕb�P�ʂ̎����ƂȂ�悤�Ȓl��Ԃ��܂��B<br>
	 * �Đ��J�n�O�i CriMvSoundInterface::Start()�Ăяo���O�j�����
	 * �Đ���~��i CriMvSoundInterface::Stop()�Ăяo����j�́A�����O�i�^�C�}�J�E���g���O�j��Ԃ��܂��B<br>
	 * �{�֐��̓}�X�^�^�C�}�Ŏw�肳�ꂽ�^�C�}�̎�����Ԃ������ŁA�r�f�I�t���[���̎�����Ԃ����̂ł͂���܂���B<br>
	 * �擾�����r�f�I�t���[���̖{���̕\�������́A�r�f�I�t���[���擾���� CriMvFrameInfo �\���̂��Q�Ƃ��Ă��������B
	 */
	void GetTime(CriUint64 &count, CriUint64 &unit, CriError &err=CriMv::ErrorContainer);	// only refer time of SyncMasterTimer

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Gets information about the movie
	 * \param       stmprm      Movie information structure
	 * \param       err         Optional error code
	 * \return      TRUE        if movie information was successfully retrieved
	 * \return      FALSE       if the header has not yet been decoded, or if an error occurred
	 *
	 * Populates the passed \a CriMvStreamingParameters structure with information about the current movie,
	 * including the bitrate, resolution, audio track information, subtitle availability, and more.
	 *
	 * This function is available once the handle status has changed to MVEASY_STATUS_WAIT_PREP.  If an application
	 * needs this information before starting playback  (for instance, to set up for playing subtitles, or to
	 * allocate a display surface based on the size of the movie), call CriMvEasyPlayer::DecodeHeader(), then
	 * call GetMovieInfo().
	 *
	 * \remarks
	 * When doing concatenated playback (via CriMvEasyPlayer::SetFileRequestCallback()), GetMovieInfo() will 
	 * return information about the currently playing movie.
	 *
	 * \sa CriMvEasyPlayer::DecodeHeader(), CriMvEasyPlayer::SetFileRequestCallback()
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		���[�r���̎擾
	 *  \param		stmprm		���[�r���
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * ���[�r��� CriMvStreamingParameters ���擾���܂��B<br>
	 * ���[�r��񂩂�͎�Ƀr�b�g���[�g��𑜓x�A�I�[�f�B�I���Ȃǂ��킩��܂��B<br>
	 * ���[�r����EasyPlayer�n���h���̏�Ԃ� MVEASY_STATUS_WAIT_PREP �ȍ~�ɂȂ������Ǝ擾�ł��܂��B<br>
	 * �Đ��J�n�O�Ƀ��[�r����m�肽���ꍇ�́A CriMvEasyPlayer::DecodeHeader()���Ăяo���ăw�b�_��͂��s���Ă��������B
	 * 
	 * �A���Đ����s�����ꍇ�A�Ō�Ɏ擾�����t���[�����܂ރ��[�r�t�@�C���ɂ��Ă̏���Ԃ��܂��B
	 * 
	 * \sa CriMvEasyPlayer::DecodeHeader()
	 */
	CriBool GetMovieInfo(CriMvStreamingParameters &stmprm, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Changes the parameters for the movie
	 * \param       stmprm      Movie information structure
	 * \param       err         Optional error code
	 *
	 * This is a DEBUG function and should not normally be used by applications.
	 *
	 * This function allows an application to change the streaming parameters for the movie as a whole. 
	 * It is available once the handle status has changed to MVEASY_STATUS_WAIT_PREP.
	 *
	 * To use this function, first call CriMvEasyPlayer::DecodeHeader(), then call CriMvEasyPlayer::GetMovieInfo()
	 * to retrieve the current movie parameters.  Change the fields of the CriMvStreamingParameters structure
	 * as appropriate, then call SetStreamingParameters().
	 * 
	 * \sa CriMvEasyPlayer::GetMovieInfo(), CriMvEasyPlayer::DecodeHeader()
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�X�g���[�~���O�p�����[�^�̕ύX
	 *  \param		stmprm		�X�g���[�~���O�p�����[�^
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * �{�֐��͒ʏ�A�A�v���P�[�V��������͎g�p���܂���B�f�o�b�O�p�̊֐��ł��B
	 * 
	 * ���[�r�Đ��̂��߂̃X�g���[�~���O�p�����[�^��EasyPlayer�n���h���ɐݒ肵�܂��B<br>
	 * �X�g���[�~���O�p�����[�^���w��ł���̂́AEasyPlayer�n���h����Ԃ�MVEASY_STATUS_WAIT_PREP�̎������ł��B<br>
	 * ���̊֐��́A�ǂݍ��݃o�b�t�@�T�C�Y�ȂǍׂ��ȃp�����[�^��S�ăA�v���P�[�V�����Œ����������ꍇ�Ɏg���܂��B<br>
	 * CriMvEasyPlayer::DecodeHeader()�Ńw�b�_��͂��s�������ƁA CriMvEasyPlayer::GetMovieInfo()�Ŏ擾�ł���
	 * ���[�r��񂪂��̂܂܃X�g���[�~���O�p�����[�^�ƂȂ�܂��̂ŁA�����������l��ύX���āA�{�֐��Őݒ肵�Ȃ�
	 * ���Ă��������B
	 * 
	 * \sa CriMvEasyPlayer::GetMovieInfo(), CriMvEasyPlayer::DecodeHeader()
	 */
	void SetStreamingParameters(CriMvStreamingParameters *stmprm, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_VIDEO_FRAME
	 *
	 * \brief       Determine if a new video frame can be displayed
	 *
	 * \param       err         Optional error code
	 *
	 * \return      TRUE        if a frame is ready to be displayed
	 * \return      FALSE       if the movie is paused or not playing, or if a new frame is
	 *                          not yet decoded or ready to be displayed
	 *
	 * Each frame in a movie corresponds to an absolute time, with the first frame being time 0.
	 * The playback time is controlled by the master timer for this handle.
	 * If the time of the next frame is less than or equal to the current playback time, as determined 
	 * by CriMvEasyPlayer::GetTime(), it is ready to be displayed, and this function will return TRUE.
	 *
	 * If an application needs to do some complex or lengthy processing before displaying a frame (such
	 * as locking a texture), it should call this function first.
	 *
	 * \remarks
	 * If the next frame has not yet been decoded, this function will return FALSE regardless of the playback time.
	 *
	 * \sa CriMvEasyPlayer::GetTime()
	 */
	/*JP
	 * \ingroup MODULE_VIDEO_FRAME
	 * \brief		���t���[���̕\����������
	 *  \param		err		�G���[���i�ȗ��j
	 *  \return 	���̃r�f�I�t���[�������łɕ\�������ɂȂ��Ă���ꍇ��TRUE(1)��Ԃ��܂��B<br>
	 * 				���̃r�f�I�t���[�����܂��f�R�[�h�ł��Ȃ��ꍇ��FALSE(0)��Ԃ��܂��B
	 * 
	 * ���̃r�f�I�t���[�������łɕ\�������ɂȂ��Ă��邩�ǂ�����₢���킹�܂��B<br>
	 * �������f�R�[�h���x��Ă��Ď��̃r�f�I�t���[�����܂��f�R�[�h�ł��Ă��Ȃ��ꍇ�́A�Đ������Ɋ֌W
	 * �Ȃ�FALSE��Ԃ��܂��B<br>
	 * �܂肱�̊֐��́u���̃t���[���� GetFrameOnTime�֐��Ŏ擾�ł��邩�ǂ����v�𒲂ׂ܂��B<br>
	 * �r�f�I�t���[�������ۂɎ擾���������ɂ��Ȃ���΂����Ȃ������i�Ⴆ�΃e�N�X�`�����b�N�Ȃǁj
	 * ������ꍇ�́A���̊֐��Ńt���[���擾�̐��ۂ𔻒肵�Ă��珈�����Ă��������B
	 *
	 * \sa CriMvEasyPlayer::GetTime()
	 */
	CriBool IsNextFrameOnTime(CriError &err=CriMv::ErrorContainer);

#if !defined(XPT_TGT_EE)
	/*EN
	 * \ingroup MODULE_VIDEO_FRAME
	 *
	 * \brief       Loads video frame data into a buffer, in 32 bit ARGB format
	 * \param       imagebuf        Pointer to output buffer
	 * \param       pitch           Pitch of output buffer, in bytes
	 * \param       bufsize         Size of output buffer, in bytes
	 * \param       frameinfo       Video frame information structure
	 * \param       err             Optional error code
	 * \return      TRUE            if a frame was retrieved
	 * \return      FALSE           otherwise
	 *
	 * Copies the next decoded frame into the 32 bit ARGB image buffer pointed to by \a imagebuf.
	 * If the display time is less than the frame time, or if the frame has not yet been decoded, the
	 * \a frameinfo parameter will be cleared and this function will return FALSE.
	 *
	 * The \a pitch parameter is the width, in bytes, of each row of the frame image, including any padding.
	 *
	 * To determine if the next frame is ready to be displayed, call CriMvEasyPlayer::IsNextFrameOnTime().
	 *
	 * CRI Movie decodes video frames into YUV420 format internally, and stores them in frame pools (see
	 * CriMvEasyPlayer::SetNumberOfFramePools()) for later display.  When this function is called, 
	 * the frame must first converted to ARGB format.  This can be a very CPU-intensive operation,
	 * especially on the PS3 and Xbox360.  For 1280 x 720 video resolution, this can use almost an
	 * entire vsync interval.  On these platforms, we recommend implementing a pixel shader and calling
	 * CriMvEasyPlayer::GetFrameOnTimeAsYUVBuffers() instead.
	 *
	 * \remarks <BR>
	 * If an application uses this function, an application need to call CriMv::InitializeFrame32bitARGB() 
	 * after CriMv::Initialize(). If an application calls this function without CriMv::InitializeFrame32bitARGB() 
	 * calling, this function failed and an error callback occurs.
	 * 
	 * \remarks
	 * This function is not available on the PS2.
	 * 
	 * \sa CriMvEasyPlayer::IsNextFrameOnTime(), CriMvEasyPlayer::GetFrameOnTimeAsYUVBuffers(),
	 *     CriMvEasyPlayer::SetNumberOfFramePools()
	 */
	/*JP
	 * \ingroup MODULE_VIDEO_FRAME
	 * \brief		32bit ARGB�t�H�[�}�b�g�ł̃f�R�[�h���ʂ̎擾
	 *  \param		imagebuf		�o�̓o�b�t�@�|�C���^
	 *  \param		pitch			�o�̓o�b�t�@�̃s�b�` [byte]
	 *  \param		bufsize			�o�̓o�b�t�@�̃T�C�Y [byte]
	 *  \param		frameinfo		�擾�����r�f�I�t���[���̏��\����
	 *  \param		err		�G���[���i�ȗ��j
	 *  \return		�t���[�����擾�ł����ꍇ��TRUE(1)�A�ł��Ȃ������ꍇ��FALSE(0)��Ԃ��܂��B
	 * 
	 * 32bit ARGB�t�H�[�}�b�g�ŁA�\�������ɂȂ��Ă���r�f�I�t���[�����擾���܂��B<br>
	 * ���̊֐����Ăяo���ꍇ�́AARGB�o�b�t�@�̎��̂��m�ۂ��������ŌĂяo���K�v������܂��B<br>
	 * �r�f�I�t���[���͈���imagebuf�Ŏw�肵��ARGB�o�b�t�@�ɏ����o����܂��B<br>
	 * ���������̃r�f�I�t���[���̕\�������ɂȂ��Ă��Ȃ�������A�f�R�[�h���I����Ă��Ȃ������ꍇ��
	 * �t���[���擾�ł����Aframeinfo�̒��g�̓N���A����܂��B<br>
	 * ���O�Ƀr�f�I�t���[�����擾�ł��邩�ǂ�����m�肽���ꍇ�� CriMvEasyPlayer::IsNextFrameOnTime()
	 * ���g�p���Ă��������B
	 * 
	 * 32bit ARGB �̎��ۂ̃s�N�Z���f�[�^�̕��тɂ��ẮA���̃v���b�g�t�H�[���ōł��W���I��
	 * �t�H�[�}�b�g�ɂȂ�܂��B
	 * 
	 * ����: <BR>
	 * �{�֐����g�p����ꍇ�̓t���[���ϊ��̏����� CriMv::InitializeFrame32bitARGB()�̌Ăяo����
	 * ���O�ɕK�v�ł��B�t���[���ϊ��̏��������s�킸�ɖ{�֐����Ăяo�����ꍇ�̓t���[���擾�Ɏ��s���A
	 * �G���[�R�[���o�b�N���������܂��B
	 * 
	 * ����: <BR>
	 * PS3, Xbox360 �ł��{�֐��͎g�p�ł��܂����A�ƂĂ�CPU���ׂ̍����֐��ƂȂ�܂��B<BR>
	 * �𑜓x�� 1280x720 �̃��[�r��{�֐����t���[���擾�����1vsync�߂����Ԃ�������܂��B<BR>
	 * PS3, Xbox360 �ł�CriMvEasyPlayer::GetFrameOnTimeAsYUVBuffers() �֐��� �s�N�Z���V�F�[�_�[
	 * �̑g�ݍ��킹�ɂ��t���[���ϊ����������߂��܂��B<BR>
	 * 
	 * ���l: <BR>
	 * PS2��CRI Movie �͖{�֐��ɑΉ����Ă��܂���B
	 * 
	 * \sa CriMvEasyPlayer::IsNextFrameOnTime()
	 */
	CriBool GetFrameOnTimeAs32bitARGB(CriUint8 *imagebuf, CriUint32 pitch, CriUint32 bufsize, CriMvFrameInfo &frameinfo, 
									CriError &err=CriMv::ErrorContainer);
#endif

#if !defined(XPT_TGT_EE)
	/*EN
	 * \ingroup MODULE_VIDEO_FRAME
	 *
	 * \brief       Loads video frame data into a set of Y,U,V separate buffers
	 * \param       yuvbuffers      Pointer to Y,U,V buffer data structure
	 * \param       frameinfo       Video frame information structure
	 * \param       err             Optional error code
	 * \return      TRUE            if a frame was retrieved
	 * \return      FALSE           otherwise
	 *
	 * Copies the next decoded frame into the Y,U,V image buffers pointed to by \a yuvbuffers, for use
	 * with a pixel shader.  CRI Movie decodes video frames into YUV420 format internally, so this is
	 * a very efficient function.
	 *
	 * If the display time is less than the frame time, or if the frame has not yet been decoded, the
	 * \a frameinfo parameter will be cleared and this function will return FALSE.
	 *
	 * To determine if the next frame is ready to be displayed, call CriMvEasyPlayer::IsNextFrameOnTime().
	 * 
	 * \remarks
	 * If the movie does not have an alpha channel, the alpha buffer fields of the \a CriMvYuvBuffers 
	 * structure are not used.
	 *
	 * \remarks
	 * This function is not available on the PS2.
	 * 
	 * \sa CriMvEasyPlayer::IsNextFrameOnTime()
	 */
	/*JP
	 * \ingroup MODULE_VIDEO_FRAME
	 * \brief		YUV�ʃo�b�t�@�ւ̃f�R�[�h���ʂ̎擾
	 *  \param		yuvbuffers		YUV�ʃo�b�t�@�̃p�����[�^�\����
	 *  \param		frameinfo		�擾�����r�f�I�t���[���̏��\����
	 *  \param		err		�G���[���i�ȗ��j
	 *  \return		�t���[�����擾�ł����ꍇ��TRUE(1)�A�ł��Ȃ������ꍇ��FALSE(0)��Ԃ��܂��B
	 * 
	 * YUV�ʃo�b�t�@�`���ŕ\�������ɂȂ��Ă���r�f�I�t���[�����擾���܂��B<br>
	 * YUV�ʃo�b�t�@�`���̓s�N�Z���V�F�[�_�[�Ńt���[����`�悷�邽�߂̏o�̓t�H�[�}�b�g�ł��B<br>
	 * ���̊֐����Ăяo���ꍇ�́AYUV�ʃo�b�t�@�̎��̂��m�ۂ��������ŌĂяo���K�v������܂��B<br>
	 * �r�f�I�t���[���͈���yuvbuffers�Ŏw�肵��YUV�ʃo�b�t�@�ɏ����o����܂��B<br>
	 * ���������̃r�f�I�t���[���̕\�������ɂȂ��Ă��Ȃ�������A�f�R�[�h���I����Ă��Ȃ������ꍇ��
	 * �t���[���擾�ł����Aframeinfo�̒��g�̓N���A����܂��B<br>
	 * ���O�Ƀr�f�I�t���[�����擾�ł��邩�ǂ�����m�肽���ꍇ�� CriMvEasyPlayer::IsNextFrameOnTime()
	 * ���g�p���Ă��������B<BR>
	 * <BR>
	 * �A���t�@���[�r�Đ����s��Ȃ��ꍇ�́A���� yuvbuffers ��Alpha�e�N�X�`���֘A�̃p�����[�^�͎g�p���܂���B<BR>
	 * 
	 * ���l: <BR>
	 * PS2��CRI Movie �͖{�֐��ɑΉ����Ă��܂���B
	 * 
	 * \sa CriMvEasyPlayer::IsNextFrameOnTime()
	 */
	CriBool GetFrameOnTimeAsYUVBuffers(CriMvYuvBuffers *yuvbuffers, CriMvFrameInfo &frameinfo, 
										CriError &err=CriMv::ErrorContainer);
#endif

#if defined(XPT_TGT_PC) || defined(XPT_TGT_SH7269) || defined(XPT_TGT_TRGP6K)
	/*EN
	 * \ingroup MODULE_VIDEO_FRAME
	 *
	 * \brief       Loads video frame data into a buffer, in YUV422 format
	 * \param       imagebuf        Pointer to output buffer
	 * \param       pitch           Pitch of output buffer, in bytes
	 * \param       bufsize         Size of output buffer, in bytes
	 * \param       frameinfo       Video frame information structure
	 * \param       err             Optional error code
	 * \return      TRUE            if a frame was retrieved
	 * \return      FALSE           otherwise
	 *
	 * Copies the next decoded frame into the YUV422 texture buffer pointed to by \a imagebuf.
	 * CRI Movie decodes video frames into YUV420 format internally, so there is some internal conversion
	 * required when using this function.
	 *	 
	 * If the display time is less than the frame time, or if the frame has not yet been decoded, the
	 * \a frameinfo parameter will be cleared and this function will return FALSE.
	 *
	 * The \a pitch parameter is the width, in bytes, of each row of the frame image, including any padding.
	 *
	 * To determine if the next frame is ready to be displayed, call CriMvEasyPlayer::IsNextFrameOnTime().
	 *
	 * \remarks
	 * This function is currently only available in the PC version of CRI Movie.
	 * 
	 * \sa CriMvEasyPlayer::IsNextFrameOnTime()
	 */
	/*JP
	 * \ingroup MODULE_VIDEO_FRAME
	 * \brief		YUV422�t�H�[�}�b�g�ł̃f�R�[�h���ʂ̎擾
	 *  \param		imagebuf		�o�̓o�b�t�@�̃|�C���^
	 *  \param		pitch			�o�̓o�b�t�@�̃s�b�` [byte]
	 *  \param		bufsize			�o�̓o�b�t�@�T�C�Y [byte]
	 *  \param		frameinfo		�擾�����r�f�I�t���[���̏��\����
	 *  \param		err				�G���[���i�ȗ��j
	 *  \return		�t���[�����擾�ł����ꍇ��TRUE(1)�A�ł��Ȃ������ꍇ��FALSE(0)��Ԃ��܂��B
	 * 
	 * YUV422�e�N�X�`���t�H�[�}�b�g�ŁA�\�������ɂȂ��Ă���r�f�I�t���[�����擾���܂��B<br>
	 * ���̊֐����Ăяo���ꍇ�́AYUV�o�b�t�@�̎��̂��m�ۂ��������ŌĂяo���K�v������܂��B<br>
	 * �r�f�I�t���[���͈���imagebuf�Ŏw�肵��YUV�o�b�t�@�ɏ����o����܂��B<br>
	 * ���������̃r�f�I�t���[���̕\�������ɂȂ��Ă��Ȃ�������A�f�R�[�h���I����Ă��Ȃ������ꍇ��
	 * �t���[���擾�ł����Aframeinfo�̒��g�̓N���A����܂��B<br>
	 * ���O�Ƀr�f�I�t���[�����擾�ł��邩�ǂ�����m�肽���ꍇ�� CriMvEasyPlayer::IsNextFrameOnTime()
	 * ���g�p���Ă��������B
	 * 
	 * �y���l�z<br>
	 * ���݂́APC��CRI Movie �̂ݖ{�֐��ɑΉ����Ă��܂��B
	 * 
	 * \sa CriMvEasyPlayer::IsNextFrameOnTime()
	 */
	CriBool GetFrameOnTimeAsYUV422(CriUint8 *imagebuf, CriUint32 pitch, CriUint32 bufsize, CriMvFrameInfo &frameinfo, 
									CriError &err=CriMv::ErrorContainer);
#endif

#if defined(XPT_TGT_IPHONE) || defined(XPT_TGT_WINMO) || defined(XPT_TGT_ANDROID) || defined(XPT_TGT_CQSH2A) || defined(XPT_TGT_ACRODEA) || defined(XPT_TGT_NACL) || defined(XPT_TGT_SH7269) || defined(XPT_TGT_PC)|| defined(XPT_TGT_TRGP6K)
	/*EN
	 * \ingroup MODULE_VIDEO_FRAME
	 * \brief		Get video frame data to 16bit RGB565 format buffer
	 * 
	 * This function is added for a prototype library for iPhone
	 * Please add comments when releasing the SDK.
	 *
	 */
	/*JP
	 * \ingroup MODULE_VIDEO_FRAME
	 * \brief		16bit RGB565�t�H�[�}�b�g�ł̃f�R�[�h���ʂ̎擾
	 *
	 * ���̊֐���iPhone��CRI Movie�̃v���g�^�C�v�p�̊֐��錾�ł��B
	 * SDK�Ƃ��ă����[�X����ۂ́A�R�����g��ǉ����ĉ������B
	 * 
	 */	
	CriBool GetFrameOnTimeAsRGB565(CriUint8 *imagebuf, CriUint32 pitch, CriUint32 bufsize, CriMvFrameInfo &frameinfo, 
									CriError &err=CriMv::ErrorContainer);
#endif
	
#if defined(XPT_TGT_EE)
	/*EN
	 * \ingroup MODULE_VIDEO_FRAME
	 *
	 * \brief       Gets a reference to EasyPlayer's internal video frame buffer, in 32 bit ARGB format
	 * \param       frameinfo       Video frame information structure
	 * \param       err             Optional error code
	 * \return      TRUE            if a frame was retrieved
	 * \return      FALSE           otherwise
	 *
	 * Locks the internal buffer for the current video frame in memory, and retrieves a pointer to it.  This is
	 * different behavior than the GetFrameXXX()functions, which copy the frame data into a caller-supplied location.
	 *
	 * On successful return from this function, the fields of \a frameinfo will be populated with information
	 * about the frame.  In particular, the \a imageptr field will be set to the image buffer, in 32 bit ARGB format.
	 * After calling this function, an application must copy the video frame into its own buffer or transfer it to
	 * GS local memory via DMA.
	 *
	 * To determine if the next frame is ready to be displayed, call CriMvEasyPlayer::IsNextFrameOnTime().
	 * 
	 * \remarks
	 * After the application has copied the frame data, it must call CriMvEasyPlayer::UnlockFrame().
	 *
	 * \remarks
	 * This function is only available on the PS2.
	 *
	 * \sa CriMvEasyPlayer::IsNextFrameOnTime(), CriMvEasyPlayer::UnlockFrame()
	 */
	/*JP
	 * \ingroup MODULE_VIDEO_FRAME
	 * \brief		�f�R�[�h���ʗ̈�(ARGB32bit)�̃��b�N�BPS2��p�B
	 *  \param		frameinfo		���b�N�����r�f�I�t���[���̏��\����
	 *  \param		err				�G���[���i�ȗ��j
	 * 
	 * �{�֐���PS2��p�̃t���[���擾�֐��ŁA���@��� GetFrame �֐��ɑ������܂��B<BR>
	 * PS2�ł� GetFrame �֐��̑���ɖ{�֐��� UnlockFrame �֐����g�p���ăt���[���擾���s���܂��B<BR>
	 * GetFrame �֐��͏o�̓o�b�t�@���w�肵�Ă����փf�R�[�h���ʂ��擾����̂ɑ΂��ALockFrame �֐��̓o�b�t�@���w�肹��
	 * CriMvEasyPlayer�n���h�������ɂ���f�R�[�h���ʃo�b�t�@�̃|�C���^���擾����Ƃ��낪�Ⴂ�܂��B<BR>
	 * 
	 * �{�֐��̓f�R�[�h���ʂ̃������̈���Q�ƊJ�n���邽�߂Ƀ��b�N���܂��B<BR>
	 * ���̊֐��Ńt���[�������b�N�ł���̂́A���̃t���[�����\���\���ԂɂȂ��Ă���ꍇ�݂̂ł��B<BR>
	 * �A�v���P�[�V�����̓t���[�������b�N�������ƁA�f�R�[�h���ʂ�DMA�Ńe�N�X�`���̈�֓]�����邩�A
	 * �ʃo�b�t�@�փR�s�[����Ȃǂ̏������s���܂��B<BR>
	 * �f�R�[�h���ʂ̎Q�Ƃ��I�������ɂ́A�K�� CriMvEasyPlayer::UnlockFrame() �֐����Ăяo���ĎQ�ƏI����ʒm���Ă��������B<BR>
	 * 
	 * ���l: <BR>
	 * �{�֐���PS2��CRI Movie �̂ݑΉ����Ă��܂��B
	 * 
	 * \sa CriMvEasyPlayer::IsNextFrameOnTime(), CriMvEasyPlayer::UnlockFrame()
	 */
	CriBool LockFrameOnTimeAs32bitARGB_PS2(CriMvFrameInfo &frameinfo, CriError &err=CriMv::ErrorContainer);
#endif

	/*EN
	 * \ingroup MODULE_VIDEO_FRAME
	 *
	 * \brief       Unlocks the video frame
	 * \param       frameinfo       Pointer to locked video frame information
	 * \param       err             Optional error code
	 *
	 * Unlocks the video frame that was locked in memory by a call to one of the LockFrameXXX() functions.
	 *
	 * The \a frameinfo parameter must be the same one that was passed to LockFrameXXX().
	 *
	 * If the frame has been locked, EasyPlayer will not be able to retrieve the next frame until the frame is
	 * unlocked.  An attempt to lock the same frame more than once will fail.
	 *
	 * The LockFrameXXX() functions are CriMvEasyPlayer::LockFrameOnTimeAs32bitARGB_PS2() and 
	 * CriMvEasyPlayer::LockFrameOnTimeAsYUVBuffers().  Depending on the platform, only one or the other of
	 * these functions will be available.
	 *
	 * \sa CriMvEasyPlayer::LockFrameOnTimeAs32bitARGB_PS2(), CriMvEasyPlayer::LockFrameOnTimeAsYUVBuffers()
	 */
	/*JP
	 * \ingroup MODULE_VIDEO_FRAME
	 * \brief		���b�N�t���[���Ŏ擾�����f�R�[�h���ʂ��A�����b�N����
	 *  \param		frameinfo		���b�N�����r�f�I�t���[���̏��\����
	 *  \param		err				�G���[���i�ȗ��j
	 * 
	 * �{�֐��̓��b�N�t���[���֐����g���ă��b�N���Ă����t���[�����A�����b�N���A�������Q�Ƃ̏I����ʒm���܂��B<BR>
	 * ���b�N�t���[���֐��ɂ� CriMvEasyPlayer::LockFrameOnTimeAs32bitARGB_PS2() �� CriMvEasyPlayer::LockFrameOnTimeAsYUVBuffers()
	 * ������܂����A�ǂ���̊֐����g���ă��b�N�����ꍇ���A�{�֐����g���ăA�����b�N���܂��B<BR>
	 * �{�֐��̈����ɂ́A�ǂ̃t���[�����A�����b�N���邩���w�����邽�߂ɁA���b�N�t���[���֐��Ŏ擾�����t���[�����\���̂��w�肵�܂��B<BR>
	 * 
	 * �{�֐��ŃA�����b�N�����t���[���́A�Ȍ�A���Ƀr�f�I�t���[���̃f�R�[�h�o�̓o�b�t�@�Ƃ��Ďg�p����܂��B<BR>
	 * �P�x�A�����b�N�����t���[����������x���b�N���邱�Ƃ͏o���܂���B<BR>
	 * 
	 * \sa CriMvEasyPlayer::LockFrameOnTimeAs32bitARGB_PS2(), CriMvEasyPlayer::LockFrameOnTimeAsYUVBuffers()
	 */
	CriBool UnlockFrame(CriMvFrameInfo *frameinfo, CriError &err=CriMv::ErrorContainer);

#if !defined(XPT_TGT_EE)
	/*EN
	 *
	 * \brief       Gets a reference to EasyPlayer's internal video frame buffer
	 * \param       yuvbuffers      Y,U,V buffer data structure
	 * \param       frameinfo       Video frame information structure
	 * \param       err             Optional error code
	 * \return      TRUE            if a frame was retrieved
	 * \return      FALSE           otherwise
	 *
	 * Locks the internal buffer for the current video frame in memory, and retrieves a pointer to it.  This is
	 * different behavior than the GetFrameXXX() functions, which copy the frame data into a caller-supplied location.
	 *
	 * On successful return from this function, the fields of \a yuvbuffers will be set to the Y,U,V fields of the
	 * video frame, and the fields of \a frameinfo will be populated with information about the frame. 
	 *
	 * After calling this function, an application must copy the video frame into its own buffer or transfer it to
	 * texture memory.
	 *
	 * To determine if the next frame is ready to be displayed, call CriMvEasyPlayer::IsNextFrameOnTime().
	 * 
	 * \remarks
	 * After the application has copied the frame data, it must call CriMvEasyPlayer::UnlockFrame().
	 *
	 * \remarks
	 * This function is not available on the PS2.
	 *
	 * \sa CriMvEasyPlayer::IsNextFrameOnTime(), CriMvEasyPlayer::UnlockFrame()
	 */
	/*JP
	 * \ingroup MODULE_VIDEO_FRAME
	 * \brief		�f�R�[�h���ʗ̈�̃��b�N
	 *  \param		yuvbuffers		YUV�ʃo�b�t�@�̃p�����[�^�\����
	 *  \param		frameinfo		���b�N�����r�f�I�t���[���̏��\����
	 *  \param		err				�G���[���i�ȗ��j
	 * 
	 * GetFrame �Ƃ͕ʂ̎d�l�̃t���[���擾�֐��ł��B<BR>
	 * �{�֐��� UnlockFrame �֐��ƃZ�b�g�Ŏg�p���܂��B<BR>
	 * GetFrame �֐��͏o�̓o�b�t�@���w�肵�Ă����փf�R�[�h���ʂ��擾����̂ɑ΂��ALockFrame �֐��̓o�b�t�@���w�肹��
	 * CriMvEasyPlayer�n���h�������ɂ���f�R�[�h���ʃo�b�t�@�̃|�C���^���擾����Ƃ��낪�Ⴂ�܂��B<BR>
	 * 
	 * �{�֐��̓f�R�[�h���ʂ̃������̈���Q�ƊJ�n���邽�߂Ƀ��b�N���A
	 * �f�R�[�h���ʂ�YUV�R��ނ̃o�b�t�@�ɂ��Ă̏������� yuvbuffers �Ɋi�[���܂��B<BR>
	 * ���̊֐��Ńt���[�������b�N�ł���̂́A���̃t���[�����\���\���ԂɂȂ��Ă���ꍇ�݂̂ł��B<BR>
	 * �A�v���P�[�V�����̓t���[�������b�N�������ƁA�f�R�[�h���ʂ��e�N�X�`���̈�փR�s�[���邩�A
	 * �ʃo�b�t�@�փR�s�[����Ȃǂ̏������s���܂��B<BR>
	 * �f�R�[�h���ʂ̎Q�Ƃ��I�������ɂ́A�K�� CriMvEasyPlayer::UnlockFrame() �֐����Ăяo���ĎQ�ƏI����ʒm���Ă��������B<BR>
	 * 
	 * ���l: <BR>
	 * PS2��CRI Movie �͖{�֐��ɑΉ����Ă��܂���B
	 * 
	 * \sa CriMvEasyPlayer::UnlockFrame()
	 */
	CriBool LockFrameOnTimeAsYUVBuffers(CriMvYuvBuffers &yuvbuffers, CriMvFrameInfo &frameinfo, 
										CriError &err=CriMv::ErrorContainer);
#endif

	/*EN
	 * \ingroup MODULE_VIDEO_FRAME
	 *
	 * \brief       Discards the next video frame
	 * \param       frameinfo       Discarded frame information structure
	 * \param       err             Optional error code
	 * \return      TRUE            if a frame was available to discard
	 * \return      FALSE           otherwise
	 * 
	 * Discards the next video frame, if it is available, and populates \a frameinfo with information about
	 * the frame.  Note that the \a imageptr field of that structure will not be available and should not 
	 * be referenced.
	 *
	 * To determine if the next frame is ready to be displayed, call CriMvEasyPlayer::IsNextFrameOnTime().
	 *
	 * \sa CriMvEasyPlayer::IsNextFrameOnTime()
	 */
	/*JP
	 * \ingroup MODULE_VIDEO_FRAME
	 * \brief		���t���[�����擾�����Ɏ̂Ă�
	 *  \param		frameinfo		�j�������r�f�I�t���[���̏��\����
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * �f�R�[�h�ς݂̃r�f�I�t���[�����̂Ă����ꍇ�Ɏg�p����֐��ł��B<br>
	 * �t���[���擾�֐��Ɣ�ׂ�ƁA�o�͗p�o�b�t�@����������K�v�����������������ł��B<br>
	 * CriMvEasyPlayer::IsNextFrameOnTime()�Ŏ��t���[�����擾�ł��邱�Ƃ��m�F������A�{�֐����Ăяo���Ă��������B<br>
	 * ����frameinfo�ɂ͎Q�l�̂��߂ɔj�������r�f�I�t���[���̏�񂪊i�[����܂����A�f�R�[�h���ʎ��̂ɂ̓A�N�Z�X�ł��܂���B
	 * 
	 * \sa CriMvEasyPlayer::IsNextFrameOnTime()
	 */
	CriBool DiscardNextFrame(CriMvFrameInfo &frameinfo, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Sets the current channel for displaying subtitles
	 *
	 * \param       channel         Subtitle channel number
	 * \param       err             Optional error code
	 *
	 * A movie can contain up to 16 distinct sets of subtitles, each on its own display channel.  This can 
	 * be used, for instance, to provide multilanguage support.
	 *
	 * Subtitle channels are numbered sequentially, starting from 0, but do not need to be contiguous.
	 * For example, a movie can have 3 sets of subtitles, on channels 1, 5, and 7.  The number of channels 
	 * can be determined once CriMvEasyPlayer::GetMovieInfo() has completed successfully by looking at the 
	 * \a num_subtitle field of the \a CriMvStreamingParameters structure passed to that function.
	 *
	 * By default, subtitle playback is off.  To turn off subtitles once they have been turned on, pass
	 * CRIMV_SUBTITLE_CHANNEL_OFF as the value of \a channel.
	 *
	 * If the selected subtitle channel does not exist, subtitles will not be displayed.
	 *
	 * \remarks
	 * If an application turns on subtitle display with this function, it must periodically call
	 * CriMvEasyPlayer::GetSubtitleOnTime(), or else movie playback will stall.
	 *
	 * \sa CriMvEasyPlayer::GetMovieInfo(), CriMvEasyPlayer::DecodeHeader(), CriMvEasyPlayer::GetSubtitleOnTime()
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�擾���鎚���`���l���̐ݒ�
	 *  \param		channel	�����`���l��
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * �擾���鎚���`���l����ݒ肵�܂��B�f�t�H���g�͎����擾�����ł��B
	 * 
	 * CriMvEasyPlayer::DecodeHeader()�� CriMvEasyPlayer::GetMovieInfo()���g�����ƂŁA�Đ����郀�[�r��
	 * �����̎������܂�ł��邩���Đ��J�n�O�ɒm�邱�Ƃ��ł��܂��B
	 * 
	 * �f�[�^�����݂��Ȃ��`���l���ԍ����w�肵���ꍇ�́A�����͎擾�ł��܂���B<br>
	 * �f�t�H���g�ݒ�i�����擾�����j�ɂ������ꍇ�́A�`���l���Ƃ���CRIMV_SUBTITLE_CHANNEL_OFF���w�肵�Ă��������B
	 * 
	 * ���̊֐��Ŏ����`���l�����w�肵���ꍇ�́A���C�����[�v�������I�� CriMvEasyPlayer::GetSubtitleOnTime() ��
	 * ���s���Ă��������B�����擾�����I�ɍs��Ȃ��ꍇ�́A���[�r�Đ����r���Ŏ~�܂�܂��B
	 * 
	 * \sa CriMvEasyPlayer::GetMovieInfo(), CriMvEasyPlayer::DecodeHeader(), CriMvEasyPlayer::GetSubtitleOnTime
	 */
	void SetSubtitleChannel(CriSint32 channel, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_VIDEO_FRAME
	 *
	 * \brief       Retrieves subtitle data for the current frame, if available
	 *
	 * \param       bufptr          Buffer to receive subtitle data
	 * \param       bufsize         Size of buffer, in bytes
	 * \param       err             Optional error code
	 *
	 * \return      The number of bytes copied into \a bufptr
	 *
	 * If the movie contains subtitle data, and there is a subtitle for the current frame, up to \a bufsize
	 * bytes of the subtitle data for the active subtitle channel will be copied into \a bufptr.  Otherwise,
	 * the entire buffer will be filled with zeroes.
	 *
	 * \remarks
	 * If an application turns on subtitle display with CriMvEasyPlayer::SetSubtitleChannel(), it must 
	 * periodically call this function, or else movie playback will stall.
	 * 
	 * \remarks
	 * An application should not make assumptions as to whether the returned data is NUL-terminated.
	 * 
	 * \sa CriMvEasyPlayer::SetSubtitleChannel()
	 */
	/*JP
	 * \ingroup MODULE_VIDEO_FRAME
	 * \brief		�����f�[�^�̎擾
	 *  \param		bufptr		�o�̓o�b�t�@�|�C���^
	 *  \param		bufsize		�o�̓o�b�t�@�T�C�Y [byte]
	 *  \param		err			�G���[���i�ȗ��j
	 *  \return		�擾���������f�[�^�̃T�C�Y[byte]��Ԃ��܂��B
	 * 
	 * �\�������ɂȂ��Ă��鎚���f�[�^���擾���܂��B
	 * ���̊֐����Ăяo���ꍇ�́A�����p�o�b�t�@�̎��̂��m�ۂ��������ŌĂяo���Ă��������B<br>
	 * �����f�[�^�͈��� bufptr �Ŏw�肵���o�b�t�@�ɏ����o����܂��B<br>
	 * ���������f�[�^�� bufsize �����傫���ꍇ�́Abufsize �Ɏ��܂�ʂ��������o���A�c��͔j������܂��B
	 * 
	 * �������\�������̎����������ꍇ�́A�o�b�t�@�̒��g�̓N���A����܂��B
	 * 
	 * CriMvEasyPlayer::SetSubtitleChannel()�ő��݂��鎚���`���l�����w�肵�Ă���ꍇ�́A
	 * ���C�����[�v�������I�ɖ{�֐������s���Ă��������B<br>
	 * ���s���Ȃ��ꍇ�́A���[�r�Đ����r���Ŏ~�܂�܂��B
	 * 
	 * \sa CriMvEasyPlayer::SetSubtitleChannel()
	 */
	CriUint32 GetSubtitleOnTime(CriUint8 *bufptr, CriUint32 bufsize, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Sets a secondary sound interface
	 *
	 * \param       sound       Secondary sound interface to attach to this handle
	 * \param       err         Optional error code
	 *
	 * A subaudio interface allows an application to play two audio tracks at the same time.  This is 
	 * typically used to play a dialog track or sound effects along with background music.  After calling 
	 * this function, an application needs to call CriMvEasyPlayer::SetSubAudioTrack() to choose the secondary 
	 * audio track to play.
	 *
	 * Note that you must create a separate sound interface to pass to this function.  The interface used 
	 * in the CriMvEasyPlayer::Create() call can not be used.
	 *
	 * If a subaudio interface is used, the application must call CriMvEasyPlayer::DetachSubAudioInterface()
	 * when the EasyPlayer handle reaches the MVEASY_STATUS_STOP or MVEASY_STATUS_PLAYEND state before calling
	 * CriMvEasyPlayer::Destroy().
	 *
	 * Calling CriMvEasyPlayer::ResetAllParameters() will not affect the value set by this function.
	 *
	 * \remarks
	 * If this function is called, it must be called before starting playback (with either CriMvEasyPlayer::Prepare() 
	 * or CriMvEasyPlayer::Start()).
	 * 
	 * \remarks
	 * An EasyPlayer handle can not use a subaudio interface and center channel replacement (see 
	 * CriMvEasyPlayer::ReplaceCenterVoice()) at the same time.
	 * 
	 * \sa CriMvEasyPlayer::DetachSubAudioInterface(), CriMvEasyPlayer::SetSubAudioTrack(),
	 *     CriMvEasyPlayer::ReplaceCenterVoice() 
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�T�u�I�[�f�B�I�p�T�E���h�C���^�t�F�[�X�̐ݒ�
	 *  \param		sound		�T�u�I�[�f�B�I�p�T�E���h�C���^�t�F�[�X
	 *  \param		err			�G���[���i�ȗ��j
	 * 
	 * �T�u�I�[�f�B�I�i���C���I�[�f�B�I�Ɠ����ɕʂ̃I�[�f�B�I���Đ�����@�\�j�̂��߂�
	 * �T�E���h�C���^�t�F�[�X��ݒ肵�܂��B<BR>
	 * �ݒ肷��T�E���h�C���^�t�F�[�X�́A CriMvEasyPlayer::Create() ���Ɏw�肵���T�E���h�C���^�t�F�[�X
	 * �Ƃ́u�ʂ́v�C���X�^���X�łȂ���΂����܂���B<BR>
	 * 
	 * �{�֐��́AEasyPlayer�n���h���쐬��A CriMvEasyPlayer::Start() �܂��� CriMvEasyPlayer::Prepare() ��
	 * �Ăяo�����O�Ɏ��s���Ȃ���΂����܂���B<BR>
	 * 
	 * �T�u�I�[�f�B�I���Đ�����ɂ́A�{�֐��ŃT�E���h�C���^�t�F�[�X��ݒ肵�����ƁA
	 * CriMvEasyPlayer::SetSubAudioTrack() �ŃT�u�I�[�f�B�I�̃g���b�N���w�肵�Ă��������B<BR>
	 * 
	 * �T�u�I�[�f�B�I�p�T�E���h�C���^�t�F�[�X��ݒ肵���n���h���j����j������O�ɁA
	 * MVEASY_STATUS_STOP �܂��� MVEASY_STATUS_PLAYEND �̏�Ԃ� CriMvEasyPlayer::DetachSubAudioInterface() ���Ă�ł��������B
	 * �Ȃ��A�T�u�I�[�f�B�I�p�T�E���h�C���^�t�F�[�X�� CriMvEasyPlayer::ResetAllParameters() ���Ăяo���Ă����Z�b�g����܂���B
	 * 
	 * ����: <BR>
	 * �T�u�I�[�f�B�I�@�\�́A CriMvEasyPlayer::ReplaceCenterVoice() �ɂ��Z���^�[�`���l���u�������@�\�Ƃ�
	 * �����Ɏg�p�ł��܂���B<BR>
	 * 
	 * \sa CriMvEasyPlayer::DetachSubAudioInterface(), CriMvEasyPlayer::SetSubAudioTrack(),
	 *     CriMvEasyPlayer::ReplaceCenterVoice()
	 */
	void AttachSubAudioInterface(CriMvSoundInterface *sound, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Removes a secondary audio interface
	 *
	 * \param       err         Optional error code
	 *
	 * Removes the secondary sound interface that was set by a call to CriMvEasyPlayer::AttachSubAudioInterface().
	 * 
	 * This function should be called when the EasyPlayer handle's state is either MVEASY_STATUS_STOP or 
	 * MVEASY_STATUS_PLAYEND.
	 * 
	 * \sa CriMvEasyPlayer::AttachSubAudioInterface()
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�T�u�I�[�f�B�I�p�T�E���h�C���^�t�F�[�X�̉���
	 *  \param		err			�G���[���i�ȗ��j
	 * 
	 * ���ݐݒ肳��Ă���T�u�I�[�f�B�I�p�T�E���h�C���^�t�F�[�X���������܂��B<BR>
	 * 
	 * �{�֐��́AEasyPlayer�n���h���̏�Ԃ� CriMvEasyPlayer::MVEASY_STATUS_STOP �܂���
	 * CriMvEasyPlayer::MVEASY_STATUS_PLAYEND �̎��ɌĂяo���Ă��������B<BR>
	 * 
	 * \sa CriMvEasyPlayer::AttachSubAudioInterface()
	 */
	void DetachSubAudioInterface(CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Sets the secondary audio track
	 *
	 * \param       track           Track number
	 * \param       err             Optional error code
	 *
	 * A secondary, or subaudio, track is typically used to play a dialog track or sound effects along 
	 * with a movie.  An application can support several different languages by setting the desired language 
	 * track with this function.  By default, the subaudio track is disabled, even if the interface has been set.
	 *
	 * Subaudio is enabled with CriMvEasyPlayer::AttachSubAudioInterface().  If the interface has not been set,
	 * this function will have no effect.
	 *
	 * The main audio track for the movie is set with CriMvEasyPlayer::SetAudioTrack().  If the same track 
	 * number is used for the main and subaudio, the subaudio will not play.
	 *
	 * To turn off the subaudio track, pass CRIMV_CENTER_VOICE_OFF as the track number.
	 *
	 * \remarks
	 * If this function is called, it must be called before starting playback (with either CriMvEasyPlayer::Prepare() 
	 * or CriMvEasyPlayer::Start()).
	 * 
	 * \remarks
	 * An EasyPlayer handle can not use a subaudio interface and center channel replacement (see 
	 * CriMvEasyPlayer::ReplaceCenterVoice()) at the same time.
	 * 
	 * \sa CriMvEasyPlayer::AttachSubAudioInterface(), CriMvEasyPlayer::ReplaceCenterVoice() 
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�T�u�I�[�f�B�I�g���b�N�̐ݒ�
	 *  \param		track		�T�u�I�[�f�B�I�Đ�����g���b�N�ԍ�
	 *  \param		err			�G���[���i�ȗ��j
	 * 
	 * �T�u�I�[�f�B�I�g���b�N��ݒ肵�܂��B�f�t�H���g�l�� CRIMV_CENTER_VOICE_OFF �ł��B<BR>
	 * 
	 * �T�u�I�[�f�B�I���Đ�����ɂ́A CriMvEasyPlayer::AttachSubAudioInterface() �ŃT�E���h�C���^�t�F�[�X��ݒ肵�����ƁA
	 * �{�֐��ŃT�u�I�[�f�B�I�̃g���b�N���w�肵�Ă��������B<BR>
	 * �{�֐��̌Ăяo���́A CriMvEasyPlayer::Start() �܂��� CriMvEasyPlayer::Prepare() �̌Ăяo�����O�łȂ���΂����܂���B<BR>
	 * 
	 * ���C���I�[�f�B�I�̃g���b�N�� CriMvEasyPlayer::SetAudioTrack() �Ŏw�肵�܂��B
	 * �T�u�I�[�f�B�I�g���b�N�Ƃ��ă��C���I�[�f�B�I�Ɠ����g���b�N���w�肵���ꍇ�́A�T�u�I�[�f�B�I����͉����Đ�����܂���B<BR>
	 * 
	 * �T�u�I�[�f�B�I�g���b�N�ɂ́A�Z���^�[�`���l���u�������@�\�Ƃ͈قȂ�`���l�����̐����͂���܂���B
	 * ���m�����A�X�e���I�A5.1ch �̂�����̃g���b�N���T�u�I�[�f�B�I�Ƃ��Ďg�p���邱�Ƃ��ł��܂��B<BR>
	 * 
	 * ����: <BR>
	 * �T�u�I�[�f�B�I�@�\�́A CriMvEasyPlayer::ReplaceCenterVoice() �ɂ��Z���^�[�`���l���u�������@�\�Ƃ�
	 * �����Ɏg�p�ł��܂���B<BR>
	 * 
	 * \sa CriMvEasyPlayer::AttachSubAudioInterface()
	 */
	// default value is -1.
	void SetSubAudioTrack(CriSint32 track, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Retrieves the secondary audio track
	 *
	 * \param       err             Optional error code
	 * \return      track number    Current subaudio track number
	 *
     * When you successfully set the subaudio track, this function return the track number
	 * that you specified by CriMvEasyPlayer::SetSubAudioTrack(). Otherwise, it retuns CRIMV_CENTER_VOICE_OFF.
	 *
	 * \sa CriMvEasyPlayer::SetSubAudioTrack()
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�T�u�I�[�f�B�I�g���b�N�̎擾
	 * \param		err			�G���[���i�ȗ��j
	 * \return      �g���b�N�ԍ�   ���ݐݒ肳��Ă���g���b�N�ԍ�
	 * 
	 * �T�u�I�[�f�B�I�Đ����L���ɂȂ��Ă���΁A���[�U��CriMvEasyPlayer::SetSubAudioTrack()�Őݒ肵��
	 * �T�u�I�[�f�B�I�g���b�N�ԍ���Ԃ��܂��B
	 *
	 * �T�u�I�[�f�B�I�Đ����L���łȂ��ꍇ��A�T�u�I�[�f�B�I�g���b�N���w�肵�Ă��Ȃ������ꍇ�́A
	 * CRIMV_CENTER_VOICE_OFF��Ԃ��܂��B
	 * 
	 * \sa CriMvEasyPlayer::SetSubAudioTrack()
	 */
	CriSint32 GetSubAudioTrack(CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Replaces the center channel of a 5.1ch audio track
	 *
	 * \param       track           Track number to use
	 * \param       err             Optional error code
	 *
	 * If the current audio track for a movie (set with CriMvEasyPlayer::SetAudioTrack()) is in 5.1ch
	 * surround sound, the center channel can be replaced with a different, mono, track.  This does not 
	 * affect any of the other channels in the 5.1ch track.
	 *
	 * If the current audio track is not 5.1ch, or the replacement track is not monaural, this call will
	 * have no effect.
	 *
	 * Passing CRIMV_CENTER_VOICE_OFF as the value of \a track will undo the replacement and revert to
	 * playing the original center channel of th 5.1ch track.
	 *
	 * \remarks
	 * An EasyPlayer handle can not use center channel replacement and a subaudio interface (see 
	 * CriMvEasyPlayer::AttachSubAudioInterface() and CriMvEasyPlayer::SetSubAudioTrack()) at the same time.
	 *
	 * \sa CriMvEasyPlayer::SetAudioTrack(), CriMvEasyPlayer::AttachSubAudioInterface(),
	 *     CriMvEasyPlayer::SetSubAudioTrack()
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�Z���^�[�{�C�X�̐ݒ�
	 *  \param		track	�{�C�X�g���b�N�ԍ�
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * 5.1ch �I�[�f�B�I�Đ����ɁA�Z���^�[�`���l��������ʂ̃��m�����g���b�N�ƒu�������邱�Ƃ��ł��܂��B<br>
	 * �{�֐��́A�u�������p�̃��m�����f�[�^���������I�[�f�B�I�g���b�N��ݒ肵�܂��B<br>
	 * 5.1ch BGM �ɑ΂��āA�{�C�X�����𕡐���ނ��獷���ւ������ꍇ�Ɏg�p���Ă��������B
	 * 
	 * �f�t�H���g�̓Z���^�[�{�C�X�w�薳���ł��B
	 * 
	 * ���̊֐����g�p�����ꍇ�A���C���̃I�[�f�B�I�g���b�N�Ƃ��čĐ����Ă���5.1ch�f�[�^�̃Z���^�[�`���l��
	 * �͔j������A����ɃZ���^�[�{�C�X�Ƃ��Ďw�肵���f�[�^������܂��B
	 * 
	 * (a) �Z���^�[�{�C�X�Ƃ��Ďg�p�ł���̂̓��m�����̃I�[�f�B�I�����ł��B<br>
	 * (b) �Z���^�[�u���������L���Ȃ̂̓��C���̃I�[�f�B�I��5.1ch�̏ꍇ�����ł��B
	 * 
	 * ���̓�̏����𖞂����Ă��Ȃ��ꍇ�́A�{�֐��Őݒ肵���l�͖�������܂��B
	 * 
	 * �f�t�H���g�l�ɖ߂������ꍇ�́A�`���l���Ƃ���CRIMV_CENTER_VOICE_OFF���w�肵�Ă��������B
	 * 
	 * \sa CriMvEasyPlayer::SetAudioTrack(), CriMvEasyPlayer::AttachSubAudioInterface(),
	 *     CriMvEasyPlayer::SetSubAudioTrack()
	 */
	void ReplaceCenterVoice(CriSint32 track, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Sets a callback function that will set the movie file
	 *
	 * \param       func        Callback function
	 * \param       usrobj      Pointer to user-provided data
	 * \param       err         Optional error code
	 *
	 * Normally, the movie file is provided directly, via CriMvEasyPlayer::SetFile() or 
	 * CriMvEasyPlayer::SetData(), before playback is started.  Setting a file request callback
	 * allows an application to do \a concatenated \a playback, playing multiple movies, one 
	 * after the other.
	 *
	 * If the callback function is set, it will be called when
	 *
	 * - The last data chunk of the current movie has been read by EasyPlayer.
	 * - Any of the playback functions (CriMvEasyPlayer::Start(), CriMvEasyPlayer::Prepare(), 
	 *   or CriMvEasyPlayer::DecodeHeader()) are called and the movie file has not been set.
	 *
	 * The callback should call CriMvEasyPlayer::SetFile() or CriMvEasyPlayer::SetData() if the
	 * application wants to continue playback.  Otherwise, playback will end once the callback returns.
	 *
	 * The \a usrobj parameter specifies a pointer to arbitrary data, that will be passed as the
	 * second parameter to the callback.
	 *
	 * The prototype of the callback function is
	 *
	 *  <b>void callback(CriMvEasyPlayer *mveasy, void *usrobj)</b>
	 *
	 * where
	 *
	 *  <b>mveasy</b>   is the EasyPlayer object
	 *  <b>usrobj</b>   is the pointer to user data that was passed to SetFileRequestCallback().
	 *
	 * \remarks
	 * In order to do concatenated playback, all movies must have the same
	 *
	 * \remarks
	 * - video resolution
	 * - framerate
	 * - video codec
	 * - audio track structure
	 * - subtitle structure
	 *
	 * \remarks
	 * The same audio track structure means that all movies must have the same number of tracks, and the
	 * same track number must be of the same audio type for each movie.  For instance, if the first movie has 
	 * 2 audio tracks, with track 1 being stereo and track 2 being mono, then \a all other movies would have
	 * to have 2 tracks, track 1 stereo and track 2 mono.
	 *
	 * \remarks
	 * Subtitles have to match in the number of channels.  For instance, if the first movie had 3 subtitle
	 * channels, then all other movies would need 3 channels.  It is also important to keep the languages on
	 * the same tracks for each movie, since otherwise the application would get confused.  CRI Movie makes
	 * no assumptions about languages or the interpretation of subtitles; subtitles are simply treated as 
	 * binary data.
	 *
	 * \remarks
	 * Currently, cuepoints are not supported with concatenated playback.
	 *
	 * \sa CriMvEasyPlayer::SetFile(), CriMvEasyPlayer::SetData()
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�t�@�C���v���R�[���o�b�N�֐��̓o�^
	 *  \param		func	�t�@�C���v���R�[���o�b�N�֐�
	 *  \param		usrobj	���[�U�I�u�W�F�N�g
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * ���[�r�̘A���Đ����s�����߂ɁA���[�r�t�@�C����v������R�[���o�b�N�֐���o�^���܂��B
	 * ���̃R�[���o�b�N�֐��͈ȉ��̃^�C�~���O�Ŕ������܂��B
	 * 
	 * �E���[�r�t�@�C����ǂݍ��ݏI���������B<br>
	 * �E�t�@�C���̎w�薳���ōĐ����J�n�������B
	 * 
	 * �t�@�C���v���R�[���o�b�N�֐����� CriMvEasyPlayer::SetFile() �܂��� CriMvEasyPlayer::SetData() 
	 * ���Ăяo�����ƂŁA�A�����Ď��̃��[�r�t�@�C�����w�肷�邱�Ƃ��ł��܂��B<br>
	 * SetFile() �� SetData() ���Ăяo���Ȃ������ꍇ�́A�ǂݍ��ݍς݂̃��[�r���I����
	 * �Đ��I���ɂȂ�܂��B
	 * 
	 * �t�@�C���v���R�[���o�b�N�������A�R�[���o�b�N�֐��̑�����usrobj�ɂ́A�o�^���Ɏw��
	 * �������[�U�I�u�W�F�N�g���n����܂��B�o�^�t�@�C�����X�g�Ȃǂ̊Ǘ��ɗ��p���Ă��������B
	 * 
	 * �A���Đ��ł��郀�[�r�t�@�C���ɂ͈ȉ��̏���������܂��B<BR>
	 * - �r�f�I�𑜓x������
	 * - �r�f�I�̃t���[�����[�g������
	 * - �r�f�I�̃R�[�f�b�N������
	 * - �I�[�f�B�I����ю����̃g���b�N�\��������
	 * 
	 * \sa CriMvEasyPlayer::SetFile(), CriMvEasyPlayer::SetData()
	 */
	void SetFileRequestCallback(void (*func)(CriMvEasyPlayer *mveasy, void *usrobj), 
								void *usrobj, CriError &err=CriMv::ErrorContainer);

#if defined(XPT_TGT_PC)
	/*EN
	 * \ingroup MODULE_OPTION
	 * \brief		Set processor parameters for decoding on PC
	 *
 	 *  \param		num_threads		Number of additional threads for load distribution in decoding (Maximum 3 threads)
	 *  \param		affinity_mask 	Pointer to an array of thread affinity masks for each thread specified with num_threads.
	 *  \param		priority		Thread priority of the decoding threads for load balancing
	 *  \param		err				Optional error code
	 * 
	 * This function sets the processor parameters for decoding.  Use it when you want to change 
	 * processors or thread priority for decoding load distribution.
	 * 
	 * If this function is called, it must be called before calling any of the playback functions 
	 * (CriMvEasyPlayer::Start(), CriMvEasyPlayer::Prepare(), or CriMvEasyPlayer::DecodeHeader()).
	 * 
	 * On initialization, CRI Movie prepares three worker threads for distributed decoding.<br>
	 *
	 * \a num_threads specifies how many worker threads CRI Movie should use.<br>
	 * \a affinity_mask is an array of affinity masks for the worker threads.  This array must have
	 * \a num_threads many elements.  The format of \a affinity_mask is same as for the value passed to the 
	 * Win32 <b>SetThreadAffinityMask()</b> API function.<br>
	 * \a priority is used as the thread priority for all of the threads specified by \a num_threads.
     *
	 * If this function is called, three distributed decoding threads will run in parallel by default.
	 * Also, processor assignment of the decoding threads is handled by the operating system, and their 
	 * priority will be normal.
	 * 
	 * To reset the parameters, call this function again, passing CRIMV_DEFAULT_AFFNITY_MASK_PC and 
	 * CRIMV_DEFAULT_THREAD_PRIORITY_PC as the affinity masks and thread priority.
	 * 
	 * \sa CRIMV_DEFAULT_AFFNITY_MASK_PC, CRIMV_DEFAULT_THREAD_PRIORITY_PC
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		PC�Ńf�R�[�h�����Ɏg���ǉ��v���Z�b�T�ݒ�
	 *  \param		num_threads		���ו��U�f�R�[�h�p�Ɏg�p����ǉ��X���b�h�̐� (�ő�R�j
	 *  \param		affinity_masks	�X���b�h�A�t�B�j�e�B�}�X�N�̔z��ւ̃|�C���^�Bnum_threads�Ŏw�肵���X���b�h���Ƃ̃}�X�N�l�B
	 *  \param		priority		���ו��U�f�R�[�h�X���b�h�̗D��x
	 *  \param		err				�G���[���i�ȗ��j
	 * 
	 * �f�R�[�h�����𕪎U���ď������邽�߂̃v���Z�b�T���w��ł��܂��B
	 * �f�R�[�h�����ɍs���v���Z�b�T��X���b�h�D��x��ύX�������ꍇ�Ɏg�p���Ă��������B
	 * �{�֐��͍Đ��J�n(Start, Prepare, DecodeHader)�O�ɌĂяo���K�v������܂��B
	 * 
	 * CRI Movie�͏������̍ۂɂR�̕��U�f�R�[�h�p�̃��[�J�[�X���b�h��p�ӂ��܂��B
	 * num_threads�����ŁA���̂����̂����̃X���b�h�����ۂɎg�p���邩���w��ł��܂��B
	 * �A�v���P�[�V�������疾���I�Ƀv���Z�b�T���蓖�Ă��s�������ꍇ�A�X�̃X���b�h�ɑ΂���
	 * �A�t�B�j�e�B�}�X�N��ݒ肵�Ă��������B
	 * �A�t�B�j�e�B�}�X�N�̒l�́AWin32 API��SetThreadAffinityMask�̈����Ɠ��������ł��B
     * �X���b�h�D��x�́Anum_threads�Ŏw�肵���f�R�[�h�Ɏg�p����X���b�h�ɑ΂��ēK�p����܂��B
	 * 
	 * ���̊֐����Ă΂Ȃ������ꍇ�A�R�̃X���b�h�ŕ���f�R�[�h���s���܂��B
	 * �f�R�[�h�X���b�h�̃v���Z�b�T�͊��蓖�Ă͑S��OS�C���ŁA�D��x�̓X���b�h�W���ɂȂ�܂��B
	 * 
	 * ��x�{�֐��Őݒ��ύX������A��Ԃ�߂������ꍇ�́ACRIMV_DEFAULT_AFFNITY_MASK_PC, CRIMV_DEFAULT_THREAD_PRIORITY_PC��
	 * �����Ƃ��Ďw�肵�A�ēx�Ăяo���Ă��������B
	 *
	 * \sa CRIMV_DEFAULT_AFFNITY_MASK_PC, CRIMV_DEFAULT_THREAD_PRIORITY_PC
	 */
	void SetUsableProcessors_PC(CriSint32 num_threads, const CriUint32 *affinity_mask, CriSint32 priority,
										CriError &err=CriMv::ErrorContainer);

#endif

#if defined(XPT_TGT_XBOX360)
	/*EN
	 * \ingroup MODULE_OPTION
	 * \brief		Set processor parameters for decoding
	 *  \param		processors_param	Processor Parameters
	 *  \param		err					Optional error code 
	 *
	 * \brief		Set processor parameters for decoding
	 *  \param		processors_param	Processor Parameters
	 *  \param		err					Optional error code
	 * 
	 * This function sets the processor parameters for decoding, along the priority of the
	 * decoding threads.
	 * 
	 * If this function is called, it must be called before calling any of the playback functions 
	 * (CriMvEasyPlayer::Start(), CriMvEasyPlayer::Prepare(), or CriMvEasyPlayer::DecodeHeader()).
	 *
	 * If you don't call this function, the EasyPlayer handle uses Processor 3 (Core 0, Thread 0)
	 * and Processor 5 (Core 0, Thread 0).
	 * 
	 * \sa CriMvProcessorParameters_XBOX360
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		Xbox360�Ńf�R�[�h�Ɏg���v���Z�b�T�ݒ�
	 *  \param		processors_param	�g�p�v���Z�b�T�p�����[�^
	 *  \param		err					�G���[���i�ȗ��j
	 * 
	 * �f�R�[�h�Ɏg�p����v���Z�b�T���w�肵�܂��B<br>
	 * �{�֐��͍Đ��J�n(Start, Prepare, DecodeHader)�O�ɌĂяo���K�v������܂��B
	 * 
	 * �܂��A�f�R�[�h�Ɏg�p��������X���b�h�̗D��x�̐ݒ肪�o���܂��B
	 * 
	 * �f�t�H���g�̃v���Z�b�T�ݒ�ł́A�v���Z�b�T�R(�R�A1�X���b�h1)�ƃv���Z�b�T�T
	 * (�R�A2�X���b�h1)���g�p���܂��B
	 *
	 * \sa CriMvProcessorParameters_XBOX360
	 */
	void SetUsableProcessors_XBOX360(const CriMvProcessorParameters_XBOX360 *processors_param, 
										CriError &err=CriMv::ErrorContainer);
#endif

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Sets the frame where playback will start
	 *
	 * \param       seek_frame_id       Frame (0-based) to start playing from
	 * \param       err                 Optional error code
	 *
	 * To play a movie from other than the first frame, call this function before starting playback.
	 * To seek to a new frame when the movie is already playing, call CriMvEasyPlayer::Stop() and wait
	 * for the handle to change to the MVEASY_STATUS_STOP state, call this function with the desired
	 * frame number, then call CriMvEasyPlayer::Start() again.
	 *
	 * Valid values for seek_frame_id are from 0 to \a num_frames - 1, where \a num_frames can be found by
	 *
	 * <pre>
	 *
	 *   CriMvStreamingParameters streaming_params;
	 *   CriSint32                num_frames;
	 *
	 *   GetMovieInfo(streaming_params);
	 *   num_frames = streaming_params.video_prm[0].total_frames;
	 *
	 * </pre>
	 *
	 * Refer to \ref usr_mech7 for more information.
	 *
	 * \remarks
	 * If the value of \a seek_frame_id is out of range, playback will start from frame 0.
	 *
	 * \sa CriMvStreamingParameters
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�V�[�N�Đ��J�n�ʒu�̐ݒ�
	 *  \param		seek_frame_id		�V�[�N�Đ��J�n����t���[���ԍ��i�O�`�j
	 *  \param		err					�G���[���i�ȗ��j
	 * 
	 * �V�[�N�Đ����J�n����t���[���ԍ����w�肵�܂��B
	 * 
	 * �Đ��J�n�O( CriMvEasyPlayer::Prepare()�܂��� CriMvEasyPlayer::Start()�Ăяo���O)�ɖ{�֐������s���Ă��������B
	 * �܂��A���̊֐��̓��[�r�̍Đ����ɌĂяo�����Ƃ͏o���܂���B�Đ����ɃV�[�N������ꍇ�́A��x�Đ����~���Ă���
	 * �{�֐����Ăяo���Ă��������B
	 *
	 * �{�֐������s���Ȃ������ꍇ�A�܂��̓t���[���ԍ��O���w�肵���ꍇ�̓��[�r�̐擪����Đ����J�n���܂��B
	 * �w�肵���t���[���ԍ����A���[�r�f�[�^�̑��t���[�������傫�������蕉�̒l�������ꍇ�����[�r�̐擪����Đ����܂��B
	 * 
	 * \ref usr_mech7 �����킹�ĎQ�Ƃ��Ă��������B
	 *
	 */
	void SetSeekPosition(CriSint32 seek_frame_id, CriError &err=CriMv::ErrorContainer);

	/*EN
	 *
	 * \brief       Calculates a  frame ID from a frame time
	 *
	 * \param       count           Timer counter
	 * \param       unit            Counter increment per second
	 * \param       err             Optional error code
	 *
	 * \return      Frame ID corresponding to given time
	 *
	 * Each frame of a movie corresponds to a particular display time, based on the framerate.  Given a
	 * time from the start of playback, this function will return the ID of the specific frame that should
	 * be displayed, barring any delays or skipped frames, at that time.
	 *
	 * The time, in seconds, is specified by \a count / \a unit.<br>
	 *
	 * This function can be used for, among other things, jumping to a particular frame when a cuepoint is reached.
	 *
	 * \remarks
	 * This function can be called once the EasyPlayer handle status has reached MVEASY_STATUS_WAIT_PREP.
	 *
	 * \sa CriMvEasyPlayer::CalcTimeFromFrameId();
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�Đ���������t���[���ԍ��̌v�Z
	 *  \param		count		�^�C�}�J�E���g
	 *  \param		unit		�P�b������̃^�C�}�J�E���g�l�Bcount �� unit �ŕb�P�ʂ̎����ƂȂ�܂��B
	 *  \param		err			�G���[���i�ȗ��j
	 *  \return		frame ID
	 * 
	 * �Đ���������t���[���ԍ����v�Z���܂��B
	 * ���̊֐��́AEasyPlayer�n���h���̏�Ԃ� MVEASY_STATUS_WAIT_PREP �ȍ~�ɂȂ������ƂɎg�p�ł��܂��B
	 * 
	 * �V�[�N�Đ��J�n�ʒu���A��������v�Z�������Ƃ��Ɏg�p���Ă��������B
	 * �i�Ⴆ�΃L���[�|�C���g��񂩂�V�[�N�ʒu�����肷��ꍇ�ȂǁB�j
	 * 
	 * \sa CriMvEasyPlayer::CalcTimeFromFrameId();
	 */
	CriSint32 CalcFrameIdFromTime(CriUint64 count, CriUint64 unit, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Calculates a frame time from a frame ID
	 *
	 * \param       frame_id        Frame ID
	 * \param       unit            Counter increment per second
	 * \param       err             Optional error code
	 *
	 * \return      Timer counter corresponding to given frame ID
	 *
	 * Each frame of a movie corresponds to a particular display time, based on the framerate.  Given a
	 * frame ID and the number of timer intervals per second, this function will return the timer count
	 * of the display time for that frame.
	 *
	 * The display time, in seconds, for this frame is calculated by dividing the timer count by the timer
	 * interval.
	 * 
	 * If you have the movie frame, you do not have to calculate the time.  The \a time and \a tunit fields
	 * of the CriMvFrameInfo structure that is passed the GetFrameOnTimeXXX() and DiscardNextFrame()
	 * functions will contain this information.
	 *
	 * \remarks
	 * This function can be called once the EasyPlayer handle status has reached MVEASY_STATUS_WAIT_PREP.
	 *
	 * \sa CriMvEasyPlayer::CalcFrameIdFromTime(), CriMvFrameInfo
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�t���[���ԍ�����Đ������̌v�Z
	 *  \param		frame_id	frame ID
	 *  \param		unit		�P�b������̃^�C�}�J�E���g�l�Bcount �� unit �ŕb�P�ʂ̎����ƂȂ�܂��B
	 *  \param		err			�G���[���i�ȗ��j
	 *  \return		�^�C�}�J�E���g
	 * 
	 * �t���[���ԍ�����Đ��������v�Z���܂��B
	 * ���̊֐��́AEasyPlayer�n���h���̏�Ԃ� MVEASY_STATUS_WAIT_PREP �ȍ~�ɂȂ������ƂɎg�p�ł��܂��B
	 * 
	 * ���ۂɃt���[���擾�����ꍇ�́A�v�Z�̕K�v�͂���܂���B�t���[�����\���̂̎������Q�Ƃ��Ă��������B
	 * 
	 * \sa CriMvEasyPlayer::CalcFrameIdFromTime(), CriMvFrameInfo
	 */
	CriUint64 CalcTimeFromFrameId(CriSint32 frame_id, CriUint64 unit, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Find the next event point, if any, after the given time counter
	 *
	 * \param       count           Timer counter
	 * \param       unit            Counter increment per second
	 * \param       type            Type of event point to look for
	 * \param       eventinfo       Returned event point information structure
	 * \param       err             Optional error code
	 *
	 * \return      Frame ID corresponding to given time
	 * 
	 * Event points allow an application to associate arbitrary actions with specific points in a movie.
	 * This function will search forward in the movie for the next event point after the given time
	 * (specifed as \a count / \a unit).  If an event point is found, \a eventinfo will be populated with
	 * the information about the event point, and the function will return the corresponding frame ID.
	 *
	 * \a type is an application-defined value that can be used to categorize event points, and is specified 
	 * when the movie is encoded (see <b>link to event-point-specification-section</b> for more information
	 * about creating event points.).  If -1 is passed as the value of \a type, all event points will be
	 * searched.  Otherwise, only matching event point types will be searched.
	 *
	 * If no event point of the requested type is found, this function will return -1.
	 *
	 * \remarks
	 * This function can be called once the EasyPlayer handle status has reached MVEASY_STATUS_WAIT_PREP.
	 *
	 * \sa  CriMvEasyPlayer::SearchPrevEventPointByTime();
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�w�莞������̃C�x���g�|�C���g�̌���
	 *  \param		count		�^�C�}�J�E���g
	 *  \param		unit		�P�b������̃^�C�}�J�E���g�l�Bcount �� unit �ŕb�P�ʂ̎����ƂȂ�܂��B
	 *  \param		type		�����ΏۂƂ���C�x���g�|�C���g��type�l
	 *  \param		eventinfo	���������C�x���g�|�C���g�̏��
	 *  \param		err			�G���[���i�ȗ��j
	 *  \return		frame ID
	 * 
	 * �w�莞���̎��ɂ���C�x���g�|�C���g���������A�C�x���g�|�C���g���ƃt���[���ԍ����擾���܂��B
	 * ���̊֐��́AEasyPlayer�n���h���̏�Ԃ� MVEASY_STATUS_WAIT_PREP �ȍ~�ɂȂ������ƂɎg�p�ł��܂��B
	 * 
	 * �����̑ΏۂƂȂ�̂� type �Ŏw�肵���l����v����C�x���g�|�C���g�ł��B
	 * type �� -1���w�肵���ꍇ�́A�S�ẴC�x���g�|�C���g�������ΏۂƂȂ�܂��B
	 * 
	 * �����ΏۂƂȂ�C�x���g�|�C���g�������ł��Ȃ������ꍇ�́A�t���[���ԍ���-1��Ԃ��܂��B
	 * 
	 * \sa  CriMvEasyPlayer::SearchPrevEventPointByTime();
	 */
	CriSint32 SearchNextEventPointByTime(CriUint64 count, CriUint64 unit, CriSint32 type, 
											CriMvEventPoint &eventinfo, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Find the previous event point, if any, before the given time counter
	 *
	 * \param       count           Timer counter
	 * \param       unit            Counter increment per second
	 * \param       type            Type of event point to look for
	 * \param       eventinfo       Returned event point information structure
	 * \param       err             Optional error code
	 *
	 * \return      Frame ID corresponding to given time
	 * 
	 * Event points allow an application to associate arbitrary actions with specific points in a movie.
	 * This function will search backward in the movie (i.e. towards the beginning) for the next event 
	 * point before the given time (specifed as \a count / \a unit).  If an event point is found, 
	 * \a eventinfo will be populated with the information about the event point, and the function will 
	 * return the corresponding frame ID.
	 *
	 * \a type is an application-defined value that can be used to categorize event points, and is specified 
	 * when the movie is encoded (see <b>link to event-point-specification-section</b> for more information
	 * about creating event points.).  If -1 is passed as the value of \a type, all event points will be
	 * searched.  Otherwise, only matching event point types will be searched.
	 *
	 * If no event point of the requested type is found, this function will return -1.
	 *
	 * \remarks
	 * This function can be called once the EasyPlayer handle status has reached MVEASY_STATUS_WAIT_PREP.
	 *
	 * \sa  CriMvEasyPlayer::SearchNextEventPointByTime();
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�w�莞�����O�̃C�x���g�|�C���g�̌���
	 *  \param		count		�^�C�}�J�E���g
	 *  \param		unit		�P�b������̃^�C�}�J�E���g�l�Bcount �� unit �ŕb�P�ʂ̎����ƂȂ�܂��B
	 *  \param		type		�����ΏۂƂ���C�x���g�|�C���g��type�l
	 *  \param		eventinfo	���������C�x���g�|�C���g�̏��
	 *  \param		err			�G���[���i�ȗ��j
	 *  \return		frame ID
	 * 
	 * �w�莞���̎�O�ɂ���C�x���g�|�C���g���������A�C�x���g�|�C���g���ƃt���[���ԍ����擾���܂��B
	 * ���̊֐��́AEasyPlayer�n���h���̏�Ԃ� MVEASY_STATUS_WAIT_PREP �ȍ~�ɂȂ������ƂɎg�p�ł��܂��B
	 * 
	 * �����̑ΏۂƂȂ�̂� type �Ŏw�肵���l����v����C�x���g�|�C���g�ł��B
	 * type �� -1���w�肵���ꍇ�́A�S�ẴC�x���g�|�C���g�������ΏۂƂȂ�܂��B
	 * 
	 * �����ΏۂƂȂ�C�x���g�|�C���g�������ł��Ȃ������ꍇ�́A�t���[���ԍ���-1��Ԃ��܂��B
	 *
	 * \sa  CriMvEasyPlayer::SearchNextEventPointByTime();
	 */
	CriSint32 SearchPrevEventPointByTime(CriUint64 count, CriUint64 unit, CriSint32 type, 
											CriMvEventPoint &eventinfo, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Gets the list of all event points in the movie
	 *
	 * \param       err         Optional error code
	 *
	 * \return      Pointer to the list of event points, or NULL if there are no event points
	 *
	 * CRI Movie considers a cue point list to be the number of event points in a movie, and a pointer to
	 * an array of event point structures.  Event points allow an application to associate arbitrary actions 
	 * with specific points in a movie.
	 *
	 * The returned pointer points to an area inside of the EasyPlayer's work buffer.  An application should
	 * not attempt to write to it.
	 * 
	 * \remarks
	 * This function can be called once the EasyPlayer handle status has reached MVEASY_STATUS_WAIT_PREP.
	 * The returned information is valid through the MVEASY_STATUS_STOP state.  Once the EasyPlayer handle 
	 * has been destroyed, or the movie has been restarted (by calling CriMvEasyPlayer::Start(),
	 * CriMvEasyPlayer::Prepare(), or CriMvEasyPlayer::DecodeHeader()), the cuepoint information will be
	 * invalid.
	 * 
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�L���[�|�C���g���i�C�x���g�|�C���g�ꗗ�j�̎擾
	 *  \param		err			�G���[���i�ȗ��j
	 *  \return		Cue point info (Event point list)
	 * 
	 * �L���[�|�C���g���i�C�x���g�|�C���g�ꗗ�j���擾���܂��B
	 * ���̊֐��́AEasyPlayer�n���h���̏�Ԃ� MVEASY_STATUS_WAIT_PREP �ȍ~�ɂȂ������ƂɎg�p�ł��܂��B
	 * 
	 * ���̊֐��Ŏ擾����L���[�|�C���g���́A�Đ��n���h���̃��[�N�o�b�t�@�𒼐ڎQ�Ƃ��Ă��܂��B<BR>
	 * �Đ���~��Ԃł̎Q�Ƃ͉\�ł����A���̍Đ����J�n������͎Q�Ƃ��֎~���܂��B<BR>
	 * ���̃L���[�|�C���g����ʂ̃������ɃR�s�[�����ꍇ�����̏����͕ς��܂���B
	 * 
	 */
	CriMvCuePointInfo* GetCuePointInfo(CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Sets a function that will be called when a cue point is reached
	 *
	 * \param       func        Callback function
	 * \param       usrobj      Pointer to user-provided data
	 * \param       err         Optional error code
	 *
	 * Beginning with version 2.0, CRI Movie allows a movie to specify arbitrary actions to
	 * happen at various points on the timeline.  These are referred to as \a cue \a points, or, 
	 * more generally, as \a event \a points.  If a cue point callback has been installed for the
	 * movie, it will be called whenever a cuepoint has been reached.
	 *
	 * The \a usrobj parameter specifies a pointer to arbitrary data, that will be passed as the
	 * third parameter to the callback.
	 *
	 * The prototype of the callback function is
	 *
	 *  <b>void callback(CriMvEasyPlayer *mveasy, CriMvEventPoint *eventinfo, void *usrobj)</b>
	 *
	 * where
	 *
	 *  <b>mveasy</b>       is the EasyPlayer object<br>
	 *  <b>eventinfo</b>    is the event info structure that was reached<br>
	 *  <b>usrobj</b>       is the pointer to user data that was passed to SetFileRequestCallback().<br>
	 *
	 * \remarks
	 * Do not call any movie playback functions (for example, CriMvEasyPlayer::Stop()) from the callback
	 * function.  If you need to do this, set a flag from the callback and refer to it in your main loop.
	 *
	 * \sa  CriMvEventPoint
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�L���[�|�C���g�R�[���o�b�N�֐��̓o�^
	 *  \param		func	�L���[�|�C���g�R�[���o�b�N�֐�
	 *  \param		usrobj	���[�U�I�u�W�F�N�g
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * �L���[�|�C���g�̃R�[���o�b�N�֐���o�^���܂��B
	 * ���̃R�[���o�b�N�֐��́A���[�r�̍Đ��������e�C�x���g�|�C���g�Ŏw�肳�ꂽ�������o�߂������ɔ������܂��B
	 * �R�[���o�b�N�֐��̌Ăяo������� CriMvEasyPlayer::Update() ����s���܂��B
	 * 
	 * �L���[�|�C���g�R�[���o�b�N�������A�R�[���o�b�N�֐��̑�Q���� eventinfo �ɂ̓G�x���g�|�C���g��񂪁A
	 * ��R����usrobj�ɂ́A�o�^���Ɏw�肵�����[�U�I�u�W�F�N�g���n����܂��B
	 * 
	 * �L���[�|�C���g�R�[���o�b�N�֐����ł́A���[�r�Đ����R���g���[������֐��i�Ⴆ�� CriMvEasyPlayer::Stop()�j
	 * ���Ăяo���Ă͂����܂���B
	 * 
	 * \sa  CriMvEventPoint
	 */
	void SetCuePointCallback(void (*func)(CriMvEasyPlayer *mveasy, CriMvEventPoint *eventinfo, void *usrobj), 
								void *usrobj, CriError &err=CriMv::ErrorContainer);


	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Resets movie handle parameters to their default values
	 *
	 * \param       err         Optional error code
	 *
	 * This function will reset most parameters back to their default values.  Call this when you are 
	 * reusing an EasyPlayer handle and need to quickly undo changes to a number of parameters.
	 *
	 * Note that this will not remove a subaudio interface (set by CriMvEasyPlayer::AttachSubAudioInterface()).
	 * CriMvEasyPlayer::DetachSubAudioInterface() must be called instead.
	 *
	 * \remarks
	 * This function can be called once the EasyPlayer handle status is either MVEASY_STATUS_STOP or
	 * MVEASY_STATUS_PLAYEND.
	 *
	 * <table>
	 * <TR><TH> Setting API <TH> Reset by ResetAllParameters() <TH> Reset by Stop()
	 * <TR><TD> SetFile                     <TD> YES <TD> (*1)
	 * <TR><TD> SetData                     <TD> YES <TD> (*1)
	 * <TR><TD> Pause                       <TD> YES <TD> YES
	 * <TR><TD> SetMasterTimer              <TD> YES <TD> NO
	 * <TR><TD> SetNumberOfFramePools       <TD> YES <TD> NO
	 * <TR><TD> SetBufferingTime            <TD> YES <TD> NO
	 * <TR><TD> SetReloadThresholdTime      <TD> YES <TD> NO
	 * <TR><TD> SetMaxBitrate               <TD> YES <TD> NO
	 * <TR><TD> SetAudioTrack               <TD> YES <TD> NO
	 * <TR><TD> SetLoopFlag                 <TD> YES <TD> NO
	 * <TR><TD> SetStreamingParameters      <TD> YES <TD> NO
	 * <TR><TD> AttachSubAudioInterface     <TD> NO  <TD> NO
	 * <TR><TD> SetSubAudioTrack            <TD> YES <TD> NO
	 * <TR><TD> ReplaceCenterVoice          <TD> YES <TD> NO
	 * <TR><TD> SetFileRequestCallback      <TD> YES <TD> NO
	 * <TR><TD> SetSeekPosition             <TD> YES <TD> NO
	 * <TR><TD> SetCuePointCallback         <TD> YES <TD> NO
	 * </table>
	 *
	 * (*1) Normally, values set by SetFile() or SetData() are not reset by a call to Stop().  However,
	 * if a file request callback has been set (via CriMvEasyPlayer::SetFileRequestCallback()), this
	 * setting will be reset.
	 * 
	 * \sa CriMvEasyPlayer::Stop(), CriMvEasyPlayer::SetFileRequestCallback()
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�p�����[�^�̃��Z�b�g
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * �Đ��n���h���ɐݒ肳�ꂽ�p�����[�^�ނ����Z�b�g���܂��B<BR>
	 * �������T�u�I�[�f�B�I�p�C���^�t�F�[�X�����̓��Z�b�g����܂���̂ŁA�A�v���P�[�V�����Ŗ����I��
	 * CriMvEasyPlayer::DetachSubAudioInterface() ���Ăяo���Ă��������B
	 * 
	 * �{�֐��̓n���h����Ԃ� MVEASY_STATUS_STOP �܂��� MVEASY_STATUS_PLAYEND �̎��ɌĂяo���Ă��������B
	 * 
	 * <table>
	 * <TR><TH> �ݒ�֐� <TH> ResetAllParameters�ɂ��<BR>���Z�b�g���� <TH> Stop�ɂ��<BR>���Z�b�g����
	 * <TR><TD> SetFile                     <TD> o <TD> (*1)
	 * <TR><TD> SetData                     <TD> o <TD> (*1)
	 * <TR><TD> Pause                       <TD> o <TD> o
	 * <TR><TD> SetMasterTimer              <TD> o <TD> x
	 * <TR><TD> SetNumberOfFramePools       <TD> o <TD> x
	 * <TR><TD> SetBufferingTime            <TD> o <TD> x
	 * <TR><TD> SetReloadThresholdTime      <TD> o <TD> x
	 * <TR><TD> SetMaxBitrate               <TD> o <TD> x
	 * <TR><TD> SetAudioTrack               <TD> o <TD> x
	 * <TR><TD> SetLoopFlag                 <TD> o <TD> x
	 * <TR><TD> SetStreamingParameters      <TD> o <TD> x
	 * <TR><TD> AttachSubAudioInterface     <TD> x <TD> x
	 * <TR><TD> SetSubAudioTrack            <TD> o <TD> x
	 * <TR><TD> ReplaceCenterVoice          <TD> o <TD> x
	 * <TR><TD> SetFileRequestCallback      <TD> o <TD> x
	 * <TR><TD> SetSeekPosition             <TD> o <TD> x
	 * <TR><TD> SetCuePointCallback         <TD> o <TD> x
	 * </table>
	 * (*1) �ʏ�̓��Z�b�g����܂���B�������t�@�C���v���R�[���o�b�N���o�^����Ă����ꍇ�̓��Z�b�g����܂��B
	 * 
	 * \sa CriMvEasyPlayer::Stop()
	 */
	void ResetAllParameters(CriError &err=CriMv::ErrorContainer);

	/* �Đ��p���[�N�o�b�t�@����щ��ʃ��W���[���̉���i�����I�ȌĂяo���p�j */
	void ReleasePlaybackWork(CriError &err=CriMv::ErrorContainer);

public:	/* for DEBUG */
	/*//EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief		Sets the maximum movie data read size
	 *
	 * \param		max_chunk_size		The maximum read size, in bytes
	 * \param		err					Optional error code
	 * 
	 * This is a DEBUG function and should not normally need to be used by applications.
	 *
	 * Sets the maximum read size.  CRI Movie will attempt to read this many bytes each time it
	 * needs to load more movie data from disk.
	 * 
	 * If this function is called, it must be called before calling any of the playback functions 
	 * (CriMvEasyPlayer::Start(), CriMvEasyPlayer::Prepare(), or CriMvEasyPlayer::DecodeHeader()).
	 *
	 * To revert to the default behavior, pass 0 as \a max_chunk_size.
	 *
	 * \remarks
	 * If this function is called, the new value for \a max_chunk_size will not be reflected in the
	 * CriMvStreapingParameters structure.  A call to CriMvEasyPlayer::GetMovieInfo() will return the
	 * original value that was set in the movie header.
	 *
	 * \remarks
	 * If you call CriMvEasyPlayer::SetStreamingParameters(), this function can not be used.
	 *
	 * \sa CriMvEasyPlayer::GetMovieInfo(), CriMvEasyPlayer::SetMinBufferSize()
	 */
	/*//JP
	 * \ingroup MODULE_OPTION
	 * \brief		�ő�`�����N�T�C�Y�̎w��
	 *  \param		max_chunk_size		�ő�`�����N�T�C�Y[byte]
	 *  \param		err					�G���[���i�ȗ��j
	 * 
	 * ���[�r�f�[�^�̍ő�`�����N�T�C�Y���w�肵�܂��B<br>
	 * ���݂̃��C�u�����ł́A�{�֐��̓A�v���P�[�V��������g�p����K�v�͂���܂���B<br>
	 * 
	 * �{�֐��Őݒ肵���ő�`�����N�T�C�Y�́ACriMvEasyPlayer::GetMovieInfo �֐��Ŏ擾���郀�[�r���ɂ͔��f����܂���B
	 * CriMvEasyPlayer::GetMovieInfo �֐��Ŏ擾�ł���̂̓��[�r�f�[�^�̖{���̏��ł��B<br>
	 * 
	 * �{�֐��̌Ăяo���́A CriMvEasyPlayer::Prepare �֐��܂��� CriMvEasyPlayer::Start �֐��̑O�܂łɎ��s���Ă��������B
	 * 
	 * �ő�`�����N�T�C�Y�� 0���w�肵���ꍇ�A�ő�`�����N�T�C�Y�̓��[�r�f�[�^�̎��l�ƂȂ�܂��B<br>
	 * �܂��A�A�v���P�[�V������ CriMvEasyPlayer::SetStreamingParameters �֐����Ăяo�����ꍇ�͖{�֐���
	 * �ݒ肵���l�����A CriMvEasyPlayer::SetStreamingParameters �֐��̎w�肪�D�悳��܂��B
	 * 
	 * \sa CriMvEasyPlayer::GetMovieInfo(), CriMvEasyPlayer::SetMinBufferSize()
	 */
	void SetMaxChunkSize(CriUint32 max_chunk_size, CriError &err=CriMv::ErrorContainer);

	/*//EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief		Sets the minimum movie data buffer size
	 *
	 * \param		min_buffer_size		The minimum buffer size, in bytes
	 * \param		err					Optional error code
	 * 
	 * This is a DEBUG function and should not normally need to be used by applications.
	 *
	 * Sets the minimum buffer size used for reading movie data from disk.
	 * 
	 * If this function is called, it must be called before calling any of the playback functions 
	 * (CriMvEasyPlayer::Start(), CriMvEasyPlayer::Prepare(), or CriMvEasyPlayer::DecodeHeader()).
	 *
	 * To revert to the default behavior, pass 0 as \a max_chunk_size.
	 *
	 * \remarks
	 * If this function is called, the new value for \a min_buffer_size will not be reflected in the
	 * CriMvStreapingParameters structure.  A call to CriMvEasyPlayer::GetMovieInfo() will return the
	 * original value that was set in the movie header.
	 *
	 * \remarks
	 * If you call CriMvEasyPlayer::SetStreamingParameters(), this function can not be used.
	 * 
	 * \sa CriMvEasyPlayer::GetMovieInfo(), CriMvEasyPlayer::SetMaxChunkSize()
	 */
	/*//JP
	 * \ingroup MODULE_OPTION
	 * \brief		�ŏ��o�b�t�@�T�C�Y�̎w��
	 *  \param		min_buffer_size		�ŏ��o�b�t�@�T�C�Y[byte]
	 *  \param		err					�G���[���i�ȗ��j
	 * 
	 * ���[�r�f�[�^�̍ŏ��o�b�t�@�T�C�Y���w�肵�܂��B<br>
	 * ���݂̃��C�u�����ł́A�{�֐��̓A�v���P�[�V��������g�p����K�v�͂���܂���B<br>
	 * 
	 * �{�֐��Őݒ肵���ŏ��o�b�t�@�T�C�Y�́ACriMvEasyPlayer::GetMovieInfo �֐��Ŏ擾���郀�[�r���ɂ͔��f����܂���B
	 * CriMvEasyPlayer::GetMovieInfo �֐��Ŏ擾�ł���̂̓��[�r�f�[�^�̖{���̏��ł��B<br>
	 * 
	 * �{�֐��̌Ăяo���́A CriMvEasyPlayer::Prepare �֐��܂��� CriMvEasyPlayer::Start �֐��̑O�܂łɎ��s���Ă��������B
	 * 
	 * �ŏ��o�b�t�@�T�C�Y�� 0���w�肵���ꍇ�A�ŏ��o�b�t�@�T�C�Y�̓��[�r�f�[�^�̎��l�ƂȂ�܂��B<br>
	 * �܂��A�A�v���P�[�V������ CriMvEasyPlayer::SetStreamingParameters �֐����Ăяo�����ꍇ�͖{�֐���
	 * �ݒ肵���l�����A CriMvEasyPlayer::SetStreamingParameters �֐��̎w�肪�D�悳��܂��B
	 * 
	 * \sa CriMvEasyPlayer::GetMovieInfo(), CriMvEasyPlayer::SetMaxChunkSize()
	 */
	void SetMinBufferSize(CriUint32 min_buffer_size, CriError &err=CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Retrieves various movie playback statistics
	 *
	 * \param       playinfo        Playback statistics structure
	 * \param       err             Optional error code
	 *
	 * This is a DEBUG function and is not normally needed by applications.
	 *
	 * Retrieves a number of performance statistics dealing with movie playback, including how often
	 * a frame could not be retrieved and how close the plaback framerate is to the movie's expected
	 * playback.
	 *
	 * This information is updated on every call to CriMvEasyPlayer::IsNextFrameOnTime().  In order for
	 * the values to be accurate, an application should call IsNextFrameOnTime() once each time through
	 * its main loop.  Calling IsNextFrameOnTime() too often or not often enough will result in
	 * misleading statistics.
	 *
	 * \sa CriMvEasyPlayer::IsNextFrameOnTime()
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		���[�r�Đ����̎擾
	 *  \param		playinfo		���[�r���i�Ԃ�l�j
	 *  \param		err		�G���[���i�ȗ��j
	 *
	 * �{�֐��͒ʏ�A�A�v���P�[�V��������͎g�p���܂���B�f�o�b�O�p�̊֐��ł��B
	 *
	 * ���ݍĐ����Ă��郀�[�r�̍Đ���� CriMvPlaybackInfo �\���̂��擾�ł��܂��B<br>
	 * ���̏�񂩂�r�f�I�t���[���̎擾�Ԋu��A�r�f�I�t���[���̃f�R�[�h�x���Ȃǂ�m�邱�Ƃ��ł��܂��B<br>
	 * 
	 * �Đ����̓A�v�����Ăяo�� CriMvEasyPlayer::IsNextFrameOnTime() ���ōX�V���܂��B<br>
	 * �A�v���P�[�V������ CriMvEasyPlayer::IsNextFrameOnTime()���Ăяo���Ȃ��ꍇ��A���C�����[�v�ŕ�����
	 * �Ăяo���ꍇ�͏�񂪐������X�V����Ȃ��̂Œ��ӂ��Ă��������B<br>
	 * 
	 * \sa CriMvEasyPlayer::IsNextFrameOnTime()
	 */
	void GetPlaybackInfo(CriMvPlaybackInfo & playinfo, CriError &  err = CriMv::ErrorContainer);

	/*EN
	 * \ingroup MODULE_OPTION
	 *
	 * \brief       Gets the result of the last attempt to retrieve a video frame
	 *
	 * \param       err         Optional error code
	 *
	 * \return      The result of the last frame retrieval
	 *
	 * Returns one of the following values:
	 *
	 * <table border = 0>
	 * <tr><th>Value</th>   <th>Meaning</th></tr>
	 * <tr><td><b>CRIMV_LASTFRAME_OK</b></td>           <td>The frame was successfully retrieved.</td></tr>
	 * <tr><td><b>CRIMV_LASTFRAME_TIME_EARLY</b></td>   <td>It is too soon to display this frame.  The frame time
	 *                                                      is greater than the current playback time.</td></tr>
	 * <tr><td><b>CRIMV_LASTFRAME_DECODE_DELAY</b></td> <td>The frame has not been decoded yet, or CRI Movie is not 
	 *                                                      done decoding it.</td></tr>
     * <tr><td><b>CRIMV_LASTFRAME_DISCARDED</b></td>    <td>The frame was discarded, by calling 
     *                                                      CriMvEasyPlayer::DiscardNextFrame()</td></tr>
     * </table>
	 * 
	 * \remarks
	 * The result is determined by calling CriMvEasyPlayer::IsNextFrameOnTime(), not the GetFrameOnTimeXXX()
	 * functions.
	 *
	 * \sa CriMvEasyPlayer::IsNextFrameOnTime(), CriMvEasyPlayer::DiscardNextFrame()
	 */
	/*JP
	 * \ingroup MODULE_OPTION
	 * \brief		�O��̃r�f�I�t���[���擾�̌��ʂ��擾����
	 *  \param		err		�G���[���i�ȗ��j
	 *  \return				�t���[���擾���ʂ̗񋓒l
	 * 
	 * �{�֐��͒ʏ�A�A�v���P�[�V��������͎g�p���܂���B�f�o�b�O�p�̊֐��ł��B
	 *
	 * �O��̃r�f�I�t���[���擾�̌��ʂ�Ԃ��܂��B
	 * �r�f�I�t���[���̃f�R�[�h���Ԃɍ����Ă���̂��ǂ������`�F�b�N���邱�Ƃ��o���܂��B
	 * 
	 * ����: <BR>
	 * �r�f�I�t���[���擾�̌��ʂƂ́A��{�I�ɃA�v���P�[�V�������Ăяo�� CriMvEasyPlayer::IsNextFrameOnTime() �̌��ʂ�����
	 * �X�V���܂��BGetFrameOnTime�֐��̌��ʂł͂���܂���B
	 *
	 * \sa CriMvEasyPlayer::IsNextFrameOnTime()
	 */
	CriMvLastFrameResult GetLastFrameResult(CriError &  err = CriMv::ErrorContainer);

	/* [Unofficial] */
	/* Set the number of decoded frames to keep internally during the MVEASY_STATUS_PREP status. */
	/* Adjust the number if you need to reduce latesy for the first frame */
	void SetNumberOfFramesForPrep(CriUint32 num_frames, CriError &  err = CriMv::ErrorContainer);

	/* [Unofficial]
	 * \ingroup MODULE_OPTION
	 * \brief		�Đ��I��/��~�ʒm�R�[���o�b�N�֐��̓o�^
	 *  \param		func	�Đ��I��/��~�ʒm�R�[���o�b�N�֐�
	 *  \param		usrobj	���[�U�I�u�W�F�N�g
	 *  \param		err		�G���[���i�ȗ��j
	 * 
	 * �Đ��I������эĐ���~��ʒm����R�[���o�b�N�֐���o�^���܂��B
	 * ���̃R�[���o�b�N�֐��́A�w�b�_���/�Đ�����/�Đ���Ԃ���Đ���~/�Đ��I����Ԃ�
	 * �J�ڂ�������Ɉ�x�����Ăяo����܂��B
	 * �R�[���o�b�N�֐��̌Ăяo���� CriMvEasyPlayer::Update() ����s���܂��B
	 * 
	 * �o�^�����R�[���o�b�N�֐����ł́A���[�r�Đ����R���g���[������֐��i�Ⴆ�� CriMvEasyPlayer::Stop()�j
	 * ���Ăяo���Ă͂����܂���B
	 * 
	 * ����: MVEASY_STATUS_PLAYEND��Ԃ���MVEASY_STATUS_STOP��Ԃւ̑J�ڎ��ɂ̓R�[���o�b�N�֐��͌Ăяo����܂���B
	 */
	void SetStopCompleteCallback(void (*func)(CriMvEasyPlayer *mveasy, void *usrobj), 
							void *usrobj, CriError &err=CriMv::ErrorContainer);

	/* For FAST_LATENCY */
	/*************************************************************************************/
	/* �R���t�B�O�w��̃n���h���쐬�֐� */
	static CriMvEasyPlayer* CRIAPI Create(CriHeap heap, 
								   CriMvHandleConfig *config,
								   CriMvFileReaderInterface *freader, 
								   CriMvSystemTimerInterface *stimer, 
								   CriMvSoundInterface *sound, 
								   CriError &err=CriMv::ErrorContainer);

	/* For Sofdec2 */
	/*************************************************************************************/
	enum MetaFlag {
		MVEASY_META_FLAG_CUE  = 0x0001,
		MVEASY_META_FLAG_SEEK = 0x0002,
		MVEASY_META_FLAG_ALL  = MVEASY_META_FLAG_CUE + MVEASY_META_FLAG_SEEK,

		/* Keep enum 4bytes */
		MVEASY_META_FLAG_MAKE_ENUM_SINT32 = 0x7FFFFFFF
	};

	enum ReferFrameResult {
		MVEASY_REFER_FRAME_RESULT_OK = (1),
		MVEASY_REFER_FRAME_RESULT_SHORT_INPUT = (2),
		MVEASY_REFER_FRAME_RESULT_SHORT_CPUTIME = (3),
		MVEASY_REFER_FRAME_RESULT_DEMUX_STUCK = (4),

		/* Keep enum 4bytes */
		MVEASY_REFER_FRAME_RESULT_MAKE_ENUM_SINT32 = 0x7FFFFFFF
	};

	/* ���[�U����̃��[�N�̈�n���ŁA�n���h���쐬�֐� */
	static CriMvEasyPlayer* CRIAPI Create(void *work, CriSint32 work_size,
								   CriMvHandleConfig *config,
								   CriMvFileReaderInterface *freader, 
								   CriMvSystemTimerInterface *stimer, 
								   CriMvSoundInterface *sound, 
								   CriError &err=CriMv::ErrorContainer);

	/* �Đ��p���[�N�̈�T�C�Y�̌v�Z */
	CriSint32 CalcPlaybackWorkSize(const CriMvStreamingParameters *stmprm, CriError &  err = CriMv::ErrorContainer);

	/* �Đ��p���[�N�̈�̐ݒ�֐� */
	void SetPlaybackWork(void *work, Sint32 work_size, CriError &  err = CriMv::ErrorContainer);

	/* ���^�f�[�^���[�N�p�R�[���o�b�N�֐� */
	void SetMetaDataWorkAllocator(CriMvMetaDataWorkMallocFunc allocfunc, CriMvMetaDataWorkFreeFunc freefunc,void *usrobj, CriMvMetaFlag meta_flag);

	/* �����Ŏw�肵���t���[�����̕\������ */
	CriBool IsFrameOnTime(const CriMvFrameInfo *frameinfo, CriError &err=CriMv::ErrorContainer);

	/* �t���[���̎Q�Ɓ@*/
	ReferFrameResult ReferFrame(CriMvFrameInfo &frameinfo, CriError &err=CriMv::ErrorContainer);

	/* YUV�ʃo�b�t�@�t�H�[�}�b�g�ł̃o�b�t�@�擾 */
	CriBool LockFrameYUVBuffersWithAlpha(CriMvYuvBuffers &yuvbuffers, CriMvFrameInfo &frameinfo, CriMvAlphaFrameInfo &alpha_frameinfo, CriError &err=CriMv::ErrorContainer);

	/* LockFrameYUVBuffersWithAlpha�Ń��b�N�����t���[���̉�� */
	CriBool UnlockFrameBufferWithAlpha(CriMvFrameInfo *frameinfo, CriMvAlphaFrameInfo *alpha_frameinfo, CriError &err=CriMv::ErrorContainer);

	/* 32bitARGB�o�b�t�@�t�H�[�}�b�g�ւ̃R�s�[�֐� */
	CriBool CopyFrameToBufferARGB32(CriUint8 *dstbuf, CriUint32 dst_pitch, CriUint32 dst_bufsize, 
			const CriMvYuvBuffers *srcbufs, const CriMvFrameInfo *src_vinf, const CriMvAlphaFrameInfo *src_ainf, CriError &err=CriMv::ErrorContainer);

	/* 32bitARGB�o�b�t�@�t�H�[�}�b�g�փ��݂̂̃R�s�[�֐� */
	CriBool CopyAlphaToBufferARGB32(
		const CriMvFrameInfo *src_vinf, 
		CriUint8 *dst_buf, 
		CriUint32 dst_pitch, 
		CriUint32 dst_bufsize, 
		CriError &err=CriMv::ErrorContainer
	);

#if defined(XPT_TGT_IPHONE) || defined(XPT_TGT_WINMO) || defined(XPT_TGT_ANDROID) || defined(XPT_TGT_CQSH2A) || defined(XPT_TGT_ACRODEA) || defined(XPT_TGT_NACL) || defined(XPT_TGT_SH7269) || defined(XPT_TGT_PC)|| defined(XPT_TGT_TRGP6K)
	CriBool CopyFrameToBufferRGB565(CriUint8 *dstbuf, CriUint32 dst_pitch, CriUint32 dst_bufsize, 
			const CriMvYuvBuffers *srcbufs, const CriMvFrameInfo *src_vinf, const CriMvAlphaFrameInfo *src_ainf, CriError &err=CriMv::ErrorContainer);
#endif

	/* YUV�ʃo�b�t�@�t�H�[�}�b�g�̃R�s�[�֐� */
	CriBool CopyFrameToBuffersYUV(CriMvYuvBuffers *dstbufs, 
			const CriMvFrameInfo *src_vinf, const CriMvAlphaFrameInfo *src_ainf, CriError &err=CriMv::ErrorContainer);

	/* ���[�h�o�b�t�@�T�C�Y�̋����w�� */
	void SetReadBufferSize(CriUint32 buffer_size, CriError &err=CriMv::ErrorContainer);

	CriUint32 GetMinBufferSize(CriError &err=CriMv::ErrorContainer);
	/**************************************************************************************/
	void SetVideoFramerate(CriUint32 framerate_n, CriUint32 framerate_d);

	void SetCompareFrameTimeCallback(
		CriBool (*func)(CriMvEasyPlayer *mveasy, CriMvFrameInfo *frameinfo, CriUint64 count, CriUint64 unit, void *usrobj), 
		void *usrobj, CriError &err=CriMv::ErrorContainer);
	void SetSeekFrameAndOffset(CriSint32 seek_frame_id, CriUint64 seek_byte, CriError &err=CriMv::ErrorContainer);

	enum InputMode {
		MVEASY_INPUT_UNDEFINED,
		MVEASY_INPUT_STREAMING,		/* �������X�g���[�~���O */
		MVEASY_INPUT_MEMORY,		/* ���������ڎQ�Ɓi���j�r�i�j */

		/* Keep enum 4bytes */
		MVEASY_INPUT_MAKE_ENUM_SINT32 = 0x7FFFFFFF
	};
	/* ������: ���̊֐��̂��߂� InputMode ��`���b���public�ֈړ� */
	void SetMemoryPlaybackType(InputMode memplay_type, CriError &err=CriMv::ErrorContainer);

	/* �f�R�[�h�X�L�b�v�̎������s���[�h */
	//void SetAutoSkipDecode(CriBool sw, CriFloat32 margin_msec, CriError &err=CriMv::ErrorContainer)

	/* �t�@�C���v���̍ăR�[���o�b�N�v�� */
	void DeferFileRequest(CriError &err=CriMv::ErrorContainer);

	/* for specific use */
	void SetHeaderAndBodyData(const CriUint64Adr header_ptr, CriSint64 header_size, 
			const CriUint64Adr body_ptr, CriSint64 body_size);
	CriSint32 CalcFramepoolWorkSize(const CriMvStreamingParameters *stmprm, CriError &  err = CriMv::ErrorContainer);
	void SetFramepoolWork(void *work, CriSint32 work_size, CriError &  err = CriMv::ErrorContainer);
	void SetFramepoolWorkAllocator(CriMvFramepoolWorkMallocFunc allocfunc, CriMvFramepoolWorkFreeFunc freefunc, void *usrobj);
	void SetCapacityOfPictureSize(CriSint32 video_picsize, CriSint32 alpha_picsize, CriError &err);
	void GetUsrCapacityOfPictureSize(CriSint32 *video_picsize, CriSint32 *alpha_picsize);
	void SetMaxMovieSize(CriUint32 max_width, CriUint32 max_height, CriError &err);
	void GetMaxMovieSize(CriUint32 *max_width, CriUint32 *max_height);
	CriBool PlaybackResourceAllocated() const;
	CriBool CanReusePlaybackResource(const CriMvStreamingParameters *stmprm) const;
	CriBool GetReusePlaybackResourceFlag() const;

private:
	/* CRI internal use only */

	void initializeHandleParameters(void);
	void initializeCompareFrameParameters(void);
	void resetHandleParameters(void);
	CriBool startInputAndDecoding(void);
	CriBool isEndReadFile(void);
	void reinputDataForLooping(void);
	void executeFileCloseServer(void);
	void supplyDataFromStreamer(void);
	void executeFileReadServer(void);
	void executeFileOpenServer(void);
	void executeCuePointServer(void);
	//void executeAutoSkipDecode(void);
	void executeWaitStatusServer(CriError &err = CriMv::ErrorContainer);
	void surveilTerminationInput(void);
	CriBool isAvailableCenterVoice(const CriMvStreamingParameters *stmprm);
	CriBool isAvailableSubAudio(const CriMvStreamingParameters *stmprm);
	CriBool attachCenterVoice(void);
	void detachCenterVoice(void);

	CriBool attachSubAudioHandle(CriHeap heap);
	void detachSubAudioHandle(void);

 	void getAudioTime(CriUint64 &out_count, CriUint64 &out_unit, CriUint64 s_count, CriUint64 s_unit);

	CriBool allocAndCreateModules(void);
	void startModules(void);
	void requestStopModules(void);
	CriBool closeFileIfOpening(void);
	CriBool isCompleteStopModules(void);
	void tryCleanupModules(CriMvPlyStatus mvstat);
	CriBool tryFreeAndDestroySubmodules(void);
	void freeAndDestroyModules(void);

	CriUint32 adjustNumTrackAudioOut(void);

	/* �X�g���[�~���O�p�̃p�����[�^�擾 */
	/* GetMovieInfo()�Ƃ̈Ⴂ�̓��[�U�w��l���ǂ��܂Ŕ��f����邩�B
	 * �Ⴆ�΁A�ő�`�����N�T�C�Y�͂��̊֐��ł̓��[�U�w��l���Ƃ邪�AGetMovieInfo���ƃt�@�C���̒l�B
	 * ���̊֐��́A�����ŉ��ʃ��W���[���쐬����у������m�ۂ��鎞�Ɏg���B */
	CriBool GetStreamingParameters(CriMvStreamingParameters &stmprm, CriError &err=CriMv::ErrorContainer);

	void setNormalErrorStatus(const CriChar8 *errmsg);

	CriBool compareFrameTimeSimple(CriMvEasyPlayer *mveasy, CriMvFrameInfo *frameinfo, CriUint64 count, CriUint64 unit, void *usrobj);
	static CriBool compareFrameTimeFluctuation(CriMvEasyPlayer *mveasy, CriMvFrameInfo *frameinfo, CriUint64 count, CriUint64 unit, void *usrobj);

	Bool isNextFrameOnTime(CriBool update_stats, CriError &err); 
	Bool checkFrameTime(CriMvFrameInfo *frameinfo);
	void updateGetFrameInfo(CriBool time_ready, CriBool acquired_frame, CriBool discard_frame, CriUint64 frame_count, CriUint64 frame_unit);

	/* for DEBUG */
	void crimveasy_SetSeekInfo(void);
	
	void executeUpdate(CriError &err);

private:
	static CriUint32 crimveasy_SupplyPcmDataByFloat32(void *obj, CriUint32 nch, CriFloat32 *pcmbuf[], CriUint32 req_nsmpl);
	static CriUint32 crimveasy_SupplyPcmDataBySint16(void *obj, CriUint32 nch, CriSint16 *pcmbuf[], CriUint32 req_nsmpl);
	static CriUint32 crimveasy_CalcAvailableNumSmpls(CriMvEasyPlayer *mveasy);
	static CriUint32 crimveasy_GetWave16(CriMvEasyPlayer *mveasy, CriUint32 nch, CriSint16 *pcmbuf[], CriUint32 req_nsmpl);
	static CriUint32 crimveasy_GetWave32(CriMvEasyPlayer *mveasy, CriUint32 nch, CriFloat32 *pcmbuf[], CriUint32 req_nsmpl);

	static CriUint32 crimveasy_SupplySubAudioDataByFloat32(void *obj, CriUint32 nch, CriFloat32 *pcmbuf[], CriUint32 req_nsmpl);
	static CriUint32 crimveasy_GetSubAudioWave32(CriMvEasyPlayer *mveasy, CriUint32 nch, CriFloat32 *pcmbuf[], CriUint32 req_nsmpl);
	static CriUint32 crimveasy_SupplySubAudioDataBySint16(void *obj, CriUint32 nch, CriSint16 *pcmbuf[], CriUint32 req_nsmpl);
	static CriUint32 crimveasy_GetSubAudioWave16(CriMvEasyPlayer *mveasy, CriUint32 nch, CriSint16 *pcmbuf[], CriUint32 req_nsmpl);

public:
	CriMvPly					mvply;	/* Temporally allowed to access for debug */

private:
	CriHeap						heap;
	CriBool						user_stmprm_flag;
	CriMvStreamingParameters	stmprm;
	CriBool						alloced_submodules_flag;
	CriMvFrameInfo				frameinfo;
	CriMvAlphaFrameInfo			alpha_frame;
	CriMvFileReaderInterface	*freader;
	CriMvSoundInterface			*sndout;
	CriMvSystemTimerInterface	*stimer;
	TimerType	timertype;
	Status		pre_ezstat;
	CriBool		req_decode_header_flag;
	CriBool		req_prepare_flag;
	CriBool		pause_flag;
	CriUint32	npools;
	CriSint32	track_play_audio;
	CriUint32	num_track_audio_data;
	CriUint32	num_track_audio_out;
	CriBool		loop_flag;
	CriUint64	time_count;
	CriUint64	time_unit;
	CriSint64	total_read;
	CriSint64	fsize_byte;
	CriChunk	read_crick;
	CriBool		exe_open;
	CriBool		exe_close;
	CriBool		exe_read;

	CriUint64		time_syslog_count;		/* �V�X�e���^�C�}�̋L�^ */
	CriUint64		time_syslog_unit;
	CriUint64		time_ofs_count;			/* �I�[�f�B�I�I�����̃V�X�e���^�C�} */
	CriUint64		time_ofs_unit;
	CriUint64		time_prev_audio_count;	/* �I�[�f�B�I�����ω��`�F�b�N�p */
	CriUint64		time_prev_audio_unit;

	CriFloat32		user_buffering_sec;		/* 0.0f means AUTO */
	CriUint32		user_max_bitrate;		/* 0 means AUTO */
	CriUint32		user_max_chunk_size;	/* 0 means AUTO */
	CriUint32		user_min_buffer_size;	/* 0 means AUTO */
	CriSint32		user_read_buffer_size;	/* CRIMV_READ_BUFFER_SIZE_AUTO means AUTO */
	CriSint32		user_video_capacity_of_picsize;	/* 0 means AUTO */
	CriSint32		user_alpha_capacity_of_picsize;	/* 0 means AUTO */
	CriUint32		user_max_width;
	CriUint32		user_max_height;

	CriSint32			usr_subtitle_channel;
	CriMvSubtitleInfo	sbtinfo;
	CriFloat32			sbt_start_msec;
	CriFloat32			sbt_end_msec;
	CriUint8			*sbtbuf;

	enum InputSrc {
		MVEASY_INPUT_SRC_UNDEFINED,
		MVEASY_INPUT_SRC_FILE,
		MVEASY_INPUT_SRC_MEMORY,
		/* Keep enum 4bytes */
		MVEASY_INPUT_SRC_MAKE_ENUM_SINT32 = 0x7FFFFFFF
	};
	InputSrc                 input_src;
	CriMvFileReaderInterface *ext_reader;
	CriMvFileReaderInterface *mem_reader;

	enum InputSupplyStatus {
		MVEASY_SUPPLY_STOP,
		MVEASY_SUPPLY_REQ_OPEN,
		MVEASY_SUPPLY_OPENING,
		MVEASY_SUPPLY_READING,
		MVEASY_SUPPLY_REQ_CLOSE,
		MVEASY_SUPPLY_CLOSING,

		/* Keep enum 4bytes */
		MVEASY_SUPPLY_MAKE_ENUM_SINT32 = 0x7FFFFFFF
	};

	InputMode			memplay_type;	/* �������Đ����X�g���[�����邩���j�r�i���邩 */
	InputMode			input_mode;
	InputSupplyStatus	supply_stat;
	CriBool				change_file_mode;
	CriChar8			file_name[CRIMV_MAX_FILE_NAME];
	CriUint64			file_offset;
	CriSint64			file_range;
	CriUint8			*dataptr;
	CriUint32			datasize;
	CriBool				terminate_flag;
	CriSint32			reinput_cnt;

	enum NextEntryState {
		MVEASY_NEXT_ENTRY_NONE,
		MVEASY_NEXT_ENTRY_READY,
		MVEASY_NEXT_ENTRY_DEFER,
		/* Keep enum 4bytes */
		MVEASY_NEXT_ENTRY_MAKE_ENUM_SINT32 = 0x7FFFFFFF
	};
	NextEntryState	next_entry_state;

	CriUint32		center_ch;
	CriSint32		usr_voice_track;			// by SetCenterVoice()
	CriSint32		voice_attached_track;

	// Sub Audio 
	CriMvSoundInterface		*if_subaudio;
	CriSint32				usr_subaudio_track;
	CriSint32				attached_subaudio_track;

	void (*cbfunc_file_request)(CriMvEasyPlayer *mveasy, void *usrobj);
	void *usrobj_file_request;

	CriBool (*cbfunc_compare_ftime)(CriMvEasyPlayer *mveasy, CriMvFrameInfo *frameinfo, CriUint64 count, CriUint64 unit, void *usrobj);
	void *usrobj_compare_ftime;

	/* Seek Playback */
	CriSint32		seek_frame_id;
	CriUint64		seek_byte;

	/* CuePoint */
	void (*cbfunc_cuepoint)(CriMvEasyPlayer *mveasy, CriMvEventPoint *eventinfo, void *usrobj);
	void *usrobj_cuepoint;

	/* Playback Statistics */
	CriBool   start_getfrm;
	CriUint64 last_getfrm_count;
	CriFloat32 sum_diff_time;
	CriMvPlaybackInfo playinfo;
	CriMvLastFrameResult last_frm_result;

	/* Block flag for simultanious calls on multi-threads */
	CriSint32 execute_decode_block_flag;
	CriSint32 update_block_flag;

	/* Stop Completion Notification */
	void (*cbfunc_stopcomplete)(CriMvEasyPlayer *mveasy, void *usrobj);
	void *usrobj_stopcomplete;	

	/* For Sofdec2 */
	void *playback_work;
	CriSint32 playback_work_size;
	void *mvply_work;
	CriHeap	heap_mveasy;
	CriHeap heap_extra_sound;
	CriBool reuse_modules_flag;

	/* For Debug */
	volatile CriUint8 end_sequence_info;

	/* Handle Protection */
	void *cshn;
	void *cs_work;

	/* Skip Decoding */
	//CriBool skip_auto_flag;
	//CriFloat32 margin_msec;

public:
	enum FrameCompareMode {
		MVEASY_COMPARE_MODE_JUST,			/* ���m�Ɏ�����r����     */
		MVEASY_COMPARE_MODE_DELAY_GET,		/* �^�C�}������O�|�����t���[���͂Ȃ�ׂ��n���Ȃ� */
		MVEASY_COMPARE_MODE_FAST_GET,		/* �^�C�}�����𐅑������t���[���͂Ȃ�ׂ��n��     */

		/* Keep enum 4bytes */
		MVEASY_COMPARE_MODE_MAKE_ENUM_SINT32 = 0x7FFFFFFF
	};
	FrameCompareMode	compare_mode;
	CriFloat32			accuracy_system_tmr_msec;	/* �V�X�e���������x   milli sec  */
	CriFloat32			accuracy_audio_tmr_msec;	/* �I�[�f�B�I�������x milli sec  */
	CriFloat32			fluctuation_system;			/* �V�X�e���iSyncFrame)�̗h�炬 milli sec */	
	CriFloat32			fluctuation_adjust;			/* �h�炬�␳ */
//	CriFloat32			fluctuation_system_msec;	/* �V�X�e��������炬��   milli sec  */
//	CriFloat32			fluctuation_audio_msec;		/* �I�[�f�B�I������炬�� milli sec  */
//	CriFloat32			fluctuation_adjust;			/* ��炬�␳�{��         */
//	CriSint32			fluctuation_system_usec;	/* �V�X�e��������炬��   micro sec */
//	CriSint32			fluctuation_audio_usec;		/* �I�[�f�B�I������炬�� micro sec */
//	CriFloat32			fluctuation_adjust_multi;	/* ��炬�␳�{��         */
//	CriSint32			fluctuation_adjust_add;		/* ��炬�␳�I�t�Z�b�g   */

private:
	CriBool		req_stop_modules;
	CriBool		error_flag;
	CriBool		req_start_modules;

protected:
	CriMvEasyPlayer(CriHeap heap,
					CriMvFileReaderInterface *freader, 
					CriMvSystemTimerInterface *stimer, 
					CriMvSoundInterface *sound, 
					CriError &err=CriMv::ErrorContainer);
	virtual ~CriMvEasyPlayer();

private:
	CriMvEasyPlayer(void);	//disabled
};




#endif	/* CRI_MOVIE_H_INCLUDED */
