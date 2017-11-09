/*
    Copyright 2016 Giulio Stramondo

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UTILITY_H
#define UTILITY_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "prf.h"

//!  Data structure used to store the user's given arguments.
typedef struct prf_configuration{
	scheme s;/*!< The mapping scheme used by the PRF */
	int p;/*!< Horizontal size of the PRF */
	int q;/*!< Vertical size of the PRF */
	int N;/*!< Horizontal size of the input matrix */
	int M;/*!< Vertical size of the input matrix */
    int separate_RW;/*!< True if the PRF supports separate read and write accesses*/
    int r_ports;/*!< Number of read ports of the PRF*/
	int error;/*!< Variable used when the user arguments generate errors */
} PRFConfiguration;

//! Parses the user arguments.
/*!
	\param argc number of user arguments.
	\param argv list of user's arguments.
	\return Options struct containing the settings defined by the user.
	\sa Options
*/
PRFConfiguration parseArguments(int argc, char** argv);

//! Prints in the console a formatted 2D matrix.
/*!
	\param inputMatrix the 2D array containing the data.
	\param dim1 the horizontal dimension of the given matrix.
	\param dim2 the vertical dimension of the given matrix.
*/
void printMatrix(int **inputMatrix, int dim1, int dim2);

//! Prints in the console a conflict matrix, highlighting the conflicts.
/*!
	\param inputMatrix the 2D array containing the confict's data.
	\param dim1 the horizontal dimension of the given matrix.
	\param dim2 the vertical dimension of the given matrix.
*/
void printConflicts(int **inputMatrix, int dim1, int dim2);

//! Prints in the console a matrix, highlighing certain elements.
/*!
	\param inputMatrix the 2D array containing all the matrix data.
	\param dim1 the horizontal dimension of the given matrix.
	\param dim2 the vertical dimension of the given matrix.
	\param highlightMatrix the 2D array containing the elements of the inputMatrix to highlight.
	\param dimH1 the horizontal dimension of the given highlightMatrix.
	\param dimH2 the vertical dimension of the given highlightMatrix.

*/
void printMatrixHighlight(int **inputMatrix,int dim1,int dim2, int **highlightMatrix, int dimH1, int dimH2);

//! Prints in the console the usage informations.
/*!
	\param programName name of the executable

*/
void printUsage(char *programName);

int debug_print(FILE* fp_log, const char *fmt, ...);


//! Prints the report information of a block read.
/*!
	\param index_i the horizontal index of the top-left element of the block read.
	\param index_j the vertical index of the top-left element of the block read.
	\param type access type defining the block access shape.
	\param data_elements1 the 2D array containing original input matrix.
	\param pR pointer to the PolymorphicRegister used for the block read.
	\param mode select the access mode ( STANDARD or CUSTOM )
*/
void performBlockRead(int index_i, int index_j, acc_type type,int ** data_elements1, PolymorphicRegister* pR, int mode);

char *accessStringFromAccessType(acc_type type);
char *schemeStringFromMappingScheme(scheme s);
int compareAddress(void *a, void *b);
#endif
