#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <webserver/utility.h>
// Setup 함수 - 유닛 테스트 환경 설정에 사용
int setup(void) {
    // 필요에 따라 초기화 작업 추가
    return 0; // 성공 시 0 반환
}
// Teardown 함수 - 유닛 테스트 환경 정리에 사용
int teardown(void) {
    // 필요에 따라 정리 작업 추가
    return 0; // 성공 시 0 반환
}


// unit test 예시
void test_example(void) {
    CU_ASSERT(2 + 5 == 7);
    CU_ASSERT(1 + -1 == 0);
    CU_ASSERT(-2 + -3 == -5);
}


int main() {
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }
    CU_pSuite suite = CU_add_suite("Console Test Suite", setup, teardown);
    if (NULL == suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    if (NULL == CU_add_test(suite, "test of test_example()", test_example)) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    int failed_cnt = CU_get_number_of_tests_failed();
    CU_cleanup_registry();
    return failed_cnt;
}