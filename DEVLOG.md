# Development log

## 2026-10-06 — ZhuyinFlow public-preparation build

- Prepared the Linux/Fcitx 5 remix under the independent name **ZhuyinFlow**,
  with matching Fcitx addon identifiers, descriptors, icon, and configuration
  variables.
- Documented Ubuntu installation, configuration, upgrade from the earlier
  local prototype, removal, upstream attribution, and component-specific
  licenses.
- Added the Fcitx 5 adapter, user-local installer, and TestFrontend smoke
  coverage. Kept the upstream Vanguard submodule unchanged at
  `977a05fe353bcc43cffbc03ea8b6d815a072fd65`.
- Excluded internal planning, evaluation, and handoff files from the release
  snapshot.
- Verified the current source locally with Swift 6.4: 6 tests passed. Built and
  installed the Fcitx addon in a temporary test location; CTest passed 2/2,
  including the TestFrontend smoke test using the pinned factory dictionary.
- Desktop panel rendering and interactive testing in the target Wayland/X11
  session remain to be checked.

## 2026-10-06 — Default and indicator follow-up

- Disabled Zhuyin furious typing by default; users can still opt in with
  `ZHUYINFLOW_FURIOUS_TYPING_ZHUYIN=1`.
- Strengthened the standalone ZhuyinFlow icon so its Z glyph is unambiguous,
  and added an explicit architecture note that the project bridges the Swift
  Vanguard engine rather than rewriting it in Rust.
- Diagnosed the missing indicator on the local desktop: Fcitx was still using
  its old `vchewing` profile entry; ZhuyinFlow's addon and icon were not yet
  installed. Installed the updated addon, added ZhuyinFlow to the profile,
  selected it, and confirmed it is now the active input method.
- Swift 6.4 tests passed 6/6 and the Fcitx TestFrontend tests passed 2/2.
  Rendering the panel icon in the user's desktop panel still needs visual
  confirmation.

## 2026-10-06 — Clean up the Fcitx input-method menu

- Removed McBopomofo and the legacy vChewing entry from the active Fcitx
  profile, leaving the English keyboard and ZhuyinFlow. Removed the obsolete
  user-local vChewing addon files; kept the system McBopomofo package and its
  user data untouched.
- Found that the user hicolor icon directory had no `index.theme`, so the
  installed ZhuyinFlow icon could not be indexed. Updated the installer to
  create the standard user-local theme index when missing and refresh its
  icon cache.
- Kept Swift as the Vanguard core and bridge implementation; no Rust rewrite
  was needed for this request.

## 2026-10-06 — Restore Fcitx addon discovery

- Diagnosed the “Not Available” input method: the D-Bus-activated Fcitx process
  did not inherit `FCITX_ADDON_DIRS`, so it could not locate the user-local
  `zhuyinflow.so`. Confirmed the addon loads when the variable is present.
- Set the path in the current D-Bus and systemd user activation environments
  and in both existing user Fcitx autostart entries. The active input method
  now resolves to `zhuyinflow`.

## 2026-10-08 — 候選輸入修復、桌面部署與 Ubuntu 打包

- 完成候選操作：Down 明確進入選字模式，方向鍵移動、PageUp/PageDown
  與 Space 翻頁，1–9 選字，Enter 選中並提交；一般輸入仍交給核心，
  不讓候選提示誤吞大千數字鍵。候選模式的無修飾 Escape 完整取消，
  Ctrl/Alt/Shift/Super+Escape 不誤取消；keyup 不重建空 UI。
- 修正 Swift bridge 對 F11 等未支援 keysym 的處理：保留組字與候選、
  回傳未處理並清掉舊 commit，避免送入上游 reset。失焦或切換輸入法
  保存各 input context，隱藏 UI、切回還原；依 client capability 選擇
  preedit 呈現。詞庫例外由 callback 邊界接住，auxDown 顯示錯誤。
  C harness 分離 JSON stdout 與 stderr 診斷，保留完整 golden 比對；
  Release assertions 亦確實執行。詳見[修復階段紀錄](docs/fix-report-2026-10-08.md)。
- 以實際舊版及正式詞庫重現 Z 提示殘留、候選 Enter 吞鍵：事件迴圈仍運作，
  並非已證實的核心死鎖。獲授權後備份並更新 user-local Release，
  重啟 Fcitx，核對實際映射檔身分與啟動日誌；修正部署 wrapper 等待
  daemon pipe EOF 而誤判失敗的問題。使用者在原終端實測確認 Z 提示
  會消失，選字與 Enter 正常；不推廣為所有桌面 client 已驗收。
- 完成 Ubuntu 24.04 LTS amd64、Fcitx 5.1.7 的 `zhuyinflow 0.1.0` `.deb`、
  對應 source archive 與 SHA256SUMS；提供「注音流設定」入口及明確遷移工具。
  套件包含固定來源重建的正式字典與私有 Swift runtimes，不自動切換預設
  輸入法或重啟。今日桌面驗收使用 user-local 版本；新 `.deb` 只在隔離
  Ubuntu 容器安裝、移除，尚未安裝到 host。見[安裝與來源建置](packaging/README.md)。
- 本地六個 Swift tests、Debug/Release 完整 C ABI 與 stdout/stderr 契約，
  以及兩配置各 12 個 headless 情境通過。實際解開無 `.git` 的 source
  archive，以新建置目錄冷重建並通過 12/12；不宣稱逐 byte 可重現。
  最終 CI 產物另核對 1,089 筆 source checksum、80 個專案 blobs、套件內
  provenance；乾淨容器的嚴格 dpkg、loader、正式詞庫單次提交、遷移及移除驗證通過。
- Foundation 的 `CFRunArrayCreate` 洩漏仍未修復：完整本地 strict LSan
  記錄 127,504 bytes／2,913 allocations，只分類為已知外部缺陷。
  required check 僅用精確符號 suppression；41-byte malloc 與
  109-byte response 負控制仍失敗，沒有關閉 leak detection。
- 兩倉庫保留各自歷史與產品預設，分別正常推送 main：私有 `eb4c4cc`、
  公開 `b19abf1`。兩端原本僅有 main，沒有額外分支需合併，公開歷史未混入
  私有祖先。最新[私有 CI](https://github.com/Jadiouo/vchewing-linux-lab/actions/runs/37795391890)
  與[公開 CI](https://github.com/Jadiouo/ZhuyinFlow/actions/runs/37795413927)
  的 test、memory、package 全成功；實際下載並核對兩份包，`dist/`
  更新為公開 CI 的交付檔，未建立 GitHub Release。
- 真實冷 CI 揭露四項缺口：BuildKit 未匯入可供 inspect 的 base tag、
  Unix Makefiles 缺 `build-essential`、Docker `join` 的 digest 型別不符，
  以及 artifact upload 無法遍歷 root-owned 0700 runtime。逐項修正為
  先取得並記錄 base digest、補工具、用 `range` 輸出 metadata，以及只
  stage 固定可讀證據。最終 workflow 保留空 dpkg 結果、原始失敗碼與
  PASS marker，逐檔開啟及 hash 後上傳，不放寬檢查或上傳 runtime 狀態。
- 按新規將暫存 worktree 移至相對 `.claude/worktrees/`，修復 Git/submodule
  登記；確認提交已遠端保存、沒有未存資料後清理。後續本機產物都留在
  repo 內，scratch、備份與 binary 不入 Git。下一步是新 `.deb` 的真實
  GUI／升級驗收及更多 client；本版僅支援 Ubuntu 24.04 amd64，其他
  發行版、架構尚未驗證，亦不宣稱 runtime 無漏。
