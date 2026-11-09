// Reference for tests/meshlet_utils.grl: meshopt_computeClusterBounds, meshopt_extractMeshletIndices
// and meshopt_optimizeMeshletLevel (levels 0, 1, 3) on spatial meshlets of a height field and a
// torus, plus a cluster with indices colliding in the dedup caches.
//   c++ -O2 -ffp-contract=off -I<meshoptimizer>/src meshlet_utils.cpp <meshoptimizer>/src/*.cpp -o meshlet_utils && ./meshlet_utils > ../meshlet_utils_expected.txt
#include "meshoptimizer.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
static unsigned fb(float f) { unsigned u; memcpy(&u, &f, 4); return u; }
static void bounds(const meshopt_Bounds& b) {
    printf(" %u %u %u %u %u %u %u %u %u %u %u %d %d %d %d", fb(b.center[0]), fb(b.center[1]), fb(b.center[2]), fb(b.radius),
           fb(b.cone_apex[0]), fb(b.cone_apex[1]), fb(b.cone_apex[2]), fb(b.cone_axis[0]), fb(b.cone_axis[1]), fb(b.cone_axis[2]), fb(b.cone_cutoff),
           b.cone_axis_s8[0], b.cone_axis_s8[1], b.cone_axis_s8[2], b.cone_cutoff_s8);
}
static void cluster(const std::vector<float>& pos, const std::vector<unsigned>& ci) {
    printf("cluster %zu", ci.size()); for (unsigned v : ci) printf(" %u", v); printf("\n");
    printf("cbounds"); bounds(meshopt_computeClusterBounds(ci.data(), ci.size(), pos.data(), pos.size() / 4, 16)); printf("\n");
    std::vector<unsigned> mv(ci.size()); std::vector<unsigned char> mt(ci.size());
    size_t vc = meshopt_extractMeshletIndices(mv.data(), mt.data(), ci.data(), ci.size());
    printf("extract %zu", vc); for (size_t i = 0; i < vc; ++i) printf(" %u", mv[i]); for (unsigned char t : mt) printf(" %u", t); printf("\n");
    for (int level : {0, 1, 3}) {
        std::vector<unsigned> v(mv.begin(), mv.begin() + vc); std::vector<unsigned char> t(mt);
        meshopt_optimizeMeshletLevel(v.data(), vc, t.data(), t.size() / 3, level);
        printf("opt %d", level); for (unsigned x : v) printf(" %u", x); for (unsigned char x : t) printf(" %u", x); printf("\n");
    }
}
static void mesh(const std::vector<float>& pos, const std::vector<unsigned>& idx) {
    size_t vc = pos.size() / 4;
    printf("pos %zu", vc); for (float f : pos) printf(" %u", fb(f)); printf("\n");
    size_t bound = meshopt_buildMeshletsBound(idx.size(), 64, 32);
    std::vector<meshopt_Meshlet> ms(bound); std::vector<unsigned> mv(bound * 64); std::vector<unsigned char> mt(bound * 64 * 3);
    size_t n = meshopt_buildMeshletsSpatial(ms.data(), mv.data(), mt.data(), idx.data(), idx.size(), pos.data(), vc, 16, 64, 32, 64, 0.5f);
    for (size_t m = 0; m < n && m < 6; ++m) {
        std::vector<unsigned> ci;
        for (unsigned t = 0; t < ms[m].triangle_count * 3; ++t) ci.push_back(mv[ms[m].vertex_offset + mt[ms[m].triangle_offset + t]]);
        cluster(pos, ci);
    }
}
int main() {
    {
        std::vector<float> pos; std::vector<unsigned> idx; const int n = 21;
        for (int y = 0; y < n; ++y) for (int x = 0; x < n; ++x) { pos.push_back(float(x)); pos.push_back(float(y)); pos.push_back(0.3f * std::sin(float(x) * 0.4f) * std::cos(float(y) * 0.25f)); pos.push_back(0); }
        for (int y = 0; y < n - 1; ++y) for (int x = 0; x < n - 1; ++x) { unsigned a = unsigned(y * n + x); idx.insert(idx.end(), {a, a + 1, a + unsigned(n), a + 1, a + unsigned(n) + 1, a + unsigned(n)}); }
        mesh(pos, idx);
        // indices that collide in the 512- and 1024-entry caches
        std::vector<unsigned> ci = {1, 1025, 2049, 1025, 1, 3073, 2049, 3073, 1, 4097, 1, 1025};
        std::vector<float> big; for (unsigned i = 0; i < 4100; ++i) { big.push_back(float(i % 7)); big.push_back(float(i % 11)); big.push_back(float(i % 5)); big.push_back(0); }
        printf("pos %zu", big.size() / 4); for (float f : big) printf(" %u", fb(f)); printf("\n");
        cluster(big, ci);
    }
    {
        std::vector<float> pos; std::vector<unsigned> idx; const int u = 32, v = 14;
        for (int i = 0; i < u; ++i) for (int j = 0; j < v; ++j) {
            float a = float(i) * 6.2831853f / float(u), b = float(j) * 6.2831853f / float(v);
            pos.push_back((3.0f + std::cos(b)) * std::cos(a)); pos.push_back((3.0f + std::cos(b)) * std::sin(a)); pos.push_back(std::sin(b)); pos.push_back(0);
        }
        for (int i = 0; i < u; ++i) for (int j = 0; j < v; ++j) {
            unsigned a = unsigned(i * v + j), b = unsigned(((i + 1) % u) * v + j), c = unsigned(i * v + (j + 1) % v), d = unsigned(((i + 1) % u) * v + (j + 1) % v);
            idx.insert(idx.end(), {a, b, c, b, d, c});
        }
        mesh(pos, idx);
    }
    return 0;
}
