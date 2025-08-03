# Simple Compiler

A Bril compiler implementation in C++ with various optimization passes.

## Features

### Implemented Optimizations
- **SSA Transformation**: Convert programs to Static Single Assignment form
- **Dead Code Elimination (DCE)**: Remove unreachable and unused code
- **Dominator Value Numbering (DVN)**: Value numbering with constant folding
- **Sparse Conditional Constant Propagation (SSCP)**: Propagate constants through control flow
- **Control Flow Optimization**: Basic control flow transformations
- **Expression Simplification**: Simplify arithmetic expressions

### Analyzers
- **Control Flow Graph (CFG)**: Forward and reverse CFG construction
- **Dominator Analysis**: Compute dominance relationships
- **Globals Analysis**: Track global variable usage

## Building

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

The compiler supports various transformation passes that can be applied to Bril programs.

## Testing

The project includes comprehensive tests for all components:
- Unit tests for parsers, analyzers, and transformers
- Integration tests with Bril programs
- SSA form validation tests

## TODO

### Planned Optimizations
- SSA PRE (https://dl.acm.org/doi/pdf/10.1145/319301.319348)
- Out of SSA (https://inria.hal.science/inria-00349925v1/document)
- Loop Unroll
- Loop Unswitching
- Strength Reduction (10.7.2 Engineering a Compiler)
