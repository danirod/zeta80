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

#include <check.h>

#include <cpu.h>
#include <opcodes.h>

#include "../opcodes_test.h" // Testcase definitions.

/*
 * This test is created in the assumption that there is only one function
 * that executes ADC A, _ and that the other operand to add is fetched
 * inside that function.
 */

START_TEST(test_ADC_A_B)
{
    cpu.mem[0] = 0x88;
    REG_A(cpu) = 0x12;
    REG_B(cpu) = 0x34;
    FLAG_SET(cpu, FLAG_C);

    execute_opcode(&cpu);

    ck_assert_uint_eq(0x47, REG_A(cpu));
    ck_assert_uint_eq(4, cpu.tstates);
}
END_TEST

START_TEST(test_ADC_A_C)
{
    cpu.mem[0] = 0x89;
    REG_A(cpu) = 0x12;
    REG_C(cpu) = 0x34;
    FLAG_SET(cpu, FLAG_C);

    execute_opcode(&cpu);

    ck_assert_uint_eq(0x47, REG_A(cpu));
    ck_assert_uint_eq(4, cpu.tstates);
}
END_TEST

START_TEST(test_ADC_A_D)
{
    cpu.mem[0] = 0x8A;
    REG_A(cpu) = 0x12;
    REG_D(cpu) = 0x34;
    FLAG_SET(cpu, FLAG_C);

    execute_opcode(&cpu);

    ck_assert_uint_eq(0x47, REG_A(cpu));
    ck_assert_uint_eq(4, cpu.tstates);
}
END_TEST

START_TEST(test_ADC_A_E)
{
    cpu.mem[0] = 0x8B;
    REG_A(cpu) = 0x12;
    REG_E(cpu) = 0x34;
    FLAG_SET(cpu, FLAG_C);

    execute_opcode(&cpu);

    ck_assert_uint_eq(0x47, REG_A(cpu));
    ck_assert_uint_eq(4, cpu.tstates);
}
END_TEST

START_TEST(test_ADC_A_H)
{
    cpu.mem[0] = 0x8C;
    REG_A(cpu) = 0x12;
    REG_H(cpu) = 0x34;
    FLAG_SET(cpu, FLAG_C);

    execute_opcode(&cpu);

    ck_assert_uint_eq(0x47, REG_A(cpu));
    ck_assert_uint_eq(4, cpu.tstates);
}
END_TEST

START_TEST(test_ADC_A_L)
{
    cpu.mem[0] = 0x8D;
    REG_A(cpu) = 0x12;
    REG_L(cpu) = 0x34;
    FLAG_SET(cpu, FLAG_C);

    execute_opcode(&cpu);

    ck_assert_uint_eq(0x47, REG_A(cpu));
    ck_assert_uint_eq(4, cpu.tstates);
}
END_TEST

START_TEST(test_ADC_A_iHL)
{
    cpu.mem[0] = 0x8E;
    REG_A(cpu) = 0x12;
    REG_HL(cpu) = 0x8000;
    cpu.mem[0x8000] = 0x34;
    FLAG_SET(cpu, FLAG_C);

    execute_opcode(&cpu);

    ck_assert_uint_eq(0x47, REG_A(cpu));
    ck_assert_uint_eq(7, cpu.tstates);
}
END_TEST

START_TEST(test_ADC_A_A)
{
    cpu.mem[0] = 0x8F;
    REG_A(cpu) = 0x12;
    FLAG_SET(cpu, FLAG_C);

    execute_opcode(&cpu);

    ck_assert_uint_eq(0x25, REG_A(cpu));
    ck_assert_uint_eq(4, cpu.tstates);
}
END_TEST

START_TEST(test_ADC_A_sf)
{
    cpu.mem[0] = 0x88;
    byte val = 0;
    do {
        PC(cpu) = 0;
        REG_A(cpu) = 0x50;
        REG_B(cpu) = val;
        FLAG_SET(cpu, FLAG_C);
        execute_opcode(&cpu);
        if ((0x50 + val + 1) & 0x80)
            ck_assert_uint_ne(0, FLAG_GET(cpu, FLAG_S));
        else
            ck_assert_uint_eq(0, FLAG_GET(cpu, FLAG_S));
        val++;
    } while (val != 0);
}
END_TEST

START_TEST(test_ADC_A_zf)
{
    cpu.mem[0] = 0x88;
    byte val = 0;
    do {
        PC(cpu) = 0;
        REG_A(cpu) = 0x80;
        REG_B(cpu) = val;
        FLAG_SET(cpu, FLAG_C);

        execute_opcode(&cpu);

        if (((0x80 + val + 1) & 0xFF) == 0)
            ck_assert_uint_ne(0, FLAG_GET(cpu, FLAG_Z));
        else
            ck_assert_uint_eq(0, FLAG_GET(cpu, FLAG_Z));
        val++;
    } while (val != 0);
}
END_TEST

START_TEST(test_ADC_A_hf)
{
    cpu.mem[0] = 0x88;
    byte val = 0;
    do {
        PC(cpu) = 0;
        REG_A(cpu) = 0x55;
        REG_B(cpu) = val;
        FLAG_SET(cpu, FLAG_C);

        execute_opcode(&cpu);

        if ((((val & 0xF) + 0x6) & 0x10) != 0)
            ck_assert_uint_ne(0, FLAG_GET(cpu, FLAG_H));
        else
            ck_assert_uint_eq(0, FLAG_GET(cpu, FLAG_H));
        val++;
    } while (val != 0);
}
END_TEST

START_TEST(test_ADC_A_vf_set)
{
    // 120 + 105 + 1 should give overflow as Z80 Manual.
    cpu.mem[0] = 0x88;
    REG_A(cpu) = 120;
    REG_B(cpu) = 105;
    FLAG_SET(cpu, FLAG_C);
    execute_opcode(&cpu);
    ck_assert_uint_ne(0, FLAG_GET(cpu, FLAG_P));
}
END_TEST

START_TEST(test_ADC_A_vf_rst)
{
    // 0x30 + 0x40 + 1 should not give overflow.
    cpu.mem[0] = 0x88;
    REG_A(cpu) = 0x30;
    REG_B(cpu) = 0x40;
    FLAG_SET(cpu, FLAG_C);
    execute_opcode(&cpu);
    ck_assert_uint_eq(0x71, REG_A(cpu));
    ck_assert_uint_eq(0, FLAG_GET(cpu, FLAG_P));
}
END_TEST

START_TEST(test_ADC_A_cf)
{
    cpu.mem[0] = 0x88;
    byte val = 0;
    do {
        PC(cpu) = 0;
        REG_A(cpu) = 0x55;
        REG_B(cpu) = val;
        FLAG_SET(cpu, FLAG_C);
        execute_opcode(&cpu);
        if ((0x55 + val + 1) & 0xF00)
            ck_assert_uint_ne(0, FLAG_GET(cpu, FLAG_C));
        else
            ck_assert_uint_eq(0, FLAG_GET(cpu, FLAG_C));
        val++;
    } while (val != 0);
}
END_TEST

START_TEST(test_ADC_A_daa)
{
    cpu.mem[0] = 0x88;
    REG_A(cpu) = 0x12;
    REG_B(cpu) = 0x34;
    FLAG_SET(cpu, FLAG_C);
    execute_opcode(&cpu);
    ck_assert_uint_eq(0, FLAG_GET(cpu, FLAG_N));
}
END_TEST

TCase* gen_x2_z1_tcase()
{
    TCase* test = tcase_create("x=2, z=1");
    tcase_add_checked_fixture(test, setup_cpu, teardown_cpu);
    tcase_add_test(test, test_ADC_A_B);
    tcase_add_test(test, test_ADC_A_C);
    tcase_add_test(test, test_ADC_A_D);
    tcase_add_test(test, test_ADC_A_E);
    tcase_add_test(test, test_ADC_A_H);
    tcase_add_test(test, test_ADC_A_L);
    tcase_add_test(test, test_ADC_A_iHL);
    tcase_add_test(test, test_ADC_A_A);
    tcase_add_test(test, test_ADC_A_sf);
    tcase_add_test(test, test_ADC_A_zf);
    tcase_add_test(test, test_ADC_A_hf);
    tcase_add_test(test, test_ADC_A_vf_set);
    tcase_add_test(test, test_ADC_A_vf_rst);
    tcase_add_test(test, test_ADC_A_cf);
    tcase_add_test(test, test_ADC_A_daa);
    return test;
}
