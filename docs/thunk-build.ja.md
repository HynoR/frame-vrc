# Vulkan thunk のビルド（`thunk` ビルド）

[English](thunk-build.md) · [中文](thunk-build.zh-CN.md) · **日本語**

`thunk` ビルドは、`eac` ビルドに同じ FEX ソースからビルドした Vulkan thunk（2 つのファイル）を加えたものです：

- `usr/lib/aarch64-linux-gnu/fex-emu/HostThunks/libvulkan-host.so`（aarch64、ネイティブで動作）
- `usr/share/fex-emu/GuestThunks/libvulkan-guest.so`（x86-64、エミュレーション中のゲームが `libvulkan.so.1` の代わりに読み込む）

ほとんどの人は配布されている `thunk` ビルドを `frame-vrc fetch` すれば十分です。自分でビルドするには SteamOS に入っていない
ヘッダが必要です。以下はすべて `~` の中で完結します：sudo なし、pacman でのインストールなし、システムは変わりません。
Valve のビルド済み thunk（FEX-2607）は eac 版 FEX（2610）とは組み合わせられません。

## 1. ホスト側（aarch64）

SteamOS には clang、libclang-cpp、libLLVM はありますが、LLVM のヘッダと `LLVMConfig.cmake` がありません。

1. `pacman -Sp llvm` でシステムと同じバージョンの `llvm` パッケージのダウンロード URL が表示されます（SteamOS 0.4.5 では
   19.1.7）。ダウンロードして `~/fex-eac-work/thunk-deps/llvm` に展開します（`bsdtar -xf`）。インストールはしません。
   SteamOS のパッケージミラーは非公開です。この URL はどこにも投稿せず、各ヘッドセットで調べてください。
2. パッケージの CMake エクスポートが必要とするライブラリはヘッドセットに既にあります：
   `~/fex-eac-work/thunk-deps/llvm/usr/lib/*.so*` を `/usr/lib` の同名ファイルへリンクします。
3. eac 用とは別のビルドディレクトリを作ります：

   ```sh
   cd ~/fex-eac-work/FEX
   cmake -B build-mod -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_THUNKS=ON \
     -DLLVM_DIR=$HOME/fex-eac-work/thunk-deps/llvm/usr/lib/cmake/llvm   # lib/build-fex.sh と同じオプションも追加
   ninja -C build-mod vulkan-host-64
   ```

## 2. ゲスト側（x86-64、clang でクロスコンパイル）

`/usr/share/guestos/fex-mesa` のゲスト用グラフィックスタック（SteamOS 0.4.5 では glibc 2.41、GLIBCXX_3.4.34）に合った
glibc と libstdc++ のヘッダを持つ x86-64 sysroot が必要です。合致した Arch Linux Archive のパッケージ：`glibc 2.41+r65`、
`linux-api-headers 6.14`、`gcc 15.1.1+r7`、`gcc-libs 15.1.1+r7`。

1. `~/x86-sysroot` に展開します。
2. `/usr/include` の X11、xcb、wayland のヘッダを `~/x86-sysroot/usr/include` にリンクします（アーキテクチャ非依存）。
3. ゲスト側サブプロジェクトのツールチェーンファイルには sysroot がないので、そこで設定してからビルドします：

   ```sh
   cd ~/fex-eac-work/FEX/build-mod/Guest && cmake -DCMAKE_SYSROOT=$HOME/x86-sysroot . && ninja vulkan-guest
   ```

## 3. stage に入れて切り替え

```sh
cp -a ~/fex-eac-work/stage ~/fex-eac-work/stage-mod
# ビルドした 2 つのライブラリ（先に strip）を stage-mod の同じパスに上書きしてから：
echo "thunk: eac stage + Vulkan thunk built from FEX <describe>" > ~/fex-eac-work/stage-mod/MOD.txt
frame-vrc switch thunk && frame-vrc launch-options set
frame-vrc pack thunk     # 共有する場合
```

確認：ビルドしたライブラリが要求する glibc は 2.34 以下で、依存ライブラリは Valve のものと同じであること。

## 起動オプションに `FEX_HOSTENV` がある理由

thunk を使うと、ネイティブの Vulkan ローダーが Steam の pressure-vessel コンテナ内で動きます。コンテナの `VK_DRIVER_FILES` には
`/run/gfx/main/...` の x86 版 Turnip しかなく、このパスはエミュレーション側にしか存在しないため、ネイティブのローダーは
ドライバを見つけられません。frame-vrc は `~/.local/share/frame-vrc/vk/freedreno_host.aarch64.json`（ホストの
`freedreno_icd.aarch64.json` の `library_path` を `/run/host/usr/lib/libvulkan_freedreno.so` に変えたもの）を書き、
`FEX_HOSTENV=VK_DRIVER_FILES=…` で渡します。FEX はこの変数をネイティブ側にだけ設定します。

## 既知の問題：VR

VR では、VRChat が最初のフレームを SteamVR に渡すときにクラッシュします（`IVRCompositor::Submit`、ネイティブ側でアクセス違反、
続いて `vrcompositor_manual.c:2321 Assertion failed: !status`）。"Unknown Vulkan function" はログに出ないので、関数の欠落では
ありません。おそらく構造体かハンドルが thunk の境界を変換されずに渡っています。これは FEX の thunk ABI の領域なので、FEX の
開発者に任せます。デスクトップモード（`--no-vr`）は動作します。
