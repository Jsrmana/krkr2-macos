# macOS arm64 빌드와 사용

## 사용

최종 검증 앱은 `krkr2-audio-fix.app`이며 이전 창 조절·메뉴·글꼴 수정을 포함합니다.
게임 데이터는 별도로 준비합니다. GitHub 소스에서 빌드하면 결과 이름은 `krkr2.app`입니다.
이 앱은 로컬 ad-hoc 서명된 Debug 빌드이며 Apple 개발자 인증서 서명이나 공증을 받은 배포판은 아닙니다.

- 창 가장자리나 모서리를 드래그하면 크기가 바뀝니다. 최소 콘텐츠 크기는 480×320입니다.
- 초록색 버튼 또는 Control + Command + F로 전체화면을 전환합니다.
- 게임이 `Window.fullScreen`을 사용하는 경우 게임 내부 전체화면 설정도 연동됩니다.
- 화면 비율을 유지하고 남는 공간에는 여백을 표시합니다.
- 전체화면을 종료하거나 앱을 다시 실행하면 창 모드의 크기와 위치를 복원합니다.

## 새 환경에서 빌드

Apple Silicon macOS에서 Xcode 또는 Command Line Tools와 Git을 준비합니다.
Command Line Tools가 없다면 `xcode-select --install`로 설치합니다.
CMake 3.31.1 이상(검증 버전 3.31.8), Ninja, Bison 3.8.2 이상, Python 3, NASM, YASM, pkg-config가 PATH에 있어야 합니다.
Homebrew를 사용한다면 해당 도구들을 설치하고 Homebrew Bison의 bin 디렉터리를 PATH 앞에 둡니다.

```sh
git clone https://github.com/Jsrmana/krkr2-macos.git
cd krkr2-macos
git clone https://github.com/microsoft/vcpkg.git ../vcpkg
# 이번 작업에서 사용한 vcpkg revision
git -C ../vcpkg checkout 04a9d8e5212d01ee1dd9478eadd9caade4f8b0d4
../vcpkg/bootstrap-vcpkg.sh -disableMetrics
export VCPKG_ROOT="$(cd ../vcpkg && pwd)"
export CMAKE_BUILD_PARALLEL_LEVEL=6
export VCPKG_MAX_CONCURRENCY=6
cmake --preset="MacOS Debug Config" -DVCPKG_MANIFEST_INSTALL=ON
cmake --build --preset="MacOS Debug Build"
ctest --test-dir out/macos/debug --output-on-failure --timeout 60
codesign --force --sign - out/macos/debug/bin/krkr2/krkr2.app
codesign --verify --deep --strict out/macos/debug/bin/krkr2/krkr2.app
open out/macos/debug/bin/krkr2/krkr2.app
```

결과물은 `out/macos/debug/bin/krkr2/krkr2.app`입니다.
첫 빌드에서는 vcpkg가 의존성을 내려받고 컴파일하므로 시간이 걸립니다.
도구 및 vcpkg 설치 경로는 사용자의 환경에 맞게 지정합니다.

## 기존 작업 폴더에서 재빌드

기존 작업 폴더의 `work/env.sh`와 `outputs/rebuild-window.command`는 설치된 로컬 도구와 캐시를 재사용합니다.
이 스크립트의 절대 경로는 기존 컴퓨터용이므로 GitHub에서 새로 복제한 폴더에는 위의 새 환경 절차를 사용합니다.

창 기능 빌드 때 vcpkg의 컴파일러 식별값이 달라져 의존성 재빌드가 요청되었습니다.
이전 성공 빌드의 arm64 의존성 캐시를 복원하고 `VCPKG_MANIFEST_INSTALL=OFF`로 현재 소스를 다시 빌드했습니다.
이는 의존성이 준비된 기존 작업 폴더만을 위한 조치입니다. 새 환경에서는 ON을 사용합니다.

## 범위와 출처

GitHub 업로드본에서는 재배포 권한을 확인하지 못한 원본 이미지·모션 샘플
(`tests/test_files/emote`)과 해당 샘플을 사용하는 테스트 소스를 전체 업로드 이력에서 제외했습니다.
해당 자료가 있는 로컬 작업본의 테스트는 12개이며, 업로드본에서는 자료 의존 테스트 2개를
등록하지 않아 나머지 10개를 실행합니다. 자체 작성한 TJS·shrinkCopy·스레드 테스트는 포함합니다.
원본 및 변경 전 이력은 로컬에 보존했고, 정리된 업로드 이력의 커밋 해시는 로컬 원본과 다릅니다.

제공된 한 게임에서 창 조절, 버튼 입력, 게임 설정을 통한 전체화면 진입·복귀를 확인했습니다.
모든 게임이나 모든 KiriKiri 플러그인의 호환성을 보장하지 않습니다.

원본 엔진: https://github.com/2468785842/krkr2

shrinkCopy 원본: https://github.com/wtnbgo/shrinkCopy

라이선스는 저장소의 `LICENSE`와 각 구성 요소의 고지를 참고합니다.
shrinkCopy의 출처와 변경 사항은 `cpp/plugins/shrinkCopy/PORTING.md`에 기록되어 있습니다.
