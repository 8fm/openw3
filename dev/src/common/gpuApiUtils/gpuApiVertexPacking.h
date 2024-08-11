/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

namespace GpuApi
{
	// Vertex packing
	namespace VertexPacking
	{
		enum ePackingType
		{
			PT_Invalid=-1,
			PT_Float1=0,
			PT_Float2,
			PT_Float3,
			PT_Float4,
			PT_Float16_2,
			PT_Float16_4,
			PT_UShort1,
			PT_UShort2,
			PT_UShort4,
			PT_UShort4N,
			PT_Short1,
			PT_Short2,
			PT_Short4,
			PT_Short4N,
			PT_UInt1,
			PT_UInt2,
			PT_UInt3,
			PT_UInt4,
			PT_Int1,
			PT_Int2,
			PT_Int3,
			PT_Int4,
			PT_Color,
			PT_UByte1,
			PT_UByte4,
			PT_UByte4N,
			PT_Byte4N,
			PT_Dec4,			// 10.10.10.2 unorm format

			PT_Index16,
			PT_Index32,

			PT_Max,
		};

		enum ePackingUsage
		{
			PS_Invalid=-1,
			PS_SysPosition,
			PS_Position,
			PS_Normal,
			PS_Tangent,
			PS_Binormal,
			PS_TexCoord,
			PS_Color,
			PS_SkinIndices,
			PS_SkinWeights,
			PS_InstanceTransform,
			PS_InstanceLODParams,
			PS_InstanceSkinningData,
			PS_PatchSize,
			PS_PatchBias,
			PS_ExtraData,
			PS_SpT_Attr,

			// This can be used to insert a gap in vertex data.
			PS_Padding,
			PS_PatchOffset,

			PS_Max
		};

		enum eSlotType
		{
			ST_Invalid=-1,
			ST_PerVertex=0,
			ST_PerInstance,

			ST_Max
		};


		// Single packing element
		struct PackingElement
		{
			ePackingType		m_type;
			ePackingUsage		m_usage;
			Uint8				m_usageIndex;
			Uint8				m_slot;
			eSlotType			m_slotType;

			Bool operator ==( const PackingElement& other ) const
			{
				return m_type == other.m_type && m_usage == other.m_usage && m_usageIndex == other.m_usageIndex && m_slot == other.m_slot && m_slotType == other.m_slotType;
			}

			Bool operator!=( const PackingElement &other ) const
			{
				return !operator==( other );
			}

			Bool IsEmpty() const
			{
				return m_type == PT_Invalid && m_usage == PS_Invalid && m_slotType == ST_Invalid;
			}

			// Can be used to denote the end of a list of PackingElements.
			static const PackingElement END_OF_ELEMENTS;
		};

		// Packing stream
		struct PackingStream
		{
			ePackingUsage		m_usage;
			Uint8				m_usageIndex;
			Uint32				m_offset;
			ePackingType		m_format;

			PackingStream()
				: m_usage( PS_Invalid )
				, m_usageIndex( 0 )
				, m_offset( 0 )
				, m_format( PT_Invalid )
			{};

			Bool operator ==( const PackingStream& other ) const
			{
				return m_usage == other.m_usage && m_usageIndex == other.m_usageIndex && m_offset == other.m_offset && m_format == other.m_format;
			}

			Bool operator!=( const PackingStream &other ) const
			{
				return !operator==( other );
			}

		};

		// Vertex layout for source data
		struct PackingVertexLayout
		{
			static const Uint32 USAGE_INDEX_COUNT = 16/*GPUAPI_VERTEX_LAYOUT_USAGE_INDEX_COUNT*/;

			PackingStream		m_streams[ PS_Max * USAGE_INDEX_COUNT ];
			Uint32				m_stride;

			PackingVertexLayout()
				: m_stride( 0 )
			{};

			void Init( Uint32 offset, ePackingUsage usage, Uint8 usageIndex, ePackingType format )
			{
				Uint32 i = usage * USAGE_INDEX_COUNT + usageIndex;
				m_streams[ i ].m_usage = usage;
				m_streams[ i ].m_usageIndex = usageIndex;
				m_streams[ i ].m_format = format;
				m_streams[ i ].m_offset = offset;
			}
		};

		// Packing params
		struct PackingParams
		{
			Float	m_positionPackScaleX;
			Float	m_positionPackScaleY;
			Float	m_positionPackScaleZ;
			Float	m_positionPackOffsetX;
			Float	m_positionPackOffsetY;
			Float	m_positionPackOffsetZ;

			PackingParams()
				: m_positionPackScaleX( 1.0f )
				, m_positionPackScaleY( 1.0f )
				, m_positionPackScaleZ( 1.0f )
				, m_positionPackOffsetX( 0.0f )
				, m_positionPackOffsetY( 0.0f )
				, m_positionPackOffsetZ( 0.0f )
			{}
		};

		/// Data buffer writer
		class IBufferWriter
		{
		public:
			virtual ~IBufferWriter() {};

		public:
			// Get current offset
			virtual Uint32 GetOffset() const=0;

			// Align buffer to given size
			virtual void Align( Uint32 size )=0;

			// Write 32-bit float to buffer
			virtual void WriteFloat( Float data )=0;

			// Write unsigned byte to buffer
			virtual void WriteUnsignedByte( Uint8 data )=0;

			// Write normalized (0.0f-1.0f) value as byte
			virtual void WriteUnsignedByteN( Float data )=0;

			// Write normalized (-1.0f-1.0f) value as byte
			virtual void WriteSignedByteN( Float data )=0;

			// Write unsigned short
			virtual void WriteUnsignedShort( Uint16 data )=0;

			// Write normalized (0.0f-1.0f) short
			virtual void WriteUnsignedShortN( Float data )=0;

			// Write unsigned integer
			virtual void WriteUnsignedInt( Uint32 data )=0;

			// Write float16
			virtual void WriteFloat16( Float f )=0;

			// Write DEC3N trio
			virtual void WriteDec3N( Float x, Float y, Float z )=0;

			// Write DEC3N trio
			virtual void WriteDec4N( Float x, Float y, Float z, Float w )=0;

			// Write color
			virtual void WriteColor( Uint32 color )=0;
		};

		/// Data buffer writer
		class IBufferReader
		{
		public:
			virtual ~IBufferReader() {};

		public:
			// Get current offset
			virtual Uint32 GetOffset() const=0;

			// Align buffer to given size
			virtual void Align( Uint32& size )=0;

			// Read 32-bit float to buffer
			virtual void ReadFloat( Float& data )=0;

			// Read unsigned byte to buffer
			virtual void ReadUnsignedByte( Uint8& data )=0;

			// Read normalized (0.0f-1.0f) value as byte
			virtual void ReadUnsignedByteN( Float& data )=0;

			// Read normalized (-1.0f-1.0f) value as byte
			virtual void ReadSignedByteN( Float& data )=0;

			// Read unsigned short
			virtual void ReadUnsignedShort( Uint16& data )=0;

			// Read normalized (0.0f-1.0f) short
			virtual void ReadUnsignedShortN( Float& data )=0;

			// Read unsigned integer
			virtual void ReadUnsignedInt( Uint32& data )=0;

			// Read float16
			virtual void ReadFloat16( Float& f )=0;

			// Read DEC3N trio
			virtual void ReadDec3N( Float& x, Float& y, Float& z )=0;

			// Read DEC3N trio
			virtual void ReadDec4N( Float& x, Float& y, Float& z, Float& w )=0;

			// Read color
			virtual void ReadColor( Uint32& color )=0;
		};

		//! Initialize packing params from range of values
		void InitPackingParams( PackingParams& packParams, Float minX, Float minY, Float minZ, Float maxX, Float maxY, Float maxZ );

		//! Calculate elements size
		Uint32 GetPackingTypeSize( ePackingType type );

		//! Calculate size of element list
		Uint32 CalcElementListSize( const PackingElement* elements );

		//! Get number of slots used by a set of packing elements, including empties (if slots 0 and 2 are used, will return 3).
		Uint32 GetStreamCount( const PackingElement* elements );

		//! Calculate sizes of element list. elements should not use any slots outside the range given by numSizes.
		void CalcElementListSizes( const PackingElement* elements, Uint32* sizes, Uint32 numSizes );

		//! Pack vertex data from vertex stream
		void PackElement( IBufferWriter* writer, const PackingElement& element, const PackingParams& params, const PackingVertexLayout& vertexLayout, const void* vertexData );

		//! Pack vertices into buffer
		void PackVertices( IBufferWriter* writer, const PackingElement* elements, const PackingParams& packParams, const PackingVertexLayout& vertexLayout, const void* vertexData, Uint32 numVertices, Uint32 slotIndex );

		//! Pack indices into buffer
		void PackIndices( IBufferWriter* writer, ePackingType type, const void* indexData, Uint32 numIndices, const Uint32 baseVertexIndex = 0 );

		//! Unpack vertex data from vertex stream
		void UnpackElement( IBufferReader* reader, const PackingElement& element, const PackingParams& params, const PackingVertexLayout& vertexLayout, void* vertexData );

		//! Unpack vertices into buffer
		void UnpackVertices( IBufferReader* reader, const PackingElement* elements, const PackingParams& packParams, const PackingVertexLayout& vertexLayout, void* vertexData, Uint32 numVertices, Uint32 slotIndex );

	}
};
