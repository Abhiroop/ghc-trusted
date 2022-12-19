/* -----------------------------------------------------------------------------
 *
 * (c) The GHC Team, 1998-2011
 *
 * STG-to-C glue.
 *
 * To run an STG function from C land, call
 *
 *              rv = StgRun(f,BaseReg);
 *
 * where "f" is the STG function to call, and BaseReg is the address of the
 * RegTable for this run (we might have separate RegTables if we're running
 * multiple threads on an SMP machine).
 *
 * In the end, "f" must JMP to StgReturn (defined below), passing the
 * return-value "rv" in R1, to return to the caller of StgRun returning "rv" in
 * the whatever way C returns a value.
 *
 * NOTE: StgRun/StgReturn do *NOT* load or store Hp or any other registers
 * (other than saving the C callee-saves registers). Instead, the called
 * function "f" must do that in STG land.
 *
 * We also initially make sure that there are @RESERVED_C_STACK_BYTES@ on the
 * C-stack. This is done to reserve some space for the allocation of
 * temporaries in STG code.
 *
 * -------------------------------------------------------------------------- */

#include "PosixSource.h"
#include "ghcconfig.h"

#if defined(sparc_HOST_ARCH) || defined(USE_MINIINTERPRETER)
/* include Stg.h first because we want real machine regs in here: we
 * have to get the value of R1 back from Stg land to C land intact.
 */

/* We include windows.h very early, as on Win64 the CONTEXT type has
   fields "R8", "R9" and "R10", which goes bad if we've already
   #define'd those names for our own purposes (in stg/Regs.h) */
#if defined(HAVE_WINDOWS_H)
#include <windows.h>
#endif

#define IN_STGCRUN 1
#include "Stg.h"
#include "Rts.h"
#else
/* The other architectures do not require the actual register macro definitions
 * here because they use hand written assembly to implement the StgRun
 * function. Including Stg.h first will define the R1 values using GCC specific
 * techniques, which we don't want for LLVM based C compilers. Since we don't
 * actually need the real machine register definitions here, we include the
 * headers in the opposite order to allow LLVM-based C compilers to work.
 */
#include "Rts.h"
#include "Stg.h"
#endif

#include "StgRun.h"
#include "Capability.h"

#include "RtsUtils.h"
#if defined(DEBUG)
#include "Printer.h"
#endif

#if defined(USE_MINIINTERPRETER)

/* -----------------------------------------------------------------------------
   any architecture (using miniinterpreter)
   -------------------------------------------------------------------------- */

StgRegTable * StgRun(StgFunPtr f, StgRegTable *basereg STG_UNUSED)
{
    while (f) {
        IF_DEBUG(interpreter,
                 debugBelch("Jumping to ");
                 printPtr((P_)f); fflush(stdout);
                 debugBelch("\n");
                 );
        f = (StgFunPtr) (f)();
    }
    return (StgRegTable *)R1.p;
}

StgFunPtr StgReturn(void)
{
    return 0;
}

#else /* !USE_MINIINTERPRETER */

#if defined(mingw32_HOST_OS)
/*
 * Note [Windows Stack allocations]
 *
 * On windows the stack has to be allocated 4k at a time, otherwise
 * we get a segfault.  The C compiler knows how to do this (it calls
 * _alloca()), so we make sure that we can allocate as much stack as
 * we need.  However since we are doing a local stack allocation and the value
 * isn't valid outside the frame, compilers are free to optimize this allocation
 * and the corresponding stack check away. So to prevent that we request that
 * this function never be optimized (See #14669).  */
STG_NO_OPTIMIZE StgWord8 *win32AllocStack(void)
{
    StgWord8 stack[RESERVED_C_STACK_BYTES + 16 + 12];
    return stack;
}
#endif

/* -----------------------------------------------------------------------------
   x86 architecture
   -------------------------------------------------------------------------- */

#if defined(i386_HOST_ARCH)

#if defined(darwin_HOST_OS) || defined(ios_HOST_OS)
#define STG_GLOBAL ".globl "
#define STG_HIDDEN ".private_extern "
#else
#define STG_GLOBAL ".global "
#define STG_HIDDEN ".hidden "
#endif

/*
 * Note [Stack Alignment on X86]
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * On X86 (both 32bit and 64bit) we keep the stack aligned on function calls at
 * a 16-byte boundary. This is done because on a number of architectures the
 * ABI requires this (x64, Mac OSX 32bit/64bit) as well as interfacing with
 * other libraries through the FFI.
 *
 * As part of this arrangment we must maintain the stack at a 16-byte boundary
 * - word_size-bytes (so 16n - 4 for i386 and 16n - 8 for x64) on entry to a
 * procedure since both GCC and LLVM expect this. This is because the stack
 * should have been 16-byte boundary aligned and then a call made which pushes
 * a return address onto the stack (so word_size more space used). In STG code
 * we only jump to other STG procedures, so we maintain the 16n - word_size
 * alignment for these jumps.
 *
 * This gives us binary compatibility with LLVM and GCC as well as dealing
 * with the FFI. Previously we just maintianed a 16n byte alignment for
 * procedure entry and calls, which led to bugs (see #4211 and #5250).
 *
 * To change this convention you need to change the code here, and in
 * compiler/nativeGen/X86/CodeGen.hs::GenCCall, and maybe the adjustor
 * code for thunks in rts/AdjustorAsm.s, rts/Adjustor.c.
 *
 * A quick way to see if this is wrong is to compile this code:
 *
 *    main = System.Exit.exitWith ExitSuccess
 *
 * And run it with +RTS -sstderr.  The stats code in the RTS, in
 * particular statsPrintf(), relies on the stack alignment because
 * it saves the %xmm regs on the stack, so it'll fall over if the
 * stack isn't aligned, and calling exitWith from Haskell invokes
 * shutdownHaskellAndExit using a C call.
 *
 * If you edit the sequence below be sure to update the unwinding information
 * for stg_stop_thread in StgStartup.cmm.
 */

static void GNUC3_ATTRIBUTE(used)
StgRunIsImplementedInAssembler(void)
{
    __asm__ volatile (
        STG_GLOBAL STG_RUN "\n"
#if !defined(mingw32_HOST_OS)
        STG_HIDDEN STG_RUN "\n"
#endif
        STG_RUN ":\n\t"

        /*
         * move %esp down to reserve an area for temporary storage
         * during the execution of STG code.
         *
         * The stack pointer has to be aligned to a multiple of 16
         * bytes from here - this is a requirement of the C ABI, so
         * that C code can assign SSE2 registers directly to/from
         * stack locations.
         */
        "subl %0, %%esp\n\t"

        /*
         * save callee-saves registers on behalf of the STG code.
         */
        "movl %%esp, %%eax\n\t"
        "addl %0-16, %%eax\n\t"
        "movl %%ebx,0(%%eax)\n\t"
        "movl %%esi,4(%%eax)\n\t"
        "movl %%edi,8(%%eax)\n\t"
        "movl %%ebp,12(%%eax)\n\t"
        /*
         * Set BaseReg
         */
        "movl 24(%%eax),%%ebx\n\t"
        /*
         * grab the function argument from the stack
         */
        "movl 20(%%eax),%%eax\n\t"
        /*
         * jump to it
         */
        "jmp *%%eax\n\t"

        STG_GLOBAL STG_RETURN "\n"
        STG_RETURN ":\n\t"

        "movl %%esi, %%eax\n\t"   /* Return value in R1  */

        /*
         * restore callee-saves registers.  (Don't stomp on %%eax!)
         */
        "movl %%esp, %%edx\n\t"
        "addl %0-16, %%edx\n\t"
        "movl 0(%%edx),%%ebx\n\t"       /* restore the registers saved above */
        "movl 4(%%edx),%%esi\n\t"
        "movl 8(%%edx),%%edi\n\t"
        "movl 12(%%edx),%%ebp\n\t"

        "addl %0, %%esp\n\t"
        "ret"

      : : "i" (RESERVED_C_STACK_BYTES + 16)
        // + 16 to make room for the 4 registers we have to save
        // See Note [Stack Alignment on X86]
    );
}

#endif // defined(i386_HOST_ARCH)

/* ----------------------------------------------------------------------------
   x86-64 is almost the same as plain x86.

   I've done it using entirely inline assembler, because I couldn't
   get gcc to generate the correct subtraction from %rsp by using
   the local array variable trick.  It didn't seem to reserve
   enough space.  Oh well, it's not much harder this way.
   ------------------------------------------------------------------------- */

#if defined(x86_64_HOST_ARCH)

#define STG_GLOBAL ".globl "

#if defined(darwin_HOST_OS) || defined(ios_HOST_OS)
#define STG_HIDDEN ".private_extern "
#else
#define STG_HIDDEN ".hidden "
#endif

/*
Note [Unwinding foreign exports on x86-64]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
For foreign exports, that is Haskell functions exported as C functions when
we unwind we have to unwind from Haskell code into C code. The current story
is as follows:

  * The Haskell stack always has stg_stop_thread_info frame at the bottom
  * We annotate stg_stop_thread_info to unwind the instruction pointer to a
    label inside StgRun called StgRunJmp. It's the last instruction before the
    code jumps into Haskell.
  * StgRun - which is implemented in assembler is annotated with some manual
    unwinding information. It unwinds all the registers that it has saved
    on the stack. This is important as rsp and rbp are often required for
    getting to the next frame and the rest of the saved registers are useful
    when inspecting locals in gdb.


 Example x86-64 stack for an FFI call
 from C into a Haskell function:


      HASKELL HEAP
      "ADDRESS SPACE"

  +--------------------+ <------ rbp
  |                    |
  |                    |
  |                    |
  |                    |
  |  Haskell           |
  |  evaluation stack  |
  |                    |
  |                    |
  |--------------------|
  |stg_catch_frame_info|
  |--------------------|
  |  stg_forceIO_info  |
  |--------------------|
  |stg_stop_thread_info| -------
  +--------------------+       |
           ...                 |
   (other heap objects)        |
           ...                 |
                               |
                               |
                               |
     C STACK "ADDRESS SPACE"   |
                               v
  +-----------------------------+ <------ rsp
  |                             |
  | RESERVED_C_STACK_BYTES ~16k |
  |                             |
  |-----------------------------|
  |             rbx             ||
  |-----------------------------| \
  |             rbp             | |
  |-----------------------------|  \
  |             r12             |  |
  |-----------------------------|   \
  |             r13             |   | STG_RUN_STACK_FRAME_SIZE
  |-----------------------------|  /
  |             r14             |  |
  |-----------------------------| /
  |             r15             | |
  |-----------------------------|/
  |  rip saved by call StgRun   |
  |        in schedule()        |
  +-----------------------------+
                ...
      schedule() stack frame


 Lower addresses on the top

One little snag in this approach is that the annotations accepted by the
assembler are surprisingly unexpressive. I had to resort to a .cfi_escape
and hand-assemble a DWARF expression. What made it worse was that big numbers
are LEB128 encoded, which makes them variable byte length, with length depending
on the magnitude.

Here's an example stack generated this way:

  Thread 1 "m" hit Breakpoint 1, Fib_zdfstableZZC0ZZCmainZZCFibZZCfib1_info () at Fib.hs:9
  9       fib a = return (a + 1)
  #0  Fib_zdfstableZZC0ZZCmainZZCFibZZCfib1_info () at Fib.hs:9
  #1  stg_catch_frame_info () at rts/Exception.cmm:372
  #2  stg_forceIO_info () at rts/StgStartup.cmm:178
  #3  stg_stop_thread_info () at rts/StgStartup.cmm:42
  #4  0x00000000007048ab in StgRunIsImplementedInAssembler () at rts/StgCRun.c:255
  #5  0x00000000006fcf42 in schedule (initialCapability=initialCapability@entry=0x8adac0 <MainCapability>, task=task@entry=0x8cf2a0) at rts/Schedule.c:451
  #6  0x00000000006fe18e in scheduleWaitThread (tso=0x4200006388, ret=<optimized out>, pcap=0x7fffffffdac0) at rts/Schedule.c:2533
  #7  0x000000000040a21e in hs_fib ()
  #8  0x000000000040a083 in main (argc=1, argv=0x7fffffffdc48) at m.cpp:15

(This is from patched gdb. See Note [Info Offset].)

The previous approach was to encode the unwinding information for select
registers in stg_stop_thread_info with Cmm annotations. The unfortunate thing
about that approach was that it required introduction of an artificial MachSp
register that wasn't meaningful outside unwinding. I discovered that to get
stack unwinding working under -threaded runtime I also needed to unwind rbp
which would require adding MachRbp. If we wanted to see saved locals in gdb,
we'd have to add more. The core of the problem is that Cmm is architecture
independent, while unwinding isn't.

Note [Unwinding foreign imports]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
For unwinding foreign imports, that is C functions exposed as Haskell functions
no special handling is required. The C function unwinds according to the rip
saved on the stack by the call instruction. Then we perform regular Haskell
stack unwinding.
*/


static void GNUC3_ATTRIBUTE(used)
StgRunIsImplementedInAssembler(void)
{
    __asm__ volatile (
        /*
         * save callee-saves registers on behalf of the STG code.
         */
        STG_GLOBAL STG_RUN "\n"
#if !defined(mingw32_HOST_OS)
        STG_HIDDEN STG_RUN "\n"
#endif
        STG_RUN ":\n\t"
        "subq %1, %%rsp\n\t"
        "movq %%rsp, %%rax\n\t"
        "subq %0, %%rsp\n\t"
        "movq %%rbx,0(%%rax)\n\t"
        "movq %%rbp,8(%%rax)\n\t"
        "movq %%r12,16(%%rax)\n\t"
        "movq %%r13,24(%%rax)\n\t"
        "movq %%r14,32(%%rax)\n\t"
        "movq %%r15,40(%%rax)\n\t"
#if defined(mingw32_HOST_OS)
        /*
         * Additional callee saved registers on Win64. This must match
         * callClobberedRegisters in compiler/nativeGen/X86/Regs.hs as
         * both represent the Win64 calling convention.
         */
        "movq %%rdi,48(%%rax)\n\t"
        "movq %%rsi,56(%%rax)\n\t"
        "movq %%xmm6,  64(%%rax)\n\t"
        "movq %%xmm7,  72(%%rax)\n\t"
        "movq %%xmm8,  80(%%rax)\n\t"
        "movq %%xmm9,  88(%%rax)\n\t"
        "movq %%xmm10, 96(%%rax)\n\t"
        "movq %%xmm11,104(%%rax)\n\t"
        "movq %%xmm12,112(%%rax)\n\t"
        "movq %%xmm13,120(%%rax)\n\t"
        "movq %%xmm14,128(%%rax)\n\t"
        "movq %%xmm15,136(%%rax)\n\t"
#endif

#if !defined(darwin_HOST_OS)
        /*
         * Let the unwinder know where we saved the registers
         * See Note [Unwinding foreign exports on x86-64].
         *
         * N.B. We don't support unwinding on Darwin due to
         * various toolchain insanity.
         */
        ".cfi_def_cfa rsp, 0\n\t"
        ".cfi_offset rbx, %c2\n\t"
        ".cfi_offset rbp, %c3\n\t"
        ".cfi_offset r12, %c4\n\t"
        ".cfi_offset r13, %c5\n\t"
        ".cfi_offset r14, %c6\n\t"
        ".cfi_offset r15, %c7\n\t"
        ".cfi_offset rip, %c8\n\t"
        ".cfi_escape " // DW_CFA_val_expression is not expressible otherwise
          "0x16, " // DW_CFA_val_expression
          "0x07, " // register num 7 - rsp
          "0x04, " // block length
          "0x77, " // DW_OP_breg7 - signed LEB128 offset from rsp
#define RSP_DELTA (RESERVED_C_STACK_BYTES + STG_RUN_STACK_FRAME_SIZE + 8)
          "%c9" // signed LEB128 encoded delta - byte 1
#if (RSP_DELTA >> 7) > 0
          ", %c10" // signed LEB128 encoded delta - byte 2
#endif

#if (RSP_DELTA >> 14) > 0
          ", %c11" // signed LEB128 encoded delta - byte 3
#endif

#if (RSP_DELTA >> 21) > 0
          ", %c12" // signed LEB128 encoded delta - byte 4
#endif

#if (RSP_DELTA >> 28) > 0
#error "RSP_DELTA too big"
#endif
          "\n\t"
#endif /* !defined(darwin_HOST_OS) */

        /*
         * Set BaseReg
         */
#if defined(mingw32_HOST_OS)
        "movq %%rdx,%%r13\n\t"
#else
        "movq %%rsi,%%r13\n\t"
#endif
        /*
         * grab the function argument from the stack, and jump to it.
         */
#if defined(mingw32_HOST_OS)
        "movq %%rcx,%%rax\n\t"
#else
        "movq %%rdi,%%rax\n\t"
#endif

        STG_GLOBAL xstr(STG_RUN_JMP) "\n"
#if !defined(mingw32_HOST_OS)
        STG_HIDDEN xstr(STG_RUN_JMP) "\n"
#endif
#if HAVE_SUBSECTIONS_VIA_SYMBOLS
        // If we have deadstripping enabled and a label is detected as unused
        // the code gets nop'd out.
        ".no_dead_strip " xstr(STG_RUN_JMP) "\n"
#endif
        xstr(STG_RUN_JMP) ":\n\t"
        "jmp *%%rax\n\t"

        ".globl " STG_RETURN "\n"
         STG_RETURN ":\n\t"

        "movq %%rbx, %%rax\n\t"   /* Return value in R1  */

        /*
         * restore callee-saves registers.  (Don't stomp on %%rax!)
         */
        "addq %0, %%rsp\n\t"
        "movq 0(%%rsp),%%rbx\n\t"       /* restore the registers saved above */
        "movq 8(%%rsp),%%rbp\n\t"
        "movq 16(%%rsp),%%r12\n\t"
        "movq 24(%%rsp),%%r13\n\t"
        "movq 32(%%rsp),%%r14\n\t"
        "movq 40(%%rsp),%%r15\n\t"
#if defined(mingw32_HOST_OS)
        "movq  48(%%rsp),%%rdi\n\t"
        "movq  56(%%rsp),%%rsi\n\t"
        "movq  64(%%rsp),%%xmm6\n\t"
        "movq  72(%%rsp),%%xmm7\n\t"
        "movq  80(%%rsp),%%xmm8\n\t"
        "movq  88(%%rsp),%%xmm9\n\t"
        "movq  96(%%rsp),%%xmm10\n\t"
        "movq 104(%%rsp),%%xmm11\n\t"
        "movq 112(%%rsp),%%xmm12\n\t"
        "movq 120(%%rsp),%%xmm13\n\t"
        "movq 128(%%rsp),%%xmm14\n\t"
        "movq 136(%%rsp),%%xmm15\n\t"
#endif
        "addq %1, %%rsp\n\t"
        "retq"

        :
        : "i"(RESERVED_C_STACK_BYTES),
          "i"(STG_RUN_STACK_FRAME_SIZE /* stack frame size */),
          "i"(RESERVED_C_STACK_BYTES /* rbx relative to cfa (rsp) */),
          "i"(RESERVED_C_STACK_BYTES + 8 /* rbp relative to cfa (rsp) */),
          "i"(RESERVED_C_STACK_BYTES + 16 /* r12 relative to cfa (rsp) */),
          "i"(RESERVED_C_STACK_BYTES + 24 /* r13 relative to cfa (rsp) */),
          "i"(RESERVED_C_STACK_BYTES + 32 /* r14 relative to cfa (rsp) */),
          "i"(RESERVED_C_STACK_BYTES + 40 /* r15 relative to cfa (rsp) */),
          "i"(RESERVED_C_STACK_BYTES + STG_RUN_STACK_FRAME_SIZE
              /* rip relative to cfa */)

#if !defined(darwin_HOST_OS)
          , "i"((RSP_DELTA & 127) | (128 * ((RSP_DELTA >> 7) > 0)))
            /* signed LEB128-encoded delta from rsp - byte 1 */
#if (RSP_DELTA >> 7) > 0
          , "i"(((RSP_DELTA >> 7) & 127) | (128 * ((RSP_DELTA >> 14) > 0)))
            /* signed LEB128-encoded delta from rsp - byte 2 */
#endif

#if (RSP_DELTA >> 14) > 0
          , "i"(((RSP_DELTA >> 14) & 127) | (128 * ((RSP_DELTA >> 21) > 0)))
            /* signed LEB128-encoded delta from rsp - byte 3 */
#endif

#if (RSP_DELTA >> 21) > 0
          , "i"(((RSP_DELTA >> 21) & 127) | (128 * ((RSP_DELTA >> 28) > 0)))
            /* signed LEB128-encoded delta from rsp - byte 4 */
#endif
#undef RSP_DELTA

#endif /* !defined(darwin_HOST_OS) */

        );
        /*
         * See Note [Stack Alignment on X86]
         */
}

#endif /* x86-64 */

/* -----------------------------------------------------------------------------
   Sparc architecture

   --
   OLD COMMENT from GHC-3.02:

   We want tailjumps to be calls, because `call xxx' is the only Sparc
   branch that allows an arbitrary label as a target.  (Gcc's ``goto
   *target'' construct ends up loading the label into a register and
   then jumping, at the cost of two extra instructions for the 32-bit
   load.)

   When entering the threaded world, we stash our return address in a
   known location so that \tr{%i7} is available as an extra
   callee-saves register.  Of course, we have to restore this when
   coming out of the threaded world.

   I hate this god-forsaken architecture.  Since the top of the
   reserved stack space is used for globals and the bottom is reserved
   for outgoing arguments, we have to stick our return address
   somewhere in the middle.  Currently, I'm allowing 100 extra
   outgoing arguments beyond the first 6.  --JSM

   Updated info (GHC 4.06): we don't appear to use %i7 any more, so
   I'm not sure whether we still need to save it.  Incedentally, what
   does the last paragraph above mean when it says "the top of the
   stack is used for globals"?  What globals?  --SDM

   Updated info (GHC 4.08.2): not saving %i7 any more (see below).
   -------------------------------------------------------------------------- */

#if defined(sparc_HOST_ARCH)

StgRegTable *
StgRun(StgFunPtr f, StgRegTable *basereg) {

    unsigned char space[RESERVED_C_STACK_BYTES];
#if 0
    register void *i7 __asm__("%i7");
    ((void **)(space))[100] = i7;
#endif
    f();
    __asm__ volatile (
                 ".align 4\n"
                 ".global " STG_RETURN "\n"
                 STG_RETURN ":"
                 : : "p" (space) : "l0","l1","l2","l3","l4","l5","l6","l7");
    /* we tell the C compiler that l0-l7 are clobbered on return to
     * StgReturn, otherwise it tries to use these to save eg. the
     * address of space[100] across the call.  The correct thing
     * to do would be to save all the callee-saves regs, but we
     * can't be bothered to do that.
     *
     * We also explicitly mark space as used since gcc eliminates it
     * otherwise.
     *
     * The code that gcc generates for this little fragment is now
     * terrible.  We could do much better by coding it directly in
     * assembler.
     */
#if 0
    /* updated 4.08.2: we don't save %i7 in the middle of the reserved
     * space any more, since gcc tries to save its address across the
     * call to f(), this gets clobbered in STG land and we end up
     * dereferencing a bogus pointer in StgReturn.
     */
    __asm__ volatile ("ld %1,%0"
                                : "=r" (i7) : "m" (((void **)(space))[100]));
#endif
    return (StgRegTable *)R1.i;
}

#endif

/* -----------------------------------------------------------------------------
   PowerPC architecture

   Everything is in assembler, so we don't have to deal with GCC...
   -------------------------------------------------------------------------- */

#if defined(powerpc_HOST_ARCH)

#define STG_GLOBAL ".globl "

#define STG_HIDDEN ".hidden "

#if defined(aix_HOST_OS)

// implementation is in StgCRunAsm.S

#else

// This version is for PowerPC Linux.

static void GNUC3_ATTRIBUTE(used)
StgRunIsImplementedInAssembler(void)
{
        __asm__ volatile (
                "\t.globl StgRun\n"
                "\t.hidden StgRun\n"
                "\t.type StgRun,@function\n"
                "StgRun:\n"
                "\tmflr 0\n"
                "\tstw 0,4(1)\n"
                "\tmr 5,1\n"
                "\tstwu 1,-%0(1)\n"
                "\tstmw 13,-220(5)\n"
                "\tstfd 14,-144(5)\n"
                "\tstfd 15,-136(5)\n"
                "\tstfd 16,-128(5)\n"
                "\tstfd 17,-120(5)\n"
                "\tstfd 18,-112(5)\n"
                "\tstfd 19,-104(5)\n"
                "\tstfd 20,-96(5)\n"
                "\tstfd 21,-88(5)\n"
                "\tstfd 22,-80(5)\n"
                "\tstfd 23,-72(5)\n"
                "\tstfd 24,-64(5)\n"
                "\tstfd 25,-56(5)\n"
                "\tstfd 26,-48(5)\n"
                "\tstfd 27,-40(5)\n"
                "\tstfd 28,-32(5)\n"
                "\tstfd 29,-24(5)\n"
                "\tstfd 30,-16(5)\n"
                "\tstfd 31,-8(5)\n"
                "\tmr 27,4\n"  // BaseReg == r27
                "\tmtctr 3\n"
                "\tmr 12,3\n"
                "\tbctr\n"
                ".globl StgReturn\n"
                "\t.type StgReturn,@function\n"
                "StgReturn:\n"
                "\tmr 3,14\n"
                "\tla 5,%0(1)\n"
                "\tlmw 13,-220(5)\n"
                "\tlfd 14,-144(5)\n"
                "\tlfd 15,-136(5)\n"
                "\tlfd 16,-128(5)\n"
                "\tlfd 17,-120(5)\n"
                "\tlfd 18,-112(5)\n"
                "\tlfd 19,-104(5)\n"
                "\tlfd 20,-96(5)\n"
                "\tlfd 21,-88(5)\n"
                "\tlfd 22,-80(5)\n"
                "\tlfd 23,-72(5)\n"
                "\tlfd 24,-64(5)\n"
                "\tlfd 25,-56(5)\n"
                "\tlfd 26,-48(5)\n"
                "\tlfd 27,-40(5)\n"
                "\tlfd 28,-32(5)\n"
                "\tlfd 29,-24(5)\n"
                "\tlfd 30,-16(5)\n"
                "\tlfd 31,-8(5)\n"
                "\tmr 1,5\n"
                "\tlwz 0,4(1)\n"
                "\tmtlr 0\n"
                "\tblr\n"
        : : "i"(RESERVED_C_STACK_BYTES+224 /*stack frame size*/));
}
#endif

#endif

/* -----------------------------------------------------------------------------
   PowerPC 64 architecture

   Everything is in assembler, so we don't have to deal with GCC...
   -------------------------------------------------------------------------- */

#if defined(powerpc64_HOST_ARCH)

static void GNUC3_ATTRIBUTE(used)
StgRunIsImplementedInAssembler(void)
{
        // r0 volatile
        // r1 stack pointer
        // r2 toc - needs to be saved
        // r3-r10 argument passing, volatile
        // r11, r12 very volatile (not saved across cross-module calls)
        // r13 thread local state (never modified, don't need to save)
        // r14-r31 callee-save
        __asm__ volatile (
                ".section \".opd\",\"aw\"\n"
                ".align 3\n"
                ".globl StgRun\n"
                ".hidden StgRun\n"
                "StgRun:\n"
                "\t.quad\t.StgRun,.TOC.@tocbase,0\n"
                "\t.size StgRun,24\n"
                ".globl StgReturn\n"
                "StgReturn:\n"
                "\t.quad\t.StgReturn,.TOC.@tocbase,0\n"
                "\t.size StgReturn,24\n"
                ".previous\n"
                ".globl .StgRun\n"
                ".type .StgRun,@function\n"
                ".StgRun:\n"
                "\tmflr 0\n"
                "\tmr 5, 1\n"
                "\tstd 0, 16(1)\n"
                "\tstdu 1, -%0(1)\n"
                "\tstd 2, -296(5)\n"
                "\tstd 14, -288(5)\n"
                "\tstd 15, -280(5)\n"
                "\tstd 16, -272(5)\n"
                "\tstd 17, -264(5)\n"
                "\tstd 18, -256(5)\n"
                "\tstd 19, -248(5)\n"
                "\tstd 20, -240(5)\n"
                "\tstd 21, -232(5)\n"
                "\tstd 22, -224(5)\n"
                "\tstd 23, -216(5)\n"
                "\tstd 24, -208(5)\n"
                "\tstd 25, -200(5)\n"
                "\tstd 26, -192(5)\n"
                "\tstd 27, -184(5)\n"
                "\tstd 28, -176(5)\n"
                "\tstd 29, -168(5)\n"
                "\tstd 30, -160(5)\n"
                "\tstd 31, -152(5)\n"
                "\tstfd 14, -144(5)\n"
                "\tstfd 15, -136(5)\n"
                "\tstfd 16, -128(5)\n"
                "\tstfd 17, -120(5)\n"
                "\tstfd 18, -112(5)\n"
                "\tstfd 19, -104(5)\n"
                "\tstfd 20, -96(5)\n"
                "\tstfd 21, -88(5)\n"
                "\tstfd 22, -80(5)\n"
                "\tstfd 23, -72(5)\n"
                "\tstfd 24, -64(5)\n"
                "\tstfd 25, -56(5)\n"
                "\tstfd 26, -48(5)\n"
                "\tstfd 27, -40(5)\n"
                "\tstfd 28, -32(5)\n"
                "\tstfd 29, -24(5)\n"
                "\tstfd 30, -16(5)\n"
                "\tstfd 31, -8(5)\n"
                "\tmr 27, 4\n"  // BaseReg == r27
                "\tld 2, 8(3)\n"
                "\tld 3, 0(3)\n"
                "\tmtctr 3\n"
                "\tbctr\n"
                ".globl .StgReturn\n"
                ".type .StgReturn,@function\n"
                ".StgReturn:\n"
                "\tmr 3,14\n"
                "\tla 5, %0(1)\n" // load address == addi r5, r1, %0
                "\tld 2, -296(5)\n"
                "\tld 14, -288(5)\n"
                "\tld 15, -280(5)\n"
                "\tld 16, -272(5)\n"
                "\tld 17, -264(5)\n"
                "\tld 18, -256(5)\n"
                "\tld 19, -248(5)\n"
                "\tld 20, -240(5)\n"
                "\tld 21, -232(5)\n"
                "\tld 22, -224(5)\n"
                "\tld 23, -216(5)\n"
                "\tld 24, -208(5)\n"
                "\tld 25, -200(5)\n"
                "\tld 26, -192(5)\n"
                "\tld 27, -184(5)\n"
                "\tld 28, -176(5)\n"
                "\tld 29, -168(5)\n"
                "\tld 30, -160(5)\n"
                "\tld 31, -152(5)\n"
                "\tlfd 14, -144(5)\n"
                "\tlfd 15, -136(5)\n"
                "\tlfd 16, -128(5)\n"
                "\tlfd 17, -120(5)\n"
                "\tlfd 18, -112(5)\n"
                "\tlfd 19, -104(5)\n"
                "\tlfd 20, -96(5)\n"
                "\tlfd 21, -88(5)\n"
                "\tlfd 22, -80(5)\n"
                "\tlfd 23, -72(5)\n"
                "\tlfd 24, -64(5)\n"
                "\tlfd 25, -56(5)\n"
                "\tlfd 26, -48(5)\n"
                "\tlfd 27, -40(5)\n"
                "\tlfd 28, -32(5)\n"
                "\tlfd 29, -24(5)\n"
                "\tlfd 30, -16(5)\n"
                "\tlfd 31, -8(5)\n"
                "\tmr 1, 5\n"
                "\tld 0, 16(1)\n"
                "\tmtlr 0\n"
                "\tblr\n"
        : : "i"(RESERVED_C_STACK_BYTES+304 /*stack frame size*/));
}

#endif

#if defined(powerpc64le_HOST_ARCH)
/* -----------------------------------------------------------------------------
   PowerPC 64 little endian architecture

   Really everything is in assembler, so we don't have to deal with GCC...
   -------------------------------------------------------------------------- */
#endif

/* -----------------------------------------------------------------------------
   ARM architecture
   -------------------------------------------------------------------------- */

#if defined(arm_HOST_ARCH)

#if defined(__thumb__)
#define THUMB_FUNC ".thumb\n\t.thumb_func\n\t"
#else
#define THUMB_FUNC
#endif

StgRegTable *
StgRun(StgFunPtr f, StgRegTable *basereg) {
    StgRegTable * r;
    __asm__ volatile (
        /*
         * save callee-saves registers on behalf of the STG code.
         */
        "stmfd sp!, {r4-r11, ip, lr}\n\t"
#if !defined(arm_HOST_ARCH_PRE_ARMv6)
        "vstmdb sp!, {d8-d11}\n\t"
#endif
        /*
         * allocate some space for Stg machine's temporary storage.
         * Note: RESERVED_C_STACK_BYTES has to be a round number here or
         * the assembler can't assemble it.
         */
        "sub sp, sp, %3\n\t"
        /*
         * Set BaseReg
         */
        "mov r4, %2\n\t"
        /*
         * Jump to function argument.
         */
        "bx %1\n\t"

        ".globl " STG_RETURN "\n\t"
        THUMB_FUNC
#if !defined(ios_HOST_OS)
        ".type " STG_RETURN ", %%function\n"
#endif
        STG_RETURN ":\n\t"
        /*
         * Free the space we allocated
         */
        "add sp, sp, %3\n\t"
        /*
         * Return the new register table, taking it from Stg's R1 (ARM's R7).
         */
        "mov %0, r7\n\t"
        /*
         * restore callee-saves registers.
         */
#if !defined(arm_HOST_ARCH_PRE_ARMv6)
        "vldmia sp!, {d8-d11}\n\t"
#endif
        "ldmfd sp!, {r4-r11, ip, lr}\n\t"
      : "=r" (r)
      : "r" (f), "r" (basereg), "i" (RESERVED_C_STACK_BYTES)
#if !defined(__thumb__)
        /* In ARM mode, r11/fp is frame-pointer and so we cannot mark
           it as clobbered. If we do so, GCC complains with error. */
      : "%r4", "%r5", "%r6", "%r7", "%r8", "%r9", "%r10", "%ip", "%lr"
#else
        /* In Thumb mode r7 is frame-pointer and so we cannot mark it
           as clobbered. On the other hand we mark as clobbered also
           those regs not used in Thumb mode. Hard to judge if this is
           needed, but certainly Haskell code is using them for
           placing GHC's virtual registers there. See
           includes/stg/MachRegs.h Please note that Haskell code is
           compiled by GHC/LLVM into ARM code (not Thumb!), at least
           as of February 2012 */
      : "%r4", "%r5", "%r6", "%r8", "%r9", "%r10", "%11", "%ip", "%lr"
#endif
    );
    return r;
}
#endif

#if defined(aarch64_HOST_ARCH)

StgRegTable *
StgRun(StgFunPtr f, StgRegTable *basereg) {
    StgRegTable * r;
    __asm__ volatile (
        /*
         * Save callee-saves registers on behalf of the STG code.
         * Floating point registers only need the bottom 64 bits preserved.
         * We need to use the names x16, x17, x29 and x30 instead of ip0
         * ip1, fp and lp because one of either clang or gcc doesn't understand
         * the later names.
         */
        "stp x29,  x30,  [sp, #-16]!\n\t"
        "mov x29, sp\n\t"
        "stp x16, x17, [sp, #-16]!\n\t"
        "stp x19, x20, [sp, #-16]!\n\t"
        "stp x21, x22, [sp, #-16]!\n\t"
        "stp x23, x24, [sp, #-16]!\n\t"
        "stp x25, x26, [sp, #-16]!\n\t"
        "stp x27, x28, [sp, #-16]!\n\t"
        "stp d8,  d9,  [sp, #-16]!\n\t"
        "stp d10, d11, [sp, #-16]!\n\t"
        "stp d12, d13, [sp, #-16]!\n\t"
        "stp d14, d15, [sp, #-16]!\n\t"

        /*
         * allocate some space for Stg machine's temporary storage.
         * Note: RESERVED_C_STACK_BYTES has to be a round number here or
         * the assembler can't assemble it.
         */
        "sub sp, sp, %3\n\t"
        /*
         * Set BaseReg
         */
        "mov x19, %2\n\t"
        /*
         * Jump to function argument.
         */
        "br %1\n\t"

        ".globl " STG_RETURN "\n\t"
#if !defined(ios_HOST_OS)
        ".type " STG_RETURN ", %%function\n"
#endif
        STG_RETURN ":\n\t"
        /*
         * Free the space we allocated
         */
        "add sp, sp, %3\n\t"
        /*
         * Return the new register table, taking it from Stg's R1 (ARM64's R22).
         */
        "mov %0, x22\n\t"
        /*
         * restore callee-saves registers.
         */

        "ldp d14, d15, [sp], #16\n\t"
        "ldp d12, d13, [sp], #16\n\t"
        "ldp d10, d11, [sp], #16\n\t"
        "ldp d8,  d9,  [sp], #16\n\t"
        "ldp x27, x28, [sp], #16\n\t"
        "ldp x25, x26, [sp], #16\n\t"
        "ldp x23, x24, [sp], #16\n\t"
        "ldp x21, x22, [sp], #16\n\t"
        "ldp x19, x20, [sp], #16\n\t"
        "ldp x16, x17, [sp], #16\n\t"
        "ldp x29,  x30,  [sp], #16\n\t"

      : "=r" (r)
      : "r" (f), "r" (basereg), "i" (RESERVED_C_STACK_BYTES)
        : "%x19", "%x20", "%x21", "%x22", "%x23", "%x24", "%x25", "%x26", "%x27", "%x28",
          "%x16", "%x17", "%x30"
    );
    return r;
}

#endif

#endif /* !USE_MINIINTERPRETER */
