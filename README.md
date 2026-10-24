# Smart Fruit Supply Chain System

A **C++17 console application** developed as a third-semester data structures group project. It models suppliers, sellers, production stock, inventory batches and delivery routes.

## Features and concepts

- Supplier and seller registration and sign-in.
- Manager and administrator menus.
- FIFO inventory using a queue of fruit batches.
- Supplier and seller ranking.
- Route-graph operations and shortest-route calculation.
- JSON export for a separate frontend.

The implementation uses maps, vectors, queues and other standard-library structures. All application code is in [FruitSupplySystem.cpp](FruitSupplySystem.cpp).

## Configure and build

Requires a C++17-capable compiler.

1. Clone the repository.
2. Edit `JSON_OUTPUT_PATH` near the start of `FruitSupplySystem.cpp`. It currently points to the original developer's Windows directory. Set it to an existing local output directory ending in a path separator, or `"./"` for the current directory.
3. Compile and run:

```bash
g++ -std=c++17 -Wall -Wextra FruitSupplySystem.cpp -o fruit-supply
./fruit-supply
```

On Windows with MinGW, build with `-o fruit-supply.exe` and run `.\fruit-supply.exe`.

## Scope

This repository contains the console program; the Vue frontend referenced in the source is **not included**. The application contains classroom login credentials and in-memory state, so use it as an educational demonstration with synthetic data.

## Related project

[SupplyGo](https://github.com/uvinN20/SupplyGo) explores supply-chain operations using Java, client-server networking and concurrency.

## Academic credit

This is a group project. Contributor history is available through GitHub; the related Java project maintains its own team documentation.
