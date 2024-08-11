/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "wccStats.h"

//////////////////////////////////////////////////////////////////////////////////
//						Static shaders cache stats								//
//////////////////////////////////////////////////////////////////////////////////

CStaticShadersStats::CStaticShadersStats()
{
	m_totalShadersSize = 0;
	m_totalElements = 0;
}

void CStaticShadersStats::GenerateShaderStat( const String& shaderName, const Uint64 shaderSize )
{
	StaticShaderStats stats;
	stats.m_shaderSize = shaderSize;

	m_stats.Insert( shaderName, stats );
}

void CStaticShadersStats::DumpStatsToFile( const String& path )
{
	// gather information
	//text += TXT("|==========================================================================|\n");
	//text += TXT("| Static Shader                   |  # ShadersGenerated  |  Memory KB      |\n");
	//text += TXT("|==========================================================================|\n");
	String text = String::Printf( TXT("Static Shader, # ShadersGenerated, # Memory KB,\n") );
	GFileManager->SaveStringToFile(path, text);

	for( auto it : m_stats )
	{
		text += String::Printf( TXT("%s,%d,%1.3f,\n"), it.m_first.AsChar(), it.m_second.m_shadersGenerated, (Float)it.m_second.m_sumGeneratedShadersSize / 1024.0f );

		m_totalElements += it.m_second.m_shadersGenerated;
		m_totalShadersSize += it.m_second.m_sumGeneratedShadersSize;
	}
	text += String::Printf( TXT("Total Shaders Generated: %d \n"), m_totalElements );
	text += String::Printf( TXT("Total Shaders Size: %1.3f \n"), (Float)m_totalShadersSize / 1024.0f );

	// write text to file
	GFileManager->SaveStringToFile(path, text);
}

void CStaticShadersStats::AddStat( const String& name, Uint64 size )
{
	m_stats[name].m_shadersGenerated++;
	m_stats[name].m_sumGeneratedShadersSize += size;
}

//////////////////////////////////////////////////////////////////////////////////
//						Material shaders cache stats							//
//////////////////////////////////////////////////////////////////////////////////

CMaterialShadersStats::CMaterialShadersStats()
{
	m_totalShadersSize = 0;
	m_totalMaterialsSize = 0;
	m_totalElements = 0;
	m_totalMaterials = 0;
	m_otherDataSize = 0;
}

void CMaterialShadersStats::GenerateShaderStat(const String& materialGraphName, Uint32 techniquesNum )
{
	MaterialShaderStats stats;
	stats.m_techniquesCount = techniquesNum;

	m_stats.Insert( materialGraphName, stats );
}

void CMaterialShadersStats::AddShaderStat( const String& name, GpuApi::eShaderType shaderType, Uint64 size )
{	
	m_stats[name].m_shadersGenerated++;
	m_stats[name].m_sumGeneratedShadersSize += (size + sizeof(Uint64)); // m_data size + m_hash size

	m_stats[name].m_shadersCountSize[shaderType].m_shadersCount++;
	m_stats[name].m_shadersCountSize[shaderType].m_shadersSummedSize += size;
}

void CMaterialShadersStats::AddMaterialGenericStat(const String& name, Uint64 pathSize)
{
	Uint64 otherDataSize = 4*sizeof(Uint64) + sizeof(Uint32) + pathSize;
	m_stats[name].m_otherDataSize += otherDataSize;
	m_stats[name].m_materialsSaved++;
}

void CMaterialShadersStats::AddMaterialStat( const String& name, eMaterialStat dataType, Uint32 samplerStatesCount, Uint32 usedParamsCount, Uint64 samplerStatesSize, Uint64 usedParamsSize )
{	
	m_stats[name].m_sumGeneratedMaterialSize += (samplerStatesSize + usedParamsSize);

	m_stats[name].m_materialsCountSize[dataType].m_samplersSummedSize += samplerStatesSize;
	m_stats[name].m_materialsCountSize[dataType].m_usedParamsSummedSize += usedParamsSize;
	m_stats[name].m_materialsCountSize[dataType].m_samplersCount += samplerStatesCount;
	m_stats[name].m_materialsCountSize[dataType].m_usedParamsCount += usedParamsCount;
}

void CMaterialShadersStats::DumpStatsToFile( const String& path )
{
	TDynArray< String > materials;
	m_stats.GetKeys(materials);
	Sort( materials.Begin(), materials.End() );

	// gather information
	//String text = String::EMPTY;
	//text += TXT("|=====================================================================================================================================================================================================================|\n");
	//text += TXT("| Material                                                                                       | # techniques |  # PS / Mem KB   |   # VS / Mem KB  |   # HS / Mem KB  |   # DS / Mem KB  |   # Total / Memory KB   |\n");
	//text += TXT("|=====================================================================================================================================================================================================================|\n");
	String text = String::Printf( TXT("Material,# techniques, # PS,PS Mem KB, # VS,VS Mem KB, # HS,HS Mem KB, # DS,DS Mem KB, # Total,Total Mem KB, # VS Samplers,VS Samplers Mem KB, # VS used params, VS used params Mem KB, # PS Samplers, PS Samplers Mem KB, # PS used params,PS used params Mem KB, # Total Material Mem KB, # other data size KB\n") );
	GFileManager->SaveStringToFile(path, text);

	text = String::EMPTY;
	Uint32 counter = 0;
	for( String m : materials )
	{
		MaterialShaderStats& stats = m_stats[m];
		text += String::Printf( TXT("%s,%d,%d,%1.2f,%d,%1.2f,%d,%1.2f,%d,%1.2f,%d,%1.2f,%d,%1.2f,%d,%1.2f,%d,%1.2f,%d,%1.2f,%1.2f,%1.2f,\n"),
			m.AsChar(),
			stats.m_techniquesCount, 
			stats.m_shadersCountSize[GpuApi::PixelShader].m_shadersCount       , stats.m_shadersCountSize[GpuApi::PixelShader].m_shadersSummedSize / 1024.0f,
			stats.m_shadersCountSize[GpuApi::VertexShader].m_shadersCount      , stats.m_shadersCountSize[GpuApi::VertexShader].m_shadersSummedSize / 1024.0f,
			stats.m_shadersCountSize[GpuApi::HullShader].m_shadersCount        , stats.m_shadersCountSize[GpuApi::HullShader].m_shadersSummedSize / 1024.0f,
			stats.m_shadersCountSize[GpuApi::DomainShader].m_shadersCount      , stats.m_shadersCountSize[GpuApi::DomainShader].m_shadersSummedSize / 1024.0f,
			stats.m_shadersGenerated, stats.m_sumGeneratedShadersSize / 1024.0f,
			stats.m_materialsCountSize[VSSamplerStates].m_samplersCount        , stats.m_materialsCountSize[VSSamplerStates].m_samplersSummedSize / 1024.0f,
			stats.m_materialsCountSize[VSSamplerStates].m_usedParamsCount      , stats.m_materialsCountSize[VSSamplerStates].m_usedParamsSummedSize / 1024.0f,
			stats.m_materialsCountSize[PSSamplerStates].m_samplersCount        , stats.m_materialsCountSize[PSSamplerStates].m_samplersSummedSize / 1024.0f,
			stats.m_materialsCountSize[PSSamplerStates].m_usedParamsCount      , stats.m_materialsCountSize[PSSamplerStates].m_usedParamsSummedSize / 1024.0f,
			stats.m_sumGeneratedMaterialSize / 1024.0f, stats.m_otherDataSize / 1024.0f	);

		counter++;
		if( counter % 10 == 0 )
		{
			GFileManager->SaveStringToFile(path, text, true);
			text = String::EMPTY;
			counter = 0;
		}
		/*text += String::Printf(TXT("%97s| %12s | %4s / %8s  | %4s / %8s  | %4s / %8s  | %4s / %8s  | %7s / %10s  "), 
			m.AsChar(),
			ToString( stats.m_techniquesCount ).AsChar(), 
			ToString( stats.m_shadersCountSize[GpuApi::PixelShader].m_shadersCount  ).AsChar(), ToString( stats.m_shadersCountSize[GpuApi::PixelShader].m_shadersSummedSize / 1024.0f ).AsChar(),
			ToString( stats.m_shadersCountSize[GpuApi::VertexShader].m_shadersCount ).AsChar(), ToString( stats.m_shadersCountSize[GpuApi::VertexShader].m_shadersSummedSize / 1024.0f ).AsChar(),
			ToString( stats.m_shadersCountSize[GpuApi::HullShader].m_shadersCount   ).AsChar(), ToString( stats.m_shadersCountSize[GpuApi::HullShader].m_shadersSummedSize / 1024.0f ).AsChar(),
			ToString( stats.m_shadersCountSize[GpuApi::DomainShader].m_shadersCount ).AsChar(), ToString( stats.m_shadersCountSize[GpuApi::DomainShader].m_shadersSummedSize / 1024.0f ).AsChar(),
			ToString( stats.m_shadersGenerated ).AsChar(), ToString( stats.m_sumGeneratedShadersSize / 1024.0f ).AsChar() );
		text += TXT("|\n");*/

		m_totalMaterials += stats.m_materialsSaved;
		m_totalElements += stats.m_shadersGenerated;
		m_otherDataSize += stats.m_otherDataSize;
		m_totalShadersSize += stats.m_sumGeneratedShadersSize;
		m_totalMaterialsSize += stats.m_sumGeneratedMaterialSize;
	}
	GFileManager->SaveStringToFile(path, text, true);

	text = String::EMPTY;
	text += String::Printf( TXT("Total Shaders Generated: %d \n"), m_totalElements );
	text += String::Printf( TXT("Total Material Entries Saved: %d \n"), m_totalMaterials );
	text += String::Printf( TXT("Total Shaders Size: %1.3f KB\n"), (Float)m_totalShadersSize / 1024.0f );
	text += String::Printf( TXT("Total Materials Size: %1.3f KB\n"), (Float)m_totalMaterialsSize / 1024.0f );
	text += String::Printf( TXT("Total Other Data Size: %1.3f KB\n"), (Float)m_otherDataSize / 1024.0f );
	text += String::Printf( TXT("Total Size: %1.3f KB\n"), (Float)(m_totalShadersSize + m_totalMaterialsSize + m_otherDataSize) / 1024.0f );
	//text += TXT("|===================================================================================================================================================================================================================|\n");
	
	// write text to file
	GFileManager->SaveStringToFile(path, text, true);
}

void CMaterialShadersStats::Clear()
{
	m_totalShadersSize = 0;
	m_totalMaterialsSize = 0;
	m_totalElements = 0;
	m_totalMaterials = 0;

	for( auto it : m_stats )
	{
		it.m_second.m_shadersCountSize.Clear();
	}

	m_stats.Clear();
}