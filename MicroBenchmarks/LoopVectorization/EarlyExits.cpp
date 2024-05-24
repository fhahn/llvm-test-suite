#include <memory>
#include <random>
#include <stdint.h>

#include "benchmark/benchmark.h"

static std::mt19937 rng;

// Initialize array A with random numbers.
template <typename Ty>
static void init_data(const std::unique_ptr<Ty[]> &A, unsigned N) {
  std::uniform_int_distribution<uint64_t> distrib(
      std::numeric_limits<Ty>::min(), std::numeric_limits<Ty>::max());
  for (unsigned i = 0; i < N; i++)
    A[i] = static_cast<Ty>(distrib(rng));
}

static int vec1(uint32_t *A) {
  unsigned i = 0;
  // Prevent the unroller from interfering.
#pragma clang loop interleave_count(1) vectorize_width(4)
#pragma clang loop unroll(disable)
      for (i = 0; i < 256; i++) {
        auto L = A[i];
        if (L == 0)break; 
      }
  return i;
}

/// Helper to block optimizing \p F based on its arguments.
template <typename F, typename... Args>
__attribute__((optnone)) static int callThroughOptnone(F &&f, Args &&...args) {
  return f(std::forward<Args>(args)...);
}

// Benchmark for when runtime checks are passing.
void early_exit_never_taken(
    benchmark::State &state) {
  std::unique_ptr<uint32_t[]> A(new uint32_t[300]);

  init_data(A, 300);
  for (unsigned i = 0; i < 256; ++i) {
    if (A[i] == 0)
      A[i] = 1;
  }

  for (auto _ : state) {
    callThroughOptnone(vec1, &A[0]);
    benchmark::DoNotOptimize(A);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(early_exit_never_taken);

void early_exit_taken_iter_0(
    benchmark::State &state) {
  std::unique_ptr<uint32_t[]> A(new uint32_t[300]);

  init_data(A, 300);
  for (unsigned i = 0; i < 256; ++i) {
    if (A[i] == 0)
      A[i] = 1;
  }

  A[0] = 0;
  for (auto _ : state) {
    callThroughOptnone(vec1, &A[0]);
    benchmark::DoNotOptimize(A);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(early_exit_taken_iter_0);

void early_exit_taken_iter_3(
    benchmark::State &state) {
  std::unique_ptr<uint32_t[]> A(new uint32_t[300]);

  init_data(A, 300);
  for (unsigned i = 0; i < 256; ++i) {
    if (A[i] == 0)
      A[i] = 1;
  }

  A[3] = 0;
  for (auto _ : state) {
    callThroughOptnone(vec1, &A[0]);
    benchmark::DoNotOptimize(A);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(early_exit_taken_iter_3);


void early_exit_taken_iter_15(
    benchmark::State &state) {
  std::unique_ptr<uint32_t[]> A(new uint32_t[300]);

  init_data(A, 300);
  for (unsigned i = 0; i < 256; ++i) {
    if (A[i] == 0)
      A[i] = 1;
  }

  A[15] = 0;
  for (auto _ : state) {
    callThroughOptnone(vec1, &A[0]);
    benchmark::DoNotOptimize(A);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(early_exit_taken_iter_15);


void early_exit_taken_iter_33(
    benchmark::State &state) {
  std::unique_ptr<uint32_t[]> A(new uint32_t[300]);

  init_data(A, 300);
  for (unsigned i = 0; i < 256; ++i) {
    if (A[i] == 0)
      A[i] = 1;
  }

  A[33] = 0;
  for (auto _ : state) {
    callThroughOptnone(vec1, &A[0]);
    benchmark::DoNotOptimize(A);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(early_exit_taken_iter_33);


void early_exit_taken_iter_51(
    benchmark::State &state) {
  std::unique_ptr<uint32_t[]> A(new uint32_t[300]);

  init_data(A, 300);
  for (unsigned i = 0; i < 256; ++i) {
    if (A[i] == 0)
      A[i] = 1;
  }

  A[51] = 0;
  for (auto _ : state) {
    callThroughOptnone(vec1, &A[0]);
    benchmark::DoNotOptimize(A);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(early_exit_taken_iter_51);

static int vec2(uint32_t *A, unsigned N) {
  unsigned i = 0;
  // Prevent the unroller from interfering.
#pragma clang loop interleave_count(1) vectorize_width(4)
#pragma clang loop unroll(disable)
      for (i = 0; i < N; i++) {
        auto L = A[i];
        if (L == 0) return i;
      }
  return N;
}
