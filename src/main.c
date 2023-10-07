/*
 * MIT License
 * 
 * Copyright (c) 2023 Can Joshua Lehmann
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

void clock();
void unreachable();
void __debug_value(void*);

// Exceptions
// TODO

typedef enum {
  E_BREAK, E_MEM, E_DECODE
} exception_t;

void exception(exception_t id) {
  // TODO
  #ifdef SIM
  printf("exception %d\n", id);
  #endif
  while(1) { clock(); }
}

// Memory

typedef struct {
  uint32_t size;
  void* data;
  uint32_t (*read32)(uint32_t addr, void* data);
  void (*write32)(uint32_t addr, uint32_t value, void* data);
} mem_interface_t;

#ifdef HLS

void __output_mem_addr(uint32_t);
uint32_t __input_mem_read();
void __output_mem_write(uint32_t);
void __output_mem_write_enable(bool);

uint32_t hls_memread32(uint32_t addr, void* data) {
  __output_mem_addr(addr >> 2);
  clock();
  return __input_mem_read();
}

void hls_memwrite32(uint32_t addr, uint32_t value, void* data) {
  __output_mem_addr(addr >> 2);
  __output_mem_write(value);
  __output_mem_write_enable(true);
  clock();
  __output_mem_write_enable(false);
}

mem_interface_t init_external_mem_interface(uint32_t size) {
  return (mem_interface_t) {
    .size = size,
    .data = NULL,
    .read32 = hls_memread32,
    .write32 = hls_memwrite32
  };
}

#endif

uint32_t memread32(mem_interface_t* mem, uint32_t addr) {
  if (addr & 3) { exception(E_MEM); }
  return mem->read32(addr, mem->data);
}

void memwrite32(mem_interface_t* mem, uint32_t addr, uint32_t value) {
  if (addr & 3) { exception(E_MEM); }
  mem->write32(addr, value, mem->data);
}

uint8_t memread8(mem_interface_t* mem, uint32_t addr) {
  uint32_t val = memread32(mem, addr & ~3);
  return val >> (8 * (addr & 3));
}

uint16_t memread16(mem_interface_t* mem, uint32_t addr) {
  if (addr & 1) { exception(E_MEM); }
  uint32_t val = memread32(mem, addr & ~3);
  return val >> (8 * (addr & 2));
}

void memwrite8(mem_interface_t* mem, uint32_t addr, uint8_t value) {
  uint32_t word = memread32(mem, addr & ~3);
  uint32_t offset = (8 * (addr & 3));
  word &= ~(0xff << offset);
  word |= (uint32_t)value << offset;
  memwrite32(mem, addr & ~3, word);
}

void memwrite16(mem_interface_t* mem, uint32_t addr, uint16_t value) {
  uint32_t word = memread32(mem, addr & ~3);
  uint32_t offset = (8 * (addr & 2));
  word &= ~(0xffff << offset);
  word |= (uint32_t)value << offset;
  memwrite32(mem, addr & ~3, word);
}

// Processor

#include "decode.c"

#define REG_SP 2
#define REG_A0 10
#define REG_A1 11

void riscv(mem_interface_t* mem, void (*ecall)(uint32_t*)) {
  uint32_t pc = 0;
  uint32_t regs[32];
  regs[REG_SP] = mem->size;
  while (1) {
    inst_t inst;
    // TODO: This should not be required
    inst.opcode = 0;
    inst.rs1 = 0;
    inst.rs2 = 0;
    inst.rd = 0;
    inst.imm = 0;
  
    if (decode(memread32(mem, pc), &inst)) {
      exception(E_DECODE);
    }
    #ifdef SIM_DEBUG
    printf("%d\t%s\n", pc, OPCODE_NAMES[inst.opcode]);
    #endif
    clock();
    regs[0] = 0;
    switch (inst.opcode) {
      // RV_I
      case ADD: regs[inst.rd] = regs[inst.rs1] + regs[inst.rs2]; break;
      case ADDI: regs[inst.rd] = regs[inst.rs1] + inst.imm; break;
      case AND: regs[inst.rd] = regs[inst.rs1] & regs[inst.rs2]; break;
      case ANDI: regs[inst.rd] = regs[inst.rs1] & inst.imm; break;
      case AUIPC: regs[inst.rd] = pc + inst.imm; break;
      case BEQ: if (regs[inst.rs1] == regs[inst.rs2]) { pc += inst.imm - 4; } break;
      case BGE: if ((int32_t)regs[inst.rs1] >= (int32_t)regs[inst.rs2]) { pc += inst.imm - 4; } break;
      case BGEU: if (regs[inst.rs1] >= regs[inst.rs2]) { pc += inst.imm - 4; } break;
      case BLT: if ((int32_t)regs[inst.rs1] < (int32_t)regs[inst.rs2]) { pc += inst.imm - 4; } break;
      case BLTU: if (regs[inst.rs1] < regs[inst.rs2]) { pc += inst.imm - 4; } break;
      case BNE: if (regs[inst.rs1] != regs[inst.rs2]) { pc += inst.imm - 4; } break;
      case EBREAK: exception(E_BREAK); break;
      case ECALL: ecall(regs); break;
      case FENCE: break;
      case JAL:
        regs[inst.rd] = pc + 4;
        pc += inst.imm - 4;
      break;
      case JALR: {
        uint32_t old_pc = pc;
        pc = ((regs[inst.rs1] + inst.imm) & ~1) - 4;
        regs[inst.rd] = old_pc + 4;
      }
      break;
      case LB: regs[inst.rd] = (int32_t)(int8_t)memread8(mem, regs[inst.rs1] + inst.imm); break;
      case LBU: regs[inst.rd] = memread8(mem, regs[inst.rs1] + inst.imm); break;
      case LH: regs[inst.rd] = (int32_t)(int8_t)memread16(mem, regs[inst.rs1] + inst.imm); break;
      case LHU: regs[inst.rd] = memread16(mem, regs[inst.rs1] + inst.imm); break;
      case LUI: regs[inst.rd] = inst.imm; break;
      case LW: regs[inst.rd] = memread32(mem, regs[inst.rs1] + inst.imm); break;
      case OR: regs[inst.rd] = regs[inst.rs1] | regs[inst.rs2]; break;
      case ORI: regs[inst.rd] = regs[inst.rs1] | inst.imm; break;
      case SB: memwrite8(mem, regs[inst.rs1] + inst.imm, regs[inst.rs2]); break;
      case SH: memwrite16(mem, regs[inst.rs1] + inst.imm, regs[inst.rs2]); break;
      case SLL: regs[inst.rd] = regs[inst.rs1] << (regs[inst.rs2] & 31); break;
      case SLT: regs[inst.rd] = (int32_t)regs[inst.rs1] < (int32_t)regs[inst.rs2] ? 1 : 0; break;
      case SLTI: regs[inst.rd] = (int32_t)regs[inst.rs1] < (int32_t)inst.imm ? 1 : 0; break;
      case SLTIU: regs[inst.rd] = regs[inst.rs1] < inst.imm ? 1 : 0; break;
      case SLTU: regs[inst.rd] = regs[inst.rs1] < regs[inst.rs2] ? 1 : 0; break;
      case SRA: regs[inst.rd] = (int32_t)regs[inst.rs1] >> (int32_t)(regs[inst.rs2] & 31); break;
      case SRL: regs[inst.rd] = regs[inst.rs1] >> (regs[inst.rs2] & 31); break;
      case SUB: regs[inst.rd] = regs[inst.rs1] - regs[inst.rs2]; break;
      case SW: memwrite32(mem, regs[inst.rs1] + inst.imm, regs[inst.rs2]); break;
      case XOR: regs[inst.rd] = regs[inst.rs1] ^ regs[inst.rs2]; break;
      case XORI: regs[inst.rd] = regs[inst.rs1] ^ inst.imm; break;
    }
    pc += 4;
  }
}

#ifdef HLS

void hls_ecall(uint32_t* regs) {
  
}

void top() {
  mem_interface_t mem = init_external_mem_interface(1 << 16);
  riscv(&mem, hls_ecall);
}
#endif

#ifdef SIM

void clock() {}

typedef struct {
  uint8_t* data;
  size_t size;
} mem_t;

void init_mem(mem_t* mem, size_t size) {
  mem->data = malloc(size);
  mem->size = size;
}

int load_mem(mem_t* mem, const char* path) {
  FILE* file = fopen(path, "r");
  if (file == NULL) {
    return 1;
  }
  
  fseek(file, 0, SEEK_END);
  size_t left = ftell(file);
  if (left > mem->size) {
    return 1;
  }
  fseek(file, 0, SEEK_SET);
  
  uint8_t* cur = mem->data;
  size_t read = 0;
  while ((read = fread(cur, 1, left, file)) > 0) {
    cur += read;
    left -= read;
  }
  
  fclose(file);
  
  return 0;
}

uint32_t sim_memread32(uint32_t addr, void* data) {
  mem_t* mem = data;
  if (addr >= mem->size) {
    abort();
  }
  return *(uint32_t*)(mem->data + addr);
}

void sim_memwrite32(uint32_t addr, uint32_t value, void* data) {
  mem_t* mem = data;
  if (addr >= mem->size) {
    abort();
  }
  *(uint32_t*)(mem->data + addr) = value;
}

void sim_ecall(uint32_t* regs) {
  #ifdef SIM_DEBUG
  printf("Environment Call %d\n", regs[REG_A0]);
  #endif
  switch (regs[REG_A0]) {
    case 0: putc(regs[REG_A1], stdout); break;
    case 1: printf("%d\n", regs[REG_A1]); break;
    default: break;
  }
}

int main(int argc, const char** argv) {
  if (argc != 2) {
    return 1;
  }
  
  mem_t mem;
  init_mem(&mem, 1 << 16);
  if (load_mem(&mem, argv[1])) {
    return 1;
  }
  
  mem_interface_t mem_interface = {
    .size = mem.size,
    .data = &mem,
    .read32 = sim_memread32,
    .write32 = sim_memwrite32
  };
  
  riscv(&mem_interface, sim_ecall);
  
  return 0;
}
#endif
