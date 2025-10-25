
llvm_modules/sha256_O1_pgo.elf:     file format elf32-i386


Disassembly of section .text:

08049000 <compute>:
 8049000:	55                   	push   %ebp
 8049001:	89 e5                	mov    %esp,%ebp
 8049003:	53                   	push   %ebx
 8049004:	83 ec 14             	sub    $0x14,%esp
 8049007:	e8 00 00 00 00       	call   804900c <compute+0xc>
 804900c:	5b                   	pop    %ebx
 804900d:	81 c3 04 21 00 00    	add    $0x2104,%ebx
 8049013:	8b 83 f0 ee ff ff    	mov    -0x1110(%ebx),%eax
 8049019:	89 45 ec             	mov    %eax,-0x14(%ebp)
 804901c:	8b 83 f4 ee ff ff    	mov    -0x110c(%ebx),%eax
 8049022:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8049025:	8b 83 f8 ee ff ff    	mov    -0x1108(%ebx),%eax
 804902b:	89 45 f4             	mov    %eax,-0xc(%ebp)
 804902e:	8b 83 fc ee ff ff    	mov    -0x1104(%ebx),%eax
 8049034:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8049037:	8d 4d ec             	lea    -0x14(%ebp),%ecx
 804903a:	e8 11 00 00 00       	call   8049050 <sha256_simple>
 804903f:	83 c4 14             	add    $0x14,%esp
 8049042:	5b                   	pop    %ebx
 8049043:	5d                   	pop    %ebp
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
 8049050:	55                   	push   %ebp
 8049051:	89 e5                	mov    %esp,%ebp
 8049053:	57                   	push   %edi
 8049054:	56                   	push   %esi
 8049055:	81 ec 5c 01 00 00    	sub    $0x15c,%esp
 804905b:	e8 00 00 00 00       	call   8049060 <sha256_simple+0x10>
 8049060:	58                   	pop    %eax
 8049061:	81 c0 b0 20 00 00    	add    $0x20b0,%eax
 8049067:	89 4d cc             	mov    %ecx,-0x34(%ebp)
 804906a:	c7 45 9c 0c 00 00 00 	movl   $0xc,-0x64(%ebp)
 8049071:	c7 45 d0 67 e6 09 6a 	movl   $0x6a09e667,-0x30(%ebp)
 8049078:	c7 45 a8 85 ae 67 bb 	movl   $0xbb67ae85,-0x58(%ebp)
 804907f:	c7 45 ac 72 f3 6e 3c 	movl   $0x3c6ef372,-0x54(%ebp)
 8049086:	c7 45 b0 3a f5 4f a5 	movl   $0xa54ff53a,-0x50(%ebp)
 804908d:	c7 45 b4 7f 52 0e 51 	movl   $0x510e527f,-0x4c(%ebp)
 8049094:	c7 45 b8 8c 68 05 9b 	movl   $0x9b05688c,-0x48(%ebp)
 804909b:	c7 45 bc ab d9 83 1f 	movl   $0x1f83d9ab,-0x44(%ebp)
 80490a2:	c7 45 c0 19 cd e0 5b 	movl   $0x5be0cd19,-0x40(%ebp)
 80490a9:	c7 45 e8 00 00 00 00 	movl   $0x0,-0x18(%ebp)
 80490b0:	83 7d e8 10          	cmpl   $0x10,-0x18(%ebp)
 80490b4:	7d 78                	jge    804912e <sha256_simple+0xde>
 80490b6:	8b 4d e8             	mov    -0x18(%ebp),%ecx
 80490b9:	c1 e1 02             	shl    $0x2,%ecx
 80490bc:	3b 4d 9c             	cmp    -0x64(%ebp),%ecx
 80490bf:	7d 52                	jge    8049113 <sha256_simple+0xc3>
 80490c1:	8b 4d cc             	mov    -0x34(%ebp),%ecx
 80490c4:	8b 55 e8             	mov    -0x18(%ebp),%edx
 80490c7:	c1 e2 02             	shl    $0x2,%edx
 80490ca:	0f b6 0c 11          	movzbl (%ecx,%edx,1),%ecx
 80490ce:	c1 e1 18             	shl    $0x18,%ecx
 80490d1:	8b 55 cc             	mov    -0x34(%ebp),%edx
 80490d4:	8b 75 e8             	mov    -0x18(%ebp),%esi
 80490d7:	c1 e6 02             	shl    $0x2,%esi
 80490da:	0f b6 54 32 01       	movzbl 0x1(%edx,%esi,1),%edx
 80490df:	c1 e2 10             	shl    $0x10,%edx
 80490e2:	09 d1                	or     %edx,%ecx
 80490e4:	8b 55 cc             	mov    -0x34(%ebp),%edx
 80490e7:	8b 75 e8             	mov    -0x18(%ebp),%esi
 80490ea:	c1 e6 02             	shl    $0x2,%esi
 80490ed:	0f b6 54 32 02       	movzbl 0x2(%edx,%esi,1),%edx
 80490f2:	c1 e2 08             	shl    $0x8,%edx
 80490f5:	09 d1                	or     %edx,%ecx
 80490f7:	8b 55 cc             	mov    -0x34(%ebp),%edx
 80490fa:	8b 75 e8             	mov    -0x18(%ebp),%esi
 80490fd:	c1 e6 02             	shl    $0x2,%esi
 8049100:	0f b6 54 32 03       	movzbl 0x3(%edx,%esi,1),%edx
 8049105:	09 d1                	or     %edx,%ecx
 8049107:	8b 55 e8             	mov    -0x18(%ebp),%edx
 804910a:	89 8c 95 9c fe ff ff 	mov    %ecx,-0x164(%ebp,%edx,4)
 8049111:	eb 0e                	jmp    8049121 <sha256_simple+0xd1>
 8049113:	8b 4d e8             	mov    -0x18(%ebp),%ecx
 8049116:	c7 84 8d 9c fe ff ff 	movl   $0x0,-0x164(%ebp,%ecx,4)
 804911d:	00 00 00 00 
 8049121:	eb 00                	jmp    8049123 <sha256_simple+0xd3>
 8049123:	8b 4d e8             	mov    -0x18(%ebp),%ecx
 8049126:	83 c1 01             	add    $0x1,%ecx
 8049129:	89 4d e8             	mov    %ecx,-0x18(%ebp)
 804912c:	eb 82                	jmp    80490b0 <sha256_simple+0x60>
 804912e:	c7 45 f4 10 00 00 00 	movl   $0x10,-0xc(%ebp)
 8049135:	83 7d f4 40          	cmpl   $0x40,-0xc(%ebp)
 8049139:	0f 8d e4 00 00 00    	jge    8049223 <sha256_simple+0x1d3>
 804913f:	8b 4d f4             	mov    -0xc(%ebp),%ecx
 8049142:	83 e9 02             	sub    $0x2,%ecx
 8049145:	8b 8c 8d 9c fe ff ff 	mov    -0x164(%ebp,%ecx,4),%ecx
 804914c:	c1 e9 11             	shr    $0x11,%ecx
 804914f:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8049152:	83 ea 02             	sub    $0x2,%edx
 8049155:	8b 94 95 9c fe ff ff 	mov    -0x164(%ebp,%edx,4),%edx
 804915c:	c1 e2 0f             	shl    $0xf,%edx
 804915f:	09 d1                	or     %edx,%ecx
 8049161:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8049164:	83 ea 02             	sub    $0x2,%edx
 8049167:	8b 94 95 9c fe ff ff 	mov    -0x164(%ebp,%edx,4),%edx
 804916e:	c1 ea 13             	shr    $0x13,%edx
 8049171:	8b 75 f4             	mov    -0xc(%ebp),%esi
 8049174:	83 ee 02             	sub    $0x2,%esi
 8049177:	8b b4 b5 9c fe ff ff 	mov    -0x164(%ebp,%esi,4),%esi
 804917e:	c1 e6 0d             	shl    $0xd,%esi
 8049181:	09 f2                	or     %esi,%edx
 8049183:	31 d1                	xor    %edx,%ecx
 8049185:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8049188:	83 ea 02             	sub    $0x2,%edx
 804918b:	8b 94 95 9c fe ff ff 	mov    -0x164(%ebp,%edx,4),%edx
 8049192:	c1 ea 0a             	shr    $0xa,%edx
 8049195:	31 d1                	xor    %edx,%ecx
 8049197:	8b 55 f4             	mov    -0xc(%ebp),%edx
 804919a:	83 ea 07             	sub    $0x7,%edx
 804919d:	03 8c 95 9c fe ff ff 	add    -0x164(%ebp,%edx,4),%ecx
 80491a4:	8b 55 f4             	mov    -0xc(%ebp),%edx
 80491a7:	83 ea 0f             	sub    $0xf,%edx
 80491aa:	8b 94 95 9c fe ff ff 	mov    -0x164(%ebp,%edx,4),%edx
 80491b1:	c1 ea 07             	shr    $0x7,%edx
 80491b4:	8b 75 f4             	mov    -0xc(%ebp),%esi
 80491b7:	83 ee 0f             	sub    $0xf,%esi
 80491ba:	8b b4 b5 9c fe ff ff 	mov    -0x164(%ebp,%esi,4),%esi
 80491c1:	c1 e6 19             	shl    $0x19,%esi
 80491c4:	09 f2                	or     %esi,%edx
 80491c6:	8b 75 f4             	mov    -0xc(%ebp),%esi
 80491c9:	83 ee 0f             	sub    $0xf,%esi
 80491cc:	8b b4 b5 9c fe ff ff 	mov    -0x164(%ebp,%esi,4),%esi
 80491d3:	c1 ee 12             	shr    $0x12,%esi
 80491d6:	8b 7d f4             	mov    -0xc(%ebp),%edi
 80491d9:	83 ef 0f             	sub    $0xf,%edi
 80491dc:	8b bc bd 9c fe ff ff 	mov    -0x164(%ebp,%edi,4),%edi
 80491e3:	c1 e7 0e             	shl    $0xe,%edi
 80491e6:	09 fe                	or     %edi,%esi
 80491e8:	31 f2                	xor    %esi,%edx
 80491ea:	8b 75 f4             	mov    -0xc(%ebp),%esi
 80491ed:	83 ee 0f             	sub    $0xf,%esi
 80491f0:	8b b4 b5 9c fe ff ff 	mov    -0x164(%ebp,%esi,4),%esi
 80491f7:	c1 ee 03             	shr    $0x3,%esi
 80491fa:	31 f2                	xor    %esi,%edx
 80491fc:	01 d1                	add    %edx,%ecx
 80491fe:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8049201:	83 ea 10             	sub    $0x10,%edx
 8049204:	03 8c 95 9c fe ff ff 	add    -0x164(%ebp,%edx,4),%ecx
 804920b:	8b 55 f4             	mov    -0xc(%ebp),%edx
 804920e:	89 8c 95 9c fe ff ff 	mov    %ecx,-0x164(%ebp,%edx,4)
 8049215:	8b 4d f4             	mov    -0xc(%ebp),%ecx
 8049218:	83 c1 01             	add    $0x1,%ecx
 804921b:	89 4d f4             	mov    %ecx,-0xc(%ebp)
 804921e:	e9 12 ff ff ff       	jmp    8049135 <sha256_simple+0xe5>
 8049223:	8b 4d d0             	mov    -0x30(%ebp),%ecx
 8049226:	89 4d ec             	mov    %ecx,-0x14(%ebp)
 8049229:	8b 4d a8             	mov    -0x58(%ebp),%ecx
 804922c:	89 4d dc             	mov    %ecx,-0x24(%ebp)
 804922f:	8b 4d ac             	mov    -0x54(%ebp),%ecx
 8049232:	89 4d e0             	mov    %ecx,-0x20(%ebp)
 8049235:	8b 4d b0             	mov    -0x50(%ebp),%ecx
 8049238:	89 4d c4             	mov    %ecx,-0x3c(%ebp)
 804923b:	8b 4d b4             	mov    -0x4c(%ebp),%ecx
 804923e:	89 4d f0             	mov    %ecx,-0x10(%ebp)
 8049241:	8b 4d b8             	mov    -0x48(%ebp),%ecx
 8049244:	89 4d d4             	mov    %ecx,-0x2c(%ebp)
 8049247:	8b 4d bc             	mov    -0x44(%ebp),%ecx
 804924a:	89 4d d8             	mov    %ecx,-0x28(%ebp)
 804924d:	8b 4d c0             	mov    -0x40(%ebp),%ecx
 8049250:	89 4d c8             	mov    %ecx,-0x38(%ebp)
 8049253:	c7 45 e4 00 00 00 00 	movl   $0x0,-0x1c(%ebp)
 804925a:	83 7d e4 40          	cmpl   $0x40,-0x1c(%ebp)
 804925e:	0f 8d ea 00 00 00    	jge    804934e <sha256_simple+0x2fe>
 8049264:	8b 4d c8             	mov    -0x38(%ebp),%ecx
 8049267:	8b 55 f0             	mov    -0x10(%ebp),%edx
 804926a:	c1 ea 06             	shr    $0x6,%edx
 804926d:	8b 75 f0             	mov    -0x10(%ebp),%esi
 8049270:	c1 e6 1a             	shl    $0x1a,%esi
 8049273:	09 f2                	or     %esi,%edx
 8049275:	8b 75 f0             	mov    -0x10(%ebp),%esi
 8049278:	c1 ee 0b             	shr    $0xb,%esi
 804927b:	8b 7d f0             	mov    -0x10(%ebp),%edi
 804927e:	c1 e7 15             	shl    $0x15,%edi
 8049281:	09 fe                	or     %edi,%esi
 8049283:	31 f2                	xor    %esi,%edx
 8049285:	8b 75 f0             	mov    -0x10(%ebp),%esi
 8049288:	c1 ee 19             	shr    $0x19,%esi
 804928b:	8b 7d f0             	mov    -0x10(%ebp),%edi
 804928e:	c1 e7 07             	shl    $0x7,%edi
 8049291:	09 fe                	or     %edi,%esi
 8049293:	31 f2                	xor    %esi,%edx
 8049295:	01 d1                	add    %edx,%ecx
 8049297:	8b 55 f0             	mov    -0x10(%ebp),%edx
 804929a:	23 55 d4             	and    -0x2c(%ebp),%edx
 804929d:	8b 75 f0             	mov    -0x10(%ebp),%esi
 80492a0:	83 f6 ff             	xor    $0xffffffff,%esi
 80492a3:	23 75 d8             	and    -0x28(%ebp),%esi
 80492a6:	31 f2                	xor    %esi,%edx
 80492a8:	01 d1                	add    %edx,%ecx
 80492aa:	8b 55 e4             	mov    -0x1c(%ebp),%edx
 80492ad:	03 8c 90 00 ef ff ff 	add    -0x1100(%eax,%edx,4),%ecx
 80492b4:	8b 55 e4             	mov    -0x1c(%ebp),%edx
 80492b7:	03 8c 95 9c fe ff ff 	add    -0x164(%ebp,%edx,4),%ecx
 80492be:	89 4d a4             	mov    %ecx,-0x5c(%ebp)
 80492c1:	8b 4d ec             	mov    -0x14(%ebp),%ecx
 80492c4:	c1 e9 02             	shr    $0x2,%ecx
 80492c7:	8b 55 ec             	mov    -0x14(%ebp),%edx
 80492ca:	c1 e2 1e             	shl    $0x1e,%edx
 80492cd:	09 d1                	or     %edx,%ecx
 80492cf:	8b 55 ec             	mov    -0x14(%ebp),%edx
 80492d2:	c1 ea 0d             	shr    $0xd,%edx
 80492d5:	8b 75 ec             	mov    -0x14(%ebp),%esi
 80492d8:	c1 e6 13             	shl    $0x13,%esi
 80492db:	09 f2                	or     %esi,%edx
 80492dd:	31 d1                	xor    %edx,%ecx
 80492df:	8b 55 ec             	mov    -0x14(%ebp),%edx
 80492e2:	c1 ea 16             	shr    $0x16,%edx
 80492e5:	8b 75 ec             	mov    -0x14(%ebp),%esi
 80492e8:	c1 e6 0a             	shl    $0xa,%esi
 80492eb:	09 f2                	or     %esi,%edx
 80492ed:	31 d1                	xor    %edx,%ecx
 80492ef:	8b 55 ec             	mov    -0x14(%ebp),%edx
 80492f2:	23 55 dc             	and    -0x24(%ebp),%edx
 80492f5:	8b 75 ec             	mov    -0x14(%ebp),%esi
 80492f8:	23 75 e0             	and    -0x20(%ebp),%esi
 80492fb:	31 f2                	xor    %esi,%edx
 80492fd:	8b 75 dc             	mov    -0x24(%ebp),%esi
 8049300:	23 75 e0             	and    -0x20(%ebp),%esi
 8049303:	31 f2                	xor    %esi,%edx
 8049305:	01 d1                	add    %edx,%ecx
 8049307:	89 4d a0             	mov    %ecx,-0x60(%ebp)
 804930a:	8b 4d d8             	mov    -0x28(%ebp),%ecx
 804930d:	89 4d c8             	mov    %ecx,-0x38(%ebp)
 8049310:	8b 4d d4             	mov    -0x2c(%ebp),%ecx
 8049313:	89 4d d8             	mov    %ecx,-0x28(%ebp)
 8049316:	8b 4d f0             	mov    -0x10(%ebp),%ecx
 8049319:	89 4d d4             	mov    %ecx,-0x2c(%ebp)
 804931c:	8b 4d c4             	mov    -0x3c(%ebp),%ecx
 804931f:	03 4d a4             	add    -0x5c(%ebp),%ecx
 8049322:	89 4d f0             	mov    %ecx,-0x10(%ebp)
 8049325:	8b 4d e0             	mov    -0x20(%ebp),%ecx
 8049328:	89 4d c4             	mov    %ecx,-0x3c(%ebp)
 804932b:	8b 4d dc             	mov    -0x24(%ebp),%ecx
 804932e:	89 4d e0             	mov    %ecx,-0x20(%ebp)
 8049331:	8b 4d ec             	mov    -0x14(%ebp),%ecx
 8049334:	89 4d dc             	mov    %ecx,-0x24(%ebp)
 8049337:	8b 4d a4             	mov    -0x5c(%ebp),%ecx
 804933a:	03 4d a0             	add    -0x60(%ebp),%ecx
 804933d:	89 4d ec             	mov    %ecx,-0x14(%ebp)
 8049340:	8b 4d e4             	mov    -0x1c(%ebp),%ecx
 8049343:	83 c1 01             	add    $0x1,%ecx
 8049346:	89 4d e4             	mov    %ecx,-0x1c(%ebp)
 8049349:	e9 0c ff ff ff       	jmp    804925a <sha256_simple+0x20a>
 804934e:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8049351:	03 45 d0             	add    -0x30(%ebp),%eax
 8049354:	89 45 d0             	mov    %eax,-0x30(%ebp)
 8049357:	8b 45 dc             	mov    -0x24(%ebp),%eax
 804935a:	03 45 a8             	add    -0x58(%ebp),%eax
 804935d:	89 45 a8             	mov    %eax,-0x58(%ebp)
 8049360:	8b 45 e0             	mov    -0x20(%ebp),%eax
 8049363:	03 45 ac             	add    -0x54(%ebp),%eax
 8049366:	89 45 ac             	mov    %eax,-0x54(%ebp)
 8049369:	8b 45 c4             	mov    -0x3c(%ebp),%eax
 804936c:	03 45 b0             	add    -0x50(%ebp),%eax
 804936f:	89 45 b0             	mov    %eax,-0x50(%ebp)
 8049372:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8049375:	03 45 b4             	add    -0x4c(%ebp),%eax
 8049378:	89 45 b4             	mov    %eax,-0x4c(%ebp)
 804937b:	8b 45 d4             	mov    -0x2c(%ebp),%eax
 804937e:	03 45 b8             	add    -0x48(%ebp),%eax
 8049381:	89 45 b8             	mov    %eax,-0x48(%ebp)
 8049384:	8b 45 d8             	mov    -0x28(%ebp),%eax
 8049387:	03 45 bc             	add    -0x44(%ebp),%eax
 804938a:	89 45 bc             	mov    %eax,-0x44(%ebp)
 804938d:	8b 45 c8             	mov    -0x38(%ebp),%eax
 8049390:	03 45 c0             	add    -0x40(%ebp),%eax
 8049393:	89 45 c0             	mov    %eax,-0x40(%ebp)
 8049396:	8b 45 d0             	mov    -0x30(%ebp),%eax
 8049399:	81 c4 5c 01 00 00    	add    $0x15c,%esp
 804939f:	5e                   	pop    %esi
 80493a0:	5f                   	pop    %edi
 80493a1:	5d                   	pop    %ebp
 80493a2:	c3                   	ret
