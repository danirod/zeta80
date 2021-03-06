/*
 * This file is part of the zeta80 emulation library.
 * Copyright (c) 2015, Dani Rodríguez
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * * Neither the name of the project's author nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 */

#include <stdio.h>
#include <opcodes.h>
#include <cpu.h>

byte* r(struct cpu_t* cpu, unsigned int index)
{
    switch (index)
    {
        case 0: return &REG_B(*cpu);
        case 1: return &REG_C(*cpu);
        case 2: return &REG_D(*cpu);
        case 3: return &REG_E(*cpu);
        case 4: return &REG_H(*cpu);
        case 5: return &REG_L(*cpu);
        case 6: return &cpu->mem[REG_HL(*cpu)];
        case 7: return &REG_A(*cpu);
        default: return NULL;
    }
}

/**
 * Executes NOP. This opcode does nothing. It just refreshes memory.
 *
 * @param cpu CPU instances
 */
static void
nop(struct cpu_t* cpu)
{
    cpu->tstates += 4;
}

static void
ex_af_af(struct cpu_t* cpu)
{
    word tmp = REG_AF(*cpu);
    REG_AF(*cpu) = ALT_AF(*cpu);
    ALT_AF(*cpu) = tmp;
    cpu->tstates += 4;
}

static void
djnz_d(struct cpu_t* cpu)
{
    char e = (char) cpu->mem[PC(*cpu)++];

    if (--REG_B(*cpu) == 0) {
        cpu->tstates += 8;
    } else {
        PC(*cpu) += e;
        cpu->tstates = 13;
    }
}

static void
jr_d(struct cpu_t* cpu)
{
    char e = (char) cpu->mem[PC(*cpu)++];
    PC(*cpu) += e;
    cpu->tstates = 12;
}

static void
jr_nz(struct cpu_t* cpu)
{
    char e = (char) cpu->mem[PC(*cpu)++];
    if (GET_FLAG(REG_F(*cpu), FLAG_Z) == 0) {
        PC(*cpu) += e;
        cpu->tstates += 12;
    } else {
        cpu->tstates += 7;
    }
}

static void
jr_z(struct cpu_t* cpu)
{
    char e = (char) cpu->mem[PC(*cpu)++];
    if (GET_FLAG(REG_F(*cpu), FLAG_Z) != 0) {
        cpu->pc.WORD += e;
        cpu->tstates += 12;
    } else {
        cpu->tstates += 7;
    }
}

static void
jr_nc(struct cpu_t* cpu)
{
    char e = (char) cpu->mem[PC(*cpu)++];
    if (GET_FLAG(REG_F(*cpu), FLAG_C) == 0) {
        cpu->pc.WORD += e;
        cpu->tstates += 12;
    } else {
        cpu->tstates += 7;
    }
}

static void
jr_c(struct cpu_t* cpu)
{
    char e = (char) cpu->mem[PC(*cpu)++];
    if(GET_FLAG(REG_F(*cpu), FLAG_C) != 0) {
        cpu->pc.WORD += e;
        cpu->tstates += 12;
    } else {
        cpu->tstates += 7;
    }
}

static void
ld_dd_nn(struct cpu_t* cpu, union register_t* reg)
{
    // Read NN in memory. Remember: Z80 is little endian.
    word nn = cpu->mem[PC(*cpu)] | (cpu->mem[PC(*cpu) + 1] << 8);
    PC(*cpu) += 2; // Increment program counter after read.
    reg->WORD = nn;
    cpu->tstates += 10;
}

static void
add_hl_ss(struct cpu_t* cpu, union register_t* reg)
{
    word op1 = REG_HL(*cpu), op2 = reg->WORD;

    RESET_FLAG(REG_F(*cpu), FLAG_N);
    SET_IF(REG_F(*cpu), FLAG_H, ((op1 & 0xFFF) + (op2 & 0xFFF)) & 0x1000);
    SET_IF(REG_F(*cpu), FLAG_C, (op1 + op2) & 0x10000);

    REG_HL(*cpu) += reg->WORD;
    cpu->tstates += 11;
}

// [BC] <- A
static void
ld_bci_a(struct cpu_t* cpu)
{
    cpu->mem[REG_BC(*cpu)] = REG_A(*cpu);
    cpu->tstates += 7;
}

// [DE] <- A
static void
ld_dei_a(struct cpu_t* cpu)
{
    cpu->mem[REG_DE(*cpu)] = REG_A(*cpu);
    cpu->tstates += 7;
}

// [NN] <- A
static void
ld_nni_a(struct cpu_t* cpu)
{
    word addr = cpu->mem[PC(*cpu)] | (cpu->mem[PC(*cpu) + 1] << 8);
    PC(*cpu) += 2;
    cpu->mem[addr] = REG_A(*cpu);
    cpu->tstates += 13;
}

// [NN] <- HL: [NN] <- L, [NN+1] <- H
static void
ld_nni_hl(struct cpu_t* cpu)
{
    word addr = cpu->mem[PC(*cpu)] | (cpu->mem[PC(*cpu) + 1] << 8);
    PC(*cpu) += 2;
    cpu->mem[addr] = REG_L(*cpu);
    cpu->mem[addr + 1] = REG_H(*cpu);
    cpu->tstates += 16;
}

// A <- [BC]
static void
ld_a_bci(struct cpu_t* cpu)
{
    REG_A(*cpu) = cpu->mem[REG_BC(*cpu)];
    cpu->tstates += 7;
}

// A <- [DE]
static void
ld_a_dei(struct cpu_t* cpu)
{
    REG_A(*cpu) = cpu->mem[REG_DE(*cpu)];
    cpu->tstates += 7;
}

// A <- [NN]
static void
ld_a_nni(struct cpu_t* cpu)
{
    word addr = cpu->mem[PC(*cpu)] | (cpu->mem[PC(*cpu)+1] << 8);
    PC(*cpu) += 2;
    REG_A(*cpu) = cpu->mem[addr];
    cpu->tstates += 13;
}

// HL <- [NN]
static void
ld_hl_nni(struct cpu_t* cpu)
{
    word addr = cpu->mem[PC(*cpu)] | (cpu->mem[PC(*cpu)+1] << 8);
    PC(*cpu) += 2;
    REG_L(*cpu) = cpu->mem[addr];
    REG_H(*cpu) = cpu->mem[addr + 1];
    cpu->tstates = 16;
}

static void
inc_r16(struct cpu_t* cpu, union register_t* reg)
{
    reg->WORD++;
    cpu->tstates += 6;
}

static void
dec_r16(struct cpu_t* cpu, union register_t* reg)
{
    reg->WORD--;
    cpu->tstates += 6;
}

static void
inc_r8(struct cpu_t* cpu, int index)
{
    byte* val = r(cpu, index);
    FLAG_SIF(*cpu, FLAG_H, (*val & 0xF));
    FLAG_SIF(*cpu, FLAG_P, (*val == 0x7F));

    (*val)++;

    FLAG_SIF(*cpu, FLAG_S, (*val & 0x80));
    FLAG_SIF(*cpu, FLAG_Z, (*val == 0));
    FLAG_RST(*cpu, FLAG_N);

    cpu->tstates += (index == 6 ? 11 : 4);
}

static void
dec_r8(struct cpu_t* cpu, int index)
{
    byte* val = r(cpu, index);
    FLAG_SIF(*cpu, FLAG_P, (*val == 0x80));

    (*val)--;

    FLAG_SIF(*cpu, FLAG_S, (*val & 0x80));
    FLAG_SIF(*cpu, FLAG_Z, (*val == 0));
    FLAG_SIF(*cpu, FLAG_H, (*val & 0xF) == 0xF);
    FLAG_SET(*cpu, FLAG_N);

    cpu->tstates += (index == 6 ? 11 : 4);
}

static void
ld_r_n(struct cpu_t* cpu, int index)
{
    byte n = cpu->mem[PC(*cpu)++];
    byte* pos = r(cpu, index);
    *pos = n;
    cpu->tstates += 7;
}

static void
rlca(struct cpu_t* cpu)
{
    byte bit7 = (REG_A(*cpu) & 0x80) >> 7;
    REG_A(*cpu) <<= 1;
    REG_A(*cpu) &= 0xFE;
    REG_A(*cpu) |= bit7;
    SET_IF(REG_F(*cpu), FLAG_C, (bit7 != 0));
    RESET_FLAG(REG_F(*cpu), FLAG_H);
    RESET_FLAG(REG_F(*cpu), FLAG_N);
    cpu->tstates += 4;
}

static void
rrca(struct cpu_t* cpu)
{
    byte bit0 = REG_A(*cpu) & 1;
    REG_A(*cpu) = ((REG_A(*cpu) >> 1) & 0x7F) | (bit0 << 7);
    SET_IF(REG_F(*cpu), FLAG_C, bit0);
    RESET_FLAG(REG_F(*cpu), FLAG_H);
    RESET_FLAG(REG_F(*cpu), FLAG_N);
    cpu->tstates += 4;
}

static void
rla(struct cpu_t* cpu)
{
    byte bit7 = (REG_A(*cpu) & 0x80) >> 7;
    byte cf = GET_FLAG(REG_F(*cpu), FLAG_C) ? 1 : 0;
    REG_A(*cpu) <<= 1;
    REG_A(*cpu) &= 0xFE;
    REG_A(*cpu) |= cf;
    SET_IF(REG_F(*cpu), FLAG_C, (bit7 != 0));
    RESET_FLAG(REG_F(*cpu), FLAG_H);
    RESET_FLAG(REG_F(*cpu), FLAG_N);
    cpu->tstates += 4;
}

static void
rra(struct cpu_t* cpu)
{
    byte bit0 = REG_A(*cpu) & 1;
    byte cf = GET_FLAG(REG_F(*cpu), FLAG_C) ? 1 : 0;
    REG_A(*cpu) >>= 1;
    REG_A(*cpu) &= 0x7F;
    REG_A(*cpu) |= (cf << 7);
    SET_IF(REG_F(*cpu), FLAG_C, bit0 != 0);
    RESET_FLAG(REG_F(*cpu), FLAG_H);
    RESET_FLAG(REG_F(*cpu), FLAG_N);
    cpu->tstates += 4;
}

static void
cpl(struct cpu_t* cpu)
{
    REG_A(*cpu) = ~REG_A(*cpu);
    SET_FLAG(REG_F(*cpu), FLAG_H);
    SET_FLAG(REG_F(*cpu), FLAG_N);
    cpu->tstates += 4;
}

static void
scf(struct cpu_t* cpu)
{
    SET_FLAG(REG_F(*cpu), FLAG_C);
    RESET_FLAG(REG_F(*cpu), FLAG_H);
    RESET_FLAG(REG_F(*cpu), FLAG_N);
    cpu->tstates += 4;
}

static void
ccf(struct cpu_t* cpu)
{
    SET_IF(REG_F(*cpu), FLAG_H, GET_FLAG(REG_F(*cpu), FLAG_C) != 0);
    SET_IF(REG_F(*cpu), FLAG_C, GET_FLAG(REG_F(*cpu), FLAG_C) == 0);
    RESET_FLAG(REG_F(*cpu), FLAG_N);
    cpu->tstates += 4;
}

// x = 1, y != 6 && z != 6 -> LD r[y], r[z]
static void ld_ry_rz(struct cpu_t* cpu, int y, int z) {
    byte* regZ = r(cpu, z);
    byte* regY = r(cpu, y);
    *regY = *regZ;
    if (y == 6 || z == 6) {
        cpu->tstates += 7;
    } else {
        cpu->tstates += 4;
    }
}

/**
 * Extrae los trozos de un opcode a partir del opcode tal cual que se
 * haya sacado de memoria. Aplica una serie de máscaras de bit para sacar
 * el resultado. Ver documentación.
 *
 * Sin probar.
 *
 * @param opcode opcode leído de memoria
 * @param opstruct la estructura en la que quiero volcar los datos
 */
void
extract_opcode(char opcode, struct opcode_t* opstruct)
{
    opstruct->x = (opcode & 0xC0) >> 6;
    opstruct->y = (opcode & 0x38) >> 3;
    opstruct->z = (opcode & 0x07);
    opstruct->p = opstruct->y >> 1;
    opstruct->q = opstruct->y & 1;
}

/**
 * Este es el tipo de datos para la función que va a procesar la tabla
 * de opcodes. Construiré con ellas un array de callbacks para que pueda
 * llamar rápidamente a la función apropiada.
 */
typedef void (*table_function)(struct cpu_t*, struct opcode_t*);

static union register_t* rp[4];

static void
execute_table0(struct cpu_t* cpu, struct opcode_t* opstruct)
{
    if (opstruct->z == 0)
    {
        if (opstruct->y == 0) nop(cpu);
        if (opstruct->y == 1) ex_af_af(cpu);
        if (opstruct->y == 2) djnz_d(cpu);
        if (opstruct->y == 3) jr_d(cpu);
        if (opstruct->y == 4) jr_nz(cpu);
        if (opstruct->y == 5) jr_z(cpu);
        if (opstruct->y == 6) jr_nc(cpu);
        if (opstruct->y == 7) jr_c(cpu);
    }
    else if (opstruct->z == 1)
    {
        if (opstruct->q == 0) ld_dd_nn(cpu, rp[(int) opstruct->p]);
        if (opstruct->q == 1) add_hl_ss(cpu, rp[(int) opstruct->p]);
    }
    else if (opstruct->z == 2)
    {
        if (opstruct->q == 0) {
            if (opstruct->p == 0) ld_bci_a(cpu);
            if (opstruct->p == 1) ld_dei_a(cpu);
            if (opstruct->p == 2) ld_nni_hl(cpu);
            if (opstruct->p == 3) ld_nni_a(cpu);
        }
        else
        {
            if (opstruct->p == 0) ld_a_bci(cpu);
            if (opstruct->p == 1) ld_a_dei(cpu);
            if (opstruct->p == 2) ld_hl_nni(cpu);
            if (opstruct->p == 3) ld_a_nni(cpu);
        }
    }
    else if (opstruct->z == 3)
    {
        if (opstruct->q == 0) {
            inc_r16(cpu, rp[opstruct->p]);
        } else {
            dec_r16(cpu, rp[opstruct->p]);
        }
    }
    else if (opstruct->z == 4)
    {
        inc_r8(cpu, opstruct->y);
    }
    else if (opstruct->z == 5)
    {
        dec_r8(cpu, opstruct->y);
    }
    else if (opstruct->z == 6)
    {
        ld_r_n(cpu, opstruct->y);
    }
    else
    {
        if (opstruct->y == 0) rlca(cpu);
        if (opstruct->y == 1) rrca(cpu);
        if (opstruct->y == 2) rla(cpu);
        if (opstruct->y == 3) rra(cpu);
        if (opstruct->y == 5) cpl(cpu);
        if (opstruct->y == 6) scf(cpu);
        if (opstruct->y == 7) ccf(cpu);
    }
}

static void
add_a(struct cpu_t* cpu, int z)
{
    byte* zz = r(cpu, z);
    byte old_a = REG_A(*cpu);
    char same_sign = ((old_a ^ *zz) & 0x80) == 0;

    FLAG_SIF(*cpu, FLAG_H, ((old_a & 0xF) + (*zz & 0xF)) & 0x10);
    FLAG_SIF(*cpu, FLAG_C, (old_a + *zz) & 0x100);

    REG_A(*cpu) += *zz;

    /*
     * H: Set if A last nibble + zz last nibble overflows.
     * C: Set if A + zz overflows.
     * Z: Set if A + zz == 0
     * S: Set if A + zz < 0 (bit 7 ON)
     * V: Set if both operands positive and result negative; or
     *        if both operands negative and result positive
     * N: Always unset.
     */
    FLAG_SIF(*cpu, FLAG_S, (REG_A(*cpu) & 0x80) != 0);
    FLAG_SIF(*cpu, FLAG_Z, REG_A(*cpu) == 0);
    FLAG_RST(*cpu, FLAG_N);
    FLAG_SIF(*cpu, FLAG_P, same_sign && ((REG_A(*cpu) ^ old_a) & 0x80));

    cpu->tstates += (z == 6 ? 7 : 4);
}

static void
adc_a(struct cpu_t* cpu, int z)
{
    byte* zz = r(cpu, z);
    byte old_a = REG_A(*cpu);
    char carry = FLAG_GET(*cpu, FLAG_C) != 0;
    char same_sign = ((REG_A(*cpu) ^ (*zz + carry)) & 0x80) == 0;

    FLAG_SIF(*cpu, FLAG_H, (((old_a & 0xF) + (*zz & 0xF) + carry) & 0xF0));
    FLAG_SIF(*cpu, FLAG_C, (old_a + (*zz + carry)) & 0x100);

    REG_A(*cpu) += *zz + carry;

    /*
     * H: Set if A last nibble + zz + last nibble + CF overflows.
     * C: Set if A + zz + CF overflows.
     * Z: Set if A + zz + CF == 0
     * S: Set if A + zz + CF < 0 (bit 7 ON)
     * V: Set if both operands + CF positive and result negative;
     *     or if both operands + CF negative and result positive
     * N: Always unset.
     */
    FLAG_SIF(*cpu, FLAG_S, (REG_A(*cpu) & 0x80) != 0);
    FLAG_SIF(*cpu, FLAG_Z, REG_A(*cpu) == 0);
    FLAG_RST(*cpu, FLAG_N);
    FLAG_SIF(*cpu, FLAG_P, same_sign && ((REG_A(*cpu) ^ old_a) & 0x80));

    cpu->tstates += (z == 6 ? 7 : 4);
}

static void
sub_a(struct cpu_t* cpu, int z)
{
    /*
     * H: Set if borrow from bit 4.
     * C: Set if borrow:
     * Z: Set if A - zz == 0
     * S: Set if A - zz < 0 (bit 7 ON)
     * V: Set if
     *     or if
     * N: Always set.
     */
    byte* zz = r(cpu, z);
    byte old_a = REG_A(*cpu);
    byte old_b = *zz;
    char same_sign = ((old_a ^ *zz) & 0x80) == 0;

    FLAG_SIF(*cpu, FLAG_H, (((old_a & 0xF) - (old_b & 0xF)) & 0xF00) != 0);
    FLAG_SIF(*cpu, FLAG_C, old_a < old_b);

    REG_A(*cpu) -= *zz;

    FLAG_SIF(*cpu, FLAG_S, (REG_A(*cpu) & 0x80) != 0);
    FLAG_SIF(*cpu, FLAG_Z, REG_A(*cpu) == 0);
    FLAG_SET(*cpu, FLAG_N);
    FLAG_SIF(*cpu, FLAG_P, !same_sign && (((REG_A(*cpu) ^ old_b) & 0x80) == 0));

    cpu->tstates += (z == 6 ? 7 : 4);
}

static void
sbc_a(struct cpu_t* cpu, int z)
{
    /*
     * H: Set if borrow from bit 4
     * C: Set if borrow
     * Z: Set if A - zz - CF == 0
     * S: Set if A - zz - CF < 0 (bit 7 ON)
     * V: Set if
     *     or if
     * N: Always set
     */
    // TODO: Implement opcode.
}

static void
execute_table1(struct cpu_t* cpu, struct opcode_t* opstruct)
{
    if (opstruct->y == 6 && opstruct->z == 6) {
        // HALT:
    } else {
        ld_ry_rz(cpu, opstruct->y, opstruct->z);
    }
}

static void
execute_table2(struct cpu_t* cpu, struct opcode_t* opstruct)
{
    switch (opstruct->y) {
        case 0:
            add_a(cpu, opstruct->z);
            break;
        case 1:
            adc_a(cpu, opstruct->z);
            break;
        case 2:
            sub_a(cpu, opstruct->z);
            break;
        case 3:
            sbc_a(cpu, opstruct->z);
    }
}

static void
execute_table3(struct cpu_t* cpu, struct opcode_t* opstruct)
{
}


/**
 * Array con las distintas tablas. La intención es que si luego tengo que
 * usar la tabla I, pueda llamar simplemente a tables[I].
 */
static table_function tables[] = {
    &execute_table0, &execute_table1, &execute_table2, &execute_table3
};

void
execute_opcode(struct cpu_t* cpu)
{
    // Fill arrays
    rp[0] = &cpu->main.bc;
    rp[1] = &cpu->main.de;
    rp[2] = &cpu->main.hl;
    rp[3] = &cpu->sp;

    // Extraer opcode.
    struct opcode_t opdata;
    byte opcode = cpu->mem[cpu->pc.WORD++];
    extract_opcode(opcode, &opdata);

    // Procesar opcode.
    table_function table = tables[(int) opdata.x];
    table(cpu, &opdata);

    // Otras operaciones.
}
