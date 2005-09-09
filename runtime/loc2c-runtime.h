/* target operations */

#include <linux/types.h>
#define intptr_t long
#define uintptr_t unsigned long


/* These three macro definitions are generic, just shorthands
   used by the generated code.  */

#define op_abs(x)	(x < 0 ? -x : x)

#define fetch_bitfield(target, base, higherbits, nbits)			      \
  target = (((base) >> (sizeof (base) * 8 - (higherbits) - (nbits)))	      \
	    & (((__typeof (base)) 1 << (nbits)) - 1))

#define store_bitfield(target, base, higherbits, nbits)			      \
  target = (target							      \
	    &~ ((((__typeof (base)) 1 << (nbits)) - 1)			      \
		<< (sizeof (base) * 8 - (higherbits) - (nbits)))	      \
	    | ((__typeof (base)) (base)					      \
	       << (sizeof (base) * 8 - (higherbits) - (nbits))))


/* These operations are target-specific.  */
#include <asm/uaccess.h>

#define fetch_register(regno) \
  ((intptr_t) dwarf_register_##regno (c->regs))
#define store_register(regno, value) \
  (dwarf_register_##regno (c->regs) = (value))

#if defined __i386__

/* The stack pointer is unlike other registers.  When a trap happens in
   kernel mode, it is not saved in the trap frame (struct pt_regs).
   The `esp' (and `xss') fields are valid only for a user-mode trap.
   For a kernel mode trap, the interrupted state's esp is actually an
   address inside where the `struct pt_regs' on the kernel trap stack points.

   For now we assume all traps are from kprobes in kernel-mode code.
   For extra paranoia, could do BUG_ON((regs->xcs & 3) == 3).  */

#define dwarf_register_0(regs)	regs->eax
#define dwarf_register_1(regs)	regs->ecx
#define dwarf_register_2(regs)	regs->edx
#define dwarf_register_3(regs)	regs->ebx
#define dwarf_register_4(regs)	((long) &regs->esp)
#define dwarf_register_5(regs)	regs->ebp
#define dwarf_register_6(regs)	regs->esi
#define dwarf_register_7(regs)	regs->edi

#elif defined __x86_64__

#define dwarf_register_0(regs)	regs->rax
#define dwarf_register_1(regs)	regs->rdx
#define dwarf_register_2(regs)	regs->rcx
#define dwarf_register_3(regs)	regs->rbx
#define dwarf_register_4(regs)	regs->rsi
#define dwarf_register_5(regs)	regs->rdi
#define dwarf_register_6(regs)	regs->rbp
#define dwarf_register_7(regs)	regs->rsp
#define dwarf_register_8(regs)	regs->r8
#define dwarf_register_9(regs)	regs->r9
#define dwarf_register_10(regs)	regs->r10
#define dwarf_register_11(regs)	regs->r11
#define dwarf_register_12(regs)	regs->r12
#define dwarf_register_13(regs)	regs->r13
#define dwarf_register_14(regs)	regs->r14
#define dwarf_register_15(regs)	regs->r15

#elif defined __powerpc__

#undef fetch_register
#undef store_register
#define fetch_register(regno) ((intptr_t) c->regs->gpr[regno])
#define store_register(regno) (c->regs->gpr[regno] = (value))

#endif

#if defined __i386__ || defined __x86_64__

#define deref(size, addr)						      \
  ({									      \
    int _bad = 0;							      \
    u8 _b; u16 _w; u32 _l; u64 _q;					      \
    intptr_t _v;							      \
    switch (size)							      \
      {									      \
      case 1: __get_user_asm(_b,addr,_bad,"b","b","=q",1); _v = _b; break;    \
      case 2: __get_user_asm(_w,addr,_bad,"w","w","=r",1); _v = _w; break;    \
      case 4: __get_user_asm(_l,addr,_bad,"l","","=r",1); _v = _l; break;     \
      case 8: __get_user_asm(_q,addr,_bad,"q","","=r",1); _v = _q; break;     \
      default: _v = __get_user_bad();					      \
      }									      \
    if (_bad)								      \
      goto deref_fault;							      \
    _v;									      \
  })

#define store_deref(size, addr, value)					      \
  ({									      \
    int _bad = 0;							      \
    switch (size)							      \
      {									      \
      case 1: __put_user_asm(((u8)(value),addr,_bad,"b","b","iq",1); break;   \
      case 2: __put_user_asm(((u16)(value),addr,_bad,"w","w","ir",1); break;  \
      case 4: __put_user_asm(((u32)(value),addr,_bad,"l","k","ir",1); break;  \
      case 8: __put_user_asm(((u64)(value),addr,_bad,"q","","ir",1); break;   \
      default: __put_user_bad();					      \
      }									      \
    if (_bad)								      \
      goto deref_fault;							      \
  })

#elif defined __powerpc64__

#define deref(size, addr)						      \
  ({									      \
    int _bad = 0;							      \
    intptr_t _v;							      \
    switch (size)							      \
      {									      \
      case 1: __get_user_asm(_v,addr,_bad,"lbz",1); break;		      \
      case 2: __get_user_asm(_v,addr,_bad,"lhz",1); break;		      \
      case 4: __get_user_asm(_v,addr,_bad,"lwz",1); break;		      \
      case 8: __get_user_asm(_v,addr,_bad,"ld",1); break;		      \
      default: _v = __get_user_bad();					      \
      }									      \
    if (_bad)								      \
      goto deref_fault;							      \
    _v;									      \
  })

#define store_deref(size, addr, value)					      \
  ({									      \
    int _bad = 0;							      \
    switch (size)							      \
      {									      \
      case 1: __put_user_asm(((u8)(value),addr,_bad,"stb",1); break;   	      \
      case 2: __put_user_asm(((u16)(value),addr,_bad,"sth",1); break;  	      \
      case 4: __put_user_asm(((u32)(value),addr,_bad,"stw",1); break;  	      \
      case 8: __put_user_asm(((u64)(value),addr,_bad,"std",1); break; 	      \
      default: __put_user_bad();					      \
      }									      \
    if (_bad)								      \
      goto deref_fault;							      \
  })

#elif defined __powerpc__

#define deref(size, addr)						      \
  ({									      \
    int _bad = 0;							      \
    intptr_t _v;							      \
    switch (size)							      \
      {									      \
      case 1: __get_user_asm(_v,addr,_bad,"lbz"); break;		      \
      case 2: __get_user_asm(_v,addr,_bad,"lhz"); break;		      \
      case 4: __get_user_asm(_v,addr,_bad,"lwz"); break;		      \
      case 8: __get_user_asm(_v,addr,_bad,"ld"); break;			      \
      default: _v = __get_user_bad();					      \
      }									      \
    if (_bad)								      \
      goto deref_fault;							      \
    _v;									      \
  })

#define store_deref(size, addr, value)					      \
  ({									      \
    int _bad = 0;							      \
    switch (size)							      \
      {									      \
      case 1: __put_user_asm(((u8)(value),addr,_bad,"stb"); break;   	      \
      case 2: __put_user_asm(((u16)(value),addr,_bad,"sth"); break;  	      \
      case 4: __put_user_asm(((u32)(value),addr,_bad,"stw"); break;  	      \
      case 8: __put_user_asm2(((u64)(value),addr,_bad); break;		      \
      default: __put_user_bad();					      \
      }									      \
    if (_bad)								      \
      goto deref_fault;							      \
  })

#endif

#define deref_string(dst, addr, maxbytes)				      \
  ({									      \
    uintptr_t _addr;							      \
    size_t _len;							      \
    unsigned char _c;							      \
    char *_d = (dst);							      \
    for (_len = (maxbytes), _addr = (addr);				      \
	 _len > 1 && (_c = deref (1, _addr)) != '\0';			      \
	 --_len, ++_addr)						      \
      *_d++ = _c;							      \
    *_d = '\0';								      \
    (dst);								      \
  })
