/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef _SCE_GNM_TOOLKIT_EMBEDDED_SHADER_H
#define _SCE_GNM_TOOLKIT_EMBEDDED_SHADER_H

#include <gnmx/shaderbinary.h>
#include <gnm/gpumem.h>
#include <string.h>
#include <gnmx/shader_parser.h>
#include <algorithm>

namespace sce
{
	namespace Gnmx
	{
		namespace Toolkit
		{
			struct MemoryRequests;

			struct EmbeddedPsShader
			{
				const uint32_t *m_source;
				Gnmx::PsShader *m_shader;
				void addToMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests) const;
				void initializeWithMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests);

				SCE_GNM_API_DEPRECATED_NOINLINE(addToMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests))
				uint32_t addToRequiredSize(uint32_t size) const;

				SCE_GNM_API_DEPRECATED_NOINLINE(initializeWithMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests))
				void *initialize(void *addr);

			};

			struct EmbeddedCsShader
			{
				const uint32_t *m_source;
				Gnmx::CsShader *m_shader;
				void addToMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests) const;
				void initializeWithMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests);

				SCE_GNM_API_DEPRECATED_NOINLINE(addToMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests))
				uint32_t addToRequiredSize(uint32_t size) const;

				SCE_GNM_API_DEPRECATED_NOINLINE(initializeWithMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests))
				void *initialize(void *addr);

			};

			struct EmbeddedVsShader
			{
				const uint32_t *m_source;
				Gnmx::VsShader *m_shader;
				void *m_fetchShader;
				void addToMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests) const;
				void initializeWithMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests);

				SCE_GNM_API_DEPRECATED_NOINLINE(addToMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests))
				uint32_t addToRequiredSize(uint32_t size) const;

				SCE_GNM_API_DEPRECATED_NOINLINE(initializeWithMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests))
				void *initialize(void *addr);

			};

			struct EmbeddedEsShader
			{
				const uint32_t *m_source;
				Gnmx::EsShader *m_shader;
				void *m_fetchShader;
				void addToMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests) const;
				void initializeWithMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests);
			};

			struct EmbeddedGsShader
			{
				const uint32_t *m_source;
				Gnmx::GsShader *m_shader;
				void addToMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests) const;
				void initializeWithMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests);
			};

			struct EmbeddedLsShader
			{
				const uint32_t *m_source;
				Gnmx::LsShader *m_shader;
				void *m_fetchShader;
				void addToMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests) const;
				void initializeWithMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests);
			};

			struct EmbeddedHsShader
			{
				const uint32_t *m_source;
				Gnmx::HsShader *m_shader;
				void addToMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests) const;
				void initializeWithMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests);
			};

			struct EmbeddedCsVsShader
			{
				const uint32_t *m_source;
				Gnmx::CsVsShader *m_shader;
				void *m_fetchShaderVs;
				void *m_fetchShaderCs;
				void addToMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests) const;
				void initializeWithMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests);

				SCE_GNM_API_DEPRECATED_NOINLINE(addToMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests))
					uint32_t addToRequiredSize(uint32_t size) const;

				SCE_GNM_API_DEPRECATED_NOINLINE(initializeWithMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests))
					void *initialize(void *addr);
			};

			struct EmbeddedShaders
			{
				EmbeddedCsShader **m_embeddedCsShader;
				uint32_t           m_embeddedCsShaders;
				EmbeddedPsShader **m_embeddedPsShader;
				uint32_t           m_embeddedPsShaders;
				EmbeddedVsShader **m_embeddedVsShader;
				uint32_t           m_embeddedVsShaders;
				EmbeddedEsShader **m_embeddedEsShader;
				uint32_t           m_embeddedEsShaders;
				EmbeddedGsShader **m_embeddedGsShader;
				uint32_t           m_embeddedGsShaders;
				EmbeddedLsShader **m_embeddedLsShader;
				uint32_t           m_embeddedLsShaders;
				EmbeddedHsShader **m_embeddedHsShader;
				uint32_t           m_embeddedHsShaders;
				EmbeddedCsVsShader **m_embeddedCsVsShader;
				uint32_t           m_embeddedCsVsShaders;
				void addToMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests) const;
				void initializeWithMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests);

				SCE_GNM_API_DEPRECATED_NOINLINE(addToMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests))
				uint32_t addToRequiredSize(uint32_t size) const;

				SCE_GNM_API_DEPRECATED_NOINLINE(initializeWithMemoryRequests(Gnmx::Toolkit::MemoryRequests *memoryRequests))
				void *initialize(void *addr);

			};
		}
	}
}
#endif // _SCE_GNM_TOOLKIT_EMBEDDED_SHADER_H
