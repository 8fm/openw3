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
//   float4x4 mvp;                      // Offset:    0 Size:    64
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
// TEXCOORD                 0   xy          0     NONE   float   xy  
// SV_Position              0   xyzw        1     NONE   float   xyzw
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// TEXCOORD                 0   xy          0     NONE   float   xy  
// SV_Position              0   xyzw        1      POS   float   xyzw
//
vs_5_0
Opaque Custom Data - XBOX Precompiled Shader Header
// First Precompiled Shader at offset:[63]
// Embedded Data:
//  0x0000003f - Offset To First Precompiled Shader
//  0x00010050 - Original Shader Descriptor
//  0x0000003a - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_constantbuffer cb0[4], immediateIndexed
dcl_input v0.xy
dcl_input v1.xyzw
dcl_output o0.xy
dcl_output_siv o1.xyzw, position
mov o0.xy, v0.xyxx
dp4 o1.x, v1.xyzw, cb0[0].xyzw
dp4 o1.y, v1.xyzw, cb0[1].xyzw
dp4 o1.z, v1.xyzw, cb0[2].xyzw
dp4 o1.w, v1.xyzw, cb0[3].xyzw
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[63], bundle is:[149] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (VS)
// Shader Hw Stage           : (VS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\VPosition3dTexUV.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- VS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask  3, v[4:5]
;   [1] generic, usageIdx  1, channelMask 15, v[8:11]
; Output Semantic Mappings
;   [0] generic, usageIdx  0, paramIdx 0, paramSlot 0

codeLenInByte        = 120;Bytes

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
NumVgprs             = 12;
NumSgprs             = 28;
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
; exportVertexSize          = 4

// Shader Instructions:
shader main
  asic(CI)
  type(VS)
  s_swappc_b64  s[2:3], s[2:3]                          // 000000000000: BE822102
  s_load_dwordx4  s[8:11], s[2:3], 0x00                 // 000000000004: C0840300
  s_waitcnt     lgkmcnt(0)                              // 000000000008: BF8C007F
  s_buffer_load_dwordx8  s[12:19], s[8:11], 0x00        // 00000000000C: C2C60900
  s_buffer_load_dwordx8  s[20:27], s[8:11], 0x08        // 000000000010: C2CA0908
  v_mov_b32     v0, 0                                   // 000000000014: 7E000280
  v_mov_b32     v1, 1.0                                 // 000000000018: 7E0202F2
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 00000000001C: BF8C0F70
  exp           param0, v4, v5, v0, v1                  // 000000000020: F800020F 01000504
  s_waitcnt     expcnt(0) & lgkmcnt(0)                  // 000000000028: BF8C000F
  v_mul_legacy_f32  v0, s15, v11                        // 00000000002C: 0E00160F
  v_mul_legacy_f32  v1, s19, v11                        // 000000000030: 0E021613
  v_mul_legacy_f32  v2, s23, v11                        // 000000000034: 0E041617
  v_mul_legacy_f32  v3, s27, v11                        // 000000000038: 0E06161B
  v_mac_legacy_f32  v0, s14, v10                        // 00000000003C: 0C00140E
  v_mac_legacy_f32  v1, s18, v10                        // 000000000040: 0C021412
  v_mac_legacy_f32  v2, s22, v10                        // 000000000044: 0C041416
  v_mac_legacy_f32  v3, s26, v10                        // 000000000048: 0C06141A
  v_mac_legacy_f32  v0, s13, v9                         // 00000000004C: 0C00120D
  v_mac_legacy_f32  v1, s17, v9                         // 000000000050: 0C021211
  v_mac_legacy_f32  v2, s21, v9                         // 000000000054: 0C041215
  v_mac_legacy_f32  v3, s25, v9                         // 000000000058: 0C061219
  v_mac_legacy_f32  v0, s12, v8                         // 00000000005C: 0C00100C
  v_mac_legacy_f32  v1, s16, v8                         // 000000000060: 0C021010
  v_mac_legacy_f32  v2, s20, v8                         // 000000000064: 0C041014
  v_mac_legacy_f32  v3, s24, v8                         // 000000000068: 0C061018
  exp           pos0, v0, v1, v2, v3 done               // 00000000006C: F80008CF 03020100
  s_endpgm                                              // 000000000074: BF810000
end


Opaque Custom Data - XBOX Precompiled Shader
// Offset:[214], bundle is:[155] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (VS)
// Shader Hw Stage           : (ES)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\VPosition3dTexUV.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- VS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask  3, v[4:5]
;   [1] generic, usageIdx  1, channelMask 15, v[8:11]
; Output Semantic Mappings
;   [0] position, usageIdx  1, paramIdx 1, paramSlot 0
;   [1] generic, usageIdx  0, paramIdx 0, paramSlot 0

codeLenInByte        = 148;Bytes

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
NumVgprs             = 12;
NumSgprs             = 28;
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
; exportVertexSize          = 8
; esGsRingItemSize          = 8

// Shader Instructions:
shader main
  asic(CI)
  type(VS)
  s_swappc_b64  s[2:3], s[2:3]                          // 000000000000: BE822102
  s_load_dwordx4  s[8:11], s[2:3], 0x00                 // 000000000004: C0840300
  s_waitcnt     lgkmcnt(0)                              // 000000000008: BF8C007F
  s_buffer_load_dwordx8  s[12:19], s[8:11], 0x00        // 00000000000C: C2C60900
  s_buffer_load_dwordx8  s[20:27], s[8:11], 0x08        // 000000000010: C2CA0908
  s_waitcnt     vmcnt(0) & lgkmcnt(0)                   // 000000000014: BF8C0070
  v_mul_legacy_f32  v0, s15, v11                        // 000000000018: 0E00160F
  s_load_dwordx4  s[8:11], s[2:3], 0x0c                 // 00000000001C: C084030C
  v_mac_legacy_f32  v0, s14, v10                        // 000000000020: 0C00140E
  v_mul_legacy_f32  v1, s19, v11                        // 000000000024: 0E021613
  v_mac_legacy_f32  v0, s13, v9                         // 000000000028: 0C00120D
  v_mac_legacy_f32  v1, s18, v10                        // 00000000002C: 0C021412
  v_mul_legacy_f32  v2, s23, v11                        // 000000000030: 0E041617
  v_mac_legacy_f32  v0, s12, v8                         // 000000000034: 0C00100C
  v_mac_legacy_f32  v1, s17, v9                         // 000000000038: 0C021211
  v_mac_legacy_f32  v2, s22, v10                        // 00000000003C: 0C041416
  v_mul_legacy_f32  v3, s27, v11                        // 000000000040: 0E06161B
  v_mac_legacy_f32  v1, s16, v8                         // 000000000044: 0C021010
  v_mac_legacy_f32  v2, s21, v9                         // 000000000048: 0C041215
  v_mac_legacy_f32  v3, s26, v10                        // 00000000004C: 0C06141A
  v_mac_legacy_f32  v2, s20, v8                         // 000000000050: 0C041014
  v_mac_legacy_f32  v3, s25, v9                         // 000000000054: 0C061219
  v_mac_legacy_f32  v3, s24, v8                         // 000000000058: 0C061018
  s_waitcnt     lgkmcnt(0)                              // 00000000005C: BF8C007F
  buffer_store_dword  v0, v0, s[8:11], s4 offset:16 glc slc // 000000000060: E0704010 04420000
  buffer_store_dword  v1, v0, s[8:11], s4 offset:20 glc slc // 000000000068: E0704014 04420100
  buffer_store_dword  v2, v0, s[8:11], s4 offset:24 glc slc // 000000000070: E0704018 04420200
  buffer_store_dword  v3, v0, s[8:11], s4 offset:28 glc slc // 000000000078: E070401C 04420300
  buffer_store_dword  v4, v0, s[8:11], s4 glc slc       // 000000000080: E0704000 04420400
  buffer_store_dword  v5, v0, s[8:11], s4 offset:4 glc slc // 000000000088: E0704004 04420500
  s_endpgm                                              // 000000000090: BF810000
end


Opaque Custom Data - XBOX Precompiled Shader
// Offset:[371], bundle is:[150] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (VS)
// Shader Hw Stage           : (LS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\VPosition3dTexUV.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- VS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask  3, v[4:5]
;   [1] generic, usageIdx  1, channelMask 15, v[8:11]
; Output Semantic Mappings
;   [0] position, usageIdx  1, paramIdx 1, paramSlot 0
;   [1] generic, usageIdx  0, paramIdx 0, paramSlot 0

codeLenInByte        = 128;Bytes

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
NumVgprs             = 12;
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
; exportVertexSize          = 8
; lsStride                  = 32

// Shader Instructions:
shader main
  asic(CI)
  type(VS)
  s_swappc_b64  s[2:3], s[2:3]                          // 000000000000: BE822102
  s_load_dwordx4  s[0:3], s[2:3], 0x00                  // 000000000004: C0800300
  s_waitcnt     lgkmcnt(0)                              // 000000000008: BF8C007F
  s_buffer_load_dwordx8  s[4:11], s[0:3], 0x00          // 00000000000C: C2C20100
  s_buffer_load_dwordx8  s[12:19], s[0:3], 0x08         // 000000000010: C2C60108
  s_waitcnt     vmcnt(0) & lgkmcnt(0)                   // 000000000014: BF8C0070
  v_mul_legacy_f32  v0, s7, v11                         // 000000000018: 0E001607
  v_mul_legacy_f32  v2, s11, v11                        // 00000000001C: 0E04160B
  v_mac_legacy_f32  v0, s6, v10                         // 000000000020: 0C001406
  v_mac_legacy_f32  v2, s10, v10                        // 000000000024: 0C04140A
  v_mul_legacy_f32  v3, s15, v11                        // 000000000028: 0E06160F
  v_mul_legacy_f32  v6, s19, v11                        // 00000000002C: 0E0C1613
  v_mac_legacy_f32  v0, s5, v9                          // 000000000030: 0C001205
  v_mac_legacy_f32  v2, s9, v9                          // 000000000034: 0C041209
  v_mac_legacy_f32  v3, s14, v10                        // 000000000038: 0C06140E
  v_mac_legacy_f32  v6, s18, v10                        // 00000000003C: 0C0C1412
  s_mov_b32     m0, 0x00010000                          // 000000000040: BEFC03FF 00010000
  v_mac_legacy_f32  v0, s4, v8                          // 000000000048: 0C001004
  v_mac_legacy_f32  v2, s8, v8                          // 00000000004C: 0C041008
  v_mac_legacy_f32  v3, s13, v9                         // 000000000050: 0C06120D
  v_mac_legacy_f32  v6, s17, v9                         // 000000000054: 0C0C1211
  v_lshlrev_b32  v1, 5, v1                              // 000000000058: 34020285
  v_mac_legacy_f32  v3, s12, v8                         // 00000000005C: 0C06100C
  v_mac_legacy_f32  v6, s16, v8                         // 000000000060: 0C0C1010
  ds_write2_b32  v1, v0, v2 offset0:4 offset1:5         // 000000000064: D8380504 00020001
  ds_write2_b32  v1, v3, v6 offset0:6 offset1:7         // 00000000006C: D8380706 00060301
  ds_write2_b32  v1, v4, v5 offset1:1                   // 000000000074: D8380100 00050401
  s_endpgm                                              // 00000000007C: BF810000
end


// Approximately 6 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_VPosition3dTexUV[] =
{
     68,  88,  66,  67,  55,  61, 
    102, 219, 114, 214,  19,  26, 
     55, 206, 133, 139, 227, 162, 
    160,   9,   1,   0,   0,   0, 
    212,  10,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
     84,   1,   0,   0, 172,   1, 
      0,   0,   4,   2,   0,   0, 
     56,  10,   0,   0,  82,  68, 
     69,  70,  24,   1,   0,   0, 
      1,   0,   0,   0, 104,   0, 
      0,   0,   1,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    254, 255,   0, 129,   0,   0, 
    220,   0,   0,   0,  82,  68, 
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
      1,   0,   0,   0, 128,   0, 
      0,   0,  64,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0, 168,   0,   0,   0, 
      0,   0,   0,   0,  64,   0, 
      0,   0,   2,   0,   0,   0, 
    184,   0,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    109, 118, 112,   0, 102, 108, 
    111,  97, 116,  52, 120,  52, 
      0, 171, 171, 171,   3,   0, 
      3,   0,   4,   0,   4,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    172,   0,   0,   0,  77, 105, 
     99, 114, 111, 115, 111, 102, 
    116,  32,  40,  82,  41,  32, 
     72,  76,  83,  76,  32,  83, 
    104,  97, 100, 101, 114,  32, 
     67, 111, 109, 112, 105, 108, 
    101, 114,  32,  70, 111, 114, 
     32,  68, 117, 114,  97, 110, 
    103, 111,  32,  57,  46,  51, 
     48,  46,  49,  50,  48,  57, 
     56,  46,  48,   0,  73,  83, 
     71,  78,  80,   0,   0,   0, 
      2,   0,   0,   0,   8,   0, 
      0,   0,  56,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   3,   3, 
      0,   0,  65,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,  15,  15, 
      0,   0,  84,  69,  88,  67, 
     79,  79,  82,  68,   0,  83, 
     86,  95,  80, 111, 115, 105, 
    116, 105, 111, 110,   0, 171, 
    171, 171,  79,  83,  71,  78, 
     80,   0,   0,   0,   2,   0, 
      0,   0,   8,   0,   0,   0, 
     56,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   3,  12,   0,   0, 
     65,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,   0,   0,   0, 
     84,  69,  88,  67,  79,  79, 
     82,  68,   0,  83,  86,  95, 
     80, 111, 115, 105, 116, 105, 
    111, 110,   0, 171, 171, 171, 
     83,  72,  69,  88,  44,   8, 
      0,   0,  80,   0,   1,   0, 
     11,   2,   0,   0,  53,  16, 
      0,   0,   5,   0,   0,   0, 
     63,   0,   0,   0,  80,   0, 
      1,   0,  58,   0,   0,   0, 
    106,   8,   0,   1,  89,   0, 
      0,   4,  70, 142,  32,   0, 
      0,   0,   0,   0,   4,   0, 
      0,   0,  95,   0,   0,   3, 
     50,  16,  16,   0,   0,   0, 
      0,   0,  95,   0,   0,   3, 
    242,  16,  16,   0,   1,   0, 
      0,   0, 101,   0,   0,   3, 
     50,  32,  16,   0,   0,   0, 
      0,   0, 103,   0,   0,   4, 
    242,  32,  16,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
     54,   0,   0,   5,  50,  32, 
     16,   0,   0,   0,   0,   0, 
     70,  16,  16,   0,   0,   0, 
      0,   0,  17,   0,   0,   8, 
     18,  32,  16,   0,   1,   0, 
      0,   0,  70,  30,  16,   0, 
      1,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  17,   0, 
      0,   8,  34,  32,  16,   0, 
      1,   0,   0,   0,  70,  30, 
     16,   0,   1,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     17,   0,   0,   8,  66,  32, 
     16,   0,   1,   0,   0,   0, 
     70,  30,  16,   0,   1,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,  17,   0,   0,   8, 
    130,  32,  16,   0,   1,   0, 
      0,   0,  70,  30,  16,   0, 
      1,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,  62,   0, 
      0,   1,  53,  16,   0,   0, 
    151,   0,   0,   0,  21,   0, 
      1,   0,  32,  71,  22,   0, 
    120,   1, 237, 152,  79, 107, 
     19,  65,  24, 135,  39, 147, 
    237,  52, 105, 167, 155, 237, 
    118, 187,  77,  82, 255,  80, 
     16, 241,  20, 140, 213, 139, 
    167, 128,  69,  79,  66, 177, 
    218, 211,  34, 172, 102,  99, 
    130, 105,  18, 178,  65,  22, 
     15, 181,  22,  15,  30,   2, 
    250,  33, 188, 250,  21,  76, 
    232,  65, 193, 139,  55,  63, 
     65,  15, 133, 130, 168,   7, 
    201,  77, 127, 111,  50, 131, 
     65, 237,  89, 193, 121, 224, 
    153, 217, 201, 236, 206, 188, 
    121, 103,  78, 239,  13, 159, 
    141, 217,  59, 247, 105,  68, 
    253, 225, 203, 195, 207,  71, 
    232, 103,  32,  97,  81,   3, 
     58,  30, 181, 140,  93,  88, 
    166,  22,  61,  53,  41, 200, 
     25, 147, 232,  78, 193,   4, 
     86,  32, 205, 105,  39,  84, 
    134, 231, 209, 186, 208, 134, 
    223, 224, 117,  88, 132, 248, 
    124, 252, 123,   6, 210, 243, 
    175, 232, 253, 105, 171, 105, 
    232,  93,   7,  78, 147, 166, 
      6, 252, 182, 206,  26,  53, 
    127, 134, 226, 166, 248,  79, 
    130,  98, 211, 208, 186,  20, 
     39,  65, 223, 253, 107,  80, 
    142, 244,  57,  17, 116,  30, 
    211,  80, 238, 117, 142, 142, 
    212, 123,  58, 175, 121, 117, 
    174, 250,  44, 135, 240,  36, 
    190,   3, 125,  46,  26,  61, 
    166,  28,   9, 118, 205,  17, 
    169,  23,  23,   5,   6,   2, 
     27,  10,  76,  10,  92,  40, 
     33, 224,  44,  68,  18,  69, 
     22, 206, 193, 121, 136,  77, 
    197,   2,  68, 128,  34,   7, 
    113, 176,  98,  17,  34, 249, 
     98,   9,  34,  86, 129, 248, 
      4, 238, 170,  88, 129, 121, 
     88, 128, 184,  64,  98,  21, 
     34,  96, 113,  26, 158, 129, 
    103,  39, 255,   9,  91, 253, 
    215,  80,  14, 244,  93,  37, 
    244, 249,  16, 211, 207,   6, 
    131, 193,  96,  48,  24,  12, 
      6, 131, 193,  96,  48,  24, 
     12, 127,  11, 190, 182, 255, 
    134, 165, 159,  13, 159, 176, 
    254, 128, 101, 223,  30, 100, 
    178, 239,  15, 246,  56, 219, 
    253, 202, 249, 110,  39, 215, 
     31, 228,  56,  27,  89,  51, 
     44, 149, 195, 124, 206, 103, 
    246, 146, 207, 237,  21, 223, 
    178,  87, 125,  97, 219,  30, 
    147, 174, 199, 165, 239,  89, 
    178, 232,   9, 185, 224,  50, 
    185, 232, 114, 185, 236,  90, 
    178, 224,  10,  41,  29,  38, 
     29, 135,  75, 207, 177, 100, 
    222,  17, 242,  67, 134, 141, 
     88, 138, 167,  25, 123,  58, 
     80, 101, 176, 113,  29, 139, 
     44,  64, 170, 165, 232, 177, 
    174,  57, 234,  49, 169, 177, 
    240, 234, 166,  90, 160, 162, 
    122, 122, 159, 234,  45, 246, 
    120, 149, 159, 116, 212, 124, 
     93, 245,  84, 171, 153, 135, 
    199,  87, 190, 188, 238, 220, 
     44, 124, 236, 171, 223, 159, 
    171, 158, 214, 152, 131, 199, 
    239, 234, 251,  92, 213, 108, 
     95, 169, 185, 187, 144, 190, 
    175,  93,  13, 238, 133, 205, 
    240, 113,  92, 234, 181, 187, 
    237, 135, 165, 184, 215, 141, 
    194, 157,  75,  65,  53, 122, 
     20,  68,  73,  47, 234, 182, 
    194, 102, 240, 160, 150,  92, 
     14, 182, 186, 247, 131,  91, 
     81, 171,  26, 117, 131, 141, 
    245, 141, 114,  18, 108, 213, 
     67,  12, 226,  96, 123, 179, 
     29,  55, 122, 141, 118, 107, 
    189, 122,  59,  74, 238, 108, 
    151, 106, 205, 114, 185,  84, 
    111, 198,  77, 172,  63, 161, 
    168, 118, 167,  10, 241,  44, 
    220,   9,  27,  45, 246,   3, 
    101, 146,  93,   0,   0,   0, 
     53,  16,   0,   0, 157,   0, 
      0,   0,  21,   0,   1,   0, 
     16, 115,  22,   0, 120,   1, 
    237, 152,  65, 107,  19,  65, 
     20, 199,  39, 155, 233, 180, 
     73, 167, 219, 237, 118, 187, 
     77, 218, 170, 244, 230, 165, 
    161, 177,  21, 193,  83, 136, 
     65, 188,   8,  98, 107,  79, 
    139, 176, 154, 141,   9, 110, 
    147, 176,  27,  36, 136,  40, 
     22, 143, 130,  34,  61, 250, 
     21, 188, 121, 240, 100,  67, 
     15,  10, 158,  60, 121, 246, 
    208, 131, 208, 139, 126,   2, 
    253, 191, 102, 166, 196,  98, 
    207,  10, 206,  15, 126, 243, 
    118, 118, 102, 103, 103, 223, 
    236, 233, 165,  62,  59, 226, 
    210, 171, 214,  99, 138,   7, 
     47,  15, 190, 127,  67,  28, 
    131, 132,  69,  13, 232, 122, 
    212,  50, 182,  57,  71,  45, 
     99, 231, 169, 201,  64,  76, 
    144,   8,  75, 112,  23,  86, 
     32, 141, 105, 135,  84,   6, 
     43, 104, 231, 161,  13, 243, 
    120, 230,  26,  98,  17,  10, 
    232, 192,  28,  60,   9, 141, 
    113,  72, 208, 171,  78,  66, 
    207, 141, 162, 231, 210, 115, 
    191, 205,  95, 166, 230, 207, 
    156, 131, 180, 255, 211, 160, 
    119, 208, 122,  26, 189,  79, 
    250, 222, 127,  13, 250, 102, 
    125,  78,   4, 157, 199,  40, 
    148, 123, 157,  35, 174, 206, 
     81, 231, 169, 166, 250, 250, 
     44,   7, 240,  52, 126,   2, 
    189,  14,  65, 215, 235, 144, 
    192, 209,  34,  95,  87,  28, 
    145, 121, 177,  42, 208,  17, 
     89, 136,   9,   2,  63, 148, 
     64,  34, 197,  56, 156, 128, 
     72, 164, 200, 195,  73, 136, 
    151, 138,  41, 136,  13, 138, 
    105, 136, 164, 139,  25, 232, 
    194,  89, 136, 111,  18, 216, 
    159, 192, 191,  42, 240,  19, 
    137,   2, 196, 207,  35,  22, 
    224,  34, 196, 134, 197,  25, 
    120,  22, 226,  64, 105,  15, 
     44,  51, 252, 178, 209, 179, 
    251, 159, 160,  28,  32, 205, 
    199, 208,  25, 105, 232, 254, 
    232, 152, 193,  96,  48,  24, 
     12,   6, 131, 193,  96,  48, 
     24,  12,   6, 195, 223, 192, 
     90, 222, 121, 207, 178, 207, 
      6,  79, 216, 243,  61, 150, 
    251, 176,  63, 145, 251, 180, 
    223, 197, 245, 180, 207, 108, 
    137, 251, 182, 199, 228, 172, 
    111, 217,  83,  46, 147, 174, 
    103, 201, 121, 159, 219, 210, 
     97, 114, 198, 181, 164, 239, 
    113, 185, 232,  11, 219, 113, 
     44,  57, 231, 114, 185, 224, 
      9, 233,  57,  92,  22,  93, 
     33,  11, 142, 144, 180, 166, 
     83, 233, 126, 101, 172, 202, 
     61, 138, 153,  42,  47,  80, 
    180, 170, 124, 137,  98, 182, 
    202,  25,  69,  94, 229, 156, 
    226,  24, 250, 236, 233, 158, 
     42, 145,  29, 213,  86,  72, 
    170,  95,  82, 149,  73, 247, 
     11, 144, 208, 125, 170,  29, 
     18, 186,  79, 106, 176,  96, 
    230, 145,  90,  48,  86, 145, 
    230, 227,  62, 179, 143, 171, 
    114,  67,  94, 171, 241,  93, 
     21, 169, 118,  51,   9,  15, 
     47, 254, 120, 211, 189,  94, 
    252, 242,  78, 221, 127, 171, 
     34, 173, 145, 135, 135,  31, 
    155,  59,  37,  85, 223, 253, 
    172, 198, 110,  67, 122, 190, 
    113,  57, 184,  19, 198, 225, 
    195, 180, 212, 235,  36, 157, 
    251, 165, 180, 151,  68, 225, 
    246, 133, 160,  30,  61,   8, 
    162, 126,  47,  74, 218,  97, 
     28, 220, 107, 244, 215, 131, 
    141, 228, 110, 112,  51, 106, 
    215, 163,  36, 168, 173, 213, 
    202, 253,  96, 163,  25, 162, 
    147,   6,  91,  55,  58, 105, 
    171, 215, 234, 180, 215, 234, 
    155,  81, 255, 214,  86, 169, 
     17, 151, 203, 165, 102, 156, 
    198,  88, 127, 200,  85, 245, 
    118, 170,  38, 143, 195, 237, 
    176, 213, 102, 191,   0,  68, 
    173, 100, 229,   0,  53,  16, 
      0,   0, 152,   0,   0,   0, 
     21,   0,   1,   0,   0,  79, 
     22,   0, 120,   1, 237, 152, 
     79, 107,  19,  65,  24, 135, 
    103, 147, 205, 155,  52, 153, 
    110, 183, 219,  24, 173, 127, 
     91,  79, 226,  97, 105, 108, 
      5, 241,  84, 176, 160,  23, 
     81, 172,  22,  15, 139, 176, 
     53,  27,  19,  76, 147, 176, 
     27,  36, 120, 177,  45, 120, 
     19, 236, 193, 143, 161,  31, 
    193,  54, 228, 160, 223,  64, 
    252,   0,  61,   8, 189, 212, 
    171,   8, 245, 247,  38,  51, 
    186, 160,  61,  43,  56,  15, 
     60, 243, 238, 236, 204, 206, 
    204, 206, 204, 233, 189,  83, 
     17,  35, 222, 111, 125, 243, 
     57, 238, 239, 236,  31, 126, 
     65, 204, 193,  52, 221,  50, 
    151,  66,  44, 156, 224,  82, 
    136,  75,  92,  88,  48,  35, 
    132,  68, 224, 230,  77, 184, 
     12, 185,  77,  59, 102, 121, 
    192, 131, 123, 208, 133, 151, 
    241, 221,  77, 196,  89, 152, 
    133, 211, 176,   0,  25, 174, 
    167, 209, 117,  76, 243,  27, 
    252,  93,  26, 221,  87, 199, 
    159, 204, 115, 241, 103, 206, 
     66,  94, 255, 113, 232,  57, 
    244, 152, 122, 157, 106,  59, 
    254,  41, 248,  56, 244,  57, 
     49, 124,  30, 105,  28, 168, 
    255, 227,  80, 245, 227, 111, 
    152,  57, 117, 174, 250,  44, 
      7, 240,  56, 142, 128, 205, 
     15,  41, 116, 157, 207, 137, 
    196,  13, 151, 172, 215,  11, 
    132,  10,  97,  66,  66,  35, 
    225,  66,  17, 193,  60, 196, 
     38, 210,   4,  44, 194,  18, 
    196, 164,  52,   9, 177,  64, 
    154, 130, 184,  36, 132, 141, 
     39,  92,  24, 154, 129,  88, 
     16,  97, 125, 132, 187,  74, 
     39, 225,  41, 136, 203,  67, 
    167, 225,  25, 136,  67, 164, 
    115, 240,  60, 188, 160, 238, 
    138,  53, 254,  51,  76, 249, 
     95, 194, 123, 160, 239,  42, 
    163, 207, 135, 119,  69, 191, 
    159, 227, 194,  96,  48,  24, 
     12,   6, 131, 193,  96,  48, 
     24,  12,   6, 131, 225,  47, 
    145, 153, 223, 222,  21, 217, 
    205, 193,  11, 241, 106,  79, 
     88, 195,  97, 193, 250,  48, 
    236, 226,  57,  95,  17,  78, 
    169,  98,  59,  84,  22, 178, 
     88, 182, 229,  84, 133, 156, 
    153, 138, 116, 114, 158, 144, 
     19, 158,  45, 157,  50,  73, 
    175,  44, 229,  81, 246, 251, 
     46, 103,  59, 108,  87, 200, 
    130, 107, 203,  73, 143, 228, 
    180,  39, 229, 203,  76, 102, 
     73, 186,  36,  93,  87,  74, 
     59, 119, 237, 179,  37,  50, 
    130, 242, 136,  89,  66, 119, 
     68, 155,  51, 159,  91, 123, 
     42,  29,  54, 202, 163, 176, 
    156, 171, 228, 220, 137, 174, 
    115,  30, 147, 209, 117,  86, 
     99, 163, 235,  67,  53, 192, 
     45,  21, 185,  63, 231,  96, 
    156, 209,  40, 191, 232, 171, 
    246, 174, 138, 156, 155,  41, 
    193, 131, 171,  95, 223, 117, 
    111, 207, 126, 122, 163, 222, 
    239, 168, 200,  99,  20, 225, 
    193, 199, 198, 118,  81, 229, 
    110, 223, 170, 182,  71, 144, 
    191, 175,  95,  15, 214, 195, 
     86, 248,  60, 241, 123, 157, 
    184, 243, 212,  79, 122, 113, 
     20, 110,  92,   9, 106, 209, 
    179,  32, 234, 247, 162, 184, 
     29, 182, 130,  39, 245, 254, 
     82, 176,  26,  63,  14, 238, 
     69, 237,  90,  20,   7,  43, 
    139,  43, 213, 126, 176, 218, 
      8,  81,  73, 130, 181, 187, 
    157, 164, 217, 107, 118, 218, 
    139, 181, 251,  81, 255, 193, 
    154,  95, 111,  85, 171, 126, 
    163, 149, 180,  48, 254, 152, 
    139, 106, 118, 222, 175,  60, 
    220,   8, 155, 109, 241,   3, 
    175, 135,  92,  64,   0,   0, 
     83,  84,  65,  84, 148,   0, 
      0,   0,   6,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
      4,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
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
      0,   0,   0,   0,   0,   0
};
