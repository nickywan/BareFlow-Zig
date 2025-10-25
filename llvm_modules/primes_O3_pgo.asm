
llvm_modules/primes_O3_pgo.elf:     file format elf32-i386


Disassembly of section .text:

08049000 <compute>:
 8049000:	55                   	push   %ebp
 8049001:	89 e5                	mov    %esp,%ebp
 8049003:	53                   	push   %ebx
 8049004:	83 ec 14             	sub    $0x14,%esp
 8049007:	e8 00 00 00 00       	call   804900c <compute+0xc>
 804900c:	5b                   	pop    %ebx
 804900d:	81 c3 f4 0f 00 00    	add    $0xff4,%ebx
 8049013:	e8 58 00 00 00       	call   8049070 <sieve_of_eratosthenes>
 8049018:	89 45 f0             	mov    %eax,-0x10(%ebp)
 804901b:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 8049022:	c7 45 f8 02 00 00 00 	movl   $0x2,-0x8(%ebp)
 8049029:	83 7d f8 32          	cmpl   $0x32,-0x8(%ebp)
 804902d:	7d 23                	jge    8049052 <compute+0x52>
 804902f:	8b 4d f8             	mov    -0x8(%ebp),%ecx
 8049032:	e8 29 01 00 00       	call   8049160 <is_prime_trial>
 8049037:	83 f8 00             	cmp    $0x0,%eax
 804903a:	74 09                	je     8049045 <compute+0x45>
 804903c:	8b 45 f4             	mov    -0xc(%ebp),%eax
 804903f:	83 c0 01             	add    $0x1,%eax
 8049042:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8049045:	eb 00                	jmp    8049047 <compute+0x47>
 8049047:	8b 45 f8             	mov    -0x8(%ebp),%eax
 804904a:	83 c0 01             	add    $0x1,%eax
 804904d:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8049050:	eb d7                	jmp    8049029 <compute+0x29>
 8049052:	69 45 f0 e8 03 00 00 	imul   $0x3e8,-0x10(%ebp),%eax
 8049059:	03 45 f4             	add    -0xc(%ebp),%eax
 804905c:	83 c4 14             	add    $0x14,%esp
 804905f:	5b                   	pop    %ebx
 8049060:	5d                   	pop    %ebp
 8049061:	c3                   	ret
 8049062:	90                   	nop
 8049063:	90                   	nop
 8049064:	90                   	nop
 8049065:	90                   	nop
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

08049070 <sieve_of_eratosthenes>:
 8049070:	55                   	push   %ebp
 8049071:	89 e5                	mov    %esp,%ebp
 8049073:	83 ec 14             	sub    $0x14,%esp
 8049076:	e8 00 00 00 00       	call   804907b <sieve_of_eratosthenes+0xb>
 804907b:	58                   	pop    %eax
 804907c:	81 c0 85 0f 00 00    	add    $0xf85,%eax
 8049082:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
 8049089:	81 7d f0 c8 00 00 00 	cmpl   $0xc8,-0x10(%ebp)
 8049090:	7d 16                	jge    80490a8 <sieve_of_eratosthenes+0x38>
 8049092:	8b 4d f0             	mov    -0x10(%ebp),%ecx
 8049095:	c6 84 08 0c 00 00 00 	movb   $0x1,0xc(%eax,%ecx,1)
 804909c:	01 
 804909d:	8b 4d f0             	mov    -0x10(%ebp),%ecx
 80490a0:	83 c1 01             	add    $0x1,%ecx
 80490a3:	89 4d f0             	mov    %ecx,-0x10(%ebp)
 80490a6:	eb e1                	jmp    8049089 <sieve_of_eratosthenes+0x19>
 80490a8:	c6 80 0c 00 00 00 00 	movb   $0x0,0xc(%eax)
 80490af:	c6 80 0d 00 00 00 00 	movb   $0x0,0xd(%eax)
 80490b6:	c7 45 fc 02 00 00 00 	movl   $0x2,-0x4(%ebp)
 80490bd:	8b 4d fc             	mov    -0x4(%ebp),%ecx
 80490c0:	0f af 4d fc          	imul   -0x4(%ebp),%ecx
 80490c4:	81 f9 c8 00 00 00    	cmp    $0xc8,%ecx
 80490ca:	7d 45                	jge    8049111 <sieve_of_eratosthenes+0xa1>
 80490cc:	8b 4d fc             	mov    -0x4(%ebp),%ecx
 80490cf:	80 bc 08 0c 00 00 00 	cmpb   $0x0,0xc(%eax,%ecx,1)
 80490d6:	00 
 80490d7:	74 2b                	je     8049104 <sieve_of_eratosthenes+0x94>
 80490d9:	8b 4d fc             	mov    -0x4(%ebp),%ecx
 80490dc:	0f af 4d fc          	imul   -0x4(%ebp),%ecx
 80490e0:	89 4d f4             	mov    %ecx,-0xc(%ebp)
 80490e3:	81 7d f4 c8 00 00 00 	cmpl   $0xc8,-0xc(%ebp)
 80490ea:	7d 16                	jge    8049102 <sieve_of_eratosthenes+0x92>
 80490ec:	8b 4d f4             	mov    -0xc(%ebp),%ecx
 80490ef:	c6 84 08 0c 00 00 00 	movb   $0x0,0xc(%eax,%ecx,1)
 80490f6:	00 
 80490f7:	8b 4d fc             	mov    -0x4(%ebp),%ecx
 80490fa:	03 4d f4             	add    -0xc(%ebp),%ecx
 80490fd:	89 4d f4             	mov    %ecx,-0xc(%ebp)
 8049100:	eb e1                	jmp    80490e3 <sieve_of_eratosthenes+0x73>
 8049102:	eb 00                	jmp    8049104 <sieve_of_eratosthenes+0x94>
 8049104:	eb 00                	jmp    8049106 <sieve_of_eratosthenes+0x96>
 8049106:	8b 4d fc             	mov    -0x4(%ebp),%ecx
 8049109:	83 c1 01             	add    $0x1,%ecx
 804910c:	89 4d fc             	mov    %ecx,-0x4(%ebp)
 804910f:	eb ac                	jmp    80490bd <sieve_of_eratosthenes+0x4d>
 8049111:	c7 45 ec 00 00 00 00 	movl   $0x0,-0x14(%ebp)
 8049118:	c7 45 f8 02 00 00 00 	movl   $0x2,-0x8(%ebp)
 804911f:	81 7d f8 c8 00 00 00 	cmpl   $0xc8,-0x8(%ebp)
 8049126:	7d 23                	jge    804914b <sieve_of_eratosthenes+0xdb>
 8049128:	8b 4d f8             	mov    -0x8(%ebp),%ecx
 804912b:	80 bc 08 0c 00 00 00 	cmpb   $0x0,0xc(%eax,%ecx,1)
 8049132:	00 
 8049133:	74 09                	je     804913e <sieve_of_eratosthenes+0xce>
 8049135:	8b 4d ec             	mov    -0x14(%ebp),%ecx
 8049138:	83 c1 01             	add    $0x1,%ecx
 804913b:	89 4d ec             	mov    %ecx,-0x14(%ebp)
 804913e:	eb 00                	jmp    8049140 <sieve_of_eratosthenes+0xd0>
 8049140:	8b 4d f8             	mov    -0x8(%ebp),%ecx
 8049143:	83 c1 01             	add    $0x1,%ecx
 8049146:	89 4d f8             	mov    %ecx,-0x8(%ebp)
 8049149:	eb d4                	jmp    804911f <sieve_of_eratosthenes+0xaf>
 804914b:	8b 45 ec             	mov    -0x14(%ebp),%eax
 804914e:	83 c4 14             	add    $0x14,%esp
 8049151:	5d                   	pop    %ebp
 8049152:	c3                   	ret
 8049153:	90                   	nop
 8049154:	90                   	nop
 8049155:	90                   	nop
 8049156:	90                   	nop
 8049157:	90                   	nop
 8049158:	90                   	nop
 8049159:	90                   	nop
 804915a:	90                   	nop
 804915b:	90                   	nop
 804915c:	90                   	nop
 804915d:	90                   	nop
 804915e:	90                   	nop
 804915f:	90                   	nop

08049160 <is_prime_trial>:
 8049160:	55                   	push   %ebp
 8049161:	89 e5                	mov    %esp,%ebp
 8049163:	83 ec 0c             	sub    $0xc,%esp
 8049166:	89 4d f8             	mov    %ecx,-0x8(%ebp)
 8049169:	83 7d f8 02          	cmpl   $0x2,-0x8(%ebp)
 804916d:	7d 09                	jge    8049178 <is_prime_trial+0x18>
 804916f:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 8049176:	eb 64                	jmp    80491dc <is_prime_trial+0x7c>
 8049178:	83 7d f8 02          	cmpl   $0x2,-0x8(%ebp)
 804917c:	75 09                	jne    8049187 <is_prime_trial+0x27>
 804917e:	c7 45 f4 01 00 00 00 	movl   $0x1,-0xc(%ebp)
 8049185:	eb 55                	jmp    80491dc <is_prime_trial+0x7c>
 8049187:	8b 45 f8             	mov    -0x8(%ebp),%eax
 804918a:	b9 02 00 00 00       	mov    $0x2,%ecx
 804918f:	99                   	cltd
 8049190:	f7 f9                	idiv   %ecx
 8049192:	83 fa 00             	cmp    $0x0,%edx
 8049195:	75 09                	jne    80491a0 <is_prime_trial+0x40>
 8049197:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 804919e:	eb 3c                	jmp    80491dc <is_prime_trial+0x7c>
 80491a0:	c7 45 fc 03 00 00 00 	movl   $0x3,-0x4(%ebp)
 80491a7:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80491aa:	0f af 45 fc          	imul   -0x4(%ebp),%eax
 80491ae:	3b 45 f8             	cmp    -0x8(%ebp),%eax
 80491b1:	7f 22                	jg     80491d5 <is_prime_trial+0x75>
 80491b3:	8b 45 f8             	mov    -0x8(%ebp),%eax
 80491b6:	99                   	cltd
 80491b7:	f7 7d fc             	idivl  -0x4(%ebp)
 80491ba:	83 fa 00             	cmp    $0x0,%edx
 80491bd:	75 09                	jne    80491c8 <is_prime_trial+0x68>
 80491bf:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 80491c6:	eb 14                	jmp    80491dc <is_prime_trial+0x7c>
 80491c8:	eb 00                	jmp    80491ca <is_prime_trial+0x6a>
 80491ca:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80491cd:	83 c0 02             	add    $0x2,%eax
 80491d0:	89 45 fc             	mov    %eax,-0x4(%ebp)
 80491d3:	eb d2                	jmp    80491a7 <is_prime_trial+0x47>
 80491d5:	c7 45 f4 01 00 00 00 	movl   $0x1,-0xc(%ebp)
 80491dc:	8b 45 f4             	mov    -0xc(%ebp),%eax
 80491df:	83 c4 0c             	add    $0xc,%esp
 80491e2:	5d                   	pop    %ebp
 80491e3:	c3                   	ret
