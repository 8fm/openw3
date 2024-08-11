/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef _SCE_GNM_TOOLKIT_SHADER_LOADER_H
#define _SCE_GNM_TOOLKIT_SHADER_LOADER_H

#include <gnmx.h>
#include <string>
#include <shader/binary.h>
#include "allocator.h"

namespace sce
{
	namespace Gnmx
	{
		namespace Toolkit
		{
			class MemBlock;

			Gnm::SizeAlign calculateMemoryRequiredForVsFetchShader(const void *pointer);
			Gnm::SizeAlign calculateMemoryRequiredForCsVsFetchShaderVs(const void *pointer);
			Gnm::SizeAlign calculateMemoryRequiredForCsVsFetchShaderCs(const void *pointer);

			Gnm::SizeAlign calculateMemoryRequiredForVsShader(const void *pointer);
			Gnm::SizeAlign calculateMemoryRequiredForPsShader(const void *pointer);
			Gnm::SizeAlign calculateMemoryRequiredForCsShader(const void *pointer);
			Gnm::SizeAlign calculateMemoryRequiredForLsShader(const void *pointer);
			Gnm::SizeAlign calculateMemoryRequiredForHsShader(const void *pointer);
			Gnm::SizeAlign calculateMemoryRequiredForEsShader(const void *pointer);
			Gnm::SizeAlign calculateMemoryRequiredForGsShader(const void *pointer);
			Gnm::SizeAlign calculateMemoryRequiredForCsVsShader(const void *pointer);

			sce::Gnmx::VsShader *parseVsShader(void *dest, const void *src);
			sce::Gnmx::PsShader *parsePsShader(void *dest, const void *src);
			sce::Gnmx::CsShader *parseCsShader(void *dest, const void *src);
			sce::Gnmx::LsShader *parseLsShader(void *dest, const void *src);
			sce::Gnmx::HsShader *parseHsShader(void *dest, const void *src);
			sce::Gnmx::EsShader *parseEsShader(void *dest, const void *src);
			sce::Gnmx::GsShader *parseGsShader(void *dest, const void *src);
			sce::Gnmx::CsVsShader *parseCsVsShader(void *dest, const void *src);


			////////////////////////////////////////////////////////////////////////////////
			//PSSL LINKAGE (with metadata)
			sce::Gnmx::VsShader *loadAndAllocateVertexProgram  (sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, sce::Gnmx::Toolkit::IAllocator *allocator);
			sce::Gnmx::PsShader *loadAndAllocatePixelProgram   (sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, sce::Gnmx::Toolkit::IAllocator *allocator);
			sce::Gnmx::CsShader *loadAndAllocateComputeProgram (sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, sce::Gnmx::Toolkit::IAllocator *allocator);
			sce::Gnmx::HsShader *loadAndAllocateHullProgram    (sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, sce::Gnmx::Toolkit::IAllocator *allocator);
			sce::Gnmx::EsShader *loadAndAllocateExportProgram  (sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, sce::Gnmx::Toolkit::IAllocator *allocator);
			sce::Gnmx::LsShader *loadAndAllocateLocalProgram   (sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, sce::Gnmx::Toolkit::IAllocator *allocator);
			sce::Gnmx::GsShader *loadAndAllocateGeometryProgram(sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, sce::Gnmx::Toolkit::IAllocator *allocator);
			sce::Gnmx::CsVsShader *loadAndAllocateComputeVertexProgram(sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, sce::Gnmx::Toolkit::IAllocator *allocator);
			
			void releaseShaderProgram(sce::Shader::Binary::Program *sbp, sce::Gnmx::Toolkit::IAllocator *allocator);

			void saveShaderProgramToFile(sce::Shader::Binary::Program *sbp, const char *sbFilename);

			// runtime helper class for shader binary element search
			class ElementNode
			{
			public:
				std::string						m_fullNameStr;
				sce::Shader::Binary::Element *m_element;     // the element represented by this node
				sce::Shader::Binary::Buffer  *m_buffer;      // the buffer represented by this node in case of buffer node. NULL otherwise.
				uint32_t                        m_numSubNodes; // number of sub nodes 
				ElementNode       **m_subNodes;    // list of pointers to sub nodes

			public:
				ElementNode();
				~ElementNode();

				ElementNode *getSubNodeAt(uint32_t index); // get the nth sub node
			};

			// initialize the tree of element nodes
			// tree will have a root node, then a buffer node for each buffer, finally an element node for each element
			ElementNode *InitElementNodes(const sce::Shader::Binary::Program *program);

			// release the tree of element nodes
			void ReleaseElementNodes(ElementNode *nodeList);

			// shader binary element query by name
			// nodeList: a pointer to the element node tree
			// bufferName: name of the buffer to be searched
			// elementName: extpanded full name of the element to be searched, i.e. m_struct.m_member
			ElementNode *GetElementNodeWithName(ElementNode *nodeList, const char *bufferName, const char *elementName);
		}
	}
}

#endif // _SCE_GNM_TOOLKIT_SHADER_LOADER_H
