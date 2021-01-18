#include "leptjson.h"
#include <assert.h>		/* assert() */
#include <stdlib.h>		/* NULL, strtod(), malloc(), realloc(), free() */
#include <errno.h>		/* errno, ERANGE */
#include <math.h>		/* HUGE_VAL */
#include <string.h>		/* memcpt() */

const int LEPT_PARSE_STACK_INIT_SIZE = 256;

struct lept_context {
	const char* json;
	char* stack;
	size_t size, top;
};

static void* lept_context_push(lept_context* c, size_t size) {
	void* ret;
	assert(size > 0);
	if (c->top + size >= c->size) {
		if (c->size == 0) {
			c->size = LEPT_PARSE_STACK_INIT_SIZE;
		}
		while (c->top + size >= c->size) {
			c->size += c->size >> 1;	/* c->size * 1.5 */
		}
		char* new_ptr = (char*)realloc(c->stack, c->size);
		if (!new_ptr) { // Memory allocation failed
			// TODO
			return NULL;
		}
		else {
			c->stack = new_ptr;
		}
	}
	ret = c->stack + c->top;
	c->top += size;
	return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {
	assert(c->top >= size);
	return c->stack + (c->top -= size);
}

inline void EXPECT(lept_context* c, char ch) {
	assert(*c->json == (ch));
	c->json++;
}

inline bool ISDIGIT1TO9(char ch) {
	return (ch >= '1' && ch <= '9');
}

inline bool ISDIGIT(char ch) {
	return (ch >= '0' && ch <= '9');
}

inline void PUTC(lept_context* c, char ch) {
	*(char *)lept_context_push(c, sizeof(char)) = ch;
}

inline void lept_init(lept_value * v) {
	v->type = LEPT_NULL;
}

inline void lept_free(lept_value* v) {
	assert(v != NULL);
	if (v->type == LEPT_STRING) {
		free(v->u.s.s);
	}
	v->type = LEPT_NULL;
}

static void lept_parse_whitespace(lept_context* c) {
	const char* p = c->json;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
		p++;
	c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
	size_t i;
	EXPECT(c, literal[0]);
	for (i = 1; literal[i]; i++) {
		if (c->json[i - 1] != literal[i]) {
			return LEPT_PARSE_INVALID_VALUE;
		}
	}
	c->json += i;
	v->type = type;
	return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
	/*
		grammar:
			number = [ "-" ] int [ frac ] [ exp ]
			int = "0" / digit1-9 *digit
			frac = "." 1*digit
			exp = ("e" / "E") ["-" / "+"] 1*digit
	*/
	const char* p = c->json;

	if (*p == '-') p++;

	if (*p == '0') p++;
	else {
		if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
		for (p++; ISDIGIT(*p); p++);
	}

	if (*p == '.') {
		p++;
		if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
		for (p++; ISDIGIT(*p); p++);
	}

	if (*p == 'e' || *p == 'E') {
		p++;
		if (*p == '+' || *p == '-') p++;
		if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
		for (p++; ISDIGIT(*p); p++);
	}
	errno = 0;
	v->u.n = strtod(c->json, NULL);
	if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
		return LEPT_PARSE_NUMBER_TOO_BIG;
	c->json = p;
	v->type = LEPT_NUMBER;
	return LEPT_PARSE_OK;
}

static int lept_parse_string(lept_context* c, lept_value* v) {
	size_t head = c->top, len;
	const char* p;
	EXPECT(c, '\"');
	p = c->json;
	for (;;) {
		char ch = *p++;
		switch (ch) {
		case '\"':
			len = c->top - head;
			lept_set_string(v, (const char*)lept_context_pop(c, len), len);
			c->json = p;
			return LEPT_PARSE_OK;
		case '\0':
			c->top = head;
			return LEPT_PARSE_MISS_QUOTATION_MARK;
		default:
			PUTC(c, ch);
		}
	}
}

static int lept_parse_value(lept_context* c, lept_value* v) {
	switch (*c->json) {
		case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
		case 't': return lept_parse_literal(c, v, "true", LEPT_TRUE);
		case 'f': return lept_parse_literal(c, v, "false", LEPT_FALSE);
		default:  return lept_parse_number(c, v);	// 0-9 || -
		case '"': return lept_parse_string(c, v);
		case '\0': return LEPT_PARSE_EXPECT_VALUE;
	}
}

int lept_parse(lept_value* v, const char* json) {
	lept_context c;
	int ret;
	assert(v != NULL);
	c.json = json;
	c.stack = NULL;
	c.size = c.top = 0;
	lept_init(v);
	lept_parse_whitespace(&c);
	ret = lept_parse_value(&c, v);
	if (ret == LEPT_PARSE_OK) {
		lept_parse_whitespace(&c);
		if (*c.json != '\0') {
			v->type = LEPT_NULL;
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	assert(c.top == 0);
	free(c.stack);
	return ret;
}

lept_type lept_get_type(const lept_value* v) {
	assert(v != NULL);
	return v->type;
}

bool lept_get_boolean(const lept_value* v) {
	assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
	return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, bool b) {
	lept_free(v);
	v->type = b ? LEPT_TRUE : LEPT_FALSE;
}

double lept_get_number(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_NUMBER);
	return v->u.n;
}

void lept_set_number(lept_value* v, double n) {
	lept_free(v);
	v->type = LEPT_NUMBER;
	v->u.n = n;
}

const char * lept_get_string(const lept_value* v) {
	return nullptr;
}

size_t lept_get_string_length(const lept_value* v) {
	return size_t();
}

void lept_set_string(lept_value* v, const char* s, size_t len) {
	assert(v != NULL && (s != NULL || len == 0));
	lept_free(v);
	v->u.s.s = (char*)malloc(len + 1);
	memcpy(v->u.s.s, s, len);	// Ìæ»»³Émemcpy_s?
	v->u.s.s[len] = '\0';
	v->u.s.len = len;
	v->type = LEPT_STRING;
}