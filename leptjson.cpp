#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "leptjson.h"
#include <assert.h>		/* assert() */
#include <stdlib.h>		/* NULL, strtod(), malloc(), realloc(), free() */
#include <errno.h>		/* errno, ERANGE */
#include <math.h>		/* HUGE_VAL */
#include <string.h>		/* memcpt() */
#include <stdio.h>   /* sprintf() */

const int LEPT_PARSE_STACK_INIT_SIZE = 256;
const int LEPT_PARSE_STRINGIFY_INIT_SIZE = 256;

struct lept_context {	// To reduce the number of parameters passed to paser-function, data is put into a stucture
	const char* json;
	char* stack;
	size_t size, top;
};

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

inline void PUTC(lept_context* c, char ch) {
	*(char *)lept_context_push(c, sizeof(char)) = ch;
}

inline void PUTS(lept_context* c, const char* s, size_t len) {
	memcpy(lept_context_push(c, len), s, len);
}

static void* lept_context_pop(lept_context* c, size_t size) {
	assert(c->top >= size);
	return c->stack + (c->top -= size);
}

inline void lept_init(lept_value * v) {
	v->type = LEPT_NULL;
}

void lept_free(lept_value* v) {
	assert(v != NULL);
	switch (v->type) {
		case LEPT_STRING:
			free(v->u.s.s);
			break;
		case LEPT_ARRAY:
			for (int i = 0; i < v->u.a.size; i++) {
				lept_free(&v->u.a.e[i]);	// Call lept_free() function recursively
			}
			free(v->u.a.e);
			break;
		case LEPT_OBJECT:
			for (int i = 0; i < v->u.o.size; i++) {
				free(v->u.o.m[i].k);
				lept_free(&v->u.o.m[i].v);
			}
			free(v->u.o.m);
			break;
		default: break;
	}
	v->type = LEPT_NULL;
}

static void lept_parse_whitespace(lept_context* c) {
	const char* p = c->json;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {	// £¨space£©/£¨tab£©/£¨LF£©/£¨CR£©
		p++;
	}
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

static const char* lept_parse_hex4(const char* p, unsigned* u) {	// Read 4 bits of hexadecimal numbers
	*u = 0;
	for (int i = 0; i < 4; i++) {
		char ch = *p++;
		*u <<= 4;
		if (ch >= '0'&&ch <= '9') *u |= ch - '0';
		else if (ch >= 'A'&&ch <= 'F')*u |= ch - 'A' + 10;
		else if (ch >= 'a'&&ch <= 'f')*u |= ch - 'a' + 10;
		else return NULL;
	}
	return p;
}

/*
BUG:
static const char* lept_parse_hex4(const char* p, unsigned* u) {
	char* end;
	*u = (unsigned)strtol(p, &end, 16);
	return end == p + 4 ? end : NULL;
}
note:	it would pass the invalid test case: "\u 123"£¬
		and it should add processing to determine whether there are spaces at the beginning. 
*/

static void lept_encode_utf8(lept_context*c, unsigned u) {
	if (u <= 0x7F) {
		PUTC(c, u & 0xFF);
	}
	else if (u <= 0x7FF) {
		PUTC(c, 0xC0 | ((u >> 6) & 0x1F));
		PUTC(c, 0x80 | (u & 0x3F));
	}
	else if (u < 0xFFFF) {
		PUTC(c, 0xE0 | ((u >> 12) & 0x0F));
		PUTC(c, (0x80 | ((u >> 6) & 0x3F)));
		PUTC(c, (0x80 | (u & 0x3F)));
	}
	else if (u < 0x10FFFF) {
		PUTC(c, 0xF0 | ((u >> 18) & 0x07));
		PUTC(c, 0x80 | ((u >> 12) & 0x3F));
		PUTC(c, (0x80 | ((u >> 6) & 0x3F)));
		PUTC(c, (0x80 | (u & 0x3F)));
	}
}

#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)

static int lept_parse_string_raw(lept_context* c, char** str, size_t* len) {
	/*
	grammar:
		string = quotation-mark *char quotation-mark
		char = unescaped /
		escape (
		%x22 /          ; "    quotation mark  U+0022
		%x5C /          ; \    reverse solidus U+005C
		%x2F /          ; /    solidus         U+002F
		%x62 /          ; b    backspace       U+0008
		%x66 /          ; f    form feed       U+000C
		%x6E /          ; n    line feed       U+000A
		%x72 /          ; r    carriage return U+000D
		%x74 /          ; t    tab             U+0009
		%x75 4HEXDIG )  ; uXXXX                U+XXXX
		escape = %x5C          ; \
		quotation-mark = %x22  ; "
		unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
	*/
	size_t head = c->top;
	unsigned u, u2;
	const char* p;
	EXPECT(c, '\"');
	p = c->json;
	for (;;) {
		char ch = *p++;
		switch (ch) {
		case '\"':	// case 1£ºReading ending quotation marks
			*len = c->top - head;
			*str = (char*)lept_context_pop(c, *len);
			c->json = p;
			return LEPT_PARSE_OK;
		case '\\':	// case 2£ºReading escape symbol
			switch (*p++) {
				case '\"': PUTC(c, '\"'); break;
				case '\\': PUTC(c, '\\'); break;
				case '/':  PUTC(c, '/'); break;
				case 'b':  PUTC(c, '\b'); break;
				case 'f':  PUTC(c, '\f'); break;
				case 'n':  PUTC(c, '\n'); break;
				case 'r':  PUTC(c, '\r'); break;
				case 't':  PUTC(c, '\t'); break;
				case 'u':
					if (!(p = lept_parse_hex4(p, &u))) {
						STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
					}
					if (u >= 0xD800 && u <= 0xDBFF) {
						if (*p++ != '\\') {
							STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
						}
						if (*p++ != 'u') {
							STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
						}
						if (!(p = lept_parse_hex4(p, &u2))) {
							STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
						}
						if (u2 < 0xDC00 || u2 > 0xDFFF) {
							STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
						}
						u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
					}
					lept_encode_utf8(c, u);
					break;
				default:	// invalid escape symbol
					STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);
			}
			break;
		case '\0':	// case 3£ºmissing quotation mark£¨see test sample in "test_parse_missing_quotation_mark"£©
			STRING_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK);
		default:	// case 4£ºnormal character
			if ((unsigned char)ch < 0x20) {	// invalid character
				STRING_ERROR(LEPT_PARSE_INVALID_STRING_CHAR);
			}
			PUTC(c, ch);
		}
	}
}

static int lept_parse_string(lept_context* c, lept_value* v) {
	int ret;
	char* s;
	size_t len;
	if ((ret = lept_parse_string_raw(c, &s, &len)) == LEPT_PARSE_OK)
		lept_set_string(v, s, len);
	return ret;
}

static int lept_parse_value(lept_context* c, lept_value* v); /* Forward Declaration */
static int lept_parse_array(lept_context* c, lept_value* v) {
	/*
	grammar:
		array = %x5B ws [ value *( ws %x2C ws value ) ] ws %x5D
	*/
	size_t size = 0;
	int ret;
	EXPECT(c, '[');
	lept_parse_whitespace(c);
	if (*c->json == ']') {
		c->json++;
		v->type = LEPT_ARRAY;
		v->u.a.size = 0;
		v->u.a.e = NULL;
		return LEPT_PARSE_OK;
	}
	//BUG:
	//for (;;) {
	//	lept_value* e = lept_context_push(c, sizeof(lept_value));
	//	lept_init(e);
	//	size++;
	//	if ((ret = lept_parse_value(c, e)) != LEPT_PARSE_OK)
	//		return ret;
	//	/* ... */
	//}
	for (;;) {
		lept_value e;
		lept_init(&e);
		if ((ret = lept_parse_value(c, &e)) != LEPT_PARSE_OK) {
			break;
		}
		memcpy(lept_context_push(c, sizeof(lept_value)), &e, sizeof(lept_value));
		size++;
		lept_parse_whitespace(c);
		if (*c->json == ',') {
			c->json++;
			lept_parse_whitespace(c);
		}
		else if (*c->json == ']') {
			c->json++;
			v->type = LEPT_ARRAY;
			v->u.a.size = size;
			size *= sizeof(lept_value);
			memcpy(v->u.a.e = (lept_value*)malloc(size), lept_context_pop(c, size), size);
			return LEPT_PARSE_OK;
		}
		else {
			ret = LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
			break;
		}
	}
	/* Pop and free values on the stack */
	for (int i = 0; i < size; i++)
		lept_free((lept_value*)lept_context_pop(c, sizeof(lept_value)));
	return ret;
}

static int lept_parse_object(lept_context* c, lept_value* v) {
	/*
	grammar:
		member = string ws %x3A ws value
		object = %x7B ws [ member *( ws %x2C ws member ) ] ws %x7D
	*/
	size_t size;
	lept_member m;
	int ret;
	EXPECT(c, '{');
	lept_parse_whitespace(c);
	if (*c->json == '}') {
		c->json++;
		v->type = LEPT_OBJECT;
		v->u.o.m = 0;
		v->u.o.size = 0;
		return LEPT_PARSE_OK;
	}
	m.k = NULL;
	size = 0;
	for (;;) {
		char* str;
		lept_init(&m.v);
		/* parse key to m.k, m.klen */
		if (*c->json != '"') {
			ret = LEPT_PARSE_MISS_KEY;
			break;
		}
		if ((ret = lept_parse_string_raw(c, &str, &m.klen)) != LEPT_PARSE_OK)
			break;
		memcpy(m.k = (char*)malloc(m.klen + 1), str, m.klen);
		m.k[m.klen] = '\0';
		/* parse ws colon ws */
		lept_parse_whitespace(c);
		if (*c->json != ':') {
			ret = LEPT_PARSE_MISS_COLON;
			break;
		}
		c->json++;
		lept_parse_whitespace(c);
		/* parse value */
		if ((ret = lept_parse_value(c, &m.v)) != LEPT_PARSE_OK)
			break;
		memcpy(lept_context_push(c, sizeof(lept_member)), &m, sizeof(lept_member));
		size++;
		m.k = NULL; /* ownership is transferred to member on stack */
		/* parse ws [comma | right-curly-brace] ws */
		lept_parse_whitespace(c);
		if (*c->json == ',') {
			c->json++;
			lept_parse_whitespace(c);
		}
		else if (*c->json == '}') {
			size_t s = sizeof(lept_member) * size;
			c->json++;
			v->type = LEPT_OBJECT;
			v->u.o.size = size;
			memcpy(v->u.o.m = (lept_member*)malloc(s), lept_context_pop(c, s), s);
			return LEPT_PARSE_OK;
		}
		else {
			ret = LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
			break;
		}
	}
	/* Pop and free members on the stack */
	/* 5. Pop and free members on the stack */
	free(m.k);
	for (int i = 0; i < size; i++) {
		lept_member* m = (lept_member*)lept_context_pop(c, sizeof(lept_member));
		free(m->k);
		lept_free(&m->v);
	}
	v->type = LEPT_NULL;
	return ret;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
	switch (*c->json) {
		case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
		case 't': return lept_parse_literal(c, v, "true", LEPT_TRUE);
		case 'f': return lept_parse_literal(c, v, "false", LEPT_FALSE);
		case '\0': return LEPT_PARSE_EXPECT_VALUE;
		case '"': return lept_parse_string(c, v);
		case '[': return lept_parse_array(c, v);
		case '{': return lept_parse_object(c, v);
		default:  return lept_parse_number(c, v);	// 0-9 || -
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

static void lept_stringify_string(lept_context* c, const char* s, size_t len) {
	static const char hex_digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	size_t i, size;
	char* head, *p;
	assert(s != NULL);
	p = head = (char *)lept_context_push(c, size = len * 6 + 2); /* "\u00xx..." */
	*p++ = '"';
	for (i = 0; i < len; i++) {
		unsigned char ch = (unsigned char)s[i];
		switch (ch) {
		case '\"': *p++ = '\\'; *p++ = '\"'; break;
		case '\\': *p++ = '\\'; *p++ = '\\'; break;
		case '\b': *p++ = '\\'; *p++ = 'b';  break;
		case '\f': *p++ = '\\'; *p++ = 'f';  break;
		case '\n': *p++ = '\\'; *p++ = 'n';  break;
		case '\r': *p++ = '\\'; *p++ = 'r';  break;
		case '\t': *p++ = '\\'; *p++ = 't';  break;
		default:
			if (ch < 0x20) {
				*p++ = '\\'; *p++ = 'u'; *p++ = '0'; *p++ = '0';
				*p++ = hex_digits[ch >> 4];
				*p++ = hex_digits[ch & 15];
			}
			else
				*p++ = s[i];
		}
	}
	*p++ = '"';
	c->top -= size - (p - head);
}

static void lept_stringify_value(lept_context* c, const lept_value* v) {
	size_t i;
	int ret;
	switch (v->type) {
		case LEPT_NULL:		PUTS(c, "null", 4); break;
		case LEPT_FALSE:	PUTS(c, "false", 5); break;
		case LEPT_TRUE:		PUTS(c, "true", 4); break;
		case LEPT_NUMBER:	c->top -= 32 - sprintf((char *)lept_context_push(c, 32), "%.17g", v->u.n); break;
		case LEPT_STRING:	lept_stringify_string(c, v->u.s.s, v->u.s.len); break;
		case LEPT_ARRAY:
			PUTC(c, '[');
			for (i = 0; i < v->u.a.size; i++) {
				if (i > 0)
					PUTC(c, ',');
				lept_stringify_value(c, &v->u.a.e[i]);
			}
			PUTC(c, ']');
			break;
		case LEPT_OBJECT:
			PUTC(c, '{');
			for (i = 0; i < v->u.o.size; i++) {
				if (i > 0)
					PUTC(c, ',');
				lept_stringify_string(c, v->u.o.m[i].k, v->u.o.m[i].klen);
				PUTC(c, ':');
				lept_stringify_value(c, &v->u.o.m[i].v);
			}
			PUTC(c, '}');
			break;
		default: assert(0 && "invalid type");
	}
}

char* lept_stringify(const lept_value* v, size_t* length) {
	lept_context c;
	assert(v != NULL);
	c.stack = (char*)malloc(c.size = LEPT_PARSE_STRINGIFY_INIT_SIZE);
	c.top = 0;
	lept_stringify_value(&c, v);
	if (length)
		*length = c.top;
	PUTC(&c, '\0');
	return c.stack;
}

lept_type lept_get_type(const lept_value* v) {
	assert(v != NULL);
	return v->type;
}

void lept_set_null(lept_value* v) {
	assert(v != NULL);
	lept_free(v);
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

const char* lept_get_string(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_STRING);
	return v->u.s.s;
}

size_t lept_get_string_length(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_STRING);
	return v->u.s.len;
}

void lept_set_string(lept_value* v, const char* s, size_t len) {
	assert(v != NULL && (s != NULL || len == 0));
	lept_free(v);
	v->type = LEPT_STRING;
	v->u.s.s = (char*)malloc(len + 1);
	memcpy(v->u.s.s, s, len);	// Ìæ»»³Émemcpy_s?
	v->u.s.s[len] = '\0';
	v->u.s.len = len;
}

size_t lept_get_array_size(const lept_value* v)
{
	assert(v != NULL && v->type == LEPT_ARRAY);
	return v->u.a.size;
}

lept_value* lept_get_array_element(const lept_value* v, size_t index)
{
	assert(v != NULL&&v->type == LEPT_ARRAY);
	assert(index < v->u.a.size);
	return &v->u.a.e[index];
}

size_t lept_get_object_size(const lept_value* v)
{
	assert(v != NULL && v->type == LEPT_OBJECT);
	return v->u.o.size;
}

const char* lept_get_object_key(const lept_value* v, size_t index)
{
	assert(v != NULL && v->type == LEPT_OBJECT);
	assert(index < v->u.o.size);
	return v->u.o.m[index].k;
}

size_t lept_get_object_key_length(const lept_value * v, size_t index)
{
	assert(v != NULL && v->type == LEPT_OBJECT);
	assert(index < v->u.o.size);
	return v->u.o.m[index].klen;
}

lept_value* lept_get_object_value(const lept_value* v, size_t index)
{
	assert(v != NULL && v->type == LEPT_OBJECT);
	assert(index < v->u.o.size);
	return &v->u.o.m[index].v;
}
