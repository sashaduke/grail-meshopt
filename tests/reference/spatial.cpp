// Reference for tests/spatial.grl: meshopt_spatialSortRemap and meshopt_spatialClusterPoints
// (meshoptimizer 1.3, -ffp-contract=off) on random point sets, including duplicates and a flat set.
//   c++ -O2 -ffp-contract=off -I<meshoptimizer>/src spatial.cpp <meshoptimizer>/src/*.cpp -o spatial && ./spatial > ../spatial_expected.txt
#include "meshoptimizer.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <vector>
static uint32_t state = 4242;
static uint32_t rnd() { state = state * 1103515245u + 12345u; return state >> 8; }
static unsigned fb(float f) { unsigned u; memcpy(&u, &f, 4); return u; }
int main() {
    for (int round = 0; round < 6; ++round) {
        size_t n = 50 + round * 113;
        std::vector<float> p(n * 4);
        for (size_t i = 0; i < n; ++i) {
            for (int c = 0; c < 3; ++c) p[i * 4 + c] = float(rnd() % 20001) / 100.0f - 100.0f;
            if (round == 4) p[i * 4 + 2] = 1.5f;
            if (i > 2 && rnd() % 5 == 0) memcpy(&p[i * 4], &p[(rnd() % i) * 4], 12);
            p[i * 4 + 3] = 0;
        }
        if (round == 5) for (size_t i = 0; i < n; ++i) for (int c = 0; c < 3; ++c) p[i * 4 + c] = 7.0f;
        printf("pos %zu", n); for (float f : p) printf(" %u", fb(f)); printf("\n");
        std::vector<unsigned> r(n);
        meshopt_spatialSortRemap(r.data(), p.data(), n, 16);
        printf("sort %zu", n); for (unsigned v : r) printf(" %u", v); printf("\n");
        for (size_t cs : {1, 7, 32}) {
            meshopt_spatialClusterPoints(r.data(), p.data(), n, 16, cs);
            printf("cluster %zu", cs); for (unsigned v : r) printf(" %u", v); printf("\n");
        }
    }
    return 0;
}
