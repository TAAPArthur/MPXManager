#include "UnitTests.h"
#include "../../bindings.h"

START_TEST(test_literal_string_rule){
    Rules r=CREATE_LITERAL_RULE("A",LITERAL,NULL);
    assert(doesStringMatchRule(&r, "A"));
    assert(doesStringMatchRule(&r, "a")==0);
    assert(doesStringMatchRule(&r, "b")==0);
    Rules r2=CREATE_LITERAL_RULE("A",LITERAL|CASE_INSENSITIVE,NULL);
    assert(doesStringMatchRule(&r2, "A"));
    assert(doesStringMatchRule(&r2, "a"));
    assert(doesStringMatchRule(&r2, "b")==0);

}END_TEST
START_TEST(test_string_rule){
    Rules r=CREATE_RULE(".*",MATCH_ANY_REGEX,NULL);
    COMPILE_RULE((&r));
    assert(doesStringMatchRule(&r, "A"));
    assert(doesStringMatchRule(&r, "B"));
    Rules r2=CREATE_RULE("A",MATCH_ANY_REGEX,NULL);
    COMPILE_RULE((&r2));
    assert(doesStringMatchRule(&r2, "A"));
    assert(doesStringMatchRule(&r2, "a"));
    assert(doesStringMatchRule(&r2, "B")==0);
    int target=(MATCH_ANY_REGEX) - (CASE_INSENSITIVE);
    Rules r3=CREATE_RULE("A",target,NULL);
    COMPILE_RULE((&r3));
    assert(doesStringMatchRule(&r3, "A"));
    assert(doesStringMatchRule(&r3, "a")==0);
    assert(doesStringMatchRule(&r3, "B")==0);
}END_TEST
Suite *bindingsSuite(void) {
    Suite*s = suite_create("Context");

    TCase*tc_core = tc_core = tcase_create("Context");
    tcase_add_test(tc_core, test_literal_string_rule);
    tcase_add_test(tc_core, test_string_rule);
    suite_add_tcase(s, tc_core);

    return s;
}
