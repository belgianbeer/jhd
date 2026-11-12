# jhd — 日本語 Hex Dump ユーティリティ（v3.1）

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
| 2026-11-12 | v3.1 | 行末に残った漢字の前半バイトの改行漏れを修正 |

- 作者：民田 雅人（Masato Minda）
- 管理：当初は **CVS**（1986 年〜）。2026 年に **Git** へ履歴移行（`git cvsimport` 使用）。

### v3.1 の修正内容（原典から受け継いだ改行漏れ）

**現象** — ダンプの最終行がちょうど埋まっていて（データサイズが行幅＝既定 16 バイト、
`-c` 時は 64 バイトの整数倍）、その**最終バイトが漢字の前半として扱われた**とき、
最終行の改行が出力されずに終わっていました。端末では次のシェルプロンプトが
ダンプ行の末尾に食い込みます。漢字コードが EUC の場合は前半バイトとみなされる
`0xA1–0xFE` のバイトなら何であっても（本来は後半バイトでも）、また半角カナの
シフト `0x8E` でも同じでした。文字欄を漢字モードで出すときの話なので、
`-a`（ASCII 表示）や TTYPE 未設定、8 進数ダンプ（`-o`/`-b`/`-r`）では起きません。

**原因** — 漢字の対が行をまたぐ場合、`jhd` は前半バイトを `Skip`（EUC では kana 用にもう一つ
`Kskip`）に保持し、`dmpnl(LINECONT)` で改行を**抑止**します。続けて次の行の先頭で
`putpartial()` が対をまとめて印字し、そこで初めて改行が出ます。
入力がちょうど行境界で尽きたときには `putpartial()` が呼ばれる機会がないため、抑止された
まま改行が失われていました。アーカイブ版 [jhdold](https://github.com/belgianbeer/jhdold)
の原典ソース（v2.4 / 1999 年）で実際に再現確認済みで、ANSI C 化以前から引きずっていた
バグです。

**修正** — `dmpmain()` が入力終了（EOF）時点で保留状態を確認し、前半バイトを示す `.` と
改行を出力して締めるようにしました。EUC / Shift-JIS 両ビルド、16 / 64 バイト幅の
すべてに効きます。それ以外のダンプ表示は変わりません（正常な系では 1 バイトも出力が増えません）。

```sh
# 再現：16 バイトちょうど、最終バイトが EUC 漢字の前半（0xA4）
perl -e 'print "A" x 15; print chr(0xa4)' > sample.bin
TTYPE=mskanji ./jhd sample.bin

# v3.0 まで → 00000000  41 41 ... 41 41 A4  AAAAAAAAAAAAAAA    ← この行が改行されずに終わる
# v3.1      → 00000000  41 41 ... 41 41 A4  AAAAAAAAAAAAAAA.   ← 前半バイトの "." が出て改行される
# （アーカイブ版 jhdold の v2.4 でも同じ結果になります）
```

> ※ `jhd.1`（man ページ）の日付も 2026 年 11 月 12 日へ更新しました。

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
| 2026-11-12 | v3.1 | fixed a lost line break after a trailing first-half kanji byte |

- Author: Masato Minda
- Version control: originally managed under **CVS** (since 1986). History migrated to **Git** in 2026 via `git cvsimport`.

### What v3.1 fixes (a lost line break inherited from the original)

**Symptom** — when the last dump line was completely filled (the data size an exact
multiple of the line width: 16 bytes by default, 64 with `-c`) and **its final byte was
taken for the first half of a kanji**, the line was never terminated: no newline was
emitted, so the next shell prompt ran into the end of the dump. In the EUC build any byte
of `0xA1`–`0xFE` counts as a first half (even one that really trails a pair), and so does
the halfwidth kana shift `0x8E`. Only the character column of a kanji terminal can wait
for a second half, so `-a` (ASCII display), an unset `TTYPE` and the octal dumps
(`-o`/`-b`/`-r`) were never affected.

**Cause** — when a kanji pair straddles a line boundary, `jhd` parks the lead byte in `Skip`
(`Kskip` as well, for kana in the EUC build) and `dmpnl(LINECONT)` **holds the newline back**;
`putpartial()` prints the pair at the head of the next line and only then breaks the line.
If the input ended exactly on a line boundary, `putpartial()` was never reached and the held
back newline was lost. Reproduced with the untouched original of the archive,
[jhdold](https://github.com/belgianbeer/jhdold) (v2.4, 1999): this predates the ANSI C port.

**Fix** — `dmpmain()` now checks for a pending first half when the input is exhausted and
closes the output with the `.` marker plus the newline. It applies to both the EUC and the
Shift-JIS build and to both the 16- and 64-byte line widths. Nothing else in the dump changed:
the extra bytes are emitted only in the broken case.

```sh
# reproduce: exactly 16 bytes, the last one a EUC kanji lead byte (0xA4)
perl -e 'print "A" x 15; print chr(0xa4)' > sample.bin
TTYPE=mskanji ./jhd sample.bin

# up to v3.0 -> 00000000  41 41 ... 41 41 A4  AAAAAAAAAAAAAAA    <- the line ends with no newline
# v3.1       -> 00000000  41 41 ... 41 41 A4  AAAAAAAAAAAAAAA.   <- the "." and the newline appear
# (the archived jhdold, v2.4, gives the same result as v3.0 here)
```

> The date in the `jhd.1` man page was moved to November 12, 2026 as well.

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
