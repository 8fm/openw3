/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "speedTreeRenderInterface.h"

#ifdef USE_SPEED_TREE

using namespace SpeedTree;

GpuApi::VertexPacking::ePackingType GetMatchingGPUAPIType( EVertexFormat eVertexFormat, st_int32 nNumComponents )
{
	GpuApi::VertexPacking::ePackingType eGPUAPIType = GpuApi::VertexPacking::PT_Invalid; // not found


	if (nNumComponents > 0 && nNumComponents < 5)
	{
		if (eVertexFormat == VERTEX_FORMAT_BYTE)
		{
			assert(nNumComponents == 4);
			eGPUAPIType = GpuApi::VertexPacking::PT_Color;
		}
		else if (eVertexFormat == VERTEX_FORMAT_HALF_FLOAT)
		{
			if (nNumComponents == 2)
				eGPUAPIType = GpuApi::VertexPacking::PT_Float16_2;
			else if (nNumComponents == 4)
				eGPUAPIType = GpuApi::VertexPacking::PT_Float16_4;
			else
				assert(false);
		}
		else if (eVertexFormat == VERTEX_FORMAT_FULL_FLOAT)
		{
			if (nNumComponents == 1)
				eGPUAPIType = GpuApi::VertexPacking::PT_Float1;
			else if (nNumComponents == 2)
				eGPUAPIType = GpuApi::VertexPacking::PT_Float2;
			else if (nNumComponents == 3)
				eGPUAPIType = GpuApi::VertexPacking::PT_Float3;
			else
				eGPUAPIType = GpuApi::VertexPacking::PT_Float4;
		}
		else
			assert(false);
	}

	return eGPUAPIType;
}


bool GetMatchingGPUAPISemantic( Uint32 eAttrib, Uint32 stream, GpuApi::VertexPacking::ePackingUsage& usage, Uint8& usageIndex )
{
	usage = GpuApi::VertexPacking::PS_SpT_Attr;
	usageIndex = (Uint8)eAttrib;

	return true;
}


st_bool CGeometryBufferGPUAPI::SetVertexDecl(const SVertexDecl& sVertexDecl, const CShaderTechnique* pTechnique, const SVertexDecl& sInstanceVertexDecl)
{
	st_bool bSuccess = false;

	// is the instance vertex decl empty?
	const st_bool c_bInstanceVertexDeclEmpty = (sInstanceVertexDecl.m_uiVertexSize == 0);

	// which vertex decl matches the shader vertex input?
	const SVertexDecl& sShaderVertexDecl = (c_bInstanceVertexDeclEmpty ? sVertexDecl : sInstanceVertexDecl);

	// count # of used attributes
	size_t siNumElements = 0;
	for (st_int32 i = 0; i < VERTEX_ATTRIB_COUNT; ++i)
		if (sShaderVertexDecl.m_asAttributes[i].IsUsed( ))
			++siNumElements;

	if (siNumElements > 0)
	{
		// Since the speedtree folks decided to have a instance and vertex stream mixed one into the other, let's maintain separate lists of elements, so that we can add one to the other at the end
		GpuApi::VertexPacking::PackingElement* aElements = new GpuApi::VertexPacking::PackingElement[VERTEX_ATTRIB_COUNT];
		GpuApi::VertexPacking::PackingElement* elementsPtr = aElements;
		size_t numVertexElements = 0;

		GpuApi::VertexPacking::PackingElement* aInstancedElements = new GpuApi::VertexPacking::PackingElement[VERTEX_ATTRIB_COUNT];
		GpuApi::VertexPacking::PackingElement* instancedElementsPtr = aInstancedElements;
		size_t numInstanceElements = 0;

		for (st_int32 i = 0; i < VERTEX_ATTRIB_COUNT; ++i)
		{
			const SVertexDecl::SAttribute& sAttrib = sShaderVertexDecl.m_asAttributes[i];

			if (sAttrib.IsUsed( ))
			{
				// setup element to match this attribute
				GpuApi::VertexPacking::PackingElement sElement;
				sElement.m_type = GetMatchingGPUAPIType( sAttrib.m_eFormat, sAttrib.NumUsedComponents( ) );

				GPUAPI_ASSERT( sAttrib.m_uiStream <= 1, TXT("Vertex declaration messed up! Up to a number of TWO vertex streams is expected in SpeedTree.") );

				if ( sElement.m_type != GpuApi::VertexPacking::PT_Invalid && GetMatchingGPUAPISemantic( i, sAttrib.m_uiStream, sElement.m_usage, sElement.m_usageIndex ) )
				{
					sElement.m_slot = sAttrib.m_uiStream;
					sElement.m_slotType = ( GpuApi::VertexPacking::eSlotType ) sAttrib.m_uiStream;

					if ( sElement.m_slotType == GpuApi::VertexPacking::ST_PerInstance )
					{
						*instancedElementsPtr = sElement;
						++instancedElementsPtr;
						++numInstanceElements;
					}
					else
					{
						*elementsPtr = sElement;
						++elementsPtr;
						++numVertexElements;
					}
					
					bSuccess = true;
				}
				else
				{
					CCore::SetError("CGeometryBufferGPUAPI::SetVertexDecl, vertex attribute not supported by GPUAPI: (%s, # elements: %d, data type: %s)\n",
						SVertexDecl::AttributeName(EVertexAttribute(i)), sAttrib.NumUsedComponents( ), SVertexDecl::FormatName(sAttrib.m_eFormat));
					bSuccess = false;
					break;
				}
			}
		}

		Red::System::MemoryCopy( &aInstancedElements[numInstanceElements], &aElements[0], numVertexElements * sizeof(GpuApi::VertexPacking::PackingElement) );
		aInstancedElements[ siNumElements ] = GpuApi::VertexPacking::PackingElement::END_OF_ELEMENTS;

		if (bSuccess)
		{
			GpuApi::VertexLayoutDesc vertexLayoutDesc( aInstancedElements );
			//vertexLayoutDesc.m_semanticRemapping = true;
			m_vertexLayout = GpuApi::CreateVertexLayout( vertexLayoutDesc );
			if ( m_vertexLayout.isNull() )
			{
				CCore::SetError("CGeometryBufferGPUAPI::SetVertexDecl failed");
			}
		}

		delete [] aElements;
		delete [] aInstancedElements;
	}

	return bSuccess;
}

#endif