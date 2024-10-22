#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler For Durango 9.30.12098.0
//
//
///
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
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// TEXCOORD                 0   xyzw        0     NONE   float   xyzw
// TEXCOORD                 1   xyzw        1     NONE   float   xyzw
// TEXCOORD                 2   xy          2     NONE   float   xy  
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
// First Precompiled Shader at offset:[183]
// Embedded Data:
//  0x000000b7 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x000000b2 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_sampler s2, mode_default
dcl_sampler s3, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_resource_texture2d (float,float,float,float) t2
dcl_resource_texture2d (float,float,float,float) t3
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.xyzw
dcl_input_ps linear v2.xy
dcl_output o0.xyzw
dcl_temps 2
sample_indexable(texture2d)(float,float,float,float) r0.x, v2.xyxx, t2.xyzw, s2
add r0.x, r0.x, l(-0.501961)
mul r0.xyz, r0.xxxx, l(1.596000, -0.813000, 0.000000, 0.000000)
sample_indexable(texture2d)(float,float,float,float) r0.w, v2.xyxx, t0.yzwx, s0
add r0.w, r0.w, l(-0.062745)
mad r0.xyz, r0.wwww, l(1.164000, 1.164000, 1.164000, 0.000000), r0.xyzx
sample_indexable(texture2d)(float,float,float,float) r0.w, v2.xyxx, t1.yzwx, s1
add r0.w, r0.w, l(-0.501961)
mad r0.xyz, r0.wwww, l(0.000000, -0.392000, 2.017000, 0.000000), r0.xyzx
sample_indexable(texture2d)(float,float,float,float) r0.w, v2.xyxx, t3.yzwx, s3
mov r1.xyz, v1.xyzx
mov r1.w, l(1.000000)
mul r0.xyzw, r0.xyzw, r1.xyzw
mul r0.xyzw, r0.xyzw, v1.wwww
mad o0.xyzw, v0.xyzw, r0.wwww, r0.xyzw
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[183], bundle is:[203] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FYUVACxformAc.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask 15, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask  3, param2, paramSlot 2, DefaultVal={0,0,0,0}

codeLenInByte        = 336;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 8;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_RESOURCE, 1, offset 8:15 dwords
;  extUserElements 1[2] = IMM_RESOURCE, 2, offset 16:23 dwords
;  extUserElements 1[3] = IMM_RESOURCE, 3, offset 24:31 dwords
;  extUserElements 1[4] = IMM_SAMPLER, 0, offset 32:35 dwords
;  extUserElements 1[5] = IMM_SAMPLER, 1, offset 36:39 dwords
;  extUserElements 1[6] = IMM_SAMPLER, 2, offset 40:43 dwords
;  extUserElements 1[7] = IMM_SAMPLER, 3, offset 44:47 dwords
NumVgprs             = 11;
NumSgprs             = 42;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x0000000F
; constBufUsage           = 0x00000000

; SPI_SHADER_PGM_RSRC2_PS = 0x00000004
SSPRP:SCRATCH_EN            = 0
SSPRP:USER_SGPR             = 2
SSPRP:TRAP_PRESENT          = 0
SSPRP:WAVE_CNT_EN           = 0
SSPRP:EXTRA_LDS_SIZE        = 0
SSPRP:EXCP_EN               = 0
; SPI_SHADER_Z_FORMAT     = 0x00000000
SPZF:Z_EXPORT_FORMAT        = 0; SPI_SHADER_ZERO
; SPI_PS_IN_CONTROL       = 0x00000003
SPIC:NUM_INTERP             = 3
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
  s_mov_b64     s[40:41], exec                          // 000000000000: BEA8047E
  s_wqm_b64     exec, exec                              // 000000000004: BEFE0A7E
  s_mov_b32     m0, s2                                  // 000000000008: BEFC0302
  s_load_dwordx8  s[4:11], s[0:1], 0x10                 // 00000000000C: C0C20110
  s_load_dwordx4  s[12:15], s[0:1], 0x28                // 000000000010: C0860128
  v_interp_p1_f32  v2, v0, attr2.x                      // 000000000014: C8080800
  v_interp_p1_f32  v3, v0, attr2.y                      // 000000000018: C80C0900
  v_interp_p2_f32  v2, v1, attr2.x                      // 00000000001C: C8090801
  v_interp_p2_f32  v3, v1, attr2.y                      // 000000000020: C80D0901
  s_waitcnt     lgkmcnt(0)                              // 000000000024: BF8C007F
  image_sample  v[4:7], v[2:3], s[4:11], s[12:15]       // 000000000028: F0800100 00610402
  s_load_dwordx8  s[16:23], s[0:1], 0x00                // 000000000030: C0C80100
  s_load_dwordx8  s[32:39], s[0:1], 0x08                // 000000000034: C0D00108
  s_load_dwordx8  s[24:31], s[0:1], 0x20                // 000000000038: C0CC0120
  s_waitcnt     lgkmcnt(0)                              // 00000000003C: BF8C007F
  image_sample  v[5:8], v[2:3], s[16:23], s[24:27]      // 000000000040: F0800100 00C40502
  image_sample  v[6:9], v[2:3], s[32:39], s[28:31]      // 000000000048: F0800100 00E80602
  s_load_dwordx8  s[4:11], s[0:1], 0x18                 // 000000000050: C0C20118
  s_load_dwordx4  s[0:3], s[0:1], 0x2c                  // 000000000054: C080012C
  s_waitcnt     lgkmcnt(0)                              // 000000000058: BF8C007F
  image_sample  v[2:5], v[2:3], s[4:11], s[0:3]         // 00000000005C: F0800100 00010202
  s_waitcnt     vmcnt(3) & lgkmcnt(15)                  // 000000000064: BF8C0F73
  v_add_f32     v3, 0xbf008081, v4                      // 000000000068: 060608FF BF008081
  v_interp_p1_f32  v8, v0, attr1.x                      // 000000000070: C8200400
  v_interp_p1_f32  v9, v0, attr1.y                      // 000000000074: C8240500
  v_interp_p1_f32  v10, v0, attr1.z                     // 000000000078: C8280600
  v_mul_legacy_f32  v4, 0xbf5020c5, v3                  // 00000000007C: 0E0806FF BF5020C5
  s_waitcnt     vmcnt(2) & lgkmcnt(15)                  // 000000000084: BF8C0F72
  v_add_f32     v5, 0xbd808081, v5                      // 000000000088: 060A0AFF BD808081
  s_mov_b32     s0, 0x3f94fdf4                          // 000000000090: BE8003FF 3F94FDF4
  v_mul_legacy_f32  v3, 0x3fcc49ba, v3                  // 000000000098: 0E0606FF 3FCC49BA
  v_mul_legacy_f32  v7, 0x3f94fdf4, v5                  // 0000000000A0: 0E0E0AFF 3F94FDF4
  v_mac_legacy_f32  v4, s0, v5                          // 0000000000A8: 0C080A00
  s_waitcnt     vmcnt(1) & lgkmcnt(15)                  // 0000000000AC: BF8C0F71
  v_add_f32     v6, 0xbf008081, v6                      // 0000000000B0: 060C0CFF BF008081
  s_mov_b32     s1, 0xbec8b439                          // 0000000000B8: BE8103FF BEC8B439
  s_mov_b32     s2, 0x40011687                          // 0000000000C0: BE8203FF 40011687
  v_interp_p2_f32  v8, v1, attr1.x                      // 0000000000C8: C8210401
  v_interp_p2_f32  v9, v1, attr1.y                      // 0000000000CC: C8250501
  v_interp_p2_f32  v10, v1, attr1.z                     // 0000000000D0: C8290601
  v_mac_legacy_f32  v3, s0, v5                          // 0000000000D4: 0C060A00
  v_mac_legacy_f32  v4, s1, v6                          // 0000000000D8: 0C080C01
  v_mac_legacy_f32  v7, s2, v6                          // 0000000000DC: 0C0E0C02
  v_interp_p1_f32  v5, v0, attr1.w                      // 0000000000E0: C8140700
  v_mul_legacy_f32  v3, v3, v8                          // 0000000000E4: 0E061103
  v_mul_legacy_f32  v4, v4, v9                          // 0000000000E8: 0E081304
  v_mul_legacy_f32  v6, v7, v10                         // 0000000000EC: 0E0C1507
  v_interp_p1_f32  v7, v0, attr0.x                      // 0000000000F0: C81C0000
  v_interp_p1_f32  v8, v0, attr0.y                      // 0000000000F4: C8200100
  v_interp_p1_f32  v9, v0, attr0.z                      // 0000000000F8: C8240200
  v_interp_p2_f32  v5, v1, attr1.w                      // 0000000000FC: C8150701
  v_interp_p1_f32  v0, v0, attr0.w                      // 000000000100: C8000300
  v_interp_p2_f32  v7, v1, attr0.x                      // 000000000104: C81D0001
  v_interp_p2_f32  v8, v1, attr0.y                      // 000000000108: C8210101
  v_interp_p2_f32  v9, v1, attr0.z                      // 00000000010C: C8250201
  v_mul_legacy_f32  v3, v3, v5                          // 000000000110: 0E060B03
  v_mul_legacy_f32  v4, v4, v5                          // 000000000114: 0E080B04
  v_mul_legacy_f32  v6, v6, v5                          // 000000000118: 0E0C0B06
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 00000000011C: BF8C0F70
  v_mul_legacy_f32  v2, v2, v5                          // 000000000120: 0E040B02
  v_interp_p2_f32  v0, v1, attr0.w                      // 000000000124: C8010301
  v_mac_legacy_f32  v3, v7, v2                          // 000000000128: 0C060507
  v_mac_legacy_f32  v4, v8, v2                          // 00000000012C: 0C080508
  v_mac_legacy_f32  v6, v9, v2                          // 000000000130: 0C0C0509
  v_mac_legacy_f32  v2, v0, v2                          // 000000000134: 0C040500
  s_mov_b64     exec, s[40:41]                          // 000000000138: BEFE0428
  v_cvt_pkrtz_f16_f32  v0, v3, v4                       // 00000000013C: 5E000903
  v_cvt_pkrtz_f16_f32  v1, v6, v2                       // 000000000140: 5E020506
  exp           mrt0, v0, v0, v1, v1 compr vm           // 000000000144: F800140F 00000100
  s_endpgm                                              // 00000000014C: BF810000
end


// Approximately 16 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FYUVACxformAc[] =
{
     68,  88,  66,  67, 105, 182, 
     11, 203, 171,  97,  19,  33, 
    237,  69, 252, 220, 230, 233, 
     61,  89,   1,   0,   0,   0, 
     88,   9,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
     12,   2,   0,   0, 112,   2, 
      0,   0, 164,   2,   0,   0, 
    188,   8,   0,   0,  82,  68, 
     69,  70, 208,   1,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   8,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
    148,   1,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
     60,   1,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0,  75,   1,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
      1,   0,   0,   0,  90,   1, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
    105,   1,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0, 120,   1,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     13,   0,   0,   0, 127,   1, 
      0,   0,   2,   0,   0,   0, 
      5,   0,   0,   0,   4,   0, 
      0,   0, 255, 255, 255, 255, 
      1,   0,   0,   0,   1,   0, 
      0,   0,  13,   0,   0,   0, 
    134,   1,   0,   0,   2,   0, 
      0,   0,   5,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   2,   0,   0,   0, 
      1,   0,   0,   0,  13,   0, 
      0,   0, 141,   1,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   3,   0, 
      0,   0,   1,   0,   0,   0, 
     13,   0,   0,   0, 115,  97, 
    109, 112, 108, 101, 114,  95, 
    116, 101, 120,  91,  48,  93, 
      0, 115,  97, 109, 112, 108, 
    101, 114,  95, 116, 101, 120, 
     91,  49,  93,   0, 115,  97, 
    109, 112, 108, 101, 114,  95, 
    116, 101, 120,  91,  50,  93, 
      0, 115,  97, 109, 112, 108, 
    101, 114,  95, 116, 101, 120, 
     91,  51,  93,   0, 116, 101, 
    120,  91,  48,  93,   0, 116, 
    101, 120,  91,  49,  93,   0, 
    116, 101, 120,  91,  50,  93, 
      0, 116, 101, 120,  91,  51, 
     93,   0,  77, 105,  99, 114, 
    111, 115, 111, 102, 116,  32, 
     40,  82,  41,  32,  72,  76, 
     83,  76,  32,  83, 104,  97, 
    100, 101, 114,  32,  67, 111, 
    109, 112, 105, 108, 101, 114, 
     32,  70, 111, 114,  32,  68, 
    117, 114,  97, 110, 103, 111, 
     32,  57,  46,  51,  48,  46, 
     49,  50,  48,  57,  56,  46, 
     48,   0,  73,  83,  71,  78, 
     92,   0,   0,   0,   3,   0, 
      0,   0,   8,   0,   0,   0, 
     80,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,  15,   0,   0, 
     80,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,  15,   0,   0, 
     80,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
      0,   0,   3,   3,   0,   0, 
     84,  69,  88,  67,  79,  79, 
     82,  68,   0, 171, 171, 171, 
     79,  83,  71,  78,  44,   0, 
      0,   0,   1,   0,   0,   0, 
      8,   0,   0,   0,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  83,  86, 
     95,  84,  97, 114, 103, 101, 
    116,   0, 171, 171,  83,  72, 
     69,  88,  16,   6,   0,   0, 
     80,   0,   0,   0, 132,   1, 
      0,   0,  53,  16,   0,   0, 
      5,   0,   0,   0, 183,   0, 
      0,   0,  80,   0,   0,   0, 
    178,   0,   0,   0, 106,   8, 
      0,   1,  90,   0,   0,   3, 
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
     98,  16,   0,   3, 242,  16, 
     16,   0,   0,   0,   0,   0, 
     98,  16,   0,   3, 242,  16, 
     16,   0,   1,   0,   0,   0, 
     98,  16,   0,   3,  50,  16, 
     16,   0,   2,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    104,   0,   0,   2,   2,   0, 
      0,   0,  69,   0,   0, 139, 
    194,   0,   0, 128,  67,  85, 
     21,   0,  18,   0,  16,   0, 
      0,   0,   0,   0,  70,  16, 
     16,   0,   2,   0,   0,   0, 
     70, 126,  16,   0,   2,   0, 
      0,   0,   0,  96,  16,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   7,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0, 129, 128, 
      0, 191,  56,   0,   0,  10, 
    114,   0,  16,   0,   0,   0, 
      0,   0,   6,   0,  16,   0, 
      0,   0,   0,   0,   2,  64, 
      0,   0, 186,  73, 204,  63, 
    197,  32,  80, 191,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     69,   0,   0, 139, 194,   0, 
      0, 128,  67,  85,  21,   0, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  70,  16,  16,   0, 
      2,   0,   0,   0, 150, 115, 
     16,   0,   0,   0,   0,   0, 
      0,  96,  16,   0,   0,   0, 
      0,   0,   0,   0,   0,   7, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0, 129, 128, 128, 189, 
     50,   0,   0,  12, 114,   0, 
     16,   0,   0,   0,   0,   0, 
    246,  15,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
    244, 253, 148,  63, 244, 253, 
    148,  63, 244, 253, 148,  63, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     69,   0,   0, 139, 194,   0, 
      0, 128,  67,  85,  21,   0, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  70,  16,  16,   0, 
      2,   0,   0,   0, 150, 115, 
     16,   0,   1,   0,   0,   0, 
      0,  96,  16,   0,   1,   0, 
      0,   0,   0,   0,   0,   7, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0, 129, 128,   0, 191, 
     50,   0,   0,  12, 114,   0, 
     16,   0,   0,   0,   0,   0, 
    246,  15,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
      0,   0,   0,   0,  57, 180, 
    200, 190, 135,  22,   1,  64, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     69,   0,   0, 139, 194,   0, 
      0, 128,  67,  85,  21,   0, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  70,  16,  16,   0, 
      2,   0,   0,   0, 150, 115, 
     16,   0,   3,   0,   0,   0, 
      0,  96,  16,   0,   3,   0, 
      0,   0,  54,   0,   0,   5, 
    114,   0,  16,   0,   1,   0, 
      0,   0,  70,  18,  16,   0, 
      1,   0,   0,   0,  54,   0, 
      0,   5, 130,   0,  16,   0, 
      1,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     56,   0,   0,   7, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      1,   0,   0,   0,  56,   0, 
      0,   7, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
    246,  31,  16,   0,   1,   0, 
      0,   0,  50,   0,   0,   9, 
    242,  32,  16,   0,   0,   0, 
      0,   0,  70,  30,  16,   0, 
      0,   0,   0,   0, 246,  15, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  62,   0,   0,   1, 
     53,  16,   0,   0, 205,   0, 
      0,   0,  21,   0,   1,   0, 
     41, 116,  11,   0, 120,   1, 
    237,  86,  77, 104,  19,  65, 
     20, 158, 217, 191, 108, 146, 
    117,  83, 233, 143,  21,  42, 
     70, 108,  33, 106, 137, 214, 
     10, 162, 151,  54,  88,  43, 
    130,  96, 105,  81, 180,  46, 
    165, 105, 155, 216,  98, 250, 
     99,  18, 106,  80, 168, 169, 
      7,   5, 169, 224, 193, 131, 
     23, 193, 163,   7, 189, 121, 
     16, 209,  77,  16, 133,  57, 
     20, 241, 224, 161,  87, 111, 
     30,  69,  60,   8, 106, 215, 
    247, 109, 118, 108,  16, 123, 
     43,  40, 216,  15, 190, 247, 
    230, 155, 121, 243, 230, 237, 
    206, 178,  51, 197,  40, 243, 
    241, 121, 112,  94, 133, 159, 
    186, 182, 114,  91,  40, 140, 
    233, 212, 230,  68, 120, 160, 
    100, 194,  50, 118,  46,   2, 
    203,  88,   2,   6,   1,  20, 
    139,  20, 123, 137,   3, 164, 
    123, 201,  99,  76, 178, 134, 
    222, 234,  19, 178, 109,  68, 
    196, 190, 165,  57, 167, 200, 
    167, 136, 200, 191, 143, 216, 
     66,  68,  58,  64, 131,   9, 
     16,  44, 203,  66,  48, 191, 
     97,  27,  76, 128,  38, 162, 
    140,  65,  78, 255,  97,  36, 
     58,  96, 254, 140, 126,  34, 
    106,  90,  15, 168,  77, 214, 
    131, 250,  80,  39, 128, 231, 
    253, 219, 168, 189, 175, 218, 
    147, 198,  96,   8, 114, 159, 
      0, 236,  71,  61, 108, 162, 
    236,  18,  97, 216, 181, 247, 
    187,  18, 104, 188,  11,  60, 
     91, 149, 184,  30,  60, 194, 
      1,  52, 234,  32,  53, 170, 
     49, 130, 106,  12, 190, 147, 
    197, 225, 149, 221, 254, 119, 
    241, 191,  64, 126,  47, 120, 
     23, 244, 169, 179,   6, 162, 
    220,  31,   0, 227, 167, 137, 
    140, 149,  43, 224, 221,  86, 
    246,  21,  74, 106,  78,  26, 
    115, 165,  86, 161, 253, 164, 
    117, 154,  18, 171,  97,  54, 
     98, 232, 202,  72, 172, 141, 
    230, 211, 198, 146, 118,  72, 
     59, 117, 122, 152, 244, 112, 
    157,  30,  37,  61,  90, 167, 
    199,  72, 143,  73,  45, 243, 
    199, 252, 245,  12,   5, 251, 
     76, 157,  11, 255,  50,  55, 
    107, 220,  24, 110, 214, 184, 
     49, 220, 172, 113, 195, 232, 
     99,  65, 123, 228,  46,  68, 
     86,  93,  69, 253, 238,  54, 
    240,  87, 213,   4, 191,  89, 
    101, 166,  41,  88, 216,  18, 
    220,  12,  11,  30, 222,  34, 
    174, 179, 165,  10, 227, 229, 
     79, 138, 150, 166,   3,  78, 
     84,  77, 254, 174,  26, 231, 
    203, 213,  95, 253, 250, 107, 
    234,  39, 111, 124, 100, 173, 
    148, 163, 147, 151, 215, 198, 
     20, 206,  10, 177, 165, 138, 
    103,  26, 198,  98, 153,  85, 
    152,  22,  23,  76, 111,  23, 
    204,  72,   8, 207,  48, 237, 
     55, 241, 129,  74,  30, 227, 
    145,   8, 141, 151,  95, 122, 
    106, 217, 253, 242, 227,  94, 
    143, 103,  24, 246, 243, 147, 
    203,  61,  94, 196, 182, 161, 
     89, 196, 180,  46,  35, 206, 
    178, 252,  60, 158, 186, 232, 
     30, 121,  42,  92,  79, 189, 
    225, 222, 106, 225, 189,  92, 
    219,  37, 184, 222,  33, 184, 
    177,  71, 176, 136,  97, 113, 
    203, 180,  20, 203, 182,  88, 
    168,  73, 168,  91,  13,  91, 
    107,  52, 237,  80, 179,  69, 
     71, 116, 155,  96, 156, 106, 
     80, 218,   5,  15,  53,  11, 
     58,  68,   4, 103,  59,   4, 
    231,  52,  95, 233,  16, 106, 
    148,  98, 163, 166, 109,  68, 
     45, 123, 142, 214,  83, 162, 
    154, 205,  85,  46,  66, 186, 
     97, 153, 186, 105, 133, 117, 
    203,  98, 186, 102,  37, 180, 
     85, 151, 254, 239, 181, 243, 
    161,  73, 254, 223,  23,  43, 
    219, 201, 250,  77,   2, 206, 
    166, 122, 208,  93, 192,  31, 
    130, 193, 189,  64, 106, 196, 
    225,  12, 147,  26, 103,  83, 
     43,  17, 218, 239,  32, 224, 
    156, 167,  35, 202, 215,  96, 
     59,  81, 106, 204, 199,  93, 
     80, 106, 204, 239,  36,  66, 
    151, 131, 251, 228,  92, 224, 
    177,  14, 245, 211,  93,   5, 
    145, 107, 120,  24, 140, 223, 
     15,  60, 173, 237, 223,  85, 
     14, 127, 107, 124, 144,  18, 
    199,  31, 191,   8, 250, 159, 
      5,  30,  57, 208, 236, 254, 
    112, 229, 206, 126,   4,  18, 
    222,   7,  99,  23, 136, 152, 
    159,  61, 234, 140, 165, 115, 
    233, 171, 133, 100, 113,  54, 
     63, 123,  41,  89,  40, 230, 
     51, 233, 233, 131, 206,  68, 
    102, 222, 201, 148, 138, 153, 
    252,  76,  58, 231,  92, 204, 
    150,  14,  57,  67, 249, 113, 
    103,  48,  51,  51, 145, 201, 
     59, 125, 221, 125,  93,  37, 
    103, 104,  50,  77, 162, 224, 
    244, 159,  63, 115,  54, 117, 
    172, 148, 157, 205,  79, 167, 
    198, 147, 217,  92,  87,  87, 
    114,  50,  87, 200,  81, 242, 
     26,  78,   4,  75, 227,  74, 
    138, 235, 233, 116, 122, 106, 
    134, 253,   4, 164, 205, 102, 
     41,   0,   0,   0,  83,  84, 
     65,  84, 148,   0,   0,   0, 
     16,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,   9,   0, 
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
