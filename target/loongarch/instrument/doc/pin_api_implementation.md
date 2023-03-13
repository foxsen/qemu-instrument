# Pin API 实现情况

打勾的为已实现的API。各API的具体效果请参考 Intel Pin 的用户手册。

{{TODO 这里只列举了INS相关的API，其他的后续也加进来。}}
{{TODO 可以另外写一个文档，说明一些有差异的部分，以及原创的API（INS_InsertInlineAdd） }}

## Instrumentation API
- [x] PIN_CALLBACK 	INS_AddInstrumentFunction (INS_INSTRUMENT_CALLBACK fun, VOID *val)
- [x] 80  VOID 	INS_InsertPredicatedCall (INS ins, IPOINT ipoint, AFUNPTR funptr,...)
- [x] 650 VOID 	INS_InsertCall (INS ins, IPOINT action, AFUNPTR funptr,...)
- [x] 150 VOID 	INS_InsertIfCall (INS ins, IPOINT action, AFUNPTR funptr,...)
- [x] 176 VOID 	INS_InsertThenCall (INS ins, IPOINT action, AFUNPTR funptr,...)
- [ ] 5   VOID 	INS_InsertIfPredicatedCall (INS ins, IPOINT action, AFUNPTR funptr,...)
- [ ] 5   VOID 	INS_InsertThenPredicatedCall (INS ins, IPOINT action, AFUNPTR funptr,...)
- [ ] 26  VOID 	INS_InsertFillBuffer (INS ins, IPOINT action, BUFFER_ID id,...)
- [ ] 4   VOID 	INS_InsertFillBufferPredicated (INS ins, IPOINT action, BUFFER_ID id,...)
- [ ] 4   VOID 	INS_InsertFillBufferThen (INS ins, IPOINT action, BUFFER_ID id,...)

## IARG_TYPE 实现情况

i: unimplemented, s: unsupported, r: reserved

- [x] IARG_INVALID,
- [x] IARG_ADDRINT,
- [x] IARG_PTR,
- [x] IARG_BOOL,
- [x] IARG_UINT32,
- [x] IARG_UINT64,
- [x] IARG_INST_PTR,
- [x] IARG_REG_VALUE,
- [x] IARG_REG_REFERENCE,
- [i] IARG_REG_CONST_REFERENCE,
- [x] IARG_MEMORYREAD_EA,
- [s] IARG_MEMORYREAD2_EA,
- [x] IARG_MEMORYWRITE_EA,
- [x] IARG_MEMORYREAD_SIZE,
- [x] IARG_MEMORYWRITE_SIZE,
- [i] IARG_MEMORYREAD_PTR,
- [s] IARG_MEMORYREAD2_PTR,
- [i] IARG_MEMORYWRITE_PTR,
- [i] IARG_MEMORYOP_PTR,
- [i] IARG_MULTI_MEMORYACCESS_EA,
- [i] IARG_MULTI_ELEMENT_OPERAND,
- [i] IARG_REWRITE_SCATTERED_MEMOP,
- [i] IARG_EXPLICIT_MEMORY_EA,
- [x] IARG_BRANCH_TAKEN,
- [x] IARG_BRANCH_TARGET_ADDR,
- [x] IARG_FALLTHROUGH_ADDR,
- [x] IARG_EXECUTING,
- [s] IARG_FIRST_REP_ITERATION,
- [r] IARG_PREDICATE,
- [r] IARG_STACK_VALUE,
- [r] IARG_STACK_REFERENCE,
- [r] IARG_MEMORY_VALUE,
- [r] IARG_MEMORY_REFERENCE,
- [x] IARG_SYSCALL_NUMBER,
- [x] IARG_SYSARG_REFERENCE,
- [x] IARG_SYSARG_VALUE,
- [i] IARG_SYSRET_VALUE,
- [i] IARG_SYSRET_ERRNO,
- [i] IARG_FUNCARG_CALLSITE_REFERENCE,
- [x] IARG_FUNCARG_CALLSITE_VALUE,
- [i] IARG_FUNCARG_ENTRYPOINT_REFERENCE,
- [x] IARG_FUNCARG_ENTRYPOINT_VALUE,
- [i] IARG_FUNCRET_EXITPOINT_REFERENCE,
- [x] IARG_FUNCRET_EXITPOINT_VALUE,
- [x] IARG_RETURN_IP,
- [i] IARG_ORIG_FUNCPTR,
- [i] IARG_PROTOTYPE,
- [i] IARG_THREAD_ID,
- [i] IARG_CONTEXT,
- [i] IARG_CONST_CONTEXT,
- [i] IARG_PARTIAL_CONTEXT,
- [i] IARG_PRESERVE,
- [i] IARG_RETURN_REGS,
- [i] IARG_CALL_ORDER,
- [i] IARG_IARGLIST,
- [s] IARG_FAST_ANALYSIS_CALL,
- [x] IARG_MEMORYOP_EA,
- [x] IARG_MEMORYOP_SIZE,
- [s] IARG_MEMORYOP_MASKED_ON,
- [x] IARG_TSC,
- [r] IARG_FILE_NAME,
- [r] IARG_LINE_NO,
- [r] IARG_LAST,
- [x] IARG_END

注：IARG_TSC

### 访存指令

{{FIXME 这段我也不知道是啥，之前实现Pin API时记录的信息}}

(addr = rj + si12)
ld.b rd, rj, si12
ld.h rd, rj, si12
ld.w rd, rj, si12
ld.d rd, rj, si12
ld.bu rd, rj, si12
ld.hu rd, rj, si12
ld.wu rd, rj, si12
st.b rd, rj, si12
st.h rd, rj, si12
st.w rd, rj, si12
st.d rd, rj, si12

(addr = rj + rk)
ldx.b rd, rj, rk
ldx.h rd, rj, rk
ldx.w rd, rj, rk
ldx.d rd, rj, rk
ldx.bu rd, rj, rk
ldx.hu rd, rj, rk
ldx.wu rd, rj, rk
stx.b rd, rj, rk
stx.h rd, rj, rk
stx.w rd, rj, rk
stx.d rd, rj, rk

(addr = rj + (si14 << 2))
ldptr.w rd, rj, si14
ldptr.d rd, rj, si14
stptr.w rd, rj, si14
stptr.d rd, rj, si14

(addr = rj + si12)
preld hint, rj, si12

(复杂，见文档)
preldx hint, rj, rk

边界访存 

(addr = rj, rk是边界检查)
ldgt.b rd, rj, rk
ldgt.h rd, rj, rk
ldgt.w rd, rj, rk
ldgt.d rd, rj, rk
ldle.b rd, rj, rk
ldle.h rd, rj, rk
ldle.w rd, rj, rk
ldle.d rd, rj, rk
stgt.b rd, rj, rk
stgt.h rd, rj, rk
stgt.w rd, rj, rk
stgt.d rd, rj, rk
stle.b rd, rj, rk
stle.h rd, rj, rk
stle.w rd, rj, rk
stle.d rd, rj, rk

原子访存 

(addr = rj)
amswap.w rd, rk, rj
amswap_db.w rd, rk, rj
amswap.d rd, rk, rj
amswap_db.d rd, rk, rj
amadd.w rd, rk, rj
amadd_db.w rd, rk, rj
amadd.d rd, rk, rj
amadd_db.d rd, rk, rj
amand.w rd, rk, rj
amand_db.w rd, rk, rj
amand.d rd, rk, rj
amand_db.d rd, rk, rj
amor.w rd, rk, rj
amor_db.w rd, rk, rj
amor.d rd, rk, rj
amor_db.d rd, rk, rj
amxor.w rd, rk, rj
amxor_db.w rd, rk, rj
amxor.d rd, rk, rj
amxor_db.d rd, rk, rj
ammax.w rd, rk, rj
ammax_db.w rd, rk, rj
ammax.d rd, rk, rj
ammax_db.d rd, rk, rj
ammin.w rd, rk, rj
ammin_db.w rd, rk, rj
ammin.d rd, rk, rj
ammin_db.d rd, rk, rj
ammax.wu rd, rk, rj
ammax_db.wu rd, rk, rj
ammax.du rd, rk, rj
ammax_db.du rd, rk, rj
ammin.wu rd, rk, rj
ammin_db.wu rd, rk, rj
ammin.du rd, rk, rj
ammin_db.du rd, rk, rj

LL/SC 

(addr = rj + si14 << 2)
ll.wrd, rj, si14
ll.drd, rj, si14
sc.wrd, rj, si14
sc.drd, rj, si14

浮点普通访存

(addr = rj + si12)
fld.s fd, rj, si12
fld.d fd, rj, si12
fst.s fd, rj, si12
fst.d fd, rj, si12

(addr = rj + rk)
fldx.s fd, rj, rk
fldx.d fd, rj, rk
fstx.s fd, rj, rk
fstx.d fd, rj, rk

浮点边界访存

(addr = rj)
fldgt.s fd, rj, rk
fldgt.d fd, rj, rk
fldle.s fd, rj, rk
fldle.d fd, rj, rk
fstgt.s fd, rj, rk
fstgt.d fd, rj, rk
fstle.s fd, rj, rk
fstle.d fd, rj, rk

向量扩展

(addr = rj + si12)
vld vd, rj, s12
xvld vd, rj, s12
vst vd, rj, si12
xvst vd, rj, si12

(addr = rj + rk)
vldx vd, rj, rk
xvldx vd, rj, rk
vstx vd, rj, rk
xvstx vd, rj, rk

vldrepl.b vd, rj, si12      (rj + si12)
xvldrepl.b xd, rj, si12     
vldrepl.h vd, rj, si11      (rj + si11 << 1)
xvldrepl.h xd, rj, si11
vldrepl.w vd, rj, si10      (rj + si10 << 2)
xvldrepl.w xd, rj, si10
vldrepl.d vd, rj, si9       (rj + si9 << 3)
xvldrepl.d xd, rj, si9
vstelm.b vd, rj, si8, idx   (rj + si8)
xvstelm.b xd, rj, si8, idx
vstelm.h vd, rj, si8, idx   (rj + si8 << 1)
xvstelm.h xd, rj, si8, idx
vstelm.w vd, rj, si8, idx   (rj + si8 << 2)
xvstelm.w xd, rj, si8, idx
vstelm.d vd, rj, si8, idx   (rj + si8 << 3)
xvstelm.d xd, rj, si8, idx

二进制扩展

(addr = rj + si12) 长度不定
ldl.w rd, rj, si12
ldl.d rd, rj, si12
ldr.w rd, rj, si12
ldr.d rd, rj, si12
stl.w rd, rj, si12
stl.d rd, rj, si12
str.w rd, rj, si12
str.d rd, rj, si12

## INS inspection API

i: unimplemented, s: unsupported, r: reserved, d: deprecated

- [s] BOOL INS_HasRealRep (INS ins)
- [s] BOOL INS_IsStandardMemop (INS ins)
- [s] BOOL INS_IsCacheLineFlush (INS ins)
- [s] INT32 INS_Category (const INS ins)
- [s] INT32 INS_Extension (const INS ins)
- [i] UINT32 INS_EffectiveAddressWidth (INS ins)
- [d] PIN_DEPRECATED_API USIZE INS_MemoryWriteSize (INS ins)
- [d] PIN_DEPRECATED_API USIZE INS_MemoryReadSize (INS ins)
- [x] PREDICATE INS_GetPredicate (INS ins)
- [x] BOOL INS_IsMemoryRead (INS ins)
- [x] BOOL INS_IsMemoryWrite (INS ins)
- [s] BOOL INS_HasMemoryRead2 (INS ins)
- [x] BOOL INS_HasFallThrough (INS ins)
- [s] BOOL INS_IsSysenter (INS ins)
- [s] BOOL INS_IsXbegin (INS ins)
- [s] BOOL INS_IsXend (INS ins)
- [s] BOOL INS_IsLea (INS ins)
- [s] BOOL INS_IsNop (INS ins)
- [s] std::string OPCODE_StringShort (UINT32 opcode)
- [x] std::string INS_Mnemonic (INS ins)
- [x] BOOL INS_IsBranch (INS ins)
- [x] BOOL INS_IsDirectBranch (INS ins)
- [x] BOOL INS_IsDirectCall (INS ins)
- [d] PIN_DEPRECATED_API BOOL INS_IsDirectBranchOrCall (INS ins)
- [x] BOOL INS_IsDirectControlFlow (INS ins)
- [s] BOOL INS_IsHalt (INS ins)
- [d] PIN_DEPRECATED_API BOOL INS_IsBranchOrCall (INS ins)
- [s] BOOL INS_Stutters (INS ins)
- [i] BOOL INS_IsPcMaterialization (INS ins)
- [x] BOOL INS_IsCall (INS ins)
- [x] BOOL INS_IsControlFlow (INS ins)
- [s] BOOL INS_IsFarCall (INS ins)
- [s] BOOL INS_IsFarJump (INS ins)
- [s] BOOL INS_IsDirectFarJump (INS ins)
- [i] BOOL INS_IsValidForIpointAfter (INS ins)
- [i] BOOL INS_IsValidForIpointTakenBranch (INS ins)
- [s] BOOL INS_IsVgather (INS ins)
- [s] BOOL INS_IsVscatter (INS ins)
- [i] BOOL INS_HasMemoryVector (INS ins)
- [s] BOOL INS_HasScatteredMemoryAccess (INS ins)
- [s] BOOL INS_IsAmx (INS ins)
- [s] VOID INS_GetFarPointer (INS ins, UINT16 &segment_selector, UINT32 &displacement)
- [i] BOOL INS_IsProcedureCall (INS ins)
- [x] BOOL INS_IsInterrupt (INS ins)
- [x] BOOL INS_IsRet (INS ins)
- [s] BOOL INS_IsSysret (INS ins)
- [s] BOOL INS_IsFarRet (INS ins)
- [x] BOOL INS_IsPrefetch (INS ins)
- [i] BOOL INS_IsSub (const INS ins)
- [x] BOOL INS_IsMov (const INS ins)
- [s] BOOL INS_IsMovdir64b (const INS ins)
- [s] BOOL INS_IsMovFullRegRegSame (const INS ins)
- [x] BOOL INS_IsAtomicUpdate (const INS ins)
- [s] BOOL INS_IsRDTSC (const INS ins)
- [s] BOOL INS_IsMaskMov (const INS ins)
- [s] REG INS_RepCountRegister (INS ins)
- [d] PIN_DEPRECATED_API BOOL INS_IsIndirectBranchOrCall (INS ins)
- [x] BOOL INS_IsIndirectControlFlow (INS ins)
- [s] REG INS_SegmentRegPrefix (INS ins)
- [x] BOOL INS_HasExplicitMemoryReference (INS ins)
- [s] VOID PIN_SetSyntaxIntel ()
- [s] VOID PIN_SetSyntaxATT ()
- [s] VOID PIN_SetSyntaxXED ()
- [s] REG INS_RegR (INS x, UINT32 k)
- [s] REG INS_RegW (INS x, UINT32 k)
- [x] OPCODE INS_Opcode (INS ins)
- [s] std::string CATEGORY_StringShort (UINT32 num)
- [s] std::string EXTENSION_StringShort (UINT32 num)
- [s] BOOL INS_SegPrefixIsMemoryRead (INS ins)
- [s] BOOL INS_SegPrefixIsMemoryWrite (INS ins)
- [s] BOOL INS_AddressSizePrefix (INS ins)
- [s] BOOL INS_BranchNotTakenPrefix (INS ins)
- [s] BOOL INS_BranchTakenPrefix (INS ins)
- [s] BOOL INS_LockPrefix (INS ins)
- [s] BOOL INS_OperandSizePrefix (INS ins)
- [s] BOOL INS_RepPrefix (INS ins)
- [s] BOOL INS_RepnePrefix (INS ins)
- [s] BOOL INS_SegmentPrefix (INS ins)
- [s] BOOL INS_IsXchg (INS ins)
- [s] BOOL INS_IsStringop (INS ins)
- [s] BOOL INS_IsIRet (INS ins)
- [s] UINT32 INS_MaxNumRRegs (INS x)
- [s] UINT32 INS_MaxNumWRegs (INS x)
- [s] BOOL INS_RegRContain (const INS ins, const REG reg)
- [s] BOOL INS_RegWContain (const INS ins, const REG reg)
- [s] BOOL INS_FullRegRContain (const INS ins, const REG reg)
- [s] BOOL INS_FullRegWContain (const INS ins, const REG reg)
- [s] BOOL INS_IsStackRead (const INS ins)
- [s] BOOL INS_IsStackWrite (const INS ins)
- [s] BOOL INS_IsIpRelRead (const INS ins)
- [s] BOOL INS_IsIpRelWrite (const INS ins)
- [s] BOOL INS_IsPredicated (INS ins)
- [i] BOOL INS_IsOriginal (INS ins)
- [x] std::string INS_Disassemble (INS ins)
- [s] ADDRDELTA INS_MemoryDisplacement (INS ins)
- [s] REG INS_MemoryBaseReg (INS ins)
- [s] REG INS_MemoryIndexReg (INS ins)
- [s] UINT32 INS_MemoryScale (INS ins)
- [i] BOOL INS_ChangeReg (const INS ins, const REG old_reg, const REG new_reg, const BOOL as_read)
- [s] xed_decoded_inst_t * INS_XedDec (INS ins)
- [s] REG INS_XedExactMapToPinReg (xed_reg_enum_t xed_reg)
- [d] PIN_DEPRECATED_API REG INS_XedExactMapToPinReg (unsigned int r)
- [s] xed_reg_enum_t INS_XedExactMapFromPinReg (REG pin_reg)
- [x] UINT32 INS_OperandCount (INS ins)
- [s] UINT32 INS_OperandElementCount (INS ins, UINT32 opIdx)
- [s] BOOL INS_OperandHasElements (INS ins, UINT32 opIdx)
- [s] USIZE INS_OperandElementSize (INS ins, UINT32 opIdx)
- [s] UINT32 INS_OperandNameId (INS ins, UINT32 n)
- [i] BOOL INS_OperandIsMemory (INS ins, UINT32 n)
- [s] REG INS_OperandMemoryBaseReg (INS ins, UINT32 n)
- [s] REG INS_OperandMemoryIndexReg (INS ins, UINT32 n)
- [s] REG INS_OperandMemorySegmentReg (INS ins, UINT32 n)
- [s] UINT32 INS_OperandMemoryScale (INS ins, UINT32 n)
- [s] ADDRDELTA INS_OperandMemoryDisplacement (INS ins, UINT32 n)
- [s] BOOL INS_OperandIsFixedMemop (INS ins, UINT32 n)
- [s] VOID GetNumberAndSizeOfMemAccesses (INS ins, int *numAccesses, int *accessSize, int *indexSize)
- [x] UINT32 INS_MemoryOperandCount (INS ins)
- [s] UINT32 INS_MemoryOperandElementCount (INS ins, UINT32 memoryOp)
- [x] USIZE INS_MemoryOperandSize (INS ins, UINT32 memoryOp)
- [s] USIZE INS_MemoryOperandElementSize (INS ins, UINT32 memoryOp)
- [s] BOOL INS_OperandIsAddressGenerator (INS ins, UINT32 n)
- [s] BOOL INS_OperandIsBranchDisplacement (INS ins, UINT32 n)
- [x] BOOL INS_MemoryOperandIsRead (INS ins, UINT32 memopIdx)
- [x] BOOL INS_MemoryOperandIsWritten (INS ins, UINT32 memopIdx)
- [i] UINT32 INS_MemoryOperandIndexToOperandIndex (INS ins, UINT32 memopIdx)
- [x] BOOL INS_OperandIsReg (INS ins, UINT32 n)
- [s] BOOL INS_OperandIsSegmentReg (INS ins, UINT32 n)
- [x] REG INS_OperandReg (INS ins, UINT32 n)
- [x] BOOL INS_OperandIsImmediate (INS ins, UINT32 n)
- [x] UINT64 INS_OperandImmediate (INS ins, UINT32 n)
- [i] BOOL INS_OperandIsImplicit (INS ins, UINT32 n)
- [i] BOOL INS_RegIsImplicit (INS ins, REG reg)
- [s] UINT32 INS_OperandWidth (INS ins, UINT32 n)
- [s] UINT32 INS_OperandSize (INS ins, UINT32 n)
- [x] BOOL INS_OperandRead (INS ins, UINT32 n)
- [x] BOOL INS_OperandWritten (INS ins, UINT32 n)
- [x] BOOL INS_OperandReadOnly (INS ins, UINT32 n)
- [x] BOOL INS_OperandWrittenOnly (INS ins, UINT32 n)
- [x] BOOL INS_OperandReadAndWritten (INS ins, UINT32 n)
- [s] REG INS_MaskRegister (INS ins)
- [x] BOOL INS_IsSyscall (INS ins)
- [s] SYSCALL_STANDARD INS_SyscallStd (INS ins)
- [i] RTN INS_Rtn (INS x)
- [x] INS INS_Next (INS x)
- [x] INS INS_Prev (INS x)
- [x] INS INS_Invalid ()
- [x] BOOL INS_Valid (INS x)
- [x] ADDRINT INS_Address (INS ins)
- [x] USIZE INS_Size (INS ins)
- [d] PIN_DEPRECATED_API ADDRINT INS_DirectBranchOrCallTargetAddress (INS ins)
- [x] ADDRINT INS_DirectControlFlowTargetAddress (INS ins)
- [x] ADDRINT INS_NextAddress (INS ins)
- [i] BOOL INS_IsAddedForFunctionReplacement (INS ins)
- [i] BOOL INS_IsValidForIarg (INS ins, IARG_TYPE argType)

