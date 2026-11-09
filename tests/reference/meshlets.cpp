// Reference for tests/meshlets.grl: meshopt_buildMeshletsSpatial (meshoptimizer 1.3,
// -ffp-contract=off) on a height-field grid, a random triangle soup and a torus, under Abundance's
// parameters (128 vertices, 42..128 triangles, fill 0.5) and three others. Prints the inputs
// ("pos", "idx") and then each result as "meshlets", "mv" and "mt" lines.
//   c++ -O2 -ffp-contract=off -I<meshoptimizer>/src meshlets.cpp <meshoptimizer>/src/*.cpp -o meshlets && ./meshlets > ../meshlets_expected.txt
#include "meshoptimizer.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <vector>
static uint32_t state = 777;
static uint32_t rnd() { state = state * 1103515245u + 12345u; return state >> 8; }
static unsigned fb(float f) { unsigned u; memcpy(&u, &f, 4); return u; }
static void run(const std::vector<float>& pos, const std::vector<unsigned>& idx) {
    size_t vc = pos.size() / 4;
    printf("pos %zu", vc); for (float f : pos) printf(" %u", fb(f)); printf("\n");
    printf("idx %zu", idx.size()); for (unsigned v : idx) printf(" %u", v); printf("\n");
    const size_t params[4][3] = {{128, 42, 128}, {64, 64, 124}, {256, 32, 256}, {32, 10, 40}};
    const float fills[4] = {0.5f, 0.f, 0.25f, 0.5f};
    for (int p = 0; p < 4; ++p) {
        size_t maxv = params[p][0], mint = params[p][1], maxt = params[p][2];
        size_t bound = meshopt_buildMeshletsBound(idx.size(), maxv, mint);
        std::vector<meshopt_Meshlet> ms(bound);
        std::vector<unsigned> mv(bound * maxv);
        std::vector<unsigned char> mt(bound * maxt * 3);
        size_t n = meshopt_buildMeshletsSpatial(ms.data(), mv.data(), mt.data(), idx.data(), idx.size(), pos.data(), vc, 16, maxv, mint, maxt, fills[p]);
        const meshopt_Meshlet& last = ms[n - 1];
        size_t nv = last.vertex_offset + last.vertex_count, nt = last.triangle_offset + last.triangle_count * 3;
        printf("meshlets %zu %zu %zu %u %zu", maxv, mint, maxt, fb(fills[p]), n);
        for (size_t i = 0; i < n; ++i) printf(" %u %u %u %u", ms[i].vertex_offset, ms[i].triangle_offset, ms[i].vertex_count, ms[i].triangle_count);
        printf("\n");
        printf("mv %zu", nv); for (size_t i = 0; i < nv; ++i) printf(" %u", mv[i]); printf("\n");
        printf("mt %zu", nt); for (size_t i = 0; i < nt; ++i) printf(" %u", mt[i]); printf("\n");
    }
}
int main() {
    {   // height-field grid
        std::vector<float> pos; std::vector<unsigned> idx; const int n = 41;
        for (int y = 0; y < n; ++y) for (int x = 0; x < n; ++x) { pos.push_back(float(x)); pos.push_back(float(y)); pos.push_back(0.3f * std::sin(float(x) * 0.37f) * std::cos(float(y) * 0.21f)); pos.push_back(0); }
        for (int y = 0; y < n - 1; ++y) for (int x = 0; x < n - 1; ++x) { unsigned a = unsigned(y * n + x); idx.insert(idx.end(), {a, a + 1, a + unsigned(n), a + 1, a + unsigned(n) + 1, a + unsigned(n)}); }
        run(pos, idx);
    }
    {   // random soup
        std::vector<float> pos; std::vector<unsigned> idx; const size_t vc = 300;
        for (size_t i = 0; i < vc; ++i) { for (int c = 0; c < 3; ++c) pos.push_back(float(rnd() % 10001) / 500.0f - 10.0f); pos.push_back(0); }
        for (size_t t = 0; t < 700; ++t) for (int c = 0; c < 3; ++c) idx.push_back(rnd() % vc);
        run(pos, idx);
    }
    {   // torus
        std::vector<float> pos; std::vector<unsigned> idx; const int u = 48, v = 20;
        for (int i = 0; i < u; ++i) for (int j = 0; j < v; ++j) {
            float a = float(i) * 6.2831853f / float(u), b = float(j) * 6.2831853f / float(v);
            pos.push_back((3.0f + std::cos(b)) * std::cos(a)); pos.push_back((3.0f + std::cos(b)) * std::sin(a)); pos.push_back(std::sin(b)); pos.push_back(0);
        }
        for (int i = 0; i < u; ++i) for (int j = 0; j < v; ++j) {
            unsigned a = unsigned(i * v + j), b = unsigned(((i + 1) % u) * v + j), c = unsigned(i * v + (j + 1) % v), d = unsigned(((i + 1) % u) * v + (j + 1) % v);
            idx.insert(idx.end(), {a, b, c, b, d, c});
        }
        run(pos, idx);
    }
    return 0;
}
