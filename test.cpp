#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "leptjson.h"

using namespace std;

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

template<typename T>
void EXPECT_EQ_BASE(bool equality, T expect, T actual, string format) {
	test_count++;
	if (equality)
		test_pass++;
	else {
		fprintf(stderr, ("%s:%d: expect: " + format + " actual: " + format + "\n").c_str(), __FILE__, __LINE__, expect, actual);
		main_ret = 1;
	}
}

void EXPECT_EQ_INT(int expect, int actual) {
	EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.0f");
}

void EXPECT_EQ_DOUBLE(double expect, double actual) {
	EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g");
}

void EXPECT_EQ_STRING(string expect, string actual) {
	EXPECT_EQ_BASE(expect.compare(actual) == 0, expect, actual, "%s");
}

void EXPECT_EQ_STRING(const char expect[], const char actual[], size_t alength) {
	EXPECT_EQ_BASE(strlen(expect) == alength && memcmp(expect, actual, alength) == 0, expect, actual, "%s");
	/*
	该测试函数存在bug，无法通过测试样例：TEST_STRING("Hello\0World", "\"Hello\\u0000World\"");
	此时：
		strlen(expect) = 5, alength = 11, 
		sizeof(expect) = 8, sizeof(int *) = 8, sizeof("Hello\0World") - 1 = 11
	*/
}

void EXPECT_TRUE(bool actual) {
	EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s");
}

void EXPECT_FALSE(bool actual) {
	EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s");
}

void TEST_NUMBER(double expect, const char* json) {
	lept_value v;
	lept_init(&v);
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));
	EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v));
	EXPECT_EQ_DOUBLE(expect, lept_get_number(&v));
	lept_free(&v);
}

void TEST_STRING(const char expect[], const char json[]) {
	lept_value v;
	lept_init(&v);
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));
	EXPECT_EQ_INT(LEPT_STRING, lept_get_type(&v));
	EXPECT_EQ_STRING(expect, lept_get_string(&v));
	lept_free(&v);
}

void TEST_ERROR(double error, const char* json) {
	lept_value v;
	lept_init(&v);
	v.type = LEPT_FALSE;
	EXPECT_EQ_INT(error, lept_parse(&v, json));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
	lept_free(&v);
}

static void test_parse_null() {
	lept_value v;
	lept_init(&v);
	lept_set_null(&v);
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null"));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
	lept_free(&v);
}

static void test_parse_true() {
	lept_value v;
	lept_init(&v);
	lept_set_boolean(&v, false);
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "true"));
	EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
	lept_free(&v);
}

static void test_parse_false() {
	lept_value v;
	lept_init(&v);
	lept_set_boolean(&v, true);
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "false"));
	EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v));
	lept_free(&v);
}

static void test_parse_number() {
	TEST_NUMBER(0.0, "0");
	TEST_NUMBER(0.0, "-0");
	TEST_NUMBER(0.0, "-0.0");
	TEST_NUMBER(1.0, "1");
	TEST_NUMBER(-1.0, "-1");
	TEST_NUMBER(1.5, "1.5");
	TEST_NUMBER(-1.5, "-1.5");
	TEST_NUMBER(3.1416, "3.1416");
	TEST_NUMBER(1E10, "1E10");
	TEST_NUMBER(1e10, "1e10");
	TEST_NUMBER(1E+10, "1E+10");
	TEST_NUMBER(1E-10, "1E-10");
	TEST_NUMBER(-1E10, "-1E10");
	TEST_NUMBER(-1e10, "-1e10");
	TEST_NUMBER(-1E+10, "-1E+10");
	TEST_NUMBER(-1E-10, "-1E-10");
	TEST_NUMBER(1.234E+10, "1.234E+10");
	TEST_NUMBER(1.234E-10, "1.234E-10");
	TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

								  /* invalid number */
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+0");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+1");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "INF");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "inf");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "NAN");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nan");

	/* add test case*/
	TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); // the smallest number > 1
	TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324"); /* Min subnormal positive double */
	TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
	TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
	TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
	TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
	TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
	TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
	TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

static void test_parse_string() {
// 代码中已有注释时，用 #if 0 ... #endif 去禁用代码是一个常用技巧，而且可以把 0 改为 1 去恢复。
	TEST_STRING("", "\"\"");
	TEST_STRING("Hello", "\"Hello\"");
	TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
	TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
	TEST_STRING("Hello\0World", "\"Hello\\u0000World\"");
#if 0
	TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
	TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
	TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
	TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
	TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
#endif
}

static void test_parse_expect_value() {
	TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
	TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nul");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "?");
}

static void test_parse_root_not_singular() {
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "null x");

	/* invalid number */
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x0");
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

static void test_parse_number_too_big() {
	TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "1e309");
	TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_missing_quotation_mark() {
	TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"");
	TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
#if 1
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
#endif
}

static void test_parse_invalid_string_char() {
#if 1
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
#endif
}

static void test_access_null() {
	lept_value v;
	lept_init(&v);
	lept_set_string(&v, "a", 1);
	lept_set_null(&v);
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
	lept_free(&v);
}

static void test_access_boolean() {
	lept_value v;
	lept_init(&v);
	lept_set_string(&v, "a", 1);
	lept_set_boolean(&v, 1);
	EXPECT_TRUE(lept_get_boolean(&v));
	lept_set_boolean(&v, 0);
	EXPECT_FALSE(lept_get_boolean(&v));
	lept_free(&v);
}

static void test_access_number() {
	lept_value v;
	lept_init(&v);
	lept_set_string(&v, "a", 1);
	lept_set_number(&v, 1234.5);
	EXPECT_EQ_DOUBLE(1234.5, lept_get_number(&v));
	lept_free(&v);
}

static void test_access_string() {
	lept_value v;
	lept_init(&v);
	lept_set_string(&v, "", 0);
	EXPECT_EQ_STRING("", lept_get_string(&v));
	lept_set_string(&v, "Hello", 5);
	EXPECT_EQ_STRING("Hello", lept_get_string(&v));
	lept_free(&v);
}

static void test_parse() {
	test_parse_null();
	test_parse_true();
	test_parse_false();
	test_parse_number();
	test_parse_string();
	test_parse_expect_value();
	test_parse_invalid_value();
	test_parse_root_not_singular();
	test_parse_number_too_big();
	test_parse_missing_quotation_mark();
	test_parse_invalid_string_escape();
	test_parse_invalid_string_char();

	test_access_null();
	test_access_boolean();
	test_access_number();
	test_access_string();
}

int main() {
#ifdef _WINDOWS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(279);
#endif
	test_parse();
	printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
	system("pause");
	return main_ret;
}
