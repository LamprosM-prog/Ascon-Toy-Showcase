#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct { uint64_t x[5]; } ascon_state;


static uint64_t ror(uint64_t v, int n) {
    return (v >> n) | (v << (64 - n)); // rotate right by n in 1,63
}

/* TODO stage 1: run the last `rounds` of the 12 round constants */
static void ascon_p(ascon_state *s, int rounds) {
    /* constant addition -> S-box -> linear layer, per round */
    uint64_t x0 = s->x[0];
    uint64_t x1 = s->x[1];
    uint64_t x2 = s->x[2];
    uint64_t x3 = s->x[3];
    uint64_t x4 = s->x[4]; //Local variables for helpers
    
    for(int i = 0; i < rounds; i++){
        //Step 1
        uint64_t constant = ((0xf - i) << 4) | i; //as taken from the github page
        x2 ^= constant;


    }


}

/* TODO stage 2: byte <-> uint64 helpers (check endianness in the spec) */

/* TODO stage 3 */
static void ascon_hash256(const uint8_t *msg, size_t len, uint8_t out[32]) {
}

static void print_hex(const uint8_t *b, size_t n) {
    for (size_t i = 0; i < n; i++){
        printf("%02x", b[i]);
    } 
    printf("\n");
}

int main(void) {
    uint8_t out[32];
    ascon_hash256((const uint8_t *)"", 0, out);   /* KAT Count = 1 */
    print_hex(out, 32);
    /* compare with the MD line from the KAT file */
    return 0;
}