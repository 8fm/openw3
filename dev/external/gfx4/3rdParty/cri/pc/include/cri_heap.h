#ifndef	_CRI_HEAP_H_INCLUDED
#define	_CRI_HEAP_H_INCLUDED
/****************************************************************************
 *																			*
 *				CRI Heap Manager "CriHeap" Library							*
 *																			*
 *				2005-03-17	written by satouo								*
 *																			*
 ****************************************************************************/
/*!
 *	\file		cri_heap.h
 */

/**************************************************************************** 
 *      �C���N���[�h�t�@�C��												* 
 *      Include file														* 
 ****************************************************************************/
#include "cri_xpt.h"

#ifdef XPT_TGT_PC
#pragma pack(push)
#pragma pack(1) //The boundary value of structure is adjusted to 1 byte. 
#endif

/**************************************************************************** 
 *		�萔�}�N��															* 
 *		MACRO CONSTANT														* 
 ****************************************************************************/
/*	Version number of CRIHEAP	*/
#define CRIHEAP_NAME_STRINGS		"CRI Heap"
#define	CRIHEAP_VERSION_STRINGS		"1.21.02"

/* Default memory alignment */
#define	CRIHEAP_DEFAULT_MEM_ALIGN	(8)

/**************************************************************************** 
 *		�񋓒萔�}�N��														* 
 *		ENUM CONSTANT														* 
 ****************************************************************************/
/*JP
 * \brief �������̃^�C�v
 */
/*EN
 * \brief Types of heap
 */
typedef enum {
	CRIHEAP_TYPE_FIX       = (1),	
	CRIHEAP_TYPE_TEMPORARY = (2),
	CRIHEAP_TYPE_DYNAMIC   = (3), // unuse
	CRIHEAP_TYPE_NONE      = (0),
	/* enum be 4bytes */
	CRIHEAP_TYPE_ENUM_BE_SINT32 = 0x7FFFFFFF
} CriHeapType;
	
/**************************************************************************** 
 *		�f�[�^�^															* 
 *      Data type declaration												* 
 ****************************************************************************/
/*	CriHeap�n���h��		*/
/*	CriHeap handle		*/
#ifndef CRIHEAP_DEFINED
#define	CRIHEAP_DEFINED

/* internal */
typedef struct CriHeapBlockTag {
	struct CriHeapBlockTag *prevblock;
	struct CriHeapBlockTag *nextblock;
	CriSint32 memsize;
	CriUint8  used;
	CriUint8  type;
	CriUint16 alignspc;
	CriUint16 gap;
	CriChar8  *nameadr;
} CriHeapBlock, *CriHeapBlockPtr;

/*JP
 * \brief CRI Heap�n���h��
 * \struct CriHeap
 * \ingroup CRIHEAP_BASIC
 * CRI Heap��p���ă������A���P�[�g���s�����߂ɕK�v�ȃn���h���ł��B<br>
 * ���̃n���h���ɑ΂��āA�������A���P�[�g�⃁�����t���[���s���܂��B
 * \sa criHeap_Create(), criHeap_Destroy()
 */
/*EN
 * \brief CRI Heap handle
 * \struct CriHeap
 * \ingroup CRIHEAP_BASIC
 * \par Description:
 * The heap handle.  This data structure needs to exist through the life cycle of
 * the heap.  It allows the library to allocate and deallocate memory
 * internally within the heap.  Typically this handle gets allocated at the
 * beginning of the heap memory that you've provided for the heap, but don't
 * depend on this behavior.  The heap itself is opaque.  You can of course
 * allocate multiple non-contiguous heaps, but these will be separate heap 
 * structures and a single allocation won't choose between them.
 *
 * \sa criHeap_Create(), criHeap_Destroy()
 */
typedef	struct _criheap_struct {
	struct _criheap_vfunctiontable *vtbl;
	CriSint32			totalsize;
	CriSint32			peaksize;
	CriSint32			currentsize;
	CriHeapBlock	*topblock;	
	CriHeapBlock	*taleblock;	
} CriHeapObj, *CriHeap;

/*JP
 * \brief CRI Heap���z�֐��e�[�u��
 * \struct criHeapVirtualFunctionTable
 * \ingroup CRIHEAP_BASIC
 * �Ǝ��̃������A���P�[�g�֐�����������ۂɎg�p���鉼�z�֐��e�[�u���\���̂ł��B<br>
 * �e�֐��̓}���`�X���b�h�Z�[�t�ł���K�v������܂��B<br>
 * \par ���l:
 * �R�Ԗڂ̃A���P�[�g�֐��͌��ݖ��g�p�ł��B
 */
/*EN
 * \brief CRI Heap Virtual Function Table
 * \struct criHeapVirtualFunctionTable
 * \ingroup CRIHEAP_BASIC
 * \par Description:
 * The virtual functions table for original allocation functions.<br>
 * The third allocation function is not in use currently.
 */
typedef struct _criheap_vfunctiontable {
	/*JP
	 * \brief �������̃A���P�[�g�i�������̈�̐�[����j
	 * \par ����:
	 * criHeap_AllocFix�֐��Ăяo�����ɌĂ΂�܂��B
	 * \sa criHeap_AllocFix()
	 */
	/*EN
	 * \brief Memory allocation (allocating from the top of the memory area)
	 * \par Description:
	 * Called from criHeap_AllocFix().
	 * \sa criHeap_AllocFix()
	 */
	void *(*AllocFix)(CriHeap heap, CriSint32 size, const CriChar8 *name, CriSint32 align);
	/*JP
	 * \brief �������̃A���P�[�g�i�������̈�̏I�[����j
	 * \par ����:
	 * criHeap_AllocFix�֐��Ăяo�����ɌĂ΂�܂��B
	 * \sa criHeap_AllocTemporary()
	 */
	/*EN
	 * \brief Memory allocation (allocating from the tail of the memory area)
	 * \par Description:
	 * Called from criHeap_AllocTemporary().
	 * \sa criHeap_AllocTemporary()
	 */
	void *(*AllocTemporary)(CriHeap heap, CriSint32 size, const CriChar8 *name, CriSint32 align);
	/*JP
	 * \brief ���g�p�֐�
	 */
	/*EN
	 * \brief Unused
	 */
	void *(*AllocDynamic)(CriHeap heap, CriSint32 size, const CriChar8 *name, CriSint32 align); // unused

	/*JP
	 * \brief �������̃t���[
	 * \par ����:
	 * criHeap_Free�֐��Ăяo�����ɌĂ΂�܂��B
	 * \sa criHeap_Free()
	 */
	/*EN
	 * \brief Memory deallocation
	 * \par Description:
	 * Called from criHeap_Free().
	 * \sa criHeap_Free()
	 */
	CriSint32 (*Free)(CriHeap heap, void *ptr);
} criHeapVirtualFunctionTable;

#endif


/****************************************************************************
 *		�֐��̐錾
 *      Function Declaration
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*==========================================================================*
 *		���C�u�����̏������ƏI������
 * 		Initialize and Finalize of Library
 *==========================================================================*/
/*JP
 * \brief CRI Heap���C�u�����̏�����
 * \ingroup CRIHEAP_BASIC
 * \par ����:
 * CRI Heap���C�u�������g�p���邽�߂ɕK�v�ȏ������֐��ł��B<br>
 * CRI Heap���g�p����ۂ͂��炩���ߖ{�֐����R�[������K�v������܂��B
 * \sa criHeap_Finalize()
 */
/*EN
 * \brief Initialize the heap manager.
 * \ingroup CRIHEAP_BASIC
 * \par Description:
 * This function initializes the heap manager and prepares it to manage heaps.
 * This function only has an effect the first time it is called in your program.
 * Subsequent calls only increment an internal counter.  This allows your 
 * program to call criHeap_Initialize() in pairs with criHeap_Finalize() 
 * throughout your program modules, and only the initial call to criHeap_Initialize()
 * and the final call to criHeap_Finalize() will have any effect.
 * \if ps2
 * \par PS2 only:
 * This function does a sanity check to make sure that the size of the CriHeapObj
 * data structure is a factor of 16.  If it is not a factor of 16, this function
 * hangs.
 * \endif
 * \sa criHeap_Finalize()
 */
void CRIAPI criHeap_Initialize(void);

/*JP
 * \brief CRI Heap���C�u�����̏I��
 * \ingroup CRIHEAP_BASIC
 * \par ����:
 * CRI Heap���C�u�������I�����邽�߂ɕK�v�ȏI���֐��ł��B<br>
 * CRI Heap�̎g�p���I������ۂ͖{�֐����R�[�����Ă��������B
 * \sa criHeap_Initialize()
 */
/*EN
 * \brief Finalize the heap manager.
 * \ingroup CRIHEAP_BASIC
 * \par Description:
 * This function finalizes the heap manager.  If the criHeap_Initialize() function
 * is called <i>n</i> times, then the <i>n</I>th time that criHeap_Finalize()
 * is called, this function invalidates any heaps currently in use.  Calling
 * any heap function after the <i>n</i>th call to criHeap_Finalize() will
 * have unpredictable results.
 * \sa criHeap_Initialize()
 */
void CRIAPI criHeap_Finalize(void);

/*==========================================================================*
 *		�n���h���̐����E���
 *==========================================================================*/
/*JP
 * \brief CRI Heap�n���h���̐���
 * \ingroup CRIHEAP_BASIC
 * \param ptr �������̈�̃|�C���^�B
 * \param size �������̈�̃T�C�Y�B
 * \return CRI Heap�n���h���B<br>�����Ɏ��s�����ꍇ�́ANULL���Ԃ�܂��B
 * \par ����:
 * CRI Heap�n���h���𐶐����܂��B<br>
 * �{�֐��ŗ^���郁�����̈�́ACRI Heap���g�̃n���h���̈��A���P�[�g����
 * �m�ۂ���郁�����̈�ƂȂ�܂��B<br>
 * �Ǘ��̈���܂ނ��߁A�������̈悷�ׂĂ��A���P�[�g�o���Ȃ��_�ɒ��ӂ��Ă��������B
 * �Ǘ��̈�̖ڈ��́u�P�n���h��������sizeof(CriHeapObj)�v�{�u�P�A���P�[�g������
 * sizeof(CriHeapBlock)�{�e�������A���C�����g�ɕK�v�ȃT�C�Y�v�ƂȂ�܂��B
 * \code
 * CriChar8 heap_buffer[0x2000];
 * CriHeap heap = criHeap_Create((void *)heap_buffer, sizeof(heap_buffer);
 * \endcode
 * \sa criHeap_Destroy()
 */
/*EN
 * \brief Create a contiguous heap in memory.
 * \ingroup CRIHEAP_BASIC
 * \param ptr A pointer to the start of the memory region to use as a heap.
 * \param size The size of the region to be used as a heap.
 * \return A valid CriHeap handle if successful, or NULL if unsuccessful.
 * \par Description:
 * This function allows you to set aside a region of memory for this library's
 * use as a "heap".  Heap allocation is required before active playback
 * can begin.
 * This function will fail and return NULL if the size of the memory region
 * is smaller than the CriHeapObj structure.
 * \code
 * CriChar8 heap_buffer[0x2000];
 * CriHeap heap = criHeap_Create((void *)heap_buffer, sizeof(heap_buffer);
 * \endcode
 * \sa criHeap_Destroy()
 */
CriHeap CRIAPI criHeap_Create(void *ptr, CriSint32 size);

/*JP
 * \brief ��X���b�h�Z�[�t��CRI Heap�n���h���̐���
 * \ingroup CRIHEAP_BASIC
 * \param ptr �������̈�̃|�C���^�B
 * \param size �������̈�̃T�C�Y�B
 * \return CRI Heap�n���h���B<br>�����Ɏ��s�����ꍇ�́ANULL���Ԃ�܂��B
 * \par ����:
 * CRI Heap�n���h���𐶐����܂��B<br>
 * �{�֐����g�p���č쐬���ꂽ�q�[�v�́A�r�����䂪�s�Ȃ��܂���B<br>
 * �����X���b�h���瓯���ɎQ�Ƃ����q�[�v�̍쐬�ɂ́AcriHeap_Create�֐��������p���������B
 * \sa criHeap_Create(), criHeap_Destroy()
 */
/*EN
 * \brief Create a contiguous heap in memory.
 * \ingroup CRIHEAP_BASIC
 * \param ptr A pointer to the start of the memory region to use as a heap.
 * \param size The size of the region to be used as a heap.
 * \return A valid CriHeap handle if successful, or NULL if unsuccessful.
 * \par Description:
 * This function creates a heap to use the static memory as dynamically allocatable memory.
 * The heap created by using this function is not threadsafe.
 * If heap will be accessed by multiple threads, you must create the heap by using the criHeap_Create function.
 * \sa criHeap_Create(), criHeap_Destroy()
 */
CriHeap CRIAPI criHeap_CreateNoSerialize(void *ptr, CriSint32 size);

/*JP
 * \brief CRI Heap�n���h���̉��
 * \ingroup CRIHEAP_BASIC
 * \param heap CRI Heap�n���h���B
 * \par ����:
 * CRI Heap�n���h����������܂��B<br>criHeap_Create() �Ŏw�肳�ꂽ�������̈悪
 * �������ACRI Heap�n���h���͖����ƂȂ�܂��B
 * \sa criHeap_Create()
 */
/*EN
 * \brief Destroy a previously created heap.
 * \ingroup CRIHEAP_BASIC
 * \param heap A CriHeap handle previously created with criHeap_Create().
 * \par Description:
 * This function frees all internal allocations previously performed on
 * the heap and frees the memory.  Internally, this function does in
 * fact walk through the heap, finding and freeing all allocations, e.g.
 * it is not stubbed.  So calling this function on a trashed heap
 * will have unpredictable results.
 * \sa criHeap_Create()
 */
void CRIAPI criHeap_Destroy(CriHeap heap);

/*==========================================================================*
 *		Allocation
 *==========================================================================*/
/*JP
 * \brief �������̃A���P�[�g�i�������̈�̐�[����j
 * \ingroup CRIHEAP_ALLOC
 * \param heap CRI Heap�n���h���B
 * \param size �A���P�[�g���郁�����̃T�C�Y�B
 * \param name �A���P�[�g���郁�����̖��O�B���O�̓f�o�b�O���Ɏg�p����܂��B
 * \param align �A���P�[�g���郁�����̃A���C�����g�B
 * \return �A���P�[�g�����������ւ̃|�C���^�B<br>�A���P�[�g�Ɏ��s�����ꍇ��NULL���Ԃ�܂��B
 * \par ����:
 * �������̈�̐�[������m�ۂ��郁�����A���P�[�V�����֐��ł��B�q�[�v�̈����\ref block_fix "Fix�u���b�N"���쐬���܂��B<br>
 * �������̒f�Љ���h�����߁A�����T�C�Y�^�A���C�����g�̉���ς�\ref block_fix "Fix�u���b�N"���[������T���āA�ė��p����悤�Ɏ��݂܂��B���ꂪ������Ȃ��ꍇ�́A���g�p�̋󂫗̈�i\ref block_free "Free�u���b�N"�j�𕪊����ă��������擾���܂��B
 * \code
 * CriChar8 heap_buffer[0x2000];
 * CriHeap heap = criHeap_Create((void *)heap_buffer, sizeof(heap_buffer);
 * void *memptr = criHeap_AllocFix(heap, 0x100, "forTexture1", 64);
 * \endcode
 * \sa criHeap_AllocTemporary(), criHeap_Free()
 */
/*EN
 * \brief Memory allocation (allocating from the top of the memory area)
 * \ingroup CRIHEAP_ALLOC
 * \param heap CRI Heap handle
 * \param size Memory block size to be allocated
 * \param name Memory block name, which is used on debugging
 * \param align Memory block alignment
 * \return A pointer to the allocated memory block.<br>Returns NULL if unsuccessful.
 * \par Description:
 * This function allocates a memory block from the top of the memory area and creates a \ref block_fix "Fix block" in the heap area.<br>
 * To prevent memory fragmentation, a released \ref block_fix "Fix block" with the same size and alignment is searched for in the heap area from the top to the bottom and tries to reused the block.  And if not found, an unused free block (\ref block_free "Free block") is divided and a new block is allocated.
 * \code
 * CriChar8 heap_buffer[0x2000];
 * CriHeap heap = criHeap_Create((void *)heap_buffer, sizeof(heap_buffer);
 * void *memptr = criHeap_AllocFix(heap, 0x100, "forTexture1", 64);
 * \endcode
 * \sa criHeap_AllocTemporary(), criHeap_Free()
 */
void * CRIAPI criHeap_AllocFix(CriHeap heap, CriSint32 size, const CriChar8 *name, CriSint32 align);

/*JP
 * \brief �������̃A���P�[�g�i�������̈�̏I�[����j
 * \ingroup CRIHEAP_ALLOC
 * \param heap CRI Heap�n���h���B
 * \param size �A���P�[�g���郁�����̃T�C�Y�B
 * \param name �A���P�[�g���郁�����̖��O�B���O�̓f�o�b�O���Ɏg�p����܂��B
 * \param align �A���P�[�g���郁�����̃A���C�����g�B
 * \return �A���P�[�g�����������ւ̃|�C���^�B<br>�A���P�[�g�Ɏ��s�����ꍇ��NULL���Ԃ�܂��B
 * \par ����:
 * �������̈�̏I�[������m�ۂ��郁�����A���P�[�V�����֐��ł��B�q�[�v�̈����\ref block_temporary "Temporary�u���b�N"���쐬���܂��B<br>
 * �v�������T�C�Y�^�A���C�����g���A���P�[�g�\�ȉ���ς�\ref block_temporary "Temporary�u���b�N"���I�[������T���āA�����܂��͍ė��p����悤�Ɏ��݂܂��B���ꂪ������Ȃ��ꍇ�́A���g�p�̋󂫗̈�i\ref block_free "Free�u���b�N"�j�𕪊����ă��������擾���܂��B<br>
 * �q�[�v�̈�̏I�[�����烁�������A���P�[�g���邽�߁A��[���ł̒f�Љ���h�����Ƃ��o���܂��B<br>
 * ��ɁA�ꎞ�I�ɗ��p���郁�����Ɏg�p���܂��B
 * \sa criHeap_AllocFix(), criHeap_Free()
 */
/*EN
 * \brief Memory allocation (allocating from the tail of the memory area)
 * \ingroup CRIHEAP_ALLOC
 * \param heap CRI Heap handle
 * \param size Memory block size to be allocated
 * \param name Memory block name, which is used on debugging
 * \param align Memory block alignment
 * \return A pointer to the allocated memory block.<br>Returns NULL if unsuccessful.
 * \par Description:
 * This function allocates a memory block from the bottom of the memory area and creates a \ref block_temporary "Temporary block" in the heap area.<br>
 * A released \ref block_temporary "Temporary block" available with the requested size and alignment is searched for in the heap area from the bottom and tries to divide and to reuse the block. And if not found, an unused free block (\ref block_free "Free block") is divided and a new block is allocated.<br>
 * Allocating from the bottom of the heap area will prevent fragmentation in the top of the heap area.<br>
 * Temporary block is mainly used for the memory area that is temporarily used.
 * \sa criHeap_AllocFix(), criHeap_Free()
 */
void * CRIAPI criHeap_AllocTemporary(CriHeap heap, CriSint32 size, const CriChar8 *name, CriSint32 align);

/* DYNAMIC (unuse) */
//void * CRIAPI criHeap_AllocDynamic(CriHeap heap, CriSint32 size, const CriChar8 *name, CriSint32 align);

void * CRIAPI criHeap_Alloc(CriHeap heap, CriSint32 size, const CriChar8 *name, CriSint32 align, CriSint32 type);

/*==========================================================================*
 *		Free
 *==========================================================================*/
/*JP
 * \brief �������̃t���[
 * \ingroup CRIHEAP_ALLOC
 * \param heap CRI Heap�n���h���B
 * \param ptr �A���P�[�g�����������̃|�C���^�B
 * \return �t���[���ꂽ�������T�C�Y�B
 * \par ����:
 * �A���P�[�g���ꂽ���������t���[���܂��B<br>
 * \code
 * void *memptr = criHeap_AllocFix(heap, 0x100, "forTexture1", 64);
 * criHeap_Free(heap, memptr);
 * \endcode
 * \sa criHeap_AllocFix(), criHeap_AllocTemporary()
 */
/*EN
 * \brief Memory deallocation
 * \ingroup CRIHEAP_ALLOC
 * \param heap CRI Heap handle
 * \param ptr The pointer to the allocated memory block
 * \return Deallocated memory block size
 * \par Description:
 * This function deallocates the allocated memory block.<br>
 * \code
 * void *memptr = criHeap_AllocFix(heap, 0x100, "forTexture1", 64);
 * criHeap_Free(heap, memptr);
 * \endcode
 * \sa criHeap_AllocFix(), criHeap_AllocTemporary()
 */
CriSint32 CRIAPI criHeap_Free(CriHeap heap, void *ptr);

/*==========================================================================*
 *		�r������p�֐�
 *==========================================================================*/
CriSint32 CRIAPI criHeap_EnterCriticalSection(void);
CriSint32 CRIAPI criHeap_LeaveCriticalSection(void);

/*==========================================================================*
 *		���̑��̊֐�
 *==========================================================================*/
/* For only internal use */
CriSint32 CRIAPI criHeap_SwitchAllocFunctions(CriHeap heap);

/*==========================================================================*
 *		�f�o�b�O�֘A
 *==========================================================================*/
/*JP
 * \brief �s�[�N�������T�C�Y�̎擾
 * \ingroup CRIHEAP_DEBUG
 * \param heap CRI Heap�n���h���B
 * \return �g�p���ꂽ�������̍ő�l�B
 * \par ����:
 * �ߋ��A�g�p���ꂽ�������ʂ̍ő�l��Ԃ��܂��B<br>
 * �ő�l��criHeap_DebugResetPeakMemorySize() �Ń��Z�b�g���邱�Ƃ��o���܂��B
 * \sa criHeap_DebugResetPeakMemorySize()
 */
/*EN
 * \brief Returns peak memory usage of the heap.
 * \ingroup CRIHEAP_DEBUG
 * \param heap The heap previously allocated by criHeap_Create().
 * \return The maximum number of bytes used by the heap.
 * \par Description:
 * The amount of memory used by these libraries is variable and depends
 * on the number of simultaneous streams being read, the seek and error
 * frequency within the stream being read, video resolution and other
 * factors.  This function allows you to tune the allocation of the heap
 * to achieve a required performance level while allocating minimal
 * heap space for this library.  To get accurate readings, this function
 * should typically be called just before criHeap_Destroy(), and after
 * exercising all the video and audio functions in your program.
 */
CriSint32 CRIAPI criHeap_DebugGetPeakMemorySize(CriHeap heap);

/*JP
 * \brief �s�[�N�������T�C�Y�̃��Z�b�g
 * \ingroup CRIHEAP_DEBUG
 * \param heap CRI Heap�n���h���B
 * \par ����:
 * �s�[�N�������T�C�Y�����Z�b�g���܂��B
 * \sa criHeap_DebugGetPeakMemorySize()
 */
/*EN
 * \brief Reset peak memory size
 * \ingroup CRIHEAP_DEBUG
 * \param heap CRI Heap handle
 * \par Description:
 * The peak memory size is reset.
 * \sa criHeap_DebugGetPeakMemorySize()
 */
void CRIAPI criHeap_DebugResetPeakMemorySize(CriHeap heap);

/*JP
 * \brief �������u���b�N���̎擾
 * \ingroup CRIHEAP_DEBUG
 * \param heap CRI Heap�n���h���B
 * \return �������u���b�N���B
 * \par ����:
 * �A���P�[�g���ꂽ�������̃u���b�N����Ԃ��܂��B<br>
 * CRI Heap�n���h����������͖��g�p�̃������u���b�N���P���݂�����
 * �ƂȂ�܂��B�܂��A�t���[���ꂽ�f�Љ����ꂽ�������u���b�N�����̐���
 * �܂݂܂��B
 * \sa criHeap_DebugGetUsedBlocks()
 */
/*EN
 * \brief Get number of allocated memory blocks
 * \ingroup CRIHEAP_DEBUG
 * \param heap CRI Heap handle
 * \return Number of allocated memory blocks
 * \par Description:
 * The number of allocated memory blocks is returned.<br>
 * One unused memory block exists right after CRI Heap handle creation.
 * The number of released fragmented memory blocks is also included.
 * \sa criHeap_DebugGetUsedBlocks()
 */
 CriSint32 CRIAPI criHeap_DebugGetNumBlocks(CriHeap heap);

/*JP
 * \brief �g�p�������u���b�N���̎擾
 * \ingroup CRIHEAP_DEBUG
 * \param heap CRI Heap�n���h���B
 * \return �g�p�������u���b�N���B
 * \par ����:
 * �A���P�[�g����Ă��郁�����u���b�N�̐���Ԃ��܂��B
 * \sa criHeap_DebugGetNumBlocks()
 */
/*EN
 * \brief Get number of used memory blocks
 * \ingroup CRIHEAP_DEBUG
 * \param heap CRI Heap
 * \return Number of used memory blocks
 * \par Description:
 * The number of used memory blocks is returned.<br>
 * \sa criHeap_DebugGetNumBlocks()
 */
CriSint32 CRIAPI criHeap_DebugGetUsedBlocks(CriHeap heap);

/*JP
 * \brief �������u���b�N���̎擾
 * \ingroup CRIHEAP_DEBUG
 * \param heap CRI Heap�n���h���B
 * \param ptr �A���P�[�g�����������̃|�C���^�B
 * \return �������u���b�N���B
 * \par ����:
 * �A���P�[�g���ɐݒ肵���������u���b�N���ւ̃|�C���^��Ԃ��܂��B<br>
 */
/*EN
 * \brief Get memory block name
 * \ingroup CRIHEAP_DEBUG
 * \param heap CRI Heap
 * \param ptr A pointer to the allocated memory block
 * \return Memory block name
 * \par Description:
 * The pointer to the memory block name specified on allocation is returned.<br>
 */
CriChar8 * CRIAPI criHeap_DebugGetBlockName(CriHeap heap, void *ptr);

/* �������u���b�N�ԍ��̎擾 */
//CriSint32 CRIAPI criHeap_DebugGetBlockNumberFromPointer(CriHeap heap, void *ptr);

/* �������u���b�N���̎擾 */
//void CRIAPI criHeap_DebugGetBlockInformation(CriHeap heap, 
//								CriSint32 blocknumber, CriHeapBlock *heapblock);

/* �������u���b�N���̕\�� */
//void CRIAPI criHeap_DebugPrintBlockInformation(CriHeap heap,
//								CriSint32 blocknumber, CriHeapBlock *heapblock);

/*JP
 * \brief �������u���b�N���̕\��
 * \ingroup CRIHEAP_DEBUG
 * \param heap CRI Heap�n���h���B
 * \par ����:
 * ���݂̃������u���b�N����\�����܂��B<br>
 * printf�֐��Ȃǂ̕W���o�͂ɕ\�����s���܂��B
 */
/*EN
 * \brief Print memory block information
 * \ingroup CRIHEAP_DEBUG
 * \param heap CRI Heap
 * \par Description:
 * The current memory block information is displayed.<br>
 * It is displayed on the standard output for the printf function.
 */
 void CRIAPI criHeap_DebugPrintBlockInformationAll(CriHeap heap);

/*JP
 * \brief �������A���P�[�g�T�C�Y�̎擾
 * \ingroup CRIHEAP_DEBUG
 * \param heap CRI Heap�n���h���B
 * \par ����:
 * ���݂̃������A���P�[�g�T�C�Y�̍��v�l��Ԃ��܂��B
 */
/*EN
 * \brief Get total allocation size
 * \ingroup CRIHEAP_DEBUG
 * \param heap CRI Heap
 * \par Description:
 * The total size of allocated memory blocks is returned.
 */
CriSint32 CRIAPI criHeap_DebugGetTotalAllocSize(CriHeap heap);

/* �t���[�T�C�Y�̎擾 */
//CriSint32 CRIAPI criHeap_DebugGetTotalFreeSize(CriHeap heap);

/* �������A���P�[�g�T�C�Y�̎擾(�^�C�v��) */
CriSint32 CRIAPI criHeap_DebugGetAllocSize(CriHeap heap, CriSint32 type);

/* �q�[�v�w�b�_�T�C�Y�̎擾 */
CriSint64 CRIAPI criHeap_DebugGetHandleHeaderSize(CriHeap heap);

/* �������u���b�N�w�b�_�T�C�Y�̎擾  ptr: Allocated Pointer */
CriSint64 CRIAPI criHeap_DebugGetMemBlockHeaderSize(void *ptr);

/* �ǉ��ŕK�v�ƂȂ�T�C�Y�̍ő�l */
CriSint32 CRIAPI criHeap_DebugGetWorstExtraSize(CriSint32 alignment);

/* criHeap_AllocFix�ŃA���P�[�g�\�ȃT�C�Y */
CriSint32 CRIAPI criHeap_DebugGetFixAllocatableSize(CriHeap heap, CriSint32 alignment);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef XPT_TGT_PC
#pragma pack(pop)
#endif

#endif // _CRI_HEAP_H_INCLUDED

/* end of file */
