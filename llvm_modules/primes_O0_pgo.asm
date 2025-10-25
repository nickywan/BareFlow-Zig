
llvm_modules/primes_O0_pgo.elf:     file format elf32-i386


Disassembly of section .text:

08049000 <compute>:
 8049000:	55                   	push   %ebp
 8049001:	89 e5                	mov    %esp,%ebp
 8049003:	53                   	push   %ebx
 8049004:	83 ec 14             	sub    $0x14,%esp
 8049007:	e8 00 00 00 00       	call   804900c <compute+0xc>
 804900c:	5b                   	pop    %ebx
 804900d:	81 c3 f4 0f 00 00    	add    $0xff4,%ebx
 8049013:	89 5d ec             	mov    %ebx,-0x14(%ebp)
 8049016:	e8 65 00 00 00       	call   8049080 <sieve_of_eratosthenes>
 804901b:	89 45 f8             	mov    %eax,-0x8(%ebp)
 804901e:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 8049025:	c7 45 f0 02 00 00 00 	movl   $0x2,-0x10(%ebp)
 804902c:	83 7d f0 32          	cmpl   $0x32,-0x10(%ebp)
 8049030:	0f 8d 33 00 00 00    	jge    8049069 <compute+0x69>
 8049036:	8b 5d ec             	mov    -0x14(%ebp),%ebx
 8049039:	8b 45 f0             	mov    -0x10(%ebp),%eax
 804903c:	89 04 24             	mov    %eax,(%esp)
 804903f:	e8 6c 01 00 00       	call   80491b0 <is_prime_trial>
 8049044:	83 f8 00             	cmp    $0x0,%eax
 8049047:	0f 84 09 00 00 00    	je     8049056 <compute+0x56>
 804904d:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8049050:	83 c0 01             	add    $0x1,%eax
 8049053:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8049056:	e9 00 00 00 00       	jmp    804905b <compute+0x5b>
 804905b:	8b 45 f0             	mov    -0x10(%ebp),%eax
 804905e:	83 c0 01             	add    $0x1,%eax
 8049061:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8049064:	e9 c3 ff ff ff       	jmp    804902c <compute+0x2c>
 8049069:	69 45 f8 e8 03 00 00 	imul   $0x3e8,-0x8(%ebp),%eax
 8049070:	03 45 f4             	add    -0xc(%ebp),%eax
 8049073:	83 c4 14             	add    $0x14,%esp
 8049076:	5b                   	pop    %ebx
 8049077:	5d                   	pop    %ebp
 8049078:	c3                   	ret
 8049079:	90                   	nop
 804907a:	90                   	nop
 804907b:	90                   	nop
 804907c:	90                   	nop
 804907d:	90                   	nop
 804907e:	90                   	nop
 804907f:	90                   	nop

08049080 <sieve_of_eratosthenes>:
 8049080:	55                   	push   %ebp
 8049081:	89 e5                	mov    %esp,%ebp
 8049083:	83 ec 18             	sub    $0x18,%esp
 8049086:	e8 00 00 00 00       	call   804908b <sieve_of_eratosthenes+0xb>
 804908b:	58                   	pop    %eax
 804908c:	81 c0 75 0f 00 00    	add    $0xf75,%eax
 8049092:	89 45 e8             	mov    %eax,-0x18(%ebp)
 8049095:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
 804909c:	81 7d fc c8 00 00 00 	cmpl   $0xc8,-0x4(%ebp)
 80490a3:	0f 8d 1c 00 00 00    	jge    80490c5 <sieve_of_eratosthenes+0x45>
 80490a9:	8b 45 e8             	mov    -0x18(%ebp),%eax
 80490ac:	8b 4d fc             	mov    -0x4(%ebp),%ecx
 80490af:	c6 84 08 0c 00 00 00 	movb   $0x1,0xc(%eax,%ecx,1)
 80490b6:	01 
 80490b7:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80490ba:	83 c0 01             	add    $0x1,%eax
 80490bd:	89 45 fc             	mov    %eax,-0x4(%ebp)
 80490c0:	e9 d7 ff ff ff       	jmp    804909c <sieve_of_eratosthenes+0x1c>
 80490c5:	8b 45 e8             	mov    -0x18(%ebp),%eax
 80490c8:	c6 80 0c 00 00 00 00 	movb   $0x0,0xc(%eax)
 80490cf:	c6 80 0d 00 00 00 00 	movb   $0x0,0xd(%eax)
 80490d6:	c7 45 f8 02 00 00 00 	movl   $0x2,-0x8(%ebp)
 80490dd:	8b 45 f8             	mov    -0x8(%ebp),%eax
 80490e0:	0f af 45 f8          	imul   -0x8(%ebp),%eax
 80490e4:	3d c8 00 00 00       	cmp    $0xc8,%eax
 80490e9:	0f 8d 5f 00 00 00    	jge    804914e <sieve_of_eratosthenes+0xce>
 80490ef:	8b 45 e8             	mov    -0x18(%ebp),%eax
 80490f2:	8b 4d f8             	mov    -0x8(%ebp),%ecx
 80490f5:	80 bc 08 0c 00 00 00 	cmpb   $0x0,0xc(%eax,%ecx,1)
 80490fc:	00 
 80490fd:	0f 84 38 00 00 00    	je     804913b <sieve_of_eratosthenes+0xbb>
 8049103:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8049106:	0f af 45 f8          	imul   -0x8(%ebp),%eax
 804910a:	89 45 f4             	mov    %eax,-0xc(%ebp)
 804910d:	81 7d f4 c8 00 00 00 	cmpl   $0xc8,-0xc(%ebp)
 8049114:	0f 8d 1c 00 00 00    	jge    8049136 <sieve_of_eratosthenes+0xb6>
 804911a:	8b 45 e8             	mov    -0x18(%ebp),%eax
 804911d:	8b 4d f4             	mov    -0xc(%ebp),%ecx
 8049120:	c6 84 08 0c 00 00 00 	movb   $0x0,0xc(%eax,%ecx,1)
 8049127:	00 
 8049128:	8b 45 f8             	mov    -0x8(%ebp),%eax
 804912b:	03 45 f4             	add    -0xc(%ebp),%eax
 804912e:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8049131:	e9 d7 ff ff ff       	jmp    804910d <sieve_of_eratosthenes+0x8d>
 8049136:	e9 00 00 00 00       	jmp    804913b <sieve_of_eratosthenes+0xbb>
 804913b:	e9 00 00 00 00       	jmp    8049140 <sieve_of_eratosthenes+0xc0>
 8049140:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8049143:	83 c0 01             	add    $0x1,%eax
 8049146:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8049149:	e9 8f ff ff ff       	jmp    80490dd <sieve_of_eratosthenes+0x5d>
 804914e:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
 8049155:	c7 45 ec 02 00 00 00 	movl   $0x2,-0x14(%ebp)
 804915c:	81 7d ec c8 00 00 00 	cmpl   $0xc8,-0x14(%ebp)
 8049163:	0f 8d 30 00 00 00    	jge    8049199 <sieve_of_eratosthenes+0x119>
 8049169:	8b 45 e8             	mov    -0x18(%ebp),%eax
 804916c:	8b 4d ec             	mov    -0x14(%ebp),%ecx
 804916f:	80 bc 08 0c 00 00 00 	cmpb   $0x0,0xc(%eax,%ecx,1)
 8049176:	00 
 8049177:	0f 84 09 00 00 00    	je     8049186 <sieve_of_eratosthenes+0x106>
 804917d:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8049180:	83 c0 01             	add    $0x1,%eax
 8049183:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8049186:	e9 00 00 00 00       	jmp    804918b <sieve_of_eratosthenes+0x10b>
 804918b:	8b 45 ec             	mov    -0x14(%ebp),%eax
 804918e:	83 c0 01             	add    $0x1,%eax
 8049191:	89 45 ec             	mov    %eax,-0x14(%ebp)
 8049194:	e9 c3 ff ff ff       	jmp    804915c <sieve_of_eratosthenes+0xdc>
 8049199:	8b 45 f0             	mov    -0x10(%ebp),%eax
 804919c:	83 c4 18             	add    $0x18,%esp
 804919f:	5d                   	pop    %ebp
 80491a0:	c3                   	ret
 80491a1:	90                   	nop
 80491a2:	90                   	nop
 80491a3:	90                   	nop
 80491a4:	90                   	nop
 80491a5:	90                   	nop
 80491a6:	90                   	nop
 80491a7:	90                   	nop
 80491a8:	90                   	nop
 80491a9:	90                   	nop
 80491aa:	90                   	nop
 80491ab:	90                   	nop
 80491ac:	90                   	nop
 80491ad:	90                   	nop
 80491ae:	90                   	nop
 80491af:	90                   	nop

080491b0 <is_prime_trial>:
 80491b0:	55                   	push   %ebp
 80491b1:	89 e5                	mov    %esp,%ebp
 80491b3:	83 ec 08             	sub    $0x8,%esp
 80491b6:	8b 45 08             	mov    0x8(%ebp),%eax
 80491b9:	83 7d 08 02          	cmpl   $0x2,0x8(%ebp)
 80491bd:	0f 8d 0c 00 00 00    	jge    80491cf <is_prime_trial+0x1f>
 80491c3:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
 80491ca:	e9 83 00 00 00       	jmp    8049252 <is_prime_trial+0xa2>
 80491cf:	83 7d 08 02          	cmpl   $0x2,0x8(%ebp)
 80491d3:	0f 85 0c 00 00 00    	jne    80491e5 <is_prime_trial+0x35>
 80491d9:	c7 45 fc 01 00 00 00 	movl   $0x1,-0x4(%ebp)
 80491e0:	e9 6d 00 00 00       	jmp    8049252 <is_prime_trial+0xa2>
 80491e5:	8b 45 08             	mov    0x8(%ebp),%eax
 80491e8:	b9 02 00 00 00       	mov    $0x2,%ecx
 80491ed:	99                   	cltd
 80491ee:	f7 f9                	idiv   %ecx
 80491f0:	83 fa 00             	cmp    $0x0,%edx
 80491f3:	0f 85 0c 00 00 00    	jne    8049205 <is_prime_trial+0x55>
 80491f9:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
 8049200:	e9 4d 00 00 00       	jmp    8049252 <is_prime_trial+0xa2>
 8049205:	c7 45 f8 03 00 00 00 	movl   $0x3,-0x8(%ebp)
 804920c:	8b 45 f8             	mov    -0x8(%ebp),%eax
 804920f:	0f af 45 f8          	imul   -0x8(%ebp),%eax
 8049213:	3b 45 08             	cmp    0x8(%ebp),%eax
 8049216:	0f 8f 2f 00 00 00    	jg     804924b <is_prime_trial+0x9b>
 804921c:	8b 45 08             	mov    0x8(%ebp),%eax
 804921f:	99                   	cltd
 8049220:	f7 7d f8             	idivl  -0x8(%ebp)
 8049223:	83 fa 00             	cmp    $0x0,%edx
 8049226:	0f 85 0c 00 00 00    	jne    8049238 <is_prime_trial+0x88>
 804922c:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
 8049233:	e9 1a 00 00 00       	jmp    8049252 <is_prime_trial+0xa2>
 8049238:	e9 00 00 00 00       	jmp    804923d <is_prime_trial+0x8d>
 804923d:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8049240:	83 c0 02             	add    $0x2,%eax
 8049243:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8049246:	e9 c1 ff ff ff       	jmp    804920c <is_prime_trial+0x5c>
 804924b:	c7 45 fc 01 00 00 00 	movl   $0x1,-0x4(%ebp)
 8049252:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8049255:	83 c4 08             	add    $0x8,%esp
 8049258:	5d                   	pop    %ebp
 8049259:	c3                   	ret
