#include <stdio.h>
#include <algorithm>
#include <getopt.h>
#include <math.h>
#include "CS149intrin.h"
#include "logger.h"
using namespace std;

#define EXP_MAX 10

Logger CS149Logger;

void usage(const char* progname);
void initValue(float* values, int* exponents, float* output, float* gold, unsigned int N);
void absSerial(float* values, float* output, int N);
void absVector(float* values, float* output, int N);
void clampedExpSerial(float* values, int* exponents, float* output, int N);
void clampedExpVector(float* values, int* exponents, float* output, int N);
float arraySumSerial(float* values, int N);
float arraySumVector(float* values, int N);
bool verifyResult(float* values, int* exponents, float* output, float* gold, int N);

int main(int argc, char * argv[]) {
  int N = 16;
  bool printLog = false;

  // parse commandline options ////////////////////////////////////////////
  int opt;
  static struct option long_options[] = {
    {"size", 1, 0, 's'},
    {"log", 0, 0, 'l'},
    {"help", 0, 0, '?'},
    {0 ,0, 0, 0}
  };

  while ((opt = getopt_long(argc, argv, "s:l?", long_options, NULL)) != EOF) {

    switch (opt) {
      case 's':
        N = atoi(optarg);
        if (N <= 0) {
          printf("Error: Workload size is set to %d (<0).\n", N);
          return -1;
        }
        break;
      case 'l':
        printLog = true;
        break;
      case '?':
      default:
        usage(argv[0]);
        return 1;
    }
  }


  float* values = new float[N+VECTOR_WIDTH];
  int* exponents = new int[N+VECTOR_WIDTH];
  float* output = new float[N+VECTOR_WIDTH];
  float* gold = new float[N+VECTOR_WIDTH];
  initValue(values, exponents, output, gold, N);

  clampedExpSerial(values, exponents, gold, N);
  clampedExpVector(values, exponents, output, N);

  //absSerial(values, gold, N);
  //absVector(values, output, N);

  printf("\e[1;31mCLAMPED EXPONENT\e[0m (required) \n");
  bool clampedCorrect = verifyResult(values, exponents, output, gold, N);
  if (printLog) CS149Logger.printLog();
  CS149Logger.printStats();

  printf("************************ Result Verification *************************\n");
  if (!clampedCorrect) {
    printf("@@@ Failed!!!\n");
  } else {
    printf("Passed!!!\n");
  }

  printf("\n\e[1;31mARRAY SUM\e[0m (bonus) \n");
  if (N % VECTOR_WIDTH == 0) {
    float sumGold = arraySumSerial(values, N);
    float sumOutput = arraySumVector(values, N);
    float epsilon = 0.1;
    bool sumCorrect = abs(sumGold - sumOutput) < epsilon * 2;
    if (!sumCorrect) {
      printf("Expected %f, got %f\n.", sumGold, sumOutput);
      printf("@@@ Failed!!!\n");
    } else {
      printf("Passed!!!\n");
    }
  } else {
    printf("Must have N %% VECTOR_WIDTH == 0 for this problem (VECTOR_WIDTH is %d)\n", VECTOR_WIDTH);
  }

  delete [] values;
  delete [] exponents;
  delete [] output;
  delete [] gold;

  return 0;
}

void usage(const char* progname) {
  printf("Usage: %s [options]\n", progname);
  printf("Program Options:\n");
  printf("  -s  --size <N>     Use workload size N (Default = 16)\n");
  printf("  -l  --log          Print vector unit execution log\n");
  printf("  -?  --help         This message\n");
}

void initValue(float* values, int* exponents, float* output, float* gold, unsigned int N) {

  for (unsigned int i=0; i<N+VECTOR_WIDTH; i++)
  {
    // random input values
    values[i] = -1.f + 4.f * static_cast<float>(rand()) / RAND_MAX;
    exponents[i] = rand() % EXP_MAX;
    output[i] = 0.f;
    gold[i] = 0.f;
  }

}

bool verifyResult(float* values, int* exponents, float* output, float* gold, int N) {
  int incorrect = -1;
  float epsilon = 0.00001;
  for (int i=0; i<N+VECTOR_WIDTH; i++) {
    if ( abs(output[i] - gold[i]) > epsilon ) {
      incorrect = i;
      break;
    }
  }

  if (incorrect != -1) {
    if (incorrect >= N)
      printf("You have written to out of bound value!\n");
    printf("Wrong calculation at value[%d]!\n", incorrect);
    printf("value  = ");
    for (int i=0; i<N; i++) {
      printf("% f ", values[i]);
    } printf("\n");

    printf("exp    = ");
    for (int i=0; i<N; i++) {
      printf("% 9d ", exponents[i]);
    } printf("\n");

    printf("output = ");
    for (int i=0; i<N; i++) {
      printf("% f ", output[i]);
    } printf("\n");

    printf("gold   = ");
    for (int i=0; i<N; i++) {
      printf("% f ", gold[i]);
    } printf("\n");
    return false;
  }
  printf("Results matched with answer!\n");
  return true;
}

// computes the absolute value of all elements in the input array
// values, stores result in output
void absSerial(float* values, float* output, int N) {
  for (int i=0; i<N; i++) {
    float x = values[i];
    if (x < 0) {
      output[i] = -x;
    } else {
      output[i] = x;
    }
  }
}


// implementation of absSerial() above, but it is vectorized using CS149 intrinsics
void absVector(float* values, float* output, int N) {
  __cs149_vec_float x;
  __cs149_vec_float result;
  __cs149_vec_float zero = _cs149_vset_float(0.f);
  __cs149_mask maskAll, maskIsNegative, maskIsNotNegative;

//  Note: Take a careful look at this loop indexing.  This example
//  code is not guaranteed to work when (N % VECTOR_WIDTH) != 0.
//  Why is that the case?
  for (int i=0; i<N; i+=VECTOR_WIDTH) {

    // All ones
    maskAll = _cs149_init_ones();

    // All zeros
    maskIsNegative = _cs149_init_ones(0);

    // Load vector of values from contiguous memory addresses
    _cs149_vload_float(x, values+i, maskAll);               // x = values[i];

    // Set mask according to predicate
    _cs149_vlt_float(maskIsNegative, x, zero, maskAll);     // if (x < 0) {

    // Execute instruction using mask ("if" clause)
    _cs149_vsub_float(result, zero, x, maskIsNegative);      //   output[i] = -x;

    // Inverse maskIsNegative to generate "else" mask
    maskIsNotNegative = _cs149_mask_not(maskIsNegative);     // } else {

    // Execute instruction ("else" clause)
    _cs149_vload_float(result, values+i, maskIsNotNegative); //   output[i] = x; }

    // Write results back to memory
    _cs149_vstore_float(output+i, result, maskAll);
  }
}


// accepts an array of values and an array of exponents
//
// For each element, compute values[i]^exponents[i] and clamp value to
// 9.999.  Store result in output.
void clampedExpSerial(float* values, int* exponents, float* output, int N) {
  for (int i=0; i<N; i++) {
    float x = values[i];
    int y = exponents[i];
    if (y == 0) {
      output[i] = 1.f;
    } else {
      float result = x;
      int count = y - 1;
      while (count > 0) {
        result *= x;
        count--;
      }
      if (result > 9.999999f) {
        result = 9.999999f;
      }
      output[i] = result;
    }
  }
}

// void clampedExpVector(float* values, int* exponents, float* output, int N) {

//   //
//   // CS149 STUDENTS TODO: Implement your vectorized version of
//   // clampedExpSerial() here.
//   //
//   // Your solution should work for any value of
//   // N and VECTOR_WIDTH, not just when VECTOR_WIDTH divides N
//   //
//   __cs149_vec_float x;
//   __cs149_vec_int y;
//   __cs149_vec_int count;
//   __cs149_vec_float result;
//   __cs149_vec_float one = _cs149_vset_float(1.f);
//   __cs149_vec_int oneInt = _cs149_vset_int(1);
//   __cs149_vec_int zero = _cs149_vset_int(0);
//   __cs149_vec_int exponent;
//   __cs149_mask maskAll, maskIsNotZero,maskIsZero,needDeal;
//   __cs149_vec_float gtNum = _cs149_vset_float(9.999999f);
//   int num = 0;


//   for(int i=0;i< (N-N % VECTOR_WIDTH);i+=VECTOR_WIDTH){
//     // All ones
//     maskAll = _cs149_init_ones();
//     // All zeros
//     maskIsZero = _cs149_init_ones(0);
//     _cs149_vload_float(x, values+i, maskAll); // float x = values[i];
//     _cs149_vload_int(y,exponents+i,maskAll);// int y = exponents[i];

//     _cs149_veq_int(maskIsZero,y,zero,maskAll);//if (y == 0) 
//     _cs149_vstore_float(output+i, one, maskIsZero);//output[i] = 1.f;

//     maskIsNotZero = _cs149_mask_not(maskIsZero);//else{
//     _cs149_vload_float(result, values+i, maskIsNotZero);//float result = x
//     _cs149_vsub_int(exponent, y, oneInt, maskIsNotZero); //int count = y-1;
//     while (_cs149_cntbits(maskIsNotZero) > 0) {
//       _cs149_vmult_float(result, result, x, maskIsNotZero);
//       _cs149_vsub_int(exponent, exponent, oneInt, maskIsNotZero);
//       _cs149_vgt_int(maskIsNotZero, exponent, zero, maskIsNotZero);
//       //每次都更新大于0的掩码，这样就可以让这些没算完的接着算
//     }//   while (count > 0) {
//     //     result *= x;
//     //     count--;
//     //   }
//     _cs149_vgt_float(needDeal, result, gtNum, maskAll);
//     _cs149_vset_float(result, 9.999999f, needDeal);
//     _cs149_vstore_float(output+i,result,maskAll);
//     num =i;
//   }
//   for (int num= (N-N % VECTOR_WIDTH );num < N; num++) {
//     float x = values[num];
//     int y = exponents[num];
//     float result;
//     if (y == 0) {
//       result = 1.f;
//     } else {
//       result = x;
//       for (int j = 1; j < y; j++) {
//         result *= x;
//       }
//     }
//     output[num] = (result > 9.999999f) ? 9.999999f : result;
//   }

// }

void clampedExpVector(float* values, int* exponents, float* output, int N) {

  //
  // CS149 STUDENTS TODO: Implement your vectorized version of
  // clampedExpSerial() here.
  //
  // Your solution should work for any value of
  // N and VECTOR_WIDTH, not just when VECTOR_WIDTH divides N
  //
  __cs149_vec_float x;
  __cs149_mask mask;
  __cs149_mask all_mask = _cs149_init_ones();
  __cs149_vec_int exp;
  __cs149_vec_int const_zero = _cs149_vset_int(0);
  __cs149_vec_int const_one = _cs149_vset_int(1);
  __cs149_vec_float const_99 = _cs149_vset_float(9.999999f);
  __cs149_vec_float res;

  for(int i=0; i< (N - N % VECTOR_WIDTH); i+=VECTOR_WIDTH){
    mask = _cs149_init_ones(); // 这个如果都是0 那么代表结束游戏
    _cs149_vload_float(x, values+i, mask);
    _cs149_vset_float(res, 1.0f, mask);
    _cs149_vload_int(exp, exponents +i, mask);
    _cs149_vgt_int(mask, exp, const_zero, mask);
    _cs149_vlt_float(mask, x, const_99, mask);
    while(_cs149_cntbits(mask)!=0){
      _cs149_vmult_float(res, res, x, mask);
      _cs149_vsub_int(exp, exp, const_one, mask);
      _cs149_vgt_int(mask, exp, const_zero, mask);
      _cs149_vlt_float(mask, x, const_99, mask);
    }
    _cs149_vgt_float(mask, res, const_99, all_mask);
    _cs149_vmove_float(res, const_99, mask);
    _cs149_vstore_float(output+i, res, all_mask);
  }
  for(int i= (N- N % VECTOR_WIDTH); i < N; i++){ // 如果出现除不尽的情况有两种做法，padding和单独计算，这里简单点直接算
    float x = values[i];
    int y = exponents[i];
    if (y == 0) {
      output[i] = 1.f;
    } else {
      float result = x;
      int count = y - 1;
      while (count > 0) {
        result *= x;
        count--;
      }
      if (result > 9.999999f) {
        result = 9.999999f;
      }
      output[i] = result;
    }
  }
  
}

// returns the sum of all elements in values
float arraySumSerial(float* values, int N) {
  float sum = 0;
  for (int i=0; i<N; i++) {
    sum += values[i];
  }

  return sum;
}

// returns the sum of all elements in values
// You can assume N is a multiple of VECTOR_WIDTH
// You can assume VECTOR_WIDTH is a power of 2
float arraySumVector(float* values, int N) {
  
  //
  // CS149 STUDENTS TODO: Implement your vectorized version of arraySumSerial here
  //
    __cs149_vec_float x;
    __cs149_vec_float sum=_cs149_vset_float(0.0f);
   __cs149_mask maskAll = _cs149_init_ones();
  for (int i=0; i<N; i+=VECTOR_WIDTH) {
    _cs149_vload_float(x, values+i, maskAll);
    _cs149_vadd_float(sum, sum, x, maskAll);
  }
    __cs149_vec_float temp;
 for(int stride = VECTOR_WIDTH;stride!=1;stride/=2){
  _cs149_hadd_float(temp,sum);
  _cs149_interleave_float(sum,temp);
 }
  return sum.value[0];
}