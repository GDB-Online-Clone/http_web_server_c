#!/bin/bash

# test_run_text_mode.sh
BASE_URL="http://localhost:10010"

echo "1. 유효한 요청 테스트..."
curl -v -X POST "$BASE_URL/run/text-mode?language=c" \
 -H "Content-Type: application/json" \
 -d '{"source_code": "print(\"Hello World\")"}'
echo -e "\n"

echo "2. language 파라미터 누락 테스트..."
curl -v -X POST "$BASE_URL/run/text-mode" \
 -H "Content-Type: application/json" \
 -d '{"source_code": "print(\"Hello World\")"}'
echo -e "\n"

echo "3. Content-Type 헤더 누락 테스트..."
curl -v -X POST "$BASE_URL/run/text-mode?language=c" \
 -d '{"source_code": "print(\"Hello World\")"}'
echo -e "\n"

echo "4. 잘못된 Content-Type 테스트..."
curl -v -X POST "$BASE_URL/run/text-mode?language=c" \
 -H "Content-Type: text/plain" \
 -d '{"source_code": "print(\"Hello World\")"}'
echo -e "\n"

echo "5. 선택적 파라미터 포함 테스트..."
curl -v -X POST "$BASE_URL/run/text-mode?language=c&compile_option=-O2&argument=--verbose" \
 -H "Content-Type: application/json" \
 -d '{"source_code": "print(\"Hello World\")"}'
echo -e "\n"

echo "6. 빈 요청 본문 테스트..."
curl -v -X POST "$BASE_URL/run/text-mode?language=c" \
 -H "Content-Type: application/json" \
 -d '{}'
echo -e "\n"