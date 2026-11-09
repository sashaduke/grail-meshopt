// Reference for tests/clusterlod.grl: clodBuild (Abundance's configuration: spatial clusters,
// partition sorting, precise bounds, UV-seam protect mask; plus border dilation, locked vertices
// and a single-level build) and clodBuildHierarchy, printed with floats as 32-bit patterns.
//   c++ -O2 -ffp-contract=off -I<meshoptimizer>/src -I<meshoptimizer>/demo clusterlod.cpp <meshoptimizer>/src/*.cpp -o clusterlod && ./clusterlod > ../clusterlod_expected.txt
#include "meshoptimizer.h"
#define CLUSTERLOD_IMPLEMENTATION
#include "clusterlod.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
static unsigned fb(float f) { unsigned u; memcpy(&u, &f, 4); return u; }
static void bounds(const clodBounds& b) { printf(" %u %u %u %u %u", fb(b.center[0]), fb(b.center[1]), fb(b.center[2]), fb(b.radius), fb(b.error)); }
struct Run { std::vector<clodGroup> groups; std::vector<float>* positions; bool snapshot; };
static void run(const char* name, clodConfig config, std::vector<float> positions, const std::vector<unsigned>& idx, const std::vector<float>& attrs, const std::vector<unsigned char>& lock, unsigned protect) {
    size_t vc = positions.size() / 3;
    const float weights[3] = {0.5f, 0.5f, 0.5f};
    printf("case %s %zu %zu %zu %zu %zu %u %u %u %u %u %u\n", name, config.max_vertices, config.min_triangles, config.max_triangles, config.partition_size,
        size_t(config.partition_sort) | size_t(config.optimize_bounds) << 1 | size_t(config.simplify_dilate_borders) << 2 | size_t(config.simplify_preserve_folds) << 3,
        fb(config.cluster_fill_weight), fb(config.simplify_ratio), protect, unsigned(lock.size() != 0), fb(config.simplify_error_edge_limit), fb(config.simplify_error_merge_additive));
    printf("pos %zu", vc); for (float f : positions) printf(" %u", fb(f)); printf("\n");
    printf("idx %zu", idx.size()); for (unsigned i : idx) printf(" %u", i); printf("\n");
    printf("attr %zu", attrs.size()); for (float f : attrs) printf(" %u", fb(f)); printf("\n");
    printf("lock %zu", lock.size()); for (unsigned char l : lock) printf(" %u", l); printf("\n");
    clodMesh mesh{};
    mesh.indices = idx.data(); mesh.index_count = idx.size(); mesh.vertex_count = vc;
    mesh.vertex_positions = positions.data(); mesh.vertex_positions_stride = 12;
    mesh.vertex_attributes = attrs.data(); mesh.vertex_attributes_stride = 5 * sizeof(float);
    mesh.vertex_lock = lock.empty() ? nullptr : lock.data();
    mesh.attribute_weights = weights; mesh.attribute_count = 3; mesh.attribute_protect_mask = protect;
    Run r{{}, &positions, config.simplify_dilate_borders};
    clodBuild(config, mesh, [&](clodGroup group, const clodCluster* cl, size_t count) -> int {
        int id = int(r.groups.size());
        r.groups.push_back(group);
        printf("group %d %zu", group.depth, count); bounds(group.simplified); printf("\n");
        for (size_t i = 0; i < count; ++i) {
            printf("cluster %d", cl[i].refined); bounds(cl[i].bounds); printf(" %zu %zu", cl[i].vertex_count, cl[i].index_count);
            for (size_t j = 0; j < cl[i].index_count; ++j) printf(" %u", cl[i].indices[j]); printf("\n");
            if (r.snapshot) {
                printf("cpos");
                for (size_t j = 0; j < cl[i].index_count; ++j) for (int k = 0; k < 3; ++k) printf(" %u", fb(positions[cl[i].indices[j] * 3 + k]));
                printf("\n");
            }
        }
        return id;
    });
    printf("final"); for (float f : positions) printf(" %u", fb(f)); printf("\n");
    int levels = 0; for (const clodGroup& g : r.groups) levels = std::max(levels, g.depth + 1);
    std::vector<clodNode> nodes(clodBuildHierarchyBound(r.groups.size(), 4, levels));
    nodes.resize(clodBuildHierarchy(nodes.data(), r.groups.data(), r.groups.size(), 4, levels));
    printf("nodes %zu %d\n", nodes.size(), levels);
    for (const clodNode& n : nodes) { printf("node %d %u %u", n.group, n.child_offset, n.child_count); bounds(n.bounds); printf("\n"); }
    printf("end\n");
}
int main() {
    // Height field with normals and UVs; column 24 is duplicated with a UV jump (a seam).
    std::vector<float> pos, attrs; std::vector<unsigned> idx; const int n = 49, seam = 24;
    std::vector<int> col(n * n);
    auto height = [](int x, int y) { return 1.7f * std::sin(float(x) * 0.27f) * std::cos(float(y) * 0.19f) + 0.3f * std::sin(float(x * y) * 0.05f); };
    auto vertex = [&](int x, int y, float u) {
        float h = height(x, y);
        pos.insert(pos.end(), {float(x) * 0.5f, float(y) * 0.5f, h});
        float nx = height(x - 1, y) - height(x + 1, y), ny = height(x, y - 1) - height(x, y + 1), nz = 1.0f;
        float l = std::sqrt(nx * nx + ny * ny + nz * nz);
        attrs.insert(attrs.end(), {nx / l, ny / l, nz / l, u, float(y) / float(n - 1)});
        return int(pos.size() / 3 - 1);
    };
    for (int y = 0; y < n; ++y) for (int x = 0; x < n; ++x) col[y * n + x] = vertex(x, y, float(x) / float(n - 1));
    std::vector<int> right(n);
    for (int y = 0; y < n; ++y) right[y] = vertex(seam, y, float(seam) / float(n - 1) + 0.5f);
    for (int y = 0; y < n - 1; ++y) for (int x = 0; x < n - 1; ++x) {
        auto at = [&](int xx, int yy) { return unsigned((xx == seam && x >= seam) ? right[yy] : col[yy * n + xx]); };
        unsigned a = at(x, y), b = at(x + 1, y), c = at(x, y + 1), d = at(x + 1, y + 1);
        idx.insert(idx.end(), {a, b, c, b, d, c});
    }
    const unsigned protect = (1u << 3) | (1u << 4);
    clodConfig abundance = clodDefaultConfig(64);
    abundance.max_vertices = 64; abundance.partition_size = 8; abundance.partition_sort = true;
    abundance.cluster_spatial = true; abundance.cluster_fill_weight = 0.5f; abundance.optimize_bounds = true;
    run("abundance", abundance, pos, idx, attrs, {}, protect);
    clodConfig foliage = abundance; foliage.simplify_dilate_borders = true; foliage.simplify_preserve_folds = true;
    // The height field's outer edge is an open border, so dilation has work to do.
    run("foliage", foliage, pos, idx, attrs, {}, protect);
    std::vector<unsigned char> lock(pos.size() / 3, 0);
    for (size_t i = 0; i < lock.size(); i += 11) lock[i] = meshopt_SimplifyVertex_Lock;
    clodConfig edged = abundance; edged.simplify_error_edge_limit = 0.25f; edged.simplify_error_merge_additive = 0.1f;
    run("locked", edged, pos, idx, attrs, lock, protect);
    clodConfig single = abundance; single.simplify_ratio = 1.0f;
    run("single", single, pos, idx, attrs, {}, protect);
    return 0;
}
