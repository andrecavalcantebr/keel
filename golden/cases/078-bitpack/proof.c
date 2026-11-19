/* proof.c — harness. The layout is checked byte by byte against bytes worked
   out by hand, not against what the verbs read back; and `partition` is checked
   for the one thing that matters to `parallel`: two parts never touch the same
   byte (spec §5.8, rule 4). */

#include "keel.type.h"
#include "app/app_packed.h"
#include "keel/keel_arena.h"
#include "keel/keel_bitbuffer_3_u8.h"
#include "keel/keel_bitslice_3_u8.h"
#include <stdio.h>
#include <string.h>

#define NEED(c, n) do { if (!(c)) { fprintf(stderr, "proof %d: %s\n", (n), #c); return (n); } } while (0)

int main(void) {
    /* 0..7 at three bits, least significant bit first, are 0x88 0xC6 0xFA:
       byte 0 = 0 | 1<<3 | (2&3)<<6; byte 1 = 2>>2 | 3<<1 | 4<<4 | (5&1)<<7;
       byte 2 = 5>>1 | 6<<2 | 7<<5 */
    u8 raw[3] = {0};
    app_packed_layout(raw);
    NEED(raw[0] == 0x88 && raw[1] == 0xC6 && raw[2] == 0xFA, 1);

    static u8 store[16384];
    keel_arena a;
    NEED(keel_arena_from_array(&a, store, sizeof store), 2);
    NEED(app_packed_widths(&a) == 0, 3);
    NEED(app_packed_views(&a) == 0, 4);
    NEED(app_packed_stack(&a) == 0, 5);
    NEED(app_packed_cloned(&a) == 0, 6);

    /* sum of q*(i+1) for q = i % 16, i < 30, then the view [4, 10): 4..9 */
    u32 want = 0;
    for (u32 i = 0; i < 30; ++i) want += (i % 16) * (i + 1);
    for (u32 i = 4; i < 10; ++i) want += i % 16;
    NEED(app_packed_total(&a) == want, 7);

    /* partition: for every n and k the parts cover the buffer, in order, and a part
       after the first starts on a byte boundary, so no byte belongs to two parts.
       Run over a whole buffer and over a view that starts in the middle of a byte. */
    for (size_t n = 0; n <= 70; n += 7) {
        for (size_t k = 1; k <= 9; ++k) {
            for (int mid = 0; mid < 2; ++mid) {
                keel_bitbuffer_3_u8 g = keel_bitbuffer_3_u8_make(&a, n + 5);
                for (size_t i = 0; i < n + 5; ++i) keel_bitbuffer_3_u8_push(&g, (u8)(i % 8));
                keel_bitslice_3_u8 whole = mid ? keel_bitbuffer_3_u8_as_slice2(&g, 5, n + 5)
                                               : keel_bitbuffer_3_u8_as_slice2(&g, 0, n);
                size_t base_bit = mid ? 15 : 0;            /* the first element of the view, in bits */
                size_t covered = 0, last_byte_end = 0;
                for (size_t w = 0; w < k; ++w) {
                    keel_bitslice_3_u8 p = keel_bitslice_3_u8_partition(whole, k, w);
                    if (p.len == 0) continue;
                    size_t start = (size_t)(p.words - g.words) * 8 + p.bit;
                    NEED(start == base_bit + covered * 3, 8);          /* in order, no gap */
                    if (covered > 0) NEED(start % 8 == 0 && start / 8 >= last_byte_end, 9);
                    covered += p.len;
                    last_byte_end = (start + p.len * 3 + 7) / 8;
                }
                NEED(covered == n, 10);
            }
        }
    }
    puts("ok");
    return 0;
}
