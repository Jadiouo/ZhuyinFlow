# Redistributed components

ZhuyinFlow-authored code: Copyright 2026 ZhuyinFlow contributors;
MulanPSL-2.0. The complete terms and component map are included under
`copyrights/project/`. The upstream authorship in its original reference
license is retained and does not describe ZhuyinFlow's authorship.

Vanguard core: pinned source `977a05fe353bcc43cffbc03ea8b6d815a072fd65`,
LGPL-3.0-or-later with the original module-specific permissions. All module
licenses, exceptions, LGPL and GPL texts are included. SwiftExtension code
also retains its MulanPSL notice. Replaceable dynamic libraries are installed
in `/usr/lib/x86_64-linux-gnu/zhuyinflow/`; the companion source archive
contains the exact source and relinking/build scripts for this combination.

Swift 6.4 Linux runtimes: Apache-2.0 with Swift runtime library exception;
the exact toolchain license is `Swift-runtime-LICENSE.txt`. Dispatch and
BlocksRuntime notices are in `swift-corelibs-libdispatch-LICENSE.txt`.
Foundation ICU notices are retained in `swift-foundation-icu-LICENSE.txt`;
the actual ICU runtime version is 76.1, whose complete Unicode/third-party
terms are in `ICU-76.1-LICENSE.txt`. The source references and binary-source
revision limitations are recorded in `provenance.json`.

Factory dictionary: rebuilt from the unmodified VanguardLexicon source at
`d41f2fc244eadf94c37df50ef98e716fdc28146d`. Its LICENSE and README accompany
this notice. MulanPSL applies to upstream-authored work; the following data
sources retain their own terms and are not relicensed by this project:

- 中華民國教育部（Ministry of Education, R.O.C.）。《重編國語辭典修訂本》
  （2015, as declared by the pinned upstream source）。
  https://dict.revised.moe.edu.tw/ — CC BY-ND 3.0 Taiwan:
  https://creativecommons.org/licenses/by-nd/3.0/tw/legalcode .
  The complete prescribed use notice is `MOE-reviseddict-public-use.pdf`.
- 數位發展部，CNS11643中文標準交換碼全字庫網站：
  https://www.cns11643.gov.tw/ — Open Government Data License v1.0.
  Complete terms and attribution appendix: `OGD-1.0-zh-Hant-full.txt`.
- LibTaBE: Pai-Hsiang Hsiao and relevant TaBE Project/Institute of Information
  Science, Academia Sinica copyright holders; full BSD notices/disclaimers
  are retained in `LibTaBE-COPYING-from-ICU76.1.txt`.
- Singaporean Mandarin Database, https://www.languagecouncils.sg/mandarin/ch/learning-resources/singaporean-mandarin-database .
- NAER frequency data and the remaining references identified in the pinned
  `VanguardLexicon-README.md` remain attributed to their respective sources.

The pinned builder extracts word/pronunciation entries and converts them to
TextMap format. MOE's specific notice permits format adjustment and restricts
changes to entries. Whether particular upstream additions or extracted facts
constitute protected derivative expression has not been independently
established; no blanket claim that MulanPSL overrides these data terms is made.
