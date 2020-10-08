#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "leptjson.h"

using namespace std;

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

inline void EXPECT_EQ_BASE(bool equality, int expect, int actual, string format) {
	test_count++;
	if (equality)
		test_pass++;
	else {
		fprintf(stderr, ("%s:%d: expect: " + format + " actual: " + format + "\n").c_str(), __FILE__, __LINE__, expect, actual);
		main_ret = 1;
	}
}

inline void EXPECT_EQ_INT(int expect, int actual) {
	EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d");
}

static void test_parse_null() {
	lept_value v;
	v.type = LEPT_FALSE;
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null"));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

static void test_parse_expect_value() {
	lept_value v;

	v.type = LEPT_FALSE;
	EXPECT_EQ_INT(LEPT_PARSE_EXPECT_VALUE, lept_parse(&v, ""));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));

	v.type = LEPT_FALSE;
	EXPECT_EQ_INT(LEPT_PARSE_EXPECT_VALUE, lept_parse(&v, " "));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

static void test_parse_invalid_value() {
	lept_value v;
	v.type = LEPT_FALSE;
	EXPECT_EQ_INT(LEPT_PARSE_INVALID_VALUE, lept_parse(&v, "nul"));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));

	v.type = LEPT_FALSE;
	EXPECT_EQ_INT(LEPT_PARSE_INVALID_VALUE, lept_parse(&v, "?"));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

static void test_parse_root_not_singular() {
	lept_value v;
	v.type = LEPT_FALSE;
	EXPECT_EQ_INT(LEPT_PARSE_ROOT_NOT_SINGULAR, lept_parse(&v, "null x"));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

static void test_parse() {
	test_parse_null();
	test_parse_expect_value();
	test_parse_invalid_value();
	test_parse_root_not_singular();
}

int main() {
	test_parse();
	printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
	return main_ret;
}
