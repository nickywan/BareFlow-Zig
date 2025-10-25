
llvm_modules/matrix_mul_O0_pgo.elf:     file format elf32-i386


Disassembly of section .text:

08049000 <compute>:
 8049000:	55                   	push   %ebp
 8049001:	89 e5                	mov    %esp,%ebp
 8049003:	53                   	push   %ebx
 8049004:	83 ec 14             	sub    $0x14,%esp
 8049007:	e8 00 00 00 00       	call   804900c <compute+0xc>
 804900c:	5b                   	pop    %ebx
 804900d:	81 c3 f4 0f 00 00    	add    $0xff4,%ebx
 8049013:	89 5d f8             	mov    %ebx,-0x8(%ebp)
 8049016:	8d 83 0c 00 00 00    	lea    0xc(%ebx),%eax
 804901c:	89 04 24             	mov    %eax,(%esp)
 804901f:	c7 44 24 04 2a 00 00 	movl   $0x2a,0x4(%esp)
 8049026:	00 
 8049027:	e8 64 00 00 00       	call   8049090 <init_matrix>
 804902c:	8b 5d f8             	mov    -0x8(%ebp),%ebx
 804902f:	8d 83 0c 01 00 00    	lea    0x10c(%ebx),%eax
 8049035:	89 04 24             	mov    %eax,(%esp)
 8049038:	c7 44 24 04 11 00 00 	movl   $0x11,0x4(%esp)
 804903f:	00 
 8049040:	e8 4b 00 00 00       	call   8049090 <init_matrix>
 8049045:	8b 5d f8             	mov    -0x8(%ebp),%ebx
 8049048:	8d 93 0c 00 00 00    	lea    0xc(%ebx),%edx
 804904e:	8d 8b 0c 01 00 00    	lea    0x10c(%ebx),%ecx
 8049054:	8d 83 0c 02 00 00    	lea    0x20c(%ebx),%eax
 804905a:	89 14 24             	mov    %edx,(%esp)
 804905d:	89 4c 24 04          	mov    %ecx,0x4(%esp)
 8049061:	89 44 24 08          	mov    %eax,0x8(%esp)
 8049065:	e8 b6 00 00 00       	call   8049120 <matrix_multiply>
 804906a:	8b 5d f8             	mov    -0x8(%ebp),%ebx
 804906d:	8d 83 0c 02 00 00    	lea    0x20c(%ebx),%eax
 8049073:	89 04 24             	mov    %eax,(%esp)
 8049076:	e8 65 01 00 00       	call   80491e0 <checksum>
 804907b:	83 c4 14             	add    $0x14,%esp
 804907e:	5b                   	pop    %ebx
 804907f:	5d                   	pop    %ebp
 8049080:	c3                   	ret
 8049081:	90                   	nop
 8049082:	90                   	nop
 8049083:	90                   	nop
 8049084:	90                   	nop
 8049085:	90                   	nop
 8049086:	90                   	nop
 8049087:	90                   	nop
 8049088:	90                   	nop
 8049089:	90                   	nop
 804908a:	90                   	nop
 804908b:	90                   	nop
 804908c:	90                   	nop
 804908d:	90                   	nop
 804908e:	90                   	nop
 804908f:	90                   	nop

08049090 <init_matrix>:
 8049090:	55                   	push   %ebp
 8049091:	89 e5                	mov    %esp,%ebp
 8049093:	83 ec 0c             	sub    $0xc,%esp
 8049096:	8b 45 0c             	mov    0xc(%ebp),%eax
 8049099:	8b 45 08             	mov    0x8(%ebp),%eax
 804909c:	8b 45 0c             	mov    0xc(%ebp),%eax
 804909f:	89 45 fc             	mov    %eax,-0x4(%ebp)
 80490a2:	c7 45 f8 00 00 00 00 	movl   $0x0,-0x8(%ebp)
 80490a9:	83 7d f8 08          	cmpl   $0x8,-0x8(%ebp)
 80490ad:	0f 8d 5b 00 00 00    	jge    804910e <init_matrix+0x7e>
 80490b3:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 80490ba:	83 7d f4 08          	cmpl   $0x8,-0xc(%ebp)
 80490be:	0f 8d 37 00 00 00    	jge    80490fb <init_matrix+0x6b>
 80490c4:	6b 45 fc 0d          	imul   $0xd,-0x4(%ebp),%eax
 80490c8:	83 c0 07             	add    $0x7,%eax
 80490cb:	b9 64 00 00 00       	mov    $0x64,%ecx
 80490d0:	99                   	cltd
 80490d1:	f7 f9                	idiv   %ecx
 80490d3:	8b 45 08             	mov    0x8(%ebp),%eax
 80490d6:	8b 4d f8             	mov    -0x8(%ebp),%ecx
 80490d9:	c1 e1 05             	shl    $0x5,%ecx
 80490dc:	01 c8                	add    %ecx,%eax
 80490de:	8b 4d f4             	mov    -0xc(%ebp),%ecx
 80490e1:	89 14 88             	mov    %edx,(%eax,%ecx,4)
 80490e4:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80490e7:	83 c0 01             	add    $0x1,%eax
 80490ea:	89 45 fc             	mov    %eax,-0x4(%ebp)
 80490ed:	8b 45 f4             	mov    -0xc(%ebp),%eax
 80490f0:	83 c0 01             	add    $0x1,%eax
 80490f3:	89 45 f4             	mov    %eax,-0xc(%ebp)
 80490f6:	e9 bf ff ff ff       	jmp    80490ba <init_matrix+0x2a>
 80490fb:	e9 00 00 00 00       	jmp    8049100 <init_matrix+0x70>
 8049100:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8049103:	83 c0 01             	add    $0x1,%eax
 8049106:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8049109:	e9 9b ff ff ff       	jmp    80490a9 <init_matrix+0x19>
 804910e:	83 c4 0c             	add    $0xc,%esp
 8049111:	5d                   	pop    %ebp
 8049112:	c3                   	ret
 8049113:	90                   	nop
 8049114:	90                   	nop
 8049115:	90                   	nop
 8049116:	90                   	nop
 8049117:	90                   	nop
 8049118:	90                   	nop
 8049119:	90                   	nop
 804911a:	90                   	nop
 804911b:	90                   	nop
 804911c:	90                   	nop
 804911d:	90                   	nop
 804911e:	90                   	nop
 804911f:	90                   	nop

08049120 <matrix_multiply>:
 8049120:	55                   	push   %ebp
 8049121:	89 e5                	mov    %esp,%ebp
 8049123:	83 ec 10             	sub    $0x10,%esp
 8049126:	8b 45 10             	mov    0x10(%ebp),%eax
 8049129:	8b 45 0c             	mov    0xc(%ebp),%eax
 804912c:	8b 45 08             	mov    0x8(%ebp),%eax
 804912f:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
 8049136:	83 7d fc 08          	cmpl   $0x8,-0x4(%ebp)
 804913a:	0f 8d 95 00 00 00    	jge    80491d5 <matrix_multiply+0xb5>
 8049140:	c7 45 f8 00 00 00 00 	movl   $0x0,-0x8(%ebp)
 8049147:	83 7d f8 08          	cmpl   $0x8,-0x8(%ebp)
 804914b:	0f 8d 71 00 00 00    	jge    80491c2 <matrix_multiply+0xa2>
 8049151:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 8049158:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
 804915f:	83 7d f0 08          	cmpl   $0x8,-0x10(%ebp)
 8049163:	0f 8d 37 00 00 00    	jge    80491a0 <matrix_multiply+0x80>
 8049169:	8b 45 08             	mov    0x8(%ebp),%eax
 804916c:	8b 4d fc             	mov    -0x4(%ebp),%ecx
 804916f:	c1 e1 05             	shl    $0x5,%ecx
 8049172:	01 c8                	add    %ecx,%eax
 8049174:	8b 4d f0             	mov    -0x10(%ebp),%ecx
 8049177:	8b 04 88             	mov    (%eax,%ecx,4),%eax
 804917a:	8b 4d 0c             	mov    0xc(%ebp),%ecx
 804917d:	8b 55 f0             	mov    -0x10(%ebp),%edx
 8049180:	c1 e2 05             	shl    $0x5,%edx
 8049183:	01 d1                	add    %edx,%ecx
 8049185:	8b 55 f8             	mov    -0x8(%ebp),%edx
 8049188:	0f af 04 91          	imul   (%ecx,%edx,4),%eax
 804918c:	03 45 f4             	add    -0xc(%ebp),%eax
 804918f:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8049192:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8049195:	83 c0 01             	add    $0x1,%eax
 8049198:	89 45 f0             	mov    %eax,-0x10(%ebp)
 804919b:	e9 bf ff ff ff       	jmp    804915f <matrix_multiply+0x3f>
 80491a0:	8b 55 f4             	mov    -0xc(%ebp),%edx
 80491a3:	8b 45 10             	mov    0x10(%ebp),%eax
 80491a6:	8b 4d fc             	mov    -0x4(%ebp),%ecx
 80491a9:	c1 e1 05             	shl    $0x5,%ecx
 80491ac:	01 c8                	add    %ecx,%eax
 80491ae:	8b 4d f8             	mov    -0x8(%ebp),%ecx
 80491b1:	89 14 88             	mov    %edx,(%eax,%ecx,4)
 80491b4:	8b 45 f8             	mov    -0x8(%ebp),%eax
 80491b7:	83 c0 01             	add    $0x1,%eax
 80491ba:	89 45 f8             	mov    %eax,-0x8(%ebp)
 80491bd:	e9 85 ff ff ff       	jmp    8049147 <matrix_multiply+0x27>
 80491c2:	e9 00 00 00 00       	jmp    80491c7 <matrix_multiply+0xa7>
 80491c7:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80491ca:	83 c0 01             	add    $0x1,%eax
 80491cd:	89 45 fc             	mov    %eax,-0x4(%ebp)
 80491d0:	e9 61 ff ff ff       	jmp    8049136 <matrix_multiply+0x16>
 80491d5:	83 c4 10             	add    $0x10,%esp
 80491d8:	5d                   	pop    %ebp
 80491d9:	c3                   	ret
 80491da:	90                   	nop
 80491db:	90                   	nop
 80491dc:	90                   	nop
 80491dd:	90                   	nop
 80491de:	90                   	nop
 80491df:	90                   	nop

080491e0 <checksum>:
 80491e0:	55                   	push   %ebp
 80491e1:	89 e5                	mov    %esp,%ebp
 80491e3:	83 ec 0c             	sub    $0xc,%esp
 80491e6:	8b 45 08             	mov    0x8(%ebp),%eax
 80491e9:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
 80491f0:	c7 45 f8 00 00 00 00 	movl   $0x0,-0x8(%ebp)
 80491f7:	83 7d f8 08          	cmpl   $0x8,-0x8(%ebp)
 80491fb:	0f 8d 49 00 00 00    	jge    804924a <checksum+0x6a>
 8049201:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 8049208:	83 7d f4 08          	cmpl   $0x8,-0xc(%ebp)
 804920c:	0f 8d 25 00 00 00    	jge    8049237 <checksum+0x57>
 8049212:	8b 45 08             	mov    0x8(%ebp),%eax
 8049215:	8b 4d f8             	mov    -0x8(%ebp),%ecx
 8049218:	c1 e1 05             	shl    $0x5,%ecx
 804921b:	01 c8                	add    %ecx,%eax
 804921d:	8b 4d f4             	mov    -0xc(%ebp),%ecx
 8049220:	8b 04 88             	mov    (%eax,%ecx,4),%eax
 8049223:	03 45 fc             	add    -0x4(%ebp),%eax
 8049226:	89 45 fc             	mov    %eax,-0x4(%ebp)
 8049229:	8b 45 f4             	mov    -0xc(%ebp),%eax
 804922c:	83 c0 01             	add    $0x1,%eax
 804922f:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8049232:	e9 d1 ff ff ff       	jmp    8049208 <checksum+0x28>
 8049237:	e9 00 00 00 00       	jmp    804923c <checksum+0x5c>
 804923c:	8b 45 f8             	mov    -0x8(%ebp),%eax
 804923f:	83 c0 01             	add    $0x1,%eax
 8049242:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8049245:	e9 ad ff ff ff       	jmp    80491f7 <checksum+0x17>
 804924a:	8b 45 fc             	mov    -0x4(%ebp),%eax
 804924d:	83 c4 0c             	add    $0xc,%esp
 8049250:	5d                   	pop    %ebp
 8049251:	c3                   	ret
