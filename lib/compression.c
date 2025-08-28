/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * lib/compression.c - compression/decompression operations.
 *
 * Copyright (c) 2022-2025 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#include <linux/fs.h>

#ifdef HAVE_LIBLZO2
#include <lzo/lzo1x.h>
#endif

#include "ssdfs_tools.h"

int ssdfs_zlib_compress(unsigned char *data_in,
			unsigned char *cdata_out,
			u32 *srclen, u32 *destlen,
			int is_debug)
{
	z_stream stream;
	int err = 0;

	SSDFS_DBG(is_debug,
		  "data_in %p, cdata_out %p, "
		  "srclen %u, destlen %u\n",
		  data_in, cdata_out, *srclen, *destlen);

	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	err = deflateInit(&stream, Z_BEST_COMPRESSION);
	if (err != Z_OK) {
		SSDFS_ERR("deflateInit() failed\n");
		err = -EINVAL;
		goto failed_compress;
	}

	stream.next_in = data_in;
	stream.avail_in = *srclen;
	stream.total_in = 0;

	stream.next_out = cdata_out;
	stream.avail_out = *destlen;
	stream.total_out = 0;

	SSDFS_DBG(is_debug,
		  "calling deflate with: "
		  "stream.avail_in %lu, stream.total_in %lu, "
		  "stream.avail_out %lu, stream.total_out %lu\n",
		  (unsigned long)stream.avail_in,
		  (unsigned long)stream.total_in,
		  (unsigned long)stream.avail_out,
		  (unsigned long)stream.total_out);

	err = deflate(&stream, Z_FINISH);

	SSDFS_DBG(is_debug,
		  "deflate returned with: "
		  "stream.avail_in %lu, stream.total_in %lu, "
		  "stream.avail_out %lu, stream.total_out %lu\n",
		  (unsigned long)stream.avail_in,
		  (unsigned long)stream.total_in,
		  (unsigned long)stream.avail_out,
		  (unsigned long)stream.total_out);

	if (err != Z_STREAM_END) {
		if (err == Z_OK) {
			err = -E2BIG;
			SSDFS_DBG(is_debug,
				  "unable to compress: "
				  "total_in %lu, total_out %lu\n",
				  (unsigned long)stream.total_in,
				  (unsigned long)stream.total_out);
		} else {
			SSDFS_ERR("ZLIB compression failed: "
				  "internal err %d\n",
				  err);
		}
		goto failed_compress;
	}

	err = deflateEnd(&stream);
	if (err != Z_OK) {
		SSDFS_ERR("ZLIB compression failed with internal err %d\n",
			  err);
		goto failed_compress;
	}

	if (stream.total_out >= stream.total_in) {
		SSDFS_DBG(is_debug,
			  "unable to compress: total_in %lu, total_out %lu\n",
			  (unsigned long)stream.total_in,
			  (unsigned long)stream.total_out);
		err = -E2BIG;
		goto failed_compress;
	}

	*destlen = stream.total_out;
	*srclen = stream.total_in;

	SSDFS_DBG(is_debug,
		  "compress has succeded: srclen %u, destlen %u\n",
		  *srclen, *destlen);

failed_compress:
	return err;
}

int ssdfs_zlib_decompress(unsigned char *cdata_in,
			  unsigned char *data_out,
			  u32 srclen, u32 destlen,
			  int is_debug)
{
	z_stream stream;
	int ret = Z_OK;

	SSDFS_DBG(is_debug,
		  "cdata_in %p, data_out %p, "
		  "srclen %u, destlen %u\n",
		  cdata_in, data_out, srclen, destlen);

	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	stream.next_in = cdata_in;
	stream.avail_in = srclen;
	stream.total_in = 0;

	stream.next_out = data_out;
	stream.avail_out = destlen;
	stream.total_out = 0;

	ret = inflateInit(&stream);
	if (ret != Z_OK) {
		SSDFS_ERR("inflateInit() failed\n");
		return -EINVAL;
	}

	do {
		ret = inflate(&stream, Z_FINISH);
	} while (ret == Z_OK);

	inflateEnd(&stream);

	if (ret != Z_STREAM_END) {
		SSDFS_ERR("inflate returned %d\n", ret);
		return -EFAULT;
	}

	SSDFS_DBG(is_debug,
		  "decompression has succeded: "
		  "total_in %lu, total_out %lu\n",
		  (unsigned long)stream.total_in,
		  (unsigned long)stream.total_out);

	return 0;
}

#ifdef HAVE_LIBLZO2
int ssdfs_lzo_compress(unsigned char *data_in,
		       unsigned char *cdata_out,
		       u32 *srclen, u32 *destlen,
		       int is_debug)
{
	lzo_uint out_len;
	unsigned char *work_mem;
	int err;

	SSDFS_DBG(is_debug,
		  "data_in %p, cdata_out %p, "
		  "srclen %u, destlen %u\n",
		  data_in, cdata_out, *srclen, *destlen);

	if (lzo_init() != LZO_E_OK) {
		SSDFS_ERR("LZO initialization failed\n");
		return -EINVAL;
	}

	work_mem = malloc(LZO1X_1_MEM_COMPRESS);
	if (!work_mem) {
		SSDFS_ERR("unable to allocate working memory\n");
		return -ENOMEM;
	}

	out_len = *destlen;
	err = lzo1x_1_compress(data_in, *srclen, cdata_out, &out_len, work_mem);

	free(work_mem);

	if (err != LZO_E_OK) {
		SSDFS_ERR("LZO compression failed: err %d\n", err);
		return -EFAULT;
	}

	if (out_len >= *srclen) {
		SSDFS_DBG(is_debug,
			  "unable to compress: srclen %u, out_len %lu\n",
			  *srclen, (unsigned long)out_len);
		return -E2BIG;
	}

	*destlen = out_len;

	SSDFS_DBG(is_debug,
		  "compress has succeded: srclen %u, destlen %u\n",
		  *srclen, *destlen);

	return 0;
}

int ssdfs_lzo_decompress(unsigned char *cdata_in,
			 unsigned char *data_out,
			 u32 srclen, u32 destlen,
			 int is_debug)
{
	lzo_uint out_len;
	int err;

	SSDFS_DBG(is_debug,
		  "cdata_in %p, data_out %p, "
		  "srclen %u, destlen %u\n",
		  cdata_in, data_out, srclen, destlen);

	if (lzo_init() != LZO_E_OK) {
		SSDFS_ERR("LZO initialization failed\n");
		return -EINVAL;
	}

	out_len = destlen;
	err = lzo1x_decompress(cdata_in, srclen, data_out, &out_len, NULL);

	if (err != LZO_E_OK) {
		SSDFS_ERR("LZO decompression failed: err %d\n", err);
		return -EFAULT;
	}

	if (out_len != destlen) {
		SSDFS_ERR("decompressed size mismatch: "
			  "expected %u, got %lu\n",
			  destlen, (unsigned long)out_len);
		return -EFAULT;
	}

	SSDFS_DBG(is_debug,
		  "decompression has succeded: "
		  "srclen %u, destlen %lu\n",
		  srclen, (unsigned long)out_len);

	return 0;
}
#else
int ssdfs_lzo_compress(unsigned char *data_in,
		       unsigned char *cdata_out,
		       u32 *srclen, u32 *destlen,
		       int is_debug)
{
	SSDFS_ERR("LZO compression is not supported\n");
	return -EOPNOTSUPP;
}

int ssdfs_lzo_decompress(unsigned char *cdata_in,
			 unsigned char *data_out,
			 u32 srclen, u32 destlen,
			 int is_debug)
{
	SSDFS_ERR("LZO decompression is not supported\n");
	return -EOPNOTSUPP;
}
#endif
