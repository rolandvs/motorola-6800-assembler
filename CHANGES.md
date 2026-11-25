# Changes

These changes bring the improvements that Albert van der Horst (HCC Forth
gg, The Netherlands) made to **as9** in 2004 to all six assemblers (as0,
as1, as4, as5, as9, as11). His changes include those of Mike Pashea (output
files instead of standard output) and are in the public domain; his
original notes are in
[documentation/as9_changes_vanderhorst_2004.txt](documentation/as9_changes_vanderhorst_2004.txt).
They have been rewritten for the ANSI C, multi-file structure of this fork.
Each item is a separate git commit.

## From van der Horst's as9

- **Larger limits:** 256 character lines, 80 character labels, 256
  character file names (was 128, 16 and about 9).
- **CR/LF line endings** are accepted, and so is a last line without a
  newline. A line that is too long is an error instead of being split.
- **Case insensitive** register names (6809) and `opt` operands. This is
  done without `strncasecmp()`, so no POSIX function is needed.
- **Comment syntax:** `;` starts a comment line, and a `;` comment can
  follow the operand without a blank. `label:` is accepted.
- **Error messages** start with `ERROR` and show the offending text (an
  undefined symbol, an unknown mnemonic or `opt` operand).
- **Comment warning** (option `w`, on by default; `now` turns it off):
  text after the operand that does not start with `;` is reported. It
  catches `ldaa #bar - foo`, which assembles as `ldaa #bar`.
- **Output files:** listing, symbol table and cross reference go to
  `.lst`, `.sym` and `.crf` instead of the terminal. New option `bin`
  writes a raw binary file.

## Beyond van der Horst's version

- **S-records stay the default**, as in the original Motorola code (in
  van der Horst's version `.s19` was only written with option `s19`).
  `nos19` turns them off.
- **No fragmented `.bin`.** van der Horst's binary file was the bytes
  run together, so after an `org` or `rmb` jump no byte was at a known
  address. The `.bin` is now only written if the code is one
  contiguous block; otherwise the assembler reports
  `ERROR bin is fragmented (code jumps to $xxxx), use s19tobin for a
  correct bin file`. The `.s19` is still written.
- **New tool `s19tobin`** makes a correct binary image from the
  S-records: gaps filled with `$FF` (or any byte with `-f`), an exact
  EPROM window with `-s`/`-e`, and errors (no output) for bad
  checksums, addresses written twice and data outside the window.
  Reads S1, S2 and S3 records and CR/LF files. Unlike
  `objcopy -O binary --gap-fill`, it does not silently lose data when
  records overlap.
- **Unknown options** stop the assembler instead of being ignored. The
  documented form `as5 prog - l cre` (lone minus) still works.

## Bugs fixed

The original code or van der Horst's version had these bugs:

- `Obj_name` was a 14-byte buffer: any input file name longer than 9
  characters overflowed it.
- `parse_line()` read `Label[-1]` on every line without a label.
- A last line without a newline made the listing print memory beyond
  the line.
- The file extension was cut at the first `.` in the path, so
  `as0 ../test/used5.asm` wrote its output to `.s19`.
- as4: the `x`/`y` register symbols installed by `localinit()` were
  thrown away by `root = NULL` in `main()`, so `lda [x]` failed.
- Several loops left `Optr` one character past the end of the operand
  (FCB, FDB and 6809 register lists), so following code read stale
  text. Indexed operands (`5,x`) on as0/as1/as5 and `a,x` on as9 did
  not skip the register. These caused the spurious comment warnings
  van der Horst described in his notes.

## Testing

`make test` assembles a test program for each CPU and checks that the
S-records are byte for byte the same as those of the original fork, also
with upper case source and with the `;`/`label:` syntax. It also checks
that the comment warning appears exactly where it should. The listings,
symbol tables and cross references were checked to be identical to the
old terminal output. All six assemblers run clean under AddressSanitizer
and UBSan on these tests.

For s19tobin, `make test` compares its output with reference images
(checked against the image built from the S-records independently and
against GNU objcopy), tests the `-s`/`-e` window, and checks that a
duplicate address, a bad checksum, a non-S-record line and data outside
the window are all refused without writing a file.
