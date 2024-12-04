## Http Web Server Library written in C

이 라이브러리는 Http 프로토콜을 지원하는 http web server 입니다.   
   
이 라이브러리는 다음과 같은 특징을 가집니다.
- http/1.1 스펙에서 설계되었습니다.
- 웹서버 ↔ 라우터 구조를 가집니다.
- http 프로토콜만 지원합니다.
- Json 파서는 아직 불완전합니다. (#195)
- realloc 으로 구현한 가변 길이 배열 구현체가 포함되어있습니다. 다양한 타입을 지원합니다.
- 스레드 기반 비동기 서버입니다.
- `DEBUG` 매크로가 선언되었을 시에만 작동하는 디버그 로그를 제공합니다.

## documents
- 코드 베이스 문서들은 <https://sony-string.net/GDB-Online-Clone/Http-Web-Server-C/> 여기서 확인하실 수 있습니다.

## How to build

### Prerequirement
다음과 같은 버전에서 빌드가 됨을 확인하였습니다.   
해당 프로젝트는 시스템 간 호환성이 좋지 못한 점이 있습니다.
```bash
gcc (Ubuntu 13.2.0-23ubuntu4) 13.2.

Distributor ID: Ubuntu
Description:    Ubuntu 24.04.1 LTS
Release:        24.04
Codename:       noble

5.15.167.4-microsoft-standard-WSL2 # 네이티브 Ubuntu 24.04 의 기본 커널에서도 문제 없을 것으로 생각됩니다
```

### 라이브러리 빌드
빌드하기 위해 우선 프로젝트를 클론합니다.   
```bash
git clone https://github.com/GDB-Online-Clone/http_web_server_c
```
   
웹서버 라이브러리를 빌드하기 위해서는 `http_web_server_c` 디렉토리에서 진행합니다.   
해당 디렉토리에서 `make` 를 실행하면 됩니다.
```bash
cd http_web_server_c
make
```
   
기본적으로 코드에는 디버깅 심볼과 디버그 로그가 포함되어있습니다.   
이를 제거하기 위해서는 다음과 같이 입력합니다.
```bash
make ADD=
```
   
다른 컴파일 옵션을 지정하고 싶다면 `ADD` 에 원하는 컴파일 옵션을 입력할 수 있습니다.
```bash
make ADD="Wall -DMACRO"
```

### GDB Online Clone 빌드
먼저 위의 라이브러리 빌드를 진행해야 합니다.   
이후 `gdbc` 디렉토리로 이동하여 빌드합니다.
```bash
cd gdbc
make ADD=
```
이 경우에도 동일하게 `ADD` 옵션이 있습니다.   
`make` 가 완료되었다면, `gdbc/gdb-online-clone` 이 생성됩니다.

### GDB Online Clone 서버 실행
```
./gdb-online-clone
```