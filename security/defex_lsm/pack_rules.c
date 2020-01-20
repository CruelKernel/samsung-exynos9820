/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/limits.h>
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/defex_rules.h"

#define SAFE_STRCOPY(dst, src) do { strncpy(dst, src, sizeof(dst)); dst[sizeof(dst) - 1] = 0; } while(0)

struct file_list_item {
	char file_name[PATH_MAX];
#ifdef DEFEX_INTEGRITY_ENABLE
	char integrity[INTEGRITY_LENGTH * 2 + 1];
#endif /* DEFEX_INTEGRITY_ENABLE */
	unsigned int is_recovery;
};

struct rule_item_struct *defex_packed_rules;
int packfiles_count, packfiles_size;

struct file_list_item *file_list = NULL;
int file_list_count = 0;

#ifndef DEFEX_DEBUG_ENABLE
int debug_ifdef_is_active = 0;
void process_debug_ifdef(const char *src_str);
#endif

/* Suplementary functions for packing rules */
struct rule_item_struct *create_file_item(const char *name, int l);
struct rule_item_struct *add_file_item(struct rule_item_struct *base, const char *name, int l);
struct rule_item_struct *lookup_dir(struct rule_item_struct *base, const char *name, int l);
struct rule_item_struct *add_file_path(const char *file_path);
struct rule_item_struct *addline2tree(char *src_line, enum feature_types feature);
char *extract_rule_text(const char *src_line);
int lookup_tree(const char *file_path, int attribute);
int store_tree(FILE *f, FILE *f_bin);

#ifdef DEFEX_INTEGRITY_ENABLE
/* Transfer string to hex */
unsigned char ascii_to_hex(unsigned char input);
int string_to_hex(unsigned char *input, size_t inputLen, unsigned char *output);
#endif /* DEFEX_INTEGRITY_ENABLE */

/* Suplementary functions for reducing rules */
int remove_substr(char *str, const char *part);
char* remove_redundant_chars(char *str);
int load_file_list(const char *name);
int lookup_file_list(const char *rule);

/* Main processing functions */
int reduce_rules(const char *source_rules_file, const char *reduced_rules_file, const char *list_file);
int pack_rules(const char *source_rules_file, const char *packed_rules_file, const char *packed_rules_binfile);


struct rule_item_struct *create_file_item(const char *name, int l)
{
	struct rule_item_struct *item;
	unsigned int offset;

	if (!name)
		l = 0;
	offset = packfiles_size;
	packfiles_size += (sizeof(struct rule_item_struct) + l);
	packfiles_count++;
	item = GET_ITEM_PTR(offset);
	item->next_file = 0;
	item->next_level = 0;
	item->feature_type = 0;
	item->size = l;
#ifdef DEFEX_INTEGRITY_ENABLE
	memset(item->integrity, 0, INTEGRITY_LENGTH);
#endif /* DEFEX_INTEGRITY_ENABLE */
	if (l)
		memcpy(item->name, name, l);
	return item;
}

struct rule_item_struct *add_file_item(struct rule_item_struct *base, const char *name, int l)
{
	struct rule_item_struct *item, *new_item = NULL;

	if (!base)
		return new_item;

	new_item = create_file_item(name, l);
	if (!base->next_level) {
		base->next_level = GET_ITEM_OFFSET(new_item);
	} else {
		item = GET_ITEM_PTR(base->next_level);
		while(item->next_file) {
			item = GET_ITEM_PTR(item->next_file);
		}
		item->next_file = GET_ITEM_OFFSET(new_item);
	}
	return new_item;
}

struct rule_item_struct *lookup_dir(struct rule_item_struct *base, const char *name, int l)
{
	struct rule_item_struct *item = NULL;
	unsigned int offset;

	if (!base || !base->next_level)
		return item;
	item = GET_ITEM_PTR(base->next_level);
	do {
		if (item->size == l && !memcmp(name, item->name, l)) return item;
		offset = item->next_file;
		item = GET_ITEM_PTR(offset);
	} while(offset);
	return NULL;
}

struct rule_item_struct *add_file_path(const char *file_path)
{
	const char *ptr, *next_separator;
	struct rule_item_struct *base, *cur_item = NULL;
	int l;

	if (!file_path || *file_path != '/')
		return NULL;
	if (!defex_packed_rules) {
		packfiles_count = 0;
		packfiles_size = 0;
		defex_packed_rules = calloc(4, 1024 * 1024);
		if (!defex_packed_rules) {
			printf("WARNING: Can not create the new item!\n");
			exit(-1);
		}
		create_file_item("HEAD", 4);
	}
	base = defex_packed_rules;
	ptr = file_path + 1;
	do {
		next_separator = strchr(ptr, '/');
		if (!next_separator)
			l = strlen(ptr);
		else
			l = next_separator - ptr;
		if (!l)
			return NULL; /* two slashes in sequence */
		cur_item = lookup_dir(base, ptr, l);
		if (!cur_item) {
			cur_item = add_file_item(base, ptr, l);
			/* slash wasn't found, it's a file */
			if (!next_separator)
				cur_item->feature_type |= feature_is_file;
		}
		base = cur_item;
		ptr += l;
		if (next_separator)
			ptr++;
	} while(*ptr);
	return cur_item;
}

int lookup_tree(const char *file_path, int attribute)
{
	const char *ptr, *next_separator;
	struct rule_item_struct *base, *cur_item = NULL;
	int l;

	if (!file_path || *file_path != '/' || !defex_packed_rules)
		return 0;
	base = defex_packed_rules;
	ptr = file_path + 1;
	do {
		next_separator = strchr(ptr, '/');
		if (!next_separator)
			l = strlen(ptr);
		else
			l = next_separator - ptr;
		if (!l)
			return 0;
		cur_item = lookup_dir(base, ptr, l);
		if (!cur_item)
			break;
		if (cur_item->feature_type & attribute)
			return 1;
		base = cur_item;
		ptr += l;
		if (next_separator)
			ptr++;
	} while(*ptr);
	return 0;
}

char *extract_rule_text(const char *src_line)
{
	char *start_ptr, *end_ptr;

	start_ptr = strchr(src_line, '\"');
	if (start_ptr) {
		start_ptr++;
		end_ptr = strchr(start_ptr, '\"');
		if (end_ptr) {
			*end_ptr = 0;
			return start_ptr;
		}
	}
	return NULL;
}

#ifdef DEFEX_INTEGRITY_ENABLE
unsigned char ascii_to_hex(unsigned char input)
{
	if (input >= 0x30 && input <=0x39)
		return input - 0x30;
	else if (input >= 0x41 && input <= 0x46)
		return input - 0x37;
	else if (input >= 0x61 && input <= 0x66)
		return input - 0x57;
	else
		return 0xFF;
}

int string_to_hex(unsigned char *input, size_t inputLen, unsigned char *output)
{
	char convert1, convert2;
	size_t i;

	if (input == NULL || output == NULL)
		return 0;

	// Check input is a paired value.
	if (inputLen % 2 != 0)
		return 0;

	// Convert ascii code to hexa.
	for (i = 0; i < inputLen / 2; i++) {
		convert1 = ascii_to_hex(input[2*i]);
		convert2 = ascii_to_hex(input[2*i+1]);

		if (convert1 == 0xFF || convert2 == 0xFF)
			return 0;

		output[i] = (convert1 << 4) | convert2;
	}

	return 1;
}
#endif /* DEFEX_INTEGRITY_ENABLE */

struct rule_item_struct *addline2tree(char *src_line, enum feature_types feature)
{
	struct rule_item_struct *item = NULL;
	char *str;

#ifdef DEFEX_INTEGRITY_ENABLE
	unsigned char *integrity;
	int value;
#endif /* DEFEX_INTEGRITY_ENABLE */

	str = extract_rule_text(src_line);
	if (str == NULL)
		return NULL;

#ifdef DEFEX_INTEGRITY_ENABLE
	integrity = (unsigned char *)extract_rule_text(str + strnlen(str, PATH_MAX) +1);
#endif /* DEFEX_INTEGRITY_ENABLE */

	if (str) {
		item = add_file_path(str);
		if (item) {
			item->feature_type |= feature;

#ifdef DEFEX_INTEGRITY_ENABLE
			if (integrity) {
				value = string_to_hex(integrity, INTEGRITY_LENGTH * 2, item->integrity);
				if (!value)
					return NULL;
			}
#endif /* DEFEX_INTEGRITY_ENABLE */
		}
	}
	return item;
}

int store_tree(FILE *f, FILE *f_bin)
{
	unsigned char *ptr = (unsigned char *)defex_packed_rules;
	static char work_str[4096];
	int i, offset = 0, index = 0;

	work_str[0] = 0;
	fprintf(f, "const unsigned char defex_packed_rules[] = {\n");
	for (i = 0; i < packfiles_size; i++) {
		if (index)
			offset += snprintf(work_str + offset, sizeof(work_str) - offset, ", ");
		offset += snprintf(work_str + offset, sizeof(work_str) - offset, "0x%02x", ptr[i]);
		index++;
		if (index == 16) {
			fprintf(f, "\t%s,\n", work_str);
			index = 0;
			offset = 0;
		}
	}
	if (index)
		fprintf(f, "\t%s\n", work_str);
	fprintf(f, "};\n");
	if (f_bin)
		fwrite(defex_packed_rules, 1, packfiles_size, f_bin);
	return 0;
}

int remove_substr(char *str, const char *part)
{
	int l, part_l, found = 0;
	char *ptr;

	l = strnlen(str, PATH_MAX - 1);
	ptr = strstr(str, part);
	if (ptr) {
		part_l = strnlen(part, 32) - 1;
		memmove(ptr, ptr + part_l, l - (ptr - str) - part_l + 1);
		found = 1;
	}
	return found;
}

char* remove_redundant_chars(char *str)
{
	int l;
	char *ptr;

	/* skip hash values in the begin */
	str += 65;

	/* remove CR or LF at the end */
	ptr = strchr(str, '\r');
	if (ptr)
		*ptr = 0;
	ptr = strchr(str, '\n');
	if (ptr)
		*ptr = 0;
	l = strnlen(str, PATH_MAX - 1);
	/* remove starting dot or space */
	while(l && (*str == '.' || *str == ' '))
		str++;
	return str;
}

int load_file_list(const char *name)
{
	int found;
	char *str;
	FILE *lst_file = NULL;
	static char work_str[PATH_MAX*2];

	lst_file = fopen(name, "r");
	if (!lst_file)
		return -1;

	while(!feof(lst_file)) {
		if (!fgets(work_str, sizeof(work_str), lst_file))
			break;
		str = remove_redundant_chars(work_str);
		if (*str == '/' &&
				(!strncmp(str, "/root/", 6) ||
				!strncmp(str, "/recovery/", 10) ||
				!strncmp(str, "/system/", 8) ||
				!strncmp(str, "/tmp/", 5) ||
				!strncmp(str, "/vendor/", 8) ||
				!strncmp(str, "/data/", 6))) {
			remove_substr(str, "/root/");
			found = remove_substr(str, "/recovery/");
			file_list_count++;
			file_list = realloc(file_list, sizeof(struct file_list_item) * file_list_count);
#ifdef DEFEX_INTEGRITY_ENABLE
			strncpy(file_list[file_list_count - 1].integrity, work_str, INTEGRITY_LENGTH * 2);
			file_list[file_list_count - 1].integrity[INTEGRITY_LENGTH * 2] = 0;
#endif /* DEFEX_INTEGRITY_ENABLE */
			SAFE_STRCOPY(file_list[file_list_count - 1].file_name, str);
			file_list[file_list_count - 1].is_recovery = found;
		}
	}
	fclose(lst_file);
	return 0;
}

int lookup_file_list(const char *rule)
{
	int i;

	for (i = 0; i < file_list_count; i++) {
		if (!strncmp(file_list[i].file_name, rule, strnlen(rule, PATH_MAX))
			&& !strncmp(file_list[i].file_name, rule, strnlen(file_list[i].file_name, PATH_MAX)))
			return i+1;
	}
	return 0;
}

#ifndef DEFEX_DEBUG_ENABLE
void process_debug_ifdef(const char *src_str)
{
	char *ptr;
	ptr = strstr(src_str, "#ifdef DEFEX_DEBUG_ENABLE");
	if (ptr) {
		while(ptr > src_str) {
			ptr--;
			if (*ptr != ' ' && *ptr != '\t')
				return;
		}
		debug_ifdef_is_active = 1;
		return;
	}
	ptr = strstr(src_str, "#endif");
	if (ptr && debug_ifdef_is_active) {
		while(ptr > src_str) {
			ptr--;
			if (*ptr != ' ' && *ptr != '\t')
				return;
		}
		debug_ifdef_is_active = 0;
		return;
	}
}
#endif

int reduce_rules(const char *source_rules_file, const char *reduced_rules_file, const char *list_file)
{
	int exist = 0, ret_val = -1;
	char *ptr, *rule;
	static char work_str[PATH_MAX*2], tmp_str[PATH_MAX*2];
	FILE *src_file = NULL, *dst_file = NULL;

	src_file = fopen(source_rules_file, "r");
	if (!src_file)
		return -1;
	dst_file = fopen(reduced_rules_file, "wt");
	if (!dst_file)
		goto do_close2;

	if (load_file_list(list_file) != 0)
		goto do_close1;

	while(!feof(src_file)) {
		if (!fgets(work_str, sizeof(work_str), src_file))
			break;
		ptr = strstr(work_str, "feature_safeplace_path");
		if (!ptr)
			ptr = strstr(work_str, "feature_ped_exception");

		if (ptr) {
			SAFE_STRCOPY(tmp_str, work_str);
			rule = extract_rule_text(tmp_str);
			exist = lookup_file_list(rule);
			if (rule && !exist && !strstr(work_str, "/* DEFAULT */")) {
				printf("- removed rule: %s\n", rule);
				continue;
			}
		}
#ifdef DEFEX_INTEGRITY_ENABLE
		if (exist) {
			/* Add hash vale after each file path */
			printf("remained rule: %s, %s\n", rule, file_list[exist-1].integrity);
			work_str[strnlen(work_str, PATH_MAX)-3]=0;
			fputs(work_str, dst_file);
			fputs(",\"", dst_file);
			fputs(file_list[exist-1].integrity, dst_file);
			fputs("\"},\n", dst_file);
			exist = 0;
		}
#else
		fputs(work_str, dst_file);
#endif /* DEFEX_INTEGRITY_ENABLE */
	}
	ret_val = 0;
do_close1:
	fclose(dst_file);
do_close2:
	fclose(src_file);
	return ret_val;
}

int pack_rules(const char *source_rules_file, const char *packed_rules_file, const char *packed_rules_binfile)
{
	int ret_val = -1;
	char *ptr;
	FILE *src_file = NULL, *dst_file = NULL, *dst_binfile = NULL;
	static char work_str[PATH_MAX*2];

	src_file = fopen(source_rules_file, "r");
	if (!src_file)
		return -1;
	dst_file = fopen(packed_rules_file, "wt");
	if (!dst_file)
		goto do_close2;
	if (packed_rules_binfile) {
		dst_binfile = fopen(packed_rules_binfile, "wt");
		if (!dst_binfile)
			goto do_close3;
	}

	while(!feof(src_file)) {
		if (!fgets(work_str, sizeof(work_str), src_file))
			break;
#ifndef DEFEX_DEBUG_ENABLE
		process_debug_ifdef(work_str);
		if (!debug_ifdef_is_active) {
#endif
		ptr = strstr(work_str, "feature_safeplace_path");
		if (ptr) {
			addline2tree(work_str, feature_safeplace_path);
			continue;
		}

		ptr = strstr(work_str, "feature_ped_exception");
		if (ptr) {
			addline2tree(work_str, feature_ped_exception);
			continue;
		}
#ifndef DEFEX_DEBUG_ENABLE
		}
#endif
	}
	store_tree(dst_file, dst_binfile);
	if (!packfiles_count)
		printf("WARNING: Defex packed rules tree is empty!\n");
	ret_val = 0;
	if (dst_binfile)
		fclose(dst_binfile);
do_close3:
	fclose(dst_file);
do_close2:
	fclose(src_file);
	return ret_val;
}


int main(int argc, char **argv)
{
	static char param1[PATH_MAX], param2[PATH_MAX], param3[PATH_MAX];

	if (argc >= 4 && argc <= 5) {
		SAFE_STRCOPY(param1, argv[2]);
		SAFE_STRCOPY(param2, argv[3]);
		if (argc == 5) {
			SAFE_STRCOPY(param3, argv[4]);
		}
		if (argc == 5 && !strncmp(argv[1], "-r", 2)) {
			if (reduce_rules(param1, param2, param3) != 0)
				goto show_help;
		}

		if (argc == 4 && !strncmp(argv[1], "-p", 2)) {
			if (pack_rules(param1, param2, NULL) != 0)
				goto show_help;
		}

		if (argc == 5 && !strncmp(argv[1], "-p", 2)) {
			if (pack_rules(param1, param2, param3) != 0)
				goto show_help;
		}
		return 0;
	}

show_help:
	printf("Defex rules processing utility.\nUSAGE:\n%s <CMD> <PARAMS>\n"
		"Commands:\n"
		"  -p - Pack rules file to the tree. Params: <SOURCE_FILE> <PACKED_FILE>\n"
		"  -r - Reduce rules file (remove unexistent files). Params: <SOURCE_FILE> <REDUCED_FILE> <FILE_LIST>\n",
		argv[0]);
	return -1;
}
