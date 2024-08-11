/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//////////////////////////////////////////////////////////////////////////////////

class IStats
{
public:
	virtual void DumpStatsToFile( const String& path ) = 0;

protected:
	
	String m_name;
	Uint64 m_totalShadersSize;
	Uint32 m_totalElements;
	Uint32 m_totalMaterials;
	Uint64 m_totalMaterialsSize;
	Uint64 m_otherDataSize;
};

//////////////////////////////////////////////////////////////////////////////////
//						Static shaders cache stats								//
//////////////////////////////////////////////////////////////////////////////////

class CStaticShadersStats : public IStats
{
public:
	struct StaticShaderStats
	{
		Uint64 m_shaderSize;

		Uint32 m_shadersGenerated;
		Uint64 m_sumGeneratedShadersSize;

		StaticShaderStats()
		{
			m_shaderSize = 0;
			m_shadersGenerated = 0;
			m_sumGeneratedShadersSize = 0;
		}
	};

public:
	CStaticShadersStats();

	RED_INLINE void Reserve( Uint32 size ){ m_stats.Reserve( size ); }

	void GenerateShaderStat( const String& shaderName, const Uint64 shaderSize );

	void AddStat( const String& name, Uint64 size );

	virtual void DumpStatsToFile( const String& path );

private:
	THashMap< String, StaticShaderStats > m_stats;
};

//////////////////////////////////////////////////////////////////////////////////
//						Material shaders cache stats							//
//////////////////////////////////////////////////////////////////////////////////

enum eMaterialStat : Uint32
{
	VSSamplerStates,
	PSSamplerStates,
};

class CMaterialShadersStats : public IStats
{
public:
	struct ShaderTypeStats
	{
		Uint32 m_shadersCount;
		Uint64 m_shadersSummedSize;
		ShaderTypeStats()
		{
			m_shadersCount = 0;
			m_shadersSummedSize = 0;
		}
	};

	struct SamplerStats
	{
		Uint32 m_samplersCount;
		Uint32 m_usedParamsCount;
		Uint64 m_samplersSummedSize;
		Uint64 m_usedParamsSummedSize;
		SamplerStats()
		{
			m_samplersCount = 0;
			m_usedParamsCount = 0;
			m_samplersSummedSize = 0;
			m_usedParamsSummedSize = 0;
		}
	};

	struct MaterialShaderStats
	{
		Uint32 m_shadersGenerated;			// techniques collected
		Uint64 m_sumGeneratedShadersSize;   // compiled shaders summarized size
		Uint32 m_techniquesCount;
		Uint32 m_materialsSaved;
		Uint64 m_sumGeneratedMaterialSize;
		Uint64 m_otherDataSize;

		THashMap < Uint32, ShaderTypeStats > m_shadersCountSize;
		THashMap < Uint32, SamplerStats >   m_materialsCountSize;

		MaterialShaderStats()
		{
			m_techniquesCount = 0;
			m_shadersGenerated = 0;
			m_materialsSaved = 0;
			m_sumGeneratedShadersSize = 0;
			m_sumGeneratedMaterialSize = 0;
			m_otherDataSize = 0;

			ShaderTypeStats shaderTypeStats;
			m_shadersCountSize.Reserve(6);
			m_shadersCountSize.Insert( GpuApi::VertexShader, shaderTypeStats );
			m_shadersCountSize.Insert( GpuApi::PixelShader, shaderTypeStats );
			m_shadersCountSize.Insert( GpuApi::HullShader, shaderTypeStats );
			m_shadersCountSize.Insert( GpuApi::DomainShader, shaderTypeStats );

			SamplerStats materialStats;
			m_materialsCountSize.Reserve(2);
			m_materialsCountSize.Insert( VSSamplerStates, materialStats );
			m_materialsCountSize.Insert( PSSamplerStates, materialStats );
		}
	};

public:
	CMaterialShadersStats();

	RED_INLINE void Reserve( Uint32 size ){ m_stats.Reserve( size ); }

	void GenerateShaderStat( const String& materialGraphName, Uint32 techniquesNum );

	void AddShaderStat( const String& name, GpuApi::eShaderType shaderType, Uint64 size );

	void AddMaterialStat( const String& name, eMaterialStat dataType, Uint32 samplerStatesCount, Uint32 usedParamsCount, Uint64 samplerStatesSize, Uint64 usedParamsSize );

	void AddMaterialGenericStat( const String& name, Uint64 pathSize );

	virtual void DumpStatsToFile( const String& path );

	void Clear();

private:
	THashMap< String, MaterialShaderStats > m_stats;
};