#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <stdint.h>

#include "common.h"

#define DEFINE_SCALAR_AND_VECTOR_FN2(Init, Loop)                               \
  auto ScalarFn = [](auto *A, auto *B, unsigned N) {                                                \
    Init _Pragma("clang loop vectorize(disable) interleave_count(1)") Loop     \
  };                                                                           \
  auto VectorFn = [](auto *A, auto *B, unsigned N) {                                                \
    Init _Pragma("clang loop vectorize(enable) vectorize_width(4) interleave_count(1)") Loop      \
  };                                                                           \
  auto VectorFnForced = [](auto *A, auto *B, unsigned N) {                                          \
    Init _Pragma("clang loop vectorize(enable) vectorize_width(4) interleave_count(2)") Loop      \
  };

template <typename Ty> using Fn2Ty = std::function<void(Ty *, Ty *, unsigned N)>;

// Run both \p ScalarFn and \p VectorFn on the same inputs with pointers to the
// same buffer. Fail if they do not produce the same results.
template <typename Ty>
static void checkOneArg(Fn2Ty<Ty> ScalarFn,
                                                  Fn2Ty<Ty> VectorFn,
                                                  Fn2Ty<Ty> VectorFnForced,
                                                  const char *Name) {
  std::cout << "Checking " << Name;

  // Make sure we have enough extra elements so we can be liberal with offsets.
  unsigned NumArrayElements = 1024;
  std::unique_ptr<Ty[]> Input1(new Ty[NumArrayElements]);
  std::unique_ptr<Ty[]> Input2(new Ty[NumArrayElements]);
  std::unique_ptr<Ty[]> Reference1(new Ty[NumArrayElements]);
  std::unique_ptr<Ty[]> Reference2(new Ty[NumArrayElements]);
  std::unique_ptr<Ty[]> ToCheck1(new Ty[NumArrayElements]);
  std::unique_ptr<Ty[]> ToCheck2(new Ty[NumArrayElements]);

  for (unsigned I = 1; I < 100; ++I) {
  std::cout << "Checking TC " << I << " - ";
  init_data(Input1, NumArrayElements);
  init_data(Input2, NumArrayElements);
  for (unsigned i = 0; i < NumArrayElements; i++) {
    Reference1[i] = Input1[i];
    Reference2[i] = Input2[i];
    ToCheck1[i] = Input1[i];
    ToCheck2[i] = Input2[i];
  }

  // Run scalar function to generate reference output.
  ScalarFn(&Reference1[0], &Reference2[0], I);

  // Run auto-vectorized function to generate output to check.
  VectorFn(&ToCheck1[0], &ToCheck2[0], I);
  // Compare scalar and vector output.
  std::cout << ": auto-vectorized version ";
  check(Reference1, ToCheck1, NumArrayElements);
  check(Reference2, ToCheck2, NumArrayElements);
  std::cout << "OK";

  for (unsigned i = 0; i < NumArrayElements; i++)
    ToCheck1[i] = Input1[i];

  // Run force-vectorized function to generate output to check.
  VectorFnForced(&ToCheck1[0], &ToCheck2[0], I);
  // Compare scalar and vector output.
  std::cout << ", force-vectorized version ";
  check(Reference1, ToCheck1, NumArrayElements);
  check(Reference2, ToCheck2, NumArrayElements);
  std::cout << "OK\n";
  }
}

int main(void) {
  rng = std::mt19937(15);

/*  {*/
    /*DEFINE_SCALAR_AND_VECTOR_FN1(*/
      /*,*/
      /*for (int i = 0; i < 256; i++) {*/
        /*A[2*i] = A[i] + 5;*/
      /*}*/
    /*);*/
    /*checkOneArg<uint64_t>(*/
        /*ScalarFn, VectorFn, "read A[i], write A[2*i]");*/
  /*}*/

  /*{*/
    /*DEFINE_SCALAR_AND_VECTOR_FN1(*/
      /*auto *a2 = A+1;,*/
      /*for (int i = 0; i < 256; i++) {*/
        /*a2[2*i] = A[i] + 5;*/
      /*}*/
    /*);*/
    /*checkOneArg<uint64_t>(*/
        /*ScalarFn, VectorFn, "read A[i], write (A+1)[2*i]");*/
  /*}*/

  /*{*/
    /*DEFINE_SCALAR_AND_VECTOR_FN1(*/
      /*auto *a2 = A+256;,*/
      /*for (int i = 0; i < 256; i++) {*/
        /*a2[2*i] = A[i] + 5;*/
      /*}*/
    /*);*/
    /*checkOneArg<uint64_t>(*/
        /*ScalarFn, VectorFn, "read A[i], write (A+256)[2*i]");*/
  /*}*/

   /*{*/
    /*DEFINE_SCALAR_AND_VECTOR_FN1(*/
      /*,*/
      /*for (int i = 0; i < 256; i++) {*/
         /*A[i] = A[2*i]+ 5;*/
      /*}*/
    /*);*/
    /*checkOneArg<uint64_t>(*/
        /*ScalarFn, VectorFn, "read A[2*i], write A[i]");*/
  /*}*/

 /*{*/
    /*DEFINE_SCALAR_AND_VECTOR_FN1(*/
      /*auto *a2 = A+1;,*/
      /*for (int i = 0; i < 256; i++) {*/
         /*A[i] = a2[2*i]+ 5;*/
      /*}*/
    /*);*/
    /*checkOneArg<uint64_t>(*/
        /*ScalarFn, VectorFn, "read (A+1)[2*i], write A[i]");*/
  /*}*/

  /*{*/
    /*DEFINE_SCALAR_AND_VECTOR_FN1(*/
      /*auto *a2 = A+256;,*/
      /*for (int i = 0; i < 256; i++) {*/
         /*A[i] = a2[2*i]+ 5;*/
      /*}*/
    /*);*/
    /*checkOneArg<uint64_t>(*/
        /*ScalarFn, VectorFn, "read (A+256)[2*i], write A[i]");*/
  /*}*/

  {
    DEFINE_SCALAR_AND_VECTOR_FN2(
      ,
      for (int i = 0; i < N; i++) {
        B[i] = A[i] + 5;
      }
    );
    checkOneArg<uint32_t>(
        ScalarFn, VectorFn, VectorFnForced, "read A[i], write A[2*i]");
  }
  return 0;
}
