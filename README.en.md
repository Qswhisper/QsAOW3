# QsAOW3

[中文](README.md)

An unofficial Android offline adaptation of Art of War 3, retaining its Unity/IL2CPP client, controls and UI. Local careers, persistent rewards and AI-controlled seats support the integrated modes.

**[Download](https://github.com/Qswhisper/QsAOW3/releases/latest)** · [Build](docs/BUILD.md) · [Architecture](docs/ARCHITECTURE.md) · [Roadmap](ROADMAP.md)

The source in this repository implements the C++ adaptation, policies, persistence, tests and build tools. A hash-pinned bootstrap APK supplies the original engine and content, whose rights remain with their owners; see [third-party notices](THIRD_PARTY_NOTICES.md).

## Play

- Android API 24+ and ARM64. Install the release APK; no network or manually supplied cache is required.
- A fresh career starts the original tutorial, with 1000 credits, 20 gold, zero experience/stars and no purchased research.
- The original settings UI can create an independent career while retaining existing progress, and switch local careers.
- Updates require the same package and signing certificate. Read [signing and saves](docs/BUILD.md#签名与存档) before installing a self-signed build over an existing installation.

| Feature | Available | Remaining limits |
| --- | --- | --- |
| Campaign | Confederation 01–21 and Resistance 01–34: 55 ordinary chapters | 45 later ordinary chapters and complete hard-mode inputs are missing |
| Training | Nine original formats, land/air/naval AI; early access | No formal rewards or rating changes |
| Formal battles | Original 1v1/2v2/3v3 formats with AI filling other seats | Original rank gates; local matchmaking |
| Cooperation | Integrated Resistance pumpkin scenario with AI allies | Other independent scenarios lack complete inputs |
| Progression | Research, heroes, skills, inventory, supported modifications and reward cycles | Some mixed effects, prerequisites and original reward pools remain unresolved |
| Careers | Independent local progression, result settlement and history | No online account synchronization |

Unavailable online purchases, ads, daily login and social operations are disabled. Offline-specific rules and their limits are documented in [GAMEPLAY.md](docs/GAMEPLAY.md).

## Build

Install Python 3.11+, Java 17+, Android SDK Build Tools 36.0.0 and NDK 29.0.14206865. The tools include Windows, Linux and macOS host paths; the complete build was verified on Windows.

```sh
git clone https://github.com/Qswhisper/QsAOW3.git
cd QsAOW3
python tools/fetch_base.py
python tools/verify.py
python tools/build.py
```

The output is `dist/QsAOW3.apk`, signed with a local development key by default. Use `--base PATH` for an existing verified bootstrap APK. Historical workspaces and private emulator data are unnecessary.

```sh
# Linux/macOS native standalone tests
python tools/test.py

# Android standalone tests; an explicit device is required
python tools/test.py --android --serial DEVICE_SERIAL
```

See the [build guide](docs/BUILD.md) for signing, SDK selection, content staging and testing. Start development with [ARCHITECTURE.md](docs/ARCHITECTURE.md), [CONTENT.md](docs/CONTENT.md), [AI.md](docs/AI.md) and [CONTRIBUTING.md](CONTRIBUTING.md).

## License

New adaptation code and tools are available under the [MIT License](LICENSE). Original game software, audiovisual assets, maps, narrative, configurations and trademarks retain their existing rights and are excluded from that license. This is an independent project; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
