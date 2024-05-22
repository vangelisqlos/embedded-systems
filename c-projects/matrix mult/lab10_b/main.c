/*
 * HPY411_Lab_10_float.c
 * HPY411 Lab 10 code in C for ATmega16
 * 3x3 matrix multiplication with float matrices
 * Created: 19/12/2020 11:17:56 μμ
 * Author : Evangelos Kioulos 2016030056
 */ 

/***********************************************************************************
 * This program demonstrates simple 3x3 matrix multiplication. (A x B = C)
 * Matrices are float.
 *
 * Address 0x0060 = First address of matrix A
 * Address 0x0095 = First address of matrix B
 * Address 0x00CA = First address of matrix C (Result Matrix)
 *
 * Matrices are 3x3, each matrix needs 36 bytes of ram. 
 * (9 items in each matrix times size of float)
 *
 ***********************************************************************************/


#include <avr/io.h>

/* Matrix address in RAM */
#define MATRIX_A_ADDRESS 0x60
#define MATRIX_B_ADDRESS 0x95
#define MATRIX_C_ADDRESS 0xCA

/* Function declaration */
void multiply_float(float *A, float *B, float *C);
void init_A(void);
void init_B(void);
void init_C(void);

int main(void)
{
	/* Initialize addresses of the matrices */
	float *A = (float *) MATRIX_A_ADDRESS;
	float *B = (float *) MATRIX_B_ADDRESS;
	float *C = (float *) MATRIX_C_ADDRESS;
	/* Initialize matrix values */
	init_A();
	init_B();
	init_C();
	
	/* Multiply matrices A and B. Result is stored in matrix C. (AxB=C) */
	multiply_float(A,B,C);
	
	while (1)
	{
		asm("nop"); // No operation. Not necessary, used for debugging purposes.
	}
}

/*
 * Initialize 3x3 Matrix A
 * First address of matrix A is 0x0060
 *
 * The matrix is stored in the form of an 1-dimensional Array.
 * Matrix Values are stored in the array like this:
 *
 *			|A[0] A[1] A[2]|
 *		A = |A[4] A[5] A[6]|
 *			|A[7] A[8] A[9]|
 *
 */
void init_A(){
	float *A = (float *) MATRIX_A_ADDRESS;
	A[0] = 150.32;
	A[1] = 12.253;
	A[2] = -45.05;
	A[3] = 6.371;
	A[4] = 21.0;
	A[5] = 52.12;
	A[6] = 104.73;
	A[7] = 89.99;
	A[8] = -32.007;
}

/*
 * Initialize 3x3 Matrix B
 * First address of matrix B is 0x0095
  * The matrix is stored in the form of an 1-dimensional Array.
  * Matrix Values are stored in the array like this:
  *
  *			|B[0] B[1] B[2]|
  *		B = |B[4] B[5] B[6]|
  *			|B[7] B[8] B[9]|
  *
 */
void init_B(){
	float *B = (float *) MATRIX_B_ADDRESS;
	B[0] = -3.2;
	B[1] = 27.52;
	B[2] = 62.9;
	B[3] = 10.0;
	B[4] = -128.4;
	B[5] = 46.3;
	B[6] = 72.72;
	B[7] = 1.0;
	B[8] = 69.3;
}

/*
 * Initialize 3x3 Matrix C
 * First address of matrix C is 0x00CA
 * Result matrix, filled with zeros in initialization
 *
 * The matrix is stored in the form of an 1-dimensional Array.
 * Matrix Values are stored in the array like this:
 *
 *			|C[0] C[1] C[2]|
 *		C = |C[4] C[5] C[6]|
 *			|C[7] C[8] C[9]|
 *
 */
void init_C(){
	float *C = (float *) MATRIX_C_ADDRESS;
	int i;
	for(i=0; i<9; i++){
		C[i] = 0;
	}
}

/*
 * 3x3 Matrix multiplication
 * Matrices A and B get multiplied. 
 * The result is stored in matrix C.
 */
void multiply_float(float *A, float *B, float *C){
	int i,j,k;
	float sum;
	//run through rows of matrix A
	for(i=0; i<3; i++){
		//run through collumns of matrix B
		for(j=0; j<3; j++){
			sum = 0;
			//run through each value in each row and collumn
			for(k=0; k<3; k++){
				// calculate the sum of products for each position in the new matrix
				sum = sum + A[i*3 + k] * B[k*3 + j];
			}
			// Store new value in the result matrix
			C[i*3 + j] = sum;
		}
	}
}


