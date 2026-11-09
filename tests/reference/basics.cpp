// Reference for tests/basics.grl: meshoptimizer 1.3 (built with -ffp-contract=off) on deterministic
// pseudo-random inputs; prints one line per result, floats as their 32-bit patterns.
//   c++ -O2 -ffp-contract=off -I<meshoptimizer>/src basics.cpp <meshoptimizer>/src/*.cpp -o basics && ./basics > ../basics_expected.txt
#include "meshoptimizer.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <vector>
static uint32_t state = 12345;
static uint32_t rnd() { state = state * 1103515245u + 12345u; return state >> 8; }
static float frand(float scale) { return (float(rnd() % 20001) / 10000.0f - 1.0f) * scale; }
static unsigned fb(float f) { unsigned u; memcpy(&u, &f, 4); return u; }
static void print_bounds(const char* tag, const meshopt_Bounds& b) {
    printf("%s %u %u %u %u %u %u %u %u %u %u %u %d %d %d %d\n", tag, fb(b.center[0]), fb(b.center[1]), fb(b.center[2]), fb(b.radius),
           fb(b.cone_apex[0]), fb(b.cone_apex[1]), fb(b.cone_apex[2]), fb(b.cone_axis[0]), fb(b.cone_axis[1]), fb(b.cone_axis[2]), fb(b.cone_cutoff),
           b.cone_axis_s8[0], b.cone_axis_s8[1], b.cone_axis_s8[2], b.cone_cutoff_s8);
}
int main() {
    for (int round = 0; round < 6; ++round) {
        size_t n = 40 + round * 37;
        std::vector<float> pos(n * 4);
        for (size_t i = 0; i < n; ++i) {
            for (int c = 0; c < 4; ++c) pos[i * 4 + c] = frand(10.0f + round);
            if (i > 3 && rnd() % 4 == 0) memcpy(&pos[i * 4], &pos[(rnd() % i) * 4], 12);  // duplicates
            if (rnd() % 17 == 0) pos[i * 4] = (rnd() % 2) ? 0.0f : -0.0f;
            if (round == 5 && rnd() % 23 == 0) pos[i * 4 + 1] = NAN;
            pos[i * 4 + 3] = std::fabs(pos[i * 4 + 3]);
        }
        printf("pos %zu", n); for (float f : pos) printf(" %u", fb(f)); printf("\n");
        std::vector<unsigned> remap(n);
        meshopt_generatePositionRemap(remap.data(), pos.data(), n, 16);
        printf("posremap %zu", n); for (unsigned r : remap) printf(" %u", r); printf("\n");
        std::vector<unsigned> idx;
        for (size_t i = 0; i < n * 2; ++i) idx.push_back(rnd() % n);
        std::vector<unsigned char> attr(n * 6);
        for (size_t i = 0; i < n * 6; ++i) attr[i] = (unsigned char)(rnd() % 3);
        printf("idx %zu", idx.size()); for (unsigned v : idx) printf(" %u", v); printf("\n");
        printf("attr %zu", attr.size()); for (unsigned char v : attr) printf(" %u", v); printf("\n");
        meshopt_Stream streams[2] = {{pos.data(), 12, 16}, {attr.data(), 5, 6}};
        std::vector<unsigned> vr(n);
        size_t unique = meshopt_generateVertexRemapMulti(vr.data(), idx.data(), idx.size(), n, streams, 2);
        printf("vremap %zu %zu", n, unique); for (unsigned r : vr) printf(" %u", r); printf("\n");
        if (round == 5) continue;  // NaN positions make sphere bounds meaningless
        print_bounds("points", meshopt_computeSphereBounds(pos.data(), n, 16, nullptr, 0));
        print_bounds("spheres", meshopt_computeSphereBounds(pos.data(), n, 16, pos.data() + 3, 16));
        // meshlets: up to 64 local vertices, triangles over them
        for (int m = 0; m < 4; ++m) {
            size_t vc = 3 + rnd() % 30, tc = 1 + rnd() % 40;
            std::vector<unsigned> mv(vc); for (auto& v : mv) v = rnd() % n;
            std::vector<unsigned char> mt(tc * 3); for (auto& t : mt) t = (unsigned char)(rnd() % vc);
            if (m == 3) { for (size_t t = 0; t < tc * 3; t += 3) { mt[t] = 0; mt[t + 1] = (unsigned char)(1 + t / 3 % (vc - 2)); mt[t + 2] = (unsigned char)(mt[t + 1] + 1); } }
            meshopt_Bounds b = meshopt_computeMeshletBounds(mv.data(), mt.data(), tc, pos.data(), n, 16);
            printf("meshlet %zu %zu", vc, tc); for (auto v : mv) printf(" %u", v); for (auto t : mt) printf(" %u", t); printf("\n");
            print_bounds("mbounds", b);
        }
    }
    // A curved patch: a useful (narrow) normal cone.
    {
        std::vector<float> p; std::vector<unsigned> mv; std::vector<unsigned char> mt;
        for (int y = 0; y < 5; ++y) for (int x = 0; x < 5; ++x) { p.push_back(float(x)); p.push_back(float(y)); p.push_back(0.05f * float(x * x + y)); p.push_back(0); mv.push_back(unsigned(y * 5 + x)); }
        for (int y = 0; y < 4; ++y) for (int x = 0; x < 4; ++x) { unsigned char a = (unsigned char)(y * 5 + x); mt.push_back(a); mt.push_back(a + 1); mt.push_back(a + 5); mt.push_back(a + 1); mt.push_back(a + 6); mt.push_back(a + 5); }
        printf("pos %zu", p.size() / 4); for (float f : p) printf(" %u", fb(f)); printf("\n");
        printf("meshlet %zu %zu", mv.size(), mt.size() / 3); for (auto v : mv) printf(" %u", v); for (auto t : mt) printf(" %u", t); printf("\n");
        print_bounds("mbounds", meshopt_computeMeshletBounds(mv.data(), mt.data(), mt.size() / 3, p.data(), 25, 16));
    }
    return 0;
}
