#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <stdint.h>

#include "common.h"

#define DEFINE_SCALAR_AND_VECTOR_FN1(Init, Loop)                               \
  auto ScalarFn = [](auto *A) {                                                \
    int i;                      \
    Init _Pragma("clang loop vectorize(disable) interleave_count(1)") Loop     \
    return i; \
  };                                                                           \
  auto VectorFn = [](auto *A) {                                                \
    int i;                      \
    Init _Pragma("clang loop vectorize(enable)") Loop                          \
    return i; \
  };                                                                           \
  auto VectorFnForced = [](auto *A) {                                          \
    int i;                      \
    Init _Pragma("clang loop vectorize(enable) vectorize_width(4) interleave_count(1)") Loop      \
    return i; \
  };

template <typename Ty> using Fn1Ty = std::function<int(Ty *)>;

// Run both \p ScalarFn and \p VectorFn on the same inputs with pointers to the
// same buffer. Fail if they do not produce the same results.
template <typename Ty>
static void checkOneArg(Fn1Ty<Ty> ScalarFn,
                                                  Fn1Ty<Ty> VectorFn,
                                                  Fn1Ty<Ty> VectorFnForced,
                                                  const char *Name) {
  std::cout << "Checking " << Name << "\n";

  // Make sure we have enough extra elements so we can be liberal with offsets.
  unsigned NumArrayElements = 1024;
  std::unique_ptr<Ty[]> Input1(new Ty[NumArrayElements]);
  std::unique_ptr<Ty[]> Reference(new Ty[NumArrayElements]);
  std::unique_ptr<Ty[]> ToCheck(new Ty[NumArrayElements]);

  init_data(Input1, NumArrayElements);
  for (int I = 256; I >= 0 ; I--) {
    if (I != 256) {
      Input1[I] = 0;
    }

    for (unsigned i = 0; i < NumArrayElements; i++) {
      Reference[i] = Input1[i];
      ToCheck[i] = Input1[i];
    }

  // Run scalar function to generate reference output.
  int ReferenceRes = ScalarFn(&Reference[0]);

  // Run auto-vectorized function to generate output to check.
  int VecRes = VectorFn(&ToCheck[0]);
  // Compare scalar and vector output.
  std::cout << "  " << I << ": auto-vectorized version ";
  check(Reference, ToCheck, NumArrayElements);
  if (ReferenceRes != VecRes) {
    std::cerr << "Return miscompare " << ReferenceRes << " != " << VecRes << "\n";
    exit(1);
  }
  std::cout << "OK";

  for (unsigned i = 0; i < NumArrayElements; i++)
    ToCheck[i] = Input1[i];

  // Run force-vectorized function to generate output to check.
  int ForcedRes = VectorFnForced(&ToCheck[0]);
  // Compare scalar and vector output.
  std::cout << ", force-vectorized version ";
  check(Reference, ToCheck, NumArrayElements);
  if (ReferenceRes != ForcedRes) {
    std::cerr << "Return miscompare " << ReferenceRes << " != " << ForcedRes << "\n";
    exit(1);
  }

  std::cout << "OK\n";
  }
}


int main(void) {
  rng = std::mt19937(15);

  {
    DEFINE_SCALAR_AND_VECTOR_FN1(
      ,
      for (i = 0; i < 256; i++) {
        auto L = A[i];
        if (L == 0) break;
      }
    );
    checkOneArg<uint64_t>(
        ScalarFn, VectorFn, VectorFnForced , "read A[i], write A[2*i]");
  }

  return 0;
}
