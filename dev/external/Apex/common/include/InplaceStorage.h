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

#ifndef __APEX_INPLACE_STORAGE_H__
#define __APEX_INPLACE_STORAGE_H__

#include "PsShare.h"
#include "InplaceTypes.h"

namespace physx
{
namespace apex
{


class InplaceStorage;

class InplaceStorageGroup
{
	friend class InplaceStorage;

	InplaceStorage*			_storage;
	physx::PxU32			_lastBlockIndex;

	InplaceStorageGroup*	_groupListPrev;
	InplaceStorageGroup*	_groupListNext;

	PX_INLINE void reset(InplaceStorage* storage)
	{
		PX_UNUSED(storage);
		PX_ASSERT(_storage == storage);
		_storage = 0;
	}

public:
	PX_INLINE InplaceStorageGroup() : _storage(0) {}
	PX_INLINE InplaceStorageGroup(InplaceStorage& storage) : _storage(0)
	{
		init(storage);
	}
	PX_INLINE ~InplaceStorageGroup()
	{
		release();
	}

	PX_INLINE void init(InplaceStorage& storage);
	PX_INLINE void release();

	PX_INLINE void begin();
	PX_INLINE void end();

	PX_INLINE InplaceStorage& getStorage()
	{
		PX_ASSERT(_storage != 0);
		return *_storage;
	}
};

class InplaceStorage
{
	class Reflector
	{
		InplaceStorage* _storage;
		physx::PxU32 _blockIndex;
		physx::PxU8* _blockPtr;

		template <bool V>
		struct BoolSelector
		{
			enum { value = V };
		};

		template <typename T>
		void reflect(T& , BoolSelector<false>)
		{
			;//do nothing
		}

		template <typename T>
		void reflect(T& obj, BoolSelector<true>)
		{
			obj.reflect(*this);
		}

	public:
		Reflector(InplaceStorage* storage, physx::PxU32 blockIndex, physx::PxU8* blockPtr)
			: _storage(storage), _blockIndex(blockIndex), _blockPtr(blockPtr) {}

		template <typename T>
		void reflect(T& obj)
		{
			reflect(obj, BoolSelector< InplaceTypeTraits<T>::hasReflect >());
		}

		void reflect(InplaceHandleBase& handle, bool autoFree = false)
		{
			size_t offset = reinterpret_cast<physx::PxU8*>(&handle) - _blockPtr;
			_storage->addHandleRef(_blockIndex, offset, autoFree);
		}

		template <typename T>
		void reflect(InplaceHandle<T>& handle, bool autoFree = false)
		{
			reflect(static_cast<InplaceHandleBase&>(handle), autoFree);
		}
	};
	friend class Reflector;

	static const physx::PxU32 NULL_INDEX = InplaceHandleBase::NULL_VALUE;

	struct Block
	{
		physx::PxU32			_size;
		physx::PxU32			_alignment;
		physx::PxU32			_offset;
		physx::PxU32			_prevIndex;
		union
		{
			physx::PxU32		_nextIndex;
			physx::PxU32		_nextFreeBlockIndex;
		};
		physx::PxU32			_firstRefIndex;
		InplaceStorageGroup*	_group;

		void reset()
		{
			_alignment = 0;
			_size = 0;
			_offset = physx::PxU32(-1);
			_prevIndex = _nextIndex = NULL_INDEX;
			_firstRefIndex = NULL_INDEX;
			_group = NULL;
		}
	};

	struct HandleRef
	{
		enum Flags
		{
			AUTO_FREE = 0x01,
		};
		physx::PxU32 flags;
		physx::PxU32 ownerBlockIndex;
		physx::PxU32 offsetInBlock;
		union
		{
			physx::PxU32 nextIndex;
			physx::PxU32 nextFreeRefIndex;
		};

		void reset()
		{
			flags = 0;
			ownerBlockIndex = NULL_INDEX;
			offsetInBlock = 0;
		}
	};

	void addHandleRef(physx::PxU32 blockIndex, size_t offset, bool autoFree)
	{
		//find free handleRef
		if (_firstFreeRefIndex == NULL_INDEX)
		{
			_firstFreeRefIndex = _handleRefs.size();
			_handleRefs.resize(_firstFreeRefIndex + 1);

			_handleRefs.back().nextFreeRefIndex = NULL_INDEX;
		}
		physx::PxU32 thisRefIndex = _firstFreeRefIndex;
		HandleRef& handleRef = _handleRefs[thisRefIndex];
		_firstFreeRefIndex = handleRef.nextFreeRefIndex;

		Block& block = _blocks[blockIndex];
		handleRef.nextIndex = block._firstRefIndex;
		block._firstRefIndex = thisRefIndex;

		handleRef.ownerBlockIndex = blockIndex;
		handleRef.offsetInBlock = (physx::PxU32) offset;
		handleRef.flags = 0;
		if (autoFree)
		{
			handleRef.flags |= HandleRef::AUTO_FREE;
		}
	}

	template <typename F>
	void removeHandleRefs(F func, physx::PxU32 blockIndex, physx::PxU32 minOffset = 0)
	{
		Block& block = _blocks[blockIndex];

		physx::PxU32 prevRefIndex = NULL_INDEX;
		physx::PxU32 currRefIndex = block._firstRefIndex;
		while (currRefIndex != NULL_INDEX)
		{
			HandleRef& handleRef = _handleRefs[currRefIndex];
			PX_ASSERT(handleRef.ownerBlockIndex == blockIndex);

			physx::PxU32 nextRefIndex = handleRef.nextIndex;
			if (handleRef.offsetInBlock >= minOffset)
			{
				//remove
				if (handleRef.flags & HandleRef::AUTO_FREE)
				{
					physx::PxU32 blockOffset = block._offset;
					PX_ASSERT(blockOffset != physx::PxU32(-1));
					InplaceHandleBase handle = *reinterpret_cast<InplaceHandleBase*>(getBufferPtr() + blockOffset + handleRef.offsetInBlock);

					(this->*func)(block, handle);
				}

				if (prevRefIndex != NULL_INDEX)
				{
					_handleRefs[prevRefIndex].nextIndex = nextRefIndex;
				}
				else
				{
					block._firstRefIndex = nextRefIndex;
				}

				handleRef.nextFreeRefIndex = _firstFreeRefIndex;
				_firstFreeRefIndex = currRefIndex;

				handleRef.reset();
			}
			else
			{
				prevRefIndex = currRefIndex;
			}
			currRefIndex = nextRefIndex;
		}
	}


	PX_INLINE void mapHandle(InplaceHandleBase& handle) const
	{
		if (handle._value != NULL_INDEX)
		{
			handle._value = _blocks[handle._value]._offset;
		}
	}
	PX_INLINE physx::PxU8* getBufferPtr()
	{
		PX_ASSERT(_bufferPtr != 0);
		_isChanged = true;
		return _bufferPtr;
	}
	PX_INLINE const physx::PxU8* getBufferPtr() const
	{
		PX_ASSERT(_bufferPtr != 0);
		return _bufferPtr;
	}

protected:
	//buffer API
	virtual physx::PxU8* storageResizeBuffer(physx::PxU32 newSize) = 0;

	virtual void storageLock() {}
	virtual void storageUnlock() {}

public:
	InplaceStorage()
	{
		_bufferPtr = 0;
		_isChanged = false;

		_firstFreeBlockIndex = NULL_INDEX;
		_lastAllocatedBlockIndex = NULL_INDEX;
		_allocatedSize = 0;

		_groupListHead = 0;
		_activeGroup = NULL;

		_firstFreeRefIndex = NULL_INDEX;
	}
	virtual ~InplaceStorage()
	{
		release();
	}

	void release()
	{
		releaseGroups();
	}

	bool isChanged() const
	{
		return _isChanged;
	}
	void setUnchanged()
	{
		_isChanged = false;
	}

	template <typename T>
	PX_INLINE T* resolveAndCastTo(InplaceHandleBase handle)
	{
		if (handle._value != NULL_INDEX)
		{
			const Block& block = _blocks[handle._value];
			PX_ASSERT(block._offset != physx::PxU32(-1));
			return reinterpret_cast<T*>(getBufferPtr() + block._offset);
		}
		return 0;
	}

	template <typename T>
	PX_INLINE T* resolve(InplaceHandle<T> handle)
	{
		return resolveAndCastTo<T>(handle);
	}

	template <typename T>
	PX_INLINE T* alloc(InplaceHandleBase& handle, physx::PxU32 count = 1)
	{
		PX_ASSERT(count > 0);
		handle._value = allocBlock(sizeof(T) * count, __alignof(T));
		if (handle._value != NULL_INDEX)
		{
			return reflectElems<T>(handle, count);
		}
		return 0;
	}

	template <typename T>
	PX_INLINE T* alloc(InplaceHandle<T>& handle, physx::PxU32 count = 1)
	{
		return alloc<T>(static_cast<InplaceHandleBase&>(handle), count);
	}

	PX_INLINE void free(InplaceHandleBase& handle)
	{
		if (handle._value != NULL_INDEX)
		{
			freeBlock(handle._value);
			handle._value = NULL_INDEX;
		}
	}

	template <typename T>
	PX_INLINE void free(InplaceHandle<T>& handle)
	{
		free(static_cast<InplaceHandleBase&>(handle));
	}

	template <typename T>
	bool realloc(InplaceHandle<T>& handle, physx::PxU32 oldCount, physx::PxU32 newCount)
	{
		if (handle._value != NULL_INDEX)
		{
			PX_ASSERT(oldCount > 0);
			if (oldCount != newCount)
			{
				if (newCount > 0)
				{
					if (resizeBlock(handle._value, sizeof(T) * newCount))
					{
						if (newCount > oldCount)
						{
							reflectElems<T>(handle, newCount, oldCount);
						}
						return true;
					}
					return false;
				}
				free(handle);
			}
		}
		else
		{
			PX_ASSERT(oldCount == 0);
			if (newCount > 0)
			{
				return (alloc(handle, newCount) != 0);
			}
		}
		return true;
	}


	template <typename T>
	PX_INLINE InplaceHandle<T> mappedHandle(InplaceHandle<T> handle) const
	{
		mapHandle(handle);
		return handle;
	}

	physx::PxU32 mapTo(physx::PxU8* destPtr) const
	{
		PX_ASSERT(_lastAllocatedBlockIndex == NULL_INDEX || _blocks[_lastAllocatedBlockIndex]._offset + _blocks[_lastAllocatedBlockIndex]._size == _allocatedSize);

		memcpy(destPtr, getBufferPtr(), _allocatedSize);

		//iterate all blocks
		for (physx::PxU32 blockIndex = _lastAllocatedBlockIndex; blockIndex != NULL_INDEX; blockIndex = _blocks[blockIndex]._prevIndex)
		{
			const Block& block = _blocks[blockIndex];
			//iterate all refs in current block
			for (physx::PxU32 refIndex = block._firstRefIndex; refIndex != NULL_INDEX; refIndex = _handleRefs[refIndex].nextIndex)
			{
				const HandleRef& handleRef = _handleRefs[refIndex];
				PX_ASSERT(handleRef.ownerBlockIndex == blockIndex);

				physx::PxU32 blockOffset = block._offset;
				PX_ASSERT(blockOffset != physx::PxU32(-1));
				InplaceHandleBase& handle = *reinterpret_cast<InplaceHandleBase*>(destPtr + blockOffset + handleRef.offsetInBlock);

				mapHandle(handle);
			}
		}
		return _allocatedSize;
	}


private:
	template <typename T>
	T* reflectElems(InplaceHandleBase handle, physx::PxU32 newCount, physx::PxU32 oldCount = 0)
	{
		const Block& block = _blocks[handle._value];

		physx::PxU8* ptr = (getBufferPtr() + block._offset);
		T* ptrT0 = reinterpret_cast<T*>(ptr);
		T* ptrT = ptrT0 + oldCount;
		Reflector r(this, handle._value, ptr);
		for (physx::PxU32 index = oldCount; index < newCount; ++index)
		{
			::new(ptrT) T;
			r.reflect(*ptrT);
			++ptrT;
		}
		return ptrT0;
	}

	static PX_INLINE physx::PxU32 alignUp(physx::PxU32 size, physx::PxU32 alignment)
	{
		PX_ASSERT(alignment > 0);
		return (size + (alignment - 1)) & ~(alignment - 1);
	}

	PX_INLINE physx::PxI32 getMoveDelta(physx::PxU32 blockIndex, physx::PxU32 moveOffset) const
	{
		PX_ASSERT(blockIndex != NULL_INDEX);
		const physx::PxU32 currOffset = _blocks[blockIndex]._offset;

		//calculate max alignment for all subsequent blocks
		physx::PxU32 alignment = 0;
		do
		{
			const Block& block = _blocks[blockIndex];
			alignment = physx::PxMax(alignment, block._alignment);

			blockIndex = block._nextIndex;
		}
		while (blockIndex != NULL_INDEX);

		physx::PxI32 moveDelta = moveOffset - currOffset;
		//align moveDelta
		if (moveDelta >= 0)
		{
			moveDelta += (alignment - 1);
			moveDelta &= ~(alignment - 1);
		}
		else
		{
			moveDelta = -moveDelta;
			moveDelta &= ~(alignment - 1);
			moveDelta = -moveDelta;
		}
		PX_ASSERT(currOffset + moveDelta >= moveOffset);
		return moveDelta;
	}

	physx::PxU32 getPrevAllocatedSize(physx::PxU32 prevBlockIndex) const
	{
		physx::PxU32 prevAllocatedSize = 0;
		if (prevBlockIndex != NULL_INDEX)
		{
			const Block& prevBlock = _blocks[prevBlockIndex];
			prevAllocatedSize = prevBlock._offset + prevBlock._size;
		}
		return prevAllocatedSize;
	}

	void moveBlocks(physx::PxU32 moveBlockIndex, physx::PxI32 moveDelta)
	{
		if (moveDelta != 0)
		{
			const physx::PxU32 currOffset = _blocks[moveBlockIndex]._offset;
			const physx::PxU32 moveOffset = currOffset + moveDelta;

			physx::PxU32 moveSize = _allocatedSize - currOffset;
			physx::PxU8* moveFromPtr = getBufferPtr() + currOffset;
			physx::PxU8* moveToPtr = getBufferPtr() + moveOffset;
			memmove(moveToPtr, moveFromPtr, moveSize);

			_allocatedSize += moveDelta;
			//update moved blocks
			do
			{
				Block& moveBlock = _blocks[moveBlockIndex];
				moveBlock._offset += moveDelta;
				PX_ASSERT((moveBlock._offset & (moveBlock._alignment - 1)) == 0);

				moveBlockIndex = moveBlock._nextIndex;
			}
			while (moveBlockIndex != NULL_INDEX);
		}
	}

	void removeBlocks(physx::PxU32 prevBlockIndex, physx::PxU32 nextBlockIndex, physx::PxU32 lastBlockIndex)
	{
		PX_UNUSED(lastBlockIndex);

		physx::PxU32 prevAllocatedSize = getPrevAllocatedSize(prevBlockIndex);
		if (prevBlockIndex != NULL_INDEX)
		{
			_blocks[prevBlockIndex]._nextIndex = nextBlockIndex;
		}
		if (nextBlockIndex != NULL_INDEX)
		{
			_blocks[nextBlockIndex]._prevIndex = prevBlockIndex;

			const physx::PxI32 moveDelta = getMoveDelta(nextBlockIndex, prevAllocatedSize);
			moveBlocks(nextBlockIndex, moveDelta);
		}
		else
		{
			//last block
			PX_ASSERT(lastBlockIndex == _lastAllocatedBlockIndex);
			_lastAllocatedBlockIndex = prevBlockIndex;

			_allocatedSize = prevAllocatedSize;
		}
	}

	bool resizeBuffer(physx::PxU32 newAllocatedSize)
	{
		if (newAllocatedSize <= _allocatedSize)
		{
			return true;
		}
		physx::PxU8* newBufferPtr = storageResizeBuffer(newAllocatedSize);
		if (newBufferPtr == 0)
		{
			PX_ASSERT(0 && "Out of memory!");
			return false;
		}
		_bufferPtr = newBufferPtr;
		return true;
	}

	physx::PxU32 allocBlock(physx::PxU32 size, physx::PxU32 alignment)
	{
		physx::PxU32 insertBlockIndex;
		physx::PxU32 offset;
		physx::PxI32 moveDelta;
		physx::PxU32 newAllocatedSize;

		PX_ASSERT(_activeGroup != NULL);
		if (_activeGroup->_lastBlockIndex == NULL_INDEX || _activeGroup->_lastBlockIndex == _lastAllocatedBlockIndex)
		{
			//push_back new block
			insertBlockIndex = NULL_INDEX;
			offset = alignUp(_allocatedSize, alignment);
			moveDelta = 0;
			newAllocatedSize = offset + size;
		}
		else
		{
			//insert new block
			insertBlockIndex = _blocks[_activeGroup->_lastBlockIndex]._nextIndex;
			PX_ASSERT(insertBlockIndex != NULL_INDEX);

			physx::PxU32 prevAllocatedSize = getPrevAllocatedSize(_blocks[insertBlockIndex]._prevIndex);
			offset = alignUp(prevAllocatedSize, alignment);
			const physx::PxU32 moveOffset = offset + size;
			moveDelta = getMoveDelta(insertBlockIndex, moveOffset);
			newAllocatedSize = _allocatedSize + moveDelta;
		}

		if (resizeBuffer(newAllocatedSize) == false)
		{
			return NULL_INDEX;
		}

		//find free block
		if (_firstFreeBlockIndex == NULL_INDEX)
		{
			_firstFreeBlockIndex = _blocks.size();
			_blocks.resize(_firstFreeBlockIndex + 1);

			_blocks.back()._nextFreeBlockIndex = NULL_INDEX;
		}
		physx::PxU32 blockIndex = _firstFreeBlockIndex;
		Block& block = _blocks[blockIndex];
		_firstFreeBlockIndex = block._nextFreeBlockIndex;

		//init block
		block._size = size;
		block._alignment = alignment;
		block._offset = offset;
		block._firstRefIndex = NULL_INDEX;
		block._group = _activeGroup;

		PX_ASSERT((block._offset & (block._alignment - 1)) == 0);

		if (insertBlockIndex == NULL_INDEX)
		{
			//add new block after the _lastAllocatedBlockIndex
			block._prevIndex = _lastAllocatedBlockIndex;
			block._nextIndex = NULL_INDEX;

			if (_lastAllocatedBlockIndex != NULL_INDEX)
			{
				PX_ASSERT(_blocks[_lastAllocatedBlockIndex]._nextIndex == NULL_INDEX);
				_blocks[_lastAllocatedBlockIndex]._nextIndex = blockIndex;
			}
			_lastAllocatedBlockIndex = blockIndex;
		}
		else
		{
			PX_ASSERT(_activeGroup->_lastBlockIndex != NULL_INDEX);
			//insert new block before the insertBlockIndex
			block._prevIndex = _activeGroup->_lastBlockIndex;
			_blocks[_activeGroup->_lastBlockIndex]._nextIndex = blockIndex;

			block._nextIndex = insertBlockIndex;
			_blocks[insertBlockIndex]._prevIndex = blockIndex;

			moveBlocks(insertBlockIndex, moveDelta);
			PX_ASSERT(_allocatedSize == newAllocatedSize);
		}
		_allocatedSize = newAllocatedSize;

		//update group
		_activeGroup->_lastBlockIndex = blockIndex;

		return blockIndex;
	}


	PX_INLINE void onRemoveHandle(const Block& block, InplaceHandleBase handle)
	{
		PX_UNUSED(block);
		if (handle._value != InplaceHandleBase::NULL_VALUE)
		{
			PX_ASSERT(handle._value < _blocks.size());
			PX_ASSERT(_blocks[handle._value]._group == block._group);

			freeBlock(handle._value);
		}
	}
	PX_INLINE void onRemoveHandleEmpty(const Block& , InplaceHandleBase )
	{
	}

	void freeBlock(physx::PxU32 blockIndex)
	{
		PX_ASSERT(blockIndex != NULL_INDEX);
		PX_ASSERT(_activeGroup != NULL);
		PX_ASSERT(_blocks[blockIndex]._group == _activeGroup);

		removeHandleRefs(&InplaceStorage::onRemoveHandle, blockIndex);

		Block& block = _blocks[blockIndex];

		removeBlocks(block._prevIndex, block._nextIndex, blockIndex);

		//update group
		if (_activeGroup->_lastBlockIndex == blockIndex)
		{
			_activeGroup->_lastBlockIndex =
			    (block._prevIndex != NULL_INDEX) &&
			    (_blocks[block._prevIndex]._group == _activeGroup) ? block._prevIndex : NULL_INDEX;
		}

		block.reset();
		//add block to free list
		block._nextFreeBlockIndex = _firstFreeBlockIndex;
		_firstFreeBlockIndex = blockIndex;
	}

	bool resizeBlock(physx::PxU32 blockIndex, physx::PxU32 newSize)
	{
		if (newSize < _blocks[blockIndex]._size)
		{
			//remove refs
			removeHandleRefs(&InplaceStorage::onRemoveHandle, blockIndex, newSize);
		}

		Block& block = _blocks[blockIndex];
		const physx::PxU32 nextBlockIndex = block._nextIndex;

		physx::PxU32 newAllocatedSize = block._offset + newSize;
		physx::PxI32 moveDelta = 0;
		if (nextBlockIndex != NULL_INDEX)
		{
			moveDelta = getMoveDelta(nextBlockIndex, newAllocatedSize);
			newAllocatedSize = _allocatedSize + moveDelta;
		}

		if (resizeBuffer(newAllocatedSize) == false)
		{
			return false;
		}

		block._size = newSize;

		if (nextBlockIndex != NULL_INDEX)
		{
			moveBlocks(nextBlockIndex, moveDelta);
			PX_ASSERT(_allocatedSize == newAllocatedSize);
		}
		_allocatedSize = newAllocatedSize;

		return true;
	}

	void groupInit(InplaceStorageGroup* group)
	{
		storageLock();

		//init new group
		group->_lastBlockIndex = NULL_INDEX;
		group->_groupListPrev = 0;
		group->_groupListNext = _groupListHead;
		if (_groupListHead != NULL)
		{
			_groupListHead->_groupListPrev = group;
		}
		_groupListHead = group;

		storageUnlock();
	}

	void groupFree(InplaceStorageGroup* group)
	{
		storageLock();

		if (group->_lastBlockIndex != NULL_INDEX)
		{
			physx::PxU32 prevBlockIndex = group->_lastBlockIndex;
			physx::PxU32 nextBlockIndex = _blocks[group->_lastBlockIndex]._nextIndex;
			do
			{
				physx::PxU32 freeBlockIndex = prevBlockIndex;
				Block& freeBlock = _blocks[freeBlockIndex];
				prevBlockIndex = freeBlock._prevIndex;

				removeHandleRefs(&InplaceStorage::onRemoveHandleEmpty, freeBlockIndex);

				freeBlock.reset();
				//add block to free list
				freeBlock._nextFreeBlockIndex = _firstFreeBlockIndex;
				_firstFreeBlockIndex = freeBlockIndex;
			}
			while (prevBlockIndex != NULL_INDEX && _blocks[prevBlockIndex]._group == group);

			PX_ASSERT(prevBlockIndex == NULL_INDEX || _blocks[prevBlockIndex]._group != group);
			PX_ASSERT(nextBlockIndex == NULL_INDEX || _blocks[nextBlockIndex]._group != group);

			removeBlocks(prevBlockIndex, nextBlockIndex, group->_lastBlockIndex);
		}

		//remove from GroupList
		if (group->_groupListNext != 0)
		{
			group->_groupListNext->_groupListPrev = group->_groupListPrev;
		}
		if (group->_groupListPrev != 0)
		{
			group->_groupListPrev->_groupListNext = group->_groupListNext;
		}
		else
		{
			PX_ASSERT(_groupListHead == group);
			_groupListHead = group->_groupListNext;
		}

		storageUnlock();
	}

	void groupBegin(InplaceStorageGroup* group)
	{
		storageLock();

		PX_ASSERT(_activeGroup == NULL);
		_activeGroup = group;
	}

	void groupEnd(InplaceStorageGroup* group)
	{
		PX_UNUSED(group);
		PX_ASSERT(group == _activeGroup);
		_activeGroup = NULL;

		storageUnlock();
	}

	void releaseGroups()
	{
		storageLock();

		while (_groupListHead != 0)
		{
			InplaceStorageGroup* group = _groupListHead;
			_groupListHead = _groupListHead->_groupListNext;

			group->reset(this);
		}

		storageUnlock();
	}

	physx::PxU8*                _bufferPtr;
	bool                        _isChanged;

	physx::PxU32                _allocatedSize;

	physx::PxU32                _firstFreeBlockIndex;
	physx::PxU32                _lastAllocatedBlockIndex;
	physx::Array<Block>         _blocks;

	physx::PxU32                _firstFreeRefIndex;
	physx::Array<HandleRef>     _handleRefs;

	InplaceStorageGroup*        _groupListHead;
	InplaceStorageGroup*        _activeGroup;

	friend class InplaceStorageGroup;
};

PX_INLINE void InplaceStorageGroup::init(InplaceStorage& storage)
{
	PX_ASSERT(_storage == 0);
	_storage = &storage;
	getStorage().groupInit(this);
}
PX_INLINE void InplaceStorageGroup::release()
{
	if (_storage != 0)
	{
		getStorage().groupFree(this);
		_storage = 0;
	}
}
PX_INLINE void InplaceStorageGroup::begin()
{
	getStorage().groupBegin(this);
}
PX_INLINE void InplaceStorageGroup::end()
{
	getStorage().groupEnd(this);
}

class InplaceStorageGroupScope
{
private:
	InplaceStorageGroupScope& operator=(const InplaceStorageGroupScope&);
	InplaceStorageGroup& _group;

public:
	InplaceStorageGroupScope(InplaceStorageGroup& group) : _group(group)
	{
		_group.begin();
	}
	~InplaceStorageGroupScope()
	{
		_group.end();
	}
};

#define INPLACE_STORAGE_GROUP_SCOPE(group) InplaceStorageGroupScope scopeAccess_##group ( group ); InplaceStorage& _storage_ = group.getStorage();

////
class ApexCpuInplaceStorage : public InplaceStorage
{
public:
	ApexCpuInplaceStorage(physx::PxU32 allocStep = 4096)
		: mAllocStep(allocStep)
	{
		mSize = 0;
		mStoragePtr = 0;
	}
	~ApexCpuInplaceStorage()
	{
		release();
	}

	void release()
	{
		if (mStoragePtr)
		{
			PX_FREE(mStoragePtr);
			mSize = 0;
			mStoragePtr = 0;
		}
	}

protected:
	//interface for InplaceStorage
	physx::PxU8* storageResizeBuffer(physx::PxU32 newSize)
	{
		if (newSize > mSize)
		{
			newSize = ((newSize + mAllocStep - 1) / mAllocStep) * mAllocStep;
			PX_ASSERT(newSize > mSize && (newSize % mAllocStep) == 0);
			physx::PxU8* newStoragePtr = (physx::PxU8*)PX_ALLOC(newSize, PX_DEBUG_EXP("ApexCpuInplaceStorage"));
			if (!newStoragePtr)
			{
				return 0;
			}
			memcpy(newStoragePtr, mStoragePtr, mSize);
			PX_FREE(mStoragePtr);
			mSize = newSize;
			mStoragePtr = newStoragePtr;
		}
		return mStoragePtr;
	}

private:
	physx::PxU32	mAllocStep;
	physx::PxU32	mSize;
	physx::PxU8*	mStoragePtr;
};



}
} // end namespace physx::apex

#endif // __APEX_INPLACE_STORAGE_H__
