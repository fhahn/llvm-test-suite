#include <iostream>
#include <math.h>
#include <memory>
#include <random>

#include "benchmark/benchmark.h"

#define N 10000

// Apply Fn(A[i]) + Fn(B[i]) in loop, with default loop vectorization settings.
template <typename T, typename T2>
static uint32_t __attribute__((noinline)) run_fn_autovec_simple_dot(T *A,
                                                                    T2 *B) {
  uint32_t Res = 0;
  // #pragma clang loop interleave_count(1)
  for (unsigned i = 0; i < N; i++) {
    Res += A[i] * B[i];
  }
  return Res;
}

template <typename T, typename T2>
static uint32_t __attribute__((noinline)) run_fn_autovec_larger_dot(T *A,
                                                                    T2 *B) {
  uint32_t Res = 0;
  int32_t sum[4] = {0};
  for (size_t k = 0; k < N; ++k) {
    auto a_index = N + k;
    uint8_t a_val = A[a_index];
    for (int l = 0; l < 4; l++) {
      auto b_index = l * N + k;
      int8_t b_val = B[b_index];
      sum[l] += int32_t(a_val * b_val);
    }
  }
  return sum[0] + sum[1] + sum[2] + sum[3];
}

// Initialize arrays A, B and T with random numbers.
template <typename T> static void init_data(T *A) {
  std::uniform_int_distribution<T> dist(-100, 100);
  std::mt19937 rng(12345);
  for (unsigned i = 0; i < N; i++)
    A[i] = dist(rng);
}

// Benchmark auto-vectorized version using Fn.
template <typename T, typename T2>
static void __attribute__((always_inline))
benchmark_fn_autovec_simple_dot(benchmark::State &state) {
  std::unique_ptr<T[]> A(new T[N]);
  std::unique_ptr<T2[]> B(new T2[N]);
  init_data(&A[0]);
  init_data(&B[0]);

  // #ifdef BENCH_AND_VERIFY
  //  Verify the vectorized and un-vectorized versions produce the same results.
  {
    /*    auto Res1 = run_fn_novec(&A[0]);*/
    /*auto Res2 = run_fn_autovec(&A[0]);*/
    /*if (Res1 != Res2 && fpclassify(Res1) != fpclassify(Res2)) {*/
    /*std::cerr << "ERROR: autovec result different to scalar result " <<Res2*/
    /*<< " != " << Res1 << "\n";*/
    /*exit(1);*/
    /*}*/
  }
  // #endif

  uint32_t Res = 0;
  for (auto _ : state) {
    Res += run_fn_autovec_simple_dot(&A[0], &B[0]);
    benchmark::DoNotOptimize(A);
    benchmark::DoNotOptimize(Res);
    benchmark::ClobberMemory();
  }
}

template <typename T, typename T2>
static void __attribute__((always_inline))
benchmark_fn_autovec_larger_dot(benchmark::State &state) {
  std::unique_ptr<T[]> A(new T[N]);
  std::unique_ptr<T2[]> B(new T2[N]);
  init_data(&A[0]);
  init_data(&B[0]);

  // #ifdef BENCH_AND_VERIFY
  //  Verify the vectorized and un-vectorized versions produce the same results.
  {
    /*    auto Res1 = run_fn_novec(&A[0]);*/
    /*auto Res2 = run_fn_autovec(&A[0]);*/
    /*if (Res1 != Res2 && fpclassify(Res1) != fpclassify(Res2)) {*/
    /*std::cerr << "ERROR: autovec result different to scalar result " <<Res2*/
    /*<< " != " << Res1 << "\n";*/
    /*exit(1);*/
    /*}*/
  }
  // #endif

  uint32_t Res = 0;
  for (auto _ : state) {
    Res += run_fn_autovec_larger_dot(&A[0], &B[0]);
    benchmark::DoNotOptimize(A);
    benchmark::DoNotOptimize(Res);
    benchmark::ClobberMemory();
  }
}

// Add add auto-vectorized and disabled vectorization benchmarks for math
// function fn and type ty.
#define ADD_BENCHMARK(ty, ty2)                                                 \
  void BENCHMARK_autovec_red_simple_dot_##ty##_##ty2##_(                       \
      benchmark::State &state) {                                               \
    benchmark_fn_autovec_simple_dot<ty, ty2>(state);                           \
  }                                                                            \
  BENCHMARK(BENCHMARK_autovec_red_simple_dot_##ty##_##ty2##_)                  \
      ->Unit(benchmark::kMicrosecond);                                         \
  void BENCHMARK_autovec_red_larger_dot_##ty##_##ty2##_(                       \
      benchmark::State &state) {                                               \
    benchmark_fn_autovec_larger_dot<ty, ty2>(state);                           \
  }                                                                            \
  BENCHMARK(BENCHMARK_autovec_red_larger_dot_##ty##_##ty2##_)                  \
      ->Unit(benchmark::kMicrosecond); /*\                                     \
                        \ */
/*  void BENCHMARK_novec_red_##ty##_(benchmark::State &state) { \*/
/*benchmark_fn_novec<ty>(state);                                         \*/
/*} \*/
/*BENCHMARK(BENCHMARK_novec_red_##ty##_)->Unit(benchmark::kMicrosecond);*/

ADD_BENCHMARK(uint8_t, int8_t)
ADD_BENCHMARK(int8_t, uint8_t)
// ADD_BENCHMARK(double)
