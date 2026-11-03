# 배경 사운드 출처

`desert_wind.wav`는 이 프로젝트를 위해 직접 합성한 사막 바람 배경음입니다.
외부 녹음, 기존 음악, 게임 사운드 샘플은 사용하지 않았습니다.

- 용도: 게임 실행 중 반복 재생하는 바람 환경음
- 형식: WAV, 16-bit PCM, 모노, 22,050 Hz
- 길이: 24초
- 생성 방법: 난수 잡음을 대역별로 필터링하고 느린 바람 세기 변화를 적용
- 반복 처리: 필터와 바람 세기를 주기적으로 구성하고, 조용한 연속 지점을 반복 경계로 선택
- 음량: 약 -21 dBFS RMS, 최대 피크 -6.9 dBFS 이하

프로젝트의 `tools/generate_desert_wind.py`를 Python 3으로 실행하면 같은 파일을 다시 생성할 수 있습니다. Python 표준 라이브러리만 사용합니다.

추가 효과음은 tools/generate_effects.py로 합성한 원본 PCM16 모노 사운드입니다.
shot.wav(사격), explosion.wav(폭발), hurt.wav(피격), pickup.wav(획득), clear.wav(클리어), jump.wav(점프)를 사용합니다.
외부 녹음이나 음악을 사용하지 않았으며 Python 표준 라이브러리로 재생성할 수 있습니다. 게임 실행에는 Python이 필요하지 않습니다.
