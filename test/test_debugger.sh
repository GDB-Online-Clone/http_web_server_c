#!/bin/bash

# test_debugger.sh
BASE_URL="http://localhost:10010"

echo "1. 유효한 요청 테스트..."
curl -v -X POST "$BASE_URL/run/debugger?language=c" \
  -H "Content-Type: application/json" \
  -d '{"code": "int main() { return 0; }"}'
echo -e "\n"

echo "2. language 파라미터 누락 테스트..."
curl -v -X POST "$BASE_URL/run/debugger" \
  -H "Content-Type: application/json" \
  -d '{"code": "int main() { return 0; }"}'
echo -e "\n"

echo "3. Content-Type 헤더 누락 테스트..."
curl -v -X POST "$BASE_URL/run/debugger?language=c" \
  -d '{"code": "int main() { return 0; }"}'
echo -e "\n"

echo "4. 잘못된 Content-Type 테스트..."
curl -v -X POST "$BASE_URL/run/debugger?language=c" \
  -H "Content-Type: text/plain" \
  -d '{"code": "int main() { return 0; }"}'
echo -e "\n"

echo "5. 선택적 파라미터 포함 테스트..."
curl -v -X POST "$BASE_URL/run/debugger?language=c&compile_option=-g&argument=--debug" \
  -H "Content-Type: application/json" \
  -d '{"code": "int main() { return 0; }"}'
echo -e "\n"