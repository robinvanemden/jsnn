#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jsnn.c"

#define MAX_CHARS 65536
static char jsfile[MAX_CHARS];

static int test_passed = 0;
static int test_failed = 0;

/* Terminate current test with error */
#define fail()	return __LINE__

/* Successfull end of the test case */
#define done() return 0

/* Check single condition */
#define check(cond) do { if (!(cond)) fail(); } while (0)

/* Test runner */
static void test(int (*func)(void), const char *name) {
	int r = func();
	if (r == 0) {
		test_passed++;
	} else {
		test_failed++;
		printf("FAILED: %s (at line %d)\n", name, r);
	}
}

#define TOKEN_EQ(t, tok_start, tok_end, tok_type) \
	((t).start == tok_start \
	 && (t).end == tok_end  \
	 && (t).type == (tok_type))

#define TOKEN_STRING(js, t, s) \
	(strncmp(js+(t).start, s, (t).end - (t).start) == 0 \
	 && strlen(s) == (t).end - (t).start)

#define TOKEN_PRINT(t) \
	printf("start: %d, end: %d, type: %d, size: %d\n", \
			(t).start, (t).end, (t).type, (t).size)

int open_json(const char *path) {
    FILE *fp;
    size_t size;
    fp = fopen(path, "r");

    if (fp == NULL)
        return -1;

    size = fread((void *)jsfile, sizeof(char), MAX_CHARS, fp);
    jsfile[size] = '\0';
    fclose(fp);

    return 0;
}

int test_empty() {
	const char *js;
	int r;
	jsnn_parser p;
	jsnntok_t t[10];

	js = "{}";
	jsnn_init(&p);
	r = jsnn_parse(&p, js, t, 10);
	check(r == JSNN_SUCCESS);
	check(t[0].type == JSNN_OBJECT);
	check(t[0].start == 0 && t[0].end == 2);

	js = "[]";
	jsnn_init(&p);
	r = jsnn_parse(&p, js, t, 10);
	check(r == JSNN_SUCCESS);
	check(t[0].type == JSNN_ARRAY);
	check(t[0].start == 0 && t[0].end == 2);

	js = "{\"a\":[]}";
	jsnn_init(&p);
	r = jsnn_parse(&p, js, t, 10);
	check(r == JSNN_SUCCESS);
	check(t[0].type == JSNN_OBJECT && t[0].start == 0 && t[0].end == 8);
	check(t[1].type == JSNN_STRING && t[1].start == 2 && t[1].end == 3);
	check(t[2].type == JSNN_ARRAY && t[2].start == 5 && t[2].end == 7);

	js = "[{},{}]";
	jsnn_init(&p);
	r = jsnn_parse(&p, js, t, 10);
	check(r == JSNN_SUCCESS);
	check(t[0].type == JSNN_ARRAY && t[0].start == 0 && t[0].end == 7);
	check(t[1].type == JSNN_OBJECT && t[1].start == 1 && t[1].end == 3);
	check(t[2].type == JSNN_OBJECT && t[2].start == 4 && t[2].end == 6);
	return 0;
}

int test_complex() {
	const char *js;
	int r;
	jsnn_parser p;
	jsnntok_t tokens[1024], *token;

	jsnn_init(&p);
    r = open_json("pets.json");
    check(r == 0);
	r = jsnn_parse(&p, jsfile, tokens, 1024);
	check(r == JSNN_SUCCESS);
    token = jsnn_get(tokens, "a.aa.aaa[1]", js, tokens);
    //printf("%.*s\n", token->end - token->start, js + token->start);
    check(token != NULL);

    return 0;
}

int test_cmp() {
	const char *js;
	int r;
	jsnn_parser p;
	jsnntok_t tokens[20], *token;

	js = "{\"a\": \"hi\"}";

	jsnn_init(&p);
	r = jsnn_parse(&p, js, tokens, 10);
	check(r == JSNN_SUCCESS);
    token = jsnn_get(tokens, "a", js, tokens);
    check(token != NULL);
    check(jsnn_cmp(token, js, "hi") == 0);
    return 0;
}

int test_deep() {
	const char *js;
	int r;
	jsnn_parser p;
	jsnntok_t tokens[20], *token;

	js = "{\"a\": {\"aa\": {\"aaa\": [3, 2, 1]}}}";

	jsnn_init(&p);
	r = jsnn_parse(&p, js, tokens, 10);
	check(r == JSNN_SUCCESS);
    //token = jsnn_match_attr(js, tokens, tokens, "a", 1);
    //check(token != NULL);
    token = jsnn_get(tokens, "a.aa.aaa[1]", js, tokens);
    //printf("%.*s\n", token->end - token->start, js + token->start);
    check(token != NULL);
	check(TOKEN_EQ(*token, 25, 26, JSNN_PRIMITIVE));
    //check(atoi(js + token->start) == 0);

    return 0;
}

int test_simple() {
	const char *js;
	int r;
	jsnn_parser p;
	jsnntok_t tokens[10], *token;

	js = "{\"a\": 0}";

	jsnn_init(&p);
	r = jsnn_parse(&p, js, tokens, 10);
	check(r == JSNN_SUCCESS);
	check(TOKEN_EQ(tokens[0], 0, 8, JSNN_OBJECT));
	check(TOKEN_EQ(tokens[1], 2, 3, JSNN_STRING));
	check(TOKEN_EQ(tokens[2], 6, 7, JSNN_PRIMITIVE));

	check(TOKEN_STRING(js, tokens[0], js));
	check(TOKEN_STRING(js, tokens[1], "a"));
	check(TOKEN_STRING(js, tokens[2], "0"));

	jsnn_init(&p);
	js = "[\"a\":{},\"b\":{}]";
	r = jsnn_parse(&p, js, tokens, 10);
	check(r == JSNN_SUCCESS);

	jsnn_init(&p);
	js = "{\n \"Day\": 26,\n \"Month\": 9,\n \"Year\": 12\n }";
	r = jsnn_parse(&p, js, tokens, 10);
	check(r == JSNN_SUCCESS);


	return 0;
}

int test_primitive() {
	int r;
	jsnn_parser p;
	jsnntok_t tok[10];
	const char *js;
#ifndef JSNN_STRICT
	js = "\"boolVar\" : true";
	jsnn_init(&p);
	r = jsnn_parse(&p, js, tok, 10);
	check(r == JSNN_SUCCESS && tok[0].type == JSNN_STRING 
			&& tok[1].type == JSNN_PRIMITIVE);
	check(TOKEN_STRING(js, tok[0], "boolVar"));
	check(TOKEN_STRING(js, tok[1], "true"));

	js = "\"boolVar\" : false";
	jsnn_init(&p);
	r = jsnn_parse(&p, js, tok, 10);
	check(r == JSNN_SUCCESS && tok[0].type == JSNN_STRING 
			&& tok[1].type == JSNN_PRIMITIVE);
	check(TOKEN_STRING(js, tok[0], "boolVar"));
	check(TOKEN_STRING(js, tok[1], "false"));

	js = "\"intVar\" : 12345";
	jsnn_init(&p);
	r = jsnn_parse(&p, js, tok, 10);
	check(r == JSNN_SUCCESS && tok[0].type == JSNN_STRING 
			&& tok[1].type == JSNN_PRIMITIVE);
	check(TOKEN_STRING(js, tok[0], "intVar"));
	check(TOKEN_STRING(js, tok[1], "12345"));

	js = "\"floatVar\" : 12.345";
	jsnn_init(&p);
	r = jsnn_parse(&p, js, tok, 10);
	check(r == JSNN_SUCCESS && tok[0].type == JSNN_STRING 
			&& tok[1].type == JSNN_PRIMITIVE);
	check(TOKEN_STRING(js, tok[0], "floatVar"));
	check(TOKEN_STRING(js, tok[1], "12.345"));

	js = "\"nullVar\" : null";
	jsnn_init(&p);
	r = jsnn_parse(&p, js, tok, 10);
	check(r == JSNN_SUCCESS && tok[0].type == JSNN_STRING 
			&& tok[1].type == JSNN_PRIMITIVE);
	check(TOKEN_STRING(js, tok[0], "nullVar"));
	check(TOKEN_STRING(js, tok[1], "null"));
#endif
	return 0;
}

int test_string() {
	int r;
	jsnn_parser p;
	jsnntok_t tok[10];
	const char *js;

	js = "\"strVar\" : \"hello world\"";
	jsnn_init(&p);
	r = jsnn_parse(&p, js, tok, 10);
	check(r == JSNN_SUCCESS && tok[0].type == JSNN_STRING 
			&& tok[1].type == JSNN_STRING);
	check(TOKEN_STRING(js, tok[0], "strVar"));
	check(TOKEN_STRING(js, tok[1], "hello world"));

	js = "\"strVar\" : \"escapes: \\/\\r\\n\\t\\b\\f\\\"\\\\\"";
	jsnn_init(&p);
	r = jsnn_parse(&p, js, tok, 10);
	check(r == JSNN_SUCCESS && tok[0].type == JSNN_STRING 
			&& tok[1].type == JSNN_STRING);
	check(TOKEN_STRING(js, tok[0], "strVar"));
	check(TOKEN_STRING(js, tok[1], "escapes: \\/\\r\\n\\t\\b\\f\\\"\\\\"));

	js = "\"strVar\" : \"\"";
	jsnn_init(&p);
	r = jsnn_parse(&p, js, tok, 10);
	check(r == JSNN_SUCCESS && tok[0].type == JSNN_STRING 
			&& tok[1].type == JSNN_STRING);
	check(TOKEN_STRING(js, tok[0], "strVar"));
	check(TOKEN_STRING(js, tok[1], ""));

	return 0;
}

int test_partial_string() {
	int r;
	jsnn_parser p;
	jsnntok_t tok[10];
	const char *js;

	jsnn_init(&p);
	js = "\"x\": \"va";
	r = jsnn_parse(&p, js, tok, 10);
	check(r == JSNN_ERROR_PART && tok[0].type == JSNN_STRING);
	check(TOKEN_STRING(js, tok[0], "x"));
	check(p.toknext == 1);

	js = "\"x\": \"valu";
	r = jsnn_parse(&p, js, tok, 10);
	check(r == JSNN_ERROR_PART && tok[0].type == JSNN_STRING);
	check(TOKEN_STRING(js, tok[0], "x"));
	check(p.toknext == 1);

	js = "\"x\": \"value\"";
	r = jsnn_parse(&p, js, tok, 10);
	check(r == JSNN_SUCCESS && tok[0].type == JSNN_STRING
			&& tok[1].type == JSNN_STRING);
	check(TOKEN_STRING(js, tok[0], "x"));
	check(TOKEN_STRING(js, tok[1], "value"));

	js = "\"x\": \"value\", \"y\": \"value y\"";
	r = jsnn_parse(&p, js, tok, 10);
	check(r == JSNN_SUCCESS && tok[0].type == JSNN_STRING
			&& tok[1].type == JSNN_STRING && tok[2].type == JSNN_STRING
			&& tok[3].type == JSNN_STRING);
	check(TOKEN_STRING(js, tok[0], "x"));
	check(TOKEN_STRING(js, tok[1], "value"));
	check(TOKEN_STRING(js, tok[2], "y"));
	check(TOKEN_STRING(js, tok[3], "value y"));

	return 0;
}

int test_unquoted_keys() {
#ifndef JSNN_STRICT
	int r;
	jsnn_parser p;
	jsnntok_t tok[10];
	const char *js;

	jsnn_init(&p);
	js = "key1: \"value\"\nkey2 : 123";

	r = jsnn_parse(&p, js, tok, 10);
	check(r == JSNN_SUCCESS && tok[0].type == JSNN_PRIMITIVE
			&& tok[1].type == JSNN_STRING && tok[2].type == JSNN_PRIMITIVE
			&& tok[3].type == JSNN_PRIMITIVE);
	check(TOKEN_STRING(js, tok[0], "key1"));
	check(TOKEN_STRING(js, tok[1], "value"));
	check(TOKEN_STRING(js, tok[2], "key2"));
	check(TOKEN_STRING(js, tok[3], "123"));
#endif
	return 0;
}

int test_partial_array() {
	int r;
	jsnn_parser p;
	jsnntok_t tok[10];
	const char *js;

	jsnn_init(&p);
	js = "  [ 1, true, ";
	r = jsnn_parse(&p, js, tok, 10);
	check(r == JSNN_ERROR_PART && tok[0].type == JSNN_ARRAY 
			&& tok[1].type == JSNN_PRIMITIVE && tok[2].type == JSNN_PRIMITIVE);

	js = "  [ 1, true, [123, \"hello";
	r = jsnn_parse(&p, js, tok, 10);
	check(r == JSNN_ERROR_PART && tok[0].type == JSNN_ARRAY 
			&& tok[1].type == JSNN_PRIMITIVE && tok[2].type == JSNN_PRIMITIVE
			&& tok[3].type == JSNN_ARRAY && tok[4].type == JSNN_PRIMITIVE);

	js = "  [ 1, true, [123, \"hello\"]";
	r = jsnn_parse(&p, js, tok, 10);
	check(r == JSNN_ERROR_PART && tok[0].type == JSNN_ARRAY 
			&& tok[1].type == JSNN_PRIMITIVE && tok[2].type == JSNN_PRIMITIVE
			&& tok[3].type == JSNN_ARRAY && tok[4].type == JSNN_PRIMITIVE
			&& tok[5].type == JSNN_STRING);
	/* check child nodes of the 2nd array */
	check(tok[3].size == 2);

	js = "  [ 1, true, [123, \"hello\"]]";
	r = jsnn_parse(&p, js, tok, 10);
	check(r == JSNN_SUCCESS && tok[0].type == JSNN_ARRAY 
			&& tok[1].type == JSNN_PRIMITIVE && tok[2].type == JSNN_PRIMITIVE
			&& tok[3].type == JSNN_ARRAY && tok[4].type == JSNN_PRIMITIVE
			&& tok[5].type == JSNN_STRING);
	check(tok[3].size == 2);
	check(tok[0].size == 3);
	return 0;
}

int test_array_nomem() {
	int i;
	int r;
	jsnn_parser p;
	jsnntok_t toksmall[10], toklarge[10];
	const char *js;

	js = "  [ 1, true, [123, \"hello\"]]";

	for (i = 0; i < 6; i++) {
		jsnn_init(&p);
		memset(toksmall, 0, sizeof(toksmall));
		memset(toklarge, 0, sizeof(toklarge));
		r = jsnn_parse(&p, js, toksmall, i);
		check(r == JSNN_ERROR_NOMEM);

		memcpy(toklarge, toksmall, sizeof(toksmall));

		r = jsnn_parse(&p, js, toklarge, 10);
		check(r == JSNN_SUCCESS);

		check(toklarge[0].type == JSNN_ARRAY && toklarge[0].size == 3);
		check(toklarge[3].type == JSNN_ARRAY && toklarge[3].size == 2);
	}
	return 0;
}

int test_objects_arrays() {
	int i;
	int r;
	jsnn_parser p;
	jsnntok_t tokens[10];
	const char *js;

	js = "[10}";
	jsnn_init(&p);
	r = jsnn_parse(&p, js, tokens, 10);
	check(r == JSNN_ERROR_INVAL);

	js = "[10]";
	jsnn_init(&p);
	r = jsnn_parse(&p, js, tokens, 10);
	check(r == JSNN_SUCCESS);

	js = "{\"a\": 1]";
	jsnn_init(&p);
	r = jsnn_parse(&p, js, tokens, 10);
	check(r == JSNN_ERROR_INVAL);

	js = "{\"a\": 1}";
	jsnn_init(&p);
	r = jsnn_parse(&p, js, tokens, 10);
	check(r == JSNN_SUCCESS);

	return 0;
}

int main() {
    test(test_cmp, "test convenience get and cmp functions");
    test(test_deep, "test a \"deeply\" nested JSON object");
	test(test_empty, "general test for a empty JSON objects/arrays");
	test(test_simple, "general test for a simple JSON string");
	test(test_primitive, "test primitive JSON data types");
	test(test_string, "test string JSON data types");
	test(test_partial_string, "test partial JSON string parsing");
	test(test_partial_array, "test partial array reading");
	test(test_array_nomem, "test array reading with a smaller number of tokens");
	test(test_unquoted_keys, "test unquoted keys (like in JavaScript)");
	test(test_objects_arrays, "test objects and arrays");
	printf("\nPASSED: %d\nFAILED: %d\n", test_passed, test_failed);
	return 0;
}

