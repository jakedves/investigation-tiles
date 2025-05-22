## Broken addition code

### Attempted fixes so far

- Using `RawUInt32`, `Int32` instead of `UInt32` for data format
- Using smaller tiles e.g. `32 x 16`
- Making host <-> device mem transfers, program launch blocking
- Using `LoFi` and `HiFi4` math fidelities

So far, only changing the data format has made a difference.

- `Int32` -> zeros
- `UInt32` -> alternating `20987906 0`
- `RawUInt32` -> half alternating, then half zeros

Temporary fix:

Just use `float` vectors and `Float32` as the data format. This works.
