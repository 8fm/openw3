/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef _SCE_GNM_TOOLKIT_DATAFORMAT_TESTS_H
#define _SCE_GNM_TOOLKIT_DATAFORMAT_TESTS_H

#include <gnm.h>
#include <gnmx.h>

namespace sce
{
	namespace Gnmx
	{
		namespace Toolkit
		{
			struct IAllocator;

			namespace DataFormatTests
			{
				/** @brief Calculates ONION and GARLIC memory requirements for DataFormatTests
					@param memoryRequests a pointer to a MemoryRequests object
				*/
				void addToMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests);

				/** @brief Initializes SurfaceFormatTests with a list of ONION and GARLIC memory requirements that has been fulfilled by the user.
					@param memoryRequests a pointer to a MemoryRequests object
					*/
				void initializeWithMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests);

				struct ReadResult
				{
					Reg32 m_bufferLoad[4];
					Reg32 m_textureLoad[4];
					Reg32 m_simulatorDecode[4];
				};

				/** @brief Builds a Buffer and Texture of the specified DataFormat, and tries to read from them in a compute shader, to see what level of support is provided by hardware for this DataFormat.
					@param result The results of the Buffer, Texture, and CPU-simulated DataFormat decode are stored here
					@param gfxc A graphics context which is used to kick off GPU work
					@param dataFormat The DataFormat to be tested
					@param allocator A memory allocator which is used to allocate memory for buffers and textures
					@param src Memory that has already been formatted in the desired dataFormat
				*/
				void read(ReadResult *result, Gnmx::GfxContext *gfxc, const Gnm::DataFormat dataFormat, Toolkit::IAllocator *allocator, uint32_t *src);

				struct WriteResult
				{
					uint32_t m_rasterizerExport[4];
					uint32_t m_bufferStore[4];
					uint32_t m_textureStore[4];
					uint32_t m_simulatorEncode[4];
				};

				/** @brief Builds a RenderTarget, Buffer and Texture of the specified DataFormat, and tries to write to them from pixel and compute shaders, to see what level of support is provided by hardware.
					@param result The results of the RenderTarget, Buffer, Texture, and CPU-simulated DataFormat encodes are stored here
					@param gfxc A graphics context which is used to kick off GPU work
					@param dataFormat The DataFormat to be tested
					@param allocator A memory allocator which is used to allocate memory for buffers and textures
					@param src simlulated shader VGPRs which already contain the data to be encoded into the DataFormat
				*/
				void write(WriteResult *result, Gnmx::GfxContext *gfxc, const Gnm::DataFormat dataFormat, Toolkit::IAllocator *allocator, Reg32 *src);
			}
		}
	}
}

#endif /* _SCE_GNM_TOOLKIT_DATAFORMAT_TESTS_H */
