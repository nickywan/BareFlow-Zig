
llvm_modules/matrix_mul_O3_pgo.elf:     file format elf32-i386


Disassembly of section .text:

08049000 <compute>:
 8049000:	55                   	push   %ebp
 8049001:	89 e5                	mov    %esp,%ebp
 8049003:	53                   	push   %ebx
 8049004:	50                   	push   %eax
 8049005:	e8 00 00 00 00       	call   804900a <compute+0xa>
 804900a:	5b                   	pop    %ebx
 804900b:	81 c3 f6 0f 00 00    	add    $0xff6,%ebx
 8049011:	8d 8b 0c 00 00 00    	lea    0xc(%ebx),%ecx
 8049017:	ba 2a 00 00 00       	mov    $0x2a,%edx
 804901c:	e8 2f 00 00 00       	call   8049050 <init_matrix>
 8049021:	8d 8b 0c 01 00 00    	lea    0x10c(%ebx),%ecx
 8049027:	ba 11 00 00 00       	mov    $0x11,%edx
 804902c:	e8 1f 00 00 00       	call   8049050 <init_matrix>
 8049031:	e8 9a 00 00 00       	call   80490d0 <matrix_multiply>
 8049036:	e8 65 01 00 00       	call   80491a0 <checksum>
 804903b:	83 c4 04             	add    $0x4,%esp
 804903e:	5b                   	pop    %ebx
 804903f:	5d                   	pop    %ebp
 8049040:	c3                   	ret
 8049041:	90                   	nop
 8049042:	90                   	nop
 8049043:	90                   	nop
 8049044:	90                   	nop
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

08049050 <init_matrix>:
 8049050:	55                   	push   %ebp
 8049051:	89 e5                	mov    %esp,%ebp
 8049053:	83 ec 14             	sub    $0x14,%esp
 8049056:	89 4d ec             	mov    %ecx,-0x14(%ebp)
 8049059:	89 55 f0             	mov    %edx,-0x10(%ebp)
 804905c:	8b 45 f0             	mov    -0x10(%ebp),%eax
 804905f:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8049062:	c7 45 f8 00 00 00 00 	movl   $0x0,-0x8(%ebp)
 8049069:	83 7d f8 08          	cmpl   $0x8,-0x8(%ebp)
 804906d:	7d 4e                	jge    80490bd <init_matrix+0x6d>
 804906f:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
 8049076:	83 7d fc 08          	cmpl   $0x8,-0x4(%ebp)
 804907a:	7d 34                	jge    80490b0 <init_matrix+0x60>
 804907c:	6b 45 f4 0d          	imul   $0xd,-0xc(%ebp),%eax
 8049080:	83 c0 07             	add    $0x7,%eax
 8049083:	b9 64 00 00 00       	mov    $0x64,%ecx
 8049088:	99                   	cltd
 8049089:	f7 f9                	idiv   %ecx
 804908b:	8b 45 ec             	mov    -0x14(%ebp),%eax
 804908e:	8b 4d f8             	mov    -0x8(%ebp),%ecx
 8049091:	c1 e1 05             	shl    $0x5,%ecx
 8049094:	01 c8                	add    %ecx,%eax
 8049096:	8b 4d fc             	mov    -0x4(%ebp),%ecx
 8049099:	89 14 88             	mov    %edx,(%eax,%ecx,4)
 804909c:	8b 45 f4             	mov    -0xc(%ebp),%eax
 804909f:	83 c0 01             	add    $0x1,%eax
 80490a2:	89 45 f4             	mov    %eax,-0xc(%ebp)
 80490a5:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80490a8:	83 c0 01             	add    $0x1,%eax
 80490ab:	89 45 fc             	mov    %eax,-0x4(%ebp)
 80490ae:	eb c6                	jmp    8049076 <init_matrix+0x26>
 80490b0:	eb 00                	jmp    80490b2 <init_matrix+0x62>
 80490b2:	8b 45 f8             	mov    -0x8(%ebp),%eax
 80490b5:	83 c0 01             	add    $0x1,%eax
 80490b8:	89 45 f8             	mov    %eax,-0x8(%ebp)
 80490bb:	eb ac                	jmp    8049069 <init_matrix+0x19>
 80490bd:	83 c4 14             	add    $0x14,%esp
 80490c0:	5d                   	pop    %ebp
 80490c1:	c3                   	ret
 80490c2:	90                   	nop
 80490c3:	90                   	nop
 80490c4:	90                   	nop
 80490c5:	90                   	nop
 80490c6:	90                   	nop
 80490c7:	90                   	nop
 80490c8:	90                   	nop
 80490c9:	90                   	nop
 80490ca:	90                   	nop
 80490cb:	90                   	nop
 80490cc:	90                   	nop
 80490cd:	90                   	nop
 80490ce:	90                   	nop
 80490cf:	90                   	nop

080490d0 <matrix_multiply>:
 80490d0:	55                   	push   %ebp
 80490d1:	89 e5                	mov    %esp,%ebp
 80490d3:	83 ec 1c             	sub    $0x1c,%esp
 80490d6:	e8 00 00 00 00       	call   80490db <matrix_multiply+0xb>
 80490db:	58                   	pop    %eax
 80490dc:	81 c0 25 0f 00 00    	add    $0xf25,%eax
 80490e2:	8d 88 0c 00 00 00    	lea    0xc(%eax),%ecx
 80490e8:	89 4d e4             	mov    %ecx,-0x1c(%ebp)
 80490eb:	8d 88 0c 01 00 00    	lea    0x10c(%eax),%ecx
 80490f1:	89 4d e8             	mov    %ecx,-0x18(%ebp)
 80490f4:	8d 80 0c 02 00 00    	lea    0x20c(%eax),%eax
 80490fa:	89 45 ec             	mov    %eax,-0x14(%ebp)
 80490fd:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 8049104:	83 7d f4 08          	cmpl   $0x8,-0xc(%ebp)
 8049108:	0f 8d 84 00 00 00    	jge    8049192 <matrix_multiply+0xc2>
 804910e:	c7 45 f8 00 00 00 00 	movl   $0x0,-0x8(%ebp)
 8049115:	83 7d f8 08          	cmpl   $0x8,-0x8(%ebp)
 8049119:	7d 67                	jge    8049182 <matrix_multiply+0xb2>
 804911b:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
 8049122:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
 8049129:	83 7d fc 08          	cmpl   $0x8,-0x4(%ebp)
 804912d:	7d 34                	jge    8049163 <matrix_multiply+0x93>
 804912f:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 8049132:	8b 4d f4             	mov    -0xc(%ebp),%ecx
 8049135:	c1 e1 05             	shl    $0x5,%ecx
 8049138:	01 c8                	add    %ecx,%eax
 804913a:	8b 4d fc             	mov    -0x4(%ebp),%ecx
 804913d:	8b 04 88             	mov    (%eax,%ecx,4),%eax
 8049140:	8b 4d e8             	mov    -0x18(%ebp),%ecx
 8049143:	8b 55 fc             	mov    -0x4(%ebp),%edx
 8049146:	c1 e2 05             	shl    $0x5,%edx
 8049149:	01 d1                	add    %edx,%ecx
 804914b:	8b 55 f8             	mov    -0x8(%ebp),%edx
 804914e:	0f af 04 91          	imul   (%ecx,%edx,4),%eax
 8049152:	03 45 f0             	add    -0x10(%ebp),%eax
 8049155:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8049158:	8b 45 fc             	mov    -0x4(%ebp),%eax
 804915b:	83 c0 01             	add    $0x1,%eax
 804915e:	89 45 fc             	mov    %eax,-0x4(%ebp)
 8049161:	eb c6                	jmp    8049129 <matrix_multiply+0x59>
 8049163:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8049166:	8b 4d ec             	mov    -0x14(%ebp),%ecx
 8049169:	8b 55 f4             	mov    -0xc(%ebp),%edx
 804916c:	c1 e2 05             	shl    $0x5,%edx
 804916f:	01 d1                	add    %edx,%ecx
 8049171:	8b 55 f8             	mov    -0x8(%ebp),%edx
 8049174:	89 04 91             	mov    %eax,(%ecx,%edx,4)
 8049177:	8b 45 f8             	mov    -0x8(%ebp),%eax
 804917a:	83 c0 01             	add    $0x1,%eax
 804917d:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8049180:	eb 93                	jmp    8049115 <matrix_multiply+0x45>
 8049182:	eb 00                	jmp    8049184 <matrix_multiply+0xb4>
 8049184:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8049187:	83 c0 01             	add    $0x1,%eax
 804918a:	89 45 f4             	mov    %eax,-0xc(%ebp)
 804918d:	e9 72 ff ff ff       	jmp    8049104 <matrix_multiply+0x34>
 8049192:	83 c4 1c             	add    $0x1c,%esp
 8049195:	5d                   	pop    %ebp
 8049196:	c3                   	ret
 8049197:	90                   	nop
 8049198:	90                   	nop
 8049199:	90                   	nop
 804919a:	90                   	nop
 804919b:	90                   	nop
 804919c:	90                   	nop
 804919d:	90                   	nop
 804919e:	90                   	nop
 804919f:	90                   	nop

080491a0 <checksum>:
 80491a0:	55                   	push   %ebp
 80491a1:	89 e5                	mov    %esp,%ebp
 80491a3:	83 ec 10             	sub    $0x10,%esp
 80491a6:	e8 00 00 00 00       	call   80491ab <checksum+0xb>
 80491ab:	58                   	pop    %eax
 80491ac:	81 c0 55 0e 00 00    	add    $0xe55,%eax
 80491b2:	8d 80 0c 02 00 00    	lea    0x20c(%eax),%eax
 80491b8:	89 45 f0             	mov    %eax,-0x10(%ebp)
 80491bb:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 80491c2:	c7 45 f8 00 00 00 00 	movl   $0x0,-0x8(%ebp)
 80491c9:	83 7d f8 08          	cmpl   $0x8,-0x8(%ebp)
 80491cd:	7d 3c                	jge    804920b <checksum+0x6b>
 80491cf:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
 80491d6:	83 7d fc 08          	cmpl   $0x8,-0x4(%ebp)
 80491da:	7d 22                	jge    80491fe <checksum+0x5e>
 80491dc:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80491df:	8b 4d f8             	mov    -0x8(%ebp),%ecx
 80491e2:	c1 e1 05             	shl    $0x5,%ecx
 80491e5:	01 c8                	add    %ecx,%eax
 80491e7:	8b 4d fc             	mov    -0x4(%ebp),%ecx
 80491ea:	8b 04 88             	mov    (%eax,%ecx,4),%eax
 80491ed:	03 45 f4             	add    -0xc(%ebp),%eax
 80491f0:	89 45 f4             	mov    %eax,-0xc(%ebp)
 80491f3:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80491f6:	83 c0 01             	add    $0x1,%eax
 80491f9:	89 45 fc             	mov    %eax,-0x4(%ebp)
 80491fc:	eb d8                	jmp    80491d6 <checksum+0x36>
 80491fe:	eb 00                	jmp    8049200 <checksum+0x60>
 8049200:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8049203:	83 c0 01             	add    $0x1,%eax
 8049206:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8049209:	eb be                	jmp    80491c9 <checksum+0x29>
 804920b:	8b 45 f4             	mov    -0xc(%ebp),%eax
 804920e:	83 c4 10             	add    $0x10,%esp
 8049211:	5d                   	pop    %ebp
 8049212:	c3                   	ret
