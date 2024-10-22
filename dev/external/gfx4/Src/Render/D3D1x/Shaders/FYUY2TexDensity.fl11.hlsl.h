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
// TEXCOORD                 0   xy          0     NONE   float   xy  
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
// First Precompiled Shader at offset:[255]
// Embedded Data:
//  0x000000ff - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x000000fa - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_constantbuffer cb0[2], immediateIndexed
dcl_sampler s0, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_input_ps linear v0.xy
dcl_output o0.xyzw
dcl_temps 3
mul r0.xyzw, v0.xyxy, cb0[1].xxyy
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
sample_indexable(texture2d)(float,float,float,float) r1.xyz, v0.xyxx, t0.xyzw, s0
add r1.xyz, r1.xyzx, l(-0.501961, -0.000000, -0.501961, 0.000000)
mad r0.w, -r1.x, l(0.186593), r1.y
mad_sat r2.y, -r1.z, l(0.466296), r0.w
mad_sat r2.xz, r1.zzxz, l(1.568648, 0.000000, 1.848352, 0.000000), r1.yyyy
mad o0.xyz, r2.xyzx, l(0.001000, 0.001000, 0.001000, 0.000000), r0.xyzx
mov o0.w, l(1.001000)
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[255], bundle is:[211] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FYUY2TexDensity.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask  3, param0, paramSlot 0, DefaultVal={0,0,0,0}

codeLenInByte        = 468;Bytes

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
NumVgprs             = 11;
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
; SPI_PS_IN_CONTROL       = 0x00000001
SPIC:NUM_INTERP             = 1
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
  v_interp_p1_f32  v7, v0, attr0.x                      // 000000000014: C81C0000
  v_interp_p1_f32  v8, v0, attr0.y                      // 000000000018: C8200100
  v_interp_p2_f32  v7, v1, attr0.x                      // 00000000001C: C81D0001
  v_interp_p2_f32  v8, v1, attr0.y                      // 000000000020: C8210101
  s_waitcnt     lgkmcnt(0)                              // 000000000024: BF8C007F
  image_sample  v[3:6], v[7:8], s[4:11], s[12:15] dmask:0x7 // 000000000028: F0800700 00610307
  s_load_dwordx4  s[0:3], s[0:1], 0x0c                  // 000000000030: C080010C
  s_waitcnt     lgkmcnt(0)                              // 000000000034: BF8C007F
  s_buffer_load_dwordx2  s[4:5], s[0:3], 0x04           // 000000000038: C2420104
  s_waitcnt     lgkmcnt(0)                              // 00000000003C: BF8C007F
  v_mul_legacy_f32  v1, s4, v7                          // 000000000040: 0E020E04
  v_mul_legacy_f32  v2, s5, v7                          // 000000000044: 0E040E05
  v_mul_legacy_f32  v6, s4, v8                          // 000000000048: 0E0C1004
  v_mul_legacy_f32  v0, s5, v8                          // 00000000004C: 0E001005
  ds_swizzle_b32  v7, v1 offset:32853                   // 000000000050: D8D48055 07000001
  ds_swizzle_b32  v8, v2 offset:32938                   // 000000000058: D8D480AA 08000002
  ds_swizzle_b32  v1, v1 offset:32768                   // 000000000060: D8D48000 01000001
  ds_swizzle_b32  v2, v2 offset:32768                   // 000000000068: D8D48000 02000002
  s_waitcnt     lgkmcnt(1)                              // 000000000070: BF8C017F
  v_sub_f32     v1, v7, v1                              // 000000000074: 08020307
  s_waitcnt     lgkmcnt(0)                              // 000000000078: BF8C007F
  v_sub_f32     v2, v8, v2                              // 00000000007C: 08040508
  ds_swizzle_b32  v7, v6 offset:32853                   // 000000000080: D8D48055 07000006
  ds_swizzle_b32  v8, v0 offset:32938                   // 000000000088: D8D480AA 08000000
  ds_swizzle_b32  v6, v6 offset:32768                   // 000000000090: D8D48000 06000006
  ds_swizzle_b32  v0, v0 offset:32768                   // 000000000098: D8D48000 00000000
  s_waitcnt     lgkmcnt(1)                              // 0000000000A0: BF8C017F
  v_sub_f32     v6, v7, v6                              // 0000000000A4: 080C0D07
  s_waitcnt     lgkmcnt(0)                              // 0000000000A8: BF8C007F
  v_sub_f32     v0, v8, v0                              // 0000000000AC: 08000108
  s_buffer_load_dword  s0, s[0:3], 0x00                 // 0000000000B0: C2000100
  v_mul_legacy_f32  v7, v1, v1                          // 0000000000B4: 0E0E0301
  v_mul_legacy_f32  v8, v2, v2                          // 0000000000B8: 0E100502
  v_mac_legacy_f32  v7, v6, v6                          // 0000000000BC: 0C0E0D06
  v_mac_legacy_f32  v8, v0, v0                          // 0000000000C0: 0C100100
  v_max_legacy_f32  v7, v7, v8                          // 0000000000C4: 1C0E1107
  v_log_f32     v7, v7                                  // 0000000000C8: 7E0E4F07
  v_mad_legacy_f32  v7, v7, 0.5, -1.0                   // 0000000000CC: D2800007 03CDE107
  v_max_legacy_f32  v7, v7, 0                           // 0000000000D4: D21C0007 00010107
  s_waitcnt     lgkmcnt(0)                              // 0000000000DC: BF8C007F
  v_add_f32     v8, s0, -1.0                            // 0000000000E0: D2060008 0001E600
  v_min_legacy_f32  v7, v7, v8                          // 0000000000E8: 1A0E1107
  v_exp_f32     v7, v7                                  // 0000000000EC: 7E0E4B07
  v_rcp_f32     v7, v7                                  // 0000000000F0: 7E0E5507
  v_mul_f32     v1, v1, v7                              // 0000000000F4: 10020F01
  v_mul_f32     v2, v2, v7                              // 0000000000F8: 10040F02
  v_mul_f32     v6, v6, v7                              // 0000000000FC: 100C0F06
  v_mul_f32     v0, v0, v7                              // 000000000100: 10000F00
  v_mul_legacy_f32  v2, v2, v2                          // 000000000104: 0E040502
  v_mul_legacy_f32  v1, v1, v1                          // 000000000108: 0E020301
  v_mac_legacy_f32  v2, v0, v0                          // 00000000010C: 0C040100
  v_mac_legacy_f32  v1, v6, v6                          // 000000000110: 0C020D06
  v_max_legacy_f32  v0, v2, v1                          // 000000000114: 1C000302
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000118: BF8C0F70
  v_add_f32     v1, 0xbf008084, v3                      // 00000000011C: 060206FF BF008084
  s_mov_b32     s0, 0x3e3f123c                          // 000000000124: BE8003FF 3E3F123C
  v_sqrt_f32    v0, v0                                  // 00000000012C: 7E006700
  v_mad_legacy_f32  v3, -v1, s0, v4                     // 000000000130: D2800003 24100101
  v_mad_legacy_f32  v0, -v0, 0.5, 1.0                   // 000000000138: D2800000 23C9E100
  v_add_f32     v2, 0xbf008084, v5                      // 000000000140: 06040AFF BF008084
  s_mov_b32     s0, 0x3eeebe59                          // 000000000148: BE8003FF 3EEEBE59
  v_max_legacy_f32  v0, v0, 0                           // 000000000150: D21C0000 00010100
  s_mov_b32     s1, 0x3ff0f0f2                          // 000000000158: BE8103FF 3FF0F0F2
  s_mov_b32     s2, 0x3fc8c975                          // 000000000160: BE8203FF 3FC8C975
  v_mad_legacy_f32  v3, -v2, s0, v3 clamp               // 000000000168: D2800803 240C0102
  s_mov_b32     s3, 0x3fec96cc                          // 000000000170: BE8303FF 3FEC96CC
  v_mul_legacy_f32  v5, v0, s1 clamp                    // 000000000178: D20E0805 00000300
  v_mad_legacy_f32  v6, -v0, s1, 2.0 clamp              // 000000000180: D2800806 23D00300
  v_mad_legacy_f32  v2, v2, s2, v4 clamp                // 000000000188: D2800802 04100502
  v_mad_legacy_f32  v0, v0, s1, -2.0 clamp              // 000000000190: D2800800 03D40300
  v_mad_legacy_f32  v4, v1, s3, v4 clamp                // 000000000198: D2800804 04100701
  s_mov_b32     s0, 0x3a83126f                          // 0000000001A0: BE8003FF 3A83126F
  v_mac_legacy_f32  v6, s0, v2                          // 0000000001A8: 0C0C0400
  v_mac_legacy_f32  v5, s0, v3                          // 0000000001AC: 0C0A0600
  v_mac_legacy_f32  v0, s0, v4                          // 0000000001B0: 0C000800
  v_mov_b32     v1, 0x3f8020c5                          // 0000000001B4: 7E0202FF 3F8020C5
  s_mov_b64     exec, s[16:17]                          // 0000000001BC: BEFE0410
  v_cvt_pkrtz_f16_f32  v2, v6, v5                       // 0000000001C0: 5E040B06
  v_cvt_pkrtz_f16_f32  v0, v0, v1                       // 0000000001C4: 5E000300
  exp           mrt0, v2, v2, v0, v0 compr vm           // 0000000001C8: F800140F 00000002
  s_endpgm                                              // 0000000001D0: BF810000
end


// Approximately 30 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FYUY2TexDensity[] =
{
     68,  88,  66,  67,   7, 116, 
    178,  57, 128, 243,  67,  49, 
    153,  87,  84,  67, 237, 250, 
    168, 194,   1,   0,   0,   0, 
     96,  10,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
      4,   2,   0,   0,  56,   2, 
      0,   0, 108,   2,   0,   0, 
    196,   9,   0,   0,  82,  68, 
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
     73,  83,  71,  78,  44,   0, 
      0,   0,   1,   0,   0,   0, 
      8,   0,   0,   0,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   3,   0,   0,  84,  69, 
     88,  67,  79,  79,  82,  68, 
      0, 171, 171, 171,  79,  83, 
     71,  78,  44,   0,   0,   0, 
      1,   0,   0,   0,   8,   0, 
      0,   0,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,   0, 
      0,   0,  83,  86,  95,  84, 
     97, 114, 103, 101, 116,   0, 
    171, 171,  83,  72,  69,  88, 
     80,   7,   0,   0,  80,   0, 
      0,   0, 212,   1,   0,   0, 
     53,  16,   0,   0,   5,   0, 
      0,   0, 255,   0,   0,   0, 
     80,   0,   0,   0, 250,   0, 
      0,   0, 106,   8,   0,   1, 
     89,   0,   0,   4,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,  90,   0, 
      0,   3,   0,  96,  16,   0, 
      0,   0,   0,   0,  88,  24, 
      0,   4,   0, 112,  16,   0, 
      0,   0,   0,   0,  85,  85, 
      0,   0,  98,  16,   0,   3, 
     50,  16,  16,   0,   0,   0, 
      0,   0, 101,   0,   0,   3, 
    242,  32,  16,   0,   0,   0, 
      0,   0, 104,   0,   0,   2, 
      3,   0,   0,   0,  56,   0, 
      0,   8, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  20, 
     16,   0,   0,   0,   0,   0, 
      6, 133,  32,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
    122,   0,   0,   5,  50,   0, 
     16,   0,   0,   0,   0,   0, 
     70,   0,  16,   0,   0,   0, 
      0,   0, 124,   0,   0,   5, 
    194,   0,  16,   0,   0,   0, 
      0,   0, 166,  14,  16,   0, 
      0,   0,   0,   0,  15,   0, 
      0,   7,  18,   0,  16,   0, 
      1,   0,   0,   0,  70,   0, 
     16,   0,   0,   0,   0,   0, 
     70,   0,  16,   0,   0,   0, 
      0,   0,  15,   0,   0,   7, 
     34,   0,  16,   0,   1,   0, 
      0,   0, 230,  10,  16,   0, 
      0,   0,   0,   0, 230,  10, 
     16,   0,   0,   0,   0,   0, 
     52,   0,   0,   7,  18,   0, 
     16,   0,   1,   0,   0,   0, 
     26,   0,  16,   0,   1,   0, 
      0,   0,  10,   0,  16,   0, 
      1,   0,   0,   0,  47,   0, 
      0,   5,  18,   0,  16,   0, 
      1,   0,   0,   0,  10,   0, 
     16,   0,   1,   0,   0,   0, 
     50,   0,   0,   9,  18,   0, 
     16,   0,   1,   0,   0,   0, 
     10,   0,  16,   0,   1,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0,  63,   1,  64, 
      0,   0,   0,   0, 128, 191, 
     52,   0,   0,   7,  18,   0, 
     16,   0,   1,   0,   0,   0, 
     10,   0,  16,   0,   1,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   8,  34,   0,  16,   0, 
      1,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128, 191, 
     10, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     51,   0,   0,   7,  18,   0, 
     16,   0,   1,   0,   0,   0, 
     26,   0,  16,   0,   1,   0, 
      0,   0,  10,   0,  16,   0, 
      1,   0,   0,   0,  25,   0, 
      0,   5,  18,   0,  16,   0, 
      1,   0,   0,   0,  10,   0, 
     16,   0,   1,   0,   0,   0, 
     14,   0,   0,   7, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,   6,   0,  16,   0, 
      1,   0,   0,   0,  15,   0, 
      0,   7,  66,   0,  16,   0, 
      0,   0,   0,   0, 230,  10, 
     16,   0,   0,   0,   0,   0, 
    230,  10,  16,   0,   0,   0, 
      0,   0,  15,   0,   0,   7, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  70,   0,  16,   0, 
      0,   0,   0,   0,  70,   0, 
     16,   0,   0,   0,   0,   0, 
     52,   0,   0,   7,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     42,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  75,   0, 
      0,   5,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
     50,   0,   0,  10,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16, 128,  65,   0, 
      0,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,  63,   1,  64,   0,   0, 
      0,   0, 128,  63,  52,   0, 
      0,   7,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,  56,   0,   0,   7, 
     34,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0, 242, 240, 240,  63, 
     50,   0,   0,  15,  82,   0, 
     16,   0,   0,   0,   0,   0, 
     86,   5,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
      0,   0, 128, 191,   0,   0, 
      0,   0,   0,   0, 128,  63, 
      0,   0,   0,   0,   2,  64, 
      0,   0,   0,   0,   0,  64, 
      0,   0,   0,   0,   0,   0, 
      0, 192,   0,   0,   0,   0, 
     54,  32,   0,   5, 114,   0, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,  69,   0,   0, 139, 
    194,   0,   0, 128,  67,  85, 
     21,   0, 114,   0,  16,   0, 
      1,   0,   0,   0,  70,  16, 
     16,   0,   0,   0,   0,   0, 
     70, 126,  16,   0,   0,   0, 
      0,   0,   0,  96,  16,   0, 
      0,   0,   0,   0,   0,   0, 
      0,  10, 114,   0,  16,   0, 
      1,   0,   0,   0,  70,   2, 
     16,   0,   1,   0,   0,   0, 
      2,  64,   0,   0, 132, 128, 
      0, 191,   0,   0,   0, 128, 
    132, 128,   0, 191,   0,   0, 
      0,   0,  50,   0,   0,  10, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16, 128, 
     65,   0,   0,   0,   1,   0, 
      0,   0,   1,  64,   0,   0, 
     60,  18,  63,  62,  26,   0, 
     16,   0,   1,   0,   0,   0, 
     50,  32,   0,  10,  34,   0, 
     16,   0,   2,   0,   0,   0, 
     42,   0,  16, 128,  65,   0, 
      0,   0,   1,   0,   0,   0, 
      1,  64,   0,   0,  89, 190, 
    238,  62,  58,   0,  16,   0, 
      0,   0,   0,   0,  50,  32, 
      0,  12,  82,   0,  16,   0, 
      2,   0,   0,   0, 166,   8, 
     16,   0,   1,   0,   0,   0, 
      2,  64,   0,   0, 117, 201, 
    200,  63,   0,   0,   0,   0, 
    204, 150, 236,  63,   0,   0, 
      0,   0,  86,   5,  16,   0, 
      1,   0,   0,   0,  50,   0, 
      0,  12, 114,  32,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   2,   0,   0,   0, 
      2,  64,   0,   0, 111,  18, 
    131,  58, 111,  18, 131,  58, 
    111,  18, 131,  58,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   5, 130,  32,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0, 197,  32, 128,  63, 
     62,   0,   0,   1,  53,  16, 
      0,   0, 213,   0,   0,   0, 
     21,   0,   1,   0,  41, 170, 
     11,   0, 120,   1, 237,  86, 
     61, 104,  20,  65,  20, 126, 
     51, 179, 127, 196, 205, 220, 
     26,  68,  84,  78,  49, 168, 
     96,  21,  72,  98,  33,  34, 
    158,  72, 208,  66,  69,  80, 
     35,  68,  86, 241, 162,  27, 
     13, 158, 137, 220,  69,  57, 
      5, 207, 245,   7, 109,  82, 
     88, 105,  99, 169,  32, 218, 
     88,  88, 239, 133, 128, 112, 
     66,   4,  61,  78, 176, 181, 
     17,  11, 149,  96,  37,  40, 
    158, 223, 219, 159, 120, 133, 
    118,   1,   5, 243, 193,  55, 
    111, 191, 153,  55, 111, 222, 
    206,  44, 251, 230, 241,  50, 
    138, 177, 181, 117, 123, 142, 
    109, 239, 158,  79, 175,  63, 
     40,  34,  19, 207,   2, 100, 
    203, 168,  58, 220,  18,  77, 
    119, 113,  75, 180, 153,  27, 
    118, 144,  68,  28, 162,   7, 
    108,  65, 239, 132, 229, 177, 
    140,   9, 118, 206, 132, 104, 
     61, 144, 125, 199,  48, 103, 
     63,  44, 251, 174,   4,   7, 
    193, 213,  32, 135, 227, 101, 
    216, 102,  72, 151,  91, 176, 
     25,  56, 175, 117, 160,  11, 
     50,  12,  48, 203, 149, 125, 
     59,  99, 208,  38, 110, 126, 
    143,  35,  32, 231, 244,  39, 
    112, 110,  28,  43, 203, 139, 
    243, 100, 240, 251, 254, 109, 
    112,  62, 157,  45,  51,  59, 
     39,   6, 159,  71,  39,  52, 
    152, 117, 237,  75,  55,  20, 
     71,  29, 195,  79,  53, 239, 
      5, 191, 219,  12, 248,  39, 
    180,   1,  62, 203,  78, 100, 
    154, 227,  91,  11,  81, 255, 
     79, 240, 183, 200, 224, 189, 
    192, 167,  30, 239,  77,  14, 
    204, 192, 227, 231,  65, 162, 
    176, 206, 188, 179, 138, 190, 
    178, 202, 180, 128, 182,  88, 
    166,  90, 177, 142,  63, 238, 
     14, 141, 224, 214,  50, 227, 
     56, 182, 250, 120,  46,  79, 
     95, 121,  29, 104,  31, 218, 
    239, 208,  71, 161, 143, 118, 
    232,  19, 208,  39,  58, 244, 
     40, 244, 104, 166, 179, 248, 
    185, 100,  61, 193, 231, 140, 
    206, 218, 191, 204, 165,  28, 
     23, 135,  75,  57,  46,  14, 
    151, 114,  92,  52, 198, 168, 
     25, 119, 162,  90, 215, 143, 
     72, 170, 239,  17, 137, 217, 
     25,  71, 220,  66, 105, 202, 
     55,  72, 172, 111,   8,  90, 
    219,  16, 162, 183, 113, 149, 
    166, 235, 100, 135, 243, 182, 
     42, 146,  43, 194,  25, 214, 
    134, 216,  53,  27,  91,  45, 
    181, 169,  13, 109, 120, 174, 
     54,  61, 210, 195,  97, 235, 
     45, 126, 157, 246,  99,  88, 
    252, 243,  28,  74, 180,  96, 
     11,  45, 175, 138, 233, 186, 
    173, 164, 195, 115,  29, 211, 
    112, 216,  31, 255, 226, 216, 
     31, 233, 196, 254, 208,  22, 
     91, 104, 138, 253, 187, 221, 
    196,  95,  96,  92, 208, 172, 
     80,  90,  75, 211, 211,  86, 
    183, 118,  73, 120, 174, 189, 
     92, 231, 237,   3, 186, 102, 
     83, 216, 180, 223, 189,  84, 
     54, 229, 155, 182,  16,  20, 
    207,  33, 171,  73, 239,   5, 
    193, 103, 141, 189,  23,  62, 
    195, 186,  38, 114, 210, 147, 
     57, 195, 179, 114, 174, 135, 
    194, 225,  73, 211, 208,  66, 
     73,  77, 194, 112, 173, 110, 
    233,  74,  69, 249, 243, 185, 
    233, 122, 219, 146, 214, 205, 
    144, 234, 109,  21,  70, 219, 
    123,  10,  59, 232,  52, 213, 
     20, 214,  16, 194, 219, 136, 
    255, 120, 147, 222, 189, 216, 
    208, 238,  50,  22, 124,  70, 
    162, 207,  59, 176, 119,  77, 
    194, 218, 109, 117,  45, 250, 
     50,  63,  95, 104, 171, 235, 
    209, 133,  23, 141, 130, 114, 
    194, 166,  20, 238, 198, 182, 
    186,  17, 205, 221, 253,  88, 
     48,  29, 221,  68, 109,  32, 
     11, 253, 164,  94, 109, 144, 
     60, 110, 122,   6, 197, 186, 
    165,  12,  88,  97, 123,   6, 
    199, 157, 236, 185, 177, 141, 
     12, 215,  37, 171, 203, 197, 
     14, 184, 109,  41, 107, 207, 
    215, 135,   5, 207, 248,  17, 
    161, 198,  36,  53, 106,  69, 
     86,  99, 174, 213, 179, 187, 
     30,  35, 233, 251,   5,  92, 
     95, 144,  93,   2, 190, 202, 
    160,  86, 138, 204, 135, 239, 
    122, 172, 159, 165, 119, 149, 
      7, 169, 229, 250, 138, 126, 
    220, 111, 178, 153,   9,  90, 
    233, 248,  92, 106,  57,  30, 
    223, 111, 158, 126, 187, 255, 
    230, 201, 253, 123, 143, 230, 
    211, 254, 143, 169, 229,  24, 
    252,  56, 126, 229,  97, 111, 
    192, 142, 128, 147, 218,  99, 
     32, 207,  31, 219, 230, 143, 
     22,  75, 197, 203, 149, 190, 
    169, 201, 242, 228, 217, 190, 
    202,  84,  57,  40, 158,  27, 
    240,  79,   5,  23, 253, 160, 
     58,  21, 148,  39, 138,  37, 
    255, 244,  88, 117, 139, 127, 
    168, 124, 210,  63,  24,  76, 
    156,  10, 202, 254, 208, 224, 
     80, 127, 213,  63, 116, 166, 
      8,  81, 241, 119, 143,  12, 
    143,  12,  28,  14, 170,  67, 
    193,  68, 101, 124, 234,  82, 
    223,  88, 169, 191, 191, 239, 
     76, 169,  82,  66, 248,   4, 
     87, 210,  69,  77, 208,   6, 
    207,  21, 199,  39, 232,  39, 
     45, 240, 130, 124,  83,  84, 
     65,  84, 148,   0,   0,   0, 
     30,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,  26,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
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
