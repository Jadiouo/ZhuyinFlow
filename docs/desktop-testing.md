# Desktop acceptance checklist

After installing ZhuyinFlow and adding it in Fcitx Configuration, test in the
Wayland or X11 desktop session that you actually use:

- [ ] In a terminal, type `s u 3 Enter` and confirm the application receives
  「你」.
- [ ] After committing Chinese with `s u 3 Enter`, type `film Space` and confirm
  the English text is committed directly.
- [ ] Set `ZHUYINFLOW_MIXED_ALPHANUMERICAL=0` and
  `ZHUYINFLOW_FURIOUS_TYPING_ZHUYIN=1` in the Fcitx process environment,
  restart Fcitx, type Dachen keys `e l e j /`, and confirm automatic Zhuyin
  syllable chopping.
- [ ] Check that the Fcitx indicator shows ZhuyinFlow's “Z” label and icon, and
  that the active method is clear when switching between input methods.
- [ ] In a browser text field, type a complete Chinese sentence and confirm
  committed text appears in the field.
- [ ] In a text editor, verify preedit, cursor movement, candidate selection,
  and committed text.
- [ ] Type a reading with multiple candidates, press Down to enter selection,
  navigate with arrows and PageUp/PageDown (Space advances a page), and select a
  displayed 1–9 label. Enter selects the highlighted candidate and commits it.
  Verify the displayed label, selected candidate, and final committed text agree.
- [ ] Page the candidate list and release the key; verify its page and highlighted
  cursor stay put. Switch windows and return while selecting; verify they restore.
- [ ] Complete `s u 3`, then start another Dachen syllable with `2`, `5`, or `8`
  without pressing Down. Verify it continues typing instead of selecting a label.
  With mixed typing enabled, also check `film2026`.
- [ ] Press F11 on both key down and key up during unfinished reading and during
  Chinese composition. Verify composition, candidates, and cursor remain intact,
  the client still receives the key, and no text is committed or replayed.
- [ ] During composition, press Backspace and verify the reading is edited;
  cancel composition with unmodified Escape and check that it does not insert text.
- [ ] While selecting candidates on a later page, press unmodified Escape and
  verify composition, candidate list, and preedit clear without a commit. Release
  the key, then type a new reading and commit it. Ctrl/Alt/Shift/Super+Escape must
  not invoke this cancellation path.
- [ ] Use Fcitx's input-method toggle with unfinished reading and completed
  Chinese text. Verify its UI hides, ordinary input resumes, and switching back
  restores the same composition without inserting text twice.
- [ ] Start composition, switch windows, return to the original window, and
  finish the text; composition should remain scoped to its input context.

The retention policy applies to both focus loss and input-method switching.
Preedit-capable clients that do not commit on unfocus receive inline preedit
marked DontCommit. Clients with ClientUnfocusCommit, or without Preedit support,
use panel-only preedit so focus changes do not insert raw readings or duplicate
text. Explicit reset and unmodified Escape cancel composition, including candidate selection.

Headless regression tests exercise Preedit, FormattedPreedit, ClientUnfocusCommit,
and no-preedit capability combinations, including a separate input context and
invalid dictionary errors. Their input-method switch uses a test-only pass-through
keyboard addon through the real Fcitx switching path. Real desktop keyboard
addons, client/toolkit behavior, panels, Wayland/X11 sessions, and installation
upgrades still require the checklist above; headless success does not establish
those results.
