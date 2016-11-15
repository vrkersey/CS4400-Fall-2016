/*******************************************
 * Solutions for the CS:APP Performance Lab
 ********************************************/

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

/* 
 * Please fill in the following student struct 
 */
student_t student = {
  "John Doe",     /* Full name */
  "u0888888@utah.edu",  /* Email address */

};

/***************
 * ROTATE KERNEL
 ***************/

/******************************************************
 * Your different versions of the rotate kernel go here
 ******************************************************/

/* 
 * naive_pinwheel - The naive baseline version of pinwheel 
 */
char naive_pinwheel_descr[] = "naive_pinwheel: Naive baseline implementation";
void naive_pinwheel(int dim, pixel *src, pixel *dest)
{
  int i, j;

  for (i = 0; i < dim/2; i++)
    for (j = 0; j < dim/2; j++)
      dest[RIDX(dim/2 - 1 - j, i, dim)] = src[RIDX(i, j, dim)];
    
  for (i = 0; i < dim/2; i++)
    for (j = 0; j < dim/2; j++)
      dest[RIDX(dim/2 - 1 - j, dim/2 + i, dim)] = src[RIDX(i, dim/2 + j, dim)];

  for (i = 0; i < dim/2; i++)
    for (j = 0; j < dim/2; j++)
      dest[RIDX(dim - 1 - j, i, dim)] = src[RIDX(dim/2 + i, j, dim)];

  for (i = 0; i < dim/2; i++)
    for (j = 0; j < dim/2; j++)
      dest[RIDX(dim - 1 - j, dim/2 + i, dim)] = src[RIDX(dim/2 + i, dim/2 + j, dim)];
}

/* 
 * rotate - Your current working version of pinwheel
 * IMPORTANT: This is the version you will be graded on
 */
char pinwheel_descr[] = "pinwheel: Current working version";
void pinwheel(int dim, pixel *src, pixel *dest)
{
  //naive_pinwheel(dim, src, dest);
	int i, j, ii, jj, iii, jjj;
	//int i, j, ii, jj;
	int semi_dim = dim / 2;

  for (i = 0; i < semi_dim; i+=8)
   	 for (j = 0; j < semi_dim; j+=8)
		for(jj = j; jj < j+8; jj+=8)
			for(ii = i; ii < i+8; ii+=8)
				for(jjj = jj; jjj < jj+8; jjj++)
					for(iii = ii; iii < ii+8; iii++)
      						dest[RIDX(semi_dim - 1 - jjj, iii, dim)] = src[RIDX(iii, jjj, dim)];
  for (i = 0; i < semi_dim; i+=8)
   	 for (j = 0; j < semi_dim; j+=8)
		for(jj = j; jj < j+8; jj+=8)
			for(ii = i; ii < i+8; ii += 8)
				for(jjj = jj; jjj < jj+8; jjj++)
					for(iii = ii; iii < ii+8; iii++)
      						dest[RIDX(semi_dim - 1 - jjj, semi_dim + iii, dim)] = src[RIDX(iii, semi_dim + jjj, dim)];

   for (i = 0; i < semi_dim; i+=8)
   	 for (j = 0; j < semi_dim; j+=8)
		for(jj = j; jj < j+8; jj+= 8)
			for(ii = i; ii < i+8; ii += 8)
				for(jjj = jj; jjj < jj+8; jjj++)
					for(iii = ii; iii < ii+8; iii++)
     					 	dest[RIDX(dim - 1 - jjj, iii, dim)] = src[RIDX(semi_dim + iii, jjj, dim)];

   for (i = 0; i < semi_dim; i+=8)
    	for (j = 0; j < semi_dim; j+=8)
		for(jj = j; jj < j+8; jj+= 8)
			for(ii = i; ii < i+8; ii += 8)
				for(jjj = jj; jjj < jj+8; jjj++)
					for(iii = ii; iii < ii+8; iii++)
      						dest[RIDX(dim - 1 - jjj, semi_dim + iii, dim)] = src[RIDX(semi_dim + iii, semi_dim + jjj, dim)];
}

/*********************************************************************
 * register_pinwheel_functions - Register all of your different versions
 *     of the pinwheel kernel with the driver by calling the
 *     add_pinwheel_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_pinwheel_functions() {
  add_pinwheel_function(&pinwheel, pinwheel_descr);
  add_pinwheel_function(&naive_pinwheel, naive_pinwheel_descr);
}


/***************
 * SMOOTH KERNEL
 **************/

/***************************************************************
 * Various typedefs and helper functions for the smooth function
 * You may modify these any way you like.
 **************************************************************/

/* A struct used to compute averaged pixel value */
typedef struct {
  int red;
  int green;
  int blue;
} pixel_sum;

/* 
 * initialize_pixel_sum - Initializes all fields of sum to 0 
 */
static void initialize_pixel_sum(pixel_sum *sum) 
{
  sum->red = sum->green = sum->blue = 0;
  return;
}

/* 
 * accumulate_sum - Accumulates field values of p in corresponding 
 * fields of sum 
 */
static void accumulate_weighted_sum(pixel_sum *sum, pixel p, double weight) 
{
  sum->red += (int) p.red * weight;
  sum->green += (int) p.green * weight;
  sum->blue += (int) p.blue * weight;
  return;
}

/* 
 * assign_sum_to_pixel - Computes averaged pixel value in current_pixel 
 */
static void assign_sum_to_pixel(pixel *current_pixel, pixel_sum sum) 
{
  current_pixel->red = (unsigned short)sum.red;
  current_pixel->green = (unsigned short)sum.green;
  current_pixel->blue = (unsigned short)sum.blue;
  return;
}

/* 
 * weighted_combo - Returns new pixel value at (i,j) 
 */
static pixel weighted_combo(int dim, int i, int j, pixel *src) 
{
  int ii, jj;
  pixel_sum sum;
  pixel current_pixel;
  double weights[3][3] = { { 0.50, 0.03125, 0.00 },
                           { 0.03125, 0.25, 0.03125 },
                           { 0.00, 0.03125, 0.125 } };

  initialize_pixel_sum(&sum);
  for(ii=0; ii < 3; ii++)
    for(jj=0; jj < 3; jj++) 
      if ((i + ii < dim) && (j + jj < dim))
        accumulate_weighted_sum(&sum,
                                src[RIDX(i+ii,j+jj,dim)],
                                weights[ii][jj]);
    
  assign_sum_to_pixel(&current_pixel, sum);
 
  return current_pixel;
}

/******************************************************
 * Your different versions of the motion kernel go here
 ******************************************************/

/*
 * naive_motion - The naive baseline version of motion 
 */
char naive_motion_descr[] = "naive_motion: Naive baseline implementation";
void naive_motion(int dim, pixel *src, pixel *dst) 
{
  int i, j;
    
  for (i = 0; i < dim; i++)
    for (j = 0; j < dim; j++)
      dst[RIDX(i, j, dim)] = weighted_combo(dim, i, j, src);
}


/*
 * motion - Your current working version of motion. 
 * IMPORTANT: This is the version you will be graded on
 */
char motion_descr[] = "motion: Current working version";
void motion(int dim, pixel *src, pixel *dst) 
{
// 	naive_motion(dim, src, dst);

	/**This does not work since original graph has been changed**/
//	int dim2 = dim + 2;
//	for(i = 0; i < dim; i++){
//		for(j = 0; j < dim; j++){
//			if(i == dim-1){
//				(dst[RIDX(i, j+1, dim)]).red =0;
//				(dst[RIDX(i, j+1, dim)]).blue =0;
//				(dst[RIDX(i, j+1, dim)]).green =0;
//				(dst[RIDX(i, j+2, dim)]).red =0;
//				(dst[RIDX(i, j+2, dim)]).blue =0;
//				(dst[RIDX(i, j+2, dim)]).green =0;
//			}
//			if(j == dim-1){
//				(dst[RIDX(i+1, j, dim)]).red =0;
//				(dst[RIDX(i+1, j, dim)]).blue =0;
//				(dst[RIDX(i+1, j, dim)]).green =0;
//				(dst[RIDX(i+2, j, dim)]).red =0;
//				(dst[RIDX(i+2, j, dim)]).blue =0;
//				(dst[RIDX(i+2, j, dim)]).green =0;
//			}
//			((dst[RIDX(i, j, dim)]).red = (src[RIDX(i, j, dim)]).red>>1) + ((src[RIDX(i+1, j+1, dim)]).red>>2) + ((src[RIDX(i+2, j+2, dim)]).red>>3) + ((src[RIDX(i, j+1, dim)]).red >> 5) + ((src[RIDX(i+1, j, dim)]).red>>5) + ((src[RIDX(i+1, j+2, dim)]).red>>5) + ((src[RIDX(i+2, j+1, dim)]).red>>5);
//			((dst[RIDX(i, j, dim)]).blue = (src[RIDX(i, j, dim)]).blue>>1) + ((src[RIDX(i+1, j+1, dim)]).blue>>2) + ((src[RIDX(i+2, j+2, dim)]).blue>>3) + ((src[RIDX(i, j+1, dim)]).blue >> 5) + ((src[RIDX(i+1, j, dim)]).blue>>5) + ((src[RIDX(i+1, j+2, dim)]).blue>>5)+((src[RIDX(i+2, j+1, dim)]).blue>>5);
//			((dst[RIDX(i, j, dim)]).green = (src[RIDX(i, j, dim)]).green>>1) + ((src[RIDX(i+1, j+1, dim)]).green>>2) + ((src[RIDX(i+2, j+2, dim)]).green>>3) + ((src[RIDX(i, j+1, dim)]).green >> 5) + ((src[RIDX(i+1, j, dim)]).green>>5) + ((src[RIDX(i+1, j+2, dim)]).green>>5) + ((src[RIDX(i+2, j+1, dim)]).green>>5);
//		}			
//	}


	/**Case1: Iterate all pixels except the last two columns and rows**/
	int i, j, ii, jj, iii, jjj;
	int dim_minus_two = dim - 2;
	for(i = 0; i < dim_minus_two; i+=8){
		for(j = 0; j < dim_minus_two; j+=8){
			for(jj = j; jj < j+8; jj+= 8){
				for(ii = i; ii < i+8; ii += 8){
					for(jjj = jj; jjj < jj+8; jjj++){
						for(iii = ii; iii < ii+8; iii++){
							int iiii = iii + 1;
							int jjjj = jjj + 1;
							int iiiii = iiii + 1;
							int jjjjj = jjjj + 1;

							dst[RIDX(iii, jjj, dim)].red = ((src[RIDX(iii, jjj, dim)]).red >> 1) + ((src[RIDX(iiii, jjjj, dim)]).red >> 2) + ((src[RIDX(iiiii, jjjjj, dim)]).red >> 3) + ((src[RIDX(iii, jjjj, dim)]).red >> 5) + ((src[RIDX(iiii, jjj, dim)]).red >> 5) + ((src[RIDX(iiii, jjjjj, dim)]).red >> 5) + ((src[RIDX(iiiii, jjjj, dim)]).red >> 5);

							dst[RIDX(iii, jjj, dim)].green = ((src[RIDX(iii, jjj, dim)]).green >> 1) + ((src[RIDX(iiii, jjjj, dim)]).green >> 2) + ((src[RIDX(iiiii, jjjjj, dim)]).green >> 3) + ((src[RIDX(iii, jjjj, dim)]).green >> 5) + ((src[RIDX(iiii, jjj, dim)]).green >> 5) + ((src[RIDX(iiii, jjjjj, dim)]).green >> 5) + ((src[RIDX(iiiii, jjjj, dim)]).green >> 5);

							dst[RIDX(iii, jjj, dim)].blue = ((src[RIDX(iii, jjj, dim)]).blue >> 1) + ((src[RIDX(iiii, jjjj, dim)]).blue >> 2) + ((src[RIDX(iiiii, jjjjj, dim)]).blue >> 3) + ((src[RIDX(iii, jjjj, dim)]).blue >> 5) + ((src[RIDX(iiii, jjj, dim)]).blue >> 5) + ((src[RIDX(iiii, jjjjj, dim)]).blue >> 5) + ((src[RIDX(iiiii, jjjj, dim)]).blue >> 5);
						}
					}
				}		
			}	
		}	
	}

	/**Case2: The most right bottom pixel does not have any neighbor. Apply the color itself**/
	int dim_minus_one = dim - 1;
	dst[RIDX(dim_minus_one, dim_minus_one, dim)].red = (src[RIDX(dim_minus_one, dim_minus_one, dim)].red >> 1);

	dst[RIDX(dim_minus_one, dim_minus_one, dim)].blue = (src[RIDX(dim_minus_one, dim_minus_one, dim)].blue >> 1);

	dst[RIDX(dim_minus_one, dim_minus_one, dim)].green = (src[RIDX(dim_minus_one, dim_minus_one, dim)].green >> 1);


	/**Case3: The last column except last two pixels all have two neighbors**/
	for(i = 0; i < dim_minus_one; i++){
		int ii = i + 1;
		dst[RIDX(i, dim_minus_one, dim)].red = (src[RIDX(i, dim_minus_one, dim)].red >> 1) + (src[RIDX(ii, dim_minus_one, dim)].red >> 5);

		dst[RIDX(i, dim_minus_one, dim)].blue = (src[RIDX(i, dim_minus_one, dim)].blue >> 1) + (src[RIDX(ii, dim_minus_one, dim)].blue >> 5);

		dst[RIDX(i, dim_minus_one, dim)].green = (src[RIDX(i, dim_minus_one, dim)].green >> 1) + (src[RIDX(ii, dim_minus_one, dim)].green >> 5);		 
	}

	/**Case4: The last row except last two pixels all have two neighbors as well**/
	for(i = 0; i < dim_minus_one; i++){
		int ii = i + 1;
		dst[RIDX(dim_minus_one, i, dim)].red = (src[RIDX(dim_minus_one, i, dim)].red >> 1) + (src[RIDX(dim_minus_one, ii, dim)].red >> 5);

		dst[RIDX(dim_minus_one, i, dim)].blue = (src[RIDX(dim_minus_one, i, dim)].blue >> 1) + (src[RIDX(dim_minus_one, ii, dim)].blue >> 5);	

		dst[RIDX(dim_minus_one, i, dim)].green = (src[RIDX(dim_minus_one, i, dim)].green >> 1) + (src[RIDX(dim_minus_one, ii, dim)].green >> 5);
	}

	/**Case5: The column before the last column all have five neighbors except (dim_minus_two, dim_minus_two)**/
	for(i = 0; i < dim_minus_two; i++){
		int ii = i + 1;
		int iii = ii + 1;
		dst[RIDX(i, dim_minus_two, dim)].red = (src[RIDX(i, dim_minus_two, dim)].red >> 1) + (src[RIDX(ii, dim_minus_two, dim)].red >> 5) + (src[RIDX(i, dim_minus_one, dim)].red >> 5) + (src[RIDX(ii, dim_minus_one, dim)].red >> 2) + (src[RIDX(iii, dim_minus_one, dim)].red >> 5);
		
		dst[RIDX(i, dim_minus_two, dim)].blue = (src[RIDX(i, dim_minus_two, dim)].blue >> 1) + (src[RIDX(ii, dim_minus_two, dim)].blue >> 5) + (src[RIDX(i, dim_minus_one, dim)].blue >> 5) + (src[RIDX(ii, dim_minus_one, dim)].blue >> 2) + (src[RIDX(iii, dim_minus_one, dim)].blue >> 5);
		
		dst[RIDX(i, dim_minus_two, dim)].green = (src[RIDX(i, dim_minus_two, dim)].green >> 1) + (src[RIDX(ii, dim_minus_two, dim)].green >> 5) + (src[RIDX(i, dim_minus_one, dim)].green >> 5) + (src[RIDX(ii, dim_minus_one, dim)].green >> 2) + (src[RIDX(iii, dim_minus_one, dim)].green >> 5);
	}

	/**Case6: The row before the last row all have five neighbors**/
	for(i = 0; i < dim_minus_two; i++){
		int ii = i + 1;
		int iii = ii + 1;
		dst[RIDX(dim_minus_two, i, dim)].red = (src[RIDX(dim_minus_two, i, dim)].red >> 1) + (src[RIDX(dim_minus_one, i, dim)].red >> 5) + (src[RIDX(dim_minus_two, ii, dim)].red >>5) + (src[RIDX(dim_minus_one, ii, dim)].red >> 2) + (src[RIDX(dim_minus_one, iii, dim)].red >> 5);

		dst[RIDX(dim_minus_two, i, dim)].blue = (src[RIDX(dim_minus_two, i, dim)].blue >> 1) + (src[RIDX(dim_minus_one, i, dim)].blue >> 5) + (src[RIDX(dim_minus_two, ii, dim)].blue >>5) + (src[RIDX(dim_minus_one, ii, dim)].blue >> 2) + (src[RIDX(dim_minus_one, iii, dim)].blue >> 5);

		dst[RIDX(dim_minus_two, i, dim)].green = (src[RIDX(dim_minus_two, i, dim)].green >> 1) + (src[RIDX(dim_minus_one, i, dim)].green >> 5) + (src[RIDX(dim_minus_two, ii, dim)].green >>5) + (src[RIDX(dim_minus_one, ii, dim)].green >> 2) + (src[RIDX(dim_minus_one, iii, dim)].green >> 5);
	}
		
	/**Special Case: dst[RIDX(dim_minus_two, dim_minus_two, dim)] has four neighors**/	
	dst[RIDX(dim_minus_two, dim_minus_two, dim)].red = (src[RIDX(dim_minus_two, dim_minus_two, dim)].red >> 1) + (src[RIDX(dim_minus_two, dim_minus_one, dim)].red >> 5) + (src[RIDX(dim_minus_one, dim_minus_two, dim)].red >> 5) + (src[RIDX(dim_minus_one, dim_minus_one, dim)].red >> 2);

	dst[RIDX(dim_minus_two, dim_minus_two, dim)].blue = (src[RIDX(dim_minus_two, dim_minus_two, dim)].blue >> 1) + (src[RIDX(dim_minus_two, dim_minus_one, dim)].blue >> 5) + (src[RIDX(dim_minus_one, dim_minus_two, dim)].blue >> 5) + (src[RIDX(dim_minus_one, dim_minus_one, dim)].blue >> 2);

	dst[RIDX(dim_minus_two, dim_minus_two, dim)].green = (src[RIDX(dim_minus_two, dim_minus_two, dim)].green >> 1) + (src[RIDX(dim_minus_two, dim_minus_one, dim)].green >> 5) + (src[RIDX(dim_minus_one, dim_minus_two, dim)].green >> 5) + (src[RIDX(dim_minus_one, dim_minus_one, dim)].green >> 2);

}

/********************************************************************* 
 * register_motion_functions - Register all of your different versions
 *     of the motion kernel with the driver by calling the
 *     add_motion_function() for each test function.  When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_motion_functions() {
  add_motion_function(&motion, motion_descr);
  add_motion_function(&naive_motion, naive_motion_descr);
}
