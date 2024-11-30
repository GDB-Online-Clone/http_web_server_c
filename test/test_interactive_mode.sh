#!/bin/bash

# test_interactive_mode.sh
BASE_URL="http://localhost:10010"

echo "1. 유효한 요청 테스트..."
curl -v -X POST "$BASE_URL/run/interactive-mode?language=c" \
  -d '#include <stdio.h>
int main() {
    printf("Hello World\n");
    return 0;
}'
echo -e "\n"

echo "2. language 파라미터 누락 테스트..."
curl -v -X POST "$BASE_URL/run/interactive-mode" \
  -d '#include <stdio.h>
int main() {
    printf("Hello World\n");
    return 0;
}'
echo -e "\n"

echo "3. 빈 본문 테스트..."
curl -v -X POST "$BASE_URL/run/interactive-mode?language=c" \
  -d ''
echo -e "\n"

echo "4. 선택적 파라미터 포함 테스트..."
curl -v -X POST "$BASE_URL/run/interactive-mode?language=c&compile_option=-O2&argument=--verbose" \
  -d '#include <stdio.h>
int main() {
    printf("Hello World\n");
    return 0;
}'
echo -e "\n"

echo "5. 파이썬 코드 테스트..."
curl -v -X POST "$BASE_URL/run/interactive-mode?language=python" \
  -d 'print("Hello World")'
echo -e "\n"

echo "6. 선택적 파라미터만 있는 테스트..."
curl -v -X POST "$BASE_URL/run/interactive-mode?compile_option=-O2" \
  -d '#include <stdio.h>
int main() {
    printf("Hello World\n");
    return 0;
}'
echo -e "\n"