/*
 * Copyright 2026 wtcat 
 */

#include <errno.h>
#include "application/helper/utils.h"

namespace helper {
uint32_t crc32_ieee_update(uint32_t crc, const uint8_t* data, size_t len) {
	/* crc table generated from polynomial 0xedb88320 */
	static const uint32_t table[16] = {
		0x00000000U, 0x1db71064U, 0x3b6e20c8U, 0x26d930acU, 0x76dc4190U, 0x6b6b51f4U,
		0x4db26158U, 0x5005713cU, 0xedb88320U, 0xf00f9344U, 0xd6d6a3e8U, 0xcb61b38cU,
		0x9b64c2b0U, 0x86d3d2d4U, 0xa00ae278U, 0xbdbdf21cU,
	};

	crc = ~crc;
	for (size_t i = 0; i < len; i++) {
		uint8_t byte = data[i];
		crc = (crc >> 4) ^ table[(crc ^ byte) & 0x0f];
		crc = (crc >> 4) ^ table[(crc ^ ((uint32_t)byte >> 4)) & 0x0f];
	}
	return ~crc;
}

std::vector<std::string> StringSplit(const std::string& s, char delim) {
	std::vector<std::string> result;
	size_t start = 0, pos;
	while ((pos = s.find(delim, start)) != std::string::npos) {
		result.emplace_back(s, start, pos - start);
		start = pos + 1;
	}
	result.emplace_back(s, start);  // last token
	return result;
}

int StrSplit(char* string, int stringlen, char** tokens, int maxtokens,
	char delim) {
	int i, tok = 0;
	int tokstart = 1; /* first token is right at start of string */

	if (string == NULL || tokens == NULL)
		goto einval_error;

	for (i = 0; i < stringlen; i++) {
		if (string[i] == '\0' || tok >= maxtokens)
			break;
		if (tokstart) {
			tokstart = 0;
			tokens[tok++] = &string[i];
		}
		if (string[i] == delim) {
			string[i] = '\0';
			tokstart = 1;
		}
	}
	return tok;

einval_error:
	return -EINVAL;
}

} //namespace helper

