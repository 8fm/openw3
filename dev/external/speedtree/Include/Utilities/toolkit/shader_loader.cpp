/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "shader_loader.h"
#include <gnmx/shader_parser.h>
#include "embedded_shader.h"
#include "memory_requests.h"
using namespace sce;

////////////////////////////////////////////////////////////////////////////////
//Shader Loading : SB File Format (with PSSL metadata)
Gnmx::VsShader *sce::Gnmx::Toolkit::loadAndAllocateVertexProgram(sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes,sce::Gnmx::Toolkit::IAllocator *allocator)
{
	sbp->loadFromMemory(pointer, bytes);
	const Gnm::SizeAlign sizeAlign = calculateMemoryRequiredForVsShader(pointer);
	void * dest = allocator->allocate(sizeAlign);
	Gnmx::VsShader *vertexShader = Toolkit::parseVsShader(dest, pointer);
	return vertexShader;
}

Gnmx::PsShader* sce::Gnmx::Toolkit::loadAndAllocatePixelProgram(sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, sce::Gnmx::Toolkit::IAllocator *allocator)
{
	sbp->loadFromMemory(pointer, bytes);
	const Gnm::SizeAlign sizeAlign = calculateMemoryRequiredForPsShader(pointer);
	void * dest = allocator->allocate(sizeAlign);
	Gnmx::PsShader *pixelShader = Toolkit::parsePsShader(dest, pointer);
	return pixelShader;
}

Gnmx::CsShader* sce::Gnmx::Toolkit::loadAndAllocateComputeProgram(sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, sce::Gnmx::Toolkit::IAllocator *allocator)
{
	sbp->loadFromMemory(pointer, bytes);
	const Gnm::SizeAlign sizeAlign = calculateMemoryRequiredForCsShader(pointer);
	void * dest = allocator->allocate(sizeAlign);
	Gnmx::CsShader *computeShader = Toolkit::parseCsShader(dest, pointer);
	return computeShader;
}

Gnmx::LsShader* sce::Gnmx::Toolkit::loadAndAllocateLocalProgram(sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, sce::Gnmx::Toolkit::IAllocator *allocator)
{
	sbp->loadFromMemory(pointer, bytes);
	const Gnm::SizeAlign sizeAlign = calculateMemoryRequiredForLsShader(pointer);
	void * dest = allocator->allocate(sizeAlign);
	Gnmx::LsShader *localShader = Toolkit::parseLsShader(dest, pointer);
	return localShader;
}

Gnmx::HsShader* sce::Gnmx::Toolkit::loadAndAllocateHullProgram(sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, sce::Gnmx::Toolkit::IAllocator *allocator)
{
	sbp->loadFromMemory(pointer, bytes);
	const Gnm::SizeAlign sizeAlign = calculateMemoryRequiredForHsShader(pointer);
	void * dest = allocator->allocate(sizeAlign);
	Gnmx::HsShader *hullShader = Toolkit::parseHsShader(dest, pointer);
	return hullShader;
}

Gnmx::EsShader* sce::Gnmx::Toolkit::loadAndAllocateExportProgram(sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, sce::Gnmx::Toolkit::IAllocator *allocator)
{
	sbp->loadFromMemory(pointer, bytes);
	const Gnm::SizeAlign sizeAlign = calculateMemoryRequiredForEsShader(pointer);
	void * dest = allocator->allocate(sizeAlign);
	Gnmx::EsShader *exportShader = Toolkit::parseEsShader(dest, pointer);
	return exportShader;
}

Gnmx::GsShader* sce::Gnmx::Toolkit::loadAndAllocateGeometryProgram(sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, sce::Gnmx::Toolkit::IAllocator *allocator)
{
	sbp->loadFromMemory(pointer, bytes);
	const Gnm::SizeAlign sizeAlign = calculateMemoryRequiredForGsShader(pointer);
	void * dest = allocator->allocate(sizeAlign);
	Gnmx::GsShader *geometryShader = Toolkit::parseGsShader(dest, pointer);
	return geometryShader;
}

Gnmx::CsVsShader *sce::Gnmx::Toolkit::loadAndAllocateComputeVertexProgram(sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, sce::Gnmx::Toolkit::IAllocator *allocator)
{
	sbp->loadFromMemory(pointer, bytes);
	const Gnm::SizeAlign sizeAlign = calculateMemoryRequiredForCsVsShader(pointer);
	void * dest = allocator->allocate(sizeAlign);
	Gnmx::CsVsShader *computeVertexShader = Toolkit::parseCsVsShader(dest, pointer);
	return computeVertexShader;
}

void sce::Gnmx::Toolkit::releaseShaderProgram(sce::Shader::Binary::Program *sbp, sce::Gnmx::Toolkit::IAllocator *allocator)
{
	SCE_GNM_VALIDATE(sbp, "sbp must not be NULL");
	SCE_GNM_VALIDATE(sbp->m_dataChunk, "sbp->m_dataChunk must not be NULL");

	allocator->release(sbp->m_dataChunk);
	sbp->m_dataChunk = NULL;
}

void sce::Gnmx::Toolkit::saveShaderProgramToFile(sce::Shader::Binary::Program *sbp, const char *sbFilename)
{
	SCE_GNM_VALIDATE(sbp, "sbp must not be NULL");
	SCE_GNM_VALIDATE(sbFilename, "sbFilename must not be NULL");

	// calculate size of this shader program
	uint32_t size = sbp->calculateSize();
	SCE_GNM_VALIDATE(size != 0, "sbp->calculateSize() must not return 0.");

	// open output file
	FILE * file = fopen( (char*)sbFilename, "wb" );
	if ( !file )
	{
		printf("Failed to open file %s for writting \n", sbFilename);
		SCE_GNM_ERROR("Failed to open file %s for writing", sbFilename);
		exit(-1);
	}

	// save the data
	if ( sbp->m_dataChunk )
	{
		fwrite( sbp->m_dataChunk, size, 1, file );
	}
	else
	{
		void* data = malloc(size);
		sbp->saveToMemory(data, size);
		fwrite( data, size, 1, file );
		free(data);
	}

	fclose( file );
}

// runtime helper methods for shader binary element search
sce::Gnmx::Toolkit::ElementNode::ElementNode()
: m_element(NULL)
, m_buffer(NULL)
, m_numSubNodes(0)
, m_subNodes(NULL)
{
}

sce::Gnmx::Toolkit::ElementNode::~ElementNode()
{
	if (m_subNodes)
		delete [] m_subNodes;
	m_subNodes = NULL;
	m_numSubNodes = 0;
}

Gnmx::Toolkit::ElementNode *sce::Gnmx::Toolkit::ElementNode::getSubNodeAt(uint32_t index)
{
	if ( index < m_numSubNodes )
		return m_subNodes[index];

	return NULL;
}

static uint32_t InitRecursiveElementNode(Gnmx::Toolkit::ElementNode *nodeList, uint32_t offset) // returns offset
{
	Gnmx::Toolkit::ElementNode *node = nodeList + offset;
	sce::Shader::Binary::Element *element = node->m_element;

	if ( element->m_type == sce::Shader::Binary::kTypeStructure )
	{
		node->m_numSubNodes = element->m_numElements;
		node->m_subNodes = new Gnmx::Toolkit::ElementNode*[node->m_numSubNodes];
		for ( uint32_t i = 0; i < node->m_numSubNodes; i++ )
		{
			Gnmx::Toolkit::ElementNode *subNode = nodeList + offset + 1;
			subNode->m_fullNameStr = std::string(node->m_fullNameStr);
			subNode->m_fullNameStr.append(element->m_isPointer ? "->" : ".");
			subNode->m_fullNameStr.append(subNode->m_element->getName());
			node->m_subNodes[i] = subNode;

			offset = InitRecursiveElementNode(nodeList, offset+1);
		}
	}

	return offset;
}

Gnmx::Toolkit::ElementNode *sce::Gnmx::Toolkit::InitElementNodes(const sce::Shader::Binary::Program *program)
{
	ElementNode *nodeList = NULL;

	uint32_t numBuffers = program->m_numBuffers;
	uint32_t numElements = program->m_numElements;

	if (numBuffers && numElements)
	{
		nodeList = new ElementNode[numElements + numBuffers + 1]; // +1 for root

		// init root
		Toolkit::ElementNode *root = &nodeList[0];
		root->m_fullNameStr = std::string("root");
		root->m_numSubNodes = numBuffers;
		root->m_subNodes = new Toolkit::ElementNode*[root->m_numSubNodes];

		// init element nodes and names
		for( uint32_t i = 0; i < numElements; i++ )
		{
			Toolkit::ElementNode *node = &nodeList[numBuffers + 1 + i];
			sce::Shader::Binary::Element *element = &program->m_elements[i];
			node->m_element = element;
			const char* name = element->getName();
			if ( name && strlen(name) && strcmp(name, "(no_name)") != 0)
			{
				node->m_fullNameStr = std::string(element->getName());
			}
			else
			{
				node->m_fullNameStr = std::string("<struct>");
			}
		}

		// init buffer nodes and sub node pointers in the root
		for( uint32_t i = 0; i < numBuffers; i++ )
		{
			Toolkit::ElementNode *bufferNode = &nodeList[i+1];
			sce::Shader::Binary::Buffer *buffer = program->m_buffers + i;
			SCE_GNM_VALIDATE( strlen((const char*)buffer->getName()) > 0, "buffer->getName() returned an empty string" );
			bufferNode->m_fullNameStr = std::string(buffer->getName());
			bufferNode->m_buffer = buffer;
			root->m_subNodes[i] = bufferNode;

			// init element nodes
			bufferNode->m_numSubNodes = buffer->m_numElements;
			if (bufferNode->m_numSubNodes)
			{
				bufferNode->m_subNodes = new Toolkit::ElementNode*[bufferNode->m_numSubNodes];
				uint32_t offset = numBuffers + 1 + buffer->m_elementOffset;
				for( uint32_t j = 0; j < bufferNode->m_numSubNodes; j++ )
				{
					bufferNode->m_subNodes[j] = nodeList + offset;
					offset = InitRecursiveElementNode(nodeList, offset) + 1;
				}
			}
		}
	}

	return nodeList;
}

void sce::Gnmx::Toolkit::ReleaseElementNodes(ElementNode *nodeList)
{
	if(nodeList)
		delete [] nodeList;
}

static Gnmx::Toolkit::ElementNode *GetRecursiveNodeWithName(Gnmx::Toolkit::ElementNode *node, const char *elementName)
{
	if ( node->m_fullNameStr.compare(elementName) == 0 )
	{
		return node;
	}

	for ( uint32_t i = 0; i < node->m_numSubNodes; i++ )
	{
		Gnmx::Toolkit::ElementNode *foundNode = GetRecursiveNodeWithName(node->getSubNodeAt(i), elementName);

		if ( foundNode )
			return foundNode;
	}

	return NULL;
}

Gnmx::Toolkit::ElementNode *sce::Gnmx::Toolkit::GetElementNodeWithName(Toolkit::ElementNode *nodeList, const char *bufferName, const char *elementName)
{
	if(nodeList)
	{
		Toolkit::ElementNode *root = &nodeList[0];
		SCE_GNM_ASSERT(root->m_fullNameStr.compare("root") == 0);

		// find a buffer node matches the buffer name
		if ( bufferName && strlen(bufferName) )
		{
			for ( uint32_t i = 0; i < root->m_numSubNodes; i++ )
			{
				Toolkit::ElementNode *bufferNode = root->m_subNodes[i];
				if ( bufferNode->m_fullNameStr.compare( bufferName ) == 0 )
				{
					// find an element node matches the element name
					for ( uint32_t j = 0; j < bufferNode->m_numSubNodes; j++ )
					{
						Toolkit::ElementNode *foundNode = GetRecursiveNodeWithName(bufferNode->m_subNodes[j], elementName);

						if ( foundNode )
							return foundNode;
					}
				}
			}
		}
	}

	return NULL;
}

Gnm::SizeAlign sce::Gnmx::Toolkit::calculateMemoryRequiredForVsFetchShader(const void *src)
{
	Gnmx::ShaderInfo shaderInfo;
	Gnmx::parseShader(&shaderInfo, src);

	Gnm::SizeAlign result;
	result.m_align = Gnm::kAlignmentOfBufferInBytes;
	result.m_size = computeVsFetchShaderSize(shaderInfo.m_vsShader);
	return result;
}

Gnm::SizeAlign sce::Gnmx::Toolkit::calculateMemoryRequiredForVsShader(const void *src)
{
	Gnmx::ShaderInfo shaderInfo;
	Gnmx::parseShader(&shaderInfo, src);

	Gnm::SizeAlign result;
	result.m_align = Gnm::kAlignmentOfShaderInBytes;
	result.m_size = roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_vsShader->computeSize())
		          + shaderInfo.m_gpuShaderCodeSize;
	return result;
}

Gnm::SizeAlign sce::Gnmx::Toolkit::calculateMemoryRequiredForPsShader(const void *src)
{
	Gnmx::ShaderInfo shaderInfo;
	Gnmx::parseShader(&shaderInfo, src);

	Gnm::SizeAlign result;
	result.m_align = Gnm::kAlignmentOfShaderInBytes;
	result.m_size = roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_psShader->computeSize())
		          + shaderInfo.m_gpuShaderCodeSize;
	return result;
}

Gnm::SizeAlign sce::Gnmx::Toolkit::calculateMemoryRequiredForCsShader(const void *src)
{
	Gnmx::ShaderInfo shaderInfo;
	Gnmx::parseShader(&shaderInfo, src);

	Gnm::SizeAlign result;
	result.m_align = Gnm::kAlignmentOfShaderInBytes;
	result.m_size = roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_csShader->computeSize())
		          + shaderInfo.m_gpuShaderCodeSize;
	return result;
}

Gnm::SizeAlign sce::Gnmx::Toolkit::calculateMemoryRequiredForLsShader(const void *src)
{
	Gnmx::ShaderInfo shaderInfo;
	Gnmx::parseShader(&shaderInfo, src);

	Gnm::SizeAlign result;
	result.m_align = Gnm::kAlignmentOfShaderInBytes;
	result.m_size = roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_lsShader->computeSize())
		          + shaderInfo.m_gpuShaderCodeSize;
	return result;
}

Gnm::SizeAlign sce::Gnmx::Toolkit::calculateMemoryRequiredForHsShader(const void *src)
{
	Gnmx::ShaderInfo shaderInfo;
	Gnmx::parseShader(&shaderInfo, src);

	Gnm::SizeAlign result;
	result.m_align = Gnm::kAlignmentOfShaderInBytes;
	result.m_size = roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_hsShader->computeSize())
		          + shaderInfo.m_gpuShaderCodeSize;
	return result;
}

Gnm::SizeAlign sce::Gnmx::Toolkit::calculateMemoryRequiredForEsShader(const void *src)
{
	Gnmx::ShaderInfo shaderInfo;
	Gnmx::parseShader(&shaderInfo, src);

	Gnm::SizeAlign result;
	result.m_align = Gnm::kAlignmentOfShaderInBytes;
	result.m_size = roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_esShader->computeSize())
		          + shaderInfo.m_gpuShaderCodeSize;
	return result;
}

Gnm::SizeAlign sce::Gnmx::Toolkit::calculateMemoryRequiredForGsShader(const void *src)
{
	Gnmx::ShaderInfo gsShaderInfo;
	Gnmx::ShaderInfo vsShaderInfo;
	Gnmx::parseGsShader(&gsShaderInfo, &vsShaderInfo, src);

	Gnm::SizeAlign result;
	result.m_align = Gnm::kAlignmentOfShaderInBytes;
	result.m_size = roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, gsShaderInfo.m_gsShader->computeSize())
				  + roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, gsShaderInfo.m_gpuShaderCodeSize) 
		          + vsShaderInfo.m_gpuShaderCodeSize;
	return result;
}

Gnm::SizeAlign sce::Gnmx::Toolkit::calculateMemoryRequiredForCsVsFetchShaderVs(const void *src)
{
	Gnmx::ShaderInfo csvsShaderInfo;
	Gnmx::ShaderInfo csShaderInfo;
	Gnmx::parseCsVsShader(&csvsShaderInfo, &csShaderInfo, src);

	Gnm::SizeAlign result;
	result.m_align = Gnm::kAlignmentOfBufferInBytes;
	result.m_size = computeVsFetchShaderSize(csvsShaderInfo.m_csvsShader);
	return result;
}

Gnm::SizeAlign sce::Gnmx::Toolkit::calculateMemoryRequiredForCsVsFetchShaderCs(const void *src)
{
	Gnmx::ShaderInfo csvsShaderInfo;
	Gnmx::ShaderInfo csShaderInfo;
	Gnmx::parseCsVsShader(&csvsShaderInfo, &csShaderInfo, src);

	Gnm::SizeAlign result;
	result.m_align = Gnm::kAlignmentOfBufferInBytes;
	result.m_size = computeCsFetchShaderSize(csvsShaderInfo.m_csvsShader);
	return result;
}

Gnm::SizeAlign sce::Gnmx::Toolkit::calculateMemoryRequiredForCsVsShader(const void *src)
{
	Gnmx::ShaderInfo csvsShaderInfo;
	Gnmx::ShaderInfo csShaderInfo;
	Gnmx::parseCsVsShader(&csvsShaderInfo, &csShaderInfo, src);

	Gnm::SizeAlign result;
	result.m_align = Gnm::kAlignmentOfShaderInBytes;
	result.m_size = roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, csvsShaderInfo.m_csvsShader->computeSize())
				  + roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, csvsShaderInfo.m_gpuShaderCodeSize) 
				  + csShaderInfo.m_gpuShaderCodeSize;
	return result;
}

sce::Gnmx::VsShader* sce::Gnmx::Toolkit::parseVsShader(void *dest, const void *src)
{
	Gnmx::ShaderInfo shaderInfo;
	Gnmx::parseShader(&shaderInfo, src);

    Gnmx::VsShader *const final = reinterpret_cast<Gnmx::VsShader*>(dest);
	memcpy(dest, shaderInfo.m_vsShader, shaderInfo.m_vsShader->computeSize());
	dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_vsShader->computeSize());

	void * vsBlockAddr = dest;
	memcpy(dest, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize);

	final->patchShaderGpuAddress(vsBlockAddr);
	return final;
}

sce::Gnmx::PsShader* sce::Gnmx::Toolkit::parsePsShader(void * dest, const void *src)
{
	Gnmx::ShaderInfo shaderInfo;
	Gnmx::parseShader(&shaderInfo, src);

	Gnmx::PsShader *const final = reinterpret_cast<Gnmx::PsShader*>(dest);
	memcpy(dest, shaderInfo.m_psShader, shaderInfo.m_psShader->computeSize());
	dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_psShader->computeSize());

	void * psBlockAddr = dest;
	memcpy(dest, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize);

	final->patchShaderGpuAddress(psBlockAddr);
	return final;
}

sce::Gnmx::CsShader* sce::Gnmx::Toolkit::parseCsShader(void * dest, const void *src)
{
	Gnmx::ShaderInfo shaderInfo;
	Gnmx::parseShader(&shaderInfo, src);

	Gnmx::CsShader *const final = reinterpret_cast<Gnmx::CsShader*>(dest);
	memcpy(dest, shaderInfo.m_csShader, shaderInfo.m_csShader->computeSize());
	dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_csShader->computeSize());

	void * csBlockAddr = dest;
	memcpy(dest, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize);

	final->patchShaderGpuAddress(csBlockAddr);
	return final;
}

sce::Gnmx::LsShader* sce::Gnmx::Toolkit::parseLsShader(void * dest, const void *src)
{
	Gnmx::ShaderInfo shaderInfo;
	Gnmx::parseShader(&shaderInfo, src);

	Gnmx::LsShader *const final = reinterpret_cast<Gnmx::LsShader*>(dest);
	memcpy(dest, shaderInfo.m_lsShader, shaderInfo.m_lsShader->computeSize());
	dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_lsShader->computeSize());

	void * lsBlockAddr = dest;
	memcpy(dest, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize);

	final->patchShaderGpuAddress(lsBlockAddr);
	return final;
}

sce::Gnmx::HsShader* sce::Gnmx::Toolkit::parseHsShader(void * dest, const void *src)
{
	Gnmx::ShaderInfo shaderInfo;
	Gnmx::parseShader(&shaderInfo, src);

	Gnmx::HsShader *const final = reinterpret_cast<Gnmx::HsShader*>(dest);
	memcpy(dest, shaderInfo.m_hsShader, shaderInfo.m_hsShader->computeSize());
	dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_hsShader->computeSize());

	void * hsBlockAddr = dest;
	memcpy(dest, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize);

	final->patchShaderGpuAddress(hsBlockAddr);
	return final;
}

sce::Gnmx::EsShader* sce::Gnmx::Toolkit::parseEsShader(void * dest, const void *src)
{
	Gnmx::ShaderInfo shaderInfo;
	Gnmx::parseShader(&shaderInfo, src);

	Gnmx::EsShader *const final = reinterpret_cast<Gnmx::EsShader*>(dest);
	memcpy(dest, shaderInfo.m_esShader, shaderInfo.m_esShader->computeSize());
	dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_esShader->computeSize());

	void * esBlockAddr = dest;
	memcpy(dest, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize);

	final->patchShaderGpuAddress(esBlockAddr);
	return final;
}

Gnmx::GsShader *sce::Gnmx::Toolkit::parseGsShader(void * dest, const void *src)
{
	Gnmx::ShaderInfo gsShaderInfo;
	Gnmx::ShaderInfo vsShaderInfo;
	Gnmx::parseGsShader(&gsShaderInfo, &vsShaderInfo, src);

	Gnmx::GsShader *const final = reinterpret_cast<Gnmx::GsShader*>(dest);
	memcpy(dest, gsShaderInfo.m_gsShader, gsShaderInfo.m_gsShader->computeSize());
	dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, gsShaderInfo.m_gsShader->computeSize());

	void * gsBlockAddr = dest;
	memcpy(dest, gsShaderInfo.m_gpuShaderCode, gsShaderInfo.m_gpuShaderCodeSize);
	dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, gsShaderInfo.m_gpuShaderCodeSize);

	void * vsBlockAddr = dest;
	memcpy(dest, vsShaderInfo.m_gpuShaderCode, vsShaderInfo.m_gpuShaderCodeSize);

	final->patchShaderGpuAddresses(gsBlockAddr, vsBlockAddr);
	return final;
}

Gnmx::CsVsShader *sce::Gnmx::Toolkit::parseCsVsShader(void *dest, const void *src)
{
	Gnmx::ShaderInfo csvsShaderInfo;
	Gnmx::ShaderInfo csShaderInfo;
	Gnmx::parseCsVsShader(&csvsShaderInfo, &csShaderInfo, src);

	Gnmx::CsVsShader *const final = reinterpret_cast<Gnmx::CsVsShader*>(dest);
	memcpy(dest, csvsShaderInfo.m_csvsShader, csvsShaderInfo.m_csvsShader->computeSize());
	dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, csvsShaderInfo.m_csvsShader->computeSize());

	void * vsBlockAddr = dest;
	memcpy(dest, csvsShaderInfo.m_gpuShaderCode, csvsShaderInfo.m_gpuShaderCodeSize);
	dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(Gnm::kAlignmentOfShaderInBytes, csvsShaderInfo.m_gpuShaderCodeSize);

	void * csBlockAddr = dest;
	memcpy(dest, csShaderInfo.m_gpuShaderCode, csShaderInfo.m_gpuShaderCodeSize);

	final->patchShaderGpuAddresses(vsBlockAddr, csBlockAddr);
	return final;
}
