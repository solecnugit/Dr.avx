# AVX-512 Instruction Rewriting Guide

This guide provides step-by-step instructions for adding new AVX-512 instruction rewrites to the Dr.avx.

## Table of Contents

1. [Overview](#overview)
2. [System Architecture](#system-architecture)
3. [Step-by-Step Implementation](#step-by-step-implementation)
4. [TLS State Management](#tls-state-management)
5. [Register Selection and Spill Management](#register-selection-and-spill-management)
6. [Implementation Patterns and Examples](#implementation-patterns-and-examples)
7. [Testing and Validation](#testing-and-validation)
8. [Best Practices](#best-practices)

## Overview

The AVX-512 instruction rewriting system in DynamoRIO provides a framework for transforming AVX-512 instructions at runtime. This is particularly useful for:

- **Compatibility**: Supporting AVX-512 instructions on systems without native hardware support
- **Instrumentation**: Adding analysis or profiling code around vector operations
- **Optimization**: Implementing custom optimizations or transformations
- **Debugging**: Providing detailed execution traces for vector operations

## System Architecture

### Core Components

The rewriting system consists of several key components:

1. **Opcode Detection**: Instructions with EVEX prefix or mask register operations are identified as AVX-512 instructions
    ```bash
    |=> avx512 instr at:0x0000000000401263, current pc at:0x000000000040126b
        0x0000000000401263  62 f1 fd 48 6f 44 24 06 
        vmovdqa64 {%k0} 0x00000180(%rsp)[64byte] -> %zmm0
    ```
2. **Function Dispatch**: The `rewrite_funcs[]` array maps opcodes to their corresponding rewrite functions
3. **Rewrite Functions**: Individual functions that handle the transformation of specific instructions
4. **Register Management**: Utilities for managing register spills and TLS state
5. **Instruction Generation**: Helper functions for creating replacement instruction sequences

### Opcode Mapping

The system uses opcode indices defined in `/core/ir/x86/opcode_api.h` to map instructions to rewrite functions, note that the opcodes in `rewrite_funcs[]` is the superset of the whole AVX-512 opcodes. We hack this so that no additonal lookup overhead of DynmoaRIO native opcode is required:

```c
#define AVX512_FIRST_OP OP_vmovss
#define AVX512_LAST_OP OP_vshufi64x2
#define TO_AVX512_RWFUNC_INDEX(opcode) (opcode - AVX512_FIRST_OP)
```

## Step-by-Step Implementation

### Step 1: Register the Function in `rewrite_funcs[]`

The `rewrite_funcs[]` array in `core/arch/rewrite.c` maps opcode indices to rewrite function pointers.

**Location**: `core/arch/rewrite.c` (around line 50)

**Process**:
1. Find the appropriate index for your instruction using the opcode value
2. Replace `rw_func_empty` with your function name
3. Add a comment with the opcode number for reference

**Example**:
```c
instr_rewrite_func_t *rewrite_funcs[] = {
    /* 1 OP_AVX512_vmovss */ rw_func_empty,
    /* 2 OP_AVX512_vmovsd */ rw_func_empty,  // <- rw_func_vmovsd to register
    // ...
};
```

**Important Notes**:
- The array index corresponds to `(opcode - AVX512_FIRST_OP)`
- Comments should include the array index number for easy reference
- Use `rw_func_empty` for unimplemented instructions
- Use `rw_func_invalid` for invalid or unsupported opcodes

### Step 2: Add Function Declaration to `rewrite.h`

Add your function declaration to the header file to make it available system-wide.

**Location**: `core/arch/rewrite.h`

**Process**:
1. Find the appropriate location (functions are roughly ordered by opcode number)
2. Add your function declaration with the standard signature
3. Include a comment with the opcode index number

**Function Signature**:
```c
instr_t *
rw_func_your_instruction(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);
```

**Example**:
```c
instr_t * /* 154 */
rw_func_vpaddd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);
```

**Parameters Explained**:
- `dcontext_t *dcontext`: Dynamic context containing thread-local state
- `instrlist_t *ilist`: Instruction list where the rewritten instructions will be inserted
- `instr_t *instr`: The original AVX-512 instruction to be rewritten
- `app_pc instr_start`: Start address of the instruction when instruction is emitted in the application, in buiding and rewriting stage it's 0x0000

### Step 3: Implement the Function Body

Create the actual implementation of your rewrite function in `core/arch/rewrite.c`.

**Basic Structure**:
```c
instr_t *
rw_func_your_instruction(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start)
{
    // 1. Extract operands from the original instruction
    opnd_t mask_opnd = instr_get_src(instr, 0);  // Mask register (if present)
    opnd_t src1_opnd = instr_get_src(instr, 1);  // First source operand
    opnd_t src2_opnd = instr_get_src(instr, 2);  // Second source operand (if present)
    opnd_t dst_opnd = instr_get_dst(instr, 0);   // Destination operand
    
    // 2. Extract register IDs
    // in many cases, opnd is not reg_kind, you need to use proper way to extract them, like base_disp_kind, reladdr_kind, absaddr_kine, .etc
    reg_id_t mask_reg = opnd_get_reg(mask_opnd);
    reg_id_t src1_reg = opnd_get_reg(src1_opnd);
    reg_id_t dst_reg = opnd_get_reg(dst_opnd);
    
#ifdef DEBUG
    // 3. Optional: Print debug information
    print_rewrite_info(dcontext, ilist, instr, instr_start, "your_instruction", 
                       true, true, false, true);
#endif

    // 4. Handle different operand combinations
    switch (src1_opnd.kind) {
    case REG_kind: {
        // Register-to-register operations
        if (dst_opnd.kind == REG_kind) {
            // Determine register size and call appropriate generator
            if (IS_XMM_REG(dst_reg))
                return your_instruction_xmm_reg2reg_gen(dcontext, ilist, instr, 
                                                       src1_reg, dst_reg, mask_reg);
            if (IS_YMM_REG(dst_reg))
                return your_instruction_ymm_reg2reg_gen(dcontext, ilist, instr, 
                                                       src1_reg, dst_reg, mask_reg);
            if (IS_ZMM_REG(dst_reg))
                return your_instruction_zmm_reg2reg_gen(dcontext, ilist, instr, 
                                                       src1_reg, dst_reg, mask_reg);
        } else {
            // Register-to-memory operations
            return your_instruction_reg2mem_gen(dcontext, ilist, instr, 
                                               src1_reg, dst_opnd, mask_reg);
        }
        break;
    }
    case BASE_DISP_kind: {
        // Memory-to-register operations
        return your_instruction_mem2reg_gen(dcontext, ilist, instr, 
                                           src1_opnd, dst_reg, mask_reg);
        break;
    }
    default:
        print_file(STD_OUTF, "[WARN]: your_instruction pattern not supported\n");
        return NULL_INSTR;
    }
    
    return NULL_INSTR;
}
```

**Key Implementation Points**:
- Always remove and destroy the original instruction: `instrlist_remove(ilist, instr); instr_destroy(dcontext, instr);`
- Handle different register sizes (XMM/YMM/ZMM) appropriately
- Support both register and memory operands
- Return the first instruction in the replacement sequence

## TLS State Management

We offer customized Thread-Local Storage (TLS) slots to save and restore register state during instruction rewriting. This is essential when registers need to be spilled to accommodate the rewrite process.

### TLS Slot Organization

The TLS structure provides dedicated slots for different register types:

```c
// ZMM register slots (0-31), each slot 512 bits
#define TLS_ZMM_idx_SLOT(zmm_idx) ((ushort)offsetof(spill_state_t, zmm_regs[zmm_idx]))

// Mask register slots (0-7), each slot 64 bits
#define TLS_K_idx_SLOT(k_idx) ((ushort)offsetof(spill_state_t, k_regs[k_idx]))
```

### TLS Macros

Dr.avx provides convenient macros for TLS operations tailored for XMM, YMM, ZMM:

```c
// Save SIMD register to TLS with specific size
#define SAVE_SIMD_TO_SIZED_TLS(dc, reg, offs, sz) \
    instr_create_save_simd_to_sized_tls(dc, reg, offs, sz)

// Restore SIMD register from TLS with specific size  
#define RESTORE_SIMD_FROM_SIZED_TLS(dc, reg, offs, sz) \
    instr_create_restore_simd_from_sized_tls(dc, reg, offs, sz)

// SAVE K register to TLS with specific size
#define SAVE_TO_SIZED_TLS(dc, reg, offs, sz) 

// Restore K register from TLS with specific size
#define RESTORE_FROM_SIZED_TLS(dc, reg, offs, sz) 
```

### Register Size Constants

Use appropriate size constants for different register types:

```c
#define SIZE_OF_XMM 0x10  /* 16 bytes (128 bits) */
#define SIZE_OF_YMM 0x20  /* 32 bytes (256 bits) */  
#define SIZE_OF_ZMM 0x40  /* 64 bytes (512 bits) */

// Operand size constants
OPSZ_16  // For XMM registers
OPSZ_32  // For YMM registers  
OPSZ_64  // For ZMM registers
```

### TLS Usage Patterns

**Saving a Register**:
```c
// Save XMM register to its TLS slot
instr_t *save_instr = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_reg,
                                             TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_reg)), 
                                             OPSZ_16);
```

**Restoring a Register**:
```c
// Restore XMM register from its TLS slot
instr_t *restore_instr = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_reg,
                                                     TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(original_reg)), 
                                                     OPSZ_16);
```

**Complete Spill/Restore Sequence**:
```c
// 1. Save spill register's current value
instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_reg,
                                     TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_reg)), OPSZ_16);

// 2. Load original register's value into spill register  
instr_t *i2 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_reg,
                                          TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(original_reg)), OPSZ_16);

// 3. Perform the actual operation using spill register
instr_t *i3 = INSTR_CREATE_your_operation(dcontext, ...);

// 4. Restore spill register's original value
instr_t *i4 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_reg,
                                          TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_reg)), OPSZ_16);
```

## Register Selection and Spill Management

The system provides utilities for selecting appropriate spill registers and managing register conflicts.

### Register Classification Macros

```c
// Check if register needs spilling (high registers: XMM16-31, YMM16-31, ZMM16-31)
#define NEED_SPILL_XMM(reg) (reg >= DR_REG_XMM16 && reg <= DR_REG_XMM31)
#define NEED_SPILL_YMM(reg) (reg >= DR_REG_YMM16 && reg <= DR_REG_YMM31)  
#define NEED_SPILL_ZMM(reg) (reg >= DR_REG_ZMM16 && reg <= DR_REG_ZMM31)

// Register type checking
#define IS_XMM_REG(reg) (reg >= DR_REG_XMM0 && reg <= DR_REG_XMM31)
#define IS_YMM_REG(reg) (reg >= DR_REG_YMM0 && reg <= DR_REG_YMM31)
#define IS_ZMM_REG(reg) (reg >= DR_REG_ZMM0 && reg <= DR_REG_ZMM31)
```

### Spill Register Selection Functions

**Single Register Selection**:
```c
// Find one available XMM spill register avoiding the specified register
reg_id_t find_one_available_spill_xmm(reg_id_t reg_to_avoid);

// Find one available YMM spill register avoiding the specified register  
reg_id_t find_one_available_spill_ymm(reg_id_t reg_to_avoid);

// Find one available ZMM spill register avoiding the specified register
reg_id_t find_one_available_spill_zmm(reg_id_t reg_to_avoid);
```

**Multiple Register Avoidance**:
```c
// Find spill register avoiding up to 3 specific registers
reg_id_t find_available_spill_xmm_avoiding(reg_id_t avoid1, reg_id_t avoid2, reg_id_t avoid3);
reg_id_t find_available_spill_ymm_avoiding(reg_id_t avoid1, reg_id_t avoid2, reg_id_t avoid3);
reg_id_t find_available_spill_zmm_avoiding(reg_id_t avoid1, reg_id_t avoid2, reg_id_t avoid3);
```

**Variadic Register Avoidance**:
```c
// Find spill register avoiding a variable number of registers
reg_id_t find_available_spill_xmm_avoiding_variadic(int num_regs, ...);
reg_id_t find_available_spill_ymm_avoiding_variadic(int num_regs, ...);
```

**GPR (General Purpose Register) Selection**:
```c
// Find available general-purpose registers for address calculations
reg_id_t find_one_available_spill_gpr_avoiding_variadic(int num_avoids, ...);
```

### Spill Management Patterns

**Determining Spill Requirements**:
```c
// Create bit flags for spill requirements
const uint src1_need_spill_flag = NEED_SPILL_YMM(src1_reg) ? 1 : 0;
const uint src2_need_spill_flag = NEED_SPILL_YMM(src2_reg) ? 2 : 0;  
const uint dst_need_spill_flag = NEED_SPILL_YMM(dst_reg) ? 4 : 0;
const uint need_spill_flag = src1_need_spill_flag | src2_need_spill_flag | dst_need_spill_flag;

// Handle different spill scenarios based on flags
switch (need_spill_flag) {
case 0: // No spill needed
    // Direct operation
    break;
case 1: // Only src1 needs spill
    // Spill src1, perform operation, restore
    break;
case 2: // Only src2 needs spill  
    // Spill src2, perform operation, restore
    break;
// ... handle other combinations
}
```

**Smart Register Selection**:
```c
// Example: Selecting spill register for src1 while avoiding conflicts
reg_id_t spill_src1_reg;
if (src2_reg == dst_reg) {
    // If src2 and dst are the same, only avoid that register
    spill_src1_reg = find_one_available_spill_ymm(src2_reg);
} else {
    // Avoid both src2 and dst registers
    spill_src1_reg = find_available_spill_ymm_avoiding(src2_reg, dst_reg, DR_REG_NULL);
}
```

## Implementation Patterns and Examples

### Pattern 1: Simple Register-to-Register Operation

For instructions that don't require complex operand handling:

```c
instr_t *
simple_reg2reg_gen(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr,
                   reg_id_t src_reg, reg_id_t dst_reg, reg_id_t mask_reg)
{
    instrlist_remove(ilist, instr);
    instr_destroy(dcontext, instr);

    // Check if any registers need spilling
    const uint src_need_spill_flag = NEED_SPILL_YMM(src_reg) ? 1 : 0;
    const uint dst_need_spill_flag = NEED_SPILL_YMM(dst_reg) ? 2 : 0;
    const uint need_spill_flag = src_need_spill_flag | dst_need_spill_flag;

    switch (need_spill_flag) {
    case 0: { // No spill needed
        opnd_t op_src = opnd_create_reg(src_reg);
        opnd_t op_dst = opnd_create_reg(dst_reg);
        return INSTR_CREATE_your_operation(dcontext, op_dst, op_src);
    }
    case 1: { // Source needs spill
        reg_id_t spill_reg = find_one_available_spill_ymm(dst_reg);
        // Implement spill/restore sequence...
    }
    // ... handle other cases
    }
}
```

### Pattern 2: Binary Operation with Two Sources

For instructions like `vpaddd` that have two source operands:

```c
instr_t *
binary_op_gen(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr,
              reg_id_t src1_reg, reg_id_t src2_reg, reg_id_t dst_reg, reg_id_t mask_reg)
{
    instrlist_remove(ilist, instr);
    instr_destroy(dcontext, instr);

    const uint src1_need_spill_flag = NEED_SPILL_YMM(src1_reg) ? 1 : 0;
    const uint src2_need_spill_flag = NEED_SPILL_YMM(src2_reg) ? 2 : 0;
    const uint dst_need_spill_flag = NEED_SPILL_YMM(dst_reg) ? 4 : 0;
    const uint need_spill_flag = src1_need_spill_flag | src2_need_spill_flag | dst_need_spill_flag;

    switch (need_spill_flag) {
    case 0: { // No spill
        opnd_t op_src1 = opnd_create_reg(src1_reg);
        opnd_t op_src2 = opnd_create_reg(src2_reg);
        opnd_t op_dst = opnd_create_reg(dst_reg);
        return INSTR_CREATE_your_binary_op(dcontext, op_dst, op_src1, op_src2);
    }
    case 3: { // Both sources need spill
        reg_id_t spill_src1_reg = find_available_spill_ymm_avoiding(src2_reg, dst_reg, DR_REG_NULL);
        reg_id_t spill_src2_reg = find_available_spill_ymm_avoiding(src1_reg, dst_reg, spill_src1_reg);
        
        // Create instruction sequence with proper spill/restore
        instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src1_reg, 
                                             TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src1_reg)), OPSZ_32);
        instr_t *i2 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src2_reg,
                                             TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src2_reg)), OPSZ_32);
        instr_t *i3 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,
                                                  TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(src1_reg)), OPSZ_32);
        instr_t *i4 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,
                                                  TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(src2_reg)), OPSZ_32);
        instr_t *i5 = INSTR_CREATE_your_binary_op(dcontext, 
                                                  opnd_create_reg(dst_reg),
                                                  opnd_create_reg(spill_src1_reg),
                                                  opnd_create_reg(spill_src2_reg));
        instr_t *i6 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,
                                                  TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src1_reg)), OPSZ_32);
        instr_t *i7 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,
                                                  TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src2_reg)), OPSZ_32);
        
        instrlist_concat_next_instr(ilist, 7, i1, i2, i3, i4, i5, i6, i7);
        return i1;
    }
    // ... handle other spill combinations
    }
}
```

### Pattern 3: Memory Operations

For instructions that involve memory operands:

```c
instr_t *
mem_operation_gen(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr,
                  opnd_t mem_opnd, reg_id_t reg, reg_id_t mask_reg)
{
    instrlist_remove(ilist, instr);
    instr_destroy(dcontext, instr);

    if (!NEED_SPILL_YMM(reg)) {
        // Simple case - no spill needed
        opnd_t op_reg = opnd_create_reg(reg);
        return INSTR_CREATE_your_mem_op(dcontext, op_reg, mem_opnd);
    } else {
        // Need to spill the register
        reg_id_t spill_reg = find_one_available_spill_ymm(DR_REG_NULL);
        
        instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_reg,
                                             TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_reg)), OPSZ_32);
        instr_t *i2 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_reg,
                                                  TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(reg)), OPSZ_32);
        instr_t *i3 = INSTR_CREATE_your_mem_op(dcontext, opnd_create_reg(spill_reg), mem_opnd);
        instr_t *i4 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_reg,
                                             TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(reg)), OPSZ_32);
        instr_t *i5 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_reg,
                                                  TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_reg)), OPSZ_32);
        
        instrlist_concat_next_instr(ilist, 5, i1, i2, i3, i4, i5);
        return i1;
    }
}
```

### Instruction Chaining

When creating multiple instructions, use `instrlist_concat_next_instr()`:

```c
// Chain multiple instructions together
instrlist_concat_next_instr(ilist, 4, instr1, instr2, instr3, instr4);
```

## Testing and Validation

### Unit Tests

Create unit tests in the `unittests/` directory, one unittests should contain 3 implementation: C emulation, assembly emulation, AVX-512 origin implementation to cross-validate.

### Debug Output

Use debug macros to trace rewrite operations:

```c
#ifdef DEBUG
    print_rewrite_info(dcontext, ilist, instr, instr_start, "your_instruction", 
                       true, true, false, true);
    print_rewrite_variadic_instr(dcontext, 1, new_instr);
#endif
```

### Validation Steps

1. **Functional Testing**: Ensure the rewritten instruction produces the same results as the original
2. **Performance Testing**: Verify that the rewrite doesn't introduce excessive overhead  
3. **Spill Testing**: Test all spill scenarios with high registers (16-31)
4. **Memory Testing**: Verify memory operand handling works correctly
5. **Mask Testing**: Ensure mask register operations are handled properly

## Best Practices

### Code Organization

1. **Consistent Naming**: Use the pattern `rw_func_<instruction_name>`
2. **Helper Functions**: Create separate generator functions for different operand combinations
3. **Template Usage**: Use provided templates for common patterns when possible
4. **Documentation**: Include comprehensive comments explaining complex logic

### Performance Considerations

1. **Minimize Spills**: Only spill registers when absolutely necessary
2. **Optimal Register Selection**: Choose spill registers that minimize conflicts
3. **Instruction Scheduling**: Order instructions to minimize pipeline stalls
4. **Template Reuse**: Leverage existing templates to reduce code duplication

### Error Handling

1. **Graceful Degradation**: Handle unsupported operand combinations gracefully
2. **Debug Information**: Provide helpful debug output for troubleshooting
3. **Validation**: Validate operands and register assignments before use
4. **Fallback Options**: Consider providing fallback implementations

### Memory Management

1. **Instruction Cleanup**: Always destroy original instructions after removal
2. **Resource Tracking**: Ensure proper cleanup of allocated resources
3. **TLS Usage**: Use TLS slots appropriately and avoid conflicts
4. **Register Restoration**: Always restore spilled registers to maintain state consistency

### Maintainability

1. **Modular Design**: Break complex rewrites into smaller, manageable functions
2. **Clear Interfaces**: Use consistent function signatures and parameter ordering
3. **Comprehensive Testing**: Cover all operand combinations and edge cases
4. **Version Compatibility**: Ensure compatibility with different processor feature sets

---

