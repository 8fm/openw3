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
//   float mipLevels;                   // Offset:    0 Size:     4
//   float2 textureDims;                // Offset:   16 Size:     8
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// sampler_tex                       sampler      NA          NA    0        1
// tex                               texture  float4          2d    0        1
// Constants                         cbuffer      NA          NA    0        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float      w
// TEXCOORD                 0   xy          1     NONE   float   xy  
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Target                0   xyzw        0   TARGET   float   xyzw
//
ps_5_0
Opaque Custom Data - XBOX Precompiled Shader Header
// First Precompiled Shader at offset:[223]
// Embedded Data:
//  0x000000df - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x000000da - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_constantbuffer cb0[2], immediateIndexed
dcl_sampler s0, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_input_ps linear v0.w
dcl_input_ps linear v1.xy
dcl_output o0.xyzw
dcl_temps 2
mul r0.xyzw, v1.xyxy, cb0[1].xxyy
deriv_rtx_coarse r0.xy, r0.xyxx
deriv_rty_coarse r0.zw, r0.zzzw
dp2 r1.x, r0.xyxx, r0.xyxx
dp2 r1.y, r0.zwzz, r0.zwzz
max r1.x, r1.y, r1.x
log r1.x, r1.x
mad r1.x, r1.x, l(0.500000), l(-1.000000)
max r1.x, r1.x, l(0.000000)
add r1.y, l(-1.000000), cb0[0].x
min r1.x, r1.y, r1.x
exp r1.x, r1.x
div r0.xyzw, r0.xyzw, r1.xxxx
dp2 r0.z, r0.zwzz, r0.zwzz
dp2 r0.x, r0.xyxx, r0.xyxx
max r0.x, r0.z, r0.x
sqrt r0.x, r0.x
mad r0.x, -r0.x, l(0.500000), l(1.000000)
max r0.x, r0.x, l(0.000000)
mul r0.y, r0.x, l(1.882353)
mad r0.xz, r0.yyyy, l(-1.000000, 0.000000, 1.000000, 0.000000), l(2.000000, 0.000000, -2.000000, 0.000000)
mov_sat r0.xyz, r0.xyzx
mov r0.w, l(1.000000)
sample_indexable(texture2d)(float,float,float,float) r1.xyzw, v1.xyxx, t0.xyzw, s0
mul r1.w, r1.w, v0.w
mad o0.xyzw, r1.xyzw, l(0.001000, 0.001000, 0.001000, 0.001000), r0.xyzw
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[223], bundle is:[205] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FTexTGEAlphaTexDensity.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask  8, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask  3, param1, paramSlot 1, DefaultVal={0,0,0,0}

codeLenInByte        = 400;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 3;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_SAMPLER, 0, offset 8:11 dwords
;  extUserElements 1[2] = IMM_CONST_BUFFER, 0, offset 12:15 dwords
NumVgprs             = 14;
NumSgprs             = 18;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000001
; constBufUsage           = 0x00000001

; SPI_SHADER_PGM_RSRC2_PS = 0x00000004
SSPRP:SCRATCH_EN            = 0
SSPRP:USER_SGPR             = 2
SSPRP:TRAP_PRESENT          = 0
SSPRP:WAVE_CNT_EN           = 0
SSPRP:EXTRA_LDS_SIZE        = 0
SSPRP:EXCP_EN               = 0
; SPI_SHADER_Z_FORMAT     = 0x00000000
SPZF:Z_EXPORT_FORMAT        = 0; SPI_SHADER_ZERO
; SPI_PS_IN_CONTROL       = 0x00000002
SPIC:NUM_INTERP             = 2
SPIC:PARAM_GEN              = 0
SPIC:FOG_ADDR               = 0
SPIC:BC_OPTIMIZE_DISABLE    = 0
SPIC:PASS_FOG_THROUGH_PS    = 0
; SPI_PS_INPUT_ADDR       = 0x00000002
SPIA:PERSP_CENTER_ENA       = 1
; DB_SHADER_CONTROL       = 0x00000010
DB:Z_ORDER                  = 1
DB:CONSERVATIVE_Z_EXPORT    = 0; EXPORT_ANY_Z
; CB_SHADER_MASK          = 0x0000000F
CB:OUTPUT0_ENABLE           = 15

// Shader Instructions:
shader main
  asic(CI)
  type(PS)
                                                            // s_ps_state in s0
  s_mov_b64     s[16:17], exec                          // 000000000000: BE90047E
  s_wqm_b64     exec, exec                              // 000000000004: BEFE0A7E
  s_mov_b32     m0, s2                                  // 000000000008: BEFC0302
  s_load_dwordx8  s[4:11], s[0:1], 0x00                 // 00000000000C: C0C20100
  s_load_dwordx4  s[12:15], s[0:1], 0x08                // 000000000010: C0860108
  v_interp_p1_f32  v10, v0, attr1.x                     // 000000000014: C8280400
  v_interp_p1_f32  v11, v0, attr1.y                     // 000000000018: C82C0500
  v_interp_p2_f32  v10, v1, attr1.x                     // 00000000001C: C8290401
  v_interp_p2_f32  v11, v1, attr1.y                     // 000000000020: C82D0501
  s_waitcnt     lgkmcnt(0)                              // 000000000024: BF8C007F
  image_sample  v[4:7], v[10:11], s[4:11], s[12:15] dmask:0xf // 000000000028: F0800F00 0061040A
  s_load_dwordx4  s[0:3], s[0:1], 0x0c                  // 000000000030: C080010C
  s_waitcnt     lgkmcnt(0)                              // 000000000034: BF8C007F
  s_buffer_load_dwordx2  s[4:5], s[0:3], 0x04           // 000000000038: C2420104
  s_waitcnt     lgkmcnt(0)                              // 00000000003C: BF8C007F
  v_mul_legacy_f32  v8, s4, v10                         // 000000000040: 0E101404
  v_mul_legacy_f32  v2, s5, v10                         // 000000000044: 0E041405
  v_mul_legacy_f32  v9, s4, v11                         // 000000000048: 0E121604
  v_mul_legacy_f32  v3, s5, v11                         // 00000000004C: 0E061605
  ds_swizzle_b32  v10, v8 offset:32853                  // 000000000050: D8D48055 0A000008
  ds_swizzle_b32  v11, v2 offset:32938                  // 000000000058: D8D480AA 0B000002
  ds_swizzle_b32  v8, v8 offset:32768                   // 000000000060: D8D48000 08000008
  ds_swizzle_b32  v2, v2 offset:32768                   // 000000000068: D8D48000 02000002
  s_waitcnt     lgkmcnt(1)                              // 000000000070: BF8C017F
  v_sub_f32     v8, v10, v8                             // 000000000074: 0810110A
  s_waitcnt     lgkmcnt(0)                              // 000000000078: BF8C007F
  v_sub_f32     v2, v11, v2                             // 00000000007C: 0804050B
  ds_swizzle_b32  v10, v9 offset:32853                  // 000000000080: D8D48055 0A000009
  ds_swizzle_b32  v11, v3 offset:32938                  // 000000000088: D8D480AA 0B000003
  ds_swizzle_b32  v9, v9 offset:32768                   // 000000000090: D8D48000 09000009
  ds_swizzle_b32  v3, v3 offset:32768                   // 000000000098: D8D48000 03000003
  s_waitcnt     lgkmcnt(1)                              // 0000000000A0: BF8C017F
  v_sub_f32     v9, v10, v9                             // 0000000000A4: 0812130A
  s_waitcnt     lgkmcnt(0)                              // 0000000000A8: BF8C007F
  v_sub_f32     v3, v11, v3                             // 0000000000AC: 0806070B
  s_buffer_load_dword  s0, s[0:3], 0x00                 // 0000000000B0: C2000100
  v_mul_legacy_f32  v10, v8, v8                         // 0000000000B4: 0E141108
  v_mul_legacy_f32  v11, v2, v2                         // 0000000000B8: 0E160502
  v_mac_legacy_f32  v10, v9, v9                         // 0000000000BC: 0C141309
  v_mac_legacy_f32  v11, v3, v3                         // 0000000000C0: 0C160703
  v_max_legacy_f32  v10, v10, v11                       // 0000000000C4: 1C14170A
  v_log_f32     v10, v10                                // 0000000000C8: 7E144F0A
  v_mad_legacy_f32  v10, v10, 0.5, -1.0                 // 0000000000CC: D280000A 03CDE10A
  v_max_legacy_f32  v10, v10, 0                         // 0000000000D4: D21C000A 0001010A
  s_waitcnt     lgkmcnt(0)                              // 0000000000DC: BF8C007F
  v_add_f32     v11, s0, -1.0                           // 0000000000E0: D206000B 0001E600
  v_min_legacy_f32  v10, v10, v11                       // 0000000000E8: 1A14170A
  v_exp_f32     v10, v10                                // 0000000000EC: 7E144B0A
  v_rcp_f32     v10, v10                                // 0000000000F0: 7E14550A
  v_mul_f32     v8, v8, v10                             // 0000000000F4: 10101508
  v_mul_f32     v2, v2, v10                             // 0000000000F8: 10041502
  v_mul_f32     v9, v9, v10                             // 0000000000FC: 10121509
  v_mul_f32     v3, v3, v10                             // 000000000100: 10061503
  v_mul_legacy_f32  v2, v2, v2                          // 000000000104: 0E040502
  v_mul_legacy_f32  v8, v8, v8                          // 000000000108: 0E101108
  v_mac_legacy_f32  v2, v3, v3                          // 00000000010C: 0C040703
  v_mac_legacy_f32  v8, v9, v9                          // 000000000110: 0C101309
  v_max_legacy_f32  v2, v2, v8                          // 000000000114: 1C041102
  v_interp_p1_f32  v0, v0, attr0.w                      // 000000000118: C8000300
  v_sqrt_f32    v2, v2                                  // 00000000011C: 7E046702
  v_mad_legacy_f32  v2, -v2, 0.5, 1.0                   // 000000000120: D2800002 23C9E102
  v_interp_p2_f32  v0, v1, attr0.w                      // 000000000128: C8010301
  v_max_legacy_f32  v2, v2, 0                           // 00000000012C: D21C0002 00010102
  s_mov_b32     s0, 0x3ff0f0f2                          // 000000000134: BE8003FF 3FF0F0F2
  v_mul_legacy_f32  v1, v2, s0 clamp                    // 00000000013C: D20E0801 00000102
  v_mad_legacy_f32  v3, -v2, s0, 2.0 clamp              // 000000000144: D2800803 23D00102
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 00000000014C: BF8C0F70
  v_mul_legacy_f32  v0, v7, v0                          // 000000000150: 0E000107
  v_mad_legacy_f32  v2, v2, s0, -2.0 clamp              // 000000000154: D2800802 03D40102
  v_mov_b32     v7, 0x3a83126f                          // 00000000015C: 7E0E02FF 3A83126F
  v_mac_legacy_f32  v3, v4, v7                          // 000000000164: 0C060F04
  v_mac_legacy_f32  v1, v5, v7                          // 000000000168: 0C020F05
  v_mac_legacy_f32  v2, v6, v7                          // 00000000016C: 0C040F06
  v_mad_legacy_f32  v0, v0, v7, 1.0                     // 000000000170: D2800000 03CA0F00
  s_mov_b64     exec, s[16:17]                          // 000000000178: BEFE0410
  v_cvt_pkrtz_f16_f32  v1, v3, v1                       // 00000000017C: 5E020303
  v_cvt_pkrtz_f16_f32  v0, v2, v0                       // 000000000180: 5E000102
  exp           mrt0, v1, v1, v0, v0 compr vm           // 000000000184: F800140F 00000001
  s_endpgm                                              // 00000000018C: BF810000
end


// Approximately 27 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FTexTGEAlphaTexDensity[] =
{
     68,  88,  66,  67,  29,  40, 
     87, 240, 144,  25,  39, 229, 
      7, 129, 230,  97,  70, 140, 
     71, 111,   1,   0,   0,   0, 
    228,   9,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
      4,   2,   0,   0,  84,   2, 
      0,   0, 136,   2,   0,   0, 
     72,   9,   0,   0,  82,  68, 
     69,  70, 200,   1,   0,   0, 
      1,   0,   0,   0, 184,   0, 
      0,   0,   3,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
    140,   1,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
    156,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0, 168,   0,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     13,   0,   0,   0, 172,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
    115,  97, 109, 112, 108, 101, 
    114,  95, 116, 101, 120,   0, 
    116, 101, 120,   0,  67, 111, 
    110, 115, 116,  97, 110, 116, 
    115,   0, 171, 171, 172,   0, 
      0,   0,   2,   0,   0,   0, 
    208,   0,   0,   0,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  32,   1, 
      0,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,   2,   0, 
      0,   0,  48,   1,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,  84,   1,   0,   0, 
     16,   0,   0,   0,   8,   0, 
      0,   0,   2,   0,   0,   0, 
    104,   1,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    109, 105, 112,  76, 101, 118, 
    101, 108, 115,   0, 102, 108, 
    111,  97, 116,   0,   0,   0, 
      3,   0,   1,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     42,   1,   0,   0, 116, 101, 
    120, 116, 117, 114, 101,  68, 
    105, 109, 115,   0, 102, 108, 
    111,  97, 116,  50,   0, 171, 
      1,   0,   3,   0,   1,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  96,   1,   0,   0, 
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
     73,  83,  71,  78,  72,   0, 
      0,   0,   2,   0,   0,   0, 
      8,   0,   0,   0,  56,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   8,   0,   0,  62,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
      3,   3,   0,   0,  67,  79, 
     76,  79,  82,   0,  84,  69, 
     88,  67,  79,  79,  82,  68, 
      0, 171,  79,  83,  71,  78, 
     44,   0,   0,   0,   1,   0, 
      0,   0,   8,   0,   0,   0, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   0,   0,   0, 
     83,  86,  95,  84,  97, 114, 
    103, 101, 116,   0, 171, 171, 
     83,  72,  69,  88, 184,   6, 
      0,   0,  80,   0,   0,   0, 
    174,   1,   0,   0,  53,  16, 
      0,   0,   5,   0,   0,   0, 
    223,   0,   0,   0,  80,   0, 
      0,   0, 218,   0,   0,   0, 
    106,   8,   0,   1,  89,   0, 
      0,   4,  70, 142,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,  90,   0,   0,   3, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   0,   0, 
      0,   0,  85,  85,   0,   0, 
     98,  16,   0,   3, 130,  16, 
     16,   0,   0,   0,   0,   0, 
     98,  16,   0,   3,  50,  16, 
     16,   0,   1,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    104,   0,   0,   2,   2,   0, 
      0,   0,  56,   0,   0,   8, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  20,  16,   0, 
      1,   0,   0,   0,   6, 133, 
     32,   0,   0,   0,   0,   0, 
      1,   0,   0,   0, 122,   0, 
      0,   5,  50,   0,  16,   0, 
      0,   0,   0,   0,  70,   0, 
     16,   0,   0,   0,   0,   0, 
    124,   0,   0,   5, 194,   0, 
     16,   0,   0,   0,   0,   0, 
    166,  14,  16,   0,   0,   0, 
      0,   0,  15,   0,   0,   7, 
     18,   0,  16,   0,   1,   0, 
      0,   0,  70,   0,  16,   0, 
      0,   0,   0,   0,  70,   0, 
     16,   0,   0,   0,   0,   0, 
     15,   0,   0,   7,  34,   0, 
     16,   0,   1,   0,   0,   0, 
    230,  10,  16,   0,   0,   0, 
      0,   0, 230,  10,  16,   0, 
      0,   0,   0,   0,  52,   0, 
      0,   7,  18,   0,  16,   0, 
      1,   0,   0,   0,  26,   0, 
     16,   0,   1,   0,   0,   0, 
     10,   0,  16,   0,   1,   0, 
      0,   0,  47,   0,   0,   5, 
     18,   0,  16,   0,   1,   0, 
      0,   0,  10,   0,  16,   0, 
      1,   0,   0,   0,  50,   0, 
      0,   9,  18,   0,  16,   0, 
      1,   0,   0,   0,  10,   0, 
     16,   0,   1,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,  63,   1,  64,   0,   0, 
      0,   0, 128, 191,  52,   0, 
      0,   7,  18,   0,  16,   0, 
      1,   0,   0,   0,  10,   0, 
     16,   0,   1,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   8, 
     34,   0,  16,   0,   1,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128, 191,  10, 128, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  51,   0, 
      0,   7,  18,   0,  16,   0, 
      1,   0,   0,   0,  26,   0, 
     16,   0,   1,   0,   0,   0, 
     10,   0,  16,   0,   1,   0, 
      0,   0,  25,   0,   0,   5, 
     18,   0,  16,   0,   1,   0, 
      0,   0,  10,   0,  16,   0, 
      1,   0,   0,   0,  14,   0, 
      0,   7, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
      6,   0,  16,   0,   1,   0, 
      0,   0,  15,   0,   0,   7, 
     66,   0,  16,   0,   0,   0, 
      0,   0, 230,  10,  16,   0, 
      0,   0,   0,   0, 230,  10, 
     16,   0,   0,   0,   0,   0, 
     15,   0,   0,   7,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     70,   0,  16,   0,   0,   0, 
      0,   0,  70,   0,  16,   0, 
      0,   0,   0,   0,  52,   0, 
      0,   7,  18,   0,  16,   0, 
      0,   0,   0,   0,  42,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,  75,   0,   0,   5, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  50,   0, 
      0,  10,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,  63, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  52,   0,   0,   7, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,   0, 
     56,   0,   0,   7,  34,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
    242, 240, 240,  63,  50,   0, 
      0,  15,  82,   0,  16,   0, 
      0,   0,   0,   0,  86,   5, 
     16,   0,   0,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
    128, 191,   0,   0,   0,   0, 
      0,   0, 128,  63,   0,   0, 
      0,   0,   2,  64,   0,   0, 
      0,   0,   0,  64,   0,   0, 
      0,   0,   0,   0,   0, 192, 
      0,   0,   0,   0,  54,  32, 
      0,   5, 114,   0,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   5, 130,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  69,   0,   0, 139, 
    194,   0,   0, 128,  67,  85, 
     21,   0, 242,   0,  16,   0, 
      1,   0,   0,   0,  70,  16, 
     16,   0,   1,   0,   0,   0, 
     70, 126,  16,   0,   0,   0, 
      0,   0,   0,  96,  16,   0, 
      0,   0,   0,   0,  56,   0, 
      0,   7, 130,   0,  16,   0, 
      1,   0,   0,   0,  58,   0, 
     16,   0,   1,   0,   0,   0, 
     58,  16,  16,   0,   0,   0, 
      0,   0,  50,   0,   0,  12, 
    242,  32,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      1,   0,   0,   0,   2,  64, 
      0,   0, 111,  18, 131,  58, 
    111,  18, 131,  58, 111,  18, 
    131,  58, 111,  18, 131,  58, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  62,   0,   0,   1, 
     53,  16,   0,   0, 207,   0, 
      0,   0,  21,   0,   1,   0, 
     41, 109,  11,   0, 120,   1, 
    237,  86, 207, 107,  19,  65, 
     20, 158, 153, 157,  77,  54, 
    219, 176, 187,  73, 182, 181, 
    149,  42, 130,  40,  21,  52, 
     82, 235, 169,   8,  86, 169, 
     63,  64, 161, 208,  31, 151, 
    178, 180,  77, 109, 218,  20, 
    211, 180,  36,  69, 162, 135, 
     52,  34, 136, 135,  30, 122, 
    209, 191, 192, 131, 136, 255, 
    128,   8,  73, 172,  10,  81, 
     16,  36, 244, 224, 181, 151, 
    122,  45, 122, 233,  65,  90, 
    191, 183,  63, 100,  15, 246, 
     86,  80, 176,  31, 124, 243, 
    230, 219, 121, 243, 230, 237, 
     62, 216,  55,  11, 109, 204, 
    197, 251,  31, 111, 207, 147, 
     61, 254, 252, 219, 135, 156, 
    194, 152, 138,  57,   7, 201, 
     18, 202,  26, 141, 140, 221, 
    212, 105, 100, 172, 135,   6, 
    114,  16, 140,  25,  48,  73, 
    112,  13, 122,   0, 150, 214, 
      2, 122,  24, 104,  20,  49, 
     90,  96,  28,  28, 198, 158, 
    219, 176, 151, 192,  20, 216, 
     11, 218,  32, 133, 163,  99, 
    200,   6, 240, 143, 251, 157, 
     71,   0, 210, 199, 192,  24, 
     72, 136, 128, 129,  15, 237, 
      9, 199,  96, 167, 104, 248, 
     51, 134,  64, 202, 127,  63, 
     80, 110,  20,  43, 200, 139, 
    242,  36, 208, 251, 254, 109, 
     80,  62, 225, 145,  24, 212, 
    137,  64, 245,   8, 131, 222, 
     51, 120, 164, 249,  31,  22, 
    165, 118, 209, 233, 107, 242, 
    161, 119, 107, 128, 251,  97, 
     15, 160,  90, 134,  17, 104, 
    148,  22, 181, 240, 146, 136, 
    240,  20,  59,  65, 147, 255, 
     12, 146,   6, 128, 190,   5, 
    145, 190, 141,   9,   6, 160, 
    245,  73, 144, 177, 106, 157, 
    184, 214, 201, 118,  72,   5, 
    154,  67, 123, 117, 241, 180, 
     66, 218,  45,  92,  72,  67, 
     41, 138, 152,  16, 156,  77, 
    152, 221, 108, 135, 150, 161, 
     29, 104,  39, 164, 199, 161, 
    199,  67, 122,  10, 122,  42, 
    164, 167, 161, 167,   3,  29, 
    196,  55, 221, 243, 132,  91, 
    103,  60, 172, 252, 203,  60, 
    204, 241,  96, 120, 152, 227, 
    193, 240,  48, 199,   3, 163, 
    139, 138,  92, 171,  85, 244, 
    221, 154,  80, 126, 214,  24, 
     95, 111, 104, 252, 113, 131, 
    201, 158,  38,  83, 207,  54, 
    185,  60, 211, 228, 234, 185, 
    230,  10,  91, 173,  51, 179, 
    186, 173, 203,  12, 139, 243, 
    106, 131, 180, 228,  87, 215, 
     93, 107,  91, 134, 106,  75, 
     67, 118,  36,  13, 181,  35, 
     98, 140,  85,  55, 190, 162, 
     59, 233,  47,  97, 241, 111, 
    110,  99, 158, 214, 200,  66, 
    139,  21, 190,  90, 215,  19, 
    150,  70, 123, 219,  84, 169, 
    145,  63, 238,  24, 174,  63, 
    254, 185, 174,  63, 116, 140, 
     44, 180, 226, 250, 167, 146, 
    158, 127,  52, 162, 161, 193, 
    174, 107,   9, 219,  16, 106, 
    135,  17,  75, 217, 113,  37, 
    218,  17, 215, 143, 216, 221, 
    250, 144,  93, 209,  89, 181, 
    165, 111, 126,  86, 116, 214, 
    221, 210,  57, 103, 238,  30, 
     22, 105, 177,  45, 206, 224, 
    115,  84, 191,   5, 159,  49, 
    187, 162, 181,  91, 150, 104, 
    151,  86, 172,  61, 105,  41, 
    237,  17,  75, 168, 210, 208, 
     18, 150, 161,  68, 101,  60, 
    150, 178, 226,  34,  33, 187, 
    209, 184, 155,  98,  78,  86, 
      4,  98, 138, 205, 143,  39, 
    185, 194, 155,   2, 113,   5, 
    226, 238,  41, 213, 218, 247, 
    237, 237, 203,  92,  51, 160, 
    145, 164,   6,  31, 254, 229, 
    228, 146, 185,  90, 143, 114, 
    102,   8,  87, 111,  40, 123, 
    194, 168,  44,  38,  31, 245, 
     75,  51,  18,  87,  77,  17, 
    143, 152,  18, 215, 179, 106, 
    139, 153, 159,  20,  75, 238, 
    214, 208,  31, 188, 254,  98, 
      7, 253, 225,  97, 189,  11, 
    163,  55, 247, 122,  91,  24, 
    248, 142,  56, 221,   3, 230, 
    212, 231, 208,  69,  60, 208, 
    189, 143, 244, 146, 127, 207, 
    152, 242,  45, 245,  70,  60, 
    199, 189,  35, 216, 233,  97, 
    205,  95, 127, 226,  91, 138, 
     71,  87, 213, 103,  91, 175, 
    251, 250, 107,  93, 111,  94, 
    249, 207,  95, 248, 150,  98, 
    208, 244, 221, 211, 201, 211, 
     61, 193, 157, 214,  95, 155, 
      1, 105, 255, 108, 191,  51, 
    157, 201, 103,  30, 148, 210, 
    203, 139, 197, 197, 187, 233, 
    210, 114,  49, 155,  89, 184, 
    224, 204, 100, 239,  57, 217, 
    242, 114, 182,  88, 200, 228, 
    157, 185, 217, 242,  69, 103, 
    164, 120, 199,  25, 206,  22, 
    102, 178,  69, 103, 176, 111, 
    176, 183, 236, 140, 228,  50, 
     16,  37, 231, 250, 104, 182, 
     60, 122, 227, 218, 149, 252, 
     82,  46, 131, 233,  96, 182, 
     80, 154,  95, 190, 159, 158, 
    205, 247, 246, 166, 115, 249, 
     82,  30, 167, 120,  24, 240, 
    115, 160,  59, 105,  20,  92, 
    200, 204,  23, 216,  47,  45, 
     88, 102,  89,   0,   0,   0, 
     83,  84,  65,  84, 148,   0, 
      0,   0,  27,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
     23,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
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
