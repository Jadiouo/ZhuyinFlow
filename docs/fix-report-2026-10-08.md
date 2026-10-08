# ZhuyinFlow 修復驗證（2026-10-08）

修復基準為 `5674dfe5174a2fcb7635575ac9b1a4f3a2953123`，上游仍固定於
`977a05fe353bcc43cffbc03ea8b6d815a072fd65`。本次未更動上游、安裝現用版本、
修改個人設定、操作桌面 Fcitx，亦未提交或推送。

## 六項問題對照

| 問題 | 根因與修正 | 驗證與結果 |
|---|---|---|
| 候選鍵盤操作與頁碼 | `VGEngine.cpp` 增加候選模式下方向鍵、PageUp/PageDown、候選模式 Space 翻頁、1–9 標籤與 Enter 選中後提交的控制；候選內容相同時保留列表、頁碼與游標。Swift JSON 增加 `candidateSelectionActive`，只在明確 `.ofCandidates` 狀態啟用，不把一般 suggestions 當選字模式。 | `fcitx5_candidates` 核對翻頁（含 Space）、key release、數字選字與提交；`fcitx5_continuous-input` 核對普通輸入 `su3` 後 Dachen 2/5/8 及 mixed `film2026` 不被誤選。候選模式另測無修飾 Escape 完整取消、不 commit、keyup 與後續重新輸入；Ctrl/Alt/Shift/Super+Escape 不觸發此取消路徑。Debug/Release 通過。 |
| 失焦或切換輸入法丟字 | `VGEngine.cpp::deactivate` 保存每 input context 的組字與候選位置並隱藏 UI；切回還原。Preedit 且無 ClientUnfocusCommit 的 client 使用 DontCommit inline preedit；ClientUnfocusCommit 或無 Preedit 則只用 panel preedit。reset／無修飾 Escape 才明確取消，候選狀態也一致。 | 五種 capability 測例覆蓋 Preedit、FormattedPreedit、ClientUnfocusCommit 與無 Preedit，檢查完成字、未完成注音、不同 IC、切回與提交不重播。Debug/Release 通過。切換使用測試用 pass-through keyboard addon 走真正 Fcitx 切換路徑。 |
| F11 等 unmapped keysym 清掉組字 | `VGBridge.swift::feed` 在 KBEvent 轉換失敗時先清除舊 commit，直接回目前 snapshot、handled=false、commit 空，不呼叫上游 nil-event reset。 | C ABI 回歸核對 F11/F12、down/up、未完成 `ㄍㄠ`、`科技`、`科技ㄍㄠ`、候選狀態與提交後清空，完整比較 candidates/cursor/selection flag。舊 bridge 反向控制 exit 134；新 Debug/Release 通過。原 F11 probe 亦確認 `ㄍㄠ` 保留並放行。 |
| 詞庫錯誤逃逸 callback | `VGEngine.cpp` 在 activation、key、selection、reset 邊界處理例外；reset 不為不存在的狀態建立 session。錯誤寫 Fcitx log 並顯示 auxDown，避免 framework 的 IM 資訊覆蓋。 | missing/invalid TextMap 測例檢查 event loop 存活、錯誤可見且提及 TextMap、可切到另一輸入法；reset 不建立 session。Debug/Release 通過。 |
| Release 鍵盤測試失效 | `key_event_adapter_test.cpp` 改用不受 NDEBUG 影響的 FCITX_ASSERT；C harness 也保留 assert。 | Debug/Release 正常測試通過；刻意錯誤的 Release 控制確實失敗，不以 exit 0 冒充有測到。C ABI harness 亦以 `-DNDEBUG` 編譯後執行。 |
| CI stdout golden 被詞庫 log 汙染 | `test_vgbridge.c` 擁有 stdout 協定：保存專用 JSON writer，再將一般 stdout 導向 stderr。Library 不改宿主 streams、不改上游全局 debug preference。`test_output_contract.sh` 比對整份 stdout 並要求詞庫 log 仍存在 stderr；workflow 加入 Debug/Release C 功能與 headless 測試。 | 舊 harness golden diff 多一行 Factory TextMap log，exit 1；新 Debug/Release 全份 golden 一致，stderr 保留診斷。未過濾非 JSON 行，預設 harness 仍執行所有功能測試。 |

## 實際驗證

工具鏈為本機既有 `swift:6.4-noble` image、Swift 6.4 RELEASE，容器使用
`--pull=never --network=none`，repo 唯讀、獨立 scratch 可寫，HOME 隔離。
Fcitx 測試在 Ubuntu 24.04 / Fcitx 5.1.7 主機建置，使用從同一 Swift image
複製的 runtime libraries。測試詞庫固定為 repository 的
`LXAssemblyMaterials4Tests/Resources/vanguardTextMap_test.txtMap`，不依賴個人詞庫。

- 6 個本地 Swift tests 通過，使用 `--no-parallel`；本次未重跑完整上游 674 tests。
- 新 Debug / Release bridge 的完整 C ABI 功能、stdout golden、stderr 診斷契約通過。
- 新 Debug / Release C++ addon 各 12/12 CTest 通過，涵蓋上述失敗情境。
- 新 library 及 addon 路徑、CMake cache、export symbols 與 SHA256 已核對。
- Shell scripts 通過 `bash -n`；workflow YAML 解析及 `git diff --check` 通過。

本機等效執行的入口：

```sh
bash vg-repl/Tests/c/test_output_contract.sh HARNESS \
  vg-repl/Tests/Fixtures/fcitx-su3.jsonl ARTIFACT_PREFIX
bash vg-repl/scripts/test-headless.sh BRIDGE_PRODUCTS BUILD_DIR Debug
bash vg-repl/scripts/test-headless.sh BRIDGE_PRODUCTS BUILD_DIR Release
bash vg-repl/scripts/test-memory.sh BRIDGE_PRODUCTS MEMORY_ARTIFACT_DIR
```

詳細命令與結果保存在 `.scratch/fix-2026-10-08/bridge-result.md`、
`bridge-build/`、`fcitx-result.md`、`fcitx-*-green.log` 及紅方報告。
本機實測腳本使用隔離容器與主機現存 Fcitx 開發套件；未實際執行遠端 GitHub Actions
workflow，也未在乾淨容器連網 apt 安裝。CI 的直接 include/link 依賴已核對 package
provenance 和乾淨 image；另外明列 `nlohmann-json3-dev`，避免依賴主機額外 header。

## LeakSanitizer 歸因與保留檢查

原審查的 47,952 bytes / 1,098 allocations 已由獨立分析歸因至 Swift 6.4 Linux
Foundation 的 CFRunArray lifetime。只建立並釋放 NSMutableAttributedString 的
standalone 程式即可重現；1/10/100 次為 112/1,120/11,200 bytes，且 weak reference
確認 NS 物件已釋放。原始所有 direct allocation roots 都含 `CFRunArrayCreate`，
單一符號抑制後無殘餘。這是已歸因的外部 runtime 缺陷，底層記憶體洩漏仍未修復。

新的完整 C 回歸增加測例後，未抑制 strict run 為 exit 1、127,504 bytes /
2,913 allocations；同樣分類為上述 roots。CI 分開執行功能/headless 與 memory job，
完整保存 strict stderr/stdout/exit，且只允許「無 leak」或已核對的 Foundation LSan
失敗；任何 signal、其它 ASan/UBSan 錯誤或未知 direct root 會失敗。

required memory check 保持 `ASAN_OPTIONS=detect_leaks=1`，唯一 suppression 是
`leak:^CFRunArrayCreate$`，不是整庫排除。反向控制證明此設定仍抓到 malloc 的
41-byte leak，以及故意不釋放 `vg_feed_key` response 的 109-byte leak（新 JSON
長度加 NUL）；兩者均 exit 1。tracked 最小重現、suppression、controls 在
`vg-repl/Tests/c/`，分類與執行腳本在 `vg-repl/scripts/`。

ASan 目前 instrument C harness；Swift bridge 與全部依賴未以完整 sanitizer 重建。
因此本次不宣稱 runtime 無漏、完整 Swift 記憶體安全或已修掉 Foundation 缺陷。
若工具鏈後續修復，strict=0 的分類會提示重新評估移除 suppression。

## 尚未驗證與後續

未驗收真實 Wayland/X11 應用程式、toolkit、桌面候選 panel、打包動態載入及安裝升級。
Headless 的 pass-through keyboard 不代表真正桌面 keyboard addon 已驗收。
安裝 staging／切換及升級復原、REPL replay 效能、symbols/associates ABI 完整性、
跨 session preferences 隔離、cursor 座標延伸仍為後續工作。GPU 不涉及本次任務。
