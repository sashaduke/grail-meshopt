// Reference for tests/simplify_sloppy.grl: meshopt_simplifySloppy (several targets and error limits,
// with and without locked vertices) and meshopt_simplifyScale on a height field and a torus.
//   c++ -O2 -ffp-contract=off -I<meshoptimizer>/src simplify_sloppy.cpp <meshoptimizer>/src/*.cpp -o simplify_sloppy && ./simplify_sloppy > ../simplify_sloppy_expected.txt
#include "meshoptimizer.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
static unsigned fb(float f) { unsigned u; memcpy(&u, &f, 4); return u; }
static void mesh(const std::vector<float>& pos, const std::vector<unsigned>& idx) {
    size_t vc = pos.size() / 4;
    printf("pos %zu", vc); for (float f : pos) printf(" %u", fb(f)); printf("\n");
    printf("idx %zu", idx.size()); for (unsigned i : idx) printf(" %u", i); printf("\n");
    printf("scale %u\n", fb(meshopt_simplifyScale(pos.data(), vc, 16)));
    std::vector<unsigned char> lock(vc, 0);
    for (size_t i = 0; i < vc; i += 7) lock[i] = meshopt_SimplifyVertex_Lock;
    for (int locked = 0; locked < 2; ++locked) {
        if (locked) { printf("lock %zu", vc); for (unsigned char l : lock) printf(" %u", l); printf("\n"); }
        else printf("lock 0\n");
        const size_t fractions[] = {2, 4, 10, 40, 200};
        const float errors[] = {0.0f, 0.01f, 0.05f, 0.3f, 2.0f};
        for (size_t f : fractions) for (float e : errors) {
            size_t target = idx.size() / f / 3 * 3;
            std::vector<unsigned> out(idx.size());
            float err = 0;
            size_t n = meshopt_simplifySloppy(out.data(), idx.data(), idx.size(), pos.data(), vc, 16, locked ? lock.data() : nullptr, target, e, &err);
            printf("sloppy %zu %u %zu %u", target, fb(e), n, fb(err)); for (size_t i = 0; i < n; ++i) printf(" %u", out[i]); printf("\n");
        }
    }
}
int main() {
    {
        std::vector<float> pos; std::vector<unsigned> idx; const int n = 33;
        for (int y = 0; y < n; ++y) for (int x = 0; x < n; ++x) { pos.push_back(float(x) * 0.5f); pos.push_back(float(y) * 0.5f); pos.push_back(1.3f * std::sin(float(x) * 0.3f) * std::cos(float(y) * 0.21f)); pos.push_back(0); }
        for (int y = 0; y < n - 1; ++y) for (int x = 0; x < n - 1; ++x) { unsigned a = unsigned(y * n + x); idx.insert(idx.end(), {a, a + 1, a + unsigned(n), a + 1, a + unsigned(n) + 1, a + unsigned(n)}); }
        mesh(pos, idx);
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
        mesh(pos, idx);
    }
    return 0;
}
