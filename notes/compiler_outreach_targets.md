# Finding a mid-1990s Hitachi SHC compiler — outreach targets & routes

**Goal:** locate a pre-v5.0 Hitachi SuperH C compiler (SHC), roughly Ver 3.x / 4.x, circa 1996,
for a non-commercial preservation / matching-decompilation of **Die Hard Arcade / Dynamite Deka**
(Sega, 1996, ST-V / Titan hardware, SH-2). The ST-V shares the Saturn's Hitachi SH-2 CPUs, so
this is the *same* compiler problem the Saturn decomp scene is already wrestling with.

**Last compiled:** 2026-10-19. Public professional presence only. No private data. Every lead cites a source.

---

## TL;DR — the single most important finding

**You are not the first person hunting this exact compiler, and someone has already done the entire
search for you.** The `atolm` project (Panzer Dragoon Saga matching decomp) spent months proving
PDS was built with a *pre-Release-31, pre-v5.0 SHC* (they bound it to **Ver 4.1B or older, library
objects stamped 1996, idioms back to 1995**) and then exhaustively searched every public archive and
came up empty — concluding the target-era SHC **shipped on floppy/DAT, never on CD, was sold
separately by Hitachi (not bundled by Sega), and almost certainly survives only on a hard drive or in
a cupboard in Japan, if at all.** Their conclusion applies 1:1 to Die Hard Arcade (same SH-2, same
1996 window, same two-compiler choice).

**Do not re-run their search. Join it.** They explicitly say: *"if it surfaces we can use it in about
thirty minutes, because we know exactly what to test."* Pooling the Die Hard Arcade effort with atolm
is the highest-leverage move available — shared test harness, shared appeal, double the surface area.

Honest headline on the compiler itself: **a public copy very probably does not exist.** The realistic
paths are (a) a surviving industrial/embedded SHC owner or ex-Hitachi/Sega staffer in Japan digging up
old media, or (b) Renesas (current IP owner) granting a legacy copy. Both are long shots. Everything
below is ranked by that reality.

---

## WHERE TO START — top 3

1. **Open an issue / email on the `atolm` repo (blythos).** Fellow matching-decomp of a 1996-era SH-2
   Sega title hunting the *identical* pre-v5.0 SHC. Join the appeal, share the ST-V angle, use their
   30-minute test harness if anything turns up. (Lead #1)
2. **Ping sozud** (preserved the Saturn compiler binaries; runs `saturn-compiler-repro`, a bit-perfect
   SH-compiler reconstruction). Most compiler-literate person in the Saturn scene; knows exactly what's
   preserved and what isn't. (Lead #2)
3. **Post in the SegaXtreme Saturn-SDK / SHC threads** — the one live venue where old SH dev tools
   actually surface, and where ndiddy, antime et al. read. One well-written post reaches the whole
   Saturn dev-preservation community at once. (Lead #5)

---

## CATEGORY 1 — The SHC compiler lineage & its people

**Honest verdict: a dead end for named individuals.** Despite targeted searching, **no public record
names the engineers who wrote Hitachi's SHC.** Compiler authors at Hitachi/Renesas were never credited;
there are no interviews, papers, or bylines attributing SHC to a person. Hitachi Micro Systems Europe
(HMSE) / Hitachi Micro Systems (HMSI) dev-tools staff are likewise not publicly identifiable. Treat
"find the SHC authors" as unachievable with public OSINT — say so plainly and spend effort elsewhere.

The *adjacent* SH-ecosystem people below are reachable and knowledgeable about SH toolchain history,
but note: they are **GNU/open-hardware side**, not Hitachi's proprietary SHC. They're "who might know
who to ask," not people who have the binary.

| # | Name / handle | Who / why relevant | Public contact vector | Approachability | Best angle |
|---|---|---|---|---|---|
| L1-a | **Kaz (Kazumoto) Kojima** | Long-time GCC **SuperH back-end maintainer**; deep knowledge of SH codegen history and the SH GNU community. | Public dev emails: `kkojima@gcc.gnu.org`; `kkojima@rr.iij4u.or.jp` (openly published in GCC/binutils MAINTAINERS files) | Med — responsive on-list, but a stranger's proprietary-tool request is off his beat | "You're the SH-GCC authority — any idea who inside Hitachi owned SHC, or where old SH industrial toolchains ended up?" Keep it short, technical, respectful of his time. GNU side, so manage expectations. |
| L1-b | **D. Jeff Dionne** (+ Rob Landley) | Founders of the **J-core / open-SuperH** project; the people who revived SH after patents expired (last SH-2 patent lapsed 2014). Plugged into SH industrial history. | Public via j-core.org / 0pf.org project channels, public talks, riscvnews interview | Med | "Preservation of SH-2-era tooling" framing; they may know surviving Hitachi/industrial SH toolchain hoarders in Japan. Low odds they have SHC themselves. |

Sources: https://gcc.gnu.org/onlinedocs/gcc/Contributors.html · https://gnu.googlesource.com/binutils-gdb/+/00d962d155e1cff6ce144f60f81a416e0f9b872e/binutils/MAINTAINERS · https://lwn.net/Articles/647636/ · https://www.riscvnews.com/posts/j-core-interview · http://0pf.org/j-core.html

---

## CATEGORY 2 — Ex-Sega Die Hard Arcade / Dynamite Deka developers

**Team reality (important):** Die Hard Arcade was built by **Sega AM1** (Japan) with **Sega Technical
Institute (STI)** (USA) contributing art/design/animation. **The programming — i.e. the people who
actually drove the SHC toolchain — was AM1 in Japan.** The reachable-today names (Hector, Hedlund) are
the *American art/design* side and almost certainly never touched the compiler. So there's a split:
the valuable targets are Japanese and hard to reach; the easy targets are low-value for the compiler
but could be relationship bridges to AM1 programmers.

**Credited key staff** (MobyGames / Sega Retro): Director/Designer **Makoto Uchida**; Chief Programmer
**Hiroshi Ando**; Enemy Programmer **Masanobu Yamamoto**; Sub Programmer **Teruhito Abe**; Producers
Roger Hector, Rikiya Nakagawa, Makoto Uchida; Exec Producer Hisashi Suzuki; Music Howard Drossin.

| # | Name | Role / why relevant | Public contact vector | Approachability | Best angle | JP? |
|---|---|---|---|---|---|---|
| L2-a | **Hiroshi Ando** | **Chief Programmer** — the single most likely person to have *used* the actual toolchain and conceivably kept media/notes. | No public profile surfaced (common name; no verified LinkedIn/X found). Realistically reachable only via ex-Sega/AM1 networks or an intermediary. | Low — not publicly locatable | "You wrote the code; do you remember which SHC version AM1 used, or still have the dev disks?" — but you need a bridge to reach him. | **JP needed** |
| L2-b | **Makoto Uchida** | Director/designer, Sega legend (Golden Axe, Altered Beast); long Sega tenure; gives interviews. Not a programmer, but the obvious doorway to ex-AM1 staff. | Public figure via Sega-16 interview, Time Extension features; approachable through press/fan-interview channels rather than direct | Med (as public figure) / Low (direct) | Respectful "celebrate your legacy" framing, then ask who on the AM1 programming team might still have dev tools. Relationship bridge, not the holder. | **JP strongly preferred** |
| L2-c | **Roger Hector** | Producer; **Director of STI**; American, very publicly reachable, does interviews, has LinkedIn. | LinkedIn (`/in/rogerhector` referenced publicly); multiple published interviews (Sega-16, Atari Compendium, DICE speaker page) | **High** | Easiest warm contact in the whole list. Honest framing: "I know STI did art/design and AM1 did the SH programming — could you point me to the AM1 programmers, or anyone who'd know the toolchain?" He's a connector, not a holder. | No (English) |
| L2-d | **Stieg Hedlund** | STI lead artist/designer on DHA; prolific, public, active on LinkedIn; has given interviews incl. detail about working with "lead programmer Hiroshi Ando." | LinkedIn `/in/stieghedlund` (public) | **High** | Same as Hector — reachable, English, remembers the team, but was art/design (zero chance he has the compiler). Use only as a bridge to AM1 / to corroborate who the programmers were. | No (English) |
| L2-e | Masanobu Yamamoto / Teruhito Abe | Other AM1 programmers (enemy / sub). | No public profiles surfaced | Low | Same as Ando — valuable in theory, not locatable publicly. | **JP needed** |

Sources: https://www.mobygames.com/game/12828/die-hard-arcade/credits/arcade/ · https://segaretro.org/Die_Hard_Arcade/Development · https://www.sega-16.com/2019/06/classic-interview-makoto-uchida-die-hard-arcade/ · https://www.timeextension.com/features/celebrating-makoto-uchida-golden-axe-creator-and-one-of-segas-longest-serving-staffers · https://www.sega-16.com/2005/02/interview-roger-hector/ · https://en.wikipedia.org/wiki/Stieg_Hedlund · https://www.linkedin.com/in/stieghedlund/

---

## CATEGORY 3 — Preservation & SDK-collector community (the people who actually surface old SH tools)

**This is where effort pays off.** These individuals and venues are the real-world path by which old
Saturn/ST-V dev tools have historically appeared.

| # | Name / handle | Who / why relevant | Public contact vector | Approachability | Best angle |
|---|---|---|---|---|---|
| L3-a | **sozud** | Preserved the Saturn **compiler binaries** the whole scene uses; runs **`saturn-compiler-repro`** (bit-perfect reconstruction of the Nov-1996 Sega DTS SH compiler) and a Saturn binary splitter; core of sotn-decomp Saturn work. The most compiler-literate person for your exact problem. | GitHub: `github.com/sozud` (issues / profile) | **High** (active, decomp-native) | Fellow-decomper, pure technical framing: "DHA is SH-2/ST-V, same pre-v5.0 SHC wall as PDS — what's the current state of SHC preservation, and is reproduction viable for the 1996 build?" He'll know every real lead. |
| L3-b | **ndiddy** | **Dumped the newest known Saturn SDK** (Sept 1997 JP toolkit); active Saturn preservation. | SegaXtreme profile / the "Newest known Saturn SDK dumped by ndiddy" thread | Med-High | Preservation-peer framing; ask directly whether any SHC (vs Sega's bundled Cygnus GCC) passed through the SDKs he's handled. |
| L3-c | **antime** | Runs `antime.kapsi.fi/sega` — long-standing Saturn docs/tools mirror; veteran of the scene. | Public site + SegaXtreme presence | Med | "Longtime Saturn archivist — any SHC media you've ever seen that wasn't the Cygnus rebrand?" |
| L3-d | **Video Game History Foundation (VGHF)** — Frank Cifaldi et al. | Non-profit running the **Video Game Source Project**; actively hunts source code & dev materials; the institution to route a "who held Sega/Hitachi dev tools" question through. | **`info@gamehistory.org`** (publicly published); source project at gamehistory.org/source | Med | Formal preservation framing. They won't *have* SHC, but they may know ex-Sega contacts and lend legitimacy to a request. Non-commercial, academic tone. |
| L3-e | **Hidden Palace** | Prototype/dev-material preservation community. | Public site/Discord | Med | Already checked by atolm (nothing), but worth a standing "wanted" note; they surface unexpected dev media. |

Sources: https://github.com/sozud · https://deepwiki.com/sozud/saturn-compiler-repro · https://segaxtreme.net/threads/newest-known-saturn-sdk-dumped-by-ndiddy.25116/ · https://antime.kapsi.fi/sega/docs.html · https://gamehistory.org/source/ · https://en.wikipedia.org/wiki/Video_Game_History_Foundation

---

## CATEGORY 4 — Existing appeals (join, don't duplicate)

| # | Lead | Who / why relevant | Contact vector | Approachability | Best angle |
|---|---|---|---|---|---|
| **L4-a** | **`atolm` project (blythos)** — Panzer Dragoon Saga matching decomp | **THE existing appeal for this exact compiler.** Proved PDS used pre-v5.0 SHC (bound to ≤Ver 4.1B, 1996 library stamps), documented the entire dead-end search in `docs/FINDING_THE_COMPILER.md` + `docs/TECHNICAL_NOTES.md`, and has a ready test harness. Target part numbers confirmed: Ver 4 pkg **`P0700CAS4`** (`-MWR` Win95 / `-SLR` Solaris / `-H7R` HP9000); Ver 3 would be `P0700CAS3` (extrapolated). Key insight: target-era SHC shipped on **floppy/DAT, never CD** — look for a box of floppies/a DAT, most plausibly in Japan. | GitHub `github.com/blythos/atolm` — issues / repo contact | **High** (clearly wants leads: "we would like to hear about it") | **Collaborate.** Bring the ST-V/Die Hard Arcade case as a second data point, offer to co-sign a joint appeal, use their 30-min verification. This is your force-multiplier. |
| L4-b | **decomp.me Saturn/SHC contributors** — **masonc15**, **gibletto** | masonc15 added **SHC v5.0 (Release 32) for Saturn** to decomp.me (PR #2115) and runs `tgm2p-decomp` (Psikyo SH-2 arcade, built with SHC v5.0). gibletto runs **`shc-5r26-decomp-sf3`** (decomp of SHC 5r26 itself, CPS3-era). These are the people actively working *with* SHC. | GitHub (`masonc15`, `gibletto`); decomp.me Discord | High (decomp-native) | Technical peer: "You've got SHC v5.0 running on decomp.me — anyone chasing the pre-5.0 builds? DHA needs the 1996 vintage." gibletto decompiling SHC itself is notable — a reconstructed older SHC could eventually come from that direction. |
| L4-c | **sotn-decomp (Castlevania SotN) Saturn team** | Published the Saturn period-compiler recipe atolm reused; active Saturn matching-decomp community (Xeeynamo, sozud, contributors). | GitHub `Xeeynamo/sotn-decomp`, `sozud/sotn-decomp`; Discord | High | Shared-infrastructure framing; overlaps heavily with sozud (L3-a). |

Sources: https://github.com/blythos/atolm (docs/FINDING_THE_COMPILER.md, docs/TECHNICAL_NOTES.md) · https://github.com/decompme/decomp.me/pull/2115 · https://github.com/masonc15/tgm2p-decomp · https://github.com/gibletto/shc-5r26-decomp-sf3 · https://github.com/Xeeynamo/sotn-decomp

---

## CATEGORY 5 — The proper corporate route: Renesas (current SHC IP owner)

**Lineage:** Hitachi semiconductor → **Renesas Technology (2003)** → **Renesas Electronics (2010)**.
Renesas owns the SHC IP today and, notably, **still sells SHC as a live product**: the "C/C++ Compiler
Package for SuperH Family," currently at **V.9** (Windows). Versions **V.5–V.8 are discontinued**; the
1996-era Ver 3/4 builds are far older still and not offered anywhere.

**Realistic odds: low, but it is the one *legitimate* channel to the actual rights-holder, and worth a
formal, explicitly non-commercial inquiry.** Two things cut against it: (1) SHC is still a commercial
product, so Renesas has a live incentive to be protective rather than generous; (2) a 1996 build is
three-plus major versions behind anything they support — it may not even survive in their own archives.

**Precedent — honest read:** Renesas **has** published *legacy source* for its toolchains — but that's
the **GNU/GCC-based** side (GNURX/GNUSH etc., via the KPIT-origin gcc-renesas.com portal), where the
GPL obliges source release anyway. That is **not** a preservation-generosity precedent for the
**proprietary** SHC. I found **no public instance of Renesas (or Hitachi) releasing an old proprietary
dev-tool build for preservation.** So frame it as a *first*, not as "you've done this before."

**Channels (public/official only), best first:**
- **Renesas technical support / contact form** on renesas.com (the C/C++ Compiler Package product page
  has a support/contact path). Route a formal written request here first.
- **Legacy / discontinued-product support** — Renesas maintains discontinued-product pages and a
  (now-ended, Sept 2026) Compiler Maintenance Service; the support org that handled those is the right
  internal audience.
- **Developer relations / PR-press** as a secondary escalation if support deflects.

**Register & framing (important):**
- Formal, concise, business-letter tone. Identify yourself and the project honestly.
- Lead with **preservation + explicitly non-commercial + personal/academic**. State there is no product,
  no redistribution, no commercial use.
- **Offer to sign terms** — an NDA or a restricted non-commercial/preservation license. Signalling
  willingness to be bound removes their easiest reason to say no.
- Ask narrowly: "Does an archival build of SHC Ver 3.x/4.x (part `P0700CAS3`/`P0700CAS4`) survive in
  your archives, and would you consider a non-commercial preservation license or loan for
  decompilation research?" Precision shows you're serious and makes an internal search cheap for them.
- Expect slow or no reply; one polite follow-up, then move on. Don't burn goodwill.

Sources: https://www.renesas.com/en/software-tool/cc-compiler-package-superh-family · https://www.renesas.com/en/software-tool/compiler-maintenance-service-discontinued-product · https://en.wikipedia.org/wiki/SuperH · https://llvm-gcc-renesas.com/rx/legacy-toolchains/ · https://gcc-renesas.com/en/rx/rx-latest-source-code/

---

## RISKS & ETIQUETTE

- **Language.** The highest-value holders (AM1 programmers: Ando, Yamamoto, Abe; Uchida) are
  **Japanese-speaking**. Any direct outreach to them should be in **polite, formal Japanese** (keigo),
  or sent through a JP-fluent intermediary. English-first will read as careless and likely be ignored.
  The American STI staff (Hector, Hedlund) and the decomp/preservation community are English-fine.
- **Corporate vs individual.** Renesas = formal, non-commercial, offer-to-sign register. Individual ex-devs
  = warm, respectful, "I admire your work" register. Don't cross the wires.
- **Don't ask people to pirate.** For ex-devs and Renesas alike, frame around *preservation of their own
  legacy / a dead toolchain*, non-commercial, no redistribution. Never ask anyone to hand over something
  they'd consider a live commercial secret without offering terms.
- **Respect "no."** If anyone has publicly asked not to be contacted, or declines, stop. (None found so far.)
- **Don't duplicate atolm's dead ends.** They already exhausted Hidden Palace, SegaXtreme archives,
  Psy-Q archives, Casio/CQ SH kits, HEW discs, archive.org SDKs. Re-asking those cold wastes goodwill —
  reference their findings instead and ask only the *new* question (industrial/embedded SHC owners, JP
  ex-staff, Renesas archives).
- **Manage your own expectations in writing.** Public copy probably doesn't exist. The win condition is
  most likely a private collector/ex-staffer in Japan or a Renesas exception — both slow, both long odds.

---

## Full ranked list (best bet first)

1. **atolm / blythos** (L4-a) — join the existing, identical appeal; shared test harness. GitHub issue/email. *High.*
2. **sozud** (L3-a) — Saturn compiler preservation lead; `saturn-compiler-repro`. GitHub. *High.*
3. **SegaXtreme SHC/SDK threads** (L3/L5 venue) — the live venue; reaches ndiddy, antime, et al. at once. *High.*
4. **decomp.me SHC crew — masonc15 / gibletto** (L4-b) — actively working with SHC; gibletto decompiling SHC itself. GitHub/Discord. *High.*
5. **Renesas corporate** (Cat 5) — the legitimate IP-owner channel; formal, non-commercial, offer terms. Contact form. *Low odds, high legitimacy.*
6. **Roger Hector** (L2-c) — easiest warm ex-Sega contact; bridge to AM1 programmers. LinkedIn. *High reach / low direct value.*
7. **ndiddy** (L3-b) — dumped newest Saturn SDK; preservation peer. SegaXtreme. *Med-High.*
8. **Stieg Hedlund** (L2-d) — reachable STI vet; corroborates team, bridge only. LinkedIn. *High reach / low value.*
9. **VGHF — info@gamehistory.org** (L3-d) — preservation legitimacy + ex-Sega network. *Med.*
10. **Makoto Uchida** (L2-b) — Sega-legend doorway to ex-AM1; JP. Via press/fan channels. *Med.*
11. **Hiroshi Ando** (L2-a) — chief programmer, likeliest actual holder, but not publicly locatable; JP. *Low reach / high value.*
12. **Kaz Kojima** (L1-a) / **Jeff Dionne** (L1-b) — SH-ecosystem knowledge (GNU/open side), "who to ask." *Med / low value.*
13. **antime** (L3-c), **Hidden Palace** (L3-e), **sotn-decomp team** (L4-c) — standing watch / shared infra. *Med.*

---

## Honest bottom line

- **Cat 1 (SHC authors): effectively empty.** No named SHC engineers are publicly identifiable. Don't chase it.
- **Cat 2 (DHA devs): split.** Real holders are Japanese AM1 programmers, hard to reach, JP-only. Reachable
  names are American art/design — good bridges, won't have the compiler.
- **Cat 3 & 4 (preservation / existing appeals): your best money.** Especially atolm + sozud.
- **Cat 5 (Renesas): legitimate but long-odds, no proprietary-release precedent.** Formal non-commercial ask.
- **The compiler itself is probably not sitting in any public archive.** Plan around collaboration and
  patient outreach, not a quick download.
