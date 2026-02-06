#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <stdint.h>

#include "common.h"

// Test functions for "find last VALUE" pattern using ExtractLastActive.
// Pattern: result = (A[I] >= B[I]) ? result : A[I]
// This finds the LAST A[I] where A[I] < B[I].
// Using noinline to prevent constant folding and ensure vectorizer runs.

__attribute__((noinline))
int32_t scalar_findlast_value(int32_t* A, int32_t* B, int32_t n) {
  int32_t result = 1;
  #pragma clang loop vectorize(disable) interleave_count(1)
  for (int32_t i = 0; i < n; i++) {
    result = (A[i] >= B[i]) ? result : A[i];
  }
  return result;
}

__attribute__((noinline))
int32_t vector_findlast_value_ic2(int32_t* A, int32_t* B, int32_t n) {
  int32_t result = 1;
  #pragma clang loop vectorize_width(4) interleave_count(2)
  for (int32_t i = 0; i < n; i++) {
    result = (A[i] >= B[i]) ? result : A[i];
  }
  return result;
}

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
    if (Reference != ToCheck) {
      std::cerr << "Miscompare\n";
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
    if (Reference != ToCheck) {
      std::cerr << "Miscompare\n";
      exit(1);
    }
  }
}

int main(void) {
  rng = std::mt19937(15);

#define INC_COND(Start, Step, RetTy) for (RetTy I = Start; I < TC; I += Step)
#define DEC_COND(End, Step, RetTy) for (RetTy I = TC; I > End; I -= Step)

#define DEFINE_FINDLAST_LOOP_BODY(TrueVal, FalseVal, ForCond)                  \
  ForCond { Rdx = A[I] > B[I] ? TrueVal : FalseVal; }                          \
  return Rdx;

  {
    // Find the last index where A[I] > B[I] and update 32-bits Rdx when the
    // condition is true.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
	int32_t Rdx = -1;,
	DEFINE_FINDLAST_LOOP_BODY(
	    /* TrueVal= */ I, /* FalseVal= */ Rdx,
	    /* ForCond= */
	    INC_COND(/* Start= */ 0, /* Step= */ 1, /* RetTy= */ int32_t)),
	int32_t);
    checkVectorFunction<int32_t, int32_t>(ScalarFn, VectorFn,
					  "findlast_icmp_s32_true_update");
    checkVectorFunction<int32_t, float>(ScalarFn, VectorFn,
					"findlast_fcmp_s32_true_update");
  }

  {
    // Find the last index where A[I] > B[I] and update 16-bits Rdx when the
    // condition is true.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
	int16_t Rdx = -1;,
	DEFINE_FINDLAST_LOOP_BODY(
	    /* TrueVal= */ I, /* FalseVal= */ Rdx,
	    /* ForCond= */
	    INC_COND(/* Start= */ 0, /* Step= */ 1, /* RetTy= */ int16_t)),
	int16_t);
    checkVectorFunction<int16_t, int16_t>(ScalarFn, VectorFn,
					  "findlast_icmp_s16_true_update");
  }

  {
    // Update 32-bits Rdx when the condition A[I] > B[I] is false.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
	int32_t Rdx = -1;,
	DEFINE_FINDLAST_LOOP_BODY(
	    /* TrueVal= */ Rdx, /* FalseVal= */ I,
	    /* ForCond= */
	    INC_COND(/* Start= */ 0, /* Step= */ 1, /* RetTy= */ int32_t)),
	int32_t);
    checkVectorFunction<int32_t, int32_t>(ScalarFn, VectorFn,
					  "findlast_icmp_s32_false_update");
    checkVectorFunction<int32_t, float>(ScalarFn, VectorFn,
					"findlast_fcmp_s32_false_update");
  }

  {
    // Update 16-bits Rdx when the condition A[I] > B[I] is false.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
	int16_t Rdx = -1;,
	DEFINE_FINDLAST_LOOP_BODY(
	    /* TrueVal= */ Rdx, /* FalseVal= */ I,
	    /* ForCond= */
	    INC_COND(/* Start= */ 0, /* Step= */ 1, /* RetTy= */ int16_t)),
	int16_t);
    checkVectorFunction<int16_t, int16_t>(ScalarFn, VectorFn,
					  "findlast_icmp_s16_false_update");
  }

  {
    // Find the last 32-bits index with the start value TC.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
	int32_t Rdx = TC;,
	DEFINE_FINDLAST_LOOP_BODY(
	    /* TrueVal= */ I, /* FalseVal= */ Rdx,
	    /* ForCond= */
	    INC_COND(/* Start= */ 0, /* Step= */ 1, /* RetTy= */ int32_t)),
	int32_t);
    checkVectorFunction<int32_t, int32_t>(ScalarFn, VectorFn,
					  "findlast_icmp_s32_start_TC");
    checkVectorFunction<int32_t, float>(ScalarFn, VectorFn,
					"findlast_fcmp_s32_start_TC");
  }

  {
    // Find the last 16-bits index with the start value TC.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
	int16_t Rdx = TC;,
	DEFINE_FINDLAST_LOOP_BODY(
	    /* TrueVal= */ I, /* FalseVal= */ Rdx,
	    /* ForCond= */
	    INC_COND(/* Start= */ 0, /* Step= */ 1, /* RetTy= */ int16_t)),
	int16_t);
    checkVectorFunction<int16_t, int16_t>(ScalarFn, VectorFn,
					  "findlast_icmp_s16_start_TC");
  }

  {
    // Increment the 32-bits induction variable by 2.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
	int32_t Rdx = -1;,
	DEFINE_FINDLAST_LOOP_BODY(
	    /* TrueVal= */ I, /* FalseVal= */ Rdx,
	    /* ForCond= */
	    INC_COND(/* Start= */ 0, /* Step= */ 2, /* RetTy= */ int32_t)),
	int32_t);
    checkVectorFunction<int32_t, int32_t>(ScalarFn, VectorFn,
					  "findlast_icmp_s32_inc_2");
    checkVectorFunction<int32_t, float>(ScalarFn, VectorFn,
					"findlast_fcmp_s32_inc_2");
  }

  {
    // Increment the 16-bits induction variable by 2.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
	int16_t Rdx = -1;,
	DEFINE_FINDLAST_LOOP_BODY(
	    /* TrueVal= */ I, /* FalseVal= */ Rdx,
	    /* ForCond= */
	    INC_COND(/* Start= */ 0, /* Step= */ 2, /* RetTy= */ int16_t)),
	int16_t);
    checkVectorFunction<int16_t, int16_t>(ScalarFn, VectorFn,
					  "findlast_icmp_s16_inc_2");
  }

  {
    // Check with decreasing 32-bits induction variable.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
	int32_t Rdx = -1;,
	DEFINE_FINDLAST_LOOP_BODY(
	    /* TrueVal= */ I, /* FalseVal= */ Rdx,
	    /* ForCond= */
	    DEC_COND(/* End= */ 0, /* Step= */ 1, /* RetTy= */ int32_t)),
	int32_t);
    checkVectorFunction<int32_t, int32_t>(
        ScalarFn, VectorFn, "findlast_icmp_s32_start_decreasing_induction");
    checkVectorFunction<int32_t, float>(
        ScalarFn, VectorFn, "findlast_fcmp_s32_start_decreasing_induction");
  }

  {
    // Check with decreasing 16-bits induction variable.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
	int16_t Rdx = -1;,
	DEFINE_FINDLAST_LOOP_BODY(
	    /* TrueVal= */ I, /* FalseVal= */ Rdx,
	    /* ForCond= */
	    DEC_COND(/* End= */ 0, /* Step= */ 1, /* RetTy= */ int16_t)),
	int16_t);
    checkVectorFunction<int16_t, int16_t>(
        ScalarFn, VectorFn, "findlast_icmp_s16_start_decreasing_induction");
  }

  {
    // Check with 32-bits the induction variable starts from 3.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
	int32_t Rdx = -1;,
	DEFINE_FINDLAST_LOOP_BODY(
	    /* TrueVal= */ I, /* FalseVal= */ Rdx,
	    /* ForCond= */
	    INC_COND(/* Start= */ 3, /* Step= */ 1, /* RetTy= */ int32_t)),
	int32_t);
    checkVectorFunction<int32_t, int32_t>(ScalarFn, VectorFn,
					  "findlast_icmp_s32_iv_start_3");
    checkVectorFunction<int32_t, float>(ScalarFn, VectorFn,
					"findlast_fcmp_s32_iv_start_3");
  }

  {
    // Check with 16-bits the induction variable starts from 3.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
	int16_t Rdx = -1;,
	DEFINE_FINDLAST_LOOP_BODY(
	    /* TrueVal= */ I, /* FalseVal= */ Rdx,
	    /* ForCond= */
	    INC_COND(/* Start= */ 3, /* Step= */ 1, /* RetTy= */ int16_t)),
	int16_t);
    checkVectorFunction<int16_t, int16_t>(ScalarFn, VectorFn,
					  "findlast_icmp_s16_iv_start_3");
  }

  {
    // Check with start value of 3 and 32-bits induction variable starts at 3.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
	int32_t Rdx = 3;,
	DEFINE_FINDLAST_LOOP_BODY(
	    /* TrueVal= */ I, /* FalseVal= */ Rdx,
	    /* ForCond= */
	    INC_COND(/* Start= */ 3, /* Step= */ 1, /* RetTy= */ int32_t)),
	int32_t);
    checkVectorFunction<int32_t, int32_t>(
	ScalarFn, VectorFn, "findlast_icmp_s32_start_3_iv_start_3");
    checkVectorFunction<int32_t, float>(ScalarFn, VectorFn,
					"findlast_fcmp_s32_start_3_iv_start_3");
  }

  {
    // Check with start value of 3 and 16-bits induction variable starts at 3.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
	int16_t Rdx = 3;,
	DEFINE_FINDLAST_LOOP_BODY(
	    /* TrueVal= */ I, /* FalseVal= */ Rdx,
	    /* ForCond= */
	    INC_COND(/* Start= */ 3, /* Step= */ 1, /* RetTy= */ int16_t)),
	int16_t);
    checkVectorFunction<int16_t, int16_t>(
	ScalarFn, VectorFn, "findlast_icmp_s16_start_3_iv_start_3");
  }

  {
    // Check with start value of 2 and 32-bits induction variable starts at 3.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
	int32_t Rdx = 2;,
	DEFINE_FINDLAST_LOOP_BODY(
	    /* TrueVal= */ I, /* FalseVal= */ Rdx,
	    /* ForCond= */
	    INC_COND(/* Start= */ 3, /* Step= */ 1, /* RetTy= */ int32_t)),
	int32_t);
    checkVectorFunction<int32_t, int32_t>(
	ScalarFn, VectorFn, "findlast_icmp_s32_start_2_iv_start_3");
    checkVectorFunction<int32_t, float>(ScalarFn, VectorFn,
					"findlast_fcmp_s32_start_2_iv_start_3");
  }

  {
    // Check with start value of 2 and 16-bits induction variable starts at 3.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
	int16_t Rdx = 2;,
	DEFINE_FINDLAST_LOOP_BODY(
	    /* TrueVal= */ I, /* FalseVal= */ Rdx,
	    /* ForCond= */
	    INC_COND(/* Start= */ 3, /* Step= */ 1, /* RetTy= */ int16_t)),
	int16_t);
    checkVectorFunction<int16_t, int16_t>(
	ScalarFn, VectorFn, "findlast_icmp_s16_start_2_iv_start_3");
  }

  {
    // Check with start value of 4 and 32-bits induction variable starts at 3.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
	int32_t Rdx = 4;,
	DEFINE_FINDLAST_LOOP_BODY(
	    /* TrueVal= */ I, /* FalseVal= */ Rdx,
	    /* ForCond= */
	    INC_COND(/* Start= */ 3, /* Step= */ 1, /* RetTy= */ int32_t)),
	int32_t);
    checkVectorFunction<int32_t, int32_t>(
	ScalarFn, VectorFn, "findlast_icmp_s32_start_4_iv_start_3");
    checkVectorFunction<int32_t, float>(ScalarFn, VectorFn,
					"findlast_fcmp_s32_start_4_iv_start_3");
  }

  {
    // Check with start value of 4 and 16-bits induction variable starts at 3.
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE(
	int16_t Rdx = 4;,
	DEFINE_FINDLAST_LOOP_BODY(
	    /* TrueVal= */ I, /* FalseVal= */ Rdx,
	    /* ForCond= */
	    INC_COND(/* Start= */ 3, /* Step= */ 1, /* RetTy= */ int16_t)),
	int16_t);
    checkVectorFunction<int16_t, int16_t>(
	ScalarFn, VectorFn, "findlast_icmp_s16_start_4_iv_start_3");
  }

  // Tests with explicit interleave count to verify multi-part handling.
  // The bug manifests when both parts have active lanes in the same
  // vector iteration: Part 0 (indices 0-3) and Part 1 (indices 4-7).
  // Expected: highest active index should win. Bug: lower part overwrites.
  {
    std::cout << "Checking findlast_interleave2_both_parts_active\n";
    // VF=4, IC=2 means 8 elements per vector iteration
    // Part 0: indices 0-3, Part 1: indices 4-7
    DEFINE_SCALAR_AND_VECTOR_FN2_TYPE_VF_INTERLEAVE(
        int32_t Rdx = -1;,
        DEFINE_FINDLAST_LOOP_BODY(
            /* TrueVal= */ I, /* FalseVal= */ Rdx,
            /* ForCond= */
            INC_COND(/* Start= */ 0, /* Step= */ 1, /* RetTy= */ int32_t)),
        int32_t, 4, 2);

    // Test case: matches at index 0 (Part 0) and index 5 (Part 1)
    // Expected result: 5 (highest active index)
    // Bug would return: 0 (Part 0 overwrites Part 1's result)
    {
      int32_t Src1[8] = {10, 0, 0, 0, 0, 20, 0, 0};
      int32_t Src2[8] = {0, 0, 0, 0, 0, 0, 0, 0};
      auto Reference = ScalarFn(&Src1[0], &Src2[0], 8);
      auto ToCheck = VectorFn(&Src1[0], &Src2[0], 8);
      if (Reference != ToCheck) {
        std::cerr << "Miscompare: expected " << Reference << ", got " << ToCheck
                  << "\n";
        exit(1);
      }
    }

    // Test case: matches at indices 1, 2, 6
    // Expected result: 6
    {
      int32_t Src1[8] = {0, 10, 10, 0, 0, 0, 20, 0};
      int32_t Src2[8] = {0, 0, 0, 0, 0, 0, 0, 0};
      auto Reference = ScalarFn(&Src1[0], &Src2[0], 8);
      auto ToCheck = VectorFn(&Src1[0], &Src2[0], 8);
      if (Reference != ToCheck) {
        std::cerr << "Miscompare: expected " << Reference << ", got " << ToCheck
                  << "\n";
        exit(1);
      }
    }

    // Test case: match only in Part 1 (index 7)
    // Expected result: 7
    {
      int32_t Src1[8] = {0, 0, 0, 0, 0, 0, 0, 20};
      int32_t Src2[8] = {0, 0, 0, 0, 0, 0, 0, 0};
      auto Reference = ScalarFn(&Src1[0], &Src2[0], 8);
      auto ToCheck = VectorFn(&Src1[0], &Src2[0], 8);
      if (Reference != ToCheck) {
        std::cerr << "Miscompare: expected " << Reference << ", got " << ToCheck
                  << "\n";
        exit(1);
      }
    }

    // Test case: match only in Part 0 (index 3)
    // Expected result: 3
    {
      int32_t Src1[8] = {0, 0, 0, 20, 0, 0, 0, 0};
      int32_t Src2[8] = {0, 0, 0, 0, 0, 0, 0, 0};
      auto Reference = ScalarFn(&Src1[0], &Src2[0], 8);
      auto ToCheck = VectorFn(&Src1[0], &Src2[0], 8);
      if (Reference != ToCheck) {
        std::cerr << "Miscompare: expected " << Reference << ", got " << ToCheck
                  << "\n";
        exit(1);
      }
    }
  }

  // Tests for "find last VALUE" pattern which uses ExtractLastActive intrinsic.
  // Pattern: result = (A[I] >= B[I]) ? result : A[I]
  // This finds the LAST A[I] where A[I] < B[I].
  // Bug: With IC=2, Part 0's result overwrites Part 1's when both have active
  // lanes. For FindLast, Part 1 (higher indices) should win.
  //
  // NOTE: These use noinline functions to prevent constant folding at compile
  // time, ensuring the vectorized code path is actually exercised at runtime.
  {
    std::cout << "Checking findlast_value_interleave2_extract_last_active\n";
    // VF=4, IC=2 means 8 elements per vector iteration
    // Part 0: indices 0-3, Part 1: indices 4-7

    // Test case: Active at index 0 (Part 0) and index 5 (Part 1)
    // A[0]=10 < B[0]=100, A[5]=50 < B[5]=100
    // Expected result: 50 (last active is at index 5)
    // Bug would return: 10 (Part 0 overwrites Part 1's result)
    {
      int32_t Src1[8] = {10, 200, 200, 200, 200, 50, 200, 200};
      int32_t Src2[8] = {100, 100, 100, 100, 100, 100, 100, 100};
      auto Reference = scalar_findlast_value(&Src1[0], &Src2[0], 8);
      auto ToCheck = vector_findlast_value_ic2(&Src1[0], &Src2[0], 8);
      if (Reference != ToCheck) {
        std::cerr << "Miscompare: expected " << Reference << ", got " << ToCheck
                  << "\n";
        exit(1);
      }
    }

    // Test case: Active at indices 1, 2, 6
    // Expected result: 60
    {
      int32_t Src1[8] = {200, 10, 20, 200, 200, 200, 60, 200};
      int32_t Src2[8] = {100, 100, 100, 100, 100, 100, 100, 100};
      auto Reference = scalar_findlast_value(&Src1[0], &Src2[0], 8);
      auto ToCheck = vector_findlast_value_ic2(&Src1[0], &Src2[0], 8);
      if (Reference != ToCheck) {
        std::cerr << "Miscompare: expected " << Reference << ", got " << ToCheck
                  << "\n";
        exit(1);
      }
    }

    // Test case: Active only in Part 1 (index 7)
    // Expected result: 70
    {
      int32_t Src1[8] = {200, 200, 200, 200, 200, 200, 200, 70};
      int32_t Src2[8] = {100, 100, 100, 100, 100, 100, 100, 100};
      auto Reference = scalar_findlast_value(&Src1[0], &Src2[0], 8);
      auto ToCheck = vector_findlast_value_ic2(&Src1[0], &Src2[0], 8);
      if (Reference != ToCheck) {
        std::cerr << "Miscompare: expected " << Reference << ", got " << ToCheck
                  << "\n";
        exit(1);
      }
    }

    // Test case: Active only in Part 0 (index 3)
    // Expected result: 30
    {
      int32_t Src1[8] = {200, 200, 200, 30, 200, 200, 200, 200};
      int32_t Src2[8] = {100, 100, 100, 100, 100, 100, 100, 100};
      auto Reference = scalar_findlast_value(&Src1[0], &Src2[0], 8);
      auto ToCheck = vector_findlast_value_ic2(&Src1[0], &Src2[0], 8);
      if (Reference != ToCheck) {
        std::cerr << "Miscompare: expected " << Reference << ", got " << ToCheck
                  << "\n";
        exit(1);
      }
    }

    // Test case: No active lanes - should return sentinel (1)
    {
      int32_t Src1[8] = {200, 200, 200, 200, 200, 200, 200, 200};
      int32_t Src2[8] = {100, 100, 100, 100, 100, 100, 100, 100};
      auto Reference = scalar_findlast_value(&Src1[0], &Src2[0], 8);
      auto ToCheck = vector_findlast_value_ic2(&Src1[0], &Src2[0], 8);
      if (Reference != ToCheck) {
        std::cerr << "Miscompare: expected " << Reference << ", got " << ToCheck
                  << "\n";
        exit(1);
      }
    }
  }

  return 0;
}
