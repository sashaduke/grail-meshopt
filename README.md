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

In progress: meshlet building (`buildMeshletsFlex`/`Spatial`), cluster partitioning, spatial sort
and clustering, meshlet optimisation and the simplifier, which together back Abundance's cluster
LOD builder.

## Use

```
dep meshopt https://github.com/sashaduke/grail-meshopt
```

Positions are flat `f32` arrays with a stride in floats (`4` for `float4` elements).

## Tests

`grail test` compares every result with meshoptimizer on inputs printed by
`tests/reference/basics.cpp` (duplicates, signed zeros, NaN positions, degenerate and curved
meshlets); floats are compared as their 32-bit patterns.

```sh
M=<meshoptimizer>
c++ -O2 -ffp-contract=off -I$M/src tests/reference/basics.cpp $M/src/*.cpp -o basics && ./basics > tests/basics_expected.txt
```

## License

MIT (`LICENSE`); meshoptimizer's MIT notice is retained in `NOTICE`.
