; MB86234 TGP program — srallycb
; source: workram 0x5F9E94 (ROM mirror maincpu+(va-0x59F000))
; words: 1665 (count @ 0x5FB898)
; ISA: MAME mb86233d (reference)
;
; Vectors: 0→#0x10 reset; 1..5 irq stubs; 4→#0xa
; FIFO dispatch: validate opcode @0x70; table base 0xAF+opcode (bsul)
;   0x52 → 0x101 → handler 0x58A (road query / fill slot $68+$103)
;   0x53 → 0x102 → handler 0x63C (16 floats + flag; bit7 in flag>>16)
; See decomp/src/tgp/firmware/README.md

0000: bf600010  brif alw #0x10
0001: bf600006  brif alw #0x6
0002: bf600006  brif alw #0x6
0003: bf600006  brif alw #0x6
0004: bf60000a  brif alw #0xa
0005: bf600006  brif alw #0x6
0006: 3c02105c  clr1 #0x105c
0007: 7c000002  ldi #0x2, mask
0008: 3c060800  set #0x0800
0009: bf6e0000  iret
000a: 3c02105c  clr1 #0x105c
000b: 1c0cfc7e  mov $126, $638
000c: 1c10fe7e  mov $638, $127
000d: 1c1dc07f  mov $127, rf0
000e: 3c060800  set #0x0800
000f: bf6e0000  iret
0010: 3c060800  set #0x0800
0011: 7c000002  ldi #0x2, mask
0012: 360a0007  stmh fp rz
0013: 3b000000  lid #0x0
0014: 1c1c322d  mov d, $45
0015: 63000000  ldi #0x0, rf3
0016: 41000200  ldi #0x200, b1
0017: 43000000  ldi #0x0, x1
0018: 3b000000  lid #0x0
0019: 1c1c3200  mov d, $0
001a: 3b000001  lid #0x1
001b: 1c1c3201  mov d, $1
001c: 3bffffff  lid #0xffffff
001d: 1c1c3202  mov d, $2
001e: 3b000000  lid #0x0
001f: 1c1c3203  mov d, $3
0020: 3c000010  clr0 d
0021: 5a00007f  ldi #0x7f, dh
0022: 1c1c3204  mov d, $4
0023: 3c000010  clr0 d
0024: 5a00007f  ldi #0x7f, dh
0025: 5b800000  ldi #0x800000, dl
0026: 1c1c3205  mov d, $5
0027: 3b000130  lid #0x130
0028: 1c1c3207  mov d, $7
0029: 1c1c3208  mov d, $8
002a: 3b000000  lid #0x0
002b: 1c1c3209  mov d, $9
002c: 3b000007  lid #0x7
002d: 1c1c320a  mov d, $10
002e: 3b000008  lid #0x8
002f: 1c1c320b  mov d, $11
0030: 3b000100  lid #0x100
0031: 1c1c320c  mov d, $12
0032: 3b000200  lid #0x200
0033: 1c1c320d  mov d, $13
0034: 3b000040  lid #0x40
0035: 1c1c320e  mov d, $14
0036: 3b00003f  lid #0x3f
0037: 1c1c320f  mov d, $15
0038: 5a00007f  ldi #0x7f, dh
0039: 5b490fdb  ldi #0x490fdb, dl
003a: 1c1c323b  mov d, $59
003b: 5a000080  ldi #0x80, dh
003c: 5b490fdb  ldi #0x490fdb, dl
003d: 1c1c3206  mov d, $6
003e: 5a00007d  ldi #0x7d, dh
003f: 5b22f983  ldi #0x22f983, dl
0040: 1c1c323c  mov d, $60
0041: 43000220  ldi #0x220, x1
0042: 5a00007e  ldi #0x7e, dh
0043: 5b000000  ldi #0x0, dl
0044: 1c1c33a1  mov d, (x1+1)
0045: 5a00007a  ldi #0x7a, dh
0046: 5b2aaaab  ldi #0x2aaaab, dl
0047: 1c1c33a1  mov d, (x1+1)
0048: 5a000075  ldi #0x75, dh
0049: 5b360b61  ldi #0x360b61, dl
004a: 1c1c33a1  mov d, (x1+1)
004b: 5a00006f  ldi #0x6f, dh
004c: 5b500d01  ldi #0x500d01, dl
004d: 1c1c33a1  mov d, (x1+1)
004e: 5a000069  ldi #0x69, dh
004f: 5b13f27e  ldi #0x13f27e, dl
0050: 1c1c33a1  mov d, (x1+1)
0051: 5a000062  ldi #0x62, dh
0052: 5b0f76c7  ldi #0xf76c7, dl
0053: 1c1c33a1  mov d, (x1+1)
0054: 4300023a  ldi #0x23a, x1
0055: 5a00007e  ldi #0x7e, dh
0056: 5b1714ba  ldi #0x1714ba, dl
0057: 1c1c33a1  mov d, (x1+1)
0058: 5a00007d  ldi #0x7d, dh
0059: 5b55a9a8  ldi #0x55a9a8, dl
005a: 1c1c33a1  mov d, (x1+1)
005b: 5a00007e  ldi #0x7e, dh
005c: 5b3504f4  ldi #0x3504f4, dl
005d: 1c1c33a1  mov d, (x1+1)
005e: 3b006000  lid #0x6000
005f: 1c1c327d  mov d, $125
0060: 1c0c0000  mov $0, $512
0061: 1c0c0201  mov $1, $513
0062: 1c0c0402  mov $2, $514
0063: 1c0c0603  mov $3, $515
0064: 1c0c0804  mov $4, $516
0065: 1c0c0a05  mov $5, $517
0066: 43000206  ldi #0x206, x1
0067: 3c000050  clr0 d, ?6
0068: 1c1c327c  mov d, $124
0069: 5a00007b  ldi #0x7b, dh
006a: 5bcccccd  ldi #0xcccccd, dl
006b: 1c1c33a1  mov d, (x1+1)
006c: 40000000  ldi #0x0, b0
006d: 41000200  ldi #0x200, b1
006e: 1c1dc000  mov $0, rf0
006f: 1c10fc01  mov $513, $126
0070: 1c1f2621  mov rf1, b
0071: 1c1f3214  mov bh, d
0072: 3900007f  lia #0x7f
0073: 1c3da00b  andd : mov $11, a
0074: 1f7f4410  subd : mov a, rf2
0075: fe000070  brif !zrd #0x70
0076: bf640282  bsif alw #0x282
0077: 1c1f2621  mov rf1, b
0078: 3c000010  clr0 d
0079: 1c1f3215  mov bl, d
007a: 3900007f  lia #0x7f
007b: 1c3f2014  andd : mov bh, a
007c: 1f7f3215  subd : mov bl, d
007d: be000090  brif zrd #0x90
007e: bf6400a1  bsif alw #0xa1
007f: 5f000008  ldi #0x8, sft
0080: 1edf1e10  lsrd
0081: 1f7f3215  subd : mov bl, d
0082: be000098  brif zrd #0x98
0083: bf6400a1  bsif alw #0xa1
0084: 3900007f  lia #0x7f
0085: 1c3f1e10  andd
0086: 1c1f2019  mov d, a
0087: 1c1f3215  mov bl, d
0088: 1edf1e10  lsrd
0089: 1f7f1e10  subd
008a: be000098  brif zrd #0x98
008b: bf6400a1  bsif alw #0xa1
008c: 3c000004  clr0 a
008d: bf640130  bsif alw #0x130
008e: 1c1f2015  mov bl, a
008f: bf600099  brif alw #0x99
0090: 3b0000af  lid #0xaf
0091: 1f5db800  addd : mov $0, p
0092: bf664019  bsul alw d
0093: 1c1db265  mov $101, d
0094: 1c1da001  mov $1, a
0095: 1f5f1e10  addd
0096: 1c1c3265  mov d, $101
0097: bf600077  brif alw #0x77
0098: bf640130  bsif alw #0x130
0099: 3b0000af  lid #0xaf
009a: 1f5db800  addd : mov $0, p
009b: bf664019  bsul alw d
009c: 1c1db265  mov $101, d
009d: 1c1da001  mov $1, a
009e: 1f5f1e10  addd
009f: 1c1c3265  mov d, $101
00a0: bf600077  brif alw #0x77
00a1: 1c1c2034  mov a, $52
00a2: 1c1c3235  mov d, $53
00a3: 1c1db27f  mov $127, d
00a4: 39000002  lia #0x2
00a5: 1c5f4019  orad : mov d, rf0
00a6: 1c1c327f  mov d, $127
00a7: 43007ffe  ldi #0x7ffe, x1
00a8: 63400000  ldi #0x400000, rf3
00a9: 1c1ca7a1  mov b, (x1+1) (e)
00aa: 1c034201  mov {0} $1, (x1+1) (e)
00ab: 63000000  ldi #0x0, rf3
00ac: 1c1db235  mov $53, d
00ad: 1c1da034  mov $52, a
00ae: bf6a0000  rtif alw
00af: bf60012f  brif alw #0x12f
00b0: bf60013d  brif alw #0x13d
00b1: bf60013f  brif alw #0x13f
00b2: bf60015e  brif alw #0x15e
00b3: bf600168  brif alw #0x168
00b4: bf60014e  brif alw #0x14e
00b5: bf60016a  brif alw #0x16a
00b6: bf60016d  brif alw #0x16d
00b7: bf600178  brif alw #0x178
00b8: bf60017d  brif alw #0x17d
00b9: bf600182  brif alw #0x182
00ba: bf600187  brif alw #0x187
00bb: bf60018c  brif alw #0x18c
00bc: bf60019d  brif alw #0x19d
00bd: bf6001a1  brif alw #0x1a1
00be: bf6001a5  brif alw #0x1a5
00bf: bf6001a6  brif alw #0x1a6
00c0: bf6001a7  brif alw #0x1a7
00c1: bf6001cb  brif alw #0x1cb
00c2: bf6001d0  brif alw #0x1d0
00c3: bf6001cc  brif alw #0x1cc
00c4: bf6001d4  brif alw #0x1d4
00c5: bf6001d8  brif alw #0x1d8
00c6: bf6001dc  brif alw #0x1dc
00c7: bf60021d  brif alw #0x21d
00c8: bf60021e  brif alw #0x21e
00c9: bf60021f  brif alw #0x21f
00ca: bf600220  brif alw #0x220
00cb: bf600221  brif alw #0x221
00cc: bf600222  brif alw #0x222
00cd: bf600223  brif alw #0x223
00ce: bf60012f  brif alw #0x12f
00cf: bf60025b  brif alw #0x25b
00d0: bf60026f  brif alw #0x26f
00d1: bf60027e  brif alw #0x27e
00d2: bf600282  brif alw #0x282
00d3: bf600286  brif alw #0x286
00d4: bf600327  brif alw #0x327
00d5: bf60032e  brif alw #0x32e
00d6: bf600332  brif alw #0x332
00d7: bf600348  brif alw #0x348
00d8: bf600363  brif alw #0x363
00d9: bf600384  brif alw #0x384
00da: bf6003a4  brif alw #0x3a4
00db: bf6003c2  brif alw #0x3c2
00dc: bf6003c0  brif alw #0x3c0
00dd: bf6002cc  brif alw #0x2cc
00de: bf60030e  brif alw #0x30e
00df: bf60012f  brif alw #0x12f
00e0: bf600224  brif alw #0x224
00e1: bf60035f  brif alw #0x35f
00e2: bf600380  brif alw #0x380
00e3: bf6003a0  brif alw #0x3a0
00e4: bf600225  brif alw #0x225
00e5: bf600228  brif alw #0x228
00e6: bf60012f  brif alw #0x12f
00e7: bf60012f  brif alw #0x12f
00e8: bf60012f  brif alw #0x12f
00e9: bf60012f  brif alw #0x12f
00ea: bf60012f  brif alw #0x12f
00eb: bf60012f  brif alw #0x12f
00ec: bf60012f  brif alw #0x12f
00ed: bf60022b  brif alw #0x22b
00ee: bf60012f  brif alw #0x12f
00ef: bf6003f4  brif alw #0x3f4
00f0: bf6003fc  brif alw #0x3fc
00f1: bf600404  brif alw #0x404
00f2: bf60040c  brif alw #0x40c
00f3: bf600414  brif alw #0x414
00f4: bf600422  brif alw #0x422
00f5: bf600430  brif alw #0x430
00f6: bf60043e  brif alw #0x43e
00f7: bf60044c  brif alw #0x44c
00f8: bf60012f  brif alw #0x12f
00f9: bf60012f  brif alw #0x12f
00fa: bf60012f  brif alw #0x12f
00fb: bf60012f  brif alw #0x12f
00fc: bf60012f  brif alw #0x12f
00fd: bf60012f  brif alw #0x12f
00fe: bf60012f  brif alw #0x12f
00ff: bf60046c  brif alw #0x46c
0100: bf60048e  brif alw #0x48e
0101: bf60058a  brif alw #0x58a
0102: bf60063c  brif alw #0x63c
0103: bf6004b5  brif alw #0x4b5
0104: bf60066e  brif alw #0x66e
0105: bf60050e  brif alw #0x50e
0106: bf600515  brif alw #0x515
0107: bf60051c  brif alw #0x51c
0108: bf600530  brif alw #0x530
0109: bf60053a  brif alw #0x53a
010a: bf60055f  brif alw #0x55f
010b: bf600577  brif alw #0x577
010c: bf600674  brif alw #0x674
010d: bf600679  brif alw #0x679
010e: bf60067b  brif alw #0x67b
010f: bf6003db  brif alw #0x3db
0110: bf6003e9  brif alw #0x3e9
0111: bf60012f  brif alw #0x12f
0112: bf60012f  brif alw #0x12f
0113: bf60012f  brif alw #0x12f
0114: bf60012f  brif alw #0x12f
0115: bf60012f  brif alw #0x12f
0116: bf60012f  brif alw #0x12f
0117: bf60012f  brif alw #0x12f
0118: bf60012f  brif alw #0x12f
0119: bf60012f  brif alw #0x12f
011a: bf60012f  brif alw #0x12f
011b: bf60012f  brif alw #0x12f
011c: bf60012f  brif alw #0x12f
011d: bf60012f  brif alw #0x12f
011e: bf60012f  brif alw #0x12f
011f: bf60012f  brif alw #0x12f
0120: bf60012f  brif alw #0x12f
0121: bf60012f  brif alw #0x12f
0122: bf60012f  brif alw #0x12f
0123: bf60012f  brif alw #0x12f
0124: bf60012f  brif alw #0x12f
0125: bf60012f  brif alw #0x12f
0126: bf60012f  brif alw #0x12f
0127: bf60012f  brif alw #0x12f
0128: bf60012f  brif alw #0x12f
0129: bf60012f  brif alw #0x12f
012a: bf60012f  brif alw #0x12f
012b: bf60012f  brif alw #0x12f
012c: bf60012f  brif alw #0x12f
012d: bf60012f  brif alw #0x12f
012e: bf60012f  brif alw #0x12f
012f: bf6a0000  rtif alw
0130: 1c1c2034  mov a, $52
0131: 3b007f80  lid #0x7f80
0132: 1f5da001  addd : mov $1, a
0133: 1c1f0619  mov d, x1
0134: 1c1db27c  mov $124, d
0135: 1f5f1e10  addd
0136: 1c1c327c  mov d, $124
0137: 63400000  ldi #0x400000, rf3
0138: 1c1e33a0  mov (x1) (e), d
0139: 1f5f1e10  addd
013a: 1c1cb3a0  mov d, (x1) (e)
013b: 1c1da034  mov $52, a
013c: bf6a0000  rtif alw
013d: 1c10f800  mov $512, $124
013e: bf6a0000  rtif alw
013f: 1c1dc47c  mov $124, rf2
0140: bf6a0000  rtif alw
0141: 1c1f3221  mov rf1, d
0142: 5f000002  ldi #0x2, sft
0143: 3d600008  lsrd : clr0 b
0144: 1c1f0619  mov d, x1
0145: 63400000  ldi #0x400000, rf3
0146: 54000001  ldi #0x1, bh
0147: 1c1ca7a1  mov b, (x1+1) (e)
0148: 1c1cc3a1  mov rf1, (x1+1) (e)
0149: 1c1cc3a1  mov rf1, (x1+1) (e)
014a: 1c1cc3a1  mov rf1, (x1+1) (e)
014b: 1c1cc3a1  mov rf1, (x1+1) (e)
014c: 63000000  ldi #0x0, rf3
014d: bf6a0000  rtif alw
014e: 63400000  ldi #0x400000, rf3
014f: 1c1d867d  mov $125, x1
0150: 1c1e33a1  mov (x1+1) (e), d
0151: 1c1c067d  mov x1, $125
0152: 5f000002  ldi #0x2, sft
0153: 3d600008  lsrd : clr0 b
0154: 1c1f0619  mov d, x1
0155: 5400000b  ldi #0xb, bh
0156: 1c1da007  mov $7, a
0157: 59000200  ldi #0x200, d
0158: 1f5ca7a1  addd : mov b, (x1+1) (e)
0159: 1c1f0419  mov d, x0
015a: 3c04000c  rep #12
015b: 1c0743a1  mov (x0+1), (x1+1) (e)
015c: 63000000  ldi #0x0, rf3
015d: bf6a0000  rtif alw
015e: 1c1c4265  mov rf1, $101
015f: 39006000  lia #0x6000
0160: 1c1c207d  mov a, $125
0161: 1c1c2034  mov a, $52
0162: 1c1c3235  mov d, $53
0163: 1c1db27f  mov $127, d
0164: 39fffffd  lia #0xfffffd
0165: 1c3f4019  andd : mov d, rf0
0166: 1c1c327f  mov d, $127
0167: bf6a0000  rtif alw
0168: 1c1dc465  mov $101, rf2
0169: bf6a0000  rtif alw
016a: 1c1f0421  mov rf1, x0
016b: 1c1ec5a1  mov (x0+1) (o), rf2
016c: bf6a0000  rtif alw
016d: 63400000  ldi #0x400000, rf3
016e: 1c1d867d  mov $125, x1
016f: 1c1e33a1  mov (x1+1) (e), d
0170: 1c1c067d  mov x1, $125
0171: 39800000  lia #0x800000
0172: 1f5f1e10  addd
0173: 1c1f4619  mov d, rf3
0174: 1c1f0619  mov d, x1
0175: 1c1e45a1  mov (x1+1) (e), rf2
0176: 63000000  ldi #0x0, rf3
0177: bf6a0000  rtif alw
0178: 1c1f3221  mov rf1, d
0179: 1c1f2021  mov rf1, a
017a: 1cdf1e10  fadd
017b: 1c1f4419  mov d, rf2
017c: bf6a0000  rtif alw
017d: 1c1f3221  mov rf1, d
017e: 1c1f2021  mov rf1, a
017f: 1cff1e10  fsbd
0180: 1c1f4419  mov d, rf2
0181: bf6a0000  rtif alw
0182: 1c1f2021  mov rf1, a
0183: 1c1f2621  mov rf1, b
0184: 1d1f1e10  fml
0185: 1c1f441c  mov p, rf2
0186: bf6a0000  rtif alw
0187: 1c1f3221  mov rf1, d
0188: 1c1f2021  mov rf1, a
0189: 1e1f1e10  fdvd
018a: 1c1f4419  mov d, rf2
018b: bf6a0000  rtif alw
018c: 1c1c4234  mov rf1, $52
018d: 1c1f2021  mov rf1, a
018e: 1c1c2035  mov a, $53
018f: 1c1ca028  mov a, $40 (e)
0190: 5100007f  ldi #0x7f, ah
0191: 1c1e3228  mov $40 (e), d
0192: 1d7f1e10  fabd
0193: 1c1f2619  mov d, b
0194: 1d1e3229  fml : mov $41 (e), d
0195: 1d5da034  fmrd : mov $52, a
0196: 1c1f2619  mov d, b
0197: 1d1da635  fml : mov $53, b
0198: 1c1f201c  mov p, a
0199: 1d1db234  fml : mov $52, d
019a: 1d5f1e10  fmrd
019b: 1c1f4419  mov d, rf2
019c: bf6a0000  rtif alw
019d: 1c1f3221  mov rf1, d
019e: 1d7f1e10  fabd
019f: 1c1f4419  mov d, rf2
01a0: bf6a0000  rtif alw
01a1: 1c1f3221  mov rf1, d
01a2: 1e3f1e10  fned
01a3: 1c1f4419  mov d, rf2
01a4: bf6a0000  rtif alw
01a5: bf6a0000  rtif alw
01a6: bf6a0000  rtif alw
01a7: 1c1f3221  mov rf1, d
01a8: bf6401ab  bsif alw #0x1ab
01a9: 1c1f441c  mov p, rf2
01aa: bf6a0000  rtif alw
01ab: 1d7da003  fabd : mov $3, a
01ac: 1cdc344f  fadd : mov dh, $79
01ad: be0001ca  brif zrd #0x1ca
01ae: 5a00007e  ldi #0x7e, dh
01af: 1c1c324e  mov d, $78
01b0: 000c744e  lab $78, $570
01b1: 010c764e  fml : lab $78, $571
01b2: 1c1f3213  mov b, d
01b3: 1d9db24e  fsmd : mov $78, d
01b4: 1c1f2019  mov d, a
01b5: 1e1f1e10  fdvd
01b6: 00cc4004  fadd : lab $4, $544
01b7: 1c1f2019  mov d, a
01b8: 1d1db24e  fml : mov $78, d
01b9: 1c1f201c  mov p, a
01ba: 1e1f1e10  fdvd
01bb: 00cc4004  fadd : lab $4, $544
01bc: 1c1f2019  mov d, a
01bd: 1d1db24f  fml : mov $79, d
01be: 5000007e  ldi #0x7e, a
01bf: 036c7801  subd : lab $1, $572
01c0: 1c3c324f  andd : mov d, $79
01c1: be0001c5  brif zrd #0x1c5
01c2: 1c1db24f  mov $79, d
01c3: 1f5f201c  addd : mov p, a
01c4: 1d1c324f  fml : mov d, $79
01c5: 1c1db24f  mov $79, d
01c6: 5f000001  ldi #0x1, sft
01c7: 1f1f201d  asrd : mov ph, a
01c8: 1f5f1e10  addd
01c9: 1c1f3a19  mov d, ph
01ca: bf6a0000  rtif alw
01cb: bf6a0000  rtif alw
01cc: 1c1f3221  mov rf1, d
01cd: 1dff1e10  cfxd
01ce: 1c1f4419  mov d, rf2
01cf: bf6a0000  rtif alw
01d0: 1c1f3221  mov rf1, d
01d1: 1ddf1e10  cxfd
01d2: 1c1f4419  mov d, rf2
01d3: bf6a0000  rtif alw
01d4: 1c1f3221  mov rf1, d
01d5: bf6401e8  bsif alw #0x1e8
01d6: 1c1f4419  mov d, rf2
01d7: bf6a0000  rtif alw
01d8: 1c1f3221  mov rf1, d
01d9: bf6401e6  bsif alw #0x1e6
01da: 1c1f4419  mov d, rf2
01db: bf6a0000  rtif alw
01dc: 1c1f3221  mov rf1, d
01dd: 1c1c323d  mov d, $61
01de: bf6401e6  bsif alw #0x1e6
01df: 1c1c3234  mov d, $52
01e0: 1c1db23d  mov $61, d
01e1: bf6401e8  bsif alw #0x1e8
01e2: 1c1da034  mov $52, a
01e3: 1e1f1e10  fdvd
01e4: 1c1f4419  mov d, rf2
01e5: bf6a0000  rtif alw
01e6: 1c1da03b  mov $59, a
01e7: 1cdf1e10  fadd
01e8: 1c1da003  mov $3, a
01e9: 1cbda000  fcpd : mov $0, a
01ea: be2c2001  ldif led $1, a
01eb: 1c1c203e  mov a, $62
01ec: 1d7da63c  fabd : mov $60, b
01ed: 1c1f2019  mov d, a
01ee: 1d1c3235  fml : mov d, $53
01ef: 1c1f321c  mov p, d
01f0: 360a0004  stmh rm
01f1: 1dfda006  cfxd : mov $6, a
01f2: 1ddc323f  cxfd : mov d, $63
01f3: 360a0007  stmh fp rz
01f4: 1c1f2619  mov d, b
01f5: 1d1db235  fml : mov $53, d
01f6: 1c1f201c  mov p, a
01f7: 1cfda03b  fsbd : mov $59, a
01f8: 1cff1e10  fsbd
01f9: 1c1f2019  mov d, a
01fa: 1c1f2610  mov a, b
01fb: 1d1f1e10  fml
01fc: 1c1f201c  mov p, a
01fd: 1c1f261c  mov p, b
01fe: 1d1c3835  fml : mov p, $53
01ff: 1c1f261c  mov p, b
0200: 1d1c3836  fml : mov p, $54
0201: 1c1f261c  mov p, b
0202: 1d1c3837  fml : mov p, $55
0203: 1c1f261c  mov p, b
0204: 1d1c3838  fml : mov p, $56
0205: 1c1f261c  mov p, b
0206: 1d1c3839  fml : mov p, $57
0207: 000c4a35  lab $53, $549
0208: 1c1f201c  mov p, a
0209: 010c4839  fml : lab $57, $548
020a: 01ac4638  fspd : lab $56, $547
020b: 014c4437  fmrd : lab $55, $546
020c: 012c4236  fmsd : lab $54, $545
020d: 014c4035  fmrd : lab $53, $544
020e: 1d3f1e10  fmsd
020f: 1c1f201c  mov p, a
0210: 1cfdb804  fsbd : mov $4, p
0211: 1d9f1e10  fsmd
0212: 1c1f2619  mov d, b
0213: 1c1db23e  mov $62, d
0214: 1c1da03f  mov $63, a
0215: 1f5da001  addd : mov $1, a
0216: 1c3da005  andd : mov $5, a
0217: be00021b  brif zrd #0x21b
0218: 1d1f1e10  fml
0219: 1c1f321c  mov p, d
021a: bf6a0000  rtif alw
021b: 1c1f3213  mov b, d
021c: bf6a0000  rtif alw
021d: bf6a0000  rtif alw
021e: bf6a0000  rtif alw
021f: bf6a0000  rtif alw
0220: bf6a0000  rtif alw
0221: bf6a0000  rtif alw
0222: bf6a0000  rtif alw
0223: bf6a0000  rtif alw
0224: bf6a0000  rtif alw
0225: 1c1cc220  mov rf1, $32 (e)
0226: 1c1e4420  mov $32 (e), rf2
0227: bf6a0000  rtif alw
0228: 1c1cc220  mov rf1, $32 (e)
0229: 1c1e4421  mov $33 (e), rf2
022a: bf6a0000  rtif alw
022b: 1c1f2021  mov rf1, a
022c: 1c1f2621  mov rf1, b
022d: bf640234  bsif alw #0x234
022e: 1c1e3227  mov $39 (e), d
022f: 5f000010  ldi #0x10, sft
0230: 1eff1e10  lsld
0231: 1f1f1e10  asrd
0232: 1c1f4419  mov d, rf2
0233: bf6a0000  rtif alw
0234: 1c1ca024  mov a, $36 (e)
0235: 1c1ca625  mov b, $37 (e)
0236: bea00249  brif gpio0 #0x249
0237: 1c1ca028  mov a, $40 (e)
0238: 5100007f  ldi #0x7f, ah
0239: 1c1e3228  mov $40 (e), d
023a: 1d7f3813  fabd : mov b, p
023b: 1c1f2619  mov d, b
023c: 1d1f261c  fml : mov p, b
023d: 1c1e3229  mov $41 (e), d
023e: 1d5f1e10  fmrd
023f: 1c1f2019  mov d, a
0240: 1d1f1e10  fml
0241: 1c1f321c  mov p, d
0242: 1d7f1e10  fabd
0243: 5100006f  ldi #0x6f, ah
0244: 527fffff  ldi #0x7fffff, al
0245: 1cbf1e10  fcpd
0246: be2c3800  ldif led $0, p
0247: 1c1cb827  mov p, $39 (e)
0248: bf6a0000  rtif alw
0249: 1c1ca628  mov b, $40 (e)
024a: 5400007f  ldi #0x7f, bh
024b: 1c1e3228  mov $40 (e), d
024c: 1d7f3810  fabd : mov a, p
024d: 1c1f2019  mov d, a
024e: 1d1f201c  fml : mov p, a
024f: 1c1e3229  mov $41 (e), d
0250: 1d5f1e10  fmrd
0251: 1c1f2619  mov d, b
0252: 1d1f1e10  fml
0253: 1c1f321c  mov p, d
0254: 1d7f1e10  fabd
0255: 5100006f  ldi #0x6f, ah
0256: 527fffff  ldi #0x7fffff, al
0257: 1cbf1e10  fcpd
0258: be2c3800  ldif led $0, p
0259: 1c1cb827  mov p, $39 (e)
025a: bf6a0000  rtif alw
025b: 1c1da00a  mov $10, a
025c: 1c1db209  mov $9, d
025d: 1f7da001  subd : mov $1, a
025e: be10026b  brif ged #0x26b
025f: 1c1d8407  mov $7, x0
0260: 43000010  ldi #0x10, x1
0261: 3c04000c  rep #12
0262: 1c1343a1  mov (x0+1)+0x200, (x1+1)
0263: 1c1db209  mov $9, d
0264: 1f5c0407  addd : mov x0, $7
0265: 1c1c3209  mov d, $9
0266: 42000010  ldi #0x10, x0
0267: 1c1d8607  mov $7, x1
0268: 3c04000c  rep #12
0269: 1c0f43a1  mov (x0+1), (x1+1)+0x200
026a: bf60026e  brif alw #0x26e
026b: 1c1db209  mov $9, d
026c: 1f5f1e10  addd
026d: 1c1c3209  mov d, $9
026e: bf6a0000  rtif alw
026f: 1c1db209  mov $9, d
0270: 1c1da000  mov $0, a
0271: 1f7da00b  subd : mov $11, a
0272: be20027d  brif led #0x27d
0273: 1c1db209  mov $9, d
0274: 1f7da001  subd : mov $1, a
0275: be10027a  brif ged #0x27a
0276: 1c1db207  mov $7, d
0277: 3900000c  lia #0xc
0278: 1f7da001  subd : mov $1, a
0279: 1c1c3207  mov d, $7
027a: 1c1db209  mov $9, d
027b: 1f7f1e10  subd
027c: 1c1c3209  mov d, $9
027d: bf6a0000  rtif alw
027e: 1c1d8607  mov $7, x1
027f: 3c04000c  rep #12
0280: 1c1c4381  mov rf1, (bx1+1)
0281: bf6a0000  rtif alw
0282: 1c1db208  mov $8, d
0283: 1c1c3207  mov d, $7
0284: 1c101200  mov $512, $9
0285: bf6a0000  rtif alw
0286: 1c1d8607  mov $7, x1
0287: 1c1c4210  mov rf1, $16
0288: 000f4610  lab $16, (x1+3)+0x200
0289: 1d1c4211  fml : mov rf1, $17
028a: 000f4611  lab $17, (x1+3)+0x200
028b: 1dbc4212  fspd : mov rf1, $18
028c: 000f7412  lab $18, (x1-6)+0x200
028d: 1d3c4213  fmsd : mov rf1, $19
028e: 000f4613  lab $19, (x1+3)+0x200
028f: 1d3c4214  fmsd : mov rf1, $20
0290: 000f4614  lab $20, (x1+3)+0x200
0291: 1c1c3228  mov d, $40
0292: 1dbc4215  fspd : mov rf1, $21
0293: 000f7415  lab $21, (x1-6)+0x200
0294: 1d3c4216  fmsd : mov rf1, $22
0295: 000f4616  lab $22, (x1+3)+0x200
0296: 1d3c4217  fmsd : mov rf1, $23
0297: 000f4617  lab $23, (x1+3)+0x200
0298: 1c1c322b  mov d, $43
0299: 1dbc4218  fspd : mov rf1, $24
029a: 000f7418  lab $24, (x1-6)+0x200
029b: 1d3c4219  fmsd : mov rf1, $25
029c: 000f4619  lab $25, (x1+3)+0x200
029d: 1d3c421a  fmsd : mov rf1, $26
029e: 000f461a  lab $26, (x1+3)+0x200
029f: 1c1c322e  mov d, $46
02a0: 1dbc421b  fspd : mov rf1, $27
02a1: 000f461b  lab $27, (x1+3)+0x200
02a2: 1d3da198  fmsd : mov (bx1-8), a
02a3: 00cf4610  fadd : lab $16, (x1+3)+0x200
02a4: 012f4611  fmsd : lab $17, (x1+3)+0x200
02a5: 1c1c3231  mov d, $49
02a6: 01af7412  fspd : lab $18, (x1-6)+0x200
02a7: 012f4613  fmsd : lab $19, (x1+3)+0x200
02a8: 012f4614  fmsd : lab $20, (x1+3)+0x200
02a9: 1c1c3229  mov d, $41
02aa: 01af7415  fspd : lab $21, (x1-6)+0x200
02ab: 012f4616  fmsd : lab $22, (x1+3)+0x200
02ac: 012f4617  fmsd : lab $23, (x1+3)+0x200
02ad: 1c1c322c  mov d, $44
02ae: 01af7418  fspd : lab $24, (x1-6)+0x200
02af: 012f4619  fmsd : lab $25, (x1+3)+0x200
02b0: 012f461a  fmsd : lab $26, (x1+3)+0x200
02b1: 1c1c322f  mov d, $47
02b2: 01af461b  fspd : lab $27, (x1+3)+0x200
02b3: 1d3da198  fmsd : mov (bx1-8), a
02b4: 00cf4610  fadd : lab $16, (x1+3)+0x200
02b5: 012f4611  fmsd : lab $17, (x1+3)+0x200
02b6: 1c1c3232  mov d, $50
02b7: 01af7412  fspd : lab $18, (x1-6)+0x200
02b8: 012f4613  fmsd : lab $19, (x1+3)+0x200
02b9: 012f4614  fmsd : lab $20, (x1+3)+0x200
02ba: 1c1c322a  mov d, $42
02bb: 01af7415  fspd : lab $21, (x1-6)+0x200
02bc: 012f4616  fmsd : lab $22, (x1+3)+0x200
02bd: 012f4617  fmsd : lab $23, (x1+3)+0x200
02be: 1c1c322d  mov d, $45
02bf: 01af7418  fspd : lab $24, (x1-6)+0x200
02c0: 012f4619  fmsd : lab $25, (x1+3)+0x200
02c1: 012f461a  fmsd : lab $26, (x1+3)+0x200
02c2: 1c1c3230  mov d, $48
02c3: 01af461b  fspd : lab $27, (x1+3)+0x200
02c4: 1d3da180  fmsd : mov (bx1), a
02c5: 1d9d8607  fsmd : mov $7, x1
02c6: 1cdf1e10  fadd
02c7: 1c1c3233  mov d, $51
02c8: 42000028  ldi #0x28, x0
02c9: 3c04000c  rep #12
02ca: 1c0f43a1  mov (x0+1), (x1+1)+0x200
02cb: bf6a0000  rtif alw
02cc: 1c1c425c  mov rf1, $92
02cd: 1c0cb85c  mov $92, $604
02ce: 1c1c425d  mov rf1, $93
02cf: 1c0cba5d  mov $93, $605
02d0: 1c1c425e  mov rf1, $94
02d1: 1c0cbc5e  mov $94, $606
02d2: 1c1c4256  mov rf1, $86
02d3: 1c1c4257  mov rf1, $87
02d4: 1c1c4258  mov rf1, $88
02d5: 1c1c4259  mov rf1, $89
02d6: 1c0cb259  mov $89, $601
02d7: 1c1c425a  mov rf1, $90
02d8: 1c0cb45a  mov $90, $602
02d9: 1c1c425b  mov rf1, $91
02da: 1c0cb65b  mov $91, $603
02db: 000cba57  lab $87, $605
02dc: 036cb658  subd : lab $88, $603
02dd: 1f7f2019  subd : mov d, a
02de: 1c1f2619  mov d, b
02df: 010cbc58  fml : lab $88, $606
02e0: 036cb457  subd : lab $87, $602
02e1: 1f7f2019  subd : mov d, a
02e2: 1c1f2619  mov d, b
02e3: 01acbc58  fspd : lab $88, $606
02e4: 1d5f1e10  fmrd
02e5: 1c1c3245  mov d, $69
02e6: 036cb256  subd : lab $86, $601
02e7: 1f7f2019  subd : mov d, a
02e8: 1c1f2619  mov d, b
02e9: 010cb856  fml : lab $86, $604
02ea: 036cb658  subd : lab $88, $603
02eb: 1f7f2019  subd : mov d, a
02ec: 1c1f2619  mov d, b
02ed: 01acb856  fspd : lab $86, $604
02ee: 1d4c8a45  fmrd : mov $69, $581
02ef: 1c1c3246  mov d, $70
02f0: 036cb457  subd : lab $87, $602
02f1: 1f7f2019  subd : mov d, a
02f2: 1c1f2619  mov d, b
02f3: 010cba57  fml : lab $87, $605
02f4: 036cb256  subd : lab $86, $601
02f5: 1f7f2019  subd : mov d, a
02f6: 1c1f2619  mov d, b
02f7: 1dac8c46  fspd : mov $70, $582
02f8: 1d5f1e10  fmrd
02f9: 1c1c3247  mov d, $71
02fa: 000c8a45  lab $69, $581
02fb: 010c8c46  fml : lab $70, $582
02fc: 1dac8e47  fspd : mov $71, $583
02fd: 000c8e47  lab $71, $583
02fe: 1d3f1e10  fmsd
02ff: 1d9f1e10  fsmd
0300: 1c1f2019  mov d, a
0301: 1c1ca02a  mov a, $42 (e)
0302: 5100007f  ldi #0x7f, ah
0303: 1c1e262a  mov $42 (e), b
0304: 1d1e322b  fml : mov $43 (e), d
0305: 1d5da045  fmrd : mov $69, a
0306: 1c1f2619  mov d, b
0307: 1d1da046  fml : mov $70, a
0308: 1c1f441c  mov p, rf2
0309: 1d1da047  fml : mov $71, a
030a: 1c1f441c  mov p, rf2
030b: 1d1f1e10  fml
030c: 1c1f441c  mov p, rf2
030d: bf6a0000  rtif alw
030e: 1c1c4234  mov rf1, $52
030f: 1c0c6834  mov $52, $564
0310: 000c6834  lab $52, $564
0311: 1c1c4235  mov rf1, $53
0312: 1c0c6a35  mov $53, $565
0313: 010c6a35  fml : lab $53, $565
0314: 1c1c4236  mov rf1, $54
0315: 1c0c6c36  mov $54, $566
0316: 01ac6c36  fspd : lab $54, $566
0317: 1d3f1e10  fmsd
0318: 1d9f1e10  fsmd
0319: 1c1f2019  mov d, a
031a: 1c1ca02a  mov a, $42 (e)
031b: 5100007f  ldi #0x7f, ah
031c: 1c1e262a  mov $42 (e), b
031d: 1d1e322b  fml : mov $43 (e), d
031e: 1d5da034  fmrd : mov $52, a
031f: 1c1f2619  mov d, b
0320: 1d1da035  fml : mov $53, a
0321: 1c1f441c  mov p, rf2
0322: 1d1da036  fml : mov $54, a
0323: 1c1f441c  mov p, rf2
0324: 1d1f1e10  fml
0325: 1c1f441c  mov p, rf2
0326: bf6a0000  rtif alw
0327: 1c1d8607  mov $7, x1
0328: 3c04000c  rep #12
0329: 1c0f4203  mov $3, (x1+1)+0x200
032a: 1c1d8607  mov $7, x1
032b: 3c040003  rep #3
032c: 1c0f4804  mov $4, (x1+4)+0x200
032d: bf6a0000  rtif alw
032e: 1c1d8607  mov $7, x1
032f: 3c04000c  rep #12
0330: 1c1dc581  mov (bx1+1), rf2
0331: bf6a0000  rtif alw
0332: 1c1d8607  mov $7, x1
0333: 1c1c4253  mov rf1, $83
0334: 000f4653  lab $83, (x1+3)+0x200
0335: 1c1c4254  mov rf1, $84
0336: 010f4c54  fml : lab $84, (x1+6)+0x200
0337: 1c1db39d  mov (bx1-3), d
0338: 1c1c4255  mov rf1, $85
0339: 012f7655  fmsd : lab $85, (x1-5)+0x200
033a: 012f4653  fmsd : lab $83, (x1+3)+0x200
033b: 012f4a54  fmsd : lab $84, (x1+5)+0x200
033c: 1c1c3381  mov d, (bx1+1)
033d: 1c1db39d  mov (bx1-3), d
033e: 012f7655  fmsd : lab $85, (x1-5)+0x200
033f: 012f4653  fmsd : lab $83, (x1+3)+0x200
0340: 012f4a54  fmsd : lab $84, (x1+5)+0x200
0341: 1c1c3381  mov d, (bx1+1)
0342: 1c1db39d  mov (bx1-3), d
0343: 012f4655  fmsd : lab $85, (x1+3)+0x200
0344: 1d3f1e10  fmsd
0345: 1d3f1e10  fmsd
0346: 1c1c3380  mov d, (bx1)
0347: bf6a0000  rtif alw
0348: 1c1d8607  mov $7, x1
0349: 1c1c4253  mov rf1, $83
034a: 000f4653  lab $83, (x1+3)+0x200
034b: 1d1c4254  fml : mov rf1, $84
034c: 000f7a54  lab $84, (x1-3)+0x200
034d: 1dbc4255  fspd : mov rf1, $85
034e: 1c1c3383  mov d, (bx1+3)
034f: 1c1c3983  mov p, (bx1+3)
0350: 000f7655  lab $85, (x1-5)+0x200
0351: 010f4653  fml : lab $83, (x1+3)+0x200
0352: 01af4454  fspd : lab $84, (x1+2)+0x200
0353: 1c1c339b  mov d, (bx1-5)
0354: 1c1c3986  mov p, (bx1+6)
0355: 010f7655  fml : lab $85, (x1-5)+0x200
0356: 01af4453  fspd : lab $83, (x1+2)+0x200
0357: 1c1c3383  mov d, (bx1+3)
0358: 1c1c399e  mov p, (bx1-2)
0359: 010f4654  fml : lab $84, (x1+3)+0x200
035a: 01af7455  fspd : lab $85, (x1-6)+0x200
035b: 1c1c3383  mov d, (bx1+3)
035c: 1d1c3983  fml : mov p, (bx1+3)
035d: 1c1c3980  mov p, (bx1)
035e: bf6a0000  rtif alw
035f: 1c1cc220  mov rf1, $32 (e)
0360: 1c086821  mov $33 (e), $52
0361: 1c086c20  mov $32 (e), $54
0362: bf60036a  brif alw #0x36a
0363: 1c1f3221  mov rf1, d
0364: 1c1c323d  mov d, $61
0365: bf6401e6  bsif alw #0x1e6
0366: 1c1c3234  mov d, $52
0367: 1c1db23d  mov $61, d
0368: bf6401e8  bsif alw #0x1e8
0369: 1c1c3236  mov d, $54
036a: 1c1d8607  mov $7, x1
036b: 000f463a  lab $58, (x1+3)+0x200
036c: 000f4634  lab $52, (x1+3)+0x200
036d: 010f7a36  fml : lab $54, (x1-3)+0x200
036e: 01af4636  fspd : lab $54, (x1+3)+0x200
036f: 014f7a34  fmrd : lab $52, (x1-3)+0x200
0370: 1c1c3381  mov d, (bx1+1)
0371: 01af4634  fspd : lab $52, (x1+3)+0x200
0372: 012f7e36  fmsd : lab $54, (x1-1)+0x200
0373: 1c1c339e  mov d, (bx1-2)
0374: 01af4636  fspd : lab $54, (x1+3)+0x200
0375: 014f7a34  fmrd : lab $52, (x1-3)+0x200
0376: 1c1c3381  mov d, (bx1+1)
0377: 01af4634  fspd : lab $52, (x1+3)+0x200
0378: 012f7e36  fmsd : lab $54, (x1-1)+0x200
0379: 1c1c339e  mov d, (bx1-2)
037a: 01af4636  fspd : lab $54, (x1+3)+0x200
037b: 014f7a34  fmrd : lab $52, (x1-3)+0x200
037c: 1dbc3383  fspd : mov d, (bx1+3)
037d: 1d9f1e10  fsmd
037e: 1c1c3380  mov d, (bx1)
037f: bf6a0000  rtif alw
0380: 1c1cc220  mov rf1, $32 (e)
0381: 1c086821  mov $33 (e), $52
0382: 1c086c20  mov $32 (e), $54
0383: bf60038b  brif alw #0x38b
0384: 1c1f3221  mov rf1, d
0385: 1c1c323d  mov d, $61
0386: bf6401e6  bsif alw #0x1e6
0387: 1c1c3234  mov d, $52
0388: 1c1db23d  mov $61, d
0389: bf6401e8  bsif alw #0x1e8
038a: 1c1c3236  mov d, $54
038b: 1c1d8607  mov $7, x1
038c: 000f4c34  lab $52, (x1+6)+0x200
038d: 010f4036  fml : lab $54, (x1)+0x200
038e: 01af7434  fspd : lab $52, (x1-6)+0x200
038f: 012f4036  fmsd : lab $54, (x1)+0x200
0390: 1c1c3381  mov d, (bx1+1)
0391: 01af4c34  fspd : lab $52, (x1+6)+0x200
0392: 014f7e36  fmrd : lab $54, (x1-1)+0x200
0393: 1c1c3381  mov d, (bx1+1)
0394: 01af7434  fspd : lab $52, (x1-6)+0x200
0395: 012f4036  fmsd : lab $54, (x1)+0x200
0396: 1c1c3381  mov d, (bx1+1)
0397: 01af4c34  fspd : lab $52, (x1+6)+0x200
0398: 014f7e36  fmrd : lab $54, (x1-1)+0x200
0399: 1c1c3381  mov d, (bx1+1)
039a: 01af7434  fspd : lab $52, (x1-6)+0x200
039b: 012f4036  fmsd : lab $54, (x1)+0x200
039c: 1dbc3386  fspd : mov d, (bx1+6)
039d: 1d5f1e10  fmrd
039e: 1c1c3380  mov d, (bx1)
039f: bf6a0000  rtif alw
03a0: 1c1cc220  mov rf1, $32 (e)
03a1: 1c086821  mov $33 (e), $52
03a2: 1c086c20  mov $32 (e), $54
03a3: bf6003ab  brif alw #0x3ab
03a4: 1c1f3221  mov rf1, d
03a5: 1c1c323d  mov d, $61
03a6: bf6401e6  bsif alw #0x1e6
03a7: 1c1c3234  mov d, $52
03a8: 1c1db23d  mov $61, d
03a9: bf6401e8  bsif alw #0x1e8
03aa: 1c1c3236  mov d, $54
03ab: 1c1d8607  mov $7, x1
03ac: 000f4634  lab $52, (x1+3)+0x200
03ad: 010f7a36  fml : lab $54, (x1-3)+0x200
03ae: 01af4636  fspd : lab $54, (x1+3)+0x200
03af: 014f7a34  fmrd : lab $52, (x1-3)+0x200
03b0: 1c1c3381  mov d, (bx1+1)
03b1: 01af4634  fspd : lab $52, (x1+3)+0x200
03b2: 012f7e36  fmsd : lab $54, (x1-1)+0x200
03b3: 1c1c339e  mov d, (bx1-2)
03b4: 01af4636  fspd : lab $54, (x1+3)+0x200
03b5: 014f7a34  fmrd : lab $52, (x1-3)+0x200
03b6: 1c1c3381  mov d, (bx1+1)
03b7: 01af4634  fspd : lab $52, (x1+3)+0x200
03b8: 012f7e36  fmsd : lab $54, (x1-1)+0x200
03b9: 1c1c339e  mov d, (bx1-2)
03ba: 01af4636  fspd : lab $54, (x1+3)+0x200
03bb: 014f7a34  fmrd : lab $52, (x1-3)+0x200
03bc: 1dbc3383  fspd : mov d, (bx1+3)
03bd: 1d9f1e10  fsmd
03be: 1c1c3380  mov d, (bx1)
03bf: bf6a0000  rtif alw
03c0: 1c1dc409  mov $9, rf2
03c1: bf6a0000  rtif alw
03c2: 1c1d8407  mov $7, x0
03c3: 1c103289  mov $521(x0), $25
03c4: 1c10348a  mov $522(x0), $26
03c5: 1c10368b  mov $523(x0), $27
03c6: 1c1db219  mov $25, d
03c7: 1c1c4253  mov rf1, $83
03c8: 0010a680  lab $512(x0), $83
03c9: 1d1c4254  fml : mov rf1, $84
03ca: 0010a883  lab $515(x0), $84
03cb: 1d3c4255  fmsd : mov rf1, $85
03cc: 0010aa86  lab $518(x0), $85
03cd: 0130a681  fmsd : lab $513(x0), $83
03ce: 0130a884  fmsd : lab $516(x0), $84
03cf: 1c1f4419  mov d, rf2
03d0: 1c1db21a  mov $26, d
03d1: 0130aa87  fmsd : lab $519(x0), $85
03d2: 0130a682  fmsd : lab $514(x0), $83
03d3: 0130a885  fmsd : lab $517(x0), $84
03d4: 1c1f4419  mov d, rf2
03d5: 1c1db21b  mov $27, d
03d6: 0130aa88  fmsd : lab $520(x0), $85
03d7: 1d3f1e10  fmsd
03d8: 1d9f1e10  fsmd
03d9: 1c1f4419  mov d, rf2
03da: bf6a0000  rtif alw
03db: 63400000  ldi #0x400000, rf3
03dc: 1c1d867d  mov $125, x1
03dd: 1c1e21a1  mov (x1+1) (e), a
03de: 1c1c067d  mov x1, $125
03df: 3b007f00  lid #0x7f00
03e0: 1f5da007  addd : mov $7, a
03e1: 1c1f0619  mov d, x1
03e2: 59000200  ldi #0x200, d
03e3: 1f5f1e10  addd
03e4: 1c1f0419  mov d, x0
03e5: 3c04000c  rep #12
03e6: 1c0743a1  mov (x0+1), (x1+1) (e)
03e7: 63000000  ldi #0x0, rf3
03e8: bf6a0000  rtif alw
03e9: 63400000  ldi #0x400000, rf3
03ea: 1c1d867d  mov $125, x1
03eb: 1c1e21a1  mov (x1+1) (e), a
03ec: 1c1c067d  mov x1, $125
03ed: 3b007f00  lid #0x7f00
03ee: 1f5d8607  addd : mov $7, x1
03ef: 1c1f0419  mov d, x0
03f0: 3c04000c  rep #12
03f1: 1c0b03a1  mov (x0+1) (e), (bx1+1)
03f2: 63000000  ldi #0x0, rf3
03f3: bf6a0000  rtif alw
03f4: 3b000070  lid #0x70
03f5: 1c1f2021  mov rf1, a
03f6: 3da00004  addd : clr0 a
03f7: 1c1f0619  mov d, x1
03f8: 1c1c21a4  mov a, (x1+4)
03f9: 1c1c21a4  mov a, (x1+4)
03fa: 1c1c21a4  mov a, (x1+4)
03fb: bf6a0000  rtif alw
03fc: 3b000070  lid #0x70
03fd: 1c1f2021  mov rf1, a
03fe: 1f5da004  addd : mov $4, a
03ff: 1c1f0619  mov d, x1
0400: 1c1c21a4  mov a, (x1+4)
0401: 1c1c21a4  mov a, (x1+4)
0402: 1c1c21a4  mov a, (x1+4)
0403: bf6a0000  rtif alw
0404: 3b000070  lid #0x70
0405: 1c1f2021  mov rf1, a
0406: 1f5f1e10  addd
0407: 1c1f0619  mov d, x1
0408: 1c1c43a4  mov rf1, (x1+4)
0409: 1c1c43a4  mov rf1, (x1+4)
040a: 1c1c43a4  mov rf1, (x1+4)
040b: bf6a0000  rtif alw
040c: 3b000070  lid #0x70
040d: 1c1f2021  mov rf1, a
040e: 1f5f1e10  addd
040f: 1c1f0619  mov d, x1
0410: 1c1dc5a4  mov (x1+4), rf2
0411: 1c1dc5a4  mov (x1+4), rf2
0412: 1c1dc5a4  mov (x1+4), rf2
0413: bf6a0000  rtif alw
0414: 3b000070  lid #0x70
0415: 1c1f2021  mov rf1, a
0416: 1f5f2021  addd : mov rf1, a
0417: 1c1f0619  mov d, x1
0418: 1c1db3a0  mov (x1), d
0419: 1cdf2021  fadd : mov rf1, a
041a: 1c1c33a4  mov d, (x1+4)
041b: 1c1db3a0  mov (x1), d
041c: 1cdf2021  fadd : mov rf1, a
041d: 1c1c33a4  mov d, (x1+4)
041e: 1c1db3a0  mov (x1), d
041f: 1cdf1e10  fadd
0420: 1c1c33a4  mov d, (x1+4)
0421: bf6a0000  rtif alw
0422: 3b000070  lid #0x70
0423: 1c1f2021  mov rf1, a
0424: 1f5f2021  addd : mov rf1, a
0425: 1c1f0619  mov d, x1
0426: 1c1db3a0  mov (x1), d
0427: 1cff2021  fsbd : mov rf1, a
0428: 1c1c33a4  mov d, (x1+4)
0429: 1c1db3a0  mov (x1), d
042a: 1cff2021  fsbd : mov rf1, a
042b: 1c1c33a4  mov d, (x1+4)
042c: 1c1db3a0  mov (x1), d
042d: 1cff1e10  fsbd
042e: 1c1c33a4  mov d, (x1+4)
042f: bf6a0000  rtif alw
0430: 3b000070  lid #0x70
0431: 1c1f2021  mov rf1, a
0432: 1f5f2021  addd : mov rf1, a
0433: 1c1f0619  mov d, x1
0434: 1c1da7a0  mov (x1), b
0435: 1d1f2021  fml : mov rf1, a
0436: 1c1c39a4  mov p, (x1+4)
0437: 1c1da7a0  mov (x1), b
0438: 1d1f2021  fml : mov rf1, a
0439: 1c1c39a4  mov p, (x1+4)
043a: 1c1da7a0  mov (x1), b
043b: 1d1f1e10  fml
043c: 1c1c39a4  mov p, (x1+4)
043d: bf6a0000  rtif alw
043e: 3b000070  lid #0x70
043f: 1c1f2021  mov rf1, a
0440: 1f5f2021  addd : mov rf1, a
0441: 1c1f0619  mov d, x1
0442: 1c1db3a0  mov (x1), d
0443: 1e1f2021  fdvd : mov rf1, a
0444: 1c1c33a4  mov d, (x1+4)
0445: 1c1db3a0  mov (x1), d
0446: 1e1f2021  fdvd : mov rf1, a
0447: 1c1c33a4  mov d, (x1+4)
0448: 1c1db3a0  mov (x1), d
0449: 1e1f1e10  fdvd
044a: 1c1c33a4  mov d, (x1+4)
044b: bf6a0000  rtif alw
044c: 3b000070  lid #0x70
044d: 1c1f2021  mov rf1, a
044e: 1f5d8407  addd : mov $7, x0
044f: 1c1f0619  mov d, x1
0450: 1c1da1a4  mov (x1+4), a
0451: 1c1c2053  mov a, $83
0452: 1c1da1a4  mov (x1+4), a
0453: 1c1c2054  mov a, $84
0454: 1c1da1a4  mov (x1+4), a
0455: 1c1c2055  mov a, $85
0456: 1c1f0619  mov d, x1
0457: 1c103289  mov $521(x0), $25
0458: 1c10348a  mov $522(x0), $26
0459: 1c10368b  mov $523(x0), $27
045a: 1c1db219  mov $25, d
045b: 0010a680  lab $512(x0), $83
045c: 0110a883  fml : lab $515(x0), $84
045d: 0130aa86  fmsd : lab $518(x0), $85
045e: 0130a681  fmsd : lab $513(x0), $83
045f: 0130a884  fmsd : lab $516(x0), $84
0460: 1c1c33a4  mov d, (x1+4)
0461: 1c1db21a  mov $26, d
0462: 0130aa87  fmsd : lab $519(x0), $85
0463: 0130a682  fmsd : lab $514(x0), $83
0464: 0130a885  fmsd : lab $517(x0), $84
0465: 1c1c33a4  mov d, (x1+4)
0466: 1c1db21b  mov $27, d
0467: 0130aa88  fmsd : lab $520(x0), $85
0468: 1d3f1e10  fmsd
0469: 1d9f1e10  fsmd
046a: 1c1c33a4  mov d, (x1+4)
046b: bf6a0000  rtif alw
046c: 1c1c423d  mov rf1, $61
046d: 1c1c423e  mov rf1, $62
046e: 1c1c423f  mov rf1, $63
046f: 1c1c4234  mov rf1, $52
0470: 1c0c7a34  mov $52, $573
0471: 1c1c4234  mov rf1, $52
0472: 1c0c7c34  mov $52, $574
0473: 1c1c4234  mov rf1, $52
0474: 1c0c7e34  mov $52, $575
0475: 00107a3d  lab $573, $61
0476: 02907c3e  d=b-a : lab $574, $62
0477: 1c1c323d  mov d, $61
0478: 02907e3f  d=b-a : lab $575, $63
0479: 1c1c323e  mov d, $62
047a: 1e8c7a3d  d=b-a : mov $61, $573
047b: 1c1c323f  mov d, $63
047c: 000c7a3d  lab $61, $573
047d: 1d0c7c3e  fml : mov $62, $574
047e: 000c7c3e  lab $62, $574
047f: 1dac7e3f  fspd : mov $63, $575
0480: 000c7e3f  lab $63, $575
0481: 1d3f1e10  fmsd
0482: 1d9f1e10  fsmd
0483: 1c1c3234  mov d, $52
0484: 1c1f2019  mov d, a
0485: 1c1ca02a  mov a, $42 (e)
0486: 5100007f  ldi #0x7f, ah
0487: 1c1e262a  mov $42 (e), b
0488: 1d1e322b  fml : mov $43 (e), d
0489: 1d5da034  fmrd : mov $52, a
048a: 1c1f2619  mov d, b
048b: 1d1f1e10  fml
048c: 1c1f441c  mov p, rf2
048d: bf6a0000  rtif alw
048e: 1c1c4234  mov rf1, $52
048f: 1c0c6834  mov $52, $564
0490: 000c6834  lab $52, $564
0491: 1c1c4235  mov rf1, $53
0492: 1c1c4236  mov rf1, $54
0493: 1c0c6c36  mov $54, $566
0494: 010c6c36  fml : lab $54, $566
0495: 1c0c6a35  mov $53, $565
0496: 01ac6a35  fspd : lab $53, $565
0497: 1d3f1e10  fmsd
0498: 1d9f2019  fsmd : mov d, a
0499: 1c1c203d  mov a, $61
049a: 1c1c323e  mov d, $62
049b: 1c1ca02a  mov a, $42 (e)
049c: 5100007f  ldi #0x7f, ah
049d: 1c1e262a  mov $42 (e), b
049e: 1d1e322b  fml : mov $43 (e), d
049f: 1d5da03d  fmrd : mov $61, a
04a0: 1c1f2619  mov d, b
04a1: 1d1da034  fml : mov $52, a
04a2: 1c1c383f  mov p, $63
04a3: 1d1da036  fml : mov $54, a
04a4: 1c1c3843  mov p, $67
04a5: 1d1f1e10  fml
04a6: 1c1c3844  mov p, $68
04a7: bf6404e3  bsif alw #0x4e3
04a8: 1c1da03e  mov $62, a
04a9: 1c1ca02a  mov a, $42 (e)
04aa: 5100007f  ldi #0x7f, ah
04ab: 1c1e262a  mov $42 (e), b
04ac: 1d1e322b  fml : mov $43 (e), d
04ad: 1d5da635  fmrd : mov $53, b
04ae: 1c1f2019  mov d, a
04af: 1d1da63f  fml : mov $63, b
04b0: 1c1c3843  mov p, $67
04b1: 1d1f1e10  fml
04b2: 1c1c3844  mov p, $68
04b3: bf6404f8  bsif alw #0x4f8
04b4: bf6a0000  rtif alw
04b5: 1c1c4253  mov rf1, $83
04b6: 1c0ca653  mov $83, $595
04b7: 000ca653  lab $83, $595
04b8: 1c1c4254  mov rf1, $84
04b9: 1c1c4255  mov rf1, $85
04ba: 1c0caa55  mov $85, $597
04bb: 010caa55  fml : lab $85, $597
04bc: 1c0ca854  mov $84, $596
04bd: 01aca854  fspd : lab $84, $596
04be: 1d3f1e10  fmsd
04bf: 1d9f2019  fsmd : mov d, a
04c0: 1c1c203d  mov a, $61
04c1: 1c1c323e  mov d, $62
04c2: 1c1ca02a  mov a, $42 (e)
04c3: 5100007f  ldi #0x7f, ah
04c4: 1c1e262a  mov $42 (e), b
04c5: 1d1e322b  fml : mov $43 (e), d
04c6: 1d5da03d  fmrd : mov $61, a
04c7: 1c1c3234  mov d, $52
04c8: 1c1f2619  mov d, b
04c9: 1d1da03e  fml : mov $62, a
04ca: 1c1c383f  mov p, $63
04cb: 1c1da03e  mov $62, a
04cc: 1c1ca02a  mov a, $42 (e)
04cd: 5100007f  ldi #0x7f, ah
04ce: 1c1e262a  mov $42 (e), b
04cf: 1d1e322b  fml : mov $43 (e), d
04d0: 1d5da654  fmrd : mov $84, b
04d1: 1c1f2019  mov d, a
04d2: 1d1da63f  fml : mov $63, b
04d3: 1c1f321c  mov p, d
04d4: 1e3f1e10  fned
04d5: 1c1c3243  mov d, $67
04d6: 1d1f1e10  fml
04d7: 1c1c3844  mov p, $68
04d8: bf6404f8  bsif alw #0x4f8
04d9: 1c1da053  mov $83, a
04da: 1c1da634  mov $52, b
04db: 1d1da055  fml : mov $85, a
04dc: 1c1f321c  mov p, d
04dd: 1e3f1e10  fned
04de: 1c1c3243  mov d, $67
04df: 1d1f1e10  fml
04e0: 1c1c3844  mov p, $68
04e1: bf6404e3  bsif alw #0x4e3
04e2: bf6a0000  rtif alw
04e3: 1c1d8607  mov $7, x1
04e4: 000f4c44  lab $68, (x1+6)+0x200
04e5: 010f4043  fml : lab $67, (x1)+0x200
04e6: 01af7444  fspd : lab $68, (x1-6)+0x200
04e7: 014f4043  fmrd : lab $67, (x1)+0x200
04e8: 1c1c3381  mov d, (bx1+1)
04e9: 01af4c44  fspd : lab $68, (x1+6)+0x200
04ea: 012f7e43  fmsd : lab $67, (x1-1)+0x200
04eb: 1c1c3381  mov d, (bx1+1)
04ec: 01af7444  fspd : lab $68, (x1-6)+0x200
04ed: 014f4043  fmrd : lab $67, (x1)+0x200
04ee: 1c1c3381  mov d, (bx1+1)
04ef: 01af4c44  fspd : lab $68, (x1+6)+0x200
04f0: 012f7e43  fmsd : lab $67, (x1-1)+0x200
04f1: 1c1c3381  mov d, (bx1+1)
04f2: 01af7444  fspd : lab $68, (x1-6)+0x200
04f3: 014f4043  fmrd : lab $67, (x1)+0x200
04f4: 1dbc3386  fspd : mov d, (bx1+6)
04f5: 1d3f1e10  fmsd
04f6: 1c1c3380  mov d, (bx1)
04f7: bf6a0000  rtif alw
04f8: 1c1d8607  mov $7, x1
04f9: 000f463a  lab $58, (x1+3)+0x200
04fa: 000f4644  lab $68, (x1+3)+0x200
04fb: 010f7a43  fml : lab $67, (x1-3)+0x200
04fc: 01af4643  fspd : lab $67, (x1+3)+0x200
04fd: 014f7a44  fmrd : lab $68, (x1-3)+0x200
04fe: 1c1c3381  mov d, (bx1+1)
04ff: 01af4644  fspd : lab $68, (x1+3)+0x200
0500: 012f7e43  fmsd : lab $67, (x1-1)+0x200
0501: 1c1c339e  mov d, (bx1-2)
0502: 01af4643  fspd : lab $67, (x1+3)+0x200
0503: 014f7a44  fmrd : lab $68, (x1-3)+0x200
0504: 1c1c3381  mov d, (bx1+1)
0505: 01af4644  fspd : lab $68, (x1+3)+0x200
0506: 012f7e43  fmsd : lab $67, (x1-1)+0x200
0507: 1c1c339e  mov d, (bx1-2)
0508: 01af4643  fspd : lab $67, (x1+3)+0x200
0509: 014f7a44  fmrd : lab $68, (x1-3)+0x200
050a: 1dbc3383  fspd : mov d, (bx1+3)
050b: 1d9f1e10  fsmd
050c: 1c1c3380  mov d, (bx1)
050d: bf6a0000  rtif alw
050e: 1c1f3221  mov rf1, d
050f: bf6401e8  bsif alw #0x1e8
0510: 1c1f2019  mov d, a
0511: 1c1f2621  mov rf1, b
0512: 1d1f1e10  fml
0513: 1c1f441c  mov p, rf2
0514: bf6a0000  rtif alw
0515: 1c1f3221  mov rf1, d
0516: bf6401e6  bsif alw #0x1e6
0517: 1c1f2019  mov d, a
0518: 1c1f2621  mov rf1, b
0519: 1d1f1e10  fml
051a: 1c1f441c  mov p, rf2
051b: bf6a0000  rtif alw
051c: 1c1f2021  mov rf1, a
051d: 1c1f3221  mov rf1, d
051e: 1cff2021  fsbd : mov rf1, a
051f: 1c1c3234  mov d, $52
0520: 1c1f3221  mov rf1, d
0521: 1cff2021  fsbd : mov rf1, a
0522: 1c1c3235  mov d, $53
0523: 1c1f3221  mov rf1, d
0524: 1cff1e10  fsbd
0525: 1c1f2019  mov d, a
0526: 1c1f2610  mov a, b
0527: 1d1da035  fml : mov $53, a
0528: 1c1f2610  mov a, b
0529: 1dbda034  fspd : mov $52, a
052a: 1c1f2610  mov a, b
052b: 1d3f1e10  fmsd
052c: 1d9f1e10  fsmd
052d: bf6401ab  bsif alw #0x1ab
052e: 1c1f441c  mov p, rf2
052f: bf6a0000  rtif alw
0530: 1c1f2021  mov rf1, a
0531: 1c1f2621  mov rf1, b
0532: 1d1f2021  fml : mov rf1, a
0533: 1c1f2621  mov rf1, b
0534: 1dbf2021  fspd : mov rf1, a
0535: 1c1f2621  mov rf1, b
0536: 1d3f1e10  fmsd
0537: 1d3f1e10  fmsd
0538: 1c1f4419  mov d, rf2
0539: bf6a0000  rtif alw
053a: 1c1c4234  mov rf1, $52
053b: 1c1da034  mov $52, a
053c: 1c1f2610  mov a, b
053d: 1d1c4235  fml : mov rf1, $53
053e: 1c1da035  mov $53, a
053f: 1c1f2610  mov a, b
0540: 1dbc4236  fspd : mov rf1, $54
0541: 1c1da036  mov $54, a
0542: 1c1f2610  mov a, b
0543: 1d3c4237  fmsd : mov rf1, $55
0544: 1d3c4238  fmsd : mov rf1, $56
0545: bf6401ab  bsif alw #0x1ab
0546: 1c1c383d  mov p, $61
0547: 1c1da037  mov $55, a
0548: 1c1f2610  mov a, b
0549: 1d1da038  fml : mov $56, a
054a: 1c1f2610  mov a, b
054b: 1dbc4239  fspd : mov rf1, $57
054c: 1c1da039  mov $57, a
054d: 1c1f2610  mov a, b
054e: 1d3f1e10  fmsd
054f: 1d3f1e10  fmsd
0550: bf6401ab  bsif alw #0x1ab
0551: 1c1f201c  mov p, a
0552: 1c1da63d  mov $61, b
0553: 1d1da034  fml : mov $52, a
0554: 1c1c383d  mov p, $61
0555: 1c1da637  mov $55, b
0556: 1d1da035  fml : mov $53, a
0557: 1c1da638  mov $56, b
0558: 1dbda036  fspd : mov $54, a
0559: 1c1da639  mov $57, b
055a: 1d3f1e10  fmsd
055b: 1d3da03d  fmsd : mov $61, a
055c: 1e1f1e10  fdvd
055d: 1c1f4419  mov d, rf2
055e: bf6a0000  rtif alw
055f: 1c1f2621  mov rf1, b
0560: 1c1c2636  mov b, $54
0561: 1c1f2021  mov rf1, a
0562: 1c1c2034  mov a, $52
0563: 1d1f2621  fml : mov rf1, b
0564: 1c1f2021  mov rf1, a
0565: 1dbc2035  fspd : mov a, $53
0566: 1c1f201c  mov p, a
0567: 1cff2021  fsbd : mov rf1, a
0568: 1c1c2037  mov a, $55
0569: 1c1f4419  mov d, rf2
056a: 1d1f2621  fml : mov rf1, b
056b: 1c1da034  mov $52, a
056c: 1dbf1e10  fspd
056d: 1c1f201c  mov p, a
056e: 1cfda035  fsbd : mov $53, a
056f: 1c1f4419  mov d, rf2
0570: 1d1da036  fml : mov $54, a
0571: 1c1da637  mov $55, b
0572: 1dbf1e10  fspd
0573: 1c1f201c  mov p, a
0574: 1cff1e10  fsbd
0575: 1c1f4419  mov d, rf2
0576: bf6a0000  rtif alw
0577: 1c1f2021  mov rf1, a
0578: 1c1f2621  mov rf1, b
0579: 1e9c2036  d=b-a : mov a, $54
057a: 1c1c3234  mov d, $52
057b: 1c1f3221  mov rf1, d
057c: 1c1f2021  mov rf1, a
057d: 1cff2621  fsbd : mov rf1, b
057e: 1c1c3235  mov d, $53
057f: 1e9da036  d=b-a : mov $54, a
0580: 1c1f2621  mov rf1, b
0581: 1e9f2619  d=b-a : mov d, b
0582: 1c1f2019  mov d, a
0583: 1d1da035  fml : mov $53, a
0584: 1c1da634  mov $52, b
0585: 1dbf1e10  fspd
0586: 1c1f201c  mov p, a
0587: 1cff1e10  fsbd
0588: 1c1f4419  mov d, rf2
0589: bf6a0000  rtif alw
058a: 3b800000  lid #0x800000
058b: 5f000008  ldi #0x8, sft
058c: 1eff1e10  lsld
058d: 1c1c3237  mov d, $55
058e: 63400000  ldi #0x400000, rf3
058f: 1c1d867d  mov $125, x1
0590: 1c1e21a1  mov (x1+1) (e), a
0591: 1c1c2045  mov a, $69
0592: 1c1e21a1  mov (x1+1) (e), a
0593: 1c1c2046  mov a, $70
0594: 1c1e21a1  mov (x1+1) (e), a
0595: 1c1c2047  mov a, $71
0596: 1c1e33a1  mov (x1+1) (e), d
0597: 39800000  lia #0x800000
0598: 1f5c2039  addd : mov a, $57
0599: 1c1e21a1  mov (x1+1) (e), a
059a: 1c1c2066  mov a, $102
059b: 1c1e21a1  mov (x1+1) (e), a
059c: 1c1c204e  mov a, $78
059d: 1c1e21a1  mov (x1+1) (e), a
059e: 1c1c2036  mov a, $54
059f: 1c1e21a1  mov (x1+1) (e), a
05a0: 1c1c204f  mov a, $79
05a1: 1c1e21a1  mov (x1+1) (e), a
05a2: 1c1c2067  mov a, $103
05a3: 1c1c067d  mov x1, $125
05a4: 63000000  ldi #0x0, rf3
05a5: 1c1da036  mov $54, a
05a6: 1f5c323d  addd : mov d, $61
05a7: bf6405c3  bsif alw #0x5c3
05a8: be0005ba  brif zrd #0x5ba
05a9: 1c1da04f  mov $79, a
05aa: 1c1db23d  mov $61, d
05ab: 1f5f1e10  addd
05ac: bf6405c3  bsif alw #0x5c3
05ad: be0005ba  brif zrd #0x5ba
05ae: 1c1da04e  mov $78, a
05af: 1c1db23d  mov $61, d
05b0: 1f5f1e10  addd
05b1: bf6405c3  bsif alw #0x5c3
05b2: be0005ba  brif zrd #0x5ba
05b3: 39000068  lia #0x68
05b4: 1c1db267  mov $103, d
05b5: 3da00004  addd : clr0 a
05b6: 1c1f0619  mov d, x1
05b7: 1c1c21a1  mov a, (x1+1)
05b8: 1c1c21a1  mov a, (x1+1)
05b9: bf6005c1  brif alw #0x5c1
05ba: 39000068  lia #0x68
05bb: 1c1db267  mov $103, d
05bc: 1f5da03f  addd : mov $63, a
05bd: 1c1f0619  mov d, x1
05be: 1c1c21a1  mov a, (x1+1)
05bf: 1c1da03e  mov $62, a
05c0: 1c1c21a1  mov a, (x1+1)
05c1: 63000000  ldi #0x0, rf3
05c2: bf6a0000  rtif alw
05c3: 1c1f4619  mov d, rf3
05c4: 1c1f0619  mov d, x1
05c5: 1c1e19a1  mov (x1+1) (e), c0
05c6: 1c1e33a1  mov (x1+1) (e), d
05c7: 5f000002  ldi #0x2, sft
05c8: 1edda039  lsrd : mov $57, a
05c9: 1f5f1e10  addd
05ca: 1c1c323e  mov d, $62
05cb: 1c1e33a1  mov (x1+1) (e), d
05cc: 1edda039  lsrd : mov $57, a
05cd: 1f5f1e10  addd
05ce: 1c1c323f  mov d, $63
05cf: 1c1dc63e  mov $62, rf3
05d0: 1c1d843e  mov $62, x0
05d1: 43000256  ldi #0x256, x1
05d2: 1c0b45a2  mov (x0+2) (e), (x1+2)
05d3: 1c0b43a1  mov (x0+1) (e), (x1+1)
05d4: 1c0b45a2  mov (x0+2) (e), (x1+2)
05d5: 1c0b43a1  mov (x0+1) (e), (x1+1)
05d6: 1c0b45a2  mov (x0+2) (e), (x1+2)
05d7: 1c0b43a1  mov (x0+1) (e), (x1+1)
05d8: 1c0b45a2  mov (x0+2) (e), (x1+2)
05d9: 1c0b43a1  mov (x0+1) (e), (x1+1)
05da: 000cb047  lab $71, $600
05db: 028cb245  d=b-a : lab $69, $601
05dc: 1c1c3234  mov d, $52
05dd: 028cac45  d=b-a : lab $69, $598
05de: 1c1c3235  mov d, $53
05df: 028cb647  d=b-a : lab $71, $603
05e0: 1e9f2619  d=b-a : mov d, b
05e1: 1c1f2019  mov d, a
05e2: 1d1da035  fml : mov $53, a
05e3: 1c1da634  mov $52, b
05e4: 1dbf1e10  fspd
05e5: 014cb647  fmrd : lab $71, $603
05e6: 1c1c3240  mov d, $64
05e7: 028cb845  d=b-a : lab $69, $604
05e8: 1c1c3234  mov d, $52
05e9: 028cb245  d=b-a : lab $69, $601
05ea: 1c1c3235  mov d, $53
05eb: 028cbc47  d=b-a : lab $71, $606
05ec: 1e9f2619  d=b-a : mov d, b
05ed: 1c1f2019  mov d, a
05ee: 1d1da035  fml : mov $53, a
05ef: 1c1da634  mov $52, b
05f0: 1dbd863f  fspd : mov $63, x1
05f1: 1d5dc63f  fmrd : mov $63, rf3
05f2: 1c1c3241  mov d, $65
05f3: 1c1e33a1  mov (x1+1) (e), d
05f4: 39000004  lia #0x4
05f5: 002cbc47  andd : lab $71, $606
05f6: be00061b  brif zrd #0x61b
05f7: 028cbe45  d=b-a : lab $69, $607
05f8: 1c1c3234  mov d, $52
05f9: 028cb845  d=b-a : lab $69, $604
05fa: 1c1c3235  mov d, $53
05fb: 028cc247  d=b-a : lab $71, $609
05fc: 1e9f2619  d=b-a : mov d, b
05fd: 1c1f2019  mov d, a
05fe: 1d1da035  fml : mov $53, a
05ff: 1c1da634  mov $52, b
0600: 1dbf1e10  fspd
0601: 014cc247  fmrd : lab $71, $609
0602: 1c1c3242  mov d, $66
0603: 028cac45  d=b-a : lab $69, $598
0604: 1c1c3234  mov d, $52
0605: 028cbe45  d=b-a : lab $69, $607
0606: 1c1c3235  mov d, $53
0607: 028cb047  d=b-a : lab $71, $600
0608: 1e9f2619  d=b-a : mov d, b
0609: 1c1f2019  mov d, a
060a: 1d1da035  fml : mov $53, a
060b: 1c1da634  mov $52, b
060c: 1dbf1e10  fspd
060d: 1d5da042  fmrd : mov $66, a
060e: 1c1f2619  mov d, b
060f: 1c5da041  orad : mov $65, a
0610: 1c5da040  orad : mov $64, a
0611: 1c5da037  orad : mov $55, a
0612: 1c3f3213  andd : mov b, d
0613: be000630  brif zrd #0x630
0614: 1c1da042  mov $66, a
0615: 1c3da041  andd : mov $65, a
0616: 1c3da040  andd : mov $64, a
0617: 1c3da037  andd : mov $55, a
0618: 1c3f1e10  andd
0619: be000632  brif zrd #0x632
061a: bf600630  brif alw #0x630
061b: 028cac45  d=b-a : lab $69, $598
061c: 1c1c3234  mov d, $52
061d: 028cb845  d=b-a : lab $69, $604
061e: 1c1c3235  mov d, $53
061f: 028cb047  d=b-a : lab $71, $600
0620: 1e9f2619  d=b-a : mov d, b
0621: 1c1f2019  mov d, a
0622: 1d1da035  fml : mov $53, a
0623: 1c1da634  mov $52, b
0624: 1dbf1e10  fspd
0625: 1d5da041  fmrd : mov $65, a
0626: 1c1f2619  mov d, b
0627: 1c5da040  orad : mov $64, a
0628: 1c5da037  orad : mov $55, a
0629: 1c3f3213  andd : mov b, d
062a: be000630  brif zrd #0x630
062b: 1c1da041  mov $65, a
062c: 1c3da040  andd : mov $64, a
062d: 1c3da037  andd : mov $55, a
062e: 1c3f1e10  andd
062f: be000632  brif zrd #0x632
0630: 3c060001  set #0x0001
0631: bf6a0000  rtif alw
0632: 1c1db23e  mov $62, d
0633: 39000010  lia #0x10
0634: 1f5da03f  addd : mov $63, a
0635: 1c1c323e  mov d, $62
0636: 3b000001  lid #0x1
0637: 1f5f1e10  addd
0638: 1c1c323f  mov d, $63
0639: bf0005cf  brif zc0 #0x5cf
063a: 3c020001  clr1 #0x0001
063b: bf6a0000  rtif alw
063c: 39000068  lia #0x68
063d: 1c1f3221  mov rf1, d
063e: 1f5f1e10  addd
063f: 1c1f0619  mov d, x1
0640: 1c1da1a1  mov (x1+1), a
0641: 1c1da7a1  mov (x1+1), b
0642: 1c1f3210  mov a, d
0643: 1c3f4610  andd : mov a, rf3
0644: be00064d  brif zrd #0x64d
0645: 1c1f0610  mov a, x1
0646: 1c1e21a1  mov (x1+1) (e), a
0647: 1c1f4613  mov b, rf3
0648: 1c1f0613  mov b, x1
0649: 3c040010  rep #16
064a: 1c1e45a1  mov (x1+1) (e), rf2
064b: 1c1f4410  mov a, rf2
064c: bf600650  brif alw #0x650
064d: 3c000004  clr0 a
064e: 3c040011  rep #17
064f: 1c1f4410  mov a, rf2
0650: 63000000  ldi #0x0, rf3
0651: bf6a0000  rtif alw
0652: 1c1f3221  mov rf1, d
0653: 1c1f2021  mov rf1, a
0654: 1cff1e10  fsbd
0655: 1c1c3234  mov d, $52
0656: 1c0c6834  mov $52, $564
0657: 1c1f3221  mov rf1, d
0658: 1c1f2021  mov rf1, a
0659: 00ec6834  fsbd : lab $52, $564
065a: 1c1c3236  mov d, $54
065b: 1c0c6c36  mov $54, $566
065c: 010c6c36  fml : lab $54, $566
065d: 01ac0603  fspd : lab $3, $515
065e: 1d3f1e10  fmsd
065f: 1d9f2019  fsmd : mov d, a
0660: 1c1c203d  mov a, $61
0661: 1c1c323e  mov d, $62
0662: 1c1ca02a  mov a, $42 (e)
0663: 5100007f  ldi #0x7f, ah
0664: 1c1e262a  mov $42 (e), b
0665: 1d1e322b  fml : mov $43 (e), d
0666: 1d5da03d  fmrd : mov $61, a
0667: 1c1f2619  mov d, b
0668: 1d1da034  fml : mov $52, a
0669: 1c1c383f  mov p, $63
066a: 1d1da036  fml : mov $54, a
066b: 1d1c3843  fml : mov p, $67
066c: 1c1c3844  mov p, $68
066d: bf6004e3  brif alw #0x4e3
066e: bf64025b  bsif alw #0x25b
066f: bf640332  bsif alw #0x332
0670: bf640652  bsif alw #0x652
0671: bf640348  bsif alw #0x348
0672: bf64014e  bsif alw #0x14e
0673: bf60026f  brif alw #0x26f
0674: bf64025b  bsif alw #0x25b
0675: bf640332  bsif alw #0x332
0676: bf640652  bsif alw #0x652
0677: bf64014e  bsif alw #0x14e
0678: bf60026f  brif alw #0x26f
0679: bf640332  bsif alw #0x332
067a: bf600652  brif alw #0x652
067b: bf64025b  bsif alw #0x25b
067c: bf640332  bsif alw #0x332
067d: bf6403a0  bsif alw #0x3a0
067e: bf64035f  bsif alw #0x35f
067f: bf64014e  bsif alw #0x14e
0680: bf60026f  brif alw #0x26f
