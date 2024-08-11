/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//---------------------------------------------------------------------------

// Specialized mask buffer that allows to filter small portion of objects from a large group
// based on their IDs.

//---------------------------------------------------------------------------

class CTriggerBitMask
{
protected:
	// Bitmap
	Uint32* m_mask;

	// Size of the allocated memory (in bytes)
	Uint32 m_memorySize;

public:
	//! Is the bit set ?
	RED_INLINE const Bool IsSet(const Uint32 index) const
	{
		return 0 != (m_mask[index >> 5] & (1 << (index & 31)));
	}

	//! Set the bit
	RED_INLINE void Set(const Uint32 index)
	{
		m_mask[index >> 5] |= (1 << (index & 31));
	}

	//! Clear the bit
	RED_INLINE void Clear(const Uint32 index)
	{
		m_mask[index >> 5] &= ~(1 << (index & 31));
	}

public:
	CTriggerBitMask();
	~CTriggerBitMask();

	// Clear the memory
	void Reset();

	// Change the capacity of the buffer (in bits)
	void Resize(const Uint32 size);
};

class CTriggerMaskBuffer
{
protected:
	// Buffer capacity
	Uint32 m_capacity;

	// Was the object tested (on/off)
	CTriggerBitMask m_testedFlags;

	// Did we determine that the activator is inside for given object ?
	CTriggerBitMask m_insideFlags;

	// Reported entry event
	CTriggerBitMask m_enterFlags;

	// Reported exit event
	CTriggerBitMask m_exitFlags;

	// General ignore flags
	CTriggerBitMask m_ignoreFlags;

	// Collected object IDs
	TDynArray<Uint32> m_testedObjects;

public:
	//! Get general capacity
	RED_INLINE const Uint32 GetCapacity() const { return m_capacity; }

	//! Was the given trigger entered ?
	RED_INLINE const Bool WasEntered(const Uint32 index) const
	{
		return m_enterFlags.IsSet(index);
	}

	//! Was the given trigger exited ?
	RED_INLINE const Bool WasExited(const Uint32 index) const
	{
		return m_exitFlags.IsSet(index);
	}

	//! Is the trigger activated ?
	RED_INLINE const Bool IsInside(const Uint32 index) const
	{
		return m_insideFlags.IsSet(index);
	}

	//! Set the inside flag for given object
	RED_INLINE void SetInsideFlag(const Uint32 index)
	{
		m_insideFlags.Set(index);
	}

	//! Clear the inside flag for given object
	RED_INLINE void ClearInsideFlag(const Uint32 index)
	{
		m_insideFlags.Clear(index);
	}

	//! Is given shape on the ignore list
	RED_INLINE const Bool IsIgnored(const Uint32 index)
	{
		return m_ignoreFlags.IsSet(index);
	}

	//! Is this object tested ?
	RED_INLINE const Bool IsTested( const Uint32 index )
	{
		return m_testedFlags.IsSet( index );
	}

	//! Add to the ignore list
	RED_INLINE void SetIgnoredFlag(const Uint32 index)
	{
		m_ignoreFlags.Set(index);
	}

	//! Remove the reported exit from object
	RED_INLINE void ClearExitFlag( const Uint32 index )
	{
		m_exitFlags.Clear( index );
	}

	//! Get number of tested objects
	RED_INLINE const Uint32 GetNumTestedObjects() const
	{
		return m_testedObjects.Size();
	}

	//! Get n-th tested object
	RED_INLINE const Uint32 GetTestedObject(const Uint32 index) const
	{
		return m_testedObjects[index];
	}

	//! Mark object as tested and add to the list
	RED_INLINE void ReportAsTested(const Uint32 index)
	{
		if (!m_testedFlags.IsSet(index))
		{
			m_testedObjects.PushBack(index);
			m_testedFlags.Set(index);
		}
	}

	//! Mask object as entered by activator
	RED_INLINE void ReportEntry(const Uint32 index)
	{
		m_enterFlags.Set(index);
	}

	//! Mask object as exited by activator
	RED_INLINE void ReportExit(const Uint32 index)
	{
		m_exitFlags.Set(index);
	}

public:
	CTriggerMaskBuffer(const Uint32 initialSize);
	~CTriggerMaskBuffer();

	//! Resize buffer to custom size
	void Resize(const Uint32 newSize);

	//! Reset the mask
	void Reset();
};