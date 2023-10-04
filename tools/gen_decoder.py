# MIT License
# 
# Copyright (c) 2023 Can Joshua Lehmann
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Script for generating the instruction decoder

from dataclasses import dataclass
import os, os.path


def read_file(path):
    with open(path, "r") as f:
        return f.read()

def write_file(path, content):
    with open(path, "w") as f:
        f.write(content)

@dataclass
class Inst:
    name: str
    mask: int
    pattern: int
    args: list[str]
    
    def load(path):
        insts = []
        for line in read_file(path).split("\n"):
            if len(line) == 0 or line.startswith("#") or line.startswith("$"):
                continue
            
            parts = [part for part in line.split(" ") if len(part) > 0]
            
            name = parts[0]
            pattern = 0
            mask = 0
            args = []
            for part in parts[1:]:
                if "=" in part:
                    lhs, rhs = part.split("=")
                    
                    if ".." in lhs:
                        msb, lsb = lhs.split("..")
                        offset = int(lsb)
                        size = int(msb) - offset + 1
                    else:
                        offset = int(lhs)
                        size = 1
                    
                    local_mask = 2 ** size - 1
                    mask |= local_mask << offset
                    
                    if rhs.startswith("0x"):
                        val = int(rhs[2:], 16)
                    elif rhs.startswith("0b"):
                        val = int(rhs[2:], 2)
                    else:
                        val = int(rhs)
                    
                    assert (val & local_mask) == val
                    pattern |= val << offset
                else:
                    args.append(part)
            
            insts.append(Inst(
                name=name,
                mask=mask,
                pattern=pattern,
                args=args
            ))
        return insts
    
    def load_dir(path, prefix="rv"):
        insts = []
        for base, dirs, files in os.walk(path):
            for file_name in files:
                if file_name.startswith(prefix):
                    insts += Inst.load(os.path.join(base, file_name))
        return insts

def gen_case(inst):
    fields = {"opcode": inst.name.upper()}
    for arg in inst.args:
        match arg:
            case "imm20": fields["imm"] = "decode_imm20(inst)"
            case "rd": fields["rd"] = "(inst >> 7) & 31"
            case "rs1": fields["rs1"] = "(inst >> 15) & 31"
            case "rs2": fields["rs2"] = "(inst >> 20) & 31"
            case "jimm20": fields["imm"] = "decode_jimm20(inst)"
            case "imm12": fields["imm"] = "decode_imm12(inst)"
            case "bimm12hi":
                assert "bimm12lo" in inst.args
                fields["imm"] = "decode_bimm12_lo_hi(inst)"
            case "imm12hi":
                assert "imm12lo" in inst.args
                fields["imm"] = "decode_imm12_lo_hi(inst)"
            case "bimm12lo": assert "bimm12hi" in inst.args
            case "imm12lo": assert "imm12hi" in inst.args
            case "fm" | "pred" | "succ": pass
            case _:
                raise Exception(f"Unknown instruction argument: {arg}")
    
    fields = ", ".join([f".{name} = {value}" for name, value in fields.items()])
    return f"CASE({inst.mask}, {inst.pattern}, ((inst_t){{ {fields} }}));"

insts = Inst.load_dir("tools/opcodes")
template = read_file("tools/decode.tmpl.c")

opcode_names = [inst.name.upper() for inst in insts]
opcode_names.sort()
opcode_cases = [gen_case(inst) for inst in insts]

code = template \
    .replace("<<opcode>>", ", ".join(opcode_names)) \
    .replace("<<names>>", ", ".join([f"\"{name}\"" for name in opcode_names])) \
    .replace("<<decode>>", "\n  ".join(opcode_cases))

write_file("src/decode.c", code)
