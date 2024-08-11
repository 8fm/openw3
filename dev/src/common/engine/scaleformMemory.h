/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#ifdef USE_SCALEFORM

//////////////////////////////////////////////////////////////////////////
// CScaleformSysAlloc
//////////////////////////////////////////////////////////////////////////
class CScaleformSysAlloc : public SF::SysAlloc
{
public:
	virtual void*	Alloc( SF::UPInt size, SF::UPInt align );
	virtual void	Free( void* ptr, SF::UPInt size, SF::UPInt align );
	virtual void*	Realloc( void* oldPtr, SF::UPInt oldSize, SF::UPInt newSize, SF::UPInt align );
};

//////////////////////////////////////////////////////////////////////////
// CScaleformSysAllocPaged
//////////////////////////////////////////////////////////////////////////
class CScaleformSysAllocPaged : public SF::SysAllocPaged
{
public:
	virtual void GetInfo(Info* i) const;

	virtual void* Alloc( SF::UPInt size, SF::UPInt align ) override;
	virtual SFBool Free( void* ptr, SF::UPInt size, SF::UPInt align ) override;

	// ReallocInPlace attempts to reallocate memory to a new size without moving it.
	// If such reallocation succeeds 'true' is returned, otherwise 'false' is returned and the
	// previous allocation remains unchanged. This function is provided as an optimization
	// for internal Realloc implementation for large blocks; it does not need to be implemented.
	virtual SFBool ReallocInPlace(void* oldPtr, SF::UPInt oldSize, SF::UPInt newSize, SF::UPInt align) override;
};

SF::SysAllocBase* CreateScaleformSysAlloc();
void DestroyScaleformSysAlloc();

#endif // USE_SCALEFORM
