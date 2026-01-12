#!/usr/bin/env python3

import re
import os

def extract_supported_instructions(rewrite_c_path):
    # Read the rewrite.c file
    with open(rewrite_c_path, 'r') as f:
        content = f.read()
    
    # Find the rewrite_funcs array
    pattern = r"instr_rewrite_func_t \*rewrite_funcs\[\] = \{([^}]+)\};"
    match = re.search(pattern, content, re.DOTALL)
    if not match:
        print("Could not find rewrite_funcs array in the file")
        return []
    
    # Extract all lines in the array
    array_content = match.group(1)
    lines = array_content.strip().split('\n')
    
    supported_instructions = []
    
    # Process each line to extract supported instructions
    for line in lines:
        # Skip empty lines
        if not line.strip():
            continue
        
        # Extract the instruction and function name
        instr_match = re.search(r"/\* \d+ (OP_AVX512_\w+) \*/ (rw_func_\w+),", line)
        if instr_match:
            instr_name = instr_match.group(1)
            func_name = instr_match.group(2)
            
            # Add to supported list if not using empty function
            if func_name != "rw_func_empty":
                supported_instructions.append(instr_name)
    
    return supported_instructions

def generate_coverage_md(supported_instructions):
    total_supported = len(supported_instructions)
    
    # Generate markdown content
    content = f"# AVX512 Instruction Coverage\n\n"
    content += f"Currently supported: **{total_supported}** instructions\n\n"
    content += "## Supported Instructions\n\n"
    
    # Add each instruction
    for instr in sorted(supported_instructions):
        content += f"- {instr}\n"
    
    return content

def main():
    # Define paths
    script_dir = os.path.dirname(os.path.abspath(__file__))
    rewrite_c_path = os.path.join(script_dir, "core", "arch", "rewrite.c")
    coverage_md_path = os.path.join(script_dir, "docs", "coverage.md")
    
    # Extract supported instructions
    supported_instructions = extract_supported_instructions(rewrite_c_path)
    
    # Generate markdown content
    md_content = generate_coverage_md(supported_instructions)
    
    # Write to coverage.md
    with open(coverage_md_path, 'w') as f:
        f.write(md_content)
    
    print(f"Generated coverage.md with {len(supported_instructions)} supported instructions")

if __name__ == "__main__":
    main()


