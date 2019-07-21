#include "../bindings.h"
#include "../devices.h"
#include "../globals.h"
#include "../windows.h"
#include "../filter.h"
#include "test-mpx-helper.h"
#include "test-event-helper.h"
#include "tester.h"

MPX_TEST_ITER("test_literal_string_rule", 1, {
    const char* var;
    int target = LITERAL;
    if(_i) {
        var = "$random_name";
        const char* value = "A";
        setenv(var + 1, value, 1);
        target |= ENV_VAR;
    }
    else var = "A";
    Filter r = {var, target | CASE_SENSITIVE | LITERAL};
    assert(r.doesStringMatch("A"));
    assert(r.doesStringMatch("random_text_A") == 0);
    assert(r.doesStringMatch("A_random_text") == 0);
    assert(r.doesStringMatch("random_text_A_random_text") == 0);
    assert(r.doesStringMatch("a") == 0);
    assert(r.doesStringMatch("b") == 0);
    Filter r2 = {var, target | LITERAL};
    assert(r2.doesStringMatch("A"));
    assert(r2.doesStringMatch("a"));
    assert(r2.doesStringMatch("b") == 0);
});
MPX_TEST("test_env_rule_sub", {
    int target = LITERAL | ENV_VAR;
    const char* var = "$_var";
    const char* var2 = "$_var2";
    const char* value = "A";
    const char* value2 = "ABCDEFGHI";
    const char* str[] = {"echo '$_var'", "$_var", "$_var text $_var", "\\$var", "$_var2 long long text", "$_unsetVar"};
    std::string subVar[] = {"echo 'A'", "A", "A text A", "$var", "ABCDEFGHI long long text", ""};
    setenv(var + 1, value, 1);
    setenv(var2 + 1, value2, 1);
    unsetenv("_unsetVar");
    for(int i = 0; i < LEN(str); i++) {
        Filter r = {str[i], target | LITERAL};
        assertEquals(r.getStr(), subVar[i]);
    }
    char longValue[255];
    for(int i = 0; i < 255; i++)longValue[i] = 'A';
    longValue[LEN(longValue) - 1] = 0;
    setenv(var + 1, longValue, 1);
    Filter r = {var, target | LITERAL};
    assert(strcmp(r.getStr().c_str(), longValue) == 0);
});
MPX_TEST("test_string_rule", {
    Filter r = {".*", MATCH_ANY_REGEX};
    assertEquals(MATCH_ANY_REGEX & LITERAL, 0);
    assert(r.doesStringMatch(".*"));
    assert(r.doesStringMatch("A"));
    assert(r.doesStringMatch("B"));
    Filter r2 = {"A", MATCH_ANY_REGEX};
    assert(r2.doesStringMatch("A"));
    assert(r2.doesStringMatch("a"));
    assert(r2.doesStringMatch("B") == 0);
    Filter r3 = {"A", (MATCH_ANY_REGEX | CASE_SENSITIVE)};
    assert(r3.doesStringMatch("A"));
    assert(r.doesStringMatch("A"));
    assert(r.doesStringMatch("B"));
    r2 = {"A", MATCH_ANY_REGEX};
    assert(r2.doesStringMatch("A"));
    assert(r2.doesStringMatch("a"));
    assert(r2.doesStringMatch("B") == 0);
    r3 = {"A", (MATCH_ANY_REGEX | CASE_SENSITIVE)};
    assert(r3.doesStringMatch("A"));
    assert(r3.doesStringMatch("a") == 0);
    assert(r3.doesStringMatch("B") == 0);
});

MPX_TEST_ITER("test_window_matching", 4, {
    WindowInfo* winInfo = new WindowInfo(1);
    std::string strings[] = {"title", "class", "instance", "type"};
    int masks[] = {TITLE, CLASS, RESOURCE, TYPE};
    winInfo->setTitle(strings[0]);
    winInfo->setClassName(strings[1]);
    winInfo->setInstanceName(strings[2]);
    winInfo->setTypeName(strings[3]);
    Filter f = Filter(strings[_i], masks[_i]);
    for(int i = 0; i < LEN(strings); i++) {
        assertEquals(f(NULL), 1);
    }
    for(int i = 0; i < LEN(strings); i++) {
        assertEquals(f(winInfo), i == _i);
    }
});

