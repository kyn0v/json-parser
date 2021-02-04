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

typedef struct lept_member lept_member; // Forward Declaration

struct lept_value {
	union {
		struct { lept_member* m; size_t size; } o; /* object */
		struct { lept_value* e; size_t size; } a; /* array */
		struct { char* s; size_t len; } s; /* string: null-terminated string, string length */
		double n; /* number */
	} u;
	lept_type type;
};

struct lept_member {
	char* k; size_t klen;   /* member key string, key string length */
	lept_value v;           /* member value */
};

enum lept_status {
	LEPT_PARSE_OK = 0,
	LEPT_PARSE_EXPECT_VALUE,	// A JSON contains only blanks
	LEPT_PARSE_INVALID_VALUE,	
	LEPT_PARSE_ROOT_NOT_SINGULAR,	// After a value, there are other characters after the blank 
	LEPT_PARSE_NUMBER_TOO_BIG,
	LEPT_PARSE_MISS_QUOTATION_MARK,
	LEPT_PARSE_INVALID_STRING_ESCAPE,
	LEPT_PARSE_INVALID_STRING_CHAR,
	LEPT_PARSE_INVALID_UNICODE_HEX,
	LEPT_PARSE_INVALID_UNICODE_SURROGATE,
	LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
	LEPT_PARSE_MISS_KEY,
	LEPT_PARSE_MISS_COLON,
	LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET
};

/* API */
void lept_init(lept_value* v);

void lept_free(lept_value* v);

int lept_parse(lept_value* v, const char* json);

lept_type lept_get_type(const lept_value* v);

void lept_set_null(lept_value* v);

bool lept_get_boolean(const lept_value* v);
void lept_set_boolean(lept_value* v, bool b);

double lept_get_number(const lept_value* v);
void lept_set_number(lept_value* v, double n);

const char* lept_get_string(const lept_value* v);
size_t lept_get_string_length(const lept_value* v);
void lept_set_string(lept_value* v, const char* s, size_t len);

size_t lept_get_array_size(const lept_value* v);
lept_value* lept_get_array_element(const lept_value* v, size_t index);

size_t lept_get_object_size(const lept_value* v);
const char* lept_get_object_key(const lept_value* v, size_t index);
size_t lept_get_object_key_length(const lept_value* v, size_t index);
lept_value* lept_get_object_value(const lept_value* v, size_t index);

#endif /* LEPTJSON_H__ */
