#include <stdint.h>
#include <stdio.h>

#define SHOW_ROUNDS 0   // 1 = print the state after every round of every permutation, 0 = only the phases 

typedef struct { uint8_t x[5]; } ascon_state;


static uint8_t ror(uint8_t v, int n) {
    return (uint8_t)((v >> n) | (v << (8 - n))); // rotate right by n in 1..7
}

/* one word per row, one bit per column: reading a column top to bottom (x0..x4) gives the 5-bit S-box input */
static void print_state(const char *label, const ascon_state *s) {
    printf("%s\n", label);
    printf("        7 6 5 4  3 2 1 0\n");
    for (int i = 0; i < 5; i++) {
        printf("  x%d |  ", i);
        for (int b = 7; b >= 0; b--)
            printf("%d%s", (s->x[i] >> b) & 1, b == 4 ? "  " : (b ? " " : ""));
        printf("   (%02x)  %s\n", s->x[i], i < 2 ? "rate" : "capacity");
    }
    printf("\n");
}

static void print_bin(uint8_t v) {
    for (int b = 7; b >= 0; b--) {
        putchar('0' + ((v >> b) & 1));
        if (b == 4) putchar(' ');
    }
}

/* runs the last `rounds` of the 12 round constants; verbose = print the state after every round */
static void ascon_p(ascon_state *s, int rounds, int verbose) {
    /* constant addition -> S-box -> linear layer, per round */
    uint8_t x0 = s->x[0];
    uint8_t x1 = s->x[1];
    uint8_t x2 = s->x[2];
    uint8_t x3 = s->x[3];
    uint8_t x4 = s->x[4]; //Local variables for helpers

    for(int i = 0; i < rounds; i++){
        //Step 1: constant addition (the last `rounds` of the 12 constants)
        int r = 12 - rounds + i;
        uint8_t constant = (uint8_t)(((0xf - r) << 4) | r);
        x2 ^= constant;

        //Step 2: substitution layer (bitsliced S-box)
        x0 ^= x4;
        x4 ^= x3;
        x2 ^= x1;

        uint8_t t0 = x0 ^ (~x1 & x2);
        uint8_t t1 = x1 ^ (~x2 & x3);
        uint8_t t2 = x2 ^ (~x3 & x4);
        uint8_t t3 = x3 ^ (~x4 & x0);
        uint8_t t4 = x4 ^ (~x0 & x1);

        t1^=t0;
        t0^=t4;
        t3^=t2;
        t2 = ~t2;

        //Step 3: linear layer (rotation amounts of the real Ascon, reduced mod 8)
        x0 = t0 ^ ror(t0, 3) ^ ror(t0, 4);
        x1 = t1 ^ ror(t1, 5) ^ ror(t1, 7);
        x2 = t2 ^ ror(t2, 1) ^ ror(t2, 6);
        x3 = t3 ^ ror(t3, 2) ^ ror(t3, 1);
        x4 = t4 ^ ror(t4, 7) ^ ror(t4, 1);

        if (verbose) {
            ascon_state tmp = {{ x0, x1, x2, x3, x4 }};
            char label[32];
            snprintf(label, sizeof label, "p^%d, round %d of %d", rounds, i + 1, rounds);
            print_state(label, &tmp);
        }
    }
    s->x[0] = x0; s->x[1] = x1; s->x[2] = x2; s->x[3] = x3; s->x[4] = x4;

}

int main(void) {
    const uint8_t IV = 0xC8;              /* toy IV: high nibble a = 12, low nibble b = 8 */
    const uint8_t key[2] = { 0x5A, 0x3C };
    const uint8_t nonce[2] = { 0xA1, 0xB2 };
    const uint8_t msg[2] = { 'h', 'i' };      /* 0x68 0x69 = exactly one 16-bit block (rate = x0,x1) */
    uint8_t ct[2], tag[2];
    ascon_state s;

    /* ---------------- initialization ---------------- */
    s.x[0] = IV;
    s.x[1] = key[0];   s.x[2] = key[1];
    s.x[3] = nonce[0]; s.x[4] = nonce[1];
    print_state("INIT: state = IV | K | N", &s);

    ascon_p(&s, 12, SHOW_ROUNDS);
    print_state("INIT: after p^12", &s);

    s.x[3] ^= key[0];  s.x[4] ^= key[1];
    print_state("INIT: after XOR of K into x3, x4", &s);

    /* ---------------- associated data: none, only the domain separation bit ---------------- */
    s.x[4] ^= 1;
    print_state("AD: none, domain separation (x4 ^= 1)", &s);

    /* ---------------- plaintext block 1: "hi" ---------------- */
    s.x[0] ^= msg[0];  s.x[1] ^= msg[1];
    ct[0] = s.x[0];    ct[1] = s.x[1];          /* ciphertext = the new rate */
    print_state("ENC block 1: after XOR of \"hi\" into x0, x1", &s);

    ascon_p(&s, 8, SHOW_ROUNDS);
    print_state("ENC block 1: after p^8", &s);

    /* ---------------- block 2: padding only (0x80 0x00), no ciphertext, no p^8 afterwards ---------------- */
    s.x[0] ^= 0x80;    s.x[1] ^= 0x00;
    print_state("ENC block 2: after XOR of the padding block 80 00", &s);

    /* ---------------- finalization ---------------- */
    s.x[2] ^= key[0];  s.x[3] ^= key[1];
    print_state("FINAL: after XOR of K into x2, x3", &s);

    ascon_p(&s, 12, SHOW_ROUNDS);
    print_state("FINAL: after p^12", &s);

    tag[0] = s.x[3] ^ key[0];
    tag[1] = s.x[4] ^ key[1];

    printf("plaintext  : "); print_bin(msg[0]); printf("  "); print_bin(msg[1]); printf("   (hi)\n");
    printf("key        : "); print_bin(key[0]); printf("  "); print_bin(key[1]); printf("   (%02x %02x)\n", key[0], key[1]);
    printf("IV         : "); print_bin(IV); printf("   (%02x)\n", IV);
    printf("nonce      : "); print_bin(nonce[0]); printf("  "); print_bin(nonce[1]); printf("   (%02x %02x)\n", nonce[0], nonce[1]);
    printf("ciphertext : "); print_bin(ct[0]);  printf("  "); print_bin(ct[1]);  printf("   (%02x %02x)\n", ct[0], ct[1]);
    printf("tag        : "); print_bin(tag[0]); printf("  "); print_bin(tag[1]); printf("   (%02x %02x)\n", tag[0], tag[1]);
    return 0;
}