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
// First Precompiled Shader at offset:[154]
// Embedded Data:
//  0x0000009a - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x00000095 - Original Shader Size
dcl_globalFlags refactoringAllowed
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
dcl_temps 1
sample_indexable(texture2d)(float,float,float,float) r0.x, v1.xyxx, t2.xyzw, s2
add r0.x, r0.x, l(-0.501961)
mul r0.xyz, r0.xxxx, l(1.596000, -0.813000, 0.000000, 0.000000)
sample_indexable(texture2d)(float,float,float,float) r0.w, v1.xyxx, t0.yzwx, s0
add r0.w, r0.w, l(-0.062745)
mad r0.xyz, r0.wwww, l(1.164000, 1.164000, 1.164000, 0.000000), r0.xyzx
sample_indexable(texture2d)(float,float,float,float) r0.w, v1.xyxx, t1.yzwx, s1
add r0.w, r0.w, l(-0.501961)
mad o0.xyz, r0.wwww, l(0.000000, -0.392000, 2.017000, 0.000000), r0.xyzx
sample_indexable(texture2d)(float,float,float,float) r0.x, v1.xyxx, t3.xyzw, s3
mul o0.w, r0.x, v0.w
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[154], bundle is:[180] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FYUVAEAlpha.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask  8, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask  3, param1, paramSlot 1, DefaultVal={0,0,0,0}

codeLenInByte        = 236;Bytes

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
NumVgprs             = 8;
NumSgprs             = 50;
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
  s_mov_b64     s[48:49], exec                          // 000000000000: BEB0047E
  s_wqm_b64     exec, exec                              // 000000000004: BEFE0A7E
  s_mov_b32     m0, s2                                  // 000000000008: BEFC0302
  s_load_dwordx8  s[16:23], s[0:1], 0x00                // 00000000000C: C0C80100
  s_load_dwordx8  s[32:39], s[0:1], 0x08                // 000000000010: C0D00108
  s_load_dwordx8  s[4:11], s[0:1], 0x10                 // 000000000014: C0C20110
  s_load_dwordx8  s[24:31], s[0:1], 0x20                // 000000000018: C0CC0120
  s_load_dwordx4  s[12:15], s[0:1], 0x28                // 00000000001C: C0860128
  v_interp_p1_f32  v2, v0, attr1.x                      // 000000000020: C8080400
  v_interp_p1_f32  v3, v0, attr1.y                      // 000000000024: C80C0500
  v_interp_p2_f32  v2, v1, attr1.x                      // 000000000028: C8090401
  v_interp_p2_f32  v3, v1, attr1.y                      // 00000000002C: C80D0501
  s_waitcnt     lgkmcnt(0)                              // 000000000030: BF8C007F
  image_sample  v[4:7], v[2:3], s[4:11], s[12:15]       // 000000000034: F0800100 00610402
  image_sample  v[5:8], v[2:3], s[16:23], s[24:27]      // 00000000003C: F0800100 00C40502
  image_sample  v[6:9], v[2:3], s[32:39], s[28:31]      // 000000000044: F0800100 00E80602
  s_load_dwordx8  s[40:47], s[0:1], 0x18                // 00000000004C: C0D40118
  s_load_dwordx4  s[0:3], s[0:1], 0x2c                  // 000000000050: C080012C
  s_waitcnt     lgkmcnt(0)                              // 000000000054: BF8C007F
  image_sample  v[2:5], v[2:3], s[40:47], s[0:3]        // 000000000058: F0800100 000A0202
  v_interp_p1_f32  v0, v0, attr0.w                      // 000000000060: C8000300
  s_waitcnt     vmcnt(3) & lgkmcnt(15)                  // 000000000064: BF8C0F73
  v_add_f32     v3, 0xbf008081, v4                      // 000000000068: 060608FF BF008081
  v_mul_legacy_f32  v4, 0xbf5020c5, v3                  // 000000000070: 0E0806FF BF5020C5
  s_waitcnt     vmcnt(2) & lgkmcnt(15)                  // 000000000078: BF8C0F72
  v_add_f32     v5, 0xbd808081, v5                      // 00000000007C: 060A0AFF BD808081
  s_mov_b32     s0, 0x3f94fdf4                          // 000000000084: BE8003FF 3F94FDF4
  v_interp_p2_f32  v0, v1, attr0.w                      // 00000000008C: C8010301
  v_mul_legacy_f32  v3, 0x3fcc49ba, v3                  // 000000000090: 0E0606FF 3FCC49BA
  v_mul_legacy_f32  v7, 0x3f94fdf4, v5                  // 000000000098: 0E0E0AFF 3F94FDF4
  v_mac_legacy_f32  v4, s0, v5                          // 0000000000A0: 0C080A00
  s_waitcnt     vmcnt(1) & lgkmcnt(15)                  // 0000000000A4: BF8C0F71
  v_add_f32     v6, 0xbf008081, v6                      // 0000000000A8: 060C0CFF BF008081
  s_mov_b32     s1, 0xbec8b439                          // 0000000000B0: BE8103FF BEC8B439
  s_mov_b32     s2, 0x40011687                          // 0000000000B8: BE8203FF 40011687
  v_mac_legacy_f32  v3, s0, v5                          // 0000000000C0: 0C060A00
  v_mac_legacy_f32  v4, s1, v6                          // 0000000000C4: 0C080C01
  v_mac_legacy_f32  v7, s2, v6                          // 0000000000C8: 0C0E0C02
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 0000000000CC: BF8C0F70
  v_mul_legacy_f32  v0, v2, v0                          // 0000000000D0: 0E000102
  s_mov_b64     exec, s[48:49]                          // 0000000000D4: BEFE0430
  v_cvt_pkrtz_f16_f32  v1, v3, v4                       // 0000000000D8: 5E020903
  v_cvt_pkrtz_f16_f32  v0, v7, v0                       // 0000000000DC: 5E000107
  exp           mrt0, v1, v1, v0, v0 compr vm           // 0000000000E0: F800140F 00000001
  s_endpgm                                              // 0000000000E8: BF810000
end


// Approximately 12 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FYUVAEAlpha[] =
{
     68,  88,  66,  67,  87,  26, 
    252,   7, 229, 251,  93,  68, 
    240, 184,  71, 118,   5, 139, 
    158, 171,   1,   0,   0,   0, 
    116,   8,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
     12,   2,   0,   0,  92,   2, 
      0,   0, 144,   2,   0,   0, 
    216,   7,   0,   0,  82,  68, 
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
     72,   0,   0,   0,   2,   0, 
      0,   0,   8,   0,   0,   0, 
     56,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   8,   0,   0, 
     62,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,   3,   3,   0,   0, 
     67,  79,  76,  79,  82,   0, 
     84,  69,  88,  67,  79,  79, 
     82,  68,   0, 171,  79,  83, 
     71,  78,  44,   0,   0,   0, 
      1,   0,   0,   0,   8,   0, 
      0,   0,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,   0, 
      0,   0,  83,  86,  95,  84, 
     97, 114, 103, 101, 116,   0, 
    171, 171,  83,  72,  69,  88, 
     64,   5,   0,   0,  80,   0, 
      0,   0,  80,   1,   0,   0, 
     53,  16,   0,   0,   5,   0, 
      0,   0, 154,   0,   0,   0, 
     80,   0,   0,   0, 149,   0, 
      0,   0, 106,   8,   0,   1, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   1,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   2,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   3,   0,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     85,  85,   0,   0,  88,  24, 
      0,   4,   0, 112,  16,   0, 
      1,   0,   0,   0,  85,  85, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   2,   0, 
      0,   0,  85,  85,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   3,   0,   0,   0, 
     85,  85,   0,   0,  98,  16, 
      0,   3, 130,  16,  16,   0, 
      0,   0,   0,   0,  98,  16, 
      0,   3,  50,  16,  16,   0, 
      1,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0, 104,   0, 
      0,   2,   1,   0,   0,   0, 
     69,   0,   0, 139, 194,   0, 
      0, 128,  67,  85,  21,   0, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  70,  16,  16,   0, 
      1,   0,   0,   0,  70, 126, 
     16,   0,   2,   0,   0,   0, 
      0,  96,  16,   0,   2,   0, 
      0,   0,   0,   0,   0,   7, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0, 129, 128,   0, 191, 
     56,   0,   0,  10, 114,   0, 
     16,   0,   0,   0,   0,   0, 
      6,   0,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
    186,  73, 204,  63, 197,  32, 
     80, 191,   0,   0,   0,   0, 
      0,   0,   0,   0,  69,   0, 
      0, 139, 194,   0,   0, 128, 
     67,  85,  21,   0, 130,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  16,  16,   0,   1,   0, 
      0,   0, 150, 115,  16,   0, 
      0,   0,   0,   0,   0,  96, 
     16,   0,   0,   0,   0,   0, 
      0,   0,   0,   7, 130,   0, 
     16,   0,   0,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
    129, 128, 128, 189,  50,   0, 
      0,  12, 114,   0,  16,   0, 
      0,   0,   0,   0, 246,  15, 
     16,   0,   0,   0,   0,   0, 
      2,  64,   0,   0, 244, 253, 
    148,  63, 244, 253, 148,  63, 
    244, 253, 148,  63,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,  69,   0, 
      0, 139, 194,   0,   0, 128, 
     67,  85,  21,   0, 130,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  16,  16,   0,   1,   0, 
      0,   0, 150, 115,  16,   0, 
      1,   0,   0,   0,   0,  96, 
     16,   0,   1,   0,   0,   0, 
      0,   0,   0,   7, 130,   0, 
     16,   0,   0,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
    129, 128,   0, 191,  50,   0, 
      0,  12, 114,  32,  16,   0, 
      0,   0,   0,   0, 246,  15, 
     16,   0,   0,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,  57, 180, 200, 190, 
    135,  22,   1,  64,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,  69,   0, 
      0, 139, 194,   0,   0, 128, 
     67,  85,  21,   0,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  16,  16,   0,   1,   0, 
      0,   0,  70, 126,  16,   0, 
      3,   0,   0,   0,   0,  96, 
     16,   0,   3,   0,   0,   0, 
     56,   0,   0,   7, 130,  32, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,  58,  16,  16,   0, 
      0,   0,   0,   0,  62,   0, 
      0,   1,  53,  16,   0,   0, 
    182,   0,   0,   0,  21,   0, 
      1,   0,  41,  14,  11,   0, 
    120,   1, 237,  86, 207, 107, 
    212,  64,  20, 126, 147, 236, 
    166, 233, 154, 102,  11,  85, 
    169,  80, 164,   7, 133,  30, 
    234, 210,  95,  74,  21, 161, 
     45,  84, 209, 155, 216,  42, 
    182, 132, 210, 172, 221,  90, 
    113, 251, 195, 236,  34, 139, 
    135, 118, 235,  65,  47,  61, 
     40,   8, 253,  55,  68, 244, 
     34, 152,  13, 162,  16, 161, 
    136, 160, 255, 128,  55,  15, 
     61, 244, 208,  67,  17, 109, 
    124,  95,  50,  67, 115, 168, 
    183, 130, 130, 253, 224, 123, 
    111, 190,  55, 111, 126, 100, 
     38, 204, 140, 125, 132,  98, 
    140, 111,  61,  91, 135, 255, 
     49, 216,  63,  54, 174,  17, 
    101, 185,  44, 152, 240,  64, 
    205, 132,  37, 218, 110, 134, 
     37,  58,   5, 131,   4, 206, 
     69,  85,  31, 115, 147,  57, 
    204, 236,  98, 162,  30,  62, 
    193, 112, 176, 194,  22,  26, 
    205,  79, 114, 155,  43, 236, 
    161, 209, 127,  27, 243,  24, 
     19, 221,   1,  25,  24, 137, 
     38,  24, 134, 242, 105, 180, 
    192,  72,  24,  76, 149, 131, 
     62, 121, 136,  61, 156, 134, 
    217,  31, 103, 152, 242, 211, 
    246,   5, 230, 166, 230, 131, 
    249,  97, 158,   0, 190, 247, 
    111,  35,  89,  47,  29, 134, 
    242,  48,  12, 181,  79,   0, 
    246,  35,  13, 155, 153, 180, 
     33, 154, 145, 251, 168, 210, 
    171,  41, 141, 111,  11, 152, 
    127,  66, 196, 232,  65,  33, 
      5, 165, 177, 238, 134, 236, 
    213,  16, 109, 212, 137, 194, 
    127,   6, 245, 191,  96,  45, 
    192,  86, 166, 218,  31,   0, 
    245, 231, 152,  68, 245,   6, 
    248, 180, 157, 118, 160, 148, 
     22, 172, 147,  93,  77, 180, 
     14,  29, 119, 154, 210, 172, 
    244, 102, 109, 170,  73, 208, 
     84, 190, 131, 118, 176, 175, 
    172,  29, 214,  78,  74,  79, 
    178, 158,  76, 233, 105, 214, 
    211,  41,  93, 100,  93,  84, 
     90, 245, 159, 143, 199, 107, 
    138, 247, 153, 131, 203, 255, 
     50,  15, 231, 120,  48,  60, 
    156, 227, 193, 240, 112, 142, 
      7, 198,  24, 203, 153,  23, 
    254, 114, 110, 215, 215, 244, 
    159,  62, 137,  48,  48, 197, 
    231, 160,  85, 188,  11,  58, 
    197,  70, 208,  37,  30,   7, 
    148,  49,  67, 202,  90, 161, 
    200,  52, 135,  34, 219,  18, 
    174, 208,  90, 131,  68, 125, 
     75, 203, 184, 124, 217, 177, 
    207, 190,  79, 188, 241, 157, 
    218, 197, 215, 160,  91, 212, 
    249,  45,  34, 115, 180,  28, 
     31, 130,  20,  86, 242, 107, 
    141, 200,  52, 140, 213,  58, 
     53,  34, 195, 180,  63, 116, 
     94, 107, 120, 136, 229, 114, 
     28, 171, 191, 141, 244, 186, 
    191, 253, 235, 249, 144, 208, 
     69,  24,  25, 134, 253, 230, 
    234, 198,  80, 148, 179, 109, 
    196,  40, 103,  90, 247, 145, 
    107,  89,  73, 123, 125, 213, 
     63, 255,  42, 244,  35, 253, 
    145, 255, 228, 184,  24, 166, 
    156,  97,   9, 203, 180,  52, 
    203, 182, 150,  56,  79,  19, 
    100, 247, 100, 118, 125,  62, 
    123, 147, 179, 251, 168,  58, 
    123,  87,  27,  39, 216,  38, 
    229, 228, 222,  72, 131, 111, 
    212, 184,  10,   6, 183, 171, 
    210, 234, 126,  81,  26, 247, 
     66,  59,  19,  58,  14,  48, 
    112,   7, 243, 245,  17, 107, 
     16, 111,  51, 165, 209,  30, 
    239,  48, 165, 209, 190, 155, 
      9, 221, 193, 203,   3,  88, 
    210,  99,  28, 142, 243,  59, 
      2, 153, 123, 184,  40, 235, 
      7, 164, 231, 177,   9, 207, 
    202, 151, 126, 244, 105, 176, 
    235, 236, 230,  45,  25,  31, 
    151,  30, 125, 160, 248, 122, 
    253, 139, 247,  81, 198, 150, 
    164, 159,  96, 162, 253, 236, 
      5, 167, 232, 150, 221, 135, 
    149,  66, 117, 209,  91, 188, 
     87, 168,  84, 189, 146,  59, 
    223, 231, 204, 148,  30,  56, 
    165,  90, 181, 228,  45, 184, 
    101, 231, 206, 108, 109, 192, 
     25, 243, 110,  59, 215,  75, 
     11,  51,  37, 207,  25, 237, 
     31, 237, 173,  57,  99, 115, 
     46, 139, 138, 115, 121, 226, 
    198, 205, 145,  75,  35, 229, 
    165,  57, 183,  48,  91, 238, 
    237,  45, 204, 149,  43, 101, 
    238,  58, 193,  55,  57,  32, 
     30, 139, 120,  56, 206, 187, 
    119,  23, 232,  55, 214, 242, 
     83,  63,   0,   0,  83,  84, 
     65,  84, 148,   0,   0,   0, 
     12,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   7,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
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
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0
};
