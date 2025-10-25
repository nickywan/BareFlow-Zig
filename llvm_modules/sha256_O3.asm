
llvm_modules/sha256_O3.elf:     file format elf32-i386


Disassembly of section .text:

08049000 <compute>:
 8049000:	55                   	push   ebp
 8049001:	89 e5                	mov    ebp,esp
 8049003:	53                   	push   ebx
 8049004:	83 ec 14             	sub    esp,0x14
 8049007:	e8 00 00 00 00       	call   804900c <compute+0xc>
 804900c:	5b                   	pop    ebx
 804900d:	81 c3 04 21 00 00    	add    ebx,0x2104
 8049013:	8b 83 f0 ee ff ff    	mov    eax,DWORD PTR [ebx-0x1110]
 8049019:	89 45 ec             	mov    DWORD PTR [ebp-0x14],eax
 804901c:	8b 83 f4 ee ff ff    	mov    eax,DWORD PTR [ebx-0x110c]
 8049022:	89 45 f0             	mov    DWORD PTR [ebp-0x10],eax
 8049025:	8b 83 f8 ee ff ff    	mov    eax,DWORD PTR [ebx-0x1108]
 804902b:	89 45 f4             	mov    DWORD PTR [ebp-0xc],eax
 804902e:	8b 83 fc ee ff ff    	mov    eax,DWORD PTR [ebx-0x1104]
 8049034:	89 45 f8             	mov    DWORD PTR [ebp-0x8],eax
 8049037:	8d 4d ec             	lea    ecx,[ebp-0x14]
 804903a:	e8 11 00 00 00       	call   8049050 <sha256_simple>
 804903f:	83 c4 14             	add    esp,0x14
 8049042:	5b                   	pop    ebx
 8049043:	5d                   	pop    ebp
 8049044:	c3                   	ret
 8049045:	90                   	nop
 8049046:	90                   	nop
 8049047:	90                   	nop
 8049048:	90                   	nop
 8049049:	90                   	nop
 804904a:	90                   	nop
 804904b:	90                   	nop
 804904c:	90                   	nop
 804904d:	90                   	nop
 804904e:	90                   	nop
 804904f:	90                   	nop

08049050 <sha256_simple>:
 8049050:	55                   	push   ebp
 8049051:	89 e5                	mov    ebp,esp
 8049053:	57                   	push   edi
 8049054:	56                   	push   esi
 8049055:	81 ec 5c 01 00 00    	sub    esp,0x15c
 804905b:	e8 00 00 00 00       	call   8049060 <sha256_simple+0x10>
 8049060:	58                   	pop    eax
 8049061:	81 c0 b0 20 00 00    	add    eax,0x20b0
 8049067:	89 4d cc             	mov    DWORD PTR [ebp-0x34],ecx
 804906a:	c7 45 9c 0c 00 00 00 	mov    DWORD PTR [ebp-0x64],0xc
 8049071:	c7 45 d0 67 e6 09 6a 	mov    DWORD PTR [ebp-0x30],0x6a09e667
 8049078:	c7 45 a8 85 ae 67 bb 	mov    DWORD PTR [ebp-0x58],0xbb67ae85
 804907f:	c7 45 ac 72 f3 6e 3c 	mov    DWORD PTR [ebp-0x54],0x3c6ef372
 8049086:	c7 45 b0 3a f5 4f a5 	mov    DWORD PTR [ebp-0x50],0xa54ff53a
 804908d:	c7 45 b4 7f 52 0e 51 	mov    DWORD PTR [ebp-0x4c],0x510e527f
 8049094:	c7 45 b8 8c 68 05 9b 	mov    DWORD PTR [ebp-0x48],0x9b05688c
 804909b:	c7 45 bc ab d9 83 1f 	mov    DWORD PTR [ebp-0x44],0x1f83d9ab
 80490a2:	c7 45 c0 19 cd e0 5b 	mov    DWORD PTR [ebp-0x40],0x5be0cd19
 80490a9:	c7 45 e8 00 00 00 00 	mov    DWORD PTR [ebp-0x18],0x0
 80490b0:	83 7d e8 10          	cmp    DWORD PTR [ebp-0x18],0x10
 80490b4:	7d 78                	jge    804912e <sha256_simple+0xde>
 80490b6:	8b 4d e8             	mov    ecx,DWORD PTR [ebp-0x18]
 80490b9:	c1 e1 02             	shl    ecx,0x2
 80490bc:	3b 4d 9c             	cmp    ecx,DWORD PTR [ebp-0x64]
 80490bf:	7d 52                	jge    8049113 <sha256_simple+0xc3>
 80490c1:	8b 4d cc             	mov    ecx,DWORD PTR [ebp-0x34]
 80490c4:	8b 55 e8             	mov    edx,DWORD PTR [ebp-0x18]
 80490c7:	c1 e2 02             	shl    edx,0x2
 80490ca:	0f b6 0c 11          	movzx  ecx,BYTE PTR [ecx+edx*1]
 80490ce:	c1 e1 18             	shl    ecx,0x18
 80490d1:	8b 55 cc             	mov    edx,DWORD PTR [ebp-0x34]
 80490d4:	8b 75 e8             	mov    esi,DWORD PTR [ebp-0x18]
 80490d7:	c1 e6 02             	shl    esi,0x2
 80490da:	0f b6 54 32 01       	movzx  edx,BYTE PTR [edx+esi*1+0x1]
 80490df:	c1 e2 10             	shl    edx,0x10
 80490e2:	09 d1                	or     ecx,edx
 80490e4:	8b 55 cc             	mov    edx,DWORD PTR [ebp-0x34]
 80490e7:	8b 75 e8             	mov    esi,DWORD PTR [ebp-0x18]
 80490ea:	c1 e6 02             	shl    esi,0x2
 80490ed:	0f b6 54 32 02       	movzx  edx,BYTE PTR [edx+esi*1+0x2]
 80490f2:	c1 e2 08             	shl    edx,0x8
 80490f5:	09 d1                	or     ecx,edx
 80490f7:	8b 55 cc             	mov    edx,DWORD PTR [ebp-0x34]
 80490fa:	8b 75 e8             	mov    esi,DWORD PTR [ebp-0x18]
 80490fd:	c1 e6 02             	shl    esi,0x2
 8049100:	0f b6 54 32 03       	movzx  edx,BYTE PTR [edx+esi*1+0x3]
 8049105:	09 d1                	or     ecx,edx
 8049107:	8b 55 e8             	mov    edx,DWORD PTR [ebp-0x18]
 804910a:	89 8c 95 9c fe ff ff 	mov    DWORD PTR [ebp+edx*4-0x164],ecx
 8049111:	eb 0e                	jmp    8049121 <sha256_simple+0xd1>
 8049113:	8b 4d e8             	mov    ecx,DWORD PTR [ebp-0x18]
 8049116:	c7 84 8d 9c fe ff ff 	mov    DWORD PTR [ebp+ecx*4-0x164],0x0
 804911d:	00 00 00 00 
 8049121:	eb 00                	jmp    8049123 <sha256_simple+0xd3>
 8049123:	8b 4d e8             	mov    ecx,DWORD PTR [ebp-0x18]
 8049126:	83 c1 01             	add    ecx,0x1
 8049129:	89 4d e8             	mov    DWORD PTR [ebp-0x18],ecx
 804912c:	eb 82                	jmp    80490b0 <sha256_simple+0x60>
 804912e:	c7 45 f4 10 00 00 00 	mov    DWORD PTR [ebp-0xc],0x10
 8049135:	83 7d f4 40          	cmp    DWORD PTR [ebp-0xc],0x40
 8049139:	0f 8d e4 00 00 00    	jge    8049223 <sha256_simple+0x1d3>
 804913f:	8b 4d f4             	mov    ecx,DWORD PTR [ebp-0xc]
 8049142:	83 e9 02             	sub    ecx,0x2
 8049145:	8b 8c 8d 9c fe ff ff 	mov    ecx,DWORD PTR [ebp+ecx*4-0x164]
 804914c:	c1 e9 11             	shr    ecx,0x11
 804914f:	8b 55 f4             	mov    edx,DWORD PTR [ebp-0xc]
 8049152:	83 ea 02             	sub    edx,0x2
 8049155:	8b 94 95 9c fe ff ff 	mov    edx,DWORD PTR [ebp+edx*4-0x164]
 804915c:	c1 e2 0f             	shl    edx,0xf
 804915f:	09 d1                	or     ecx,edx
 8049161:	8b 55 f4             	mov    edx,DWORD PTR [ebp-0xc]
 8049164:	83 ea 02             	sub    edx,0x2
 8049167:	8b 94 95 9c fe ff ff 	mov    edx,DWORD PTR [ebp+edx*4-0x164]
 804916e:	c1 ea 13             	shr    edx,0x13
 8049171:	8b 75 f4             	mov    esi,DWORD PTR [ebp-0xc]
 8049174:	83 ee 02             	sub    esi,0x2
 8049177:	8b b4 b5 9c fe ff ff 	mov    esi,DWORD PTR [ebp+esi*4-0x164]
 804917e:	c1 e6 0d             	shl    esi,0xd
 8049181:	09 f2                	or     edx,esi
 8049183:	31 d1                	xor    ecx,edx
 8049185:	8b 55 f4             	mov    edx,DWORD PTR [ebp-0xc]
 8049188:	83 ea 02             	sub    edx,0x2
 804918b:	8b 94 95 9c fe ff ff 	mov    edx,DWORD PTR [ebp+edx*4-0x164]
 8049192:	c1 ea 0a             	shr    edx,0xa
 8049195:	31 d1                	xor    ecx,edx
 8049197:	8b 55 f4             	mov    edx,DWORD PTR [ebp-0xc]
 804919a:	83 ea 07             	sub    edx,0x7
 804919d:	03 8c 95 9c fe ff ff 	add    ecx,DWORD PTR [ebp+edx*4-0x164]
 80491a4:	8b 55 f4             	mov    edx,DWORD PTR [ebp-0xc]
 80491a7:	83 ea 0f             	sub    edx,0xf
 80491aa:	8b 94 95 9c fe ff ff 	mov    edx,DWORD PTR [ebp+edx*4-0x164]
 80491b1:	c1 ea 07             	shr    edx,0x7
 80491b4:	8b 75 f4             	mov    esi,DWORD PTR [ebp-0xc]
 80491b7:	83 ee 0f             	sub    esi,0xf
 80491ba:	8b b4 b5 9c fe ff ff 	mov    esi,DWORD PTR [ebp+esi*4-0x164]
 80491c1:	c1 e6 19             	shl    esi,0x19
 80491c4:	09 f2                	or     edx,esi
 80491c6:	8b 75 f4             	mov    esi,DWORD PTR [ebp-0xc]
 80491c9:	83 ee 0f             	sub    esi,0xf
 80491cc:	8b b4 b5 9c fe ff ff 	mov    esi,DWORD PTR [ebp+esi*4-0x164]
 80491d3:	c1 ee 12             	shr    esi,0x12
 80491d6:	8b 7d f4             	mov    edi,DWORD PTR [ebp-0xc]
 80491d9:	83 ef 0f             	sub    edi,0xf
 80491dc:	8b bc bd 9c fe ff ff 	mov    edi,DWORD PTR [ebp+edi*4-0x164]
 80491e3:	c1 e7 0e             	shl    edi,0xe
 80491e6:	09 fe                	or     esi,edi
 80491e8:	31 f2                	xor    edx,esi
 80491ea:	8b 75 f4             	mov    esi,DWORD PTR [ebp-0xc]
 80491ed:	83 ee 0f             	sub    esi,0xf
 80491f0:	8b b4 b5 9c fe ff ff 	mov    esi,DWORD PTR [ebp+esi*4-0x164]
 80491f7:	c1 ee 03             	shr    esi,0x3
 80491fa:	31 f2                	xor    edx,esi
 80491fc:	01 d1                	add    ecx,edx
 80491fe:	8b 55 f4             	mov    edx,DWORD PTR [ebp-0xc]
 8049201:	83 ea 10             	sub    edx,0x10
 8049204:	03 8c 95 9c fe ff ff 	add    ecx,DWORD PTR [ebp+edx*4-0x164]
 804920b:	8b 55 f4             	mov    edx,DWORD PTR [ebp-0xc]
 804920e:	89 8c 95 9c fe ff ff 	mov    DWORD PTR [ebp+edx*4-0x164],ecx
 8049215:	8b 4d f4             	mov    ecx,DWORD PTR [ebp-0xc]
 8049218:	83 c1 01             	add    ecx,0x1
 804921b:	89 4d f4             	mov    DWORD PTR [ebp-0xc],ecx
 804921e:	e9 12 ff ff ff       	jmp    8049135 <sha256_simple+0xe5>
 8049223:	8b 4d d0             	mov    ecx,DWORD PTR [ebp-0x30]
 8049226:	89 4d ec             	mov    DWORD PTR [ebp-0x14],ecx
 8049229:	8b 4d a8             	mov    ecx,DWORD PTR [ebp-0x58]
 804922c:	89 4d dc             	mov    DWORD PTR [ebp-0x24],ecx
 804922f:	8b 4d ac             	mov    ecx,DWORD PTR [ebp-0x54]
 8049232:	89 4d e0             	mov    DWORD PTR [ebp-0x20],ecx
 8049235:	8b 4d b0             	mov    ecx,DWORD PTR [ebp-0x50]
 8049238:	89 4d c4             	mov    DWORD PTR [ebp-0x3c],ecx
 804923b:	8b 4d b4             	mov    ecx,DWORD PTR [ebp-0x4c]
 804923e:	89 4d f0             	mov    DWORD PTR [ebp-0x10],ecx
 8049241:	8b 4d b8             	mov    ecx,DWORD PTR [ebp-0x48]
 8049244:	89 4d d4             	mov    DWORD PTR [ebp-0x2c],ecx
 8049247:	8b 4d bc             	mov    ecx,DWORD PTR [ebp-0x44]
 804924a:	89 4d d8             	mov    DWORD PTR [ebp-0x28],ecx
 804924d:	8b 4d c0             	mov    ecx,DWORD PTR [ebp-0x40]
 8049250:	89 4d c8             	mov    DWORD PTR [ebp-0x38],ecx
 8049253:	c7 45 e4 00 00 00 00 	mov    DWORD PTR [ebp-0x1c],0x0
 804925a:	83 7d e4 40          	cmp    DWORD PTR [ebp-0x1c],0x40
 804925e:	0f 8d ea 00 00 00    	jge    804934e <sha256_simple+0x2fe>
 8049264:	8b 4d c8             	mov    ecx,DWORD PTR [ebp-0x38]
 8049267:	8b 55 f0             	mov    edx,DWORD PTR [ebp-0x10]
 804926a:	c1 ea 06             	shr    edx,0x6
 804926d:	8b 75 f0             	mov    esi,DWORD PTR [ebp-0x10]
 8049270:	c1 e6 1a             	shl    esi,0x1a
 8049273:	09 f2                	or     edx,esi
 8049275:	8b 75 f0             	mov    esi,DWORD PTR [ebp-0x10]
 8049278:	c1 ee 0b             	shr    esi,0xb
 804927b:	8b 7d f0             	mov    edi,DWORD PTR [ebp-0x10]
 804927e:	c1 e7 15             	shl    edi,0x15
 8049281:	09 fe                	or     esi,edi
 8049283:	31 f2                	xor    edx,esi
 8049285:	8b 75 f0             	mov    esi,DWORD PTR [ebp-0x10]
 8049288:	c1 ee 19             	shr    esi,0x19
 804928b:	8b 7d f0             	mov    edi,DWORD PTR [ebp-0x10]
 804928e:	c1 e7 07             	shl    edi,0x7
 8049291:	09 fe                	or     esi,edi
 8049293:	31 f2                	xor    edx,esi
 8049295:	01 d1                	add    ecx,edx
 8049297:	8b 55 f0             	mov    edx,DWORD PTR [ebp-0x10]
 804929a:	23 55 d4             	and    edx,DWORD PTR [ebp-0x2c]
 804929d:	8b 75 f0             	mov    esi,DWORD PTR [ebp-0x10]
 80492a0:	83 f6 ff             	xor    esi,0xffffffff
 80492a3:	23 75 d8             	and    esi,DWORD PTR [ebp-0x28]
 80492a6:	31 f2                	xor    edx,esi
 80492a8:	01 d1                	add    ecx,edx
 80492aa:	8b 55 e4             	mov    edx,DWORD PTR [ebp-0x1c]
 80492ad:	03 8c 90 00 ef ff ff 	add    ecx,DWORD PTR [eax+edx*4-0x1100]
 80492b4:	8b 55 e4             	mov    edx,DWORD PTR [ebp-0x1c]
 80492b7:	03 8c 95 9c fe ff ff 	add    ecx,DWORD PTR [ebp+edx*4-0x164]
 80492be:	89 4d a4             	mov    DWORD PTR [ebp-0x5c],ecx
 80492c1:	8b 4d ec             	mov    ecx,DWORD PTR [ebp-0x14]
 80492c4:	c1 e9 02             	shr    ecx,0x2
 80492c7:	8b 55 ec             	mov    edx,DWORD PTR [ebp-0x14]
 80492ca:	c1 e2 1e             	shl    edx,0x1e
 80492cd:	09 d1                	or     ecx,edx
 80492cf:	8b 55 ec             	mov    edx,DWORD PTR [ebp-0x14]
 80492d2:	c1 ea 0d             	shr    edx,0xd
 80492d5:	8b 75 ec             	mov    esi,DWORD PTR [ebp-0x14]
 80492d8:	c1 e6 13             	shl    esi,0x13
 80492db:	09 f2                	or     edx,esi
 80492dd:	31 d1                	xor    ecx,edx
 80492df:	8b 55 ec             	mov    edx,DWORD PTR [ebp-0x14]
 80492e2:	c1 ea 16             	shr    edx,0x16
 80492e5:	8b 75 ec             	mov    esi,DWORD PTR [ebp-0x14]
 80492e8:	c1 e6 0a             	shl    esi,0xa
 80492eb:	09 f2                	or     edx,esi
 80492ed:	31 d1                	xor    ecx,edx
 80492ef:	8b 55 ec             	mov    edx,DWORD PTR [ebp-0x14]
 80492f2:	23 55 dc             	and    edx,DWORD PTR [ebp-0x24]
 80492f5:	8b 75 ec             	mov    esi,DWORD PTR [ebp-0x14]
 80492f8:	23 75 e0             	and    esi,DWORD PTR [ebp-0x20]
 80492fb:	31 f2                	xor    edx,esi
 80492fd:	8b 75 dc             	mov    esi,DWORD PTR [ebp-0x24]
 8049300:	23 75 e0             	and    esi,DWORD PTR [ebp-0x20]
 8049303:	31 f2                	xor    edx,esi
 8049305:	01 d1                	add    ecx,edx
 8049307:	89 4d a0             	mov    DWORD PTR [ebp-0x60],ecx
 804930a:	8b 4d d8             	mov    ecx,DWORD PTR [ebp-0x28]
 804930d:	89 4d c8             	mov    DWORD PTR [ebp-0x38],ecx
 8049310:	8b 4d d4             	mov    ecx,DWORD PTR [ebp-0x2c]
 8049313:	89 4d d8             	mov    DWORD PTR [ebp-0x28],ecx
 8049316:	8b 4d f0             	mov    ecx,DWORD PTR [ebp-0x10]
 8049319:	89 4d d4             	mov    DWORD PTR [ebp-0x2c],ecx
 804931c:	8b 4d c4             	mov    ecx,DWORD PTR [ebp-0x3c]
 804931f:	03 4d a4             	add    ecx,DWORD PTR [ebp-0x5c]
 8049322:	89 4d f0             	mov    DWORD PTR [ebp-0x10],ecx
 8049325:	8b 4d e0             	mov    ecx,DWORD PTR [ebp-0x20]
 8049328:	89 4d c4             	mov    DWORD PTR [ebp-0x3c],ecx
 804932b:	8b 4d dc             	mov    ecx,DWORD PTR [ebp-0x24]
 804932e:	89 4d e0             	mov    DWORD PTR [ebp-0x20],ecx
 8049331:	8b 4d ec             	mov    ecx,DWORD PTR [ebp-0x14]
 8049334:	89 4d dc             	mov    DWORD PTR [ebp-0x24],ecx
 8049337:	8b 4d a4             	mov    ecx,DWORD PTR [ebp-0x5c]
 804933a:	03 4d a0             	add    ecx,DWORD PTR [ebp-0x60]
 804933d:	89 4d ec             	mov    DWORD PTR [ebp-0x14],ecx
 8049340:	8b 4d e4             	mov    ecx,DWORD PTR [ebp-0x1c]
 8049343:	83 c1 01             	add    ecx,0x1
 8049346:	89 4d e4             	mov    DWORD PTR [ebp-0x1c],ecx
 8049349:	e9 0c ff ff ff       	jmp    804925a <sha256_simple+0x20a>
 804934e:	8b 45 ec             	mov    eax,DWORD PTR [ebp-0x14]
 8049351:	03 45 d0             	add    eax,DWORD PTR [ebp-0x30]
 8049354:	89 45 d0             	mov    DWORD PTR [ebp-0x30],eax
 8049357:	8b 45 dc             	mov    eax,DWORD PTR [ebp-0x24]
 804935a:	03 45 a8             	add    eax,DWORD PTR [ebp-0x58]
 804935d:	89 45 a8             	mov    DWORD PTR [ebp-0x58],eax
 8049360:	8b 45 e0             	mov    eax,DWORD PTR [ebp-0x20]
 8049363:	03 45 ac             	add    eax,DWORD PTR [ebp-0x54]
 8049366:	89 45 ac             	mov    DWORD PTR [ebp-0x54],eax
 8049369:	8b 45 c4             	mov    eax,DWORD PTR [ebp-0x3c]
 804936c:	03 45 b0             	add    eax,DWORD PTR [ebp-0x50]
 804936f:	89 45 b0             	mov    DWORD PTR [ebp-0x50],eax
 8049372:	8b 45 f0             	mov    eax,DWORD PTR [ebp-0x10]
 8049375:	03 45 b4             	add    eax,DWORD PTR [ebp-0x4c]
 8049378:	89 45 b4             	mov    DWORD PTR [ebp-0x4c],eax
 804937b:	8b 45 d4             	mov    eax,DWORD PTR [ebp-0x2c]
 804937e:	03 45 b8             	add    eax,DWORD PTR [ebp-0x48]
 8049381:	89 45 b8             	mov    DWORD PTR [ebp-0x48],eax
 8049384:	8b 45 d8             	mov    eax,DWORD PTR [ebp-0x28]
 8049387:	03 45 bc             	add    eax,DWORD PTR [ebp-0x44]
 804938a:	89 45 bc             	mov    DWORD PTR [ebp-0x44],eax
 804938d:	8b 45 c8             	mov    eax,DWORD PTR [ebp-0x38]
 8049390:	03 45 c0             	add    eax,DWORD PTR [ebp-0x40]
 8049393:	89 45 c0             	mov    DWORD PTR [ebp-0x40],eax
 8049396:	8b 45 d0             	mov    eax,DWORD PTR [ebp-0x30]
 8049399:	81 c4 5c 01 00 00    	add    esp,0x15c
 804939f:	5e                   	pop    esi
 80493a0:	5f                   	pop    edi
 80493a1:	5d                   	pop    ebp
 80493a2:	c3                   	ret
