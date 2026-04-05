// Tests for epilogue vectorization with narrow-interleave patterns.
//
// When the loop body accesses multiple struct fields per iteration, the
// narrow-interleave transformation folds these into vector operations. If the
// scalar epilogue's resume values are not correctly set, the scalar remainder
// restarts from the beginning instead of continuing from where the main vector
// loop left off, causing elements to be processed twice and producing wrong
// results.
//
// Requires -mllvm -epilogue-vectorization-minimum-VF=1 to trigger epilogue
// vectorization for these patterns.

#include <cstdint>
#include <iostream>
#include <memory>

struct S4i32 {
  int32_t a, b, c, d;
};
struct S2i64 {
  int64_t a, b;
};
struct S2f64 {
  double a, b;
};

static const long TripCounts[] = {1,  2,   3,   5,   7,   15,  17,   19,
                                  31, 33,  35,  63,  65,  99,  127,  129,
                                  255, 257, 511, 513, 999, 1000, 1023, 2503};

static int Failures = 0;

// Initialize N bytes of Ref and Test identically with a non-trivial pattern.
static void initBuffers(void *Ref, void *Test, long N) {
  auto *R = reinterpret_cast<unsigned char *>(Ref);
  auto *T = reinterpret_cast<unsigned char *>(Test);
  for (long i = 0; i < N; i++)
    R[i] = T[i] = (unsigned char)(100 + (i % 13));
}

// Compare N bytes and report the first mismatch.
static void compareBuffers(const char *Name, const void *Ref, const void *Test,
                           long N, long TC) {
  auto *R = reinterpret_cast<const unsigned char *>(Ref);
  auto *T = reinterpret_cast<const unsigned char *>(Test);
  for (long i = 0; i < N; i++) {
    if (R[i] != T[i]) {
      std::cerr << Name << ": Miscompare at byte " << i << " for N=" << TC
                << ": expected " << (int)R[i] << ", got " << (int)T[i] << "\n";
      Failures++;
      return;
    }
  }
}

// Run scalar and vec over all trip counts, comparing byte-for-byte.
template <typename T, typename ScalarFn, typename VecFn>
static void check(const char *Name, ScalarFn Scalar, VecFn Vec) {
  std::cout << "Checking " << Name << "\n";
  for (long TC : TripCounts) {
    auto Ref = std::make_unique<T[]>(TC);
    auto Test = std::make_unique<T[]>(TC);
    initBuffers(Ref.get(), Test.get(), TC * sizeof(T));
    Scalar(Ref.get(), TC);
    Vec(Test.get(), TC);
    compareBuffers(Name, Ref.get(), Test.get(), TC * sizeof(T), TC);
  }
}

// Define local ScalarFn/VectorFn lambdas with vectorize(disable) on scalar.
#define DEFINE_SCALAR_AND_VECTOR_FN(type, init, loop)                          \
  auto ScalarFn = [](type *A, long N) {                                        \
    init                                                                       \
    _Pragma("clang loop vectorize(disable) interleave_count(1)")               \
    loop                                                                       \
  };                                                                           \
  auto VectorFn = [](type *A, long N) {                                        \
    init                                                                       \
    loop                                                                       \
  };

int main(void) {
  // --- Basic 4xi32 struct narrow-interleave ---
  {
    DEFINE_SCALAR_AND_VECTOR_FN(S4i32, ,
      for (long i = 0; i < N; i++) {
        A[i].a += A[i].a;
        A[i].b += A[i].b;
        A[i].c += A[i].c;
        A[i].d += A[i].d;
      })
    check<S4i32>("double_s4i32", ScalarFn, VectorFn);
  }

  // --- 2xi64 struct (different VF=2 narrowed to 1) ---
  {
    DEFINE_SCALAR_AND_VECTOR_FN(S2i64, ,
      for (long i = 0; i < N; i++) {
        A[i].a += A[i].a;
        A[i].b += A[i].b;
      })
    check<S2i64>("double_s2i64", ScalarFn, VectorFn);
  }

  // --- 2xf64 struct (floating point narrow-interleave) ---
  {
    DEFINE_SCALAR_AND_VECTOR_FN(S2f64, ,
      for (long i = 0; i < N; i++) {
        A[i].a += A[i].a;
        A[i].b += A[i].b;
      })
    check<S2f64>("double_s2f64", ScalarFn, VectorFn);
  }

  // --- Non-zero start value ---
  // Note: when N <= 3 the loop body never executes; both versions trivially
  // match, so those trip counts do not exercise this pattern.
  {
    DEFINE_SCALAR_AND_VECTOR_FN(S4i32, ,
      for (long i = 3; i < N; i++) {
        A[i].a += A[i].a;
        A[i].b += A[i].b;
        A[i].c += A[i].c;
        A[i].d += A[i].d;
      })
    check<S4i32>("offset_start", ScalarFn, VectorFn);
  }

  // --- Live-out IV: verify both array contents and final IV value ---
  {
    auto ScalarFn = [](S4i32 *A, long N) -> long {
      long i = 0;
      _Pragma("clang loop vectorize(disable) interleave_count(1)")
      for (; i < N; i++) {
        A[i].a += A[i].a;
        A[i].b += A[i].b;
        A[i].c += A[i].c;
        A[i].d += A[i].d;
      }
      return i;
    };
    auto VectorFn = [](S4i32 *A, long N) -> long {
      long i = 0;
      for (; i < N; i++) {
        A[i].a += A[i].a;
        A[i].b += A[i].b;
        A[i].c += A[i].c;
        A[i].d += A[i].d;
      }
      return i;
    };
    std::cout << "Checking liveout_iv\n";
    for (long TC : TripCounts) {
      auto Ref = std::make_unique<S4i32[]>(TC);
      auto Test = std::make_unique<S4i32[]>(TC);
      initBuffers(Ref.get(), Test.get(), TC * sizeof(S4i32));
      long ScalarIV = ScalarFn(Ref.get(), TC);
      long VecIV = VectorFn(Test.get(), TC);
      compareBuffers("liveout_iv", Ref.get(), Test.get(),
                     TC * sizeof(S4i32), TC);
      if (ScalarIV != VecIV) {
        std::cerr << "liveout_iv: IV mismatch for N=" << TC
                  << ": expected " << ScalarIV << ", got " << VecIV << "\n";
        Failures++;
      }
    }
  }

  // --- Pointer-based stepping ---
  {
    DEFINE_SCALAR_AND_VECTOR_FN(S4i32, ,
      for (S4i32 *p = A, *End = A + N; p != End; p++) {
        p->a += p->a;
        p->b += p->b;
        p->c += p->c;
        p->d += p->d;
      })
    check<S4i32>("ptr_step", ScalarFn, VectorFn);
  }

  // --- Truncated IV (i32 IV zero-extended to i64 for indexing) ---
  {
    DEFINE_SCALAR_AND_VECTOR_FN(S4i32, int Limit = (int)N;,
      for (int i = 0; i < Limit; i++) {
        A[i].a += A[i].a;
        A[i].b += A[i].b;
        A[i].c += A[i].c;
        A[i].d += A[i].d;
      })
    check<S4i32>("trunc_iv", ScalarFn, VectorFn);
  }

  // --- Down-counting loop ---
  {
    DEFINE_SCALAR_AND_VECTOR_FN(S4i32, ,
      for (long i = N - 1; i >= 0; i--) {
        A[i].a += A[i].a;
        A[i].b += A[i].b;
        A[i].c += A[i].c;
        A[i].d += A[i].d;
      })
    check<S4i32>("countdown", ScalarFn, VectorFn);
  }

  // --- Reduction + interleave (prevents narrow-interleave):
  //     verify both array contents and reduction result ---
  {
    auto ScalarFn = [](S4i32 *A, long N) -> int32_t {
      int32_t sum = 0;
      _Pragma("clang loop vectorize(disable) interleave_count(1)")
      for (long i = 0; i < N; i++) {
        A[i].a += A[i].a;
        A[i].b += A[i].b;
        A[i].c += A[i].c;
        A[i].d += A[i].d;
        sum += A[i].a;
      }
      return sum;
    };
    auto VectorFn = [](S4i32 *A, long N) -> int32_t {
      int32_t sum = 0;
      for (long i = 0; i < N; i++) {
        A[i].a += A[i].a;
        A[i].b += A[i].b;
        A[i].c += A[i].c;
        A[i].d += A[i].d;
        sum += A[i].a;
      }
      return sum;
    };
    std::cout << "Checking reduction_interleave\n";
    for (long TC : TripCounts) {
      auto Ref = std::make_unique<S4i32[]>(TC);
      auto Test = std::make_unique<S4i32[]>(TC);
      initBuffers(Ref.get(), Test.get(), TC * sizeof(S4i32));
      int32_t ScalarSum = ScalarFn(Ref.get(), TC);
      int32_t VecSum = VectorFn(Test.get(), TC);
      compareBuffers("reduction_interleave", Ref.get(), Test.get(),
                     TC * sizeof(S4i32), TC);
      if (ScalarSum != VecSum) {
        std::cerr << "reduction_interleave: sum mismatch for N=" << TC
                  << ": expected " << ScalarSum << ", got " << VecSum << "\n";
        Failures++;
      }
    }
  }

  if (Failures) {
    std::cerr << Failures << " failures detected\n";
    return 1;
  }

  return 0;
}
