/*
 * Copyright (c) 2015, Linaro Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
#include <string.h>

#include "xtest_test.h"
#include "xtest_helpers.h"

#include <ta_bonnie.h>

#define DO_VERIFY 0
#define DEFAULT_DATA_SIZE (2 * 1024 * 1024) // 2MB
#define DEFAULT_CHUNK_SIZE (4 * 1024) // 4KB

static void xtest_tee_benchmark_1001(ADBG_Case_t *Case_p);
static void xtest_tee_benchmark_1002(ADBG_Case_t *Case_p);
static void xtest_tee_benchmark_1003(ADBG_Case_t *Case_p);
static void xtest_tee_benchmark_1004(ADBG_Case_t *Case_p);
static void xtest_tee_benchmark_1005(ADBG_Case_t *Case_p);


static TEEC_Result run_test(enum bonnie_cmd cmd,
		uint32_t data_size, uint32_t chunk_size)
{
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	TEEC_Result res;
	TEEC_Session sess;
	uint32_t orig;
	float spent_time;
	float speed_in_kb;

	res = xtest_teec_open_session(&sess, &bonnie_ta_uuid, NULL, &orig);
	if (res != TEEC_SUCCESS)
		return res;

	op.params[0].value.a = data_size;
	op.params[0].value.b = chunk_size;
	op.params[1].value.a = DO_VERIFY;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT,
			TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE);

	res = TEEC_InvokeCommand(&sess, cmd, &op, &orig);

	spent_time = (float)op.params[0].value.a / 1000.0;
	speed_in_kb = ((float)data_size / 1024.0) / spent_time;

	Do_ADBG_Log("Spent time: %.3f (s)", spent_time);
	Do_ADBG_Log("Speed: %.3f (kB/s)", speed_in_kb);

	TEEC_CloseSession(&sess);

	return res;
}

static void xtest_tee_benchmark_1001(ADBG_Case_t *c)
{
	uint32_t data_size = 4096;
	uint32_t chunk_size = 1;

	ADBG_EXPECT_TEEC_SUCCESS(c,
			run_test(TA_BONNIE_CMD_TEST_WRITE, data_size, chunk_size));
}

static void xtest_tee_benchmark_1002(ADBG_Case_t *c)
{
	uint32_t data_size = DEFAULT_DATA_SIZE;
	uint32_t chunk_size = DEFAULT_CHUNK_SIZE;

	ADBG_EXPECT_TEEC_SUCCESS(c,
			run_test(TA_BONNIE_CMD_TEST_WRITE, data_size, chunk_size));
}

static void xtest_tee_benchmark_1003(ADBG_Case_t *c)
{
	uint32_t data_size = 4096;
	uint32_t chunk_size = 1;

	ADBG_EXPECT_TEEC_SUCCESS(c,
			run_test(TA_BONNIE_CMD_TEST_READ, data_size, chunk_size));
}

static void xtest_tee_benchmark_1004(ADBG_Case_t *c)
{
	uint32_t data_size = DEFAULT_DATA_SIZE;
	uint32_t chunk_size = DEFAULT_CHUNK_SIZE;

	ADBG_EXPECT_TEEC_SUCCESS(c,
			run_test(TA_BONNIE_CMD_TEST_READ, data_size, chunk_size));
}

static void xtest_tee_benchmark_1005(ADBG_Case_t *c)
{
	uint32_t data_size = DEFAULT_DATA_SIZE;
	uint32_t chunk_size = DEFAULT_CHUNK_SIZE;

	ADBG_EXPECT_TEEC_SUCCESS(c,
			run_test(TA_BONNIE_CMD_TEST_REWRITE, data_size, chunk_size));
}


ADBG_CASE_DEFINE(XTEST_TEE_BENCHMARK_1001, xtest_tee_benchmark_1001,
		/* Title */
		"TEE Trusted Storage Performance Test (PUTC)",
		/* Short description */
		"Write one byte at a time",
		/* Requirement IDs */ "",
		/* How to implement */ ""
		);

ADBG_CASE_DEFINE(XTEST_TEE_BENCHMARK_1002, xtest_tee_benchmark_1002,
		/* Title */
		"TEE Trusted Storage Performance Test (FASTWRITE)",
		/* Short description */
		"Write a chunk at a time",
		/* Requirement IDs */ "",
		/* How to implement */ ""
		);

ADBG_CASE_DEFINE(XTEST_TEE_BENCHMARK_1003, xtest_tee_benchmark_1003,
		/* Title */
		"TEE Trusted Storage Performance Test (GETC)",
		/* Short description */
		"Read one byte at a time",
		/* Requirement IDs */ "",
		/* How to implement */ ""
		);

ADBG_CASE_DEFINE(XTEST_TEE_BENCHMARK_1004, xtest_tee_benchmark_1004,
		/* Title */
		"TEE Trusted Storage Performance Test (FASTREAD)",
		/* Short description */
		"Read a chunk at a time",
		/* Requirement IDs */ "",
		/* How to implement */ ""
		);

ADBG_CASE_DEFINE(XTEST_TEE_BENCHMARK_1005, xtest_tee_benchmark_1005,
		/* Title */
		"TEE Trusted Storage Performance Test (REWRITE)",
		/* Short description */
		"Read a chunk then write back to file at the same position",
		/* Requirement IDs */ "",
		/* How to implement */ ""
		);


