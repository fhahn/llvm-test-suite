#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <stdint.h>

#include "common.h"

#define DEFINE_SCALAR_AND_VECTOR_FN1(Init, Loop)                               \
  auto ScalarFn = [](auto *A, unsigned N) {                                                \
    Init _Pragma("clang loop vectorize(disable) interleave_count(1)") Loop     \
  };                                                                           \
  auto VectorFn = [](auto *A, unsigned N) {                                                \
    Init _Pragma("clang loop vectorize(enable) vectorize_width(4) interleave_count(1)") Loop      \
  };                                                                           \
  auto VectorFnForced = [](auto *A, unsigned N) {                                          \
    Init _Pragma("clang loop vectorize(enable) vectorize_width(4) interleave_count(2)") Loop      \
  };

template <typename Ty> using Fn1Ty = std::function<void(Ty *, unsigned N)>;

// Run both \p ScalarFn and \p VectorFn on the same inputs with pointers to the
// same buffer. Fail if they do not produce the same results.
template <typename Ty>
static void checkOneArg(Fn1Ty<Ty> ScalarFn,
                                                  Fn1Ty<Ty> VectorFn,
                                                  Fn1Ty<Ty> VectorFnForced,
                                                  const char *Name) {
  std::cout << "Checking " << Name;

  // Make sure we have enough extra elements so we can be liberal with offsets.
  unsigned NumArrayElements = 1024;
  std::unique_ptr<Ty[]> Input1(new Ty[NumArrayElements]);
  std::unique_ptr<Ty[]> Reference(new Ty[NumArrayElements]);
  std::unique_ptr<Ty[]> ToCheck(new Ty[NumArrayElements]);

  for (unsigned I = 1; I < 10; ++I) {
  std::cout << "Checking TC " << I << " - ";
  init_data(Input1, NumArrayElements);
  for (unsigned i = 0; i < NumArrayElements; i++) {
    Reference[i] = Input1[i];
    ToCheck[i] = Input1[i];
  }

  // Run scalar function to generate reference output.
  ScalarFn(&Reference[0], I);

  // Run auto-vectorized function to generate output to check.
  VectorFn(&ToCheck[0], I);
  // Compare scalar and vector output.
  std::cout << ": auto-vectorized version ";
  check(Reference, ToCheck, NumArrayElements);
  std::cout << "OK";

  for (unsigned i = 0; i < NumArrayElements; i++)
    ToCheck[i] = Input1[i];

  // Run force-vectorized function to generate output to check.
  VectorFnForced(&ToCheck[0], I);
  // Compare scalar and vector output.
  std::cout << ", force-vectorized version ";
  check(Reference, ToCheck, NumArrayElements);
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
    DEFINE_SCALAR_AND_VECTOR_FN1(
      ,
      for (int i = 0; i < N; i++) {
        A[i] = A[i] + 5;
      }
    );
    checkOneArg<uint32_t>(
        ScalarFn, VectorFn, VectorFnForced, "read A[i], write A[2*i]");
  }


  return 0;
}
