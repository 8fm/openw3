/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "memory_requests.h"
#include <string.h>
#include <algorithm>
using namespace sce;

uint64_t Gnmx::Toolkit::roundUpToAlignment(Gnm::Alignment alignment, uint64_t bytes)
{
	const uint64_t mask = alignment - 1;
	return (bytes + mask) & ~mask;
}

void *Gnmx::Toolkit::roundUpToAlignment(Gnm::Alignment alignment, void *addr)
{
	return reinterpret_cast<void*>(roundUpToAlignment(alignment, reinterpret_cast<uint64_t>(addr)));
}

void Gnmx::Toolkit::MemoryRequest::initialize()
{
	m_sizeAlign.m_size = 0;
	m_sizeAlign.m_align = 0;
}

void Gnmx::Toolkit::MemoryRequest::request(uint32_t size, Gnm::AlignmentType alignment)
{
	m_sizeAlign.m_align = std::max(m_sizeAlign.m_align, alignment);
	m_sizeAlign.m_size = roundUpToAlignment(static_cast<Gnm::Alignment>(alignment), m_sizeAlign.m_size);
	m_sizeAlign.m_size += size;
}

void *Gnmx::Toolkit::MemoryRequest::redeem(uint32_t size, Gnm::AlignmentType alignment)
{
	alignPointer(alignment);
	void *result = m_pointer;
	advancePointer(size);
	return result;
}

void Gnmx::Toolkit::MemoryRequest::fulfill(void* pointer)
{
	SCE_GNM_ASSERT((uintptr_t(pointer) & (m_sizeAlign.m_align-1)) == 0);
	m_begin = pointer;
	m_pointer = pointer;
	m_end = static_cast<uint8_t*>(pointer) + m_sizeAlign.m_size;
}

void Gnmx::Toolkit::MemoryRequest::validate()
{
	SCE_GNM_ASSERT(m_pointer >= m_begin && m_pointer <= m_end);
}

void Gnmx::Toolkit::MemoryRequest::alignPointer(Gnm::AlignmentType alignment)
{
	m_pointer = roundUpToAlignment(static_cast<Gnm::Alignment>(alignment), m_pointer);
	validate();
}

void Gnmx::Toolkit::MemoryRequest::advancePointer(uint32_t bytes)
{
	reinterpret_cast<uint8_t*&>(m_pointer) += bytes;
	validate();
}

void Gnmx::Toolkit::MemoryRequest::copyToPointerFrom(const void *src, uint32_t bytes)
{
	SCE_GNM_ASSERT(static_cast<uint8_t*>(m_pointer) + bytes <= m_end);
	memcpy(m_pointer, src, bytes);
}

void Gnmx::Toolkit::MemoryRequests::initialize()
{
	m_garlic.initialize();
	m_onion.initialize();
}
