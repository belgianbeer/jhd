# jhd — 日本語 Hex Dump ユーティリティ（v3.0）

[日本語](#日本語) / [English](#english)

> 🔧 こちらは**現行の開発版**（ANSI C 化 + POSIX 前提の保守版）です。
> 1986–1999 年のオリジナルのままのソースと man は、下記に保存してあります
> 👉 https://github.com/belgianbeer/jhdold

## 日本語

### 概要

`jhd`（Japanese HexDump）は、1986 年に開発を開始した日本語対応の hex dump ツールです。
Shift-JIS / EUC を意識した端末出力、漢字・かなの 2 バイト処理、8/16 ビットのオクタル・リトル/ビッグエンディアンダンプなど、当時の日本語 UNIX 端末で「文字化けしないダンプ」を出す工夫が詰まっています。

### 来歴

| 日付 | バージョン | 備考 |
|------|-----------|------|
| 1986-09-11 | v1.0 | 初版 |
| 1988-09-14 | v2.0 | |
| 1989-01-20 | v2.1 | octal-word のバグ修正 |
| 1989-02-23 | v2.2 | stdin 時のオフセット修正 |
| 1999-01-27 | v2.4 | CVS 上での最終版 |
| 2026-11-10 | v3.0 | ANSI C 化、OS-9 / LSI-C 分岐を削除し POSIX 単独に |

- 作者：民田 雅人（Masato Minda）
- 管理：当初は **CVS**（1986 年〜）。2026 年に **Git** へ履歴移行（`git cvsimport` 使用）。

### 収録物

- `jhd.c` — ソース（ANSI C / ISO C、POSIX 環境用。OS-9・LSI-C 用の分岐は削除済み）
- `jhd.1` — 日本語 man ページ
- `Makefile` — ビルド / インストール用（`PREFIX`, `DESTDIR` 可変）
- `LICENSE` / `README.md`

> ※ `Makefile` は v3.0 から追加しました。当時の配布物にはありませんでした。

### ビルド / man 参照

```sh
make                            # → ./jhd ができます
make check                      # EUC / Shift-JIS 両分岐の構文チェック
sudo make install               # 既定は /usr/local、PREFIX・DESTDIR 変更可
man -l ./jhd.1                  # BSD の man なら -l でローカル man を読む
```

漢字コードは既定で EUC-JP です。Shift-JIS 版は `make CFLAGS="-O2 -DEUC=0"` でビルドします。
`isatty()`, `fileno()`, `fseeko()` など POSIX の関数を使うため、MSVC 単体では調整が必要です。

### ライセンス

作者本人が所有するフリーソフトウェアです（→ `LICENSE` 参照）。

---

*開発はこちらのリポジトリで行っています。原版（アーカイブ）は jhdold を参照してください。*

## English

### Overview

`jhd` (Japanese HexDump) is a Japanese-aware hex dump utility, first developed in 1986.
It handles Shift-JIS / EUC terminal output, 2-byte kanji/kana processing, and 8/16-bit octal dumps (both endiannesses), so that a hex dump of Japanese text wouldn't garble on the Japanese UNIX terminals of the day.

### History

| Date | Version | Notes |
|------|---------|-------|
| 1986-09-11 | v1.0 | Initial release |
| 1988-09-14 | v2.0 | |
| 1989-01-20 | v2.1 | bug fix in octal-word |
| 1989-02-23 | v2.2 | fix offset when file is stdin |
| 1999-01-27 | v2.4 | last version under CVS |
| 2026-11-10 | v3.0 | ANSI C, POSIX only (OS-9 / LSI-C branches dropped) |

- Author: Masato Minda
- Version control: originally managed under **CVS** (since 1986). History migrated to **Git** in 2026 via `git cvsimport`.

### Contents

- `jhd.c` — ANSI C / ISO C source, for POSIX systems (the OS-9 / LSI-C branches are gone)
- `jhd.1` — Japanese man page
- `Makefile` — build / install (`PREFIX`, `DESTDIR` are overridable)
- `LICENSE` / `README.md`

> The `Makefile` was added in v3.0; it was not part of the original distribution.

### Build / viewing the man page

```sh
make                            # builds ./jhd
make check                      # syntax-checks both the EUC and Shift-JIS branches
sudo make install               # default /usr/local; PREFIX and DESTDIR are overridable
man -l ./jhd.1                  # on BSD man, -l reads a local man page
```

The Kanji code is EUC-JP by default; build the Shift-JIS variant with `make CFLAGS="-O2 -DEUC=0"`.
It relies on POSIX interfaces (`isatty()`, `fileno()`, `fseeko()`), so MSVC alone will need adjustments.

### License

Free software owned by its author. See `LICENSE`.

---

*Development happens in this repository; the untouched original lives in `jhdold`.*
