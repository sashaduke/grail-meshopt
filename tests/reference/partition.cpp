// Reference for tests/partition.grl: meshopt_partitionClusters (meshoptimizer 1.3,
// -ffp-contract=off) on clusters made by meshopt_buildMeshletsSpatial from a height field and a
// torus, with and without positions, for target sizes 4, 8 and 16. Prints the inputs ("pos",
// "clusters": counts then indices) and each result ("parts").
//   c++ -O2 -ffp-contract=off -I<meshoptimizer>/src partition.cpp <meshoptimizer>/src/*.cpp -o partition && ./partition > ../partition_expected.txt
#include "meshoptimizer.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
static unsigned fb(float f) { unsigned u; memcpy(&u, &f, 4); return u; }
static void run(const std::vector<float>& pos, const std::vector<unsigned>& idx) {
    size_t vc = pos.size() / 4;
    size_t bound = meshopt_buildMeshletsBound(idx.size(), 64, 20);
    std::vector<meshopt_Meshlet> ms(bound); std::vector<unsigned> mv(bound * 64); std::vector<unsigned char> mt(bound * 64 * 3);
    size_t n = meshopt_buildMeshletsSpatial(ms.data(), mv.data(), mt.data(), idx.data(), idx.size(), pos.data(), vc, 16, 64, 20, 64, 0.5f);
    std::vector<unsigned> ci, cc;
    for (size_t m = 0; m < n; ++m) {
        for (unsigned t = 0; t < ms[m].triangle_count * 3; ++t) ci.push_back(mv[ms[m].vertex_offset + mt[ms[m].triangle_offset + t]]);
        cc.push_back(ms[m].triangle_count * 3);
    }
    printf("pos %zu", vc); for (float f : pos) printf(" %u", fb(f)); printf("\n");
    printf("clusters %zu", cc.size()); for (unsigned c : cc) printf(" %u", c); for (unsigned v : ci) printf(" %u", v); printf("\n");
    for (size_t target : {4, 8, 16})
        for (int spatial = 0; spatial < 2; ++spatial) {
            std::vector<unsigned> part(cc.size());
            size_t parts = meshopt_partitionClusters(part.data(), ci.data(), ci.size(), cc.data(), cc.size(), spatial ? pos.data() : nullptr, vc, 16, target);
            printf("parts %zu %d %zu", target, spatial, parts); for (unsigned p : part) printf(" %u", p); printf("\n");
        }
}
int main() {
    {
        std::vector<float> pos; std::vector<unsigned> idx; const int n = 41;
        for (int y = 0; y < n; ++y) for (int x = 0; x < n; ++x) { pos.push_back(float(x)); pos.push_back(float(y)); pos.push_back(0.3f * std::sin(float(x) * 0.37f) * std::cos(float(y) * 0.21f)); pos.push_back(0); }
        for (int y = 0; y < n - 1; ++y) for (int x = 0; x < n - 1; ++x) { unsigned a = unsigned(y * n + x); idx.insert(idx.end(), {a, a + 1, a + unsigned(n), a + 1, a + unsigned(n) + 1, a + unsigned(n)}); }
        run(pos, idx);
    }
    {
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
