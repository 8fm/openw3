/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApiUtils.h"

namespace GpuApi
{


	VertexLayoutDesc::VertexLayoutDesc( const VertexPacking::PackingElement* elements )
		: m_elementCount( 0 )
	{
		for ( size_t i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++i )
		{
			m_elements[i] = VertexPacking::PackingElement::END_OF_ELEMENTS;
		}

		AddElements( elements );
	}


	void VertexLayoutDesc::UpdateStrides()
	{
		Red::System::MemoryZero(m_slotStrides, sizeof(m_slotStrides));
		m_slotMask = 0;

		for ( size_t i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++i )
		{
			if ( !m_elements[i].IsEmpty() )
			{
				const Uint8 slotIndex = m_elements[i].m_slot;
				GPUAPI_ASSERT( m_slotStrides[slotIndex] <= 239, TXT( "Storing slot stride in a byte to save memory, so we can't exceed 255."));	// Largest attribute is 16 bytes, so make sure we're at no more than 239 before adding to avoid overflow.
				m_slotStrides[slotIndex] += VertexPacking::GetPackingTypeSize( m_elements[i].m_type );

				m_slotMask |= ( 1 << slotIndex );
			}
		}
	}

	Bool VertexLayoutDesc::AddElements( const VertexPacking::PackingElement* newElements, Uint8 slotOverride, VertexPacking::eSlotType slotTypeOverride )
	{
		GPUAPI_ASSERT( newElements, TXT( "NULL given for VertexLayoutDesc::AddElements" ) );
		if ( !newElements )
		{
			return false;
		}

		GPUAPI_ASSERT( slotOverride < GPUAPI_VERTEX_LAYOUT_MAX_SLOTS || slotOverride == invalidOverride, TXT( "Slot override out of range: %d" ), slotOverride );
		if ( slotOverride >= GPUAPI_VERTEX_LAYOUT_MAX_SLOTS && slotOverride != invalidOverride )
		{
			return false;
		}

		// Find first non-empty element.
		Uint32 addIndex = 0;
		while ( addIndex < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS && !m_elements[addIndex].IsEmpty() )
		{
			++addIndex;
		}

		// Count how many we're adding.
		Uint32 numToAdd = 0;
		while ( numToAdd < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS && !newElements[numToAdd].IsEmpty() )
		{
			++numToAdd;
		}

		// Check if we're going to overflow.
		GPUAPI_ASSERT( addIndex + numToAdd <= GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS, TXT( "Too many PackingElements in one VertexLayoutDesc!" ) );
		if ( addIndex + numToAdd > GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS )
		{
			return false;
		}

		for ( Uint32 i = 0; i < numToAdd; ++i )
		{
			m_elements[addIndex + i] = newElements[i];

			if ( slotOverride != invalidOverride )
			{
				m_elements[addIndex + i].m_slot = slotOverride;
				m_elements[addIndex + i].m_slotType = slotTypeOverride;
			}
			else
			{
				GPUAPI_ASSERT( m_elements[addIndex + i].m_slot < GPUAPI_VERTEX_LAYOUT_MAX_SLOTS, TXT( "Slot index out of range: %d. Defaulting to 0." ), m_elements[addIndex + i].m_slot );
				if ( m_elements[addIndex + i].m_slot >= GPUAPI_VERTEX_LAYOUT_MAX_SLOTS )
				{
					m_elements[addIndex + i].m_slot = 0;
				}
			}
		}

		UpdateStrides();

		m_elementCount = m_elementCount + numToAdd;

		return true;
	}


	Int32 VertexLayoutDesc::GetElementOffset( const VertexPacking::PackingElement& element ) const
	{
		Int32 offset = 0;
		for ( size_t i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++i )
		{
			if ( m_elements[i].IsEmpty() )
			{
				break;
			}

			if ( m_elements[i].m_slot == element.m_slot )
			{
				if ( m_elements[i] == element )
				{
					return offset;
				}

				offset += VertexPacking::GetPackingTypeSize( m_elements[i].m_type );
			}
		}

		return -1;
	}

	Int32 VertexLayoutDesc::GetUsageOffset( VertexPacking::ePackingUsage usage, Uint8 usageIndex ) const
	{
		// Not optimal, but it's simple and shouldn't really need to be used in speed-critical places.
		for ( size_t i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++i )
		{
			if ( m_elements[i].IsEmpty() )
			{
				break;
			}

			if ( m_elements[i].m_usage == usage && m_elements[i].m_usageIndex == usageIndex )
			{
				return GetElementOffset( m_elements[i] );
			}
		}

		return -1;
	}

	Int32 VertexLayoutDesc::FindHighestIndexForUsage( VertexPacking::ePackingUsage usage ) const
	{
		Int32 highestIndex = -1;
		for ( size_t i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++i )
		{
			if ( m_elements[i].IsEmpty() )
			{
				break;
			}

			if ( m_elements[i].m_usage == usage && m_elements[i].m_usageIndex > highestIndex )
			{
				highestIndex = m_elements[i].m_usageIndex;
			}
		}

		return highestIndex;
	}

}
