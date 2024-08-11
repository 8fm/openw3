#pragma once

/// Memory buffer for sector prefetch data
/// TEMPORARY SOLUTION FOR W3 ONLY
class CSectorPrefetchMemoryBuffer
{
public:
	CSectorPrefetchMemoryBuffer(); // size is taken from config
	~CSectorPrefetchMemoryBuffer();

	/// Get size of the buffer
	RED_FORCE_INLINE const Uint32 GetSize() const { return m_size; }

	/// Get memory for the buffer
	RED_FORCE_INLINE void* GetMemory() const { return m_memory; }

private:
	Uint32			m_size;
	void*			m_memory;
};
