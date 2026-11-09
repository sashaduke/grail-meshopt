// Reference for tests/simplify_quadrics.grl: meshoptimizer 1.3's simplifier quadric fills (face,
// vertex regularization, border/seam edges, folds, attributes), reached by including
// simplifier.cpp. Prints the inputs and every quadric float as its bit pattern.
//   c++ -O2 -ffp-contract=off -I<meshoptimizer>/src simplify_quadrics.cpp <meshoptimizer>/src/allocator.cpp -o simplify_quadrics && ./simplify_quadrics > ../simplify_quadrics_expected.txt
#include "simplifier.cpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
static uint32_t state = 99;
static uint32_t rnd() { state = state * 1103515245u + 12345u; return state >> 8; }
static unsigned fb(float f) { unsigned u; memcpy(&u, &f, 4); return u; }
using namespace meshopt;
static void run(const std::vector<float>& pos, std::vector<unsigned> idx, const std::vector<unsigned char>& lock, unsigned options) {
    size_t vc = pos.size() / 4;
    printf("pos %zu", vc); for (float f : pos) printf(" %u", fb(f)); printf("\n");
    printf("idx %zu", idx.size()); for (unsigned v : idx) printf(" %u", v); printf("\n");
    printf("lock %zu", lock.size()); for (unsigned char v : lock) printf(" %u", v); printf("\n");
    meshopt_Allocator allocator;
    size_t n = vc;
    unsigned int* sparse = NULL;
    if (options & meshopt_SimplifySparse) sparse = buildSparseRemap(idx.data(), idx.size(), vc, &n, allocator);
    EdgeAdjacency adj = {};
    prepareEdgeAdjacency(adj, idx.size(), n, allocator);
    updateEdgeAdjacency(adj, idx.data(), idx.size(), n, NULL);
    std::vector<unsigned> remap(n), wedge(n), loop(n), loopback(n);
    std::vector<unsigned char> kind(n);
    buildPositionRemap(remap.data(), wedge.data(), pos.data(), n, 16, sparse, allocator);
    classifyVertices(kind.data(), loop.data(), loopback.data(), n, adj, remap.data(), wedge.data(), lock.empty() ? NULL : lock.data(), sparse, options);
    std::vector<Vector3> vp(n);
    float offset[3];
    float scale = rescalePositions(vp.data(), pos.data(), n, 16, sparse, offset);
    const float weights[3] = {0.5f, 0.f, 2.f};
    std::vector<float> attrs(vc * 3);
    for (size_t i = 0; i < vc; ++i) for (int k = 0; k < 3; ++k) attrs[i * 3 + k] = float((i * 7 + k * 3) % 11) * 0.1f;
    unsigned int arem[3]; size_t ac = 0;
    for (unsigned k = 0; k < 3; ++k) if (weights[k] > 0) arem[ac++] = k;
    std::vector<float> va(n * ac);
    rescaleAttributes(va.data(), attrs.data(), n, 12, weights, ac, arem, sparse);
    std::vector<Quadric> vq(n), aq(n); std::vector<QuadricGrad> ag(n * ac);
    memset(vq.data(), 0, n * sizeof(Quadric)); memset(aq.data(), 0, n * sizeof(Quadric)); memset(ag.data(), 0, n * ac * sizeof(QuadricGrad));
    fillFaceQuadrics(vq.data(), NULL, idx.data(), idx.size(), vp.data(), remap.data());
    fillVertexQuadrics(vq.data(), vp.data(), n, remap.data(), lock.empty() ? NULL : lock.data(), sparse, options);
    fillEdgeQuadrics(vq.data(), idx.data(), idx.size(), vp.data(), remap.data(), kind.data(), loop.data(), loopback.data());
    if (options & meshopt_SimplifyPreserveFolds) fillFoldQuadrics(vq.data(), adj, vp.data(), n, remap.data(), wedge.data());
    fillAttributeQuadrics(aq.data(), ag.data(), idx.data(), idx.size(), vp.data(), va.data(), ac);
    printf("quad %u %zu %u", options, n, fb(scale));
    for (size_t i = 0; i < n; ++i) { const float* q = &vq[i].a00; for (int k = 0; k < 11; ++k) printf(" %u", fb(q[k])); }
    for (size_t i = 0; i < n; ++i) { const float* q = &aq[i].a00; for (int k = 0; k < 11; ++k) printf(" %u", fb(q[k])); }
    for (size_t i = 0; i < n * ac; ++i) { const float* g = &ag[i].gx; for (int k = 0; k < 4; ++k) printf(" %u", fb(g[k])); }
    printf("\n");
}
int main() {
    // A grid with UV seams (duplicated positions along x == 4), an open border, and a hole.
    std::vector<float> pos; std::vector<unsigned> idx;
    const int n = 9;
    for (int y = 0; y < n; ++y) for (int x = 0; x < n; ++x) { pos.push_back(float(x)); pos.push_back(float(y)); pos.push_back(0.1f * float((x * y) % 3)); pos.push_back(0); }
    unsigned base = unsigned(pos.size() / 4);
    for (int y = 0; y < n; ++y) { pos.push_back(4.0f); pos.push_back(float(y)); pos.push_back(0.1f * float((4 * y) % 3)); pos.push_back(0); }
    for (int y = 0; y < n - 1; ++y) for (int x = 0; x < n - 1; ++x) {
        if (x == 2 && y == 3) continue;  // hole
        auto at = [&](int xx, int yy) { return (xx == 4 && x >= 4) ? base + unsigned(yy) : unsigned(yy * n + xx); };
        unsigned a = at(x, y), b = at(x + 1, y), c = at(x, y + 1), d = at(x + 1, y + 1);
        idx.insert(idx.end(), {a, b, c, b, d, c});
    }
    std::vector<unsigned char> none, locks(pos.size() / 4);
    for (size_t i = 0; i < locks.size(); ++i) locks[i] = (rnd() % 11 == 0) ? 1 : ((rnd() % 13 == 0) ? 2 : 0);
    for (unsigned opt : {0u, unsigned(meshopt_SimplifySparse), unsigned(meshopt_SimplifyRegularize), unsigned(meshopt_SimplifyPermissive | meshopt_SimplifyPreserveFolds), unsigned(meshopt_SimplifyRegularizeLight | meshopt_SimplifySparse)})
        run(pos, idx, none, opt);
    run(pos, idx, locks, meshopt_SimplifyPermissive);
    for (size_t i = 0; i < locks.size(); ++i) if (i % 5 == 0) locks[i] |= 4;
    run(pos, idx, locks, meshopt_SimplifyPreserveFolds);
    run(pos, idx, locks, 0);
    // A subset of the triangles with sparse indices.
    std::vector<unsigned> half(idx.begin() + idx.size() / 3 * 3 / 2 / 3 * 3, idx.end());
    run(pos, half, locks, meshopt_SimplifySparse | meshopt_SimplifyPermissive);
    return 0;
}
