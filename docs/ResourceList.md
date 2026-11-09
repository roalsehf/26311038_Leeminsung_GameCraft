# 3스테이지 캠페인 리소스 목록

## 스테이지와 전투

- 사막 진입로: 제공받은 사막 파노라마와 모래 지형, 장갑 지휘차.
- 철제 보급기지: 석양 화물기지 배경, 콘크리트 바닥·고가 발판, 드론과 공성포.
- 야간 요새: 달빛 요새 배경, 콘크리트와 암벽, 드론과 로켓 장갑차.

스테이지 배치, 보급품, 적, 클리어 위치는 `GameStage.cpp`의 스테이지 데이터에서 관리합니다. `GameSession.cpp`는 입력과 스테이지 상태를 받아 게임을 초당 120회 갱신합니다. `GameSessionCombat.cpp`는 적의 발사 계획과 피해 판정을 처리하며, 충돌 계산은 `GameCombat.h`에 있습니다. `GameView.cpp`는 월드, HUD, 메뉴를 그립니다. 보스의 경고 표시와 발사는 같은 탄환 계획을 사용합니다.

## 이미지

RunAndGun_Desert_Assets.zip의 원본 리소스는 resource/RunAndGun_Desert_Assets/에 보관합니다. 배경·지형·소품·병사·기본 포탑·장갑차·효과·보급품·HUD에서 필요한 이미지를 로드합니다. 달리기와 착지에는 원본 `sand_dust.png`를 사용합니다.

| 파일 | 용도 |
| --- | --- |
| resource/player/soldier_actions_v2.png | 동일 병사의 대기·8프레임 이동·정면 사격·앉기·점프·투척·사망 |
| resource/campaign/vertical_aim.png | 동일 병사의 위 조준 3종과 공중 아래 조준, 1254 × 1254 RGBA |
| resource/campaign/depot_fortress.png | 보급기지와 야간 요새의 파노라마, 1536 × 1024 RGB |
| resource/campaign/machines.png | 드론 2자세, 고가 공성포와 로켓 장갑차, 1254 × 1254 RGBA |

보강 이미지는 내장 image_gen으로 제작했습니다. 신규 이미지의 프롬프트와 출처는 docs/CampaignArt.md, 기존 통합 캐릭터는 docs/PlayerAnimationSource.md에 기록했습니다. 위·아래 탄환 및 섬광은 게임 코드에서 비행 벡터에 맞춰 그립니다.

## 사운드

| 사용 구간 | 원본 | 재생용 파일 |
| --- | --- | --- |
| 세 스테이지의 일반 구간 | 사용자가 제공한 Negev Desert Loop.wav | resource/sound/negev_desert_loop.wav |
| 각 스테이지의 보스 교전 | 사용자가 제공한 Negev Fight Loop.wav | resource/sound/negev_fight_loop.wav |

두 BGM은 44.1 kHz PCM16 스테레오 WAV입니다. 원본은 보존하고, 재생용 복사본은 효과음과 균형을 맞춰 음량을 조정했습니다. 반복 경계의 클릭을 줄이기 위해 처음과 끝 4ms만 페이드 처리했으며, 재생 길이는 그대로입니다.
사격·폭발·피격·보급품·클리어·점프 효과음은 기존 PCM16 모노 WAV를 사용합니다. 이전 사막 바람 반복음은 보관용으로 남깁니다.

## 배포

GameResources의 실제 로드 경로를 기준으로 tools/audit_resources.py가 docs/ResourceManifest.json을 생성합니다. 파일 존재, 이미지 헤더, WAV 형식과 SHA256을 검사합니다.

tools/package_release.ps1은 Release / x64를 빌드하고 실제 사용하는 이미지·사운드, EXE, DirectX DLL, 실행 안내를 dist/GameCraft-3Stages.zip에 담습니다. 미사용 원본과 빌드 중간 파일은 ZIP에 포함하지 않습니다.
