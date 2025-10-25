
llvm_modules/fibonacci_O1_pgo.elf:     file format elf32-i386


Disassembly of section .text:

08049000 <fibonacci>:
 8049000:	55                   	push   %ebp
 8049001:	89 e5                	mov    %esp,%ebp
 8049003:	53                   	push   %ebx
 8049004:	56                   	push   %esi
 8049005:	83 ec 10             	sub    $0x10,%esp
 8049008:	e8 00 00 00 00       	call   804900d <fibonacci+0xd>
 804900d:	5b                   	pop    %ebx
 804900e:	81 c3 f3 0f 00 00    	add    $0xff3,%ebx
 8049014:	8b 45 08             	mov    0x8(%ebp),%eax
 8049017:	83 7d 08 01          	cmpl   $0x1,0x8(%ebp)
 804901b:	7f 08                	jg     8049025 <fibonacci+0x25>
 804901d:	8b 45 08             	mov    0x8(%ebp),%eax
 8049020:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8049023:	eb 23                	jmp    8049048 <fibonacci+0x48>
 8049025:	8b 45 08             	mov    0x8(%ebp),%eax
 8049028:	83 e8 01             	sub    $0x1,%eax
 804902b:	89 04 24             	mov    %eax,(%esp)
 804902e:	e8 cd ff ff ff       	call   8049000 <fibonacci>
 8049033:	89 c6                	mov    %eax,%esi
 8049035:	8b 45 08             	mov    0x8(%ebp),%eax
 8049038:	83 e8 02             	sub    $0x2,%eax
 804903b:	89 04 24             	mov    %eax,(%esp)
 804903e:	e8 bd ff ff ff       	call   8049000 <fibonacci>
 8049043:	01 c6                	add    %eax,%esi
 8049045:	89 75 f4             	mov    %esi,-0xc(%ebp)
 8049048:	8b 45 f4             	mov    -0xc(%ebp),%eax
 804904b:	83 c4 10             	add    $0x10,%esp
 804904e:	5e                   	pop    %esi
 804904f:	5b                   	pop    %ebx
 8049050:	5d                   	pop    %ebp
 8049051:	c3                   	ret
 8049052:	90                   	nop
 8049053:	90                   	nop
 8049054:	90                   	nop
 8049055:	90                   	nop
 8049056:	90                   	nop
 8049057:	90                   	nop
 8049058:	90                   	nop
 8049059:	90                   	nop
 804905a:	90                   	nop
 804905b:	90                   	nop
 804905c:	90                   	nop
 804905d:	90                   	nop
 804905e:	90                   	nop
 804905f:	90                   	nop

08049060 <fibonacci_iter>:
 8049060:	55                   	push   %ebp
 8049061:	89 e5                	mov    %esp,%ebp
 8049063:	83 ec 14             	sub    $0x14,%esp
 8049066:	8b 45 08             	mov    0x8(%ebp),%eax
 8049069:	83 7d 08 01          	cmpl   $0x1,0x8(%ebp)
 804906d:	7f 08                	jg     8049077 <fibonacci_iter+0x17>
 804906f:	8b 45 08             	mov    0x8(%ebp),%eax
 8049072:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8049075:	eb 43                	jmp    80490ba <fibonacci_iter+0x5a>
 8049077:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 804907e:	c7 45 fc 01 00 00 00 	movl   $0x1,-0x4(%ebp)
 8049085:	c7 45 f8 02 00 00 00 	movl   $0x2,-0x8(%ebp)
 804908c:	8b 45 f8             	mov    -0x8(%ebp),%eax
 804908f:	3b 45 08             	cmp    0x8(%ebp),%eax
 8049092:	7f 20                	jg     80490b4 <fibonacci_iter+0x54>
 8049094:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8049097:	03 45 fc             	add    -0x4(%ebp),%eax
 804909a:	89 45 ec             	mov    %eax,-0x14(%ebp)
 804909d:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80490a0:	89 45 f4             	mov    %eax,-0xc(%ebp)
 80490a3:	8b 45 ec             	mov    -0x14(%ebp),%eax
 80490a6:	89 45 fc             	mov    %eax,-0x4(%ebp)
 80490a9:	8b 45 f8             	mov    -0x8(%ebp),%eax
 80490ac:	83 c0 01             	add    $0x1,%eax
 80490af:	89 45 f8             	mov    %eax,-0x8(%ebp)
 80490b2:	eb d8                	jmp    804908c <fibonacci_iter+0x2c>
 80490b4:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80490b7:	89 45 f0             	mov    %eax,-0x10(%ebp)
 80490ba:	8b 45 f0             	mov    -0x10(%ebp),%eax
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

080490d0 <compute>:
 80490d0:	55                   	push   %ebp
 80490d1:	89 e5                	mov    %esp,%ebp
 80490d3:	53                   	push   %ebx
 80490d4:	50                   	push   %eax
 80490d5:	e8 00 00 00 00       	call   80490da <compute+0xa>
 80490da:	5b                   	pop    %ebx
 80490db:	81 c3 26 0f 00 00    	add    $0xf26,%ebx
 80490e1:	c7 04 24 0a 00 00 00 	movl   $0xa,(%esp)
 80490e8:	e8 13 ff ff ff       	call   8049000 <fibonacci>
 80490ed:	83 c4 04             	add    $0x4,%esp
 80490f0:	5b                   	pop    %ebx
 80490f1:	5d                   	pop    %ebp
 80490f2:	c3                   	ret
