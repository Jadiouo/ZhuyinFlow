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
- [ ] Type a reading with multiple candidates and select using the configured
  navigation/number keys.
- [ ] During composition, press Backspace and verify the reading is edited;
  cancel composition and check that it does not insert text.
- [ ] Use Fcitx's input-method toggle and verify ZhuyinFlow deactivates and
  ordinary input resumes.
- [ ] Start composition, switch windows, return to the original window, and
  finish the text; composition should remain scoped to its input context.

TestFrontend and automated builds do not replace desktop-application checks.
