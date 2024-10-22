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
//   float4 scolor2;                    // Offset:   48 Size:    16
//   float4 srctexscale;                // Offset:   64 Size:    16
//   float4 texscale;                   // Offset:   80 Size:    16
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
// First Precompiled Shader at offset:[328]
// Embedded Data:
//  0x00000148 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x00000143 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_constantbuffer cb0[6], immediateIndexed
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.xyzw
dcl_input_ps linear v2.xy
dcl_output o0.xyzw
dcl_temps 3
mov r0.xy, l(0,0,0,0)
mov r0.z, -cb0[0].x
loop 
  lt r0.w, cb0[0].x, r0.z
  breakc_nz r0.w
  add r1.x, r0.z, cb0[1].x
  mov r2.xy, r0.yxyy
  mov r2.z, -cb0[0].y
  loop 
    lt r0.w, cb0[0].y, r2.z
    breakc_nz r0.w
    add r1.y, r2.z, cb0[1].y
    mad r1.zw, r1.xxxy, cb0[5].xxxy, v2.xxxy
    sample_l_indexable(texture2d)(float,float,float,float) r0.w, r1.zwzz, t1.xyzw, s1, l(0.000000)
    add r2.y, r0.w, r2.y
    mad r1.yz, -r1.xxyx, cb0[5].xxyx, v2.xxyx
    sample_l_indexable(texture2d)(float,float,float,float) r0.w, r1.yzyy, t1.xyzw, s1, l(0.000000)
    add r2.x, r0.w, r2.x
    add r2.z, r2.z, l(1.000000)
  endloop 
  mov r0.xy, r2.yxyy
  add r0.z, r0.z, l(1.000000)
endloop 
mul r0.xy, r0.xyxx, cb0[0].wwww
mad r0.xy, -r0.xyxx, cb0[0].zzzz, l(1.000000, 1.000000, 0.000000, 0.000000)
add_sat r0.xy, -r0.yxyy, r0.xyxx
mul r0.zw, v2.xxxy, cb0[4].xxxy
sample_l_indexable(texture2d)(float,float,float,float) r1.xyzw, r0.zwzz, t0.xyzw, s0, l(0.000000)
mov r2.x, cb0[2].w
mov r2.y, cb0[3].w
mul r0.xy, r0.xyxx, r2.xyxx
mul r2.xyzw, r0.yyyy, cb0[3].xyzw
mad r0.xyzw, cb0[2].xyzw, r0.xxxx, r2.xyzw
add r2.x, -r1.w, l(1.000000)
mad r0.xyzw, r0.xyzw, r2.xxxx, r1.xyzw
mov r1.xyz, v1.xyzx
mov r1.w, l(1.000000)
mul r0.xyzw, r0.xyzw, r1.xyzw
mul r0.xyzw, r0.xyzw, v1.wwww
mad o0.xyzw, v0.xyzw, r0.wwww, r0.xyzw
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[328], bundle is:[223] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FSColor2OuterBevel.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask 15, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask  3, param2, paramSlot 2, DefaultVal={0,0,0,0}

codeLenInByte        = 480;Bytes

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
NumVgprs             = 15;
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
  v_mov_b32     v12, 0                                  // 000000000010: 7E180280
  v_mov_b32     v3, 0                                   // 000000000014: 7E060280
  s_load_dwordx8  s[16:23], s[0:1], 0x08                // 000000000018: C0C80108
  s_waitcnt     lgkmcnt(0)                              // 00000000001C: BF8C007F
  v_max_f32     v4, -s12, -s12                          // 000000000020: D2200004 6000180C
label_000A:
  v_mov_b32     v5, s12                                 // 000000000028: 7E0A020C
  v_cmp_gt_f32  vcc, v4, v5                             // 00000000002C: 7C080B04
  s_cbranch_vccnz  label_0030                           // 000000000030: BF870023
  s_buffer_load_dwordx2  s[2:3], s[8:11], 0x04          // 000000000034: C2410904
  s_waitcnt     lgkmcnt(0)                              // 000000000038: BF8C007F
  v_add_f32     v5, s2, v4                              // 00000000003C: 060A0802
  v_max_f32     v6, -s13, -s13                          // 000000000040: D2200006 60001A0D
label_0012:
  v_mov_b32     v7, s13                                 // 000000000048: 7E0E020D
  v_cmp_gt_f32  vcc, v6, v7                             // 00000000004C: 7C080F06
  s_cbranch_vccnz  label_002E                           // 000000000050: BF870019
  s_buffer_load_dwordx2  s[24:25], s[8:11], 0x14        // 000000000054: C24C0914
  v_interp_p1_f32  v7, v0, attr2.x                      // 000000000058: C81C0800
  v_interp_p1_f32  v8, v0, attr2.y                      // 00000000005C: C8200900
  v_interp_p2_f32  v7, v1, attr2.x                      // 000000000060: C81D0801
  v_interp_p2_f32  v8, v1, attr2.y                      // 000000000064: C8210901
  v_add_f32     v9, s3, v6                              // 000000000068: 06120C03
  s_waitcnt     lgkmcnt(0)                              // 00000000006C: BF8C007F
  v_mad_legacy_f32  v10, v5, s24, v7                    // 000000000070: D280000A 041C3105
  v_mad_legacy_f32  v11, v9, s25, v8                    // 000000000078: D280000B 04203309
  v_mad_legacy_f32  v7, -v5, s24, v7                    // 000000000080: D2800007 241C3105
  v_mad_legacy_f32  v8, -v9, s25, v8                    // 000000000088: D2800008 24203309
  image_sample_lz  v[9:12], v[10:11], s[16:23], s[4:7] dmask:0x8 // 000000000090: F09C0800 0024090A
  image_sample_lz  v[7:10], v[7:8], s[16:23], s[4:7] dmask:0x8 // 000000000098: F09C0800 00240707
  s_waitcnt     vmcnt(1) & lgkmcnt(15)                  // 0000000000A0: BF8C0F71
  v_add_f32     v12, v12, v9                            // 0000000000A4: 0618130C
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 0000000000A8: BF8C0F70
  v_add_f32     v3, v3, v7                              // 0000000000AC: 06060F03
  v_add_f32     v6, 1.0, v6                             // 0000000000B0: 060C0CF2
  s_branch      label_0012                              // 0000000000B4: BF82FFE4
label_002E:
  v_add_f32     v4, 1.0, v4                             // 0000000000B8: 060808F2
  s_branch      label_000A                              // 0000000000BC: BF82FFDA
label_0030:
  s_buffer_load_dwordx2  s[2:3], s[8:11], 0x10          // 0000000000C0: C2410910
  v_interp_p1_f32  v4, v0, attr2.x                      // 0000000000C4: C8100800
  v_interp_p1_f32  v5, v0, attr2.y                      // 0000000000C8: C8140900
  s_load_dwordx8  s[16:23], s[0:1], 0x00                // 0000000000CC: C0C80100
  s_load_dwordx4  s[4:7], s[0:1], 0x10                  // 0000000000D0: C0820110
  v_interp_p2_f32  v4, v1, attr2.x                      // 0000000000D4: C8110801
  v_interp_p2_f32  v5, v1, attr2.y                      // 0000000000D8: C8150901
  s_waitcnt     lgkmcnt(0)                              // 0000000000DC: BF8C007F
  v_mul_legacy_f32  v4, s2, v4                          // 0000000000E0: 0E080802
  v_mul_legacy_f32  v5, s3, v5                          // 0000000000E4: 0E0A0A03
  image_sample_lz  v[4:7], v[4:5], s[16:23], s[4:7] dmask:0xf // 0000000000E8: F09C0F00 00240404
  s_buffer_load_dwordx8  s[0:7], s[8:11], 0x08          // 0000000000F0: C2C00908
  v_mul_legacy_f32  v2, s15, v12                        // 0000000000F4: 0E04180F
  v_mul_legacy_f32  v3, s15, v3                         // 0000000000F8: 0E06060F
  v_mad_legacy_f32  v2, -v2, s14, 1.0                   // 0000000000FC: D2800002 23C81D02
  v_mad_legacy_f32  v3, -v3, s14, 1.0                   // 000000000104: D2800003 23C81D03
  v_add_f32     v8, v3, -v2 clamp                       // 00000000010C: D2060808 40020503
  v_add_f32     v2, v2, -v3 clamp                       // 000000000114: D2060802 40020702
  s_waitcnt     lgkmcnt(0)                              // 00000000011C: BF8C007F
  v_mul_legacy_f32  v3, s7, v8                          // 000000000120: 0E061007
  v_mul_legacy_f32  v8, s4, v3                          // 000000000124: 0E100604
  v_mul_legacy_f32  v9, s5, v3                          // 000000000128: 0E120605
  v_mul_legacy_f32  v10, s6, v3                         // 00000000012C: 0E140606
  v_interp_p1_f32  v12, v0, attr1.x                     // 000000000130: C8300400
  v_mul_legacy_f32  v2, s3, v2                          // 000000000134: 0E040403
  v_mul_legacy_f32  v3, s7, v3                          // 000000000138: 0E060607
  v_interp_p1_f32  v13, v0, attr1.y                     // 00000000013C: C8340500
  v_mac_legacy_f32  v8, s0, v2                          // 000000000140: 0C100400
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000144: BF8C0F70
  v_sub_f32     v11, 1.0, v7                            // 000000000148: 08160EF2
  v_interp_p1_f32  v14, v0, attr1.z                     // 00000000014C: C8380600
  v_mac_legacy_f32  v3, s3, v2                          // 000000000150: 0C060403
  v_interp_p2_f32  v12, v1, attr1.x                     // 000000000154: C8310401
  v_mac_legacy_f32  v9, s1, v2                          // 000000000158: 0C120401
  v_mac_legacy_f32  v4, v8, v11                         // 00000000015C: 0C081708
  v_interp_p2_f32  v13, v1, attr1.y                     // 000000000160: C8350501
  v_mac_legacy_f32  v7, v3, v11                         // 000000000164: 0C0E1703
  v_mac_legacy_f32  v10, s2, v2                         // 000000000168: 0C140402
  v_mac_legacy_f32  v5, v9, v11                         // 00000000016C: 0C0A1709
  v_interp_p2_f32  v14, v1, attr1.z                     // 000000000170: C8390601
  v_mul_legacy_f32  v3, v4, v12                         // 000000000174: 0E061904
  v_mac_legacy_f32  v6, v10, v11                        // 000000000178: 0C0C170A
  v_interp_p1_f32  v2, v0, attr1.w                      // 00000000017C: C8080700
  v_mul_legacy_f32  v4, v5, v13                         // 000000000180: 0E081B05
  v_mul_legacy_f32  v5, v6, v14                         // 000000000184: 0E0A1D06
  v_interp_p2_f32  v2, v1, attr1.w                      // 000000000188: C8090701
  v_interp_p1_f32  v6, v0, attr0.x                      // 00000000018C: C8180000
  v_interp_p1_f32  v8, v0, attr0.y                      // 000000000190: C8200100
  v_interp_p1_f32  v9, v0, attr0.z                      // 000000000194: C8240200
  v_interp_p1_f32  v0, v0, attr0.w                      // 000000000198: C8000300
  v_mul_legacy_f32  v3, v3, v2                          // 00000000019C: 0E060503
  v_mul_legacy_f32  v4, v4, v2                          // 0000000001A0: 0E080504
  v_mul_legacy_f32  v5, v5, v2                          // 0000000001A4: 0E0A0505
  v_interp_p2_f32  v6, v1, attr0.x                      // 0000000001A8: C8190001
  v_interp_p2_f32  v8, v1, attr0.y                      // 0000000001AC: C8210101
  v_interp_p2_f32  v9, v1, attr0.z                      // 0000000001B0: C8250201
  v_mul_legacy_f32  v2, v7, v2                          // 0000000001B4: 0E040507
  v_interp_p2_f32  v0, v1, attr0.w                      // 0000000001B8: C8010301
  v_mac_legacy_f32  v3, v6, v2                          // 0000000001BC: 0C060506
  v_mac_legacy_f32  v4, v8, v2                          // 0000000001C0: 0C080508
  v_mac_legacy_f32  v5, v9, v2                          // 0000000001C4: 0C0A0509
  v_mac_legacy_f32  v2, v0, v2                          // 0000000001C8: 0C040500
  v_cvt_pkrtz_f16_f32  v0, v3, v4                       // 0000000001CC: 5E000903
  v_cvt_pkrtz_f16_f32  v1, v5, v2                       // 0000000001D0: 5E020505
  exp           mrt0, v0, v0, v1, v1 compr vm           // 0000000001D4: F800140F 00000100
  s_endpgm                                              // 0000000001DC: BF810000
end


// Approximately 41 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FSColor2OuterBevel[] =
{
     68,  88,  66,  67, 149, 141, 
    191, 148,  39, 112, 146, 201, 
     39,  62, 254, 224, 182,  84, 
    233, 119,   1,   0,   0,   0, 
    204,  12,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    236,   2,   0,   0,  80,   3, 
      0,   0, 132,   3,   0,   0, 
     48,  12,   0,   0,  82,  68, 
     69,  70, 176,   2,   0,   0, 
      1,   0,   0,   0,  12,   1, 
      0,   0,   5,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
    115,   2,   0,   0,  82,  68, 
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
      0,   0,   6,   0,   0,   0, 
     36,   1,   0,   0,  96,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  20,   2, 
      0,   0,   0,   0,   0,   0, 
     16,   0,   0,   0,   2,   0, 
      0,   0,  36,   2,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,  72,   2,   0,   0, 
     16,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
     36,   2,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
     79,   2,   0,   0,  32,   0, 
      0,   0,  16,   0,   0,   0, 
      2,   0,   0,   0,  36,   2, 
      0,   0,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,  86,   2, 
      0,   0,  48,   0,   0,   0, 
     16,   0,   0,   0,   2,   0, 
      0,   0,  36,   2,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,  94,   2,   0,   0, 
     64,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
     36,   2,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    106,   2,   0,   0,  80,   0, 
      0,   0,  16,   0,   0,   0, 
      2,   0,   0,   0,  36,   2, 
      0,   0,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 102, 115, 
    105, 122, 101,   0, 102, 108, 
    111,  97, 116,  52,   0, 171, 
    171, 171,   1,   0,   3,   0, 
      1,   0,   4,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  26,   2, 
      0,   0, 111, 102, 102, 115, 
    101, 116,   0, 115,  99, 111, 
    108, 111, 114,   0, 115,  99, 
    111, 108, 111, 114,  50,   0, 
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
     83,  72,  69,  88, 164,   8, 
      0,   0,  80,   0,   0,   0, 
     41,   2,   0,   0,  53,  16, 
      0,   0,   5,   0,   0,   0, 
     72,   1,   0,   0,  80,   0, 
      0,   0,  67,   1,   0,   0, 
    106,   8,   0,   1,  89,   0, 
      0,   4,  70, 142,  32,   0, 
      0,   0,   0,   0,   6,   0, 
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
     54,   0,   0,   8,  50,   0, 
     16,   0,   0,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  54,   0,   0,   7, 
     66,   0,  16,   0,   0,   0, 
      0,   0,  10, 128,  32, 128, 
     65,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     48,   0,   0,   1,  49,   0, 
      0,   8, 130,   0,  16,   0, 
      0,   0,   0,   0,  10, 128, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  42,   0, 
     16,   0,   0,   0,   0,   0, 
      3,   0,   4,   3,  58,   0, 
     16,   0,   0,   0,   0,   0, 
      0,   0,   0,   8,  18,   0, 
     16,   0,   1,   0,   0,   0, 
     42,   0,  16,   0,   0,   0, 
      0,   0,  10, 128,  32,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  54,   0,   0,   5, 
     50,   0,  16,   0,   2,   0, 
      0,   0,  22,   5,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   7,  66,   0,  16,   0, 
      2,   0,   0,   0,  26, 128, 
     32, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  48,   0,   0,   1, 
     49,   0,   0,   8, 130,   0, 
     16,   0,   0,   0,   0,   0, 
     26, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     42,   0,  16,   0,   2,   0, 
      0,   0,   3,   0,   4,   3, 
     58,   0,  16,   0,   0,   0, 
      0,   0,   0,   0,   0,   8, 
     34,   0,  16,   0,   1,   0, 
      0,   0,  42,   0,  16,   0, 
      2,   0,   0,   0,  26, 128, 
     32,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,  50,   0, 
      0,  10, 194,   0,  16,   0, 
      1,   0,   0,   0,   6,   4, 
     16,   0,   1,   0,   0,   0, 
      6, 132,  32,   0,   0,   0, 
      0,   0,   5,   0,   0,   0, 
      6,  20,  16,   0,   2,   0, 
      0,   0,  72,   0,   0, 141, 
    194,   0,   0, 128,  67,  85, 
     21,   0, 130,   0,  16,   0, 
      0,   0,   0,   0, 230,  10, 
     16,   0,   1,   0,   0,   0, 
     70, 126,  16,   0,   1,   0, 
      0,   0,   0,  96,  16,   0, 
      1,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   7,  34,   0, 
     16,   0,   2,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,  26,   0,  16,   0, 
      2,   0,   0,   0,  50,   0, 
      0,  11,  98,   0,  16,   0, 
      1,   0,   0,   0,   6,   1, 
     16, 128,  65,   0,   0,   0, 
      1,   0,   0,   0,   6, 129, 
     32,   0,   0,   0,   0,   0, 
      5,   0,   0,   0,   6,  17, 
     16,   0,   2,   0,   0,   0, 
     72,   0,   0, 141, 194,   0, 
      0, 128,  67,  85,  21,   0, 
    130,   0,  16,   0,   0,   0, 
      0,   0, 150,   5,  16,   0, 
      1,   0,   0,   0,  70, 126, 
     16,   0,   1,   0,   0,   0, 
      0,  96,  16,   0,   1,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   7,  18,   0,  16,   0, 
      2,   0,   0,   0,  58,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   2,   0, 
      0,   0,   0,   0,   0,   7, 
     66,   0,  16,   0,   2,   0, 
      0,   0,  42,   0,  16,   0, 
      2,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     22,   0,   0,   1,  54,   0, 
      0,   5,  50,   0,  16,   0, 
      0,   0,   0,   0,  22,   5, 
     16,   0,   2,   0,   0,   0, 
      0,   0,   0,   7,  66,   0, 
     16,   0,   0,   0,   0,   0, 
     42,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128,  63,  22,   0, 
      0,   1,  56,   0,   0,   8, 
     50,   0,  16,   0,   0,   0, 
      0,   0,  70,   0,  16,   0, 
      0,   0,   0,   0, 246, 143, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  50,   0, 
      0,  14,  50,   0,  16,   0, 
      0,   0,   0,   0,  70,   0, 
     16, 128,  65,   0,   0,   0, 
      0,   0,   0,   0, 166, 138, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   2,  64, 
      0,   0,   0,   0, 128,  63, 
      0,   0, 128,  63,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,  32,   0,   8,  50,   0, 
     16,   0,   0,   0,   0,   0, 
     22,   5,  16, 128,  65,   0, 
      0,   0,   0,   0,   0,   0, 
     70,   0,  16,   0,   0,   0, 
      0,   0,  56,   0,   0,   8, 
    194,   0,  16,   0,   0,   0, 
      0,   0,   6,  20,  16,   0, 
      2,   0,   0,   0,   6, 132, 
     32,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,  72,   0, 
      0, 141, 194,   0,   0, 128, 
     67,  85,  21,   0, 242,   0, 
     16,   0,   1,   0,   0,   0, 
    230,  10,  16,   0,   0,   0, 
      0,   0,  70, 126,  16,   0, 
      0,   0,   0,   0,   0,  96, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,  54,   0,   0,   6, 
     18,   0,  16,   0,   2,   0, 
      0,   0,  58, 128,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,  54,   0,   0,   6, 
     34,   0,  16,   0,   2,   0, 
      0,   0,  58, 128,  32,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,  56,   0,   0,   7, 
     50,   0,  16,   0,   0,   0, 
      0,   0,  70,   0,  16,   0, 
      0,   0,   0,   0,  70,   0, 
     16,   0,   2,   0,   0,   0, 
     56,   0,   0,   8, 242,   0, 
     16,   0,   2,   0,   0,   0, 
     86,   5,  16,   0,   0,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,  50,   0,   0,  10, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,   6,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   2,   0,   0,   0, 
      0,   0,   0,   8,  18,   0, 
     16,   0,   2,   0,   0,   0, 
     58,   0,  16, 128,  65,   0, 
      0,   0,   1,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  50,   0,   0,   9, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,   6,   0, 
     16,   0,   2,   0,   0,   0, 
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
     53,  16,   0,   0, 225,   0, 
      0,   0,  21,   0,   1,   0, 
     41, 217,  11,   0, 120,   1, 
    237,  86, 207, 111,  27,  69, 
     20, 126,  51,  59,  51,  59, 
    187, 153, 174,  77,  48, 196, 
     41,  78, 113,  21,  23,  33, 
     85, 138, 234,  54,  72, 192, 
     41,  64, 129,  11,   2, 137, 
     92, 170, 106,  21, 217, 161, 
     14, 129,  58,   9, 216, 161, 
     10, 168,  50, 161,   7, 224, 
    208,   3,   7, 142,  61,  35, 
     33, 241,   7, 112,  64,  56, 
    248,  52,  71, 148,  91,  57, 
     21, 137,  63, 160, 202, 169, 
      7, 164, 240, 189, 253, 129, 
    124, 160, 183,  72,  32, 145, 
     79, 254, 230, 237,  55, 111, 
    223, 143, 157, 177,  52, 115, 
    127, 134,  50, 108, 157, 187, 
    191, 206, 246, 248, 184, 251, 
    186, 211,  68, 248, 145,   0, 
    217,  50, 246,  44, 143,  68, 
     63, 198,  60,  18,  61, 207, 
      3, 191,  32, 137,  42,  48, 
    103, 193,   7, 208,  43, 176, 
    236,  43, 153, 195, 255, 114, 
     17, 190, 231, 240, 116,  14, 
    140, 145, 244,  38, 108,  10, 
     38, 224, 219,  96,  29, 196, 
     43,  25,   2,  30,  10, 176, 
    159,  81, 250,  74,  40, 144, 
    243, 113,  59, 104,  33, 139, 
    143,  64,   6, 251, 120, 238, 
    111,  92, 224, 225, 159, 241, 
     33, 200, 253,  63,  14, 220, 
     91, 217,  15, 247, 192, 117, 
     24, 252, 189, 255,  54, 242, 
     53, 201, 191, 148, 123, 100, 
     93, 238,  19, 131, 247,  99, 
     26, 188, 150, 229, 212, 181, 
     98,  31, 203, 253, 221,  44, 
    116, 185, 151,   7, 224, 227, 
    112,  12,  52, 248,  97,  10, 
    165, 230,  62,  76, 177, 162, 
     70,  60,  75,  77, 182, 114, 
     49, 251,  95, 252,  95, 192, 
    255,  63,   6, 175,   5, 239, 
     78,  21, 204,  87,  36,   7, 
    251, 135,  32, 209, 254, 152, 
    249,  77, 157,  30, 177,  42, 
    181, 128, 230, 216,  82,   7, 
    172, 179, 164,  83,  26, 137, 
    131, 136, 214, 180, 150, 107, 
    149,   6, 226, 177, 177, 208, 
     41, 116,  58, 165, 175,  67, 
     95, 159, 210,  29, 232, 206, 
    148,  94, 135,  94,  47, 117, 
    153, 191, 146, 213, 211, 146, 
    247,  25, 147, 163, 255,  50, 
     79, 123,  60,  25, 158, 246, 
    120,  50,  60, 237, 241, 196, 
    152,  65,   6, 127, 254,  92, 
     19, 147, 131, 207, 233, 238, 
    152, 162,  47,  39, 251, 178, 
     62, 218, 151, 102, 100, 133, 
    207, 230,  20,  53,  15,  93, 
    157,  58,  78, 198,  35,  53, 
     99, 111,  47, 210,  87,  99, 
     21, 189,  50,  97, 159, 180, 
    177,  49, 240, 159,  57,  75, 
    157,  51,  50,  25, 153, 138, 
    189,  61,  15, 127,  45, 122, 
    107,  66, 182, 225,  41, 106, 
    122,  97,  23, 188, 136, 206, 
    251, 192, 205,  26, 142, 137, 
    105, 255,  80, 183,  27, 106, 
      6,  54, 186, 210,  84,  97, 
    174,  91,  54, 215,  45, 178, 
    247,  30, 198,  81, 139, 216, 
    134,  97, 139,  62, 174, 220, 
     29, 187,  39, 235, 230,  35, 
    216, 160,  98, 204, 145, 115, 
    230, 143, 227,  59, 227,  35, 
    107, 205, 111, 176,  85, 244, 
     66, 182, 138,  90,  53,  79, 
    232, 185,  42, 238,  28,   8, 
    251,   4, 106,  62, 229, 243, 
     30, 109,  18, 196, 113,  66, 
    149, 123,  15, 149,  66, 250, 
    232,  96,  82, 169, 171,   4, 
    169,  18, 137, 154, 114, 193, 
     47,   6, 176,   1,  44,  82, 
     30,   6,  90, 174,  72,  88, 
     25, 202,  21, 142,  15, 171, 
     38,  81, 166, 154, 104,  51, 
    155,  24,  83,  75,  72,  93, 
    242, 129,  82,  73, 136, 120, 
    210, 203, 158,  84, 213, 113, 
    111,  71, 201, 211, 150, 204, 
    139, 240,  25,  39,  84, 219, 
     11,  53, 235, 236, 156, 117, 
     66, 191, 224, 131, 185, 196, 
     73,  85, 115, 209,  92, 236, 
    132, 121, 201, 171, 121, 147, 
    196, 115, 206,  81, 104, 189, 
    126, 198,  38, 102,  33,  78, 
     68,  24, 121, 220, 126, 240, 
     13,  77,  79, 178, 229, 113, 
    184, 249,  64, 163, 182, 182, 
    137, 214, 240, 211, 188,  23, 
    226, 188,  23, 242, 130,  15, 
    181,  74,  68,  32, 188, 209, 
    198,  89, 109,  93, 164,  99, 
     71,  90,  57, 156,  55, 249, 
    121,  85,  43, 207, 155,  47, 
    198, 243,  24, 179,  71, 128, 
    207, 202, 105, 224,  42, 147, 
    185, 120, 224, 107,  13, 235, 
    108,   2, 224,  51,  21,  71, 
     98, 166, 153,  53, 144, 117, 
    153, 131, 239, 105, 172,  31, 
     20, 247, 154,  95,  11,  91, 
    196, 225,  46, 196,  81,  83, 
     40, 238, 193, 143, 138, 247, 
    184,  30,  79, 253, 254, 245, 
    167, 151, 190, 247, 223, 217, 
     70, 225, 175,  23, 150, 115, 
    240, 171,  63,  93, 124, 115, 
    237, 219,  98, 110, 185, 176, 
     29, 144, 227,  55,  94,  78, 
    215, 187, 253, 238, 103, 195, 
    165, 221, 157, 193, 206, 205, 
    165, 225, 238, 160, 215, 221, 
    186, 156, 222, 232, 221,  74, 
    123, 123, 187, 189, 193, 118, 
    183, 159, 190, 191, 177, 183, 
    156, 174,  14, 222,  75, 223, 
    237, 109, 223, 232,  13, 210, 
    171,  87, 174, 182, 247, 210, 
    213, 205,  46, 196,  48, 125, 
     99, 245, 181, 157, 254, 206, 
    224, 242,  59, 159, 224, 245, 
     87, 123, 183, 122, 253, 165, 
    141, 126, 187, 189, 180, 217, 
     31, 246,  81,  33, 199,  15, 
     69,  93, 190, 203, 133, 224, 
     86, 247, 131, 109, 250,  11, 
     72,  36, 108, 229,   0,   0, 
     83,  84,  65,  84, 148,   0, 
      0,   0,  41,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
     22,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      9,   0,   0,   0,   0,   0, 
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
