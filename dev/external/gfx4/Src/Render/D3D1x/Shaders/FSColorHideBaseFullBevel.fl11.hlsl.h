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
//   float4 fsize;                      // Offset:    0 Size:    16
//   float4 offset;                     // Offset:   16 Size:    16
//   float4 scolor;                     // Offset:   32 Size:    16
//   float4 srctexscale;                // Offset:   48 Size:    16
//   float4 texscale;                   // Offset:   64 Size:    16
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// sampler_srctex                    sampler      NA          NA    0        1
// sampler_tex                       sampler      NA          NA    1        1
// srctex                            texture  float4          2d    0        1
// tex                               texture  float4          2d    1        1
// Constants                         cbuffer      NA          NA    0        1
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
// First Precompiled Shader at offset:[277]
// Embedded Data:
//  0x00000115 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x00000110 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_constantbuffer cb0[5], immediateIndexed
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.xyzw
dcl_input_ps linear v2.xy
dcl_output o0.xyzw
dcl_temps 3
mov r0.x, l(0)
mov r0.y, -cb0[0].x
loop 
  lt r0.z, cb0[0].x, r0.y
  breakc_nz r0.z
  add r1.x, r0.y, cb0[1].x
  mov r2.x, r0.x
  mov r2.y, -cb0[0].y
  loop 
    lt r0.z, cb0[0].y, r2.y
    breakc_nz r0.z
    add r1.y, r2.y, cb0[1].y
    mad r0.zw, r1.xxxy, cb0[4].xxxy, v2.xxxy
    sample_l_indexable(texture2d)(float,float,float,float) r0.z, r0.zwzz, t1.xywz, s1, l(0.000000)
    add r2.x, r0.z, r2.x
    add r2.y, r2.y, l(1.000000)
  endloop 
  mov r0.x, r2.x
  add r0.y, r0.y, l(1.000000)
endloop 
mul r0.x, r0.x, cb0[0].w
mul_sat r0.x, r0.x, cb0[0].z
mul r0.yz, v2.xxyx, cb0[3].xxyx
sample_l_indexable(texture2d)(float,float,float,float) r0.y, r0.yzyy, t0.xwyz, s0, l(0.000000)
mul r1.w, r0.y, l(0.000100)
mul r0.y, r0.x, cb0[2].w
mad r0.x, -r0.x, cb0[2].w, l(1.000000)
mov r1.xyz, l(0,0,0,0)
mul r1.xyzw, r0.xxxx, r1.xyzw
mad r0.xyzw, cb0[2].xyzw, r0.yyyy, r1.xyzw
mov r1.xyz, v1.xyzx
mov r1.w, l(1.000000)
mul r0.xyzw, r0.xyzw, r1.xyzw
mul r0.xyzw, r0.xyzw, v1.wwww
mad o0.xyzw, v0.xyzw, r0.wwww, r0.xyzw
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[277], bundle is:[209] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FSColorHideBaseFullBevel.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask 15, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask  3, param2, paramSlot 2, DefaultVal={0,0,0,0}

codeLenInByte        = 396;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 5;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_RESOURCE, 1, offset 8:15 dwords
;  extUserElements 1[2] = IMM_SAMPLER, 0, offset 16:19 dwords
;  extUserElements 1[3] = IMM_SAMPLER, 1, offset 20:23 dwords
;  extUserElements 1[4] = IMM_CONST_BUFFER, 0, offset 24:27 dwords
NumVgprs             = 13;
NumSgprs             = 26;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000003
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
  s_mov_b32     m0, s2                                  // 000000000000: BEFC0302
  s_load_dwordx8  s[4:11], s[0:1], 0x14                 // 000000000004: C0C20114
  s_waitcnt     lgkmcnt(0)                              // 000000000008: BF8C007F
  s_buffer_load_dwordx4  s[12:15], s[8:11], 0x00        // 00000000000C: C2860900
  v_mov_b32     v2, 0                                   // 000000000010: 7E040280
  s_load_dwordx8  s[16:23], s[0:1], 0x08                // 000000000014: C0C80108
  s_waitcnt     lgkmcnt(0)                              // 000000000018: BF8C007F
  v_max_f32     v3, -s12, -s12                          // 00000000001C: D2200003 6000180C
label_0009:
  v_mov_b32     v4, s12                                 // 000000000024: 7E08020C
  v_cmp_gt_f32  vcc, v3, v4                             // 000000000028: 7C080903
  s_cbranch_vccnz  label_0027                           // 00000000002C: BF87001B
  s_buffer_load_dwordx2  s[2:3], s[8:11], 0x04          // 000000000030: C2410904
  s_waitcnt     lgkmcnt(0)                              // 000000000034: BF8C007F
  v_add_f32     v4, s2, v3                              // 000000000038: 06080602
  v_max_f32     v5, -s13, -s13                          // 00000000003C: D2200005 60001A0D
label_0011:
  v_mov_b32     v6, s13                                 // 000000000044: 7E0C020D
  v_cmp_gt_f32  vcc, v5, v6                             // 000000000048: 7C080D05
  s_cbranch_vccnz  label_0025                           // 00000000004C: BF870011
  s_buffer_load_dwordx2  s[24:25], s[8:11], 0x10        // 000000000050: C24C0910
  v_interp_p1_f32  v7, v0, attr2.x                      // 000000000054: C81C0800
  v_interp_p1_f32  v8, v0, attr2.y                      // 000000000058: C8200900
  v_interp_p2_f32  v7, v1, attr2.x                      // 00000000005C: C81D0801
  v_interp_p2_f32  v8, v1, attr2.y                      // 000000000060: C8210901
  v_add_f32     v6, s3, v5                              // 000000000064: 060C0A03
  s_waitcnt     lgkmcnt(0)                              // 000000000068: BF8C007F
  v_mad_legacy_f32  v9, v4, s24, v7                     // 00000000006C: D2800009 041C3104
  v_mad_legacy_f32  v10, v6, s25, v8                    // 000000000074: D280000A 04203306
  image_sample_lz  v[6:9], v[9:10], s[16:23], s[4:7] dmask:0x8 // 00000000007C: F09C0800 00240609
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000084: BF8C0F70
  v_add_f32     v2, v2, v6                              // 000000000088: 06040D02
  v_add_f32     v5, 1.0, v5                             // 00000000008C: 060A0AF2
  s_branch      label_0011                              // 000000000090: BF82FFEC
label_0025:
  v_add_f32     v3, 1.0, v3                             // 000000000094: 060606F2
  s_branch      label_0009                              // 000000000098: BF82FFE2
label_0027:
  s_buffer_load_dwordx2  s[2:3], s[8:11], 0x0c          // 00000000009C: C241090C
  v_interp_p1_f32  v3, v0, attr2.x                      // 0000000000A0: C80C0800
  v_interp_p1_f32  v4, v0, attr2.y                      // 0000000000A4: C8100900
  s_load_dwordx8  s[16:23], s[0:1], 0x00                // 0000000000A8: C0C80100
  s_load_dwordx4  s[4:7], s[0:1], 0x10                  // 0000000000AC: C0820110
  v_interp_p2_f32  v3, v1, attr2.x                      // 0000000000B0: C80D0801
  v_interp_p2_f32  v4, v1, attr2.y                      // 0000000000B4: C8110901
  s_waitcnt     lgkmcnt(0)                              // 0000000000B8: BF8C007F
  v_mul_legacy_f32  v3, s2, v3                          // 0000000000BC: 0E060602
  v_mul_legacy_f32  v4, s3, v4                          // 0000000000C0: 0E080803
  image_sample_lz  v[3:6], v[3:4], s[16:23], s[4:7] dmask:0x8 // 0000000000C4: F09C0800 00240303
  s_buffer_load_dwordx4  s[0:3], s[8:11], 0x08          // 0000000000CC: C2800908
  v_mul_legacy_f32  v2, s15, v2                         // 0000000000D0: 0E04040F
  v_mul_legacy_f32  v2, s14, v2 clamp                   // 0000000000D4: D20E0802 0002040E
  s_waitcnt     lgkmcnt(0)                              // 0000000000DC: BF8C007F
  v_mul_legacy_f32  v4, s3, v2                          // 0000000000E0: 0E080403
  v_interp_p1_f32  v5, v0, attr1.x                      // 0000000000E4: C8140400
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 0000000000E8: BF8C0F70
  v_mul_legacy_f32  v3, 0x38d1b717, v3                  // 0000000000EC: 0E0606FF 38D1B717
  v_mad_legacy_f32  v2, -v2, s3, 1.0                    // 0000000000F4: D2800002 23C80702
  v_interp_p1_f32  v6, v0, attr1.y                      // 0000000000FC: C8180500
  v_interp_p1_f32  v7, v0, attr1.z                      // 000000000100: C81C0600
  v_mul_legacy_f32  v2, v3, v2                          // 000000000104: 0E040503
  v_interp_p2_f32  v5, v1, attr1.x                      // 000000000108: C8150401
  v_mul_legacy_f32  v3, s0, v4                          // 00000000010C: 0E060800
  v_interp_p2_f32  v6, v1, attr1.y                      // 000000000110: C8190501
  v_interp_p2_f32  v7, v1, attr1.z                      // 000000000114: C81D0601
  v_mul_legacy_f32  v8, s1, v4                          // 000000000118: 0E100801
  v_mul_legacy_f32  v9, s2, v4                          // 00000000011C: 0E120802
  v_mac_legacy_f32  v2, s3, v4                          // 000000000120: 0C040803
  v_mul_legacy_f32  v3, v3, v5                          // 000000000124: 0E060B03
  v_interp_p1_f32  v10, v0, attr1.w                     // 000000000128: C8280700
  v_mul_legacy_f32  v4, v8, v6                          // 00000000012C: 0E080D08
  v_mul_legacy_f32  v5, v9, v7                          // 000000000130: 0E0A0F09
  v_interp_p1_f32  v6, v0, attr0.x                      // 000000000134: C8180000
  v_interp_p1_f32  v7, v0, attr0.y                      // 000000000138: C81C0100
  v_interp_p1_f32  v8, v0, attr0.z                      // 00000000013C: C8200200
  v_interp_p2_f32  v10, v1, attr1.w                     // 000000000140: C8290701
  v_interp_p1_f32  v0, v0, attr0.w                      // 000000000144: C8000300
  v_interp_p2_f32  v6, v1, attr0.x                      // 000000000148: C8190001
  v_interp_p2_f32  v7, v1, attr0.y                      // 00000000014C: C81D0101
  v_interp_p2_f32  v8, v1, attr0.z                      // 000000000150: C8210201
  v_mul_legacy_f32  v3, v3, v10                         // 000000000154: 0E061503
  v_mul_legacy_f32  v4, v4, v10                         // 000000000158: 0E081504
  v_mul_legacy_f32  v5, v5, v10                         // 00000000015C: 0E0A1505
  v_mul_legacy_f32  v2, v2, v10                         // 000000000160: 0E041502
  v_interp_p2_f32  v0, v1, attr0.w                      // 000000000164: C8010301
  v_mac_legacy_f32  v3, v6, v2                          // 000000000168: 0C060506
  v_mac_legacy_f32  v4, v7, v2                          // 00000000016C: 0C080507
  v_mac_legacy_f32  v5, v8, v2                          // 000000000170: 0C0A0508
  v_mac_legacy_f32  v2, v0, v2                          // 000000000174: 0C040500
  v_cvt_pkrtz_f16_f32  v0, v3, v4                       // 000000000178: 5E000903
  v_cvt_pkrtz_f16_f32  v1, v5, v2                       // 00000000017C: 5E020505
  exp           mrt0, v0, v0, v1, v1 compr vm           // 000000000180: F800140F 00000100
  s_endpgm                                              // 000000000188: BF810000
end


// Approximately 36 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FSColorHideBaseFullBevel[] =
{
     68,  88,  66,  67, 165, 233, 
     27, 166,   4, 118, 116,  34, 
    178,   4, 246,   7,  77, 188, 
    178,  10,   1,   0,   0,   0, 
    152,  11,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    188,   2,   0,   0,  32,   3, 
      0,   0,  84,   3,   0,   0, 
    252,  10,   0,   0,  82,  68, 
     69,  70, 128,   2,   0,   0, 
      1,   0,   0,   0,  12,   1, 
      0,   0,   5,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
     67,   2,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
    220,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0, 235,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
      1,   0,   0,   0, 247,   0, 
      0,   0,   2,   0,   0,   0, 
      5,   0,   0,   0,   4,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  13,   0,   0,   0, 
    254,   0,   0,   0,   2,   0, 
      0,   0,   5,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   1,   0,   0,   0, 
      1,   0,   0,   0,  13,   0, 
      0,   0,   2,   1,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0, 115,  97, 
    109, 112, 108, 101, 114,  95, 
    115, 114,  99, 116, 101, 120, 
      0, 115,  97, 109, 112, 108, 
    101, 114,  95, 116, 101, 120, 
      0, 115, 114,  99, 116, 101, 
    120,   0, 116, 101, 120,   0, 
     67, 111, 110, 115, 116,  97, 
    110, 116, 115,   0,   2,   1, 
      0,   0,   5,   0,   0,   0, 
     36,   1,   0,   0,  80,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0, 236,   1, 
      0,   0,   0,   0,   0,   0, 
     16,   0,   0,   0,   2,   0, 
      0,   0, 252,   1,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,  32,   2,   0,   0, 
     16,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
    252,   1,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
     39,   2,   0,   0,  32,   0, 
      0,   0,  16,   0,   0,   0, 
      2,   0,   0,   0, 252,   1, 
      0,   0,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,  46,   2, 
      0,   0,  48,   0,   0,   0, 
     16,   0,   0,   0,   2,   0, 
      0,   0, 252,   1,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,  58,   2,   0,   0, 
     64,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
    252,   1,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    102, 115, 105, 122, 101,   0, 
    102, 108, 111,  97, 116,  52, 
      0, 171, 171, 171,   1,   0, 
      3,   0,   1,   0,   4,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    242,   1,   0,   0, 111, 102, 
    102, 115, 101, 116,   0, 115, 
     99, 111, 108, 111, 114,   0, 
    115, 114,  99, 116, 101, 120, 
    115,  99,  97, 108, 101,   0, 
    116, 101, 120, 115,  99,  97, 
    108, 101,   0,  77, 105,  99, 
    114, 111, 115, 111, 102, 116, 
     32,  40,  82,  41,  32,  72, 
     76,  83,  76,  32,  83, 104, 
     97, 100, 101, 114,  32,  67, 
    111, 109, 112, 105, 108, 101, 
    114,  32,  70, 111, 114,  32, 
     68, 117, 114,  97, 110, 103, 
    111,  32,  57,  46,  51,  48, 
     46,  49,  50,  48,  57,  56, 
     46,  48,   0, 171,  73,  83, 
     71,  78,  92,   0,   0,   0, 
      3,   0,   0,   0,   8,   0, 
      0,   0,  80,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,  15, 
      0,   0,  80,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,  15,  15, 
      0,   0,  80,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      2,   0,   0,   0,   3,   3, 
      0,   0,  84,  69,  88,  67, 
     79,  79,  82,  68,   0, 171, 
    171, 171,  79,  83,  71,  78, 
     44,   0,   0,   0,   1,   0, 
      0,   0,   8,   0,   0,   0, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   0,   0,   0, 
     83,  86,  95,  84,  97, 114, 
    103, 101, 116,   0, 171, 171, 
     83,  72,  69,  88, 160,   7, 
      0,   0,  80,   0,   0,   0, 
    232,   1,   0,   0,  53,  16, 
      0,   0,   5,   0,   0,   0, 
     21,   1,   0,   0,  80,   0, 
      0,   0,  16,   1,   0,   0, 
    106,   8,   0,   1,  89,   0, 
      0,   4,  70, 142,  32,   0, 
      0,   0,   0,   0,   5,   0, 
      0,   0,  90,   0,   0,   3, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  90,   0,   0,   3, 
      0,  96,  16,   0,   1,   0, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   0,   0, 
      0,   0,  85,  85,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   1,   0,   0,   0, 
     85,  85,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      0,   0,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      1,   0,   0,   0,  98,  16, 
      0,   3,  50,  16,  16,   0, 
      2,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0, 104,   0, 
      0,   2,   3,   0,   0,   0, 
     54,   0,   0,   5,  18,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,  54,   0,   0,   7, 
     34,   0,  16,   0,   0,   0, 
      0,   0,  10, 128,  32, 128, 
     65,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     48,   0,   0,   1,  49,   0, 
      0,   8,  66,   0,  16,   0, 
      0,   0,   0,   0,  10, 128, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
      3,   0,   4,   3,  42,   0, 
     16,   0,   0,   0,   0,   0, 
      0,   0,   0,   8,  18,   0, 
     16,   0,   1,   0,   0,   0, 
     26,   0,  16,   0,   0,   0, 
      0,   0,  10, 128,  32,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  54,   0,   0,   5, 
     18,   0,  16,   0,   2,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   7,  34,   0,  16,   0, 
      2,   0,   0,   0,  26, 128, 
     32, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  48,   0,   0,   1, 
     49,   0,   0,   8,  66,   0, 
     16,   0,   0,   0,   0,   0, 
     26, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     26,   0,  16,   0,   2,   0, 
      0,   0,   3,   0,   4,   3, 
     42,   0,  16,   0,   0,   0, 
      0,   0,   0,   0,   0,   8, 
     34,   0,  16,   0,   1,   0, 
      0,   0,  26,   0,  16,   0, 
      2,   0,   0,   0,  26, 128, 
     32,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,  50,   0, 
      0,  10, 194,   0,  16,   0, 
      0,   0,   0,   0,   6,   4, 
     16,   0,   1,   0,   0,   0, 
      6, 132,  32,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
      6,  20,  16,   0,   2,   0, 
      0,   0,  72,   0,   0, 141, 
    194,   0,   0, 128,  67,  85, 
     21,   0,  66,   0,  16,   0, 
      0,   0,   0,   0, 230,  10, 
     16,   0,   0,   0,   0,   0, 
     70, 123,  16,   0,   1,   0, 
      0,   0,   0,  96,  16,   0, 
      1,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   7,  18,   0, 
     16,   0,   2,   0,   0,   0, 
     42,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   7,  34,   0,  16,   0, 
      2,   0,   0,   0,  26,   0, 
     16,   0,   2,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  22,   0,   0,   1, 
     54,   0,   0,   5,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   2,   0, 
      0,   0,   0,   0,   0,   7, 
     34,   0,  16,   0,   0,   0, 
      0,   0,  26,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     22,   0,   0,   1,  56,   0, 
      0,   8,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
     58, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     56,  32,   0,   8,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,  42, 128,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  56,   0,   0,   8, 
     98,   0,  16,   0,   0,   0, 
      0,   0,   6,  17,  16,   0, 
      2,   0,   0,   0,   6, 129, 
     32,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,  72,   0, 
      0, 141, 194,   0,   0, 128, 
     67,  85,  21,   0,  34,   0, 
     16,   0,   0,   0,   0,   0, 
    150,   5,  16,   0,   0,   0, 
      0,   0, 198, 121,  16,   0, 
      0,   0,   0,   0,   0,  96, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,  56,   0,   0,   7, 
    130,   0,  16,   0,   1,   0, 
      0,   0,  26,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,  23, 183, 209,  56, 
     56,   0,   0,   8,  34,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,  58, 128,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,  50,   0,   0,  11, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16, 128, 
     65,   0,   0,   0,   0,   0, 
      0,   0,  58, 128,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128,  63,  54,   0, 
      0,   8, 114,   0,  16,   0, 
      1,   0,   0,   0,   2,  64, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     56,   0,   0,   7, 242,   0, 
     16,   0,   1,   0,   0,   0, 
      6,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      1,   0,   0,   0,  50,   0, 
      0,  10, 242,   0,  16,   0, 
      0,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,  86,   5, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   1,   0, 
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
     53,  16,   0,   0, 211,   0, 
      0,   0,  21,   0,   1,   0, 
     41, 139,  11,   0, 120,   1, 
    237,  86, 207, 107,  19,  65, 
     24, 253, 246, 215, 100,  54, 
    221, 108,  82,  73, 236,  15, 
     35,  86, 218,  74, 188,  20, 
     98,  61,  72,  47, 214,  90, 
    138,   7,  15,  98,  79, 234, 
     82, 154, 218, 173,  45, 110, 
     91,  73, 106,   9,  82, 106, 
    172, 160, 130,  61, 248,   7, 
    136,  39, 111, 122, 247, 164, 
    169, 193, 195,  92, 205,  95, 
     80, 188, 120, 241,  32,  61, 
    136, 168,  80, 223, 183, 217, 
    213,  28, 234, 173, 160,  96, 
     31, 188, 249, 246, 237,  55, 
    243, 230, 155, 157, 133, 153, 
    199,  29,  20,  98, 190,  48, 
    190, 195, 177,  99, 228, 251, 
    235,  81, 147, 200, 194, 179, 
      6, 114, 100,  84,  37, 183, 
     68, 179,  73, 110, 137,  10, 
    220, 112,   7, 157,  40, 133, 
    208,  11, 110,  66, 143,  34, 
    114,  46, 102,  11, 234, 237, 
     32, 114,  39, 240, 116,  24, 
    252,   2, 255, 107, 136, 151, 
     64,  27,  60,  11, 118, 129, 
    232,  18,   2, 150, 191, 192, 
    222, 140,  56,  23, 195,   0, 
    243, 160,   0, 185, 127,  55, 
    200,  94,  12, 206, 181, 123, 
    208,  32,  55, 123, 227,  10, 
     24, 207, 177,  23, 184, 182, 
    216, 139, 107, 224,  58,  25, 
    188, 222, 191, 141, 214,  55, 
    105,  85, 199, 107, 102,  29, 
    239,  19, 131, 247, 163,  29, 
     46,  24, 191,  50, 163, 125, 
    140, 247,  55,  27, 105, 254, 
     22, 188, 182,  45, 240,  79, 
    216,   5, 248, 219, 183,  35, 
    214,  92, 135, 160,  52,  90, 
     68, 237,  24, 245, 113, 212, 
    251, 195, 255, 226, 127,   1, 
    126, 239,  16, 252,  45, 120, 
    119,  50,  96, 235, 139, 180, 
    192, 249,  41, 144, 168,  86, 
    103,  62, 233, 166, 175, 172, 
     98, 173,  65, 243, 216,  88, 
     27, 172,  67, 211,  54,  13, 
     99, 195, 166,  41, 203, 210, 
    167, 210, 121, 140, 199, 198, 
     66, 123, 208,  94, 155, 190, 
     10, 125, 181,  77,  79,  67, 
     79, 183, 233,  25, 232, 153, 
     88, 199, 254, 233, 112,  62, 
     75, 231, 125, 198, 203, 245, 
    127, 153,   7,  53, 238,  15, 
     15, 106, 220,  31,  30, 212, 
    184, 111,  12, 161,  27,  63, 
    222, 100, 181, 198, 214,  93, 
    218, 172, 147, 253, 160,  81, 
    211, 205, 117, 169, 169,  80, 
     27, 212, 215, 116, 186, 105, 
    218, 209, 229, 186,  97, 203, 
    181,  35, 244, 176, 110, 218, 
    231,  26, 156, 211, 133,  20, 
     22, 242, 169,  94, 154,  78, 
    233, 206, 186, 149, 146, 107, 
    157, 200, 103, 236, 139,  13, 
    146, 121,  69, 118, 159, 210, 
    228,  81, 165, 217, 199, 149, 
    145, 116,   4, 143, 177, 169, 
    214,  52, 139, 121,  28, 141, 
    181, 166,  24, 238,  51,  73, 
     62, 253, 108, 139,   1, 186, 
    149, 134,  95, 202,  20,  59, 
    201, 164, 248, 180, 187,  81, 
    223,  17,  66, 124,  64, 116, 
     48,  23,  73,   7,  94,  25, 
     69, 168,  41, 163, 109, 108, 
    105,  50,   5, 207,  78, 213, 
    170,  65, 184, 134, 148,  46, 
    251,  24, 198,   0,  73, 187, 
    214,  72, 155, 166, 171,  75, 
    183, 233, 154,  58, 113,  31, 
    195,  68, 222, 204,  42, 158, 
     99,  23, 253, 187,  94, 189, 
     63, 163,  99, 126,  61, 161, 
    250, 201, 234,  86,  36, 242, 
    202, 176,  76,  87,  51, 115, 
    138, 164, 112,  53, 171,  71, 
    105,   2, 117, 203,  12, 124, 
     14, 193, 223, 116, 140,  14, 
    225,  82, 162, 160, 100,  74, 
    186, 118,  58, 137,  99,  30, 
    227,  52, 172,  81, 199,  26, 
     19,  39,  21,  14,  34, 165, 
     17, 198, 105,  24, 167,  99, 
    189,  57, 225, 154,  57, 233, 
     90, 185, 164, 171, 231, 224, 
    109, 104,  74,  88, 194,  73, 
     88, 210, 145,  86, 210,  33, 
     11, 158, 241, 217, 146, 141, 
    207, 134, 123, 245,  30, 180, 
    225,  35, 192, 231,  90,  59, 
    112, 237,   8,  83, 220, 240, 
     21, 132, 117, 248,   2, 224, 
    243,  15, 199,  87, 168, 153, 
     89, 144, 117, 236, 193, 119, 
     55, 214, 155, 209,  29, 100, 
     45, 138, 209,  56, 220,  91, 
    120, 212, 111, 188, 140, 242, 
    207, 163, 200, 243, 241,  53, 
    118, 176, 246, 226, 219, 253, 
    194, 199, 103,  42, 122, 255, 
     46, 138, 236, 193, 143, 219, 
    205, 224, 209,  68, 116, 223, 
    221, 142, 114, 115,  32, 143, 
    159,  27, 241, 102,  74,  65, 
    233,  78, 101, 104, 101, 185, 
    188, 124, 115, 168, 178,  82, 
    246,  75, 139, 167, 188,  89, 
    127, 213, 243, 171,  43, 126, 
    121, 169,  20, 120,  55, 230, 
    170, 167, 189, 201, 242, 117, 
    239, 178, 191,  52, 235, 151, 
    189, 241, 225, 241,  98, 213, 
    155, 156,  47,  65,  84, 188, 
    137, 201, 243, 203, 193, 114, 
    249, 194, 194, 172,  63,  86, 
    170, 248,  19, 183, 131,  96, 
    204,  95, 245, 131, 161, 185, 
    160,  88,  28, 154,  15,  42, 
      1, 230, 105,  97,  42, 170, 
    130, 111,  95,   9, 112, 177, 
    180, 176,  68,  63,   1, 176, 
     14,  98,  82,   0,  83,  84, 
     65,  84, 148,   0,   0,   0, 
     36,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,  19,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   8,   0, 
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
