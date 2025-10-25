
llvm_modules/fibonacci_O0_pgo.elf:     file format elf32-i386


Disassembly of section .text:

08049000 <fibonacci>:
 8049000:	55                   	push   %ebp
 8049001:	89 e5                	mov    %esp,%ebp
 8049003:	53                   	push   %ebx
 8049004:	83 ec 14             	sub    $0x14,%esp
 8049007:	e8 00 00 00 00       	call   804900c <fibonacci+0xc>
 804900c:	58                   	pop    %eax
 804900d:	81 c0 f4 0f 00 00    	add    $0xff4,%eax
 8049013:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8049016:	8b 45 08             	mov    0x8(%ebp),%eax
 8049019:	83 7d 08 01          	cmpl   $0x1,0x8(%ebp)
 804901d:	0f 8f 0b 00 00 00    	jg     804902e <fibonacci+0x2e>
 8049023:	8b 45 08             	mov    0x8(%ebp),%eax
 8049026:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8049029:	e9 2f 00 00 00       	jmp    804905d <fibonacci+0x5d>
 804902e:	8b 5d f4             	mov    -0xc(%ebp),%ebx
 8049031:	8b 45 08             	mov    0x8(%ebp),%eax
 8049034:	83 e8 01             	sub    $0x1,%eax
 8049037:	89 04 24             	mov    %eax,(%esp)
 804903a:	e8 c1 ff ff ff       	call   8049000 <fibonacci>
 804903f:	8b 5d f4             	mov    -0xc(%ebp),%ebx
 8049042:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8049045:	8b 45 08             	mov    0x8(%ebp),%eax
 8049048:	83 e8 02             	sub    $0x2,%eax
 804904b:	89 04 24             	mov    %eax,(%esp)
 804904e:	e8 ad ff ff ff       	call   8049000 <fibonacci>
 8049053:	89 c1                	mov    %eax,%ecx
 8049055:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8049058:	01 c8                	add    %ecx,%eax
 804905a:	89 45 f8             	mov    %eax,-0x8(%ebp)
 804905d:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8049060:	83 c4 14             	add    $0x14,%esp
 8049063:	5b                   	pop    %ebx
 8049064:	5d                   	pop    %ebp
 8049065:	c3                   	ret
 8049066:	90                   	nop
 8049067:	90                   	nop
 8049068:	90                   	nop
 8049069:	90                   	nop
 804906a:	90                   	nop
 804906b:	90                   	nop
 804906c:	90                   	nop
 804906d:	90                   	nop
 804906e:	90                   	nop
 804906f:	90                   	nop

08049070 <fibonacci_iter>:
 8049070:	55                   	push   %ebp
 8049071:	89 e5                	mov    %esp,%ebp
 8049073:	83 ec 14             	sub    $0x14,%esp
 8049076:	8b 45 08             	mov    0x8(%ebp),%eax
 8049079:	83 7d 08 01          	cmpl   $0x1,0x8(%ebp)
 804907d:	0f 8f 0b 00 00 00    	jg     804908e <fibonacci_iter+0x1e>
 8049083:	8b 45 08             	mov    0x8(%ebp),%eax
 8049086:	89 45 fc             	mov    %eax,-0x4(%ebp)
 8049089:	e9 4a 00 00 00       	jmp    80490d8 <fibonacci_iter+0x68>
 804908e:	c7 45 f8 00 00 00 00 	movl   $0x0,-0x8(%ebp)
 8049095:	c7 45 f4 01 00 00 00 	movl   $0x1,-0xc(%ebp)
 804909c:	c7 45 f0 02 00 00 00 	movl   $0x2,-0x10(%ebp)
 80490a3:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80490a6:	3b 45 08             	cmp    0x8(%ebp),%eax
 80490a9:	0f 8f 23 00 00 00    	jg     80490d2 <fibonacci_iter+0x62>
 80490af:	8b 45 f8             	mov    -0x8(%ebp),%eax
 80490b2:	03 45 f4             	add    -0xc(%ebp),%eax
 80490b5:	89 45 ec             	mov    %eax,-0x14(%ebp)
 80490b8:	8b 45 f4             	mov    -0xc(%ebp),%eax
 80490bb:	89 45 f8             	mov    %eax,-0x8(%ebp)
 80490be:	8b 45 ec             	mov    -0x14(%ebp),%eax
 80490c1:	89 45 f4             	mov    %eax,-0xc(%ebp)
 80490c4:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80490c7:	83 c0 01             	add    $0x1,%eax
 80490ca:	89 45 f0             	mov    %eax,-0x10(%ebp)
 80490cd:	e9 d1 ff ff ff       	jmp    80490a3 <fibonacci_iter+0x33>
 80490d2:	8b 45 f4             	mov    -0xc(%ebp),%eax
 80490d5:	89 45 fc             	mov    %eax,-0x4(%ebp)
 80490d8:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80490db:	83 c4 14             	add    $0x14,%esp
 80490de:	5d                   	pop    %ebp
 80490df:	c3                   	ret

080490e0 <compute>:
 80490e0:	55                   	push   %ebp
 80490e1:	89 e5                	mov    %esp,%ebp
 80490e3:	53                   	push   %ebx
 80490e4:	50                   	push   %eax
 80490e5:	e8 00 00 00 00       	call   80490ea <compute+0xa>
 80490ea:	5b                   	pop    %ebx
 80490eb:	81 c3 16 0f 00 00    	add    $0xf16,%ebx
 80490f1:	c7 04 24 0a 00 00 00 	movl   $0xa,(%esp)
 80490f8:	e8 03 ff ff ff       	call   8049000 <fibonacci>
 80490fd:	83 c4 04             	add    $0x4,%esp
 8049100:	5b                   	pop    %ebx
 8049101:	5d                   	pop    %ebp
 8049102:	c3                   	ret
