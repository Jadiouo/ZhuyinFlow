# License and attribution notices

ZhuyinFlow is an independent Linux/Fcitx 5 remix. It links to, but does not
modify, the vChewing Vanguard core pinned in the `upstream` Git submodule at
`977a05fe353bcc43cffbc03ea8b6d815a072fd65`.

## Components

| Component | License | Source |
|---|---|---|
| ZhuyinFlow-authored project files, including bridge, Fcitx adapter, REPL, packaging, tests, icon, and documentation | MulanPSL-2.0 | This repository |
| vChewing Vanguard Swift core and its modules | LGPL-3.0-or-later, with module-specific additional permissions | `upstream/Packages/vChewing_OSNeutral_LibVanguard/` |
| Vanguard Lexicon builder source | MulanPSL-2.0 | [vChewing-VanguardLexicon](https://github.com/vChewing/vChewing-VanguardLexicon), pinned by the installer |
| Fcitx 5, Swift runtime, and host packages | Their respective upstream licenses | System packages / Docker image; not vendored here |

The complete upstream Vanguard package license, module license texts, and all
custom LGPL exceptions are copied unchanged under `upstream-vanguard/`. The
custom exception files are material: the upstream package requires them to be
included and referenced when applying those additional permissions. The copy
under `MulanPSL-2.0-upstream-reference.txt` retains the upstream's original
attribution and contains the MulanPSL-2.0 terms referenced for our code; it is
not a claim that ZhuyinFlow was authored by the vChewing Project.

The installer downloads the lexicon source and builds a TextMap locally; this
repository does not distribute that generated dictionary. The builder's
upstream license notice is included in `lexicon/VanguardLexicon-LICENSE.txt`.
The source data used to build a dictionary may carry additional notices, so
review those notices before redistributing generated data.

## Required notices when redistributing

When redistributing source or binaries:

- Preserve the applicable license and copyright notices.
- Include the LGPL text and applicable custom exception notices for Vanguard.
- Clearly identify the Vanguard core as an upstream component and keep it
  replaceable/relinkable as required by its LGPL terms.
- Include the corresponding source and build instructions needed to reproduce
  or relink the distributed combination.
- Do not imply endorsement by the vChewing Project or use its name/logo as the
  name or branding of this independent project.

This notice is a practical component map, not legal advice. Review the actual
license texts and obtain qualified advice for a commercial distribution.
