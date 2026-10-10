# LeetCode.com Database

A C++ linked list database for storing and sorting LeetCode problems.

## Features

- Stores problems with `id`, `name`, and `difficulty` fields
- Custom merge sort implementation on a singly linked list
- Sort by any field: problem ID, name (alphabetical), or difficulty (Easy → Medium → Hard)

## Build

```bash
make
```

## Run

```bash
./leetcode_db
```

## Clean

```bash
make clean
```

## Project Structure

| File | Description |
|------|-------------|
| `Problem.h` | `Problem` struct definition |
| `LinkedList.h` | `Node` struct + `LinkedList` class declaration |
| `LinkedList.cpp` | Linked list implementation with merge sort |
| `main.cpp` | Demo with 10 seeded problems, sorted 3 ways |
| `Makefile` | Build configuration |
