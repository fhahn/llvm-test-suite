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

static int vec2_u32(uint32_t *A, uint32_t *B, unsigned N) {
  unsigned i = 0;
  // Prevent the unroller from interfering.
#pragma clang loop unroll(disable)
      for (i = 0; i < N; i++) {
        B[i] = A[i] + 5;
      }
  return i;
}

/// Helper to block optimizing \p F based on its arguments.
template <typename F, typename... Args>
__attribute__((optnone)) static int callThroughOptnone(F &&f, Args &&...args) {
  return f(std::forward<Args>(args)...);
}

/*
// Benchmark for when runtime checks are passing.
void realign_u32_tc_4(
    benchmark::State &state) {
  std::unique_ptr<uint32_t[]> A(new uint32_t[300]);
  std::unique_ptr<uint32_t[]> B(new uint32_t[300]);

  init_data(A, 300);
  init_data(B, 300);

  for (auto _ : state) {
    callThroughOptnone(vec2_u32, &A[0], &B[0], 4);
    benchmark::DoNotOptimize(A);
    benchmark::DoNotOptimize(B);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(realign_u32_tc_4);

void realign_u32_tc_5(
    benchmark::State &state) {
  std::unique_ptr<uint32_t[]> A(new uint32_t[300]);
  std::unique_ptr<uint32_t[]> B(new uint32_t[300]);

  init_data(A, 300);
  init_data(B, 300);

  for (auto _ : state) {
    callThroughOptnone(vec2_u32, &A[0], &B[0], 5);
    benchmark::DoNotOptimize(A);
    benchmark::DoNotOptimize(B);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(realign_u32_tc_5);

void realign_u32_tc_7(
    benchmark::State &state) {
  std::unique_ptr<uint32_t[]> A(new uint32_t[300]);
  std::unique_ptr<uint32_t[]> B(new uint32_t[300]);

  init_data(A, 300);
  init_data(B, 300);

  for (auto _ : state) {
    callThroughOptnone(vec2_u32, &A[0], &B[0], 7);
    benchmark::DoNotOptimize(A);
    benchmark::DoNotOptimize(B);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(realign_u32_tc_7);



void realign_u32_tc_11(
    benchmark::State &state) {
  std::unique_ptr<uint32_t[]> A(new uint32_t[300]);
  std::unique_ptr<uint32_t[]> B(new uint32_t[300]);

  init_data(A, 300);
  init_data(B, 300);

  for (auto _ : state) {
    callThroughOptnone(vec2_u32, &A[0], &B[0], 11);
    benchmark::DoNotOptimize(A);
    benchmark::DoNotOptimize(B);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(realign_u32_tc_11);


void realign_u32_tc_32(
    benchmark::State &state) {
  std::unique_ptr<uint32_t[]> A(new uint32_t[300]);
  std::unique_ptr<uint32_t[]> B(new uint32_t[300]);

  init_data(A, 300);
  init_data(B, 300);

  for (auto _ : state) {
    callThroughOptnone(vec2_u32, &A[0], &B[0], 32);
    benchmark::DoNotOptimize(A);
    benchmark::DoNotOptimize(B);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(realign_u32_tc_32);


void realign_u32_tc_33(
    benchmark::State &state) {
  std::unique_ptr<uint32_t[]> A(new uint32_t[300]);
  std::unique_ptr<uint32_t[]> B(new uint32_t[300]);

  init_data(A, 300);
  init_data(B, 300);

  for (auto _ : state) {
    callThroughOptnone(vec2_u32, &A[0], &B[0], 33);
    benchmark::DoNotOptimize(A);
    benchmark::DoNotOptimize(B);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(realign_u32_tc_33);

void realign_u32_tc_35(
    benchmark::State &state) {
  std::unique_ptr<uint32_t[]> A(new uint32_t[300]);
  std::unique_ptr<uint32_t[]> B(new uint32_t[300]);

  init_data(A, 300);
  init_data(B, 300);

  for (auto _ : state) {
    callThroughOptnone(vec2_u32, &A[0], &B[0], 35);
    benchmark::DoNotOptimize(A);
    benchmark::DoNotOptimize(B);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(realign_u32_tc_35);

*/
static int vec2_u8(uint8_t *A, uint8_t *B, unsigned N) {
  unsigned i = 0;
  // Prevent the unroller from interfering.
#pragma clang loop unroll(disable)
      for (i = 0; i < N; i++) {
        B[i] = A[i] + 5;
      }
  return i;
}


void realign_u8(
    benchmark::State &state) {
  std::unique_ptr<uint8_t[]> A(new uint8_t[300]);
  std::unique_ptr<uint8_t[]> B(new uint8_t[300]);

  init_data(A, 300);
  init_data(B, 300);

  for (auto _ : state) {
    callThroughOptnone(vec2_u8, &A[0], &B[0], state.range(0));
    benchmark::DoNotOptimize(A);
    benchmark::DoNotOptimize(B);
    benchmark::ClobberMemory();
  }
}

BENCHMARK(realign_u8)->DenseRange(16, 48, 1);

