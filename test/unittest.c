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

// 테스트 대상 (실제로는 라이브러리에 있다고 가정)
int add(int a, int b) {
    return a + b;
}
// 테스트 함수 - 실제 테스트를 수행하는 함수
void test_add(void) {
    CU_ASSERT(add(2, 3) == 5);
    CU_ASSERT(add(-1, 1) == 0);
    CU_ASSERT(add(-2, -3) != -5);
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

    if (NULL == CU_add_test(suite, "test of add()", test_add)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}