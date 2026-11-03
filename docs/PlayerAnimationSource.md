# 플레이어 동작 시트 보강 및 외형 통일

실제 사용 파일: resource/player/soldier_actions_v2.png
생성 방식: 내장 image_gen 도구. 기존 캐릭터를 참고한 새 통합 시트이며 별도 API/CLI를 사용하지 않았습니다.
이전 8프레임 달리기 전용 시트와 원본 팩의 서로 다른 플레이어 이미지는 런타임 로드 목록에서 제외했습니다.
원본 제공 팩은 수정하지 않았습니다.

## 규격

1254 x 1254 RGBA, 4열 x 4행, 16개 셀입니다. 실제 셀 경계는 이미지 크기에 1/4, 2/4, 3/4 비율을 적용한 정수 좌표입니다.
행 1: 대기 2장, 정지 사격, 수류탄 투척.
행 2~3: 소총을 겨누는 달리기 8프레임.
행 4: 점프, 앉기, 앉은 사격, 사망.
모든 자세에 공통 픽셀 배율 128/266을 적용하며, 앉은 이미지를 전체 128픽셀 높이로 늘리지 않습니다.
PlayerAnimation.h에 각 프레임의 발 기준점과 총구 좌표를 기록했습니다.
투척 셀에 그려진 공중 수류탄 영역은 렌더링에서 제외하여 실제 투사체 한 개만 나타나도록 했습니다.
HUD 초상화도 이 시트의 머리 부분을 사용합니다.

## 최종 생성 프롬프트

Production sprite atlas for a 2D side-scrolling run-and-gun game. Reference image is IDENTITY AND PIXEL-ART STYLE ONLY. Create ONE SAME SOLDIER with exactly the same face, brown spiky hair, olive headband, tan sleeveless vest, green trousers and dark boots in EVERY cell. He carries the SAME black compact assault rifle. Transparent alpha background. Exact 4 columns x 4 rows equal grid, no gaps, preferably 1024x1024, cell256x256. ALL images face RIGHT. Same camera, same head size and pixel density throughout. Standing body is 192 pixels tall, from y32 to feet y224 inside each cell; pelvis/root at x128. Crouching is genuinely SHORTER with feet still at y224, DO NOT enlarge crouching to fill cell. Boots of all poses baseline y224, including jump pose which is drawn in the air by game code. NO muzzle flash baked into images. Entire rifle muzzle stays inside the cell with margin. Exact chronology ROW 1: cell0 idle holding rifle aimed horizontally right, cell1 same idle subtle breathing, cell2 standing firing rifle aimed horizontal right with slight recoil but same scale, cell3 throwing a small grenade with one hand, same soldier and scale. ROW 2: run with rifle aimed horizontally right, first FOUR phases of an eight-pose complete running cycle: contact, down, passing, up. ROW 3: run with rifle aimed horizontally right, next FOUR phases: opposite contact, down, passing, up. Arms hold weapon aimed while legs run. ROW 4: cell0 jump with knees slightly bent and rifle aimed right, boots kept at common baseline; cell1 kneeling crouch holding rifle right, body approx half standing height, SAME head and torso scale; cell2 crouch firing slight recoil, exact same short crouch and same scale as cell1; cell3 lying dead on ground, same soldier identity and scale. Preserve the same proportions; do not redraw him as a different cartoon or larger head in any frame. No numbers, text, borders, panels, floor lines, ground, shadows, guides, background or checkerboard. Only sixteen clean full sprites on genuinely transparent pixels.
