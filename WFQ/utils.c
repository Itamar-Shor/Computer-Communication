#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include "utils.h"
#include "defines.h"
#include <stdio.h>
#include <stdlib.h>

int split(char* str, const char delim, int max_splits, char** dst)
{
	int curr_len = 0;
	int nof_splits = 0;
	char* start = str;

	while (*str && nof_splits < max_splits) {
		curr_len++;
		if (*str == delim) {
			if (curr_len == 1) {
				curr_len = 0;
				str++;
				continue;
			}
			*str = '\0';
			dst[nof_splits] = malloc((curr_len+1) * sizeof(char)); // to include the null terminator!
			if (!dst[nof_splits]) {
				return -1;
			}
			strncpy(dst[nof_splits], start, curr_len+1);
			curr_len = 0;
			nof_splits++;
			*str = delim;
			start = str + 1;
		}
		str++;
	}
	if (curr_len > 0) {
		dst[nof_splits] = malloc((curr_len + 1) * sizeof(char)); // to include the null terminator!
		if (!dst[nof_splits]) {
			return -1;
		}
		strncpy(dst[nof_splits], start, curr_len+1);
	}
	return nof_splits;
}

char* extract_field(char* packet, int field) {
	char** str = (char**) malloc(sizeof(char*)*(field+1));
	malloc_check("extract_field", "str", str);
	int nof_splits = split(packet, ' ', field + 1, str);
	if (nof_splits < field) {
		for (int i = 0; i < nof_splits; i++) {
			free(str[i]);
		}
		return NULL;
	}
	char* wanted_field = str[field];
	for (int i = 0; i < field + 1; i++) {
		if (i != field) {
			free(str[i]);
		}
	}
	return wanted_field;
}