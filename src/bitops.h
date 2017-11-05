#pragma once

// get bit mask for n-th bit
#define BIT(n) (1 << (n))

// bitmask operations

// get bits by mask
#define BITMGET(x, m) ((x) & (m))
// set bits by mask
#define BITMSET(x, m) ((x) | (m))
// clear bits by mask
#define BITMCLR(x, m) ((x) & ~(m))
// change values of bits by mask
#define BITMCHG(x, m, v) ((v) ? BITMSET(x, m) : BITMCLR(x, m))
// flip bits by mask
#define BITMTGL(x, m) (BITMGET(x, m) ? BITMCLR(x, m) : BITMSET(x, m))

// bit operations

// get n-th bit
#define BITGET(x, n) BITMGET(x, BIT(n))
// set n-th bit
#define BITSET(x, n) BITMSET(x, BIT(n))
// clear n-th bit
#define BITCLR(x, n) BITMCLR(x, BIT(n))
// change n-th bit
#define BITCHG(x, n, v) BITMCHG(x, BIT(n), v)
// flip n-th bit
#define BITTGL(x, n) BITMTGL(x, BIT(n))
