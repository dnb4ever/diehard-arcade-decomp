/* 0x2217d62  NOT RECONSTRUCTED. This fn.py "function" could not be decompiled by
   m2c ("Cannot find branch target .L217ed2") and is in fact several real routines
   mis-merged into one span. It is dominated by bsrf/braf PC-relative calls into
   HWRAM runtime routines (e.g. pool deltas 0xffffe3da, 0x00020416, 0xffffcf2a,
   0x00024ab2 ...), whose targets depend on exact PC layout and are not present in
   this cart image. Additionally, fn.py attributes a 10-byte prefix to this span
   that is really the shared literal pool (0x0609742c, 0x060b659c) of 0x2217c10 /
   0x2217ca2; real code begins ~0x2217d6c. BLOCKER: unmatchable without the HWRAM
   runtime symbol map and a corrected function boundary split. */
void f_217d62(void) {
}
