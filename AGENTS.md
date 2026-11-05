# QsAOW3 development

- Read `README.md`, `docs/BUILD.md`, `docs/ARCHITECTURE.md` and the relevant module documentation.
- Build with `python tools/build.py`; the pinned bootstrap supplies the original engine/content. Keep its SHA256 and ABI checks.
- Run `python tools/test.py` on POSIX hosts, or explicitly select an Android device with `--android --serial ...`.
- Use fresh temporary directories or isolated Android users for engine validation. Preserve current player data; never clear app storage or restore an old player snapshot as part of a test.
- Implement shared mechanisms/configuration rather than duplicating individual chapters/cards. Preserve real engine outcomes and atomic, once-only reward transactions.
- Treat persisted structs, slot numbers, checksums and receipt meanings as compatibility interfaces. Add migrations and failure/corruption tests for format changes.
- State missing original inputs and unresolved mixed effects precisely. Document any deliberate offline rule separately from verified original behavior.
- Keep APKs, keys, credentials, player data, dumps and workstation-specific history outside Git. Signing credentials belong in maintainer-controlled private storage.
- Windows shell work should use PowerShell 7 when available. Check resolved paths before removing generated directories; avoid cross-shell destructive commands.
