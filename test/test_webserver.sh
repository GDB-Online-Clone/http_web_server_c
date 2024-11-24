#!/bin/bash

SERVER_URL="http://localhost:10010/hello-world"

echo "Starting tests on $SERVER_URL"

# Basic GET Test
echo -e "\n[TEST] Basic GET Request"
curl -v "$SERVER_URL"

# Query Parameter Test
echo -e "\n[TEST] GET Request with Query Parameters"
curl -v "$SERVER_URL?key1=value1&key2=value2"

# POST Request Test
echo -e "\n[TEST] POST Request with Form Data"
curl -v -X POST "$SERVER_URL" -d "name=John&age=30"

# POST JSON Test
echo -e "\n[TEST] POST Request with JSON Data"
curl -v -X POST "$SERVER_URL" -H "Content-Type: application/json" -d '{"name":"John", "age":30}'

# PUT Request Test
echo -e "\n[TEST] PUT Request"
curl -v -X PUT "$SERVER_URL" -d "update=data"

# Custom Header Test
echo -e "\n[TEST] Request with Custom Header"
curl -v -H "X-Custom-Header: myvalue" "$SERVER_URL"

# DELETE Request Test
echo -e "\n[TEST] DELETE Request"
curl -v -X DELETE "$SERVER_URL"

# Large Data POST Test
echo -e "\n[TEST] POST Request with Large File"
curl -v -X POST "$SERVER_URL" --data-binary @..src/http.c

# 404 Not Found Test
echo -e "\n[TEST] 404 Not Found Request"
curl -v http://localhost:10010/nonexistent

# Invalid HTTP Request Test
echo -e "\n[TEST] Invalid HTTP Request"
echo -e "INVALID REQUEST\r\n\r\n" | nc localhost 10010

# Keep-Alive Test
echo -e "\n[TEST] Keep-Alive Connection"
curl -v "$SERVER_URL" --header "Connection: keep-alive"

# Chunked Transfer-Encoding Test
echo -e "\n[TEST] Chunked Transfer-Encoding"
curl -v -X POST "$SERVER_URL" -H "Transfer-Encoding: chunked" --data-binary @../src/http.c

echo -e "\nAll tests completed."