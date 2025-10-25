
llvm_modules/sha256_O0_pgo.elf:     file format elf32-i386


Disassembly of section .text:

08049000 <compute>:
 8049000:	55                   	push   %ebp
 8049001:	89 e5                	mov    %esp,%ebp
 8049003:	53                   	push   %ebx
 8049004:	83 ec 24             	sub    $0x24,%esp
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
 8049037:	8d 45 ec             	lea    -0x14(%ebp),%eax
 804903a:	89 04 24             	mov    %eax,(%esp)
 804903d:	c7 44 24 04 0c 00 00 	movl   $0xc,0x4(%esp)
 8049044:	00 
 8049045:	e8 06 00 00 00       	call   8049050 <sha256_simple>
 804904a:	83 c4 24             	add    $0x24,%esp
 804904d:	5b                   	pop    %ebx
 804904e:	5d                   	pop    %ebp
 804904f:	c3                   	ret

08049050 <sha256_simple>:
 8049050:	55                   	push   %ebp
 8049051:	89 e5                	mov    %esp,%ebp
 8049053:	57                   	push   %edi
 8049054:	56                   	push   %esi
 8049055:	81 ec 58 01 00 00    	sub    $0x158,%esp
 804905b:	e8 00 00 00 00       	call   8049060 <sha256_simple+0x10>
 8049060:	58                   	pop    %eax
 8049061:	81 c0 b0 20 00 00    	add    $0x20b0,%eax
 8049067:	89 85 a0 fe ff ff    	mov    %eax,-0x160(%ebp)
 804906d:	8b 45 0c             	mov    0xc(%ebp),%eax
 8049070:	8b 45 08             	mov    0x8(%ebp),%eax
 8049073:	c7 45 f4 67 e6 09 6a 	movl   $0x6a09e667,-0xc(%ebp)
 804907a:	c7 45 f0 85 ae 67 bb 	movl   $0xbb67ae85,-0x10(%ebp)
 8049081:	c7 45 ec 72 f3 6e 3c 	movl   $0x3c6ef372,-0x14(%ebp)
 8049088:	c7 45 e8 3a f5 4f a5 	movl   $0xa54ff53a,-0x18(%ebp)
 804908f:	c7 45 e4 7f 52 0e 51 	movl   $0x510e527f,-0x1c(%ebp)
 8049096:	c7 45 e0 8c 68 05 9b 	movl   $0x9b05688c,-0x20(%ebp)
 804909d:	c7 45 dc ab d9 83 1f 	movl   $0x1f83d9ab,-0x24(%ebp)
 80490a4:	c7 45 d8 19 cd e0 5b 	movl   $0x5be0cd19,-0x28(%ebp)
 80490ab:	c7 85 d4 fe ff ff 00 	movl   $0x0,-0x12c(%ebp)
 80490b2:	00 00 00 
 80490b5:	83 bd d4 fe ff ff 10 	cmpl   $0x10,-0x12c(%ebp)
 80490bc:	0f 8d a0 00 00 00    	jge    8049162 <sha256_simple+0x112>
 80490c2:	8b 85 d4 fe ff ff    	mov    -0x12c(%ebp),%eax
 80490c8:	c1 e0 02             	shl    $0x2,%eax
 80490cb:	3b 45 0c             	cmp    0xc(%ebp),%eax
 80490ce:	0f 8d 64 00 00 00    	jge    8049138 <sha256_simple+0xe8>
 80490d4:	8b 45 08             	mov    0x8(%ebp),%eax
 80490d7:	8b 8d d4 fe ff ff    	mov    -0x12c(%ebp),%ecx
 80490dd:	c1 e1 02             	shl    $0x2,%ecx
 80490e0:	0f b6 0c 08          	movzbl (%eax,%ecx,1),%ecx
 80490e4:	c1 e1 18             	shl    $0x18,%ecx
 80490e7:	8b 45 08             	mov    0x8(%ebp),%eax
 80490ea:	8b 95 d4 fe ff ff    	mov    -0x12c(%ebp),%edx
 80490f0:	c1 e2 02             	shl    $0x2,%edx
 80490f3:	0f b6 44 10 01       	movzbl 0x1(%eax,%edx,1),%eax
 80490f8:	c1 e0 10             	shl    $0x10,%eax
 80490fb:	09 c1                	or     %eax,%ecx
 80490fd:	8b 45 08             	mov    0x8(%ebp),%eax
 8049100:	8b 95 d4 fe ff ff    	mov    -0x12c(%ebp),%edx
 8049106:	c1 e2 02             	shl    $0x2,%edx
 8049109:	0f b6 44 10 02       	movzbl 0x2(%eax,%edx,1),%eax
 804910e:	c1 e0 08             	shl    $0x8,%eax
 8049111:	09 c1                	or     %eax,%ecx
 8049113:	8b 45 08             	mov    0x8(%ebp),%eax
 8049116:	8b 95 d4 fe ff ff    	mov    -0x12c(%ebp),%edx
 804911c:	c1 e2 02             	shl    $0x2,%edx
 804911f:	0f b6 44 10 03       	movzbl 0x3(%eax,%edx,1),%eax
 8049124:	09 c1                	or     %eax,%ecx
 8049126:	8b 85 d4 fe ff ff    	mov    -0x12c(%ebp),%eax
 804912c:	89 8c 85 d8 fe ff ff 	mov    %ecx,-0x128(%ebp,%eax,4)
 8049133:	e9 11 00 00 00       	jmp    8049149 <sha256_simple+0xf9>
 8049138:	8b 85 d4 fe ff ff    	mov    -0x12c(%ebp),%eax
 804913e:	c7 84 85 d8 fe ff ff 	movl   $0x0,-0x128(%ebp,%eax,4)
 8049145:	00 00 00 00 
 8049149:	e9 00 00 00 00       	jmp    804914e <sha256_simple+0xfe>
 804914e:	8b 85 d4 fe ff ff    	mov    -0x12c(%ebp),%eax
 8049154:	83 c0 01             	add    $0x1,%eax
 8049157:	89 85 d4 fe ff ff    	mov    %eax,-0x12c(%ebp)
 804915d:	e9 53 ff ff ff       	jmp    80490b5 <sha256_simple+0x65>
 8049162:	c7 85 d0 fe ff ff 10 	movl   $0x10,-0x130(%ebp)
 8049169:	00 00 00 
 804916c:	83 bd d0 fe ff ff 40 	cmpl   $0x40,-0x130(%ebp)
 8049173:	0f 8d 11 01 00 00    	jge    804928a <sha256_simple+0x23a>
 8049179:	8b 85 d0 fe ff ff    	mov    -0x130(%ebp),%eax
 804917f:	83 e8 02             	sub    $0x2,%eax
 8049182:	8b 8c 85 d8 fe ff ff 	mov    -0x128(%ebp,%eax,4),%ecx
 8049189:	c1 e9 11             	shr    $0x11,%ecx
 804918c:	8b 85 d0 fe ff ff    	mov    -0x130(%ebp),%eax
 8049192:	83 e8 02             	sub    $0x2,%eax
 8049195:	8b 84 85 d8 fe ff ff 	mov    -0x128(%ebp,%eax,4),%eax
 804919c:	c1 e0 0f             	shl    $0xf,%eax
 804919f:	09 c1                	or     %eax,%ecx
 80491a1:	8b 85 d0 fe ff ff    	mov    -0x130(%ebp),%eax
 80491a7:	83 e8 02             	sub    $0x2,%eax
 80491aa:	8b 84 85 d8 fe ff ff 	mov    -0x128(%ebp,%eax,4),%eax
 80491b1:	c1 e8 13             	shr    $0x13,%eax
 80491b4:	8b 95 d0 fe ff ff    	mov    -0x130(%ebp),%edx
 80491ba:	83 ea 02             	sub    $0x2,%edx
 80491bd:	8b 94 95 d8 fe ff ff 	mov    -0x128(%ebp,%edx,4),%edx
 80491c4:	c1 e2 0d             	shl    $0xd,%edx
 80491c7:	09 d0                	or     %edx,%eax
 80491c9:	31 c1                	xor    %eax,%ecx
 80491cb:	8b 85 d0 fe ff ff    	mov    -0x130(%ebp),%eax
 80491d1:	83 e8 02             	sub    $0x2,%eax
 80491d4:	8b 84 85 d8 fe ff ff 	mov    -0x128(%ebp,%eax,4),%eax
 80491db:	c1 e8 0a             	shr    $0xa,%eax
 80491de:	31 c1                	xor    %eax,%ecx
 80491e0:	8b 85 d0 fe ff ff    	mov    -0x130(%ebp),%eax
 80491e6:	83 e8 07             	sub    $0x7,%eax
 80491e9:	03 8c 85 d8 fe ff ff 	add    -0x128(%ebp,%eax,4),%ecx
 80491f0:	8b 85 d0 fe ff ff    	mov    -0x130(%ebp),%eax
 80491f6:	83 e8 0f             	sub    $0xf,%eax
 80491f9:	8b 84 85 d8 fe ff ff 	mov    -0x128(%ebp,%eax,4),%eax
 8049200:	c1 e8 07             	shr    $0x7,%eax
 8049203:	8b 95 d0 fe ff ff    	mov    -0x130(%ebp),%edx
 8049209:	83 ea 0f             	sub    $0xf,%edx
 804920c:	8b 94 95 d8 fe ff ff 	mov    -0x128(%ebp,%edx,4),%edx
 8049213:	c1 e2 19             	shl    $0x19,%edx
 8049216:	09 d0                	or     %edx,%eax
 8049218:	8b 95 d0 fe ff ff    	mov    -0x130(%ebp),%edx
 804921e:	83 ea 0f             	sub    $0xf,%edx
 8049221:	8b 94 95 d8 fe ff ff 	mov    -0x128(%ebp,%edx,4),%edx
 8049228:	c1 ea 12             	shr    $0x12,%edx
 804922b:	8b b5 d0 fe ff ff    	mov    -0x130(%ebp),%esi
 8049231:	83 ee 0f             	sub    $0xf,%esi
 8049234:	8b b4 b5 d8 fe ff ff 	mov    -0x128(%ebp,%esi,4),%esi
 804923b:	c1 e6 0e             	shl    $0xe,%esi
 804923e:	09 f2                	or     %esi,%edx
 8049240:	31 d0                	xor    %edx,%eax
 8049242:	8b 95 d0 fe ff ff    	mov    -0x130(%ebp),%edx
 8049248:	83 ea 0f             	sub    $0xf,%edx
 804924b:	8b 94 95 d8 fe ff ff 	mov    -0x128(%ebp,%edx,4),%edx
 8049252:	c1 ea 03             	shr    $0x3,%edx
 8049255:	31 d0                	xor    %edx,%eax
 8049257:	01 c1                	add    %eax,%ecx
 8049259:	8b 85 d0 fe ff ff    	mov    -0x130(%ebp),%eax
 804925f:	83 e8 10             	sub    $0x10,%eax
 8049262:	03 8c 85 d8 fe ff ff 	add    -0x128(%ebp,%eax,4),%ecx
 8049269:	8b 85 d0 fe ff ff    	mov    -0x130(%ebp),%eax
 804926f:	89 8c 85 d8 fe ff ff 	mov    %ecx,-0x128(%ebp,%eax,4)
 8049276:	8b 85 d0 fe ff ff    	mov    -0x130(%ebp),%eax
 804927c:	83 c0 01             	add    $0x1,%eax
 804927f:	89 85 d0 fe ff ff    	mov    %eax,-0x130(%ebp)
 8049285:	e9 e2 fe ff ff       	jmp    804916c <sha256_simple+0x11c>
 804928a:	8b 45 f4             	mov    -0xc(%ebp),%eax
 804928d:	89 85 cc fe ff ff    	mov    %eax,-0x134(%ebp)
 8049293:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8049296:	89 85 c8 fe ff ff    	mov    %eax,-0x138(%ebp)
 804929c:	8b 45 ec             	mov    -0x14(%ebp),%eax
 804929f:	89 85 c4 fe ff ff    	mov    %eax,-0x13c(%ebp)
 80492a5:	8b 45 e8             	mov    -0x18(%ebp),%eax
 80492a8:	89 85 c0 fe ff ff    	mov    %eax,-0x140(%ebp)
 80492ae:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 80492b1:	89 85 bc fe ff ff    	mov    %eax,-0x144(%ebp)
 80492b7:	8b 45 e0             	mov    -0x20(%ebp),%eax
 80492ba:	89 85 b8 fe ff ff    	mov    %eax,-0x148(%ebp)
 80492c0:	8b 45 dc             	mov    -0x24(%ebp),%eax
 80492c3:	89 85 b4 fe ff ff    	mov    %eax,-0x14c(%ebp)
 80492c9:	8b 45 d8             	mov    -0x28(%ebp),%eax
 80492cc:	89 85 b0 fe ff ff    	mov    %eax,-0x150(%ebp)
 80492d2:	c7 85 ac fe ff ff 00 	movl   $0x0,-0x154(%ebp)
 80492d9:	00 00 00 
 80492dc:	83 bd ac fe ff ff 40 	cmpl   $0x40,-0x154(%ebp)
 80492e3:	0f 8d 7d 01 00 00    	jge    8049466 <sha256_simple+0x416>
 80492e9:	8b 8d a0 fe ff ff    	mov    -0x160(%ebp),%ecx
 80492ef:	8b 85 b0 fe ff ff    	mov    -0x150(%ebp),%eax
 80492f5:	8b 95 bc fe ff ff    	mov    -0x144(%ebp),%edx
 80492fb:	c1 ea 06             	shr    $0x6,%edx
 80492fe:	8b b5 bc fe ff ff    	mov    -0x144(%ebp),%esi
 8049304:	c1 e6 1a             	shl    $0x1a,%esi
 8049307:	09 f2                	or     %esi,%edx
 8049309:	8b b5 bc fe ff ff    	mov    -0x144(%ebp),%esi
 804930f:	c1 ee 0b             	shr    $0xb,%esi
 8049312:	8b bd bc fe ff ff    	mov    -0x144(%ebp),%edi
 8049318:	c1 e7 15             	shl    $0x15,%edi
 804931b:	09 fe                	or     %edi,%esi
 804931d:	31 f2                	xor    %esi,%edx
 804931f:	8b b5 bc fe ff ff    	mov    -0x144(%ebp),%esi
 8049325:	c1 ee 19             	shr    $0x19,%esi
 8049328:	8b bd bc fe ff ff    	mov    -0x144(%ebp),%edi
 804932e:	c1 e7 07             	shl    $0x7,%edi
 8049331:	09 fe                	or     %edi,%esi
 8049333:	31 f2                	xor    %esi,%edx
 8049335:	01 d0                	add    %edx,%eax
 8049337:	8b 95 bc fe ff ff    	mov    -0x144(%ebp),%edx
 804933d:	23 95 b8 fe ff ff    	and    -0x148(%ebp),%edx
 8049343:	8b b5 bc fe ff ff    	mov    -0x144(%ebp),%esi
 8049349:	83 f6 ff             	xor    $0xffffffff,%esi
 804934c:	23 b5 b4 fe ff ff    	and    -0x14c(%ebp),%esi
 8049352:	31 f2                	xor    %esi,%edx
 8049354:	01 d0                	add    %edx,%eax
 8049356:	8b 95 ac fe ff ff    	mov    -0x154(%ebp),%edx
 804935c:	03 84 91 00 ef ff ff 	add    -0x1100(%ecx,%edx,4),%eax
 8049363:	8b 8d ac fe ff ff    	mov    -0x154(%ebp),%ecx
 8049369:	03 84 8d d8 fe ff ff 	add    -0x128(%ebp,%ecx,4),%eax
 8049370:	89 85 a8 fe ff ff    	mov    %eax,-0x158(%ebp)
 8049376:	8b 85 cc fe ff ff    	mov    -0x134(%ebp),%eax
 804937c:	c1 e8 02             	shr    $0x2,%eax
 804937f:	8b 8d cc fe ff ff    	mov    -0x134(%ebp),%ecx
 8049385:	c1 e1 1e             	shl    $0x1e,%ecx
 8049388:	09 c8                	or     %ecx,%eax
 804938a:	8b 8d cc fe ff ff    	mov    -0x134(%ebp),%ecx
 8049390:	c1 e9 0d             	shr    $0xd,%ecx
 8049393:	8b 95 cc fe ff ff    	mov    -0x134(%ebp),%edx
 8049399:	c1 e2 13             	shl    $0x13,%edx
 804939c:	09 d1                	or     %edx,%ecx
 804939e:	31 c8                	xor    %ecx,%eax
 80493a0:	8b 8d cc fe ff ff    	mov    -0x134(%ebp),%ecx
 80493a6:	c1 e9 16             	shr    $0x16,%ecx
 80493a9:	8b 95 cc fe ff ff    	mov    -0x134(%ebp),%edx
 80493af:	c1 e2 0a             	shl    $0xa,%edx
 80493b2:	09 d1                	or     %edx,%ecx
 80493b4:	31 c8                	xor    %ecx,%eax
 80493b6:	8b 8d cc fe ff ff    	mov    -0x134(%ebp),%ecx
 80493bc:	23 8d c8 fe ff ff    	and    -0x138(%ebp),%ecx
 80493c2:	8b 95 cc fe ff ff    	mov    -0x134(%ebp),%edx
 80493c8:	23 95 c4 fe ff ff    	and    -0x13c(%ebp),%edx
 80493ce:	31 d1                	xor    %edx,%ecx
 80493d0:	8b 95 c8 fe ff ff    	mov    -0x138(%ebp),%edx
 80493d6:	23 95 c4 fe ff ff    	and    -0x13c(%ebp),%edx
 80493dc:	31 d1                	xor    %edx,%ecx
 80493de:	01 c8                	add    %ecx,%eax
 80493e0:	89 85 a4 fe ff ff    	mov    %eax,-0x15c(%ebp)
 80493e6:	8b 85 b4 fe ff ff    	mov    -0x14c(%ebp),%eax
 80493ec:	89 85 b0 fe ff ff    	mov    %eax,-0x150(%ebp)
 80493f2:	8b 85 b8 fe ff ff    	mov    -0x148(%ebp),%eax
 80493f8:	89 85 b4 fe ff ff    	mov    %eax,-0x14c(%ebp)
 80493fe:	8b 85 bc fe ff ff    	mov    -0x144(%ebp),%eax
 8049404:	89 85 b8 fe ff ff    	mov    %eax,-0x148(%ebp)
 804940a:	8b 85 c0 fe ff ff    	mov    -0x140(%ebp),%eax
 8049410:	03 85 a8 fe ff ff    	add    -0x158(%ebp),%eax
 8049416:	89 85 bc fe ff ff    	mov    %eax,-0x144(%ebp)
 804941c:	8b 85 c4 fe ff ff    	mov    -0x13c(%ebp),%eax
 8049422:	89 85 c0 fe ff ff    	mov    %eax,-0x140(%ebp)
 8049428:	8b 85 c8 fe ff ff    	mov    -0x138(%ebp),%eax
 804942e:	89 85 c4 fe ff ff    	mov    %eax,-0x13c(%ebp)
 8049434:	8b 85 cc fe ff ff    	mov    -0x134(%ebp),%eax
 804943a:	89 85 c8 fe ff ff    	mov    %eax,-0x138(%ebp)
 8049440:	8b 85 a8 fe ff ff    	mov    -0x158(%ebp),%eax
 8049446:	03 85 a4 fe ff ff    	add    -0x15c(%ebp),%eax
 804944c:	89 85 cc fe ff ff    	mov    %eax,-0x134(%ebp)
 8049452:	8b 85 ac fe ff ff    	mov    -0x154(%ebp),%eax
 8049458:	83 c0 01             	add    $0x1,%eax
 804945b:	89 85 ac fe ff ff    	mov    %eax,-0x154(%ebp)
 8049461:	e9 76 fe ff ff       	jmp    80492dc <sha256_simple+0x28c>
 8049466:	8b 85 cc fe ff ff    	mov    -0x134(%ebp),%eax
 804946c:	03 45 f4             	add    -0xc(%ebp),%eax
 804946f:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8049472:	8b 85 c8 fe ff ff    	mov    -0x138(%ebp),%eax
 8049478:	03 45 f0             	add    -0x10(%ebp),%eax
 804947b:	89 45 f0             	mov    %eax,-0x10(%ebp)
 804947e:	8b 85 c4 fe ff ff    	mov    -0x13c(%ebp),%eax
 8049484:	03 45 ec             	add    -0x14(%ebp),%eax
 8049487:	89 45 ec             	mov    %eax,-0x14(%ebp)
 804948a:	8b 85 c0 fe ff ff    	mov    -0x140(%ebp),%eax
 8049490:	03 45 e8             	add    -0x18(%ebp),%eax
 8049493:	89 45 e8             	mov    %eax,-0x18(%ebp)
 8049496:	8b 85 bc fe ff ff    	mov    -0x144(%ebp),%eax
 804949c:	03 45 e4             	add    -0x1c(%ebp),%eax
 804949f:	89 45 e4             	mov    %eax,-0x1c(%ebp)
 80494a2:	8b 85 b8 fe ff ff    	mov    -0x148(%ebp),%eax
 80494a8:	03 45 e0             	add    -0x20(%ebp),%eax
 80494ab:	89 45 e0             	mov    %eax,-0x20(%ebp)
 80494ae:	8b 85 b4 fe ff ff    	mov    -0x14c(%ebp),%eax
 80494b4:	03 45 dc             	add    -0x24(%ebp),%eax
 80494b7:	89 45 dc             	mov    %eax,-0x24(%ebp)
 80494ba:	8b 85 b0 fe ff ff    	mov    -0x150(%ebp),%eax
 80494c0:	03 45 d8             	add    -0x28(%ebp),%eax
 80494c3:	89 45 d8             	mov    %eax,-0x28(%ebp)
 80494c6:	8b 45 f4             	mov    -0xc(%ebp),%eax
 80494c9:	81 c4 58 01 00 00    	add    $0x158,%esp
 80494cf:	5e                   	pop    %esi
 80494d0:	5f                   	pop    %edi
 80494d1:	5d                   	pop    %ebp
 80494d2:	c3                   	ret
