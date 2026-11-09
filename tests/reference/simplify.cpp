// Reference for tests/simplify.grl: meshopt_simplifyWithAttributes (meshoptimizer 1.3,
// -ffp-contract=off) on a seamed height field and a torus, under Abundance's cluster-LOD options
// (Sparse | ErrorAbsolute, plus Clamped / Permissive / Regularize / PreserveFolds) and the plain
// defaults, with and without attributes and locks. Prints inputs and the result indices + error.
//   c++ -O2 -ffp-contract=off -I<meshoptimizer>/src simplify.cpp <meshoptimizer>/src/*.cpp -o simplify && ./simplify > ../simplify_expected.txt
#include "meshoptimizer.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
static uint32_t state = 7;
static uint32_t rnd() { state = state * 1103515245u + 12345u; return state >> 8; }
static unsigned fb(float f) { unsigned u; memcpy(&u, &f, 4); return u; }
static void run(const std::vector<float>& pos, const std::vector<unsigned>& idx, const std::vector<float>& attrs, const std::vector<unsigned char>& lock) {
    size_t vc = pos.size() / 4;
    printf("pos %zu", vc); for (float f : pos) printf(" %u", fb(f)); printf("\n");
    printf("idx %zu", idx.size()); for (unsigned v : idx) printf(" %u", v); printf("\n");
    printf("attr %zu", attrs.size()); for (float f : attrs) printf(" %u", fb(f)); printf("\n");
    printf("lock %zu", lock.size()); for (unsigned char v : lock) printf(" %u", v); printf("\n");
    const float weights[3] = {0.5f, 0.f, 2.f};
    const unsigned base = meshopt_SimplifySparse | meshopt_SimplifyErrorAbsolute;
    const unsigned opts[6] = {0, base, base | meshopt_SimplifyErrorClamped, base | meshopt_SimplifyPermissive, base | meshopt_SimplifyRegularize | meshopt_SimplifyPreserveFolds, base | meshopt_SimplifyLockBorder};
    for (int withattr = 0; withattr < 2; ++withattr)
        for (unsigned o : opts)
            for (float ratio : {0.5f, 0.1f}) {
                size_t target = size_t(float(idx.size() / 3) * ratio) * 3;
                float terr = (o & meshopt_SimplifyErrorAbsolute) ? 0.5f : 0.05f;
                std::vector<unsigned> out(idx.size());
                float err = 0;
                size_t n = meshopt_simplifyWithAttributes(out.data(), idx.data(), idx.size(), pos.data(), vc, 16,
                    withattr ? attrs.data() : NULL, 12, withattr ? weights : NULL, withattr ? 3 : 0, lock.empty() ? NULL : lock.data(), target, terr, o, &err);
                printf("simp %d %u %u %u %zu %u", withattr, o, fb(float(target)), fb(terr), n, fb(err));
                for (size_t i = 0; i < n; ++i) printf(" %u", out[i]);
                printf("\n");
            }
}
int main() {
    {   // seamed height field with a hole
        std::vector<float> pos; std::vector<unsigned> idx; const int n = 17;
        for (int y = 0; y < n; ++y) for (int x = 0; x < n; ++x) { pos.push_back(float(x)); pos.push_back(float(y)); pos.push_back(0.4f * std::sin(float(x) * 0.5f) * std::cos(float(y) * 0.3f)); pos.push_back(0); }
        unsigned seam = unsigned(pos.size() / 4);
        for (int y = 0; y < n; ++y) { pos.push_back(8.0f); pos.push_back(float(y)); pos.push_back(0.4f * std::sin(4.0f) * std::cos(float(y) * 0.3f)); pos.push_back(0); }
        for (int y = 0; y < n - 1; ++y) for (int x = 0; x < n - 1; ++x) {
            if (x == 3 && y == 5) continue;
            auto at = [&](int xx, int yy) { return (xx == 8 && x >= 8) ? seam + unsigned(yy) : unsigned(yy * n + xx); };
            unsigned a = at(x, y), b = at(x + 1, y), c = at(x, y + 1), d = at(x + 1, y + 1);
            idx.insert(idx.end(), {a, b, c, b, d, c});
        }
        std::vector<float> attrs(pos.size() / 4 * 3);
        for (size_t i = 0; i < attrs.size() / 3; ++i) { attrs[i * 3] = pos[i * 4] / 16.f; attrs[i * 3 + 1] = float(rnd() % 100) / 100.f; attrs[i * 3 + 2] = (i >= seam) ? 1.f : 0.f; }
        std::vector<unsigned char> locks(pos.size() / 4);
        for (size_t i = 0; i < locks.size(); ++i) locks[i] = (rnd() % 29 == 0) ? 1 : 0;
        run(pos, idx, attrs, std::vector<unsigned char>());
        run(pos, idx, attrs, locks);
    }
    {   // torus
        std::vector<float> pos; std::vector<unsigned> idx; const int u = 32, v = 14;
        for (int i = 0; i < u; ++i) for (int j = 0; j < v; ++j) {
            float a = float(i) * 6.2831853f / float(u), b = float(j) * 6.2831853f / float(v);
            pos.push_back((3.0f + std::cos(b)) * std::cos(a)); pos.push_back((3.0f + std::cos(b)) * std::sin(a)); pos.push_back(std::sin(b)); pos.push_back(0);
        }
        for (int i = 0; i < u; ++i) for (int j = 0; j < v; ++j) {
            unsigned a = unsigned(i * v + j), b = unsigned(((i + 1) % u) * v + j), c = unsigned(i * v + (j + 1) % v), d = unsigned(((i + 1) % u) * v + (j + 1) % v);
            idx.insert(idx.end(), {a, b, c, b, d, c});
        }
        std::vector<float> attrs(pos.size() / 4 * 3);
        for (size_t i = 0; i < attrs.size() / 3; ++i) { attrs[i * 3] = pos[i * 4 + 2]; attrs[i * 3 + 1] = 0.f; attrs[i * 3 + 2] = float(i % 5) * 0.2f; }
        run(pos, idx, attrs, std::vector<unsigned char>());
    }
    return 0;
}
