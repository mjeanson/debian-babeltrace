/*
 * Common Trace Format
 *
 * Strings read/write functions.
 *
 * Copyright 2010-2011 EfficiOS Inc. and Linux Foundation
 *
 * Author: Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */

#include <babeltrace/babeltrace-internal.h>
#include <babeltrace/ctf/types.h>
#include <limits.h>		/* C99 limits */
#include <string.h>

int ctf_string_read(struct stream_pos *ppos, struct definition *definition)
{
	struct definition_string *string_definition =
		container_of(definition, struct definition_string, p);
	const struct declaration_string *string_declaration =
		string_definition->declaration;
	struct ctf_stream_pos *pos = ctf_pos(ppos);
	size_t len;
	ssize_t max_len;
	char *srcaddr;

	ctf_align_pos(pos, string_declaration->p.alignment);

	srcaddr = ctf_get_pos_addr(pos);
	if (pos->offset == EOF)
		return -EFAULT;
	/* Not counting \0 */
	max_len = pos->packet_size - pos->offset - 1;
	if (max_len < 0)
		return -EFAULT;
	len = strnlen(srcaddr, max_len) + 1;	/* Add \0 */
	/* Truncated string, unexpected. Trace probably corrupted. */
	if (srcaddr[len - 1] != '\0')
		return -EFAULT;

	if (string_definition->alloc_len < len) {
		string_definition->value =
			g_realloc(string_definition->value, len);
		string_definition->alloc_len = len;
	}
	printf_debug("CTF string read %s\n", srcaddr);
	memcpy(string_definition->value, srcaddr, len);
	string_definition->len = len;
	ctf_move_pos(pos, len * CHAR_BIT);
	return 0;
}

int ctf_string_write(struct stream_pos *ppos,
		      struct definition *definition)
{
	struct definition_string *string_definition =
		container_of(definition, struct definition_string, p);
	const struct declaration_string *string_declaration =
		string_definition->declaration;
	struct ctf_stream_pos *pos = ctf_pos(ppos);
	size_t len;
	char *destaddr;

	ctf_align_pos(pos, string_declaration->p.alignment);
	assert(string_definition->value != NULL);
	len = string_definition->len;

	if (!ctf_pos_access_ok(pos, len))
		return -EFAULT;

	if (pos->dummy)
		goto end;
	destaddr = ctf_get_pos_addr(pos);
	memcpy(destaddr, string_definition->value, len);
end:
	ctf_move_pos(pos, len * CHAR_BIT);
	return 0;
}
