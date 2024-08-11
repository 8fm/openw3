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
// sampler_tex[0]                    sampler      NA          NA    0        1
// sampler_tex[1]                    sampler      NA          NA    1        1
// sampler_tex[2]                    sampler      NA          NA    2        1
// sampler_tex[3]                    sampler      NA          NA    3        1
// tex[0]                            texture  float4          2d    0        1
// tex[1]                            texture  float4          2d    1        1
// tex[2]                            texture  float4          2d    2        1
// tex[3]                            texture  float4          2d    3        1
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
// First Precompiled Shader at offset:[332]
// Embedded Data:
//  0x0000014c - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x00000147 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_constantbuffer cb0[2], immediateIndexed
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_sampler s2, mode_default
dcl_sampler s3, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_resource_texture2d (float,float,float,float) t2
dcl_resource_texture2d (float,float,float,float) t3
dcl_input_ps linear v0.w
dcl_input_ps linear v1.xy
dcl_output o0.xyzw
dcl_temps 3
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
sample_indexable(texture2d)(float,float,float,float) r1.x, v1.xyxx, t2.xyzw, s2
add r1.x, r1.x, l(-0.501961)
mul r1.xyz, r1.xxxx, l(1.596000, -0.813000, 0.000000, 0.000000)
sample_indexable(texture2d)(float,float,float,float) r1.w, v1.xyxx, t0.yzwx, s0
add r1.w, r1.w, l(-0.062745)
mad r1.xyz, r1.wwww, l(1.164000, 1.164000, 1.164000, 0.000000), r1.xyzx
sample_indexable(texture2d)(float,float,float,float) r1.w, v1.xyxx, t1.yzwx, s1
add r1.w, r1.w, l(-0.501961)
mad r1.xyz, r1.wwww, l(0.000000, -0.392000, 2.017000, 0.000000), r1.xyzx
sample_indexable(texture2d)(float,float,float,float) r2.x, v1.xyxx, t3.xyzw, s3
mul r1.w, r2.x, v0.w
mov r0.w, l(1.000000)
mad o0.xyzw, r1.xyzw, l(0.001000, 0.001000, 0.001000, 0.001000), r0.xyzw
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[332], bundle is:[247] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FYUVAEAlphaTexDensity.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask  8, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask  3, param1, paramSlot 1, DefaultVal={0,0,0,0}

codeLenInByte        = 548;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 9;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_RESOURCE, 1, offset 8:15 dwords
;  extUserElements 1[2] = IMM_RESOURCE, 2, offset 16:23 dwords
;  extUserElements 1[3] = IMM_RESOURCE, 3, offset 24:31 dwords
;  extUserElements 1[4] = IMM_SAMPLER, 0, offset 32:35 dwords
;  extUserElements 1[5] = IMM_SAMPLER, 1, offset 36:39 dwords
;  extUserElements 1[6] = IMM_SAMPLER, 2, offset 40:43 dwords
;  extUserElements 1[7] = IMM_SAMPLER, 3, offset 44:47 dwords
;  extUserElements 1[8] = IMM_CONST_BUFFER, 0, offset 48:51 dwords
NumVgprs             = 14;
NumSgprs             = 54;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x0000000F
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
  s_mov_b64     s[52:53], exec                          // 000000000000: BEB4047E
  s_wqm_b64     exec, exec                              // 000000000004: BEFE0A7E
  s_mov_b32     m0, s2                                  // 000000000008: BEFC0302
  s_load_dwordx8  s[4:11], s[0:1], 0x10                 // 00000000000C: C0C20110
  s_load_dwordx4  s[12:15], s[0:1], 0x28                // 000000000010: C0860128
  v_interp_p1_f32  v10, v0, attr1.x                     // 000000000014: C8280400
  v_interp_p1_f32  v11, v0, attr1.y                     // 000000000018: C82C0500
  v_interp_p2_f32  v10, v1, attr1.x                     // 00000000001C: C8290401
  v_interp_p2_f32  v11, v1, attr1.y                     // 000000000020: C82D0501
  s_waitcnt     lgkmcnt(0)                              // 000000000024: BF8C007F
  image_sample  v[6:9], v[10:11], s[4:11], s[12:15]     // 000000000028: F0800100 0061060A
  s_load_dwordx8  s[16:23], s[0:1], 0x00                // 000000000030: C0C80100
  s_load_dwordx8  s[32:39], s[0:1], 0x08                // 000000000034: C0D00108
  s_load_dwordx8  s[40:47], s[0:1], 0x18                // 000000000038: C0D40118
  s_load_dwordx8  s[24:31], s[0:1], 0x20                // 00000000003C: C0CC0120
  s_load_dwordx4  s[48:51], s[0:1], 0x2c                // 000000000040: C098012C
  s_waitcnt     lgkmcnt(0)                              // 000000000044: BF8C007F
  image_sample  v[7:10], v[10:11], s[16:23], s[24:27]   // 000000000048: F0800100 00C4070A
  image_sample  v[8:11], v[10:11], s[32:39], s[28:31]   // 000000000050: F0800100 00E8080A
  image_sample  v[4:7], v[10:11], s[40:47], s[48:51]    // 000000000058: F0800100 018A040A
  s_load_dwordx4  s[0:3], s[0:1], 0x30                  // 000000000060: C0800130
  s_waitcnt     lgkmcnt(0)                              // 000000000064: BF8C007F
  s_buffer_load_dwordx2  s[4:5], s[0:3], 0x04           // 000000000068: C2420104
  s_waitcnt     lgkmcnt(0)                              // 00000000006C: BF8C007F
  v_mul_legacy_f32  v5, s4, v10                         // 000000000070: 0E0A1404
  v_mul_legacy_f32  v2, s5, v10                         // 000000000074: 0E041405
  v_mul_legacy_f32  v9, s4, v11                         // 000000000078: 0E121604
  v_mul_legacy_f32  v3, s5, v11                         // 00000000007C: 0E061605
  ds_swizzle_b32  v10, v5 offset:32853                  // 000000000080: D8D48055 0A000005
  ds_swizzle_b32  v11, v2 offset:32938                  // 000000000088: D8D480AA 0B000002
  ds_swizzle_b32  v5, v5 offset:32768                   // 000000000090: D8D48000 05000005
  ds_swizzle_b32  v2, v2 offset:32768                   // 000000000098: D8D48000 02000002
  s_waitcnt     lgkmcnt(1)                              // 0000000000A0: BF8C017F
  v_sub_f32     v5, v10, v5                             // 0000000000A4: 080A0B0A
  s_waitcnt     lgkmcnt(0)                              // 0000000000A8: BF8C007F
  v_sub_f32     v2, v11, v2                             // 0000000000AC: 0804050B
  ds_swizzle_b32  v10, v9 offset:32853                  // 0000000000B0: D8D48055 0A000009
  ds_swizzle_b32  v11, v3 offset:32938                  // 0000000000B8: D8D480AA 0B000003
  ds_swizzle_b32  v9, v9 offset:32768                   // 0000000000C0: D8D48000 09000009
  ds_swizzle_b32  v3, v3 offset:32768                   // 0000000000C8: D8D48000 03000003
  s_waitcnt     lgkmcnt(1)                              // 0000000000D0: BF8C017F
  v_sub_f32     v9, v10, v9                             // 0000000000D4: 0812130A
  s_waitcnt     lgkmcnt(0)                              // 0000000000D8: BF8C007F
  v_sub_f32     v3, v11, v3                             // 0000000000DC: 0806070B
  s_buffer_load_dword  s0, s[0:3], 0x00                 // 0000000000E0: C2000100
  v_mul_legacy_f32  v10, v5, v5                         // 0000000000E4: 0E140B05
  v_mul_legacy_f32  v11, v2, v2                         // 0000000000E8: 0E160502
  v_mac_legacy_f32  v10, v9, v9                         // 0000000000EC: 0C141309
  v_mac_legacy_f32  v11, v3, v3                         // 0000000000F0: 0C160703
  v_max_legacy_f32  v10, v10, v11                       // 0000000000F4: 1C14170A
  v_log_f32     v10, v10                                // 0000000000F8: 7E144F0A
  v_mad_legacy_f32  v10, v10, 0.5, -1.0                 // 0000000000FC: D280000A 03CDE10A
  v_max_legacy_f32  v10, v10, 0                         // 000000000104: D21C000A 0001010A
  s_waitcnt     lgkmcnt(0)                              // 00000000010C: BF8C007F
  v_add_f32     v11, s0, -1.0                           // 000000000110: D206000B 0001E600
  v_min_legacy_f32  v10, v10, v11                       // 000000000118: 1A14170A
  v_exp_f32     v10, v10                                // 00000000011C: 7E144B0A
  v_rcp_f32     v10, v10                                // 000000000120: 7E14550A
  v_mul_f32     v5, v5, v10                             // 000000000124: 100A1505
  v_mul_f32     v2, v2, v10                             // 000000000128: 10041502
  v_mul_f32     v9, v9, v10                             // 00000000012C: 10121509
  v_mul_f32     v3, v3, v10                             // 000000000130: 10061503
  v_mul_legacy_f32  v2, v2, v2                          // 000000000134: 0E040502
  v_mul_legacy_f32  v5, v5, v5                          // 000000000138: 0E0A0B05
  v_mac_legacy_f32  v2, v3, v3                          // 00000000013C: 0C040703
  v_mac_legacy_f32  v5, v9, v9                          // 000000000140: 0C0A1309
  v_max_legacy_f32  v2, v2, v5                          // 000000000144: 1C040B02
  v_interp_p1_f32  v0, v0, attr0.w                      // 000000000148: C8000300
  v_sqrt_f32    v2, v2                                  // 00000000014C: 7E046702
  s_waitcnt     vmcnt(3) & lgkmcnt(15)                  // 000000000150: BF8C0F73
  v_add_f32     v3, 0xbf008081, v6                      // 000000000154: 06060CFF BF008081
  v_mad_legacy_f32  v2, -v2, 0.5, 1.0                   // 00000000015C: D2800002 23C9E102
  v_interp_p2_f32  v0, v1, attr0.w                      // 000000000164: C8010301
  v_mul_legacy_f32  v5, 0xbf5020c5, v3                  // 000000000168: 0E0A06FF BF5020C5
  s_waitcnt     vmcnt(2) & lgkmcnt(15)                  // 000000000170: BF8C0F72
  v_add_f32     v6, 0xbd808081, v7                      // 000000000174: 060C0EFF BD808081
  s_mov_b32     s0, 0x3f94fdf4                          // 00000000017C: BE8003FF 3F94FDF4
  v_max_legacy_f32  v2, v2, 0                           // 000000000184: D21C0002 00010102
  s_mov_b32     s1, 0x3ff0f0f2                          // 00000000018C: BE8103FF 3FF0F0F2
  v_mul_legacy_f32  v3, 0x3fcc49ba, v3                  // 000000000194: 0E0606FF 3FCC49BA
  v_mul_legacy_f32  v7, 0x3f94fdf4, v6                  // 00000000019C: 0E0E0CFF 3F94FDF4
  v_mac_legacy_f32  v5, s0, v6                          // 0000000001A4: 0C0A0C00
  s_waitcnt     vmcnt(1) & lgkmcnt(15)                  // 0000000001A8: BF8C0F71
  v_add_f32     v8, 0xbf008081, v8                      // 0000000001AC: 061010FF BF008081
  s_mov_b32     s2, 0xbec8b439                          // 0000000001B4: BE8203FF BEC8B439
  s_mov_b32     s3, 0x40011687                          // 0000000001BC: BE8303FF 40011687
  v_mul_legacy_f32  v1, v2, s1 clamp                    // 0000000001C4: D20E0801 00000302
  v_mad_legacy_f32  v9, -v2, s1, 2.0 clamp              // 0000000001CC: D2800809 23D00302
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 0000000001D4: BF8C0F70
  v_mul_legacy_f32  v0, v4, v0                          // 0000000001D8: 0E000104
  v_mad_legacy_f32  v2, v2, s1, -2.0 clamp              // 0000000001DC: D2800802 03D40302
  v_mac_legacy_f32  v3, s0, v6                          // 0000000001E4: 0C060C00
  v_mac_legacy_f32  v5, s2, v8                          // 0000000001E8: 0C0A1002
  v_mac_legacy_f32  v7, s3, v8                          // 0000000001EC: 0C0E1003
  v_mov_b32     v4, 0x3a83126f                          // 0000000001F0: 7E0802FF 3A83126F
  v_mac_legacy_f32  v9, v3, v4                          // 0000000001F8: 0C120903
  v_mac_legacy_f32  v1, v5, v4                          // 0000000001FC: 0C020905
  v_mac_legacy_f32  v2, v7, v4                          // 000000000200: 0C040907
  v_mad_legacy_f32  v0, v0, v4, 1.0                     // 000000000204: D2800000 03CA0900
  s_mov_b64     exec, s[52:53]                          // 00000000020C: BEFE0434
  v_cvt_pkrtz_f16_f32  v1, v9, v1                       // 000000000210: 5E020309
  v_cvt_pkrtz_f16_f32  v0, v2, v0                       // 000000000214: 5E000102
  exp           mrt0, v1, v1, v0, v0 compr vm           // 000000000218: F800140F 00000001
  s_endpgm                                              // 000000000220: BF810000
end


// Approximately 36 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FYUVAEAlphaTexDensity[] =
{
     68,  88,  66,  67,  79,  62, 
    153,  66,  23,  32, 191, 160, 
     67, 230, 130,  95, 155, 118, 
    226, 214,   1,   0,   0,   0, 
     72,  13,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
     12,   3,   0,   0,  92,   3, 
      0,   0, 144,   3,   0,   0, 
    172,  12,   0,   0,  82,  68, 
     69,  70, 208,   2,   0,   0, 
      1,   0,   0,   0, 192,   1, 
      0,   0,   9,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
    148,   2,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
     92,   1,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0, 107,   1,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
      1,   0,   0,   0, 122,   1, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
    137,   1,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0, 152,   1,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     13,   0,   0,   0, 159,   1, 
      0,   0,   2,   0,   0,   0, 
      5,   0,   0,   0,   4,   0, 
      0,   0, 255, 255, 255, 255, 
      1,   0,   0,   0,   1,   0, 
      0,   0,  13,   0,   0,   0, 
    166,   1,   0,   0,   2,   0, 
      0,   0,   5,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   2,   0,   0,   0, 
      1,   0,   0,   0,  13,   0, 
      0,   0, 173,   1,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   3,   0, 
      0,   0,   1,   0,   0,   0, 
     13,   0,   0,   0, 180,   1, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
    115,  97, 109, 112, 108, 101, 
    114,  95, 116, 101, 120,  91, 
     48,  93,   0, 115,  97, 109, 
    112, 108, 101, 114,  95, 116, 
    101, 120,  91,  49,  93,   0, 
    115,  97, 109, 112, 108, 101, 
    114,  95, 116, 101, 120,  91, 
     50,  93,   0, 115,  97, 109, 
    112, 108, 101, 114,  95, 116, 
    101, 120,  91,  51,  93,   0, 
    116, 101, 120,  91,  48,  93, 
      0, 116, 101, 120,  91,  49, 
     93,   0, 116, 101, 120,  91, 
     50,  93,   0, 116, 101, 120, 
     91,  51,  93,   0,  67, 111, 
    110, 115, 116,  97, 110, 116, 
    115,   0, 171, 171, 180,   1, 
      0,   0,   2,   0,   0,   0, 
    216,   1,   0,   0,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  40,   2, 
      0,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,   2,   0, 
      0,   0,  56,   2,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,  92,   2,   0,   0, 
     16,   0,   0,   0,   8,   0, 
      0,   0,   2,   0,   0,   0, 
    112,   2,   0,   0,   0,   0, 
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
     50,   2,   0,   0, 116, 101, 
    120, 116, 117, 114, 101,  68, 
    105, 109, 115,   0, 102, 108, 
    111,  97, 116,  50,   0, 171, 
      1,   0,   3,   0,   1,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0, 104,   2,   0,   0, 
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
     83,  72,  69,  88,  20,   9, 
      0,   0,  80,   0,   0,   0, 
     69,   2,   0,   0,  53,  16, 
      0,   0,   5,   0,   0,   0, 
     76,   1,   0,   0,  80,   0, 
      0,   0,  71,   1,   0,   0, 
    106,   8,   0,   1,  89,   0, 
      0,   4,  70, 142,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,  90,   0,   0,   3, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  90,   0,   0,   3, 
      0,  96,  16,   0,   1,   0, 
      0,   0,  90,   0,   0,   3, 
      0,  96,  16,   0,   2,   0, 
      0,   0,  90,   0,   0,   3, 
      0,  96,  16,   0,   3,   0, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   0,   0, 
      0,   0,  85,  85,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   1,   0,   0,   0, 
     85,  85,   0,   0,  88,  24, 
      0,   4,   0, 112,  16,   0, 
      2,   0,   0,   0,  85,  85, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   3,   0, 
      0,   0,  85,  85,   0,   0, 
     98,  16,   0,   3, 130,  16, 
     16,   0,   0,   0,   0,   0, 
     98,  16,   0,   3,  50,  16, 
     16,   0,   1,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    104,   0,   0,   2,   3,   0, 
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
     69,   0,   0, 139, 194,   0, 
      0, 128,  67,  85,  21,   0, 
     18,   0,  16,   0,   1,   0, 
      0,   0,  70,  16,  16,   0, 
      1,   0,   0,   0,  70, 126, 
     16,   0,   2,   0,   0,   0, 
      0,  96,  16,   0,   2,   0, 
      0,   0,   0,   0,   0,   7, 
     18,   0,  16,   0,   1,   0, 
      0,   0,  10,   0,  16,   0, 
      1,   0,   0,   0,   1,  64, 
      0,   0, 129, 128,   0, 191, 
     56,   0,   0,  10, 114,   0, 
     16,   0,   1,   0,   0,   0, 
      6,   0,  16,   0,   1,   0, 
      0,   0,   2,  64,   0,   0, 
    186,  73, 204,  63, 197,  32, 
     80, 191,   0,   0,   0,   0, 
      0,   0,   0,   0,  69,   0, 
      0, 139, 194,   0,   0, 128, 
     67,  85,  21,   0, 130,   0, 
     16,   0,   1,   0,   0,   0, 
     70,  16,  16,   0,   1,   0, 
      0,   0, 150, 115,  16,   0, 
      0,   0,   0,   0,   0,  96, 
     16,   0,   0,   0,   0,   0, 
      0,   0,   0,   7, 130,   0, 
     16,   0,   1,   0,   0,   0, 
     58,   0,  16,   0,   1,   0, 
      0,   0,   1,  64,   0,   0, 
    129, 128, 128, 189,  50,   0, 
      0,  12, 114,   0,  16,   0, 
      1,   0,   0,   0, 246,  15, 
     16,   0,   1,   0,   0,   0, 
      2,  64,   0,   0, 244, 253, 
    148,  63, 244, 253, 148,  63, 
    244, 253, 148,  63,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      1,   0,   0,   0,  69,   0, 
      0, 139, 194,   0,   0, 128, 
     67,  85,  21,   0, 130,   0, 
     16,   0,   1,   0,   0,   0, 
     70,  16,  16,   0,   1,   0, 
      0,   0, 150, 115,  16,   0, 
      1,   0,   0,   0,   0,  96, 
     16,   0,   1,   0,   0,   0, 
      0,   0,   0,   7, 130,   0, 
     16,   0,   1,   0,   0,   0, 
     58,   0,  16,   0,   1,   0, 
      0,   0,   1,  64,   0,   0, 
    129, 128,   0, 191,  50,   0, 
      0,  12, 114,   0,  16,   0, 
      1,   0,   0,   0, 246,  15, 
     16,   0,   1,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,  57, 180, 200, 190, 
    135,  22,   1,  64,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      1,   0,   0,   0,  69,   0, 
      0, 139, 194,   0,   0, 128, 
     67,  85,  21,   0,  18,   0, 
     16,   0,   2,   0,   0,   0, 
     70,  16,  16,   0,   1,   0, 
      0,   0,  70, 126,  16,   0, 
      3,   0,   0,   0,   0,  96, 
     16,   0,   3,   0,   0,   0, 
     56,   0,   0,   7, 130,   0, 
     16,   0,   1,   0,   0,   0, 
     10,   0,  16,   0,   2,   0, 
      0,   0,  58,  16,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   5, 130,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     50,   0,   0,  12, 242,  32, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   1,   0, 
      0,   0,   2,  64,   0,   0, 
    111,  18, 131,  58, 111,  18, 
    131,  58, 111,  18, 131,  58, 
    111,  18, 131,  58,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     62,   0,   0,   1,  53,  16, 
      0,   0, 249,   0,   0,   0, 
     21,   0,   1,   0,  41,  96, 
     12,   0, 120,   1, 237,  86, 
     79, 104,  28,  85,  24, 255, 
    222, 123,  51, 179,  59, 111, 
    199, 201, 176, 217, 172, 209, 
     46,  18, 104, 197,   8,  49, 
     52,  54,   8, 182, 133, 164, 
    146,  10, 234,  65,  77,  27, 
    197,  58, 148, 108, 154,  77, 
     83, 220,  38, 117,  55, 200, 
     42, 178, 221, 180, 165, 138, 
    205, 193, 131,   7, 193,  30, 
     60, 139,  39, 201,  65,   4, 
    119, 151, 208, 194,  42,  17, 
    202, 146, 131, 215,  94, 196, 
     99,  16, 209,  30, 106, 198, 
    223,  55, 127, 210,  61, 212, 
     91,  64, 193, 252, 216, 223, 
    251, 230,  55, 223, 247, 222, 
    247, 205, 188, 221, 125, 223, 
    172,  67,  33, 222,  44, 169, 
     47, 216,  78, 223, 159, 221, 
     44, 152,  68, 248, 144,   0, 
    217,  50, 106, 105,  30, 137, 
    142, 103, 120,  36,  58, 196, 
      3,   7,  72,  34,  23, 230, 
     57, 240,  16, 174,  39,  97, 
    135, 249,  58, 182,  17,  38, 
    219,  55,  48, 178, 230, 116, 
    127,  32, 238, 117, 216, 105, 
     48,  15,  30,   3, 135,  64, 
     94, 142, 211,  24,  96,   2, 
     94, 155,  17, 167, 223,   5, 
    215, 245,  20, 104, 131,  12, 
     11, 212,  32, 227,  17,  16, 
     41,  30, 224,  73,  30,  30, 
    142,  69,  48, 201, 241,  48, 
    112, 109,  92,  15, 231, 231, 
    250, 184,  78,   6,  63, 239, 
    191,  13, 174, 135,  72, 241, 
     64, 125,  32, 235, 100, 159, 
     24, 188,  31, 189, 224, 231, 
    140, 230,  16, 221, 140,  95, 
     86, 242, 254, 190, 142,  53, 
    199, 240, 179, 181, 193, 127, 
     66,   0, 140, 243,  69,  15, 
     18, 205,  41, 173, 120, 183, 
     44, 209, 191, 251, 190, 254, 
     79,  72, 190, 191, 252,  46, 
    152,  30, 200, 251, 147, 128, 
    253, 215,  64, 162,  70, 139, 
    249, 233,  32, 221,  99, 149, 
    104,   1,  29, 237,  75, 164, 
     21, 235, 112, 227, 122,  52, 
    148, 173, 228,  89,  41, 232, 
    108,  95, 129, 238, 177,  27, 
    218, 135, 246, 123, 244,  25, 
    232,  51,  61, 122,  22, 122, 
    182,  71, 207,  65, 207,  37, 
     58,  89, 191,  47, 204,  39, 
    195, 125, 198, 205, 250, 127, 
    153, 251,  53, 238,  13, 247, 
    107, 220,  27, 238, 215, 184, 
    103,  12,  81,  55, 214, 155, 
    117, 189, 211, 148, 234, 126, 
    211,  19,  27, 237,  97, 113, 
    189,  77, 198, 112, 135, 204, 
    145, 142,  48, 158, 238,   8, 
    243, 153, 206, 101,  90, 107, 
    145, 104, 108, 107, 171, 136, 
      3, 174, 211,  78, 139,  59, 
    237,  65, 177, 213,  30,  18, 
    155, 237,  17, 241, 121, 123, 
    215, 159, 186,   5,  63, 108, 
    250, 215, 200,  26, 159, 136, 
    195, 162,  17, 250,  13, 241, 
    194,  70, 104, 115, 218,  53, 
    115, 134, 107, 228, 179, 174, 
    153, 183, 220, 153, 198, 214, 
    207, 232,  53, 244,  87, 176, 
    248,  47, 207,  80, 164,  77, 
    182, 208, 242, 178,  88, 107, 
    233, 140,  78, 243, 220, 140, 
    105, 164,  57,  30, 255, 205, 
     97,  60,  78, 230,  48,  30, 
    218, 102,  11, 173, 194, 248, 
    254, 108,  20, 159, 178, 210, 
     56, 144,  55, 204,  76, 206, 
    149, 102, 222, 181, 251, 115, 
    142,  74, 229,  29, 253, 104, 
    174, 160,  95, 205, 213,  53, 
     53, 186, 250, 238,  79,  74, 
     83, 161, 171, 133, 160, 112, 
     14,  89,  93, 250,  69,  16, 
     98,  30, 215, 175,  32, 102, 
     38,  87,  55,   7, 180,  39, 
      7,  12, 207,  30, 200, 122, 
    106, 192, 242, 164, 105, 184, 
    102,  70, 187,  42, 101,  56, 
    118, 191, 118, 100, 198,  40, 
    160,  77, 232, 200, 243,  70, 
    189, 218, 183, 214,  10,  28, 
    203,  90, 109,  80,  75,  98, 
    125, 121, 247, 135, 131,  66, 
    137,  78,  96, 105, 247, 246, 
    208, 107, 173,  10, 251,  93, 
      7, 254, 198, 247, 129, 106, 
     52, 127, 255, 235, 179,   9, 
    137, 252,  18, 249,   3, 181, 
    218, 252, 109, 123, 123,  34, 
    176,  44, 247, 187, 151,  54, 
     39,   2, 199, 117, 217,  79, 
    142, 118, 222, 229, 121, 158, 
     23, 174,  27, 168,  43, 205, 
    231, 215,  59, 205,  64,  93, 
    109, 126, 148,  23, 147,  34, 
    237, 118,  37,  30, 222,  78, 
     35, 159, 186, 115, 240,  18, 
     98,  13,  65, 174,  12, 245, 
    150,  34, 199, 114, 164, 167, 
     29, 229, 185,  78,  32, 211, 
    245, 229, 236, 213, 163, 202, 
    206,  58, 166,  45, 157, 148, 
    109, 160, 101, 108, 116, 201, 
    254,  81, 141,  27,  59,  77, 
    156,  89, 209, 153, 151,  75, 
    206, 172, 213, 214,  99,  24, 
    163, 235, 232, 188, 237,   5, 
     58, 145, 208, 197,   3, 119, 
     37, 137, 230,  56,  62, 151, 
     19, 141, 242, 104,  16, 100, 
     29, 222,   0, 184, 119, 193, 
     89,  29, 106,  38, 247, 180, 
    137, 230, 249, 220, 207,  38, 
    154, 231, 143, 128, 172, 217, 
    199,  56,  12, 178, 158, 143, 
    123, 230, 211, 177, 229, 188, 
    184, 143,  94, 139, 103,  62, 
    192, 181, 216, 255,  97, 108, 
     81,  11, 190,  64,  68, 127, 
    126, 188, 243, 246, 203, 183, 
    143, 157, 252,  50, 190, 127, 
     51, 182, 188,   6, 183, 109, 
    223,  92, 127,  98, 253,   0, 
     55, 214, 192, 183, 177, 239, 
     28, 200, 243,  23, 142, 250, 
    115, 197, 114, 241, 131, 234, 
    232, 202, 114, 101, 249, 157, 
    209, 234,  74, 165,  84, 188, 
    248, 172,  63,  95, 122, 207, 
     47, 213,  86,  74, 149, 165, 
     98, 217,  63, 191,  80,  27, 
    247,  79,  85, 206, 249, 211, 
    165, 165, 249,  82, 197, 159, 
     58,  50,  53,  86, 243,  79, 
     45,  22,  33, 170, 254, 139, 
    111, 205, 188, 113, 226, 228, 
    137, 242, 165, 197, 226, 233, 
     82, 109, 170, 180,  84, 189, 
    176, 242, 254, 232,  66, 121, 
    108, 108, 116, 177,  92,  45, 
     35,  73, 132,  35, 113,   9, 
    248, 121,  80,  10, 188,  88, 
    188, 176,  68, 127,   3,  81, 
    145, 158, 233,   0,  83,  84, 
     65,  84, 148,   0,   0,   0, 
     36,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,  29,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   2,   0, 
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
      0,   0,   0,   0
};
