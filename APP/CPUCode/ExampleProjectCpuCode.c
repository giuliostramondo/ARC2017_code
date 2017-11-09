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

#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "prf.h"
#include "utility.h"
#include "testbench_utils.h"

#include "Maxfiles.h"
#include "MaxSLiCInterface.h"

int N;
int M;

//If PRINT_ALL is 1 everything is printed ( Correct and Incorrect tests)
//If PRINT_ALL is 0 only the incorrect tests are printed
#define PRINT_ALL 1
//LOG_PRINTS 0 means that the results of the tests are going to be shown in the console
//LOG_PRINTS 1 means that the results of the tests are going to be printed in the prf_testbench.log file 
#define LOG_PRINTS 1

#define MANUAL_TESTS_ONLY 0
int main(int argc, char* argv[])
{
    int i,j,k,w;
    PRFConfiguration configuration = parseArguments(argc,argv);
    if(configuration.error)
        return 0;
    int p = configuration.p;
    int q = configuration.q;
    N = configuration.N;
    M = configuration.M;
    scheme s = configuration.s;
    int separate_RW = configuration.separate_RW;
    int nb_read_port=configuration.r_ports;
    int error_found=0;
    int log_file="prf_testbench.log";
    FILE *fp_log=NULL;
    
    if(LOG_PRINTS){
        fp_log=fopen(log_file,"w");
    }
    if(!separate_RW){
        nb_read_port=1;
    }
    int **A_test = (int**)malloc(sizeof(int*)*N);

    for(i=0;i<N;i++){
        A_test[i]= (int*)malloc(sizeof(int)*M);
    }
    for(i=0;i<N;i++){
        for(j=0;j<M;j++){
            A_test[i][j]= i*M+j;
        }
    }
    data_type* input_data;
    data_type* output_data;
    struct timeval after,before;
    int counter=0;
	char* scheme_name = schemeStringFromMappingScheme(s);
        printf("Testing %s scheme ...\n",scheme_name);
        printf("Filling PRF...\n");

        input_data = fillPRF( p, q, RECT_ROW,A_test,  &counter, separate_RW,nb_read_port);
    	output_data = (data_type*) malloc(sizeof(data_type)*p*q*counter*nb_read_port);
    	gettimeofday(&before, NULL);
    	ExampleProject( counter, input_data,output_data);
    	gettimeofday(&after, NULL);
    	double time =(double)(after.tv_sec - before.tv_sec) +
    	        		(double)(after.tv_usec - before.tv_usec) / 1e6;
    	printf("Read %d data from memory in %lf seconds\n",p*q*counter,time);
    	printf("The average time to read one element is %lf seconds\n",time/(p*q*counter));
    	printf("Assuming that each element is 64bit the bandwidth is %lf bit/s, %lf byte/s, %lf KByte/s, %lf MByte/s\n",(p*q*counter*64)/time,(p*q*counter*64)/(8*time),(p*q*counter*64)/(1024*8*time),(p*q*counter*64)/(1024*1024*8*time));
        //PRF_Complete( counter, input_data,output_data);
        t_list* available_acc_type = getAvaliableAccessType(s);
        while(available_acc_type!=NULL && ! MANUAL_TESTS_ONLY){
            acc_type* curr_shape = (acc_type*) available_acc_type->data;
            char* shape_name = accessStringFromAccessType(*curr_shape);
            printf("Testing %s shape...\n",shape_name);
            input_data = all_accesses(p,q,*curr_shape,N,M,&counter, separate_RW,nb_read_port);
            for(int row=0;row<3;row++){
            for(int i=0;i<p*q+8;i++)
                printf("%d ",input_data[(row*(p*q+8))+i]);
            printf("\n");
            }
            data_type* expected_output = generate_expected_output(p,q,A_test,input_data,counter, separate_RW,nb_read_port);
            data_type* output_data = (data_type*) malloc(sizeof(data_type)*p*q*counter*nb_read_port);
	    //Send the input_data to the PRF and collect outputs
           // PRF_Complete( counter, input_data,output_data);
        	gettimeofday(&before, NULL);
        	ExampleProject( counter, input_data,output_data);
        	gettimeofday(&after, NULL);
        	double time =(double)(after.tv_sec - before.tv_sec) +
        	        		(double)(after.tv_usec - before.tv_usec) / 1e6;
        	printf("Read %d data from memory in %lf seconds\n",p*q*counter,time);
        	printf("The average time to read one element is %lf seconds\n",time/(p*q*counter));
        	printf("Assuming that each element is 64bit the bandwidth is %lf bit/s, %lf byte/s, %lf KByte/s, %lf MByte/s\n",(p*q*counter*64)/time,(p*q*counter*64)/(8*time),(p*q*counter*64)/(1024*8*time),(p*q*counter*64)/(1024*1024*8*time));

            
            int error = 0;
            //Check equivalence
            for (j=0;j<counter;j++){
            	if (s == ROW_COL && !((input_data[j*((p*q)+4) + (p*q)] % p==0) || (input_data[j*((p*q)+4) + (p*q)+1] % q==0)) ){
                        continue;
                }
                for(k=0;k<p*q*nb_read_port;k++){
                    if(output_data[j*(p*q)*nb_read_port+k] != expected_output[j*(p*q)*nb_read_port+k]){
                        error=1;
                    }
                }
                if(error){
                    //Print Error
                    error_found=1;  
                    debug_print(fp_log,"[Error]\t output %d\n",j);
                    debug_print(fp_log,"Input: ");
                    for(w=0;w<(p*q)+4+4*nb_read_port;w++){
                        debug_print(fp_log,"%d ",input_data[j*((p*q)+4+4*nb_read_port)+w]);
                    }
                    debug_print(fp_log,"\n");
                    debug_print(fp_log,"Expected Output: ");
                    for(w=0;w<p*q*nb_read_port;w++){
                        debug_print(fp_log,"%d ",expected_output[j*(p*q)*nb_read_port+w]);
                    }
                    debug_print(fp_log,"\n");
                    debug_print(fp_log,"Output: ");
                    for(w=0;w<p*q*nb_read_port;w++){
                        debug_print(fp_log,"%d ",output_data[j*(p*q)*nb_read_port+w]);
                    }
                    debug_print(fp_log,"\n");
                    error=0;
                }else{
                    if(PRINT_ALL){
                        debug_print(fp_log,"\033[32m[Correct]\033[0m\t output %d\n",j);
                        debug_print(fp_log,"Input: ");
                        for(w=0;w<(p*q)+4;w++){
                            debug_print(fp_log,"%d ",input_data[j*((p*q)+4)+w]);
                        }
                        debug_print(fp_log,"\n");
                        debug_print(fp_log,"Output: ");
                        for(w=0;w<p*q;w++){
                            debug_print(fp_log,"%d ",output_data[j*(p*q)+w]);
                        }
                        debug_print(fp_log,"\n");
                    }
                }
            }
            available_acc_type = available_acc_type->next;
        }
    if (error_found){
        printf("\033[31m[Fail]\033[0m\t Error where encountered during the testbench execution \n");
    }else{
        printf("\033[32m[Pass]\033[0m\t The testbench ran without errors\n");
    }
    PRFAccess access;
    access.input_data=(data_type*) calloc(p*q,sizeof(data_type));
    access.r_accesses=(BlockAccess*) calloc(configuration.r_ports,sizeof(BlockAccess));
    access.write_en=0;
    access.w_access.start_index.i=0;
    access.w_access.start_index.j=0;
    access.w_access.type=0;
    access.r_accesses[0].start_index.i=0;
    access.r_accesses[0].start_index.j=0;
    access.r_accesses[0].type=RECTANGLE;
    access.r_accesses[1].start_index.i=1;
    access.r_accesses[1].start_index.j=1;
    access.r_accesses[1].type=MAIN_DIAG;



    one_access(configuration,access, 1);
    
    access.write_en=0xFFFFFFFF;
    access.w_access.start_index.i=0;
    access.w_access.start_index.j=0;
    access.w_access.type=0;
    access.r_accesses[0].start_index.i=0;
    access.r_accesses[0].start_index.j=0;
    access.r_accesses[0].type=RECTANGLE;
    access.r_accesses[1].start_index.i=1;
    access.r_accesses[1].start_index.j=1;
    access.r_accesses[1].type=MAIN_DIAG;

    one_access(configuration,access, 1);
    access.write_en=0;
    one_access(configuration,access, 1);

    if(LOG_PRINTS){
        fclose(fp_log);
    }
    return 0;
}


