#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod()*/

struct lept_context {
	const char* json;
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
	v->n = strtod(c->json, NULL);
	c->json = p;
	v->type = LEPT_NUMBER;
	return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
	switch (*c->json) {
	case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
	case 't': return lept_parse_literal(c, v, "true", LEPT_TRUE);
	case 'f': return lept_parse_literal(c, v, "false", LEPT_FALSE);
	default:  return lept_parse_number(c, v);	// 0-9 || -
	case '\0': return LEPT_PARSE_EXPECT_VALUE;
	}
}

int lept_parse(lept_value* v, const char* json) {
	lept_context c;
	int ret;
	assert(v != NULL);
	c.json = json;
	v->type = LEPT_NULL;
	lept_parse_whitespace(&c);
	ret = lept_parse_value(&c, v);
	if (ret == LEPT_PARSE_OK) {
		lept_parse_whitespace(&c);
		if (*c.json != '\0') {
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	return ret;
}

lept_type lept_get_type(const lept_value* v) {
	assert(v != NULL);
	return v->type;
}

double lept_get_number(const lept_value* v)
{
	assert(v != NULL && v->type == LEPT_NUMBER);
	return v->n;
}
