#include<stdio.h>
#include<stdlib.h>

void multiply(int A[3][3], int B[3][3], int C[3][3]);
void multiply_float(float A[3][3], float B[3][3], float C[3][3]);

void multiply(int A[3][3], int B[3][3], int C[3][3]){
    int i,j,k,sum;

    for (i = 0; i <= 2; i++) {
      for (j = 0; j <= 2; j++) {
         sum = 0;
         for (k = 0; k <= 2; k++) {
            sum = sum + A[i][k] * B[k][j];
         }
         C[i][j] = sum;
      }
   }
}

void multiply_float(float A[3][3], float B[3][3], float C[3][3]){
    int i,j,k;
    float sum;

    for (i = 0; i <= 2; i++) {
      for (j = 0; j <= 2; j++) {
         sum = 0;
         for (k = 0; k <= 2; k++) {
            sum = sum + A[i][k] * B[k][j];
         }
         C[i][j] = sum;
      }
   }

}


int main(int argc, char const *argv[])
{
    int i,j,k;
    int A[3][3] = {{1,2,3},{4,5,6},{7,8,9}};
    int B[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
    int C[3][3] = {{0,0,0},{0,0,0},{0,0,0}};

    float Af[3][3] = {{1.05, 2.02, 3.1},{4.7, 5.4, 6.2},{7, 8.8, 9.1}};
    float Bf[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
    float Cf[3][3] = {{0,0,0},{0,0,0},{0,0,0}};

    multiply(A,B,C);

    printf("\nMultiplication Of Two Matrices : \n");
    for (i = 0; i < 3; i++) {
       for (j = 0; j < 3; j++) {
          printf(" %d ", C[i][j]);
       }
       printf("\n");
    }

    multiply_float(Af,Bf,Cf);

    printf("\nMultiplication Of Two Float Matrices : \n");
    for (i = 0; i < 3; i++) {
       for (j = 0; j < 3; j++) {
          printf(" %f ", Cf[i][j]);
       }
       printf("\n");
    }

    return 0;
}



