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
//   float4 mvp[2];                     // Offset:    0 Size:    32
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
// SV_Position              0   xyzw        1     NONE   float   xyzw
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float   xyzw
// SV_Position              0   xyzw        1      POS   float   xyzw
//
vs_5_0
Opaque Custom Data - XBOX Precompiled Shader Header
// First Precompiled Shader at offset:[55]
// Embedded Data:
//  0x00000037 - Offset To First Precompiled Shader
//  0x00010050 - Original Shader Descriptor
//  0x00000032 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_constantbuffer cb0[2], immediateIndexed
dcl_input v0.xyzw
dcl_input v1.xyzw
dcl_output o0.xyzw
dcl_output_siv o1.xyzw, position
mov o0.xyzw, v0.xyzw
dp4 o1.x, v1.xyzw, cb0[0].xyzw
dp4 o1.y, v1.xyzw, cb0[1].xyzw
mov o1.zw, l(0,0,0,1.000000)
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[55], bundle is:[139] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (VS)
// Shader Hw Stage           : (VS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\VVertex.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- VS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, v[4:7]
;   [1] generic, usageIdx  1, channelMask 15, v[8:11]
; Output Semantic Mappings
;   [0] generic, usageIdx  0, paramIdx 0, paramSlot 0

codeLenInByte        = 80;Bytes

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
NumSgprs             = 16;
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
  s_buffer_load_dwordx8  s[8:15], s[8:11], 0x00         // 00000000000C: C2C40900
  s_waitcnt     vmcnt(0) & lgkmcnt(0)                   // 000000000010: BF8C0070
  v_mul_legacy_f32  v0, s11, v11                        // 000000000014: 0E00160B
  v_mul_legacy_f32  v1, s15, v11                        // 000000000018: 0E02160F
  v_mac_legacy_f32  v0, s10, v10                        // 00000000001C: 0C00140A
  v_mac_legacy_f32  v1, s14, v10                        // 000000000020: 0C02140E
  v_mac_legacy_f32  v0, s9, v9                          // 000000000024: 0C001209
  v_mac_legacy_f32  v1, s13, v9                         // 000000000028: 0C02120D
  v_mac_legacy_f32  v0, s8, v8                          // 00000000002C: 0C001008
  v_mac_legacy_f32  v1, s12, v8                         // 000000000030: 0C02100C
  v_mov_b32     v2, 0                                   // 000000000034: 7E040280
  v_mov_b32     v3, 1.0                                 // 000000000038: 7E0602F2
  exp           pos0, v0, v1, v2, v3 done               // 00000000003C: F80008CF 03020100
  exp           param0, v4, v5, v6, v7                  // 000000000044: F800020F 07060504
  s_endpgm                                              // 00000000004C: BF810000
end


Opaque Custom Data - XBOX Precompiled Shader
// Offset:[196], bundle is:[149] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (VS)
// Shader Hw Stage           : (ES)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\VVertex.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- VS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, v[4:7]
;   [1] generic, usageIdx  1, channelMask 15, v[8:11]
; Output Semantic Mappings
;   [0] position, usageIdx  1, paramIdx 1, paramSlot 0
;   [1] generic, usageIdx  0, paramIdx 0, paramSlot 0

codeLenInByte        = 136;Bytes

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
; exportVertexSize          = 8
; esGsRingItemSize          = 8

// Shader Instructions:
shader main
  asic(CI)
  type(VS)
  s_swappc_b64  s[2:3], s[2:3]                          // 000000000000: BE822102
  s_load_dwordx4  s[8:11], s[2:3], 0x00                 // 000000000004: C0840300
  s_waitcnt     lgkmcnt(0)                              // 000000000008: BF8C007F
  s_buffer_load_dwordx8  s[8:15], s[8:11], 0x00         // 00000000000C: C2C40900
  s_waitcnt     vmcnt(0) & lgkmcnt(0)                   // 000000000010: BF8C0070
  v_mul_legacy_f32  v0, s11, v11                        // 000000000014: 0E00160B
  s_load_dwordx4  s[16:19], s[2:3], 0x0c                // 000000000018: C088030C
  v_mac_legacy_f32  v0, s10, v10                        // 00000000001C: 0C00140A
  v_mul_legacy_f32  v1, s15, v11                        // 000000000020: 0E02160F
  v_mac_legacy_f32  v0, s9, v9                          // 000000000024: 0C001209
  v_mac_legacy_f32  v1, s14, v10                        // 000000000028: 0C02140E
  v_mac_legacy_f32  v0, s8, v8                          // 00000000002C: 0C001008
  v_mac_legacy_f32  v1, s13, v9                         // 000000000030: 0C02120D
  v_mac_legacy_f32  v1, s12, v8                         // 000000000034: 0C02100C
  v_mov_b32     v2, 0                                   // 000000000038: 7E040280
  s_waitcnt     lgkmcnt(0)                              // 00000000003C: BF8C007F
  buffer_store_dword  v0, v0, s[16:19], s4 offset:16 glc slc // 000000000040: E0704010 04440000
  v_mov_b32     v0, 1.0                                 // 000000000048: 7E0002F2
  buffer_store_dword  v1, v0, s[16:19], s4 offset:20 glc slc // 00000000004C: E0704014 04440100
  buffer_store_dword  v2, v0, s[16:19], s4 offset:24 glc slc // 000000000054: E0704018 04440200
  buffer_store_dword  v0, v0, s[16:19], s4 offset:28 glc slc // 00000000005C: E070401C 04440000
  buffer_store_dword  v4, v0, s[16:19], s4 glc slc      // 000000000064: E0704000 04440400
  buffer_store_dword  v5, v0, s[16:19], s4 offset:4 glc slc // 00000000006C: E0704004 04440500
  buffer_store_dword  v6, v0, s[16:19], s4 offset:8 glc slc // 000000000074: E0704008 04440600
  buffer_store_dword  v7, v0, s[16:19], s4 offset:12 glc slc // 00000000007C: E070400C 04440700
  s_endpgm                                              // 000000000084: BF810000
end


Opaque Custom Data - XBOX Precompiled Shader
// Offset:[347], bundle is:[143] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (VS)
// Shader Hw Stage           : (LS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\VVertex.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- VS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, v[4:7]
;   [1] generic, usageIdx  1, channelMask 15, v[8:11]
; Output Semantic Mappings
;   [0] position, usageIdx  1, paramIdx 1, paramSlot 0
;   [1] generic, usageIdx  0, paramIdx 0, paramSlot 0

codeLenInByte        = 108;Bytes

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
NumSgprs             = 8;
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
  s_buffer_load_dwordx8  s[0:7], s[0:3], 0x00           // 00000000000C: C2C00100
  s_waitcnt     vmcnt(0) & lgkmcnt(0)                   // 000000000010: BF8C0070
  v_mul_legacy_f32  v0, s3, v11                         // 000000000014: 0E001603
  v_mul_legacy_f32  v2, s7, v11                         // 000000000018: 0E041607
  v_mac_legacy_f32  v0, s2, v10                         // 00000000001C: 0C001402
  v_mac_legacy_f32  v2, s6, v10                         // 000000000020: 0C041406
  v_mac_legacy_f32  v0, s1, v9                          // 000000000024: 0C001201
  v_mac_legacy_f32  v2, s5, v9                          // 000000000028: 0C041205
  s_mov_b32     m0, 0x00010000                          // 00000000002C: BEFC03FF 00010000
  v_mac_legacy_f32  v0, s0, v8                          // 000000000034: 0C001000
  v_mac_legacy_f32  v2, s4, v8                          // 000000000038: 0C041004
  v_lshlrev_b32  v1, 5, v1                              // 00000000003C: 34020285
  ds_write2_b32  v1, v0, v2 offset0:4 offset1:5         // 000000000040: D8380504 00020001
  v_mov_b32     v0, 0                                   // 000000000048: 7E000280
  v_mov_b32     v2, 1.0                                 // 00000000004C: 7E0402F2
  ds_write2_b32  v1, v0, v2 offset0:6 offset1:7         // 000000000050: D8380706 00020001
  ds_write2_b32  v1, v4, v5 offset1:1                   // 000000000058: D8380100 00050401
  ds_write2_b32  v1, v6, v7 offset0:2 offset1:3         // 000000000060: D8380302 00070601
  s_endpgm                                              // 000000000068: BF810000
end


// Approximately 5 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_VVertex[] =
{
     68,  88,  66,  67, 174, 176, 
    100, 167, 233, 155,  57,  11, 
     98, 236, 230, 151, 161, 194, 
     22, 229,   1,   0,   0,   0, 
     76,  10,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
     80,   1,   0,   0, 164,   1, 
      0,   0, 248,   1,   0,   0, 
    176,   9,   0,   0,  82,  68, 
     69,  70,  20,   1,   0,   0, 
      1,   0,   0,   0, 104,   0, 
      0,   0,   1,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    254, 255,   0, 129,   0,   0, 
    216,   0,   0,   0,  82,  68, 
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
      0,   0,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0, 168,   0,   0,   0, 
      0,   0,   0,   0,  32,   0, 
      0,   0,   2,   0,   0,   0, 
    180,   0,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    109, 118, 112,   0, 102, 108, 
    111,  97, 116,  52,   0, 171, 
      1,   0,   3,   0,   1,   0, 
      4,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0, 172,   0,   0,   0, 
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
     73,  83,  71,  78,  76,   0, 
      0,   0,   2,   0,   0,   0, 
      8,   0,   0,   0,  56,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,  15,   0,   0,  62,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
     15,  15,   0,   0,  67,  79, 
     76,  79,  82,   0,  83,  86, 
     95,  80, 111, 115, 105, 116, 
    105, 111, 110,   0, 171, 171, 
     79,  83,  71,  78,  76,   0, 
      0,   0,   2,   0,   0,   0, 
      8,   0,   0,   0,  56,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  62,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
     15,   0,   0,   0,  67,  79, 
     76,  79,  82,   0,  83,  86, 
     95,  80, 111, 115, 105, 116, 
    105, 111, 110,   0, 171, 171, 
     83,  72,  69,  88, 176,   7, 
      0,   0,  80,   0,   1,   0, 
    236,   1,   0,   0,  53,  16, 
      0,   0,   5,   0,   0,   0, 
     55,   0,   0,   0,  80,   0, 
      1,   0,  50,   0,   0,   0, 
    106,   8,   0,   1,  89,   0, 
      0,   4,  70, 142,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,  95,   0,   0,   3, 
    242,  16,  16,   0,   0,   0, 
      0,   0,  95,   0,   0,   3, 
    242,  16,  16,   0,   1,   0, 
      0,   0, 101,   0,   0,   3, 
    242,  32,  16,   0,   0,   0, 
      0,   0, 103,   0,   0,   4, 
    242,  32,  16,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
     54,   0,   0,   5, 242,  32, 
     16,   0,   0,   0,   0,   0, 
     70,  30,  16,   0,   0,   0, 
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
     54,   0,   0,   8, 194,  32, 
     16,   0,   1,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    128,  63,  62,   0,   0,   1, 
     53,  16,   0,   0, 141,   0, 
      0,   0,  21,   0,   1,   0, 
     32,  22,  22,   0, 120,   1, 
    237, 152,  79, 107,  19,  65, 
     24, 135,  39, 147, 116, 154, 
     52, 219,  77,  26,  99, 108, 
    235, 159, 182, 208,  67,  79, 
     33,  81,  79, 158, 114,  40, 
    165,  55, 197,  98,  64,  24, 
    208, 213, 108,  90, 113, 155, 
    150,  77, 144,  32,  82, 181, 
    120,  20, 252,   0, 126,   2, 
    253,   8,  30, 108,  40,  30, 
     60,  41,  30, 188, 121,  86, 
     60,  41, 158,  10,  30, 244, 
    247,  75, 102, 234,  34, 246, 
    172, 224,  60, 240, 204, 187, 
    179,  59,  59,  59, 121, 103, 
    114, 121,  43,  21,  49, 228, 
    253, 250, 167, 231, 140, 205, 
    171, 223, 219, 175,  17, 199, 
     32, 201, 176,   1, 219, 101, 
    182, 224,  56,  27,  33, 150, 
    216, 164, 160,  20, 194,  67, 
     40, 194,  75, 176,   1, 249, 
    204,  58, 162,  49, 152,  71, 
     59,   1,  57, 238,  13,  92, 
    129, 188, 198, 235, 195, 251, 
     10, 242, 250, 119, 210, 108, 
      0,  63, 149, 132,  99, 179, 
     48, 137, 125, 223, 198,  67, 
     22, 216, 252, 153,  18, 228, 
    250, 143, 130, 107, 179, 112, 
     94, 174, 147, 112, 237, 255, 
     26, 204, 209, 225,  62,   1, 
    238,  71,  18,  31, 218, 124, 
     14, 204,  56, 155, 215, 175, 
    166, 111, 247, 114,   0, 143, 
    226,   7, 176, 231, 194,  98, 
    251, 163,  28,  93, 172, 169, 
    212, 147, 154,  66,  71, 225, 
    131,  10,  15,  21,  14, 148, 
     66, 242, 212,  56, 196, 198, 
    169,  28,  68, 114,  85,  30, 
    226, 163, 106,  18,  98, 129, 
    170,   0, 177,   0,  53,   5, 
    177,  57, 234,  24, 196, 218, 
     20, 206, 157, 194,  89,  85, 
     39, 224,  52, 156, 129, 179, 
    240,  36,  60,   5,  79, 195, 
     51, 112, 110, 244, 155, 236, 
     62, 253, 175,  48,   7, 201, 
    255, 135, 221,  31, 146, 188, 
    118,  56,  28,  14, 135, 195, 
    225, 112,  56,  28,  14, 135, 
    195, 225, 248,  91, 200, 133, 
    221, 151,  34, 253, 104, 112, 
     95,  60, 222,  19, 185,  87, 
    251, 219, 136, 249, 138, 240, 
     11,  21, 233,  79, 148, 133, 
    231, 151, 165, 151,  43,   9, 
    111, 178,  36, 189, 108,  81, 
    120,  94,  81, 122,  15, 100, 
    102, 231, 155,  84,  59, 111, 
    179, 226,  64, 164, 100, 186, 
     32, 197,  65, 102, 140,  21, 
    167, 135, 123, 166, 108,  57, 
    172,  79, 209,  25, 200,  26, 
    137, 237, 179, 230,  69, 108, 
    159,  90,  50,  24, 186, 100, 
     38, 152,  54, 145, 227,  89, 
     71, 241, 135, 179, 252,  98, 
    213,  60, 111, 152, 200,  26, 
     76,  30, 126, 188,  86, 251, 
    240, 244, 203, 254, 108, 203, 
    220, 191, 110,  34, 231,  96, 
    141, 241, 217, 226, 238, 139, 
    119, 230, 222,  61,  19, 175, 
     64, 190, 223, 190, 160, 111, 
      4,  81, 112, 183,  91, 237, 
    109, 197,  91, 183, 171, 221, 
     94,  28,   6, 155, 103, 117, 
     43, 188, 163, 195, 126,  47, 
    140,  59,  65, 164, 215, 219, 
    253, 243, 122,  45, 190, 169, 
     47, 135, 157,  86,  24, 235, 
    229, 115, 203, 245, 190,  94, 
    219,   8, 208, 233, 234, 102, 
     51, 140, 123,  97, 191, 218, 
    142, 234, 245, 234,  70, 212, 
    141,  48, 237, 136, 207, 230, 
     99,  44, 248,  34,  83,  98, 
     51, 184, 213,  17,  63,   1, 
    232, 108,  83, 138,   0,   0, 
     53,  16,   0,   0, 151,   0, 
      0,   0,  21,   0,   1,   0, 
     16,  94,  22,   0, 120,   1, 
    237, 152,  65, 107,  19,  65, 
     24, 134, 103,  55, 219,  73, 
    210, 108, 183,  49, 198, 152, 
    212, 170,  20,  60, 180, 151, 
    144, 104,  15, 226,  41, 135, 
     61, 244,  38,  88,  12,   8, 
    139, 186, 218,  77,  43, 110, 
    211, 176,   9,  18,  60,  84, 
     45,  30, 114,  16, 244, 238, 
     31,  80, 232,  47,  16, 177, 
    161, 244, 224, 217, 179,   7, 
     79,  30, 188,   8, 158,  68, 
     15, 250, 190, 205,  76,   9, 
    193, 158,  21, 156,   7, 158, 
    249,  50,  51, 187, 179, 147, 
    111, 230, 244, 221,  44, 137, 
     67, 190, 188, 253, 176, 203, 
    216, 188, 241, 179, 245,  30, 
    113,  10,  18, 155,  13, 232, 
     20, 217,  10, 177, 114, 138, 
    173,  16, 139, 108,  44, 136, 
      7,  92,   4,  78,  15,  96, 
      3, 114,  78,  59, 162,  49, 
     92,  66, 155, 135,  30, 252, 
    129, 247,  86,  16,  11,  48, 
      3, 167,  85, 156, 132,  99, 
     14,  36, 252, 212,  56, 220, 
    215, 228,  59,  41,  54, 128, 
    227, 156,  63,  98, 129, 205, 
    159, 153, 131, 220, 255, 113, 
     76, 238,  77, 255,  86, 233, 
    248, 167,  96, 142, 244,  57, 
     17, 158, 199,  56, 204, 189, 
    206, 231, 119, 245, 156, 206, 
    235, 101, 117, 174, 250,  44, 
    135, 240,  56, 126,   1, 189, 
     14, 225, 239, 101,  72, 152, 
    119,  41, 174, 214, 164, 245, 
    188,  38, 209, 145,  56,  20, 
    137,   7,  36,  46, 148, 148, 
     48,  13, 145,  68, 153, 133, 
     72, 174, 204,  65, 124,  84, 
    206,  64, 108,  80, 206,  66, 
     92,  20, 121,   2, 226, 130, 
    200, 147,  16,  27, 146, 216, 
    159, 196,  93, 149, 167,  97, 
     25,  86,  32,  14,  79, 158, 
    129, 243, 240,  44,  60,   7, 
    207, 171, 179, 183,  70, 255, 
     12, 159, 252,  47,  97,  14, 
    244,  93,  37,  60,  35,  13, 
    199, 199, 231,  12,   6, 131, 
    193,  96,  48,  24,  12,   6, 
    131, 193,  96,  48,  24, 254, 
      6, 246, 194, 206,  59, 145, 
    122,  58, 124,  36, 158, 237, 
    137, 236, 193, 126,   7,  49, 
     87,  18, 158, 155,  26,  12, 
    167, 139, 194, 157,  45, 217, 
     94, 182,  32,  92, 175, 104, 
    187, 153, 188, 112, 103,  10, 
    182, 235, 230, 109, 247, 177, 
    237, 108, 243, 157, 124, 163, 
    243,  73,   8, 223, 249, 102, 
    139, 237,  34, 127,  91, 190, 
     83, 102, 180, 125, 103, 158, 
     17, 115, 130, 209, 241,  29, 
    135, 113, 202, 119,  50, 140, 
    210, 119,  92, 198,  52, 230, 
    197, 147,  61,  85,  22,  59, 
    172, 167, 208,  10, 100, 101, 
     73, 247, 203, 144, 232,  62, 
    107, 156,  68, 247, 169,   6, 
     11,  90,  29, 181, 224, 109, 
     21, 249,  60, 198, 133, 119, 
     84, 137,  27, 241,  66, 205, 
     15,  84, 100, 189,  38,   7, 
     63, 223, 170, 125, 124, 249, 
    117, 127, 110,  87, 141, 191, 
     82, 145, 107, 176,  86, 249, 
    250, 194, 206, 155, 138, 170, 
    233,  30, 168, 185, 235, 144, 
    239, 183, 174,   4, 119, 194, 
     56, 124, 216, 173, 246, 182, 
    146, 173, 251, 213, 110,  47, 
    137, 194, 205, 139, 193,  90, 
    244,  32, 136, 250, 189,  40, 
    105, 135, 113, 176, 222, 234, 
     47,   7, 171, 201, 221, 224, 
     90, 212,  94, 139, 146, 192, 
    191, 228, 215, 251, 193, 234, 
     70, 136,  78,  55, 104,  54, 
    163, 164,  23, 245, 171, 173, 
    184,  94, 175, 110, 196, 221, 
     24, 203, 142, 168, 171, 143, 
    178, 112, 156, 134, 155, 225, 
    189, 182, 248,  13,  62,  50, 
    101,  45,   0,   0,  53,  16, 
      0,   0, 145,   0,   0,   0, 
     21,   0,   1,   0,   0,  50, 
     22,   0, 120,   1, 237, 152, 
     77, 107,  19,  65,  24, 199, 
    159, 221, 108, 158, 248, 178, 
    141,  49, 198, 104, 107, 125, 
     41, 120, 208,  75,  72,  98, 
     14, 226, 169,  72, 192, 163, 
     96,  49,  32,  44,  72,  52, 
    155,  86,  76, 211, 178,   9, 
     18,  68, 106,  44, 120,  19, 
    188,  10, 126,   2, 191, 130, 
    135,  54, 244, 228,  71, 240, 
    144, 179,  55,  17, 188,  42, 
    212, 255,  63, 153, 161, 177, 
    216, 179, 130, 243, 131, 223, 
     60, 204, 238, 236, 204, 236, 
     60, 115, 122, 170,  69, 153, 
     48, 119, 123, 156, 103, 108, 
     60, 248, 209, 254, 132, 152, 
    134, 179, 108,  22, 216, 138, 
     44, 158, 101,  43, 114, 141, 
    141,   7, 125, 145,  16, 225, 
     24, 236, 192, 101, 200, 119, 
    214,  41, 203, 163, 235, 104, 
     79, 192,  28, 172, 226, 187, 
     59, 136,  92,  48, 128,  39, 
     97,   6,  18, 246, 103,  73, 
    177,   1,  88, 230,  55, 216, 
     63,  14, 103, 177,  99,  14, 
    207,  33,  75, 108, 254,  12, 
    127, 159, 251,  63,  10, 238, 
    141, 216,  57, 237,  62, 249, 
    191, 255,  26,  76, 135, 205, 
     19,  97,  62, 102, 201,  66, 
    123, 158,  99,  51, 142, 223, 
    144, 208, 228, 213, 230, 114, 
      4, 143,  98,  31,  28,  62, 
     99, 219, 103,  14,  84, 238, 
    150, 213, 123,  91,  86, 116, 
     20,  11,  42,  94,  42,  46, 
    148,  42, 196,   1,  42,  22, 
     80,  36,  79, 113,  33,  20, 
      7, 172,  88,  84, 231,  32, 
     54, 168, 167,  32,  46, 137, 
    158, 134, 184,  32, 122,   6, 
     98, 175, 138, 253,  41, 146, 
    165, 231, 224, 121,  56,  15, 
     23, 224,   5, 184,   8,  47, 
    194,  75, 240, 178, 185,   7, 
    222, 244, 207, 176, 228, 127, 
      9, 207, 128, 121, 180, 216, 
    252, 240,  84, 236, 243,  43, 
    108,  28,  14, 135, 195, 225, 
    112,  56,  28,  14, 135, 195, 
    225, 112,  56, 254,  18, 254, 
    210, 246, 142, 164, 134, 163, 
    151, 242, 102,  87, 188, 209, 
    222,  38,  98, 170,  40, 217, 
     76,  49, 200, 250,   5,   9, 
    181,  16, 132,  94,  94, 194, 
    116,  62,   8, 247,  83,  63, 
    119,  38, 149, 141, 156, 132, 
     65,  46,   8,  95, 251, 126, 
     45,  72, 223, 252, 236, 137, 
     47,  67,  95, 182, 190, 251, 
    193, 150, 102, 166, 125, 241, 
     16, 131, 180, 248,  41,  68, 
     22, 163, 228, 213, 174,  41, 
    125, 241, 237, 196, 121, 200, 
     58, 137, 237, 179, 102,  73, 
    108, 159,  90,   2,  12, 173, 
    155,   9, 106,  38, 114,  60, 
    235,  45, 217, 201,  44,   7, 
    180, 204, 251, 200,  68, 214, 
     97,  88,  95, 252, 242, 176, 
     60, 126, 255, 109, 111,  97, 
    104, 158, 191,  48, 145, 115, 
    176, 102, 250, 225, 234, 246, 
    199, 175, 230, 217,  59,  19, 
    239,  67, 126, 223, 190,  21, 
     61, 106, 118, 154, 207, 123, 
    165, 254,  70, 178, 241, 180, 
    212, 235,  39, 113, 115, 189, 
     26, 181, 226, 103,  81,  60, 
    232, 199,  73, 183, 217, 137, 
     86, 219, 131,  90, 180, 146, 
     60, 142, 238, 197, 221,  86, 
    156,  68, 245,  27, 245, 202, 
     32,  90,  89, 107, 162, 211, 
    139,  26, 141,  56, 233, 199, 
    131,  82, 187,  83, 169, 148, 
    214,  58, 189, 131, 178,  97, 
    154, 213,  81, 192,  98,  48, 
    143, 106, 189, 249, 164,  43, 
    191,   0, 147, 175,  85, 226, 
      0,   0,  83,  84,  65,  84, 
    148,   0,   0,   0,   5,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   4,   0, 
      0,   0,   2,   0,   0,   0, 
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
