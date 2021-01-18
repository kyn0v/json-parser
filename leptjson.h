#ifndef LEPTJSON_H__
#define LEPTJSON_H__

enum lept_type {
	LEPT_NULL,
	LEPT_FALSE,
	LEPT_TRUE,
	LEPT_NUMBER,
	LEPT_STRING,
	LEPT_ARRAY,
	LEPT_OBJECT
};

struct lept_value {
	union {
		struct { char* s; size_t len; } s; /* string: null-terminated string, string length */
		double n; /* number */
	} u;
	lept_type type;
};

enum lept_status {
	LEPT_PARSE_OK = 0,
	LEPT_PARSE_EXPECT_VALUE,
	LEPT_PARSE_INVALID_VALUE,
	LEPT_PARSE_ROOT_NOT_SINGULAR,
	LEPT_PARSE_NUMBER_TOO_BIG,
	LEPT_PARSE_MISS_QUOTATION_MARK,
	LEPT_PARSE_INVALID_STRING_ESCAPE,
	LEPT_PARSE_INVALID_STRING_CHAR
};

/* API */
void lept_init(lept_value* v);

void lept_free(lept_value* v);

int lept_parse(lept_value* v, const char* json);

lept_type lept_get_type(const lept_value* v);

bool lept_get_boolean(const lept_value* v);
void lept_set_boolean(lept_value* v, bool b);

double lept_get_number(const lept_value* v);
void lept_set_number(lept_value* v, double n);

const char* lept_get_string(const lept_value* v);
size_t lept_get_string_length(const lept_value* v);
void lept_set_string(lept_value* v, const char* s, size_t len);

#endif /* LEPTJSON_H__ */
