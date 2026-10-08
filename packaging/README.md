# Ubuntu 24.04 套件

目前支援 Ubuntu 24.04 LTS、amd64、Fcitx 5.1.7。這是 Fcitx 的輸入法套件，
不是獨立輸入法 daemon；不宣稱適用其他 Linux 發行版或 CPU 架構。

## 安裝與設定

從同一份 release 下載 `zhuyinflow_0.1.0_amd64.deb`、
`zhuyinflow_0.1.0_sources.tar.xz` 與 `zhuyinflow_0.1.0_SHA256SUMS`。

```sh
sha256sum -c zhuyinflow_0.1.0_SHA256SUMS
sudo apt install ./zhuyinflow_0.1.0_amd64.deb
zhuyinflow-migrate --check
```

若之前使用 `vg-repl/fcitx5/install.sh` 安裝到 `~/.local`，舊 addon 或註冊檔
可能優先於系統套件。確認檢查清單後手動執行 `zhuyinflow-migrate --migrate`。
工具將六種已知 ZhuyinFlow 檔案移至 `~/.local/state/zhuyinflow/migration-*`
（或 `$XDG_STATE_HOME`），每個 payload 有原始路徑清單，方便復原。
它不刪除個人詞庫、偏好或其它 Swift 檔案，不停止任何程序。

登出再登入，開啟應用程式選單「注音流設定」，在 Fcitx 設定加入 ZhuyinFlow。
此入口啟動 `fcitx5-configtool`，並顯示遷移檢查；不變更預設輸入法。
套件安裝、升級或移除都不自動重啟 Fcitx，也不修改桌面的輸入法環境。
尚未配置 Fcitx 的桌面仍需依 Ubuntu 的輸入法設定啟用 Fcitx 5。

使用大千鍵盤輸入；Down 進入候選模式，方向鍵移動候選、PageUp/PageDown 或
Space 翻頁，1–9 選字，Enter 提交。Escape 取消，切換輸入法或失焦保留組字。

## 詞庫與執行期

字典依序採用：明確的 `ZHUYINFLOW_TEXTMAP`、個人
`$XDG_DATA_HOME/zhuyinflow/VanguardFactoryDict4Typing.txtMap`
（預設 `~/.local/share/...`）、套件 `/usr/share/zhuyinflow/VanguardFactoryDict4Typing.txtMap`。
明確指定的不存在或損壞檔案會顯示錯誤，不暗中改用其它字典。
設定空白 `ZHUYINFLOW_TEXTMAP` 保留原 bridge 預設行為。

Addon 安裝於 `/usr/lib/x86_64-linux-gnu/fcitx5/zhuyinflow.so`；Swift、bridge 與
Vanguard 在 `/usr/lib/x86_64-linux-gnu/zhuyinflow/`，以相對 RUNPATH 載入。
套件不覆蓋宿主 Fcitx、glibc 或系統 Swift。套件沒有 Multi-Arch 宣告，僅供 amd64。

## 升級、移除與復原

```sh
sudo apt install ./zhuyinflow_NEWVERSION_amd64.deb
sudo apt remove zhuyinflow
```

升級後登出再登入。移除（含 `apt purge`）保留使用者 HOME 的字典與偏好。
移除後可從 Fcitx 設定刪除 ZhuyinFlow 項目。需要回復舊版本時安裝先前 `.deb`；
需要恢復使用者安裝時，按 migration 目錄的 `*.original-path` 將對應
`*.payload` 移回，再登出登入。不要同時保留兩套 addon。

## 從來源建置

主機需 Docker、Git、Python 3、GNU tar/xz（不需要主機 Swift 或 sudo apt）。

```sh
git submodule update --init --recursive
bash packaging/build-deb.sh
bash packaging/verify-deb.sh dist/zhuyinflow_0.1.0_amd64.deb
```

建置容器使用 `swift:6.4-noble`，自動安裝 CMake、Ubuntu Fcitx 開發套件及 Debian
打包工具；建置時關閉網路，容器沒有桌面/session bus。需要取得 image、Ubuntu
依賴與首次 lexicon checkout 時會連網。可用 `SWIFT_IMAGE` 指定相同工具鏈的
image digest；`ZHUYINFLOW_PACKAGE_WORK` 指定新的 scratch 目錄，
`ZHUYINFLOW_PACKAGE_OUTPUT` 指定輸出目錄，`ZHUYINFLOW_VERSION` 指定版本。
重跑需提供新的 work 目錄，避免混用舊產物。

交付的 source archive 已內含兩個固定來源，不需要 Git metadata 或再次抓取詞庫：

```sh
mkdir zhuyinflow-source
tar -xJf zhuyinflow_0.1.0_sources.tar.xz -C zhuyinflow-source
cd zhuyinflow-source
bash packaging/rebuild-export.sh
```

此入口先核對 `SOURCE-SHA256` 再建置。`SWIFT-IMAGE` 記錄實際 Swift image ID/digest，
`BUILD-PACKAGES` 記錄容器 Debian 依賴版本；可指定記錄中的 `SWIFT_IMAGE` digest。
Ubuntu apt 更新與工具鏈編譯輸出可能改變，因此不宣稱不同日期建置必定逐 byte 相同。

正式字典從 VanguardLexicon `d41f2fc244eadf94c37df50ef98e716fdc28146d` 的乾淨
Git export 重建，沒有使用個人已安裝詞庫。核心固定於 Vanguard
`977a05fe353bcc43cffbc03ea8b6d815a072fd65`。每份來源包包含實際工作樹、兩個固定
來源 export、SHA256 manifest 和重建腳本；二進位套件包含來源 manifest、版號與
完整授權 notices（`/usr/share/doc/zhuyinflow/`）。Swift/Vanguard 可從對應源碼
重建並替換套件私有 libraries。重新連結 addon 的參數在 `container-build.sh`。

Vanguard export 排除不參與 Linux 建置的
`LegacyZone/ARCLite/libarclite_macosx.a`（Xcode 附帶的 Darwin binary）；
排除項記錄於 `BUILD-PROVENANCE`。Linux 核心來源、授權與建置輸入完整保留。

`verify-deb.sh` 在乾淨 Ubuntu 容器安裝、檢查 loader、驗證正式字典和優先順序、
移除再檢查保留的個人資料。這不等同於真實 Wayland/X11 桌面的完整驗收。
