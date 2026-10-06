# ZhuyinFlow Swift bridge and REPL

This Swift package contains:

- `vg-repl`, a spelling-only Dachen Zhuyin REPL.
- `vgbridge`, a C-ABI dynamic library wrapping the upstream Vanguard input
  session for the Fcitx 5 addon.

From the repository root, run the spelling-only REPL:

```sh
./vg-repl/run.sh --layout dachen
```

Enter one key per line (`s`, `u`, `3`, `backspace`, `space`, or `shift+a`).
The command prints one JSON response per key. Press Ctrl+D to finish.

To build the bridge with Swift 6.4 in Docker:

```sh
docker run --rm -v "$PWD":/work -w /work swift:6.4-noble \
  swift build --package-path /work/vg-repl --product vgbridge
```

The C interface is declared in `Sources/VGBridge/include/vgbridge.h`. It accepts
`{"layout":"dachen"}` for spelling-only input. To enable upstream composition,
candidates, and commits, include an absolute path to a factory TextMap:

```c
vg_session_new("{\"layout\":\"dachen\",\"lexicon\":\"/path/to/factory.txtMap\"}");
```

Optional JSON fields `mixedAlphanumericalEnabled` and
`furiousTypingEnabled4Zhuyin` override the corresponding upstream preferences.
The bridge is main-thread-bound and calls for sessions must be serialized. One
TextMap is shared by all live sessions in the process; concurrent sessions
cannot use different TextMap paths.

The Fcitx addon source and user-local installer are in `fcitx5/`. See the root
README for installation, configuration, and licensing.
