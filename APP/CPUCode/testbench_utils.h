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

#include<stdio.h>
#include "prf.h"
#include "utility.h"
#include "Maxfiles.h"
#include "MaxSLiCInterface.h"


#define data_type int64_t

typedef struct prf_access{
    data_type *input_data;
    BlockAccess w_access;
    int write_en;
    BlockAccess *r_accesses;
} PRFAccess;

void printAGUOutput(int p, int q, Address2d* elements);

/*
 * Tests if a list of address is within the matrix bound
 * returns 1 if it is whitin bound, 0 otherwise
 */
int checkBoundaries(int p,int q, Address2d* elements);
//Returns the number of parallel access required, with a given shape, to access all the elements of the matrix
int totAccessToScanMatrix(int p, int q, acc_type shape);

data_type* fillPRF(int p, int q, scheme s, int **A_test, int *el_counter, int separate_RW,int r_ports);
data_type* generate_expected_output(int p, int q, int** A_test, data_type* input_data, int size, int separate_RW,int r_ports);
data_type* all_accesses(int p, int q, acc_type shape, int N, int M, int *el_counter,int separate_RW,int r_ports);
data_type* one_access(PRFConfiguration c, PRFAccess access, int print);


