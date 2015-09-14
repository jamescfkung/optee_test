/*
 * Copyright (c) 2015, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <tee_api.h>
#include <bonnie.h>
#include <ta_bonnie.h>
#include <tee_internal_api_extensions.h>

#define DEFAULT_CHUNK_SIZE (1 << 12)
#define DEFAULT_DATA_SIZE (DEFAULT_CHUNK_SIZE * 1023)

#define SCRAMBLE(x) ((x & 0xff) ^ 0xaa)

#define ASSERT_PARAM_TYPE(pt_in, pt_expect) \
do { \
	if ((pt_in) != (pt_expect)) \
		return TEE_ERROR_BAD_PARAMETERS; \
} while (0)


static uint8_t filename[] = "BonnieTestFile";

static void fill_buffer(uint8_t *buf, size_t size)
{
	size_t i;

	if (!buf)
		return;

	for (i = 0; i < size; i++)
		buf[i] = SCRAMBLE(i);
}

static TEE_Result verify_buffer(uint8_t *buf, size_t size)
{
	size_t i;

	if (!buf)
		return TEE_ERROR_BAD_PARAMETERS;

	for (i = 0; i < size; i++) {
		uint8_t expect_data = SCRAMBLE(i);

		if (expect_data != buf[i]) {
			return TEE_ERROR_CORRUPT_OBJECT;
		}
	}

	return TEE_SUCCESS;
}

static inline uint32_t tee_time_to_ms(TEE_Time t)
{
	return t.seconds * 1000 + t.millis;
}

static inline uint32_t get_delta_time_in_ms(TEE_Time start, TEE_Time stop)
{
	return tee_time_to_ms(stop) -tee_time_to_ms(start);
}

static TEE_Result test_write(TEE_ObjectHandle object, size_t data_size,
		uint8_t *chunk_buf, size_t chunk_size,
		uint32_t *spent_time_in_ms)
{
	TEE_Time start_time, stop_time;
	size_t tmp_data_size = data_size;
	TEE_Result res = TEE_SUCCESS;

	/* Seek position to the beginning of the second block */
	res = TEE_SeekObjectData(object, 4080, TEE_DATA_SEEK_SET);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to seek to the beginning of the second block");
		goto exit;
	}

	TEE_GetSystemTime(&start_time);

	while (tmp_data_size > 0) {
		DMSG("Write data, remain bytes: %u", tmp_data_size);
		res = TEE_WriteObjectData(object, chunk_buf, chunk_size);
		if (res != TEE_SUCCESS) {
			EMSG("Failed to write data, res=0x%08x", res);
			goto exit;
		}
		tmp_data_size -= chunk_size;
	}

	TEE_GetSystemTime(&stop_time);

	*spent_time_in_ms = get_delta_time_in_ms(start_time, stop_time);

	IMSG("start: %u.%u(s), stop: %u.%u(s), delta: %u(ms)",
			start_time.seconds, start_time.millis,
			stop_time.seconds, stop_time.millis,
			*spent_time_in_ms);

exit:
	return res;
}

static TEE_Result test_read(TEE_ObjectHandle object, size_t data_size,
		uint8_t *chunk_buf, size_t chunk_size,
		uint32_t *spent_time_in_ms)
{
	TEE_Time start_time, stop_time;
	size_t tmp_data_size = data_size;
	TEE_Result res = TEE_SUCCESS;
	uint32_t read_bytes = 0;

	/* Seek position to the beginning of the second block */
	res = TEE_SeekObjectData(object, 4080, TEE_DATA_SEEK_SET);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to seek to the beginning of the second block");
		goto exit;
	}

	/* Prepare file data */
	while (tmp_data_size > 0) {
		res = TEE_WriteObjectData(object, chunk_buf, chunk_size);
		if (res != TEE_SUCCESS) {
			EMSG("Failed to write data, res=0x%08x", res);
			goto exit;
		}
		tmp_data_size -= chunk_size;
	}
	
	/* Seek position to the beginning of the second block */
	res = TEE_SeekObjectData(object, 4080, TEE_DATA_SEEK_SET);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to seek to the beginning of the second block");
		goto exit;
	}

	tmp_data_size = data_size;

	/* Start test */
	TEE_GetSystemTime(&start_time);

	while (tmp_data_size > 0) {
		DMSG("Read data, remain bytes: %u", tmp_data_size);
		res = TEE_ReadObjectData(object, chunk_buf, chunk_size,
				&read_bytes);
		if (res != TEE_SUCCESS) {
			EMSG("Failed to read data, res=0x%08x", res);
			goto exit;
		}

		tmp_data_size -= chunk_size;
	}

	TEE_GetSystemTime(&stop_time);

	*spent_time_in_ms = get_delta_time_in_ms(start_time, stop_time);

	IMSG("start: %u.%u(s), stop: %u.%u(s), delta: %u(ms)",
			start_time.seconds, start_time.millis,
			stop_time.seconds, stop_time.millis,
			*spent_time_in_ms);

exit:
	return res;
}

static TEE_Result test_rewrite(TEE_ObjectHandle object, size_t data_size,
		uint8_t *chunk_buf, size_t chunk_size,
		uint32_t *spent_time_in_ms)
{
	TEE_Time start_time, stop_time;
	size_t tmp_data_size = data_size;
	TEE_Result res = TEE_SUCCESS;
	uint32_t read_bytes = 0;
	int32_t negative_chunk_size = -(int32_t)chunk_size;

	/* Seek position to the beginning of the second block */
	res = TEE_SeekObjectData(object, 4080, TEE_DATA_SEEK_SET);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to seek to the beginning of the second block");
		goto exit;
	}

	/* Prepare file data */
	while (tmp_data_size > 0) {
		res = TEE_WriteObjectData(object, chunk_buf, chunk_size);
		if (res != TEE_SUCCESS) {
			EMSG("Failed to write data, res=0x%08x", res);
			goto exit;
		}
		tmp_data_size -= chunk_size;
	}

	/* Seek position to the beginning of the second block */
	res = TEE_SeekObjectData(object, 4080, TEE_DATA_SEEK_SET);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to seek to the beginning of the second block");
		goto exit;
	}

	tmp_data_size = data_size;

	/* Start test */
	TEE_GetSystemTime(&start_time);

	while (tmp_data_size > 0) {
		/* Read a chunk */
		res = TEE_ReadObjectData(object, chunk_buf, chunk_size,
				&read_bytes);
		if (res != TEE_SUCCESS) {
			EMSG("Failed to read data, res=0x%08x", res);
			goto exit;
		}

		if (read_bytes != chunk_size) {
			EMSG("Partial data read, bytes=%u", read_bytes);
			res = TEE_ERROR_CORRUPT_OBJECT;
			goto exit;
		}

		/* Seek to the position before read */
		res = TEE_SeekObjectData(object, negative_chunk_size,
				TEE_DATA_SEEK_CUR);
		if (res != TEE_SUCCESS) {
			EMSG("Failed to seek to previous offset");
			goto exit;
		}

		/* Write a chunk*/
		res = TEE_WriteObjectData(object, chunk_buf, chunk_size);
		if (res != TEE_SUCCESS) {
			EMSG("Failed to write data, res=0x%08x", res);
			goto exit;
		}

		tmp_data_size -= chunk_size;
	}

	TEE_GetSystemTime(&stop_time);

	*spent_time_in_ms = get_delta_time_in_ms(start_time, stop_time);

	IMSG("start: %u.%u(s), stop: %u.%u(s), delta: %u(ms)",
			start_time.seconds, start_time.millis,
			stop_time.seconds, stop_time.millis,
			*spent_time_in_ms);

exit:
	return res;
}

static TEE_Result verify_file_data(TEE_ObjectHandle object, size_t data_size,
		uint8_t *chunk_buf, size_t chunk_size)
{
	TEE_Result res;
	size_t tmp_data_size = data_size;

	res = TEE_SeekObjectData(object, 0, TEE_DATA_SEEK_SET);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to seek to offset 0");
		goto exit;
	}

	TEE_MemFill(chunk_buf, 0, chunk_size);

	tmp_data_size = data_size;
	while (tmp_data_size > 0) {
		uint32_t read_bytes = 0;

		res = TEE_ReadObjectData(object, chunk_buf, chunk_size,
				&read_bytes);
		if (res != TEE_SUCCESS) {
			EMSG("Failed to read data, res=0x%08x", res);
			goto exit;
		}

		if (read_bytes != chunk_size) {
			EMSG("Data size not match");
			res = TEE_ERROR_CORRUPT_OBJECT;
			goto exit;
		}

		res = verify_buffer(chunk_buf, chunk_size);
		if (res != TEE_SUCCESS) {
			EMSG("Verify data failed, res=0x%08x", res);
			goto exit;
		}

		tmp_data_size -= chunk_size;
	}

exit:
	return res;
}

TEE_Result ta_bonnie_cmd_handler(uint32_t nCommandID,
		uint32_t param_types, TEE_Param params[4])
{
	TEE_Result res;
	size_t data_size;
	size_t chunk_size;
	TEE_ObjectHandle object = TEE_HANDLE_NULL;
	uint8_t *chunk_buf;
	uint32_t *spent_time_in_ms = &params[0].value.a;
	bool do_verify;

	ASSERT_PARAM_TYPE(param_types, TEE_PARAM_TYPES(
					TEE_PARAM_TYPE_VALUE_INOUT,
					TEE_PARAM_TYPE_VALUE_INPUT,
					TEE_PARAM_TYPE_NONE,
					TEE_PARAM_TYPE_NONE));

	data_size = params[0].value.a;
	chunk_size = params[0].value.b;
	do_verify = params[1].value.a;

	if (data_size == 0)
		data_size = DEFAULT_DATA_SIZE;

	if (chunk_size == 0)
		chunk_size = DEFAULT_CHUNK_SIZE;

	/* rounded down to multiple of chunk */
	data_size = chunk_size * (data_size / chunk_size);

	IMSG("command id: %u, test data size: %zd, chunk size: %zd\n",
			nCommandID, data_size, chunk_size);

	chunk_buf = TEE_Malloc(chunk_size, TEE_MALLOC_FILL_ZERO);
	if (!chunk_buf) {
		EMSG("Failed to allocate memory");
		res = TEE_ERROR_OUT_OF_MEMORY;
		goto exit;
	}

	fill_buffer(chunk_buf, chunk_size);

	res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
			filename, sizeof(filename),
			TEE_DATA_FLAG_ACCESS_READ |
			TEE_DATA_FLAG_ACCESS_WRITE |
			TEE_DATA_FLAG_ACCESS_WRITE_META |
			TEE_DATA_FLAG_OVERWRITE,
			NULL, NULL, 0, &object);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to create persistent object, res=0x%08x",
				res);
		goto exit;
	}

	switch (nCommandID) {
	case TA_BONNIE_CMD_TEST_READ:
		res = test_read(object, data_size, chunk_buf,
				chunk_size, spent_time_in_ms);
		break;

	case TA_BONNIE_CMD_TEST_WRITE:
		res = test_write(object, data_size, chunk_buf,
				chunk_size, spent_time_in_ms);
		break;

	case TA_BONNIE_CMD_TEST_REWRITE:
		res = test_rewrite(object, data_size, chunk_buf,
				chunk_size, spent_time_in_ms);
		break;

	default:
		res = TEE_ERROR_BAD_PARAMETERS;
	}

	if (res != TEE_SUCCESS)
		goto exit;

	if (do_verify)
		res = verify_file_data(object, data_size,
				chunk_buf, chunk_size);

exit:
	TEE_Free(chunk_buf);
	TEE_CloseAndDeletePersistentObject1(object);

	return res;
}

