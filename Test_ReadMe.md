## 사전 준비 사항
- `text-test.js, interactive-test.js, debug-test.js, k6_visualize.py` 를 `/gdbc` 디렉토리로 옮겨야 합니다.
- Ubuntu 24.04에서는 Python 패키지를 시스템 전역에 직접 설치하는 것을 권장하지 않습니다. 
- 대신 가상 환경을 만들어서 사용하는 것이 좋습니다. 다음 단계를 따라 설정해보겠습니다:

1. 필요한 패키지 설치:
```bash
sudo apt install python3-full python3-venv
```

2. 가상 환경 생성:
```bash
# 프로젝트 디렉토리에서
python3 -m venv venv
```

3. 가상 환경 활성화:
```bash
source venv/bin/activate
```

4. 필요한 패키지 설치:
```bash
pip install requests matplotlib pandas
```

5. 스크립트 실행:
```bash
python k6_visualize.py your.json
```

6. 작업 완료 후 가상 환경 비활성화:
```bash
deactivate
```

가상 환경을 사용하면 다음과 같은 이점이 있습니다:
- 시스템 Python 환경과 분리
- 프로젝트별 의존성 관리 용이
- 패키지 버전 충돌 방지
- 쉬운 환경 재생성

이렇게 하면 이전에 공유했던 테스트 스크립트를 안전하게 실행할 수 있습니다.

### text-mode 테스트 방법
```bash
#1
k6 run --summary-export=text_mode.json text-test.js
#2
python3 k6_visualize.py text_mode.json
```

### interactive-mode 테스트 방법
```bash
#1
k6 run --summary-export=interactive_mode.json interactive-test.js
#2
python3 k6_visualize.py interactive_mode.json
```
   
### debug-mode 테스트 방법
```bash
#1
k6 run --summary-export=debug_mode.json debug-test.js
#2
python3 k6_visualize.py debug_mode.json
```
