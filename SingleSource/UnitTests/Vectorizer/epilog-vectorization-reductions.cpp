// Tests for epilogue vectorization with reduction resume values.
//
// When the main vector loop computes a reduction result and the epilogue
// vector loop is bypassed (remainder < epilogue VF), the scalar epilogue
// must resume with the main loop's result, not the original start value.
// This was a miscompile fixed by correctly forwarding resume values.
//
// Requires -mllvm -epilogue-vectorization-minimum-VF=1 to trigger epilogue
// vectorization for these patterns.

#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <random>

static std::mt19937 rng;
static int Failures = 0;

// Use trip counts that exercise the epilogue bypass path: the remainder
// after the main vector loop must be non-zero but smaller than the
// epilogue VF.
static const unsigned TripCounts[] = {1,   2,   3,   5,   7,   15,  17,  18,
                                      19,  31,  33,  34,  35,  63,  65,  66,
                                      127, 129, 255, 257, 511, 513, 999, 1000,
                                      1023};

template <typename Ty>
static void init_data(const std::unique_ptr<Ty[]> &A, unsigned N) {
  // std::uniform_int_distribution requires short or wider per the standard;
  // use int and cast to avoid UB with int8_t.
  std::uniform_int_distribution<int> distrib(std::numeric_limits<Ty>::min(),
                                             std::numeric_limits<Ty>::max());
  for (unsigned i = 0; i < N; i++)
    A[i] = static_cast<Ty>(distrib(rng));
}

// Scalar reference: find index of first element below initial value (0).
// Note: MinVal starts at 0, so this only triggers for negative elements.
template <typename Ty>
__attribute__((noinline)) static long find_first_min_idx_scalar(Ty *A,
                                                                unsigned TC) {
  long MinIdx = 0;
  Ty MinVal = 0;
#pragma clang loop vectorize(disable) interleave_count(1)
  for (unsigned i = 0; i < TC; i++) {
    Ty Val = A[i];
    if (MinVal > Val) {
      MinVal = Val;
      MinIdx = i;
    }
  }
  return MinIdx;
}

// Vectorized version of find_first_min_idx.
template <typename Ty>
__attribute__((noinline)) static long find_first_min_idx_vec(Ty *A,
                                                             unsigned TC) {
  long MinIdx = 0;
  Ty MinVal = 0;
#pragma clang loop vectorize(enable)
  for (unsigned i = 0; i < TC; i++) {
    Ty Val = A[i];
    if (MinVal > Val) {
      MinVal = Val;
      MinIdx = i;
    }
  }
  return MinIdx;
}

// Scalar reference: find index of first element above initial value (0).
// Note: MaxVal starts at 0, so this only triggers for positive elements.
template <typename Ty>
__attribute__((noinline)) static long find_first_max_idx_scalar(Ty *A,
                                                                unsigned TC) {
  long MaxIdx = 0;
  Ty MaxVal = 0;
#pragma clang loop vectorize(disable) interleave_count(1)
  for (unsigned i = 0; i < TC; i++) {
    Ty Val = A[i];
    if (MaxVal < Val) {
      MaxVal = Val;
      MaxIdx = i;
    }
  }
  return MaxIdx;
}

// Vectorized version of find_first_max_idx.
template <typename Ty>
__attribute__((noinline)) static long find_first_max_idx_vec(Ty *A,
                                                             unsigned TC) {
  long MaxIdx = 0;
  Ty MaxVal = 0;
#pragma clang loop vectorize(enable)
  for (unsigned i = 0; i < TC; i++) {
    Ty Val = A[i];
    if (MaxVal < Val) {
      MaxVal = Val;
      MaxIdx = i;
    }
  }
  return MaxIdx;
}

// Shared test driver for find-first-min/max-idx patterns. Compares scalar
// reference against vectorized version across many trip counts and data
// patterns.
//   BgVal:       background fill value for positioned tests
//   ExtremeVal:  value placed at specific positions to trigger the reduction
//   BothMainVal: value placed in main-loop range for "both parts" test
//   BothTailVal: value placed in remainder for "both parts" test
//   TailOnlyVal: value more extreme than BothMainVal, placed only in remainder
//                to directly test resume value forwarding
template <typename Ty>
static void checkFindFirstIdx(const char *Name,
                              long (*Scalar)(Ty *, unsigned),
                              long (*Vec)(Ty *, unsigned),
                              Ty BgVal, Ty ExtremeVal,
                              Ty BothMainVal, Ty BothTailVal,
                              Ty TailOnlyVal) {
  std::cout << "Checking " << Name << "\n";

  unsigned MaxN = 1024;
  std::unique_ptr<Ty[]> Src(new Ty[MaxN]);

  for (unsigned N : TripCounts) {
    auto runTest = [&](const char *Desc) {
      long Reference = Scalar(&Src[0], N);
      long ToCheck = Vec(&Src[0], N);
      if (Reference != ToCheck) {
        std::cerr << Name << ": Miscompare for N=" << N << " (" << Desc
                  << "): expected " << Reference << ", got " << ToCheck << "\n";
        Failures++;
      }
    };

    init_data(Src, N);
    runTest("random");

    for (unsigned I = 0; I != N; ++I)
      Src[I] = 0;
    runTest("all zero");

    for (unsigned I = 0; I != N; ++I)
      Src[I] = std::numeric_limits<Ty>::max();
    runTest("all max");

    for (unsigned I = 0; I != N; ++I)
      Src[I] = std::numeric_limits<Ty>::lowest();
    runTest("all min");

    // Extreme value at start (in main vector loop range).
    for (unsigned I = 0; I != N; ++I)
      Src[I] = BgVal;
    Src[std::min(3u, N - 1)] = ExtremeVal;
    runTest("extreme at start");

    // Extreme value near end (in scalar remainder range).
    for (unsigned I = 0; I != N; ++I)
      Src[I] = BgVal;
    Src[N - 1] = ExtremeVal;
    runTest("extreme at end");

    if (N > 4) {
      // Extreme values in both main loop and remainder, with the more
      // extreme value in the main loop range.
      for (unsigned I = 0; I != N; ++I)
        Src[I] = BgVal;
      Src[1] = BothMainVal;
      Src[N - 1] = BothTailVal;
      runTest("extremes in both parts");

      // Extreme value only in remainder (more extreme than main loop value).
      // Directly tests that resume values are correctly forwarded from the
      // main vector loop to the scalar epilogue.
      for (unsigned I = 0; I != N; ++I)
        Src[I] = BgVal;
      Src[1] = BothTailVal;
      Src[N - 1] = TailOnlyVal;
      runTest("extreme only in remainder");
    }
  }
}

template <typename Ty>
static void checkFindFirstMinIdx(const char *Name) {
  checkFindFirstIdx<Ty>(Name,
                        find_first_min_idx_scalar<Ty>,
                        find_first_min_idx_vec<Ty>,
                        static_cast<Ty>(10),
                        std::numeric_limits<Ty>::lowest(),
                        static_cast<Ty>(-20), static_cast<Ty>(-5),
                        static_cast<Ty>(-30));
}

template <typename Ty>
static void checkFindFirstMaxIdx(const char *Name) {
  checkFindFirstIdx<Ty>(Name,
                        find_first_max_idx_scalar<Ty>,
                        find_first_max_idx_vec<Ty>,
                        static_cast<Ty>(-10),
                        std::numeric_limits<Ty>::max(),
                        static_cast<Ty>(20), static_cast<Ty>(5),
                        static_cast<Ty>(30));
}

int main(void) {
  rng = std::mt19937(15);

  checkFindFirstMinIdx<int8_t>("find_first_min_idx_i8");
  checkFindFirstMinIdx<int16_t>("find_first_min_idx_i16");
  checkFindFirstMinIdx<int32_t>("find_first_min_idx_i32");
  checkFindFirstMinIdx<int64_t>("find_first_min_idx_i64");
  checkFindFirstMaxIdx<int8_t>("find_first_max_idx_i8");
  checkFindFirstMaxIdx<int16_t>("find_first_max_idx_i16");
  checkFindFirstMaxIdx<int32_t>("find_first_max_idx_i32");
  checkFindFirstMaxIdx<int64_t>("find_first_max_idx_i64");

  if (Failures) {
    std::cerr << Failures << " failures detected\n";
    return 1;
  }

  return 0;
}
