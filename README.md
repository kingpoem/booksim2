BookSim Interconnection Network Simulator
=========================================

BookSim is a cycle-accurate interconnection network simulator.
Originally developed for and introduced with the [Principles and Practices of Interconnection Networks](http://cva.stanford.edu/books/ppin/) book, its functionality has since been continuously extended.
The current major release, BookSim 2.0, supports a wide range of topologies such as mesh, torus and flattened butterfly networks, provides diverse routing algorithms and includes numerous options for customizing the network's router microarchitecture.

---

If you use BookSim in your research, we would appreciate the following citation in any publications to which it has contributed:

Nan Jiang, Daniel U. Becker, George Michelogiannakis, James Balfour, Brian Towles, John Kim and William J. Dally. A Detailed and Flexible Cycle-Accurate Network-on-Chip Simulator. In *Proceedings of the 2013 IEEE International Symposium on Performance Analysis of Systems and Software*, 2013.

## Windows-First Build (MSVC + CMake + C++20)

This branch now provides a Windows-first CMake flow targeting MSVC `cl` and C++20.

### Configure and build

```powershell
cmake --preset x64-debug
cmake --build --preset build-debug
```

Binary output:
- `out/build/x64-debug/bin/Debug/booksim.exe`

### Run

```powershell
out/build/x64-debug/bin/Debug/booksim.exe src/examples/singleconfig
```

### Regression baseline workflow

```powershell
python tests/run_regression.py --exe out/build/x64-debug/bin/Debug/booksim.exe --repo-root .
```

The first run initializes `tests/golden/*.golden.txt`; later runs compare against these goldens.

## Submodule-Oriented CMake Target

`booksim_core` is exported as `Booksim::booksim_core` for downstream CMake integration.

Example in parent project:

```cmake
add_subdirectory(external/booksim2)
target_link_libraries(my_app PRIVATE Booksim::booksim_core)
```