#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <stdint.h>

#include "common.h"

template <typename RetTy, typename Ty>
using Fn2Ty = std::function<RetTy(Ty *, Ty *, RetTy)>;
template <typename RetTy, typename Ty>
static void checkVectorFunction(Fn2Ty<RetTy, Ty> ScalarFn,
                                Fn2Ty<RetTy, Ty> VectorFn, const char *Name) {
  std::cout << "Checking " << Name << "\n";

  unsigned N = 1000;
  std::unique_ptr<Ty[]> Src1(new Ty[N]);
  std::unique_ptr<Ty[]> Src2(new Ty[N]);
  init_data(Src1, N);
  init_data(Src2, N);

  // Test VectorFn with different input data.
  {
    // Check with random inputs.
    auto Reference = ScalarFn(&Src1[0], &Src2[0], N);
    auto ToCheck = VectorFn(&Src1[0], &Src2[0], N);
    std::cerr << ToCheck << " " << Reference << "\n";
    if (Reference != ToCheck) {
      std::cerr << "Miscompare\n";
      exit(1);
    }
  }

  {
    // Check with Src1 > Src2 for all elements.
    for (unsigned I = 0; I != N; ++I) {
      Src1[I] = std::numeric_limits<Ty>::max();
      Src2[I] = std::numeric_limits<Ty>::min();
    }
    auto Reference = ScalarFn(&Src1[0], &Src2[0], N);
    auto ToCheck = VectorFn(&Src1[0], &Src2[0], N);
    std::cerr << ToCheck << " " << Reference << "\n";
    if (Reference != ToCheck) {
      std::cerr << "Miscompare\n";
      exit(1);
    }
  }

  {
    // Check with Src1 < Src2 for all elements.
    for (unsigned I = 0; I != N; ++I) {
      Src1[I] = std::numeric_limits<Ty>::min();
      Src2[I] = std::numeric_limits<Ty>::max();
    }
    auto Reference = ScalarFn(&Src1[0], &Src2[0], N);
    auto ToCheck = VectorFn(&Src1[0], &Src2[0], N);
    std::cerr << ToCheck << " " << Reference << "\n";
    if (Reference != ToCheck) {
      std::cerr << "Miscompare\n";
      exit(1);
    }
  }

  {
    // Check with only Src1[998] > Src2[998].
    for (unsigned I = 0; I != N; ++I)
      Src1[I] = Src2[I] = std::numeric_limits<Ty>::min();
    Src1[998] = std::numeric_limits<Ty>::max();
    auto Reference = ScalarFn(&Src1[0], &Src2[0], N);
    auto ToCheck = VectorFn(&Src1[0], &Src2[0], N);
    std::cerr << ToCheck << " " << Reference << "\n";
    if (Reference != ToCheck) {
      std::cerr << "Miscompare\n";
      exit(1);
    }
  }

  {
    // Check with only Src1[0] > Src2[0].
    for (unsigned I = 0; I != N; ++I)
      Src1[I] = Src2[I] = std::numeric_limits<Ty>::min();
    Src1[0] = std::numeric_limits<Ty>::max();
    auto Reference = ScalarFn(&Src1[0], &Src2[0], N);
    auto ToCheck = VectorFn(&Src1[0], &Src2[0], N);
    std::cerr << ToCheck << " " << Reference << "\n";
    if (Reference != ToCheck) {
      std::cerr << "Miscompare " << ToCheck << " " << Reference << "\n";
      exit(1);
    }
  }

  {
    // Check with only Src1[N - 1] > Src2[N - 1].
    for (unsigned I = 0; I != N; ++I)
      Src1[I] = Src2[I] = std::numeric_limits<Ty>::min();
    Src1[N - 1] = std::numeric_limits<Ty>::max();
    auto Reference = ScalarFn(&Src1[0], &Src2[0], N);
    auto ToCheck = VectorFn(&Src1[0], &Src2[0], N);
    std::cerr << ToCheck << " " << Reference << "\n";
    if (Reference != ToCheck) {
      std::cerr << "Miscompare\n";
      exit(1);
    }
  }

  {
    // Check with only Src1[0] > Src2[0] and Src1[N - 1] > Src2[N - 1].
    for (unsigned I = 0; I != N; ++I)
      Src1[I] = Src2[I] = std::numeric_limits<Ty>::min();
    Src1[0] = Src1[N - 1] = std::numeric_limits<Ty>::max();
    auto Reference = ScalarFn(&Src1[0], &Src2[0], N);
    auto ToCheck = VectorFn(&Src1[0], &Src2[0], N);
    std::cerr << ToCheck << " " << Reference << "\n";
    if (Reference != ToCheck) {
      std::cerr << "Miscompare\n";
      exit(1);
    }
  }

  {
    // Check with only Src1[499] > Src2[499].
    for (unsigned I = 0; I != N; ++I)
      Src1[I] = Src2[I] = std::numeric_limits<Ty>::min();
    Src1[499] = std::numeric_limits<Ty>::max();
    auto Reference = ScalarFn(&Src1[0], &Src2[0], N);
    auto ToCheck = VectorFn(&Src1[0], &Src2[0], N);
    std::cerr << ToCheck << " " << Reference << "\n";
    if (Reference != ToCheck) {
      std::cerr << "Miscompare\n";
      exit(1);
    }
  }

  {
    // Check with only Src1[199] > Src2[199].
    for (unsigned I = 0; I != N; ++I)
      Src1[I] = Src2[I] = std::numeric_limits<Ty>::min();
    Src1[199] = std::numeric_limits<Ty>::max();
    auto Reference = ScalarFn(&Src1[0], &Src2[0], N);
    auto ToCheck = VectorFn(&Src1[0], &Src2[0], N);
    std::cerr << ToCheck << " " << Reference << "\n";
    if (Reference != ToCheck) {
      std::cerr << "Miscompare\n";
      exit(1);
    }
  }
  {
    // Check with Src1[199] > Src2[199] and Src1[499] > Src2[499].
    for (unsigned I = 0; I != N; ++I)
      Src1[I] = Src2[I] = std::numeric_limits<Ty>::min();
    Src1[199] = std::numeric_limits<Ty>::max();
    Src1[499] = std::numeric_limits<Ty>::max();
    auto Reference = ScalarFn(&Src1[0], &Src2[0], N);
    auto ToCheck = VectorFn(&Src1[0], &Src2[0], N);
    std::cerr << ToCheck << " " << Reference << "\n";
    if (Reference != ToCheck) {
      std::cerr << "Miscompare\n";
      exit(1);
    }
  }
}

int main(void) {
  rng = std::mt19937(15);

#define DEC_COND(End, Step, RetTy) for (RetTy I = 999; I > End; I -= Step)

#define DEFINE_FINDLAST_LOOP_BODY(TrueVal, FalseVal, ForCond)                  \
  ForCond { Rdx = A[I] > B[I] ? TrueVal : FalseVal; }                          \
  return Rdx;

  {
    // Check with decreasing 32-bits induction variable.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
        int32_t Rdx = -1;
        ,
        DEFINE_FINDLAST_LOOP_BODY(
            /* TrueVal= */ I, /* FalseVal= */ Rdx,
            /* ForCond= */
            DEC_COND(/* End= */ 0, /* Step= */ 1, /* RetTy= */ int32_t)),
        int32_t);
    checkVectorFunction<int32_t, int32_t>(
        ScalarFn, VectorFn, "findlast_icmp_s32_start_decreasing_induction1");
    checkVectorFunction<int32_t, float>(
        ScalarFn, VectorFn, "findlast_fcmp_s32_start_decreasing_induction");
  }

  {
    // Check with decreasing 16-bits induction variable.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
        int16_t Rdx = -1;
        ,
        DEFINE_FINDLAST_LOOP_BODY(
            /* TrueVal= */ I, /* FalseVal= */ Rdx,
            /* ForCond= */
            DEC_COND(/* End= */ 0, /* Step= */ 1, /* RetTy= */ int16_t)),
        int16_t);
    checkVectorFunction<int16_t, int16_t>(
        ScalarFn, VectorFn, "findlast_icmp_s16_start_decreasing_induction");
  }

  {
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
        int16_t Rdx = -1;
        ,
        DEFINE_FINDLAST_LOOP_BODY(
            /* TrueVal= */ I, /* FalseVal= */ Rdx,
            /* ForCond= */
            DEC_COND(/* End= */ 0, /* Step= */ 2, /* RetTy= */ int16_t)),
        int16_t);
    checkVectorFunction<int16_t, int16_t>(
        ScalarFn, VectorFn, "findlast_icmp_s16_start_decreasing_induction");
  }

  {
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
        uint32_t MinIdx = 1000; uint32_t Idx = -1u;
        ,
        for (unsigned I = 0; I != 1000; ++I) {
          if (A[I] > B[I])
            MinIdx = Idx;
          Idx -= 1;
        } return MinIdx;
        , uint32_t);
    checkVectorFunction<uint32_t, uint32_t>(
        ScalarFn, VectorFn, "findlast_icmp_s32_start_decreasing_induction2");
  }

  {
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
        uint32_t MinIdx = 1000; uint32_t Idx = -1u;
        ,
        for (unsigned I = 0; I != 1000; ++I) {
          if (A[I] < B[I])
            MinIdx = Idx;
          Idx -= 1;
        } return MinIdx;
        , uint32_t);
    checkVectorFunction<uint32_t, uint32_t>(
        ScalarFn, VectorFn, "findlast_icmp_s32_start_decreasing_induction3");
  }

  {
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
        uint32_t MinIdx = 1000; uint32_t Idx = -2u;
        ,
        for (unsigned I = 0; I != 1000; ++I) {
          if (A[I] > B[I])
            MinIdx = Idx;
          Idx -= 1;
        } return MinIdx;
        , uint32_t);
    checkVectorFunction<uint32_t, uint32_t>(
        ScalarFn, VectorFn, "findlast_icmp_s32_start_decreasing_induction4");
  }

  {
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
        uint32_t MinIdx = 1000; uint32_t Idx = -2u;
        ,
        for (unsigned I = 0; I != 1000; ++I) {
          if (A[I] > B[I])
            MinIdx = Idx;
          Idx -= 1;
        } return MinIdx;
        , uint32_t);
    checkVectorFunction<uint32_t, uint32_t>(
        ScalarFn, VectorFn, "findlast_icmp_s32_start_decreasing_induction5");
  }

  {
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
        int32_t MinIdx = 1000; int32_t Idx = 300;
        ,
        for (unsigned I = 0; I != 1000; ++I) {
          if (A[I] > B[I])
            MinIdx = Idx;
          Idx -= 1;
        } return MinIdx;
        , int32_t);
    checkVectorFunction<int32_t, int32_t>(
        ScalarFn, VectorFn, "findlast_icmp_s32_start_decreasing_induction6");
  }

  {
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
        int32_t MinIdx = 1000; int32_t Idx = 600;
        ,
        for (unsigned I = 0; I != 1000; ++I) {
          if (A[I] > B[I])
            MinIdx = Idx;
          Idx -= 1;
        } return MinIdx;
        , int32_t);
    checkVectorFunction<int32_t, int32_t>(
        ScalarFn, VectorFn, "findlast_icmp_s32_start_decreasing_induction7");
  }

  return 0;
}
