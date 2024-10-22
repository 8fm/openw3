#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler For Durango 9.30.12098.0
//
//
///
// Buffer Definitions: 
//
// cbuffer Constants
// {
//
//   float4 cxadd;                      // Offset:    0 Size:    16
//   float4 cxmul;                      // Offset:   16 Size:    16
//   float4 mvp[2];                     // Offset:   32 Size:    32
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// Constants                         cbuffer      NA          NA    0        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float   xyzw
// TEXCOORD                 0   xy          1     NONE   float   xy  
// SV_Position              0   xyzw        2     NONE   float   xyzw
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// TEXCOORD                 0   xy          0     NONE   float   xy  
// COLOR                    0   xyzw        1     NONE   float   xyzw
// SV_Position              0   xyzw        2      POS   float   xyzw
//
vs_5_0
Opaque Custom Data - XBOX Precompiled Shader Header
// First Precompiled Shader at offset:[72]
// Embedded Data:
//  0x00000048 - Offset To First Precompiled Shader
//  0x00010050 - Original Shader Descriptor
//  0x00000043 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_constantbuffer cb0[4], immediateIndexed
dcl_input v0.xyzw
dcl_input v1.xy
dcl_input v2.xyzw
dcl_output o0.xy
dcl_output o1.xyzw
dcl_output_siv o2.xyzw, position
mov o0.xy, v1.xyxx
mad o1.xyzw, v0.xyzw, cb0[1].xyzw, cb0[0].xyzw
dp4 o2.x, v2.xyzw, cb0[2].xyzw
dp4 o2.y, v2.xyzw, cb0[3].xyzw
mov o2.zw, l(0,0,0,1.000000)
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[72], bundle is:[149] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (VS)
// Shader Hw Stage           : (VS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\VText.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- VS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, v[4:7]
;   [1] generic, usageIdx  1, channelMask  3, v[8:9]
;   [2] generic, usageIdx  2, channelMask 15, v[12:15]
; Output Semantic Mappings
;   [0] generic, usageIdx  0, paramIdx 0, paramSlot 0
;   [1] generic, usageIdx  1, paramIdx 1, paramSlot 0

codeLenInByte        = 136;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 3;
;  userElements[0]      = PTR_VERTEX_BUFFER_TABLE, Element=1 dwords, s[2:3]
;  userElements[1]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[2:3]
;  userElements[2]      = SUB_PTR_FETCH_SHADER, s[2:3]
extUserElementCount  = 1;
;  extUserElements 1[0] = IMM_CONST_BUFFER, 0, offset 0:3 dwords
NumVgprs             = 16;
NumSgprs             = 20;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000000
; constBufUsage           = 0x00000001

; SPI_SHADER_PGM_RSRC2_VS = 0x00000008
SSPRV:USER_SGPR             = 4
; PA_CL_VS_OUT_CNTL       = 0x00000000
; SPI_VS_OUT_CONFIG       = 0x00000002
SVOC:VS_EXPORT_COUNT        = 1
SVOC:VS_HALF_PACK           = 0
SVOC:VS_EXPORTS_FOG         = 0
SVOC:VS_OUT_FOG_VEC_ADDR    = 0
; SPI_SHADER_POS_FORMAT     = 0x00000004
SSPF:POS0_EXPORT_FORMAT     = 4
SSPF:POS1_EXPORT_FORMAT     = 0
SSPF:POS2_EXPORT_FORMAT     = 0
SSPF:POS3_EXPORT_FORMAT     = 0
VGT_STRMOUT_CONFIG = 0x0
VGT_STRMOUT_CONFIG:RAST_STREAM = 0
VGT_STRMOUT_CONFIG:STREAMOUT_0_EN = 0
VGT_STRMOUT_CONFIG:STREAMOUT_1_EN = 0
VGT_STRMOUT_CONFIG:STREAMOUT_2_EN = 0
VGT_STRMOUT_CONFIG:STREAMOUT_3_EN = 0
; exportVertexSize          = 4

// Shader Instructions:
shader main
  asic(CI)
  type(VS)
  s_swappc_b64  s[2:3], s[2:3]                          // 000000000000: BE822102
  s_load_dwordx4  s[8:11], s[2:3], 0x00                 // 000000000004: C0840300
  s_waitcnt     lgkmcnt(0)                              // 000000000008: BF8C007F
  s_buffer_load_dwordx8  s[12:19], s[8:11], 0x08        // 00000000000C: C2C60908
  s_waitcnt     vmcnt(0) & lgkmcnt(0)                   // 000000000010: BF8C0070
  v_mul_legacy_f32  v0, s15, v15                        // 000000000014: 0E001E0F
  v_mul_legacy_f32  v1, s19, v15                        // 000000000018: 0E021E13
  v_mac_legacy_f32  v0, s14, v14                        // 00000000001C: 0C001C0E
  v_mac_legacy_f32  v1, s18, v14                        // 000000000020: 0C021C12
  v_mac_legacy_f32  v0, s13, v13                        // 000000000024: 0C001A0D
  v_mac_legacy_f32  v1, s17, v13                        // 000000000028: 0C021A11
  v_mac_legacy_f32  v0, s12, v12                        // 00000000002C: 0C00180C
  v_mac_legacy_f32  v1, s16, v12                        // 000000000030: 0C021810
  v_mov_b32     v2, 0                                   // 000000000034: 7E040280
  v_mov_b32     v3, 1.0                                 // 000000000038: 7E0602F2
  exp           pos0, v0, v1, v2, v3 done               // 00000000003C: F80008CF 03020100
  s_buffer_load_dwordx8  s[0:7], s[8:11], 0x00          // 000000000044: C2C00900
  s_waitcnt     expcnt(0) & lgkmcnt(0)                  // 000000000048: BF8C000F
  v_mov_b32     v0, s0                                  // 00000000004C: 7E000200
  v_mov_b32     v1, s1                                  // 000000000050: 7E020201
  v_mac_legacy_f32  v0, s4, v4                          // 000000000054: 0C000804
  v_mac_legacy_f32  v1, s5, v5                          // 000000000058: 0C020A05
  v_mov_b32     v2, s2                                  // 00000000005C: 7E040202
  v_mov_b32     v3, s3                                  // 000000000060: 7E060203
  v_mac_legacy_f32  v2, s6, v6                          // 000000000064: 0C040C06
  v_mac_legacy_f32  v3, s7, v7                          // 000000000068: 0C060E07
  v_mov_b32     v4, 0                                   // 00000000006C: 7E080280
  v_mov_b32     v5, 1.0                                 // 000000000070: 7E0A02F2
  exp           param0, v8, v9, v4, v5                  // 000000000074: F800020F 05040908
  exp           param1, v0, v1, v2, v3                  // 00000000007C: F800021F 03020100
  s_endpgm                                              // 000000000084: BF810000
end


Opaque Custom Data - XBOX Precompiled Shader
// Offset:[223], bundle is:[161] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (VS)
// Shader Hw Stage           : (ES)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\VText.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- VS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, v[4:7]
;   [1] generic, usageIdx  1, channelMask  3, v[8:9]
;   [2] generic, usageIdx  2, channelMask 15, v[12:15]
; Output Semantic Mappings
;   [0] position, usageIdx  2, paramIdx 2, paramSlot 0
;   [1] generic, usageIdx  0, paramIdx 0, paramSlot 0
;   [2] generic, usageIdx  1, paramIdx 1, paramSlot 0

codeLenInByte        = 196;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 4;
;  userElements[0]      = PTR_VERTEX_BUFFER_TABLE, Element=1 dwords, s[2:3]
;  userElements[1]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[2:3]
;  userElements[2]      = PTR_INTERNAL_GLOBAL_TABLE, Element=1 dwords, s[2:3]
;  userElements[3]      = SUB_PTR_FETCH_SHADER, s[2:3]
extUserElementCount  = 1;
;  extUserElements 1[0] = IMM_CONST_BUFFER, 0, offset 0:3 dwords
NumVgprs             = 16;
NumSgprs             = 20;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000000
; constBufUsage           = 0x00000001

; SPI_SHADER_PGM_RSRC2_VS = 0x00000008
SSPRV:USER_SGPR             = 4
; PA_CL_VS_OUT_CNTL       = 0x00000000
; SPI_VS_OUT_CONFIG       = 0x00000000
SVOC:VS_EXPORT_COUNT        = 0
SVOC:VS_HALF_PACK           = 0
SVOC:VS_EXPORTS_FOG         = 0
SVOC:VS_OUT_FOG_VEC_ADDR    = 0
; SPI_SHADER_POS_FORMAT     = 0x00000004
SSPF:POS0_EXPORT_FORMAT     = 4
SSPF:POS1_EXPORT_FORMAT     = 0
SSPF:POS2_EXPORT_FORMAT     = 0
SSPF:POS3_EXPORT_FORMAT     = 0
VGT_STRMOUT_CONFIG = 0x0
VGT_STRMOUT_CONFIG:RAST_STREAM = 0
VGT_STRMOUT_CONFIG:STREAMOUT_0_EN = 0
VGT_STRMOUT_CONFIG:STREAMOUT_1_EN = 0
VGT_STRMOUT_CONFIG:STREAMOUT_2_EN = 0
VGT_STRMOUT_CONFIG:STREAMOUT_3_EN = 0
; exportVertexSize          = 12
; esGsRingItemSize          = 12

// Shader Instructions:
shader main
  asic(CI)
  type(VS)
  s_swappc_b64  s[2:3], s[2:3]                          // 000000000000: BE822102
  s_load_dwordx4  s[8:11], s[2:3], 0x00                 // 000000000004: C0840300
  s_waitcnt     lgkmcnt(0)                              // 000000000008: BF8C007F
  s_buffer_load_dwordx8  s[12:19], s[8:11], 0x00        // 00000000000C: C2C60900
  s_waitcnt     lgkmcnt(0)                              // 000000000010: BF8C007F
  v_mov_b32     v2, s12                                 // 000000000014: 7E04020C
  v_mov_b32     v3, s13                                 // 000000000018: 7E06020D
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 00000000001C: BF8C0F70
  v_mac_legacy_f32  v2, s16, v4                         // 000000000020: 0C040810
  v_mac_legacy_f32  v3, s17, v5                         // 000000000024: 0C060A11
  v_mov_b32     v4, s14                                 // 000000000028: 7E08020E
  v_mov_b32     v5, s15                                 // 00000000002C: 7E0A020F
  s_buffer_load_dwordx8  s[8:15], s[8:11], 0x08         // 000000000030: C2C40908
  v_mac_legacy_f32  v4, s18, v6                         // 000000000034: 0C080C12
  s_waitcnt     lgkmcnt(0)                              // 000000000038: BF8C007F
  v_mul_legacy_f32  v6, s11, v15                        // 00000000003C: 0E0C1E0B
  s_load_dwordx4  s[0:3], s[2:3], 0x0c                  // 000000000040: C080030C
  v_mac_legacy_f32  v6, s10, v14                        // 000000000044: 0C0C1C0A
  v_mac_legacy_f32  v6, s9, v13                         // 000000000048: 0C0C1A09
  v_mac_legacy_f32  v6, s8, v12                         // 00000000004C: 0C0C1808
  s_waitcnt     lgkmcnt(0)                              // 000000000050: BF8C007F
  buffer_store_dword  v6, v0, s[0:3], s4 offset:32 glc slc // 000000000054: E0704020 04400600
  v_mul_legacy_f32  v6, s15, v15                        // 00000000005C: 0E0C1E0F
  v_mac_legacy_f32  v6, s14, v14                        // 000000000060: 0C0C1C0E
  v_mac_legacy_f32  v6, s13, v13                        // 000000000064: 0C0C1A0D
  v_mac_legacy_f32  v6, s12, v12                        // 000000000068: 0C0C180C
  v_mov_b32     v0, 0                                   // 00000000006C: 7E000280
  v_mov_b32     v1, 1.0                                 // 000000000070: 7E0202F2
  buffer_store_dword  v6, v0, s[0:3], s4 offset:36 glc slc // 000000000074: E0704024 04400600
  buffer_store_dword  v0, v0, s[0:3], s4 offset:40 glc slc // 00000000007C: E0704028 04400000
  buffer_store_dword  v1, v0, s[0:3], s4 offset:44 glc slc // 000000000084: E070402C 04400100
  buffer_store_dword  v8, v0, s[0:3], s4 glc slc        // 00000000008C: E0704000 04400800
  buffer_store_dword  v9, v0, s[0:3], s4 offset:4 glc slc // 000000000094: E0704004 04400900
  v_mac_legacy_f32  v5, s19, v7                         // 00000000009C: 0C0A0E13
  buffer_store_dword  v2, v0, s[0:3], s4 offset:16 glc slc // 0000000000A0: E0704010 04400200
  buffer_store_dword  v3, v0, s[0:3], s4 offset:20 glc slc // 0000000000A8: E0704014 04400300
  buffer_store_dword  v4, v0, s[0:3], s4 offset:24 glc slc // 0000000000B0: E0704018 04400400
  buffer_store_dword  v5, v0, s[0:3], s4 offset:28 glc slc // 0000000000B8: E070401C 04400500
  s_endpgm                                              // 0000000000C0: BF810000
end


Opaque Custom Data - XBOX Precompiled Shader
// Offset:[386], bundle is:[157] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (VS)
// Shader Hw Stage           : (LS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\VText.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- VS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, v[4:7]
;   [1] generic, usageIdx  1, channelMask  3, v[8:9]
;   [2] generic, usageIdx  2, channelMask 15, v[12:15]
; Output Semantic Mappings
;   [0] position, usageIdx  2, paramIdx 2, paramSlot 0
;   [1] generic, usageIdx  0, paramIdx 0, paramSlot 0
;   [2] generic, usageIdx  1, paramIdx 1, paramSlot 0

codeLenInByte        = 160;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 3;
;  userElements[0]      = PTR_VERTEX_BUFFER_TABLE, Element=1 dwords, s[2:3]
;  userElements[1]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[2:3]
;  userElements[2]      = SUB_PTR_FETCH_SHADER, s[2:3]
extUserElementCount  = 1;
;  extUserElements 1[0] = IMM_CONST_BUFFER, 0, offset 0:3 dwords
NumVgprs             = 16;
NumSgprs             = 20;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000000
; constBufUsage           = 0x00000001

; SPI_SHADER_PGM_RSRC2_VS = 0x00000008
SSPRV:USER_SGPR             = 4
; PA_CL_VS_OUT_CNTL       = 0x00000000
; SPI_VS_OUT_CONFIG       = 0x00000000
SVOC:VS_EXPORT_COUNT        = 0
SVOC:VS_HALF_PACK           = 0
SVOC:VS_EXPORTS_FOG         = 0
SVOC:VS_OUT_FOG_VEC_ADDR    = 0
; SPI_SHADER_POS_FORMAT     = 0x00000004
SSPF:POS0_EXPORT_FORMAT     = 4
SSPF:POS1_EXPORT_FORMAT     = 0
SSPF:POS2_EXPORT_FORMAT     = 0
SSPF:POS3_EXPORT_FORMAT     = 0
VGT_STRMOUT_CONFIG = 0x0
VGT_STRMOUT_CONFIG:RAST_STREAM = 0
VGT_STRMOUT_CONFIG:STREAMOUT_0_EN = 0
VGT_STRMOUT_CONFIG:STREAMOUT_1_EN = 0
VGT_STRMOUT_CONFIG:STREAMOUT_2_EN = 0
VGT_STRMOUT_CONFIG:STREAMOUT_3_EN = 0
; vgprCompCnt               = 1
; exportVertexSize          = 12
; lsStride                  = 48

// Shader Instructions:
shader main
  asic(CI)
  type(VS)
  s_swappc_b64  s[2:3], s[2:3]                          // 000000000000: BE822102
  s_load_dwordx4  s[0:3], s[2:3], 0x00                  // 000000000004: C0800300
  s_waitcnt     lgkmcnt(0)                              // 000000000008: BF8C007F
  s_buffer_load_dwordx8  s[4:11], s[0:3], 0x00          // 00000000000C: C2C20100
  s_buffer_load_dwordx8  s[12:19], s[0:3], 0x08         // 000000000010: C2C60108
  s_waitcnt     lgkmcnt(0)                              // 000000000014: BF8C007F
  v_mov_b32     v2, s5                                  // 000000000018: 7E040205
  v_mov_b32     v3, s6                                  // 00000000001C: 7E060206
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000020: BF8C0F70
  v_mac_legacy_f32  v2, s9, v5                          // 000000000024: 0C040A09
  v_mac_legacy_f32  v3, s10, v6                         // 000000000028: 0C060C0A
  v_mul_legacy_f32  v5, s15, v15                        // 00000000002C: 0E0A1E0F
  v_mul_legacy_f32  v6, s19, v15                        // 000000000030: 0E0C1E13
  v_mac_legacy_f32  v5, s14, v14                        // 000000000034: 0C0A1C0E
  v_mac_legacy_f32  v6, s18, v14                        // 000000000038: 0C0C1C12
  v_mac_legacy_f32  v5, s13, v13                        // 00000000003C: 0C0A1A0D
  v_mac_legacy_f32  v6, s17, v13                        // 000000000040: 0C0C1A11
  s_mov_b32     m0, 0x00010000                          // 000000000044: BEFC03FF 00010000
  v_mac_legacy_f32  v5, s12, v12                        // 00000000004C: 0C0A180C
  v_mac_legacy_f32  v6, s16, v12                        // 000000000050: 0C0C1810
  v_mul_lo_i32  v1, v1, 48                              // 000000000054: D2D60001 00016101
  ds_write2_b32  v1, v5, v6 offset0:8 offset1:9         // 00000000005C: D8380908 00060501
  v_mov_b32     v0, s4                                  // 000000000064: 7E000204
  v_mov_b32     v5, 0                                   // 000000000068: 7E0A0280
  v_mov_b32     v6, 1.0                                 // 00000000006C: 7E0C02F2
  v_mac_legacy_f32  v0, s8, v4                          // 000000000070: 0C000808
  ds_write2_b32  v1, v5, v6 offset0:10 offset1:11       // 000000000074: D8380B0A 00060501
  v_mov_b32     v4, s7                                  // 00000000007C: 7E080207
  ds_write2_b32  v1, v8, v9 offset1:1                   // 000000000080: D8380100 00090801
  v_mac_legacy_f32  v4, s11, v7                         // 000000000088: 0C080E0B
  ds_write2_b32  v1, v0, v2 offset0:4 offset1:5         // 00000000008C: D8380504 00020001
  ds_write2_b32  v1, v3, v4 offset0:6 offset1:7         // 000000000094: D8380706 00040301
  s_endpgm                                              // 00000000009C: BF810000
end


// Approximately 6 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_VText[] =
{
     68,  88,  66,  67, 178, 254, 
    180, 162,  84,  24, 178,  94, 
    197,  81, 255,  40, 117,  65, 
     78, 156,   1,   0,   0,   0, 
    228,  11,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    212,   1,   0,   0,  72,   2, 
      0,   0, 188,   2,   0,   0, 
     72,  11,   0,   0,  82,  68, 
     69,  70, 152,   1,   0,   0, 
      1,   0,   0,   0, 104,   0, 
      0,   0,   1,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    254, 255,   0, 129,   0,   0, 
     92,   1,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
     92,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,  67, 111, 110, 115, 
    116,  97, 110, 116, 115,   0, 
    171, 171,  92,   0,   0,   0, 
      3,   0,   0,   0, 128,   0, 
      0,   0,  64,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0, 248,   0,   0,   0, 
      0,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
      8,   1,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
     44,   1,   0,   0,  16,   0, 
      0,   0,  16,   0,   0,   0, 
      2,   0,   0,   0,   8,   1, 
      0,   0,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,  50,   1, 
      0,   0,  32,   0,   0,   0, 
     32,   0,   0,   0,   2,   0, 
      0,   0,  56,   1,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,  99, 120,  97, 100, 
    100,   0, 102, 108, 111,  97, 
    116,  52,   0, 171, 171, 171, 
      1,   0,   3,   0,   1,   0, 
      4,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0, 254,   0,   0,   0, 
     99, 120, 109, 117, 108,   0, 
    109, 118, 112,   0, 171, 171, 
      1,   0,   3,   0,   1,   0, 
      4,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0, 254,   0,   0,   0, 
     77, 105,  99, 114, 111, 115, 
    111, 102, 116,  32,  40,  82, 
     41,  32,  72,  76,  83,  76, 
     32,  83, 104,  97, 100, 101, 
    114,  32,  67, 111, 109, 112, 
    105, 108, 101, 114,  32,  70, 
    111, 114,  32,  68, 117, 114, 
     97, 110, 103, 111,  32,  57, 
     46,  51,  48,  46,  49,  50, 
     48,  57,  56,  46,  48,   0, 
     73,  83,  71,  78, 108,   0, 
      0,   0,   3,   0,   0,   0, 
      8,   0,   0,   0,  80,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,  15,   0,   0,  86,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
      3,   3,   0,   0,  95,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   2,   0,   0,   0, 
     15,  15,   0,   0,  67,  79, 
     76,  79,  82,   0,  84,  69, 
     88,  67,  79,  79,  82,  68, 
      0,  83,  86,  95,  80, 111, 
    115, 105, 116, 105, 111, 110, 
      0, 171,  79,  83,  71,  78, 
    108,   0,   0,   0,   3,   0, 
      0,   0,   8,   0,   0,   0, 
     80,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   3,  12,   0,   0, 
     89,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,   0,   0,   0, 
     95,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
      0,   0,  15,   0,   0,   0, 
     84,  69,  88,  67,  79,  79, 
     82,  68,   0,  67,  79,  76, 
     79,  82,   0,  83,  86,  95, 
     80, 111, 115, 105, 116, 105, 
    111, 110,   0, 171,  83,  72, 
     69,  88, 132,   8,   0,   0, 
     80,   0,   1,   0,  33,   2, 
      0,   0,  53,  16,   0,   0, 
      5,   0,   0,   0,  72,   0, 
      0,   0,  80,   0,   1,   0, 
     67,   0,   0,   0, 106,   8, 
      0,   1,  89,   0,   0,   4, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
     95,   0,   0,   3, 242,  16, 
     16,   0,   0,   0,   0,   0, 
     95,   0,   0,   3,  50,  16, 
     16,   0,   1,   0,   0,   0, 
     95,   0,   0,   3, 242,  16, 
     16,   0,   2,   0,   0,   0, 
    101,   0,   0,   3,  50,  32, 
     16,   0,   0,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   1,   0,   0,   0, 
    103,   0,   0,   4, 242,  32, 
     16,   0,   2,   0,   0,   0, 
      1,   0,   0,   0,  54,   0, 
      0,   5,  50,  32,  16,   0, 
      0,   0,   0,   0,  70,  16, 
     16,   0,   1,   0,   0,   0, 
     50,   0,   0,  11, 242,  32, 
     16,   0,   1,   0,   0,   0, 
     70,  30,  16,   0,   0,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  17,   0,   0,   8, 
     18,  32,  16,   0,   2,   0, 
      0,   0,  70,  30,  16,   0, 
      2,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,  17,   0, 
      0,   8,  34,  32,  16,   0, 
      2,   0,   0,   0,  70,  30, 
     16,   0,   2,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
     54,   0,   0,   8, 194,  32, 
     16,   0,   2,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    128,  63,  62,   0,   0,   1, 
     53,  16,   0,   0, 151,   0, 
      0,   0,  21,   0,   1,   0, 
     32,  76,  22,   0, 120,   1, 
    237, 152,  61, 111,  19,  49, 
     24, 199, 159,  56,  23, 231, 
    146, 186, 151, 164,  45, 144, 
    134, 144, 168,  91, 197,  16, 
     26,  94,  36, 196, 212, 161, 
     18,  11,  18,  21,  65,  44, 
    220, 192,  65,  47,  45,  34, 
     77, 163,  75,  90,  69,  21, 
     10, 229,  77,  98, 224,   3, 
     48, 244,  99,  48,  49, 208, 
    208, 129, 181,   3,   3, 159, 
    128,  17,   6, 166, 108, 229, 
    255,  36,  54,  58,   1, 157, 
     65, 194,  63, 233, 103, 159, 
    207, 143, 125,  62, 219, 211, 
    115, 227,  52, 141, 233, 124, 
    126, 247, 141, 235,  47, 119, 
     87,  31, 171,   4,  81, 138, 
     27, 192, 225,   2, 116, 230, 
    184,  36, 186, 122, 138,  75, 
    162,  69,  46,  16,  71, 130, 
     40, 143, 138, 187,  95, 193, 
    101, 200, 125, 198,   9, 203, 
    195,  58,  74,  15, 114, 220, 
    121, 140, 187, 142, 186,  12, 
    147, 144, 223, 185, 144, 159, 
    127, 197, 124, 159,  63,  21, 
    135, 227,  21, 140,  99, 198, 
    255,  54, 207,   2,  23, 127, 
    166,  10, 121, 253,  39, 193, 
    107,  51, 240, 188, 252,  93, 
     38, 254, 254,  95, 129, 247, 
    200, 156,  19, 195, 231,  17, 
    135, 247, 223, 236, 205,  72, 
    199, 153, 125,  93, 212, 231, 
    106, 206, 114,   8,  79, 226, 
     24, 152, 115,  49, 152,  54, 
    207,  47, 233, 230, 146,  76, 
     60, 207,  75, 113, 180,  36, 
    241,  66, 162,  83, 226,  66, 
     73,   9, 211,  16, 155,  40, 
     51,  48,  11, 167,  32,  14, 
     82,  78,  67,  44,  80, 230, 
     32,  22,  33,  11, 112,   6, 
    206,  66,  44,  72,  98, 125, 
     18, 119,  85, 158, 129,  69, 
     56,  15,  75, 240,  44, 196, 
     69, 146, 231,  96,   5, 226, 
     64, 113,  37, 177, 134,   9, 
     50,  97, 254, 240, 255, 130, 
    247, 192, 220,  85, 126,  54, 
    231, 195, 196, 159,  45,  22, 
    139, 197,  98, 177,  88,  44, 
     22, 139, 197,  98, 177,  88, 
    254,  22,  98, 225, 217, 123, 
     74, 190,  24,  62, 161, 215, 
      7, 110, 230, 227,  97,   7, 
    117, 174,  66, 222, 108,  69, 
    120,  94, 153, 212,  76,  89, 
    168, 233,  18, 169,  66,  73, 
     40,  85,  36, 149,  47,  10, 
    181,  39, 156, 193, 119,  33, 
      7,  71,  46, 141,  40,  33, 
    146, 148,  25,  30, 230,  48, 
    142,   4,  13,  18,  66,  12, 
     28, 151,  84,  42,  43, 148, 
     64,  92,  18, 113,  82,  57, 
     42, 237,  73, 140, 115,  49, 
     46,  59, 200,   9,  26, 185, 
     25,  39,  85,  69,  61,  30, 
     79,  79,  15, 116,  90, 108, 
    156,  67,  97, 231,  33, 103, 
    148,  76, 155, 243, 101, 140, 
    105, 179,   6,   7, 161, 247, 
    244,   4, 171, 186, 230, 120, 
    206, 191, 120,  63,  51, 111, 
     19, 246, 116, 127,  95, 215, 
    156, 187, 153, 130,  95, 231, 
    118,  63, 125, 120, 185, 189, 
    191, 175, 223, 191, 209,  53, 
    207, 145, 133, 133,  11, 199, 
     87, 210,  58, 135, 251,  86, 
    247,  53,  32, 143, 111,  94, 
    243, 239,   7, 173,  96, 183, 
     91, 235, 109,  69,  91, 143, 
    106, 221,  94,  20,   6, 155, 
     23, 253, 181, 112, 199,  15, 
    251, 189,  48, 106,   7,  45, 
    127, 189, 217, 191, 236,  55, 
    162,   7, 254, 173, 176, 189, 
     22,  70, 254, 202, 165, 149, 
    122, 223, 111, 108,   4, 104, 
    116, 253,  59, 183,  17,  88, 
    107, 182, 234, 245, 218,  70, 
    171, 219, 194, 164,  19, 170, 
    250, 147, 156,  38,  78, 195, 
    205, 224,  97, 155, 126,   0, 
     32, 241,  91, 230,  53,  16, 
      0,   0, 163,   0,   0,   0, 
     21,   0,   1,   0,  16, 152, 
     22,   0, 120,   1, 237, 152, 
    205,  78,  83,  65,  24, 134, 
    167, 167, 195, 199, 105,  25, 
    218, 242, 163,  22, 228,  71, 
     18,  19,  89, 144,  10,  88, 
     19, 227, 234,  44,  88, 184, 
    211,  88, 227, 198, 179, 169, 
    210, 138, 161,  20, 210,  86, 
    211,  16,  83,  81, 113, 199, 
      5, 184, 224,  18, 188,   6, 
    169, 132, 184,  50, 113, 225, 
     13,  24, 163, 137,  27,  55, 
    222,   0, 190,  47, 204,  52, 
    196, 200,  90,  19, 231,  73, 
    158, 249,  58, 103, 230,  76, 
    167, 223,  76,  55, 223, 235, 
    179, 234, 136, 201, 197,  47, 
    115, 140, 223, 238, 221, 122, 
    106,  18,  74, 245, 177,   3, 
      2,  54,  96,  99, 148, 173, 
     82, 219, 103, 216,  42,  53, 
    203,   6, 243,  56,  33, 135, 
    192, 225,   3,  24,  65, 142, 
     57, 143, 137, 186, 215, 208, 
    230, 225,  57,  88, 197,  59, 
     55,  16,  39,  97,  26,  14, 
    195,   1, 248,  59,  28,  19, 
     72, 248,  85,  39, 225, 115, 
      3,  79, 162, 217,   0, 190, 
    151, 132,  61, 102, 216, 252, 
    153,  75, 144, 251,  63,  13, 
    238, 141, 235,  57, 220,  62, 
    109,  58, 254,  41, 152,  35, 
    119,  78, 132, 231, 113, 146, 
     12, 116,  57,  42, 218, 115, 
    116, 121, 109, 217, 190,  59, 
    203,  46,  60, 141,  67, 224, 
    214,  33, 252,  92, 132, 132, 
    121,  23, 117, 115,  94,  18, 
     47, 115,  18, 124, 156,  23, 
     60,  16,  76,  16,  92,  40, 
    193, 161,  73,  63,  12,  97, 
     10,  34, 177, 130, 132,  10, 
     14,  82,   6,  33,  54,  40, 
     89, 136,  77, 200,  16,  68, 
    242, 101,   4,  98,  67, 130, 
    253,   9, 238, 170, 224,   2, 
      9,  46, 146, 140, 193, 113, 
    120,  30,  78,  64,  92,  38, 
    153, 130, 211, 246, 236,   3, 
     92,  50, 192, 123,  66,  36, 
    225, 126, 233, 255,   1, 115, 
    128,  52, 247, 224,  25,  57, 
    248, 191, 161,  30, 143, 199, 
    227, 241, 120,  60,  30, 143, 
    199, 227, 241, 120,  60, 127, 
    147,  96, 230, 197,  91, 149, 
    220, 238,  62,  83,  59, 123, 
     42, 245, 126, 159, 209,   4, 
    186,  51,  24,  72, 103,  35, 
    187, 179, 151,  11, 181,  25, 
     74, 139, 201,   4,  97,  39, 
     27, 164,  59,  97, 234,  96, 
    127, 216, 132, 134, 243,   6, 
    166,  76, 198,  36, 183, 186, 
    233,   9,  99,  82, 227, 198, 
    132, 121, 115, 244, 252,  66, 
    180, 241,  89,  73, 164, 179, 
     24, 207,  96, 108,  16,  99, 
      6,  99,  91, 129, 234, 252, 
     12, 130, 206,  69,  59,  62, 
    203, 168,  34,  61, 199, 152, 
    136, 180,  98,  12,  35, 173, 
     25,  83, 145,  30, 201, 164, 
     77, 142, 159, 131,  72, 143, 
     50,  38,  35, 157, 103, 212, 
    145, 158,  96, 236, 195,  59, 
    234, 249, 158,  45, 169,  29, 
    213,  80, 233,  24, 100,  21, 
    202, 245,  89,  15,  37, 174, 
    207, 218,  27, 113, 125, 234, 
    192, 130, 137,  55, 118, 193, 
     93,  27,  57,  31, 207,  85, 
    166,  87, 197,  59, 230, 131, 
     29,  63, 176, 145, 117,  32, 
    214,  45, 127, 140, 110, 126, 
    122, 247, 234, 241, 238, 119, 
    251, 252, 171, 141,  92, 131, 
     53, 206, 161, 203, 135,  87, 
     75, 182,  30, 172, 108,  44, 
     65, 190,  95, 189,  30, 223, 
     47, 215, 202, 155, 205,  66, 
    107, 189, 177, 190,  90, 104, 
    182,  26, 149, 242, 218,  98, 
    188,  92, 121,  18,  87, 218, 
    173,  74, 163,  94, 174, 197, 
     15, 171, 237,  98,  92, 106, 
     60, 136, 111,  87, 234, 203, 
    149,  70, 188, 116, 101, 105, 
    161,  29, 151,  86, 202, 232, 
     52, 227, 187, 119,  48, 177, 
     80, 173,  45,  44,  20,  86, 
    106, 205,  26,  22,  61, 102, 
    213, 126,  21,  75, 206, 253, 
    112, 173, 252, 168, 174, 126, 
      1,  36, 181, 112,  97,   0, 
      0,   0,  53,  16,   0,   0, 
    159,   0,   0,   0,  21,   0, 
      1,   0,   0, 100,  22,   0, 
    120,   1, 237, 152,  49, 111, 
    211,  80,  16, 199, 207,  78, 
    114, 113, 156,  87,  39, 105, 
     11, 164, 165, 164, 234, 198, 
     20,  26,  40,   2,  49, 117, 
    168, 196,  72,  69,  16,  11, 
     94,  12, 113,   8,  34,  77, 
    171, 164, 160, 168,  66, 166, 
    128, 216,  58,  48, 130, 196, 
    192,  71,  96,  96, 110,  74, 
      6,  38,   4,  18,  72, 136, 
     47, 192, 200, 194, 140,  84, 
    254, 215, 188,   7, 165, 162, 
     51,  72, 188, 159, 244, 191, 
    243, 243, 179, 207, 151, 187, 
    151, 229,  26,  71, 105, 143, 
    103, 239,  10, 129, 248,  47, 
    215, 150, 239,  41, 135,  40, 
     35, 139, 125, 172,  77, 138, 
     37,  90,  62,  34, 150, 232, 
    164,  24,  60,  71,  46,  81, 
     17,  78, 182,  95,  64, 139, 
    144, 236,  25, 141,  88, 220, 
     89, 128,  29, 135, 202,  80, 
     19, 239,  93, 132,  63,  14, 
     49,  52,   1, 229,  32, 225, 
    224, 119, 211,  98,   0,  62, 
    243,  27, 242, 158, 130,  36, 
      5,  67,  74,  12,  56,  24, 
    131, 230, 196, 252,  25, 217, 
    146, 252,  15,  67, 114,  19, 
     76,  76, 147, 167,  46, 199, 
     63, 133, 212, 194, 244,  73, 
    144, 126, 236,  71,  26, 108, 
    106,  84, 212, 125,  52, 245, 
     91, 212, 107, 211, 203,  29, 
    232,  48, 118, 129, 233, 139, 
    193, 172,  37,  62, 211, 165, 
    121, 118,  30,  21, 217, 125, 
     63, 207, 184, 193, 216, 100, 
     20, 144, 209,  52, 206,  66, 
     30, 132,  66, 178,  15, 229, 
     33,  52, 146, 199,  32,  36, 
    200,   5,   8,  73, 112,   9, 
    194, 129,  97,  52, 128, 145, 
     16,  35,  63, 198,  89, 229, 
     99,  16,  14,  17,  79,  65, 
    211,  16,  14,  17, 207,  64, 
     39, 160,  10,  52, 171, 127, 
    163,  59,  58,  49, 114,  78, 
      4, 118, 204,  47, 253,  63, 
    144,  26, 160, 204,  63,  49, 
    253, 145,  42, 200, 255,  70, 
    152,  23,  99, 177,  88,  44, 
     22, 139, 197,  98, 177,  88, 
     44,  22, 139, 197, 242, 151, 
    112, 231,  30, 110,  83, 106, 
    115, 231,  62, 109,  13, 200, 
     25,  14,  61, 231, 205,  80, 
    174,  51, 110,  58,  97, 151, 
    147, 181, 194, 214,  32, 231, 
    167, 149, 175,  88,  21,  42, 
    126,  48,  81,  81,  65,  48, 
    227, 171, 241,  25, 165, 198, 
    166, 125,  85, 154,  86, 106, 
     55, 245, 125,  91,  38,  30, 
    170, 236, 171,  98,  89,  41, 
    135,  62, 125, 112,  34, 135, 
    188, 220, 249, 207,  78, 134, 
     41, 237,  82, 178, 233, 250, 
    201,  55,  87,  37, 158,  71, 
    202, 207, 143, 238, 103,  93, 
     47,  33,   7, 215,  94, 142, 
    242, 129, 167, 210,  25,  92, 
    147,  75, 156, 133,  79, 201, 
     36, 229, 193,  64, 143, 203, 
    246, 230, 146, 162,  41,  72, 
    102,  43, 102,  45, 115,  52, 
    193, 172,  69,   6,  68, 112, 
    250,  58,  64,  75, 123, 121, 
     94,  34,   7, 123,  81, 126, 
    241,  84, 239,  63, 209,  94, 
    102,  58, 121, 232, 235, 228, 
    198, 199, 215, 143, 239,  60, 
    127, 165, 239, 191, 212,  94, 
     98, 248,  80, 233, 212, 238, 
    217,  89,  61, 219, 125, 171, 
    247, 234, 144, 188, 223, 188, 
     16,  94, 143, 218, 209,  70, 
    175, 186, 190, 218,  93, 189, 
     93, 237, 173, 119, 227, 104, 
    229, 116, 216, 136, 239, 134, 
    113, 127,  61, 238, 118, 162, 
    118, 120, 179, 217,  95,   8, 
    235, 221,  27, 225, 229, 184, 
    211, 136, 187, 225, 210, 153, 
    165,  90,  63, 172, 183,  34, 
     44, 122, 225, 213,  43, 120, 
    176, 218, 108, 215, 106, 213, 
     86, 187, 215,  70, 208,  17, 
    231, 244,  39, 101,  92, 154, 
    133,  86, 162,  91,  29, 250, 
      1, 227, 248,  96,  83,   0, 
      0,   0,  83,  84,  65,  84, 
    148,   0,   0,   0,   6,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   6,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0
};
