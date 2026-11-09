# grail-meshopt

A port of [meshoptimizer](https://github.com/zeux/meshoptimizer) 1.3 to
[Grail](https://github.com/graillang/grail) with results identical to the C++ library built
without floating-point contraction (`-ffp-contract=off`, as Abundance builds it), down to the bit
pattern of every float.

Ported so far:

- `generate_position_remap` (meshopt_generatePositionRemap)
- `generate_vertex_remap_multi` (meshopt_generateVertexRemapMulti; the unique count is appended
  as the last element)
- `compute_sphere_bounds` (meshopt_computeSphereBounds, points or spheres)
- `compute_meshlet_bounds` (meshopt_computeMeshletBounds, including the normal cone and its 8-bit
  SNORM form)
- `build_meshlets_spatial` and `build_meshlets_bound` (meshopt_buildMeshletsSpatial: SAH-split
  BVH clusters, as Abundance's cluster builder uses them)
- `spatial_sort_remap`, `spatial_cluster_points` (meshopt_spatialSortRemap / spatialClusterPoints)
- `partition_clusters` (meshopt_partitionClusters, with or without positions; the partition count
  is appended as the last element)

- `simplify_prepare`: the simplifier front end (sparse remap, edge adjacency, position remap with
  wedge rings, vertex classification and edge loops) and `fill_quadrics` (face, vertex, edge, fold
  and attribute quadrics)

- `simplify` (meshopt_simplify / meshopt_simplifyWithAttributes): quadric edge collapse with
  seams, borders, locked and protected vertices, attribute quadrics, `Sparse`, `ErrorAbsolute`,
  `ErrorClamped`, `Permissive`, `Regularize`, `RegularizeLight`, `LockBorder` and `PreserveFolds`
  (`Prune` is not ported yet and returns None)

- `compute_cluster_bounds`, `extract_meshlet_indices`, `optimize_meshlet_level`
  (meshopt_computeClusterBounds / extractMeshletIndices / optimizeMeshletLevel)

In progress: `simplify_sloppy` and `simplify_scale`, the remaining pieces of Abundance's cluster LOD
path, which together
with the above back Abundance's cluster LOD builder.

## Use

```
dep meshopt https://github.com/sashaduke/grail-meshopt
```

Positions are flat `f32` arrays with a stride in floats (`4` for `float4` elements).

## Tests

`grail test` compares every result with meshoptimizer on inputs printed by the programs in
`tests/reference/`: `basics.cpp` (duplicates, signed zeros, NaN positions, degenerate and curved
meshlets; floats compared as their 32-bit patterns) and `meshlets.cpp` (a height field, a random
soup and a torus under Abundance's 128/42..128/0.5 parameters and three others) and
`partition.cpp` (spatial meshlets of two meshes partitioned at three target sizes, with and
without positions).

```sh
M=<meshoptimizer>
c++ -O2 -ffp-contract=off -I$M/src tests/reference/basics.cpp $M/src/*.cpp -o basics && ./basics > tests/basics_expected.txt
```

## License

MIT (`LICENSE`); meshoptimizer's MIT notice is retained in `NOTICE`.
