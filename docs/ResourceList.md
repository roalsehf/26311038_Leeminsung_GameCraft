# 4차 구현 및 리소스 목록

## 한 판 구현

시작 화면에서 Enter로 출발하여 사막 진입로, 보급소, 통신기지를 진행합니다.
병사 6명, 포탑 3개, 마지막 장갑 보스 1개가 등장합니다. 보스를 처치하고 마지막 깃발을 통과하면 클리어입니다.
이동, 점프, 앉기, 사격, 수류탄, 충돌, 체력, 피격 무적, 보급품, 점수, 일시정지, 게임 오버, 재시작을 포함합니다.
화면은 기존 960 x 600, 월드는 가로 5120픽셀입니다. CApplication 전역 객체와 main 호출 순서를 유지했습니다.

## 이미지 분류

기준 경로는 resource/RunAndGun_Desert_Assets/입니다. 출처는 사용자가 제공한 RunAndGun_Desert_Assets.zip이며, 원본 133개 파일을 수정하지 않고 보관했습니다.

| 폴더 | 현재 사용 |
| --- | --- |
| backgrounds | desert_full_wide.png의 시차 스크롤 |
| player | 원본 동작 이미지 보관. 실제 플레이어는 별도 통합 시트 사용 |
| enemies | 병사 이동·사격·사망, 포탑 대기·사격·잔해 |
| boss | 대기·사격·피격·잔해 |
| tiles | 모래 지면, 철제 발판 좌·중·우 |
| props / scenery | 상자, 드럼통, 철조망, 무전기, 나무, 감시탑 등 |
| interactive | 마지막 깃발 |
| effects | 총알, 포탄, 수류탄, 발사·피격·폭발 표현 |
| items | 회복팩, 수류탄 보급 |
| hud | 초상화, 체력·수류탄 아이콘, 패널, 보스 체력바 |
| sheets | 제공 원본 아틀라스 보관용. 실행 패키지에서 제외 |

## 보강 리소스

| 파일 | 출처·규격·용도 |
| --- | --- |
| resource/player/soldier_actions_v2.png | 내장 image_gen으로 원본 캐릭터 참고, 1254 x 1254 RGBA, 4 x 4 셀, 모든 플레이어 자세 통일, 달리기 8프레임 75ms 간격 |
| resource/sound/desert_wind.wav | 직접 합성, 24초, 22050Hz PCM16 모노, 반복 배경음 |
| resource/sound/shot.wav | 직접 합성, 약 0.13초, 사격 |
| resource/sound/explosion.wav | 직접 합성, 0.8초, 폭발 |
| resource/sound/hurt.wav | 직접 합성, 0.26초, 피격 |
| resource/sound/pickup.wav | 직접 합성, 0.3초, 보급품 획득 |
| resource/sound/clear.wav | 직접 합성, 1.2초, 클리어 |
| resource/sound/jump.wav | 직접 합성, 0.18초, 점프 |

외부 음원을 가져오지 않았습니다. 생성 코드는 tools/generate_desert_wind.py와 tools/generate_effects.py입니다.
이미지 생성 프롬프트·참조·규격은 docs/PlayerAnimationSource.md에 기록했습니다.
사용자 제공 이미지의 이용 조건을 새로 지정하지 않았으며 원본 안내 문서를 보관합니다.

## 실제 로드 목록과 배포

GameResources.h와 GameResources.cpp에서 텍스처·사운드 경로와 핸들을 관리합니다.
초기화 시 한 번 로드하고, 종료 시 재생을 멈춘 뒤 해제합니다. PNG는 실제 알파 채널을 사용합니다.
로드 실패 시 누락된 경로를 콘솔에 표시합니다.

docs/ResourceManifest.json은 실제 C++ 로드 경로에 따른 목록입니다. 이미지 크기·SHA256과 사운드 형식·길이·피크·SHA256을 포함합니다.
tools/audit_resources.py로 파일 누락, enum 개수, PNG 헤더, PCM16 형식, 클리핑을 검사하고 목록을 갱신합니다.

tools/package_release.ps1은 Release / x64를 빌드하고 실제 사용하는 이미지·사운드, EXE, DirectX DLL, 실행 안내를 dist/GameCraft-f4.zip으로 묶습니다.
보관용 원본 아틀라스와 중간 파일은 실행 패키지에서 제외합니다. 기존 README 기획서는 수정하지 않았습니다.

