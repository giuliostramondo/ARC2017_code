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
#include "testbench_utils.h"

extern int N;
extern int M;

void printAGUOutput(int p, int q, Address2d* elements){
    int i;

    for(i=0; i< p*q; i++){
        printf("(%d,%d) ",elements[i].i,elements[i].j);
    }
    printf("\n");
    return;
}

/*
 * Tests if a list of address is within the matrix bound
 * returns 1 if it is whitin bound, 0 otherwise
 */
int checkBoundaries(int p,int q, Address2d* elements){
    int success=1;
    int i;
    for(i=0;i<p*q;i++){
        if (elements[i].i>=N || elements[i].j >=M || elements[i].i<0 || elements[i].j<0){
            success=0;
            return success;
        }
    }
    return success;
}
//Returns the number of parallel access required, with a given shape, to access all the elements of the matrix
int totAccessToScanMatrix(int p, int q, acc_type shape){
    int tot_accesses = 0;
    switch(shape){
        case RECTANGLE:
            //Access required to write in the PRF using RECTANGLE shape
            tot_accesses = (N/p)*(M/q);
            //if N%p!=0 need to add the right edge
            if( N%p != 0){
                tot_accesses+=M/q;

            } 
            //if M%q!=0 need to add the bottom edge
            if (M%q!=0){
                tot_accesses+=N/p;
            }
            //if N%p!=0 and M%q!=0 add bottom right elements
            if( N%p !=0 && M%q !=0){
                tot_accesses+=1;
            }
            break;

        case ROW:
            //Access required to write in the PRF using ROW shape
            tot_accesses = N*(M/(p*q));
            //if M%(p*q) need to add the right edge
            if( M%(p*q) != 0){
                tot_accesses+=N;
            } 
            break;
        case COLUMN:
            //Access required to write in the PRF using COLUMN shape
            tot_accesses = (N/(p*q))*M;
            //if N%(p*q)!=0 need to add the bottom edge
            if( N%(p*q) != 0){
                tot_accesses+=M;
            } 
            break;
        case MAIN_DIAG:
            return totAccessToScanMatrix(p*q,p*q,RECTANGLE);
        case SECONDARY_DIAG:
            return totAccessToScanMatrix(p*q,p*q,RECTANGLE);
        case TRANSP_RECTANGLE:
           return totAccessToScanMatrix(q,p,RECTANGLE);
        default:
            return 0;
    }       
    return tot_accesses;
}

int findInputStreamPadding(int rows, int p, int q, int ADD_INPUT_NB, int data_type){

    int padding =0;
    while ((((p*q)+7)*rows+padding*8)%16!=0)
        padding++;
    return padding;
}

data_type* one_access(PRFConfiguration c, PRFAccess access, int print){
    int ADD_INPUT_NB=0,i,j;
    if(c.separate_RW){
        ADD_INPUT_NB=4+4*c.r_ports;
    }else{
        ADD_INPUT_NB=4;
    }
    int input_data_size= 4; // Minimum input data size

    data_type *input_data =(data_type*) malloc(((sizeof(data_type)*((c.p*c.q)+ADD_INPUT_NB))*input_data_size));
    
    //Copying input vector
    for ( i=0;i<c.p*c.q;i++){
        input_data[i]=access.input_data[i];
    }
    input_data[i++]=access.w_access.start_index.i;
    input_data[i++]=access.w_access.start_index.j;
    input_data[i++]=access.w_access.type;
    input_data[i++]=access.write_en;

    for ( j=0;j<c.r_ports;j++){
        input_data[i]=access.r_accesses[j].start_index.i;
        i++;
        input_data[i]=access.r_accesses[j].start_index.j;
        i++;
        input_data[i]=access.r_accesses[j].type;
        i++;
        i++;
    }    
   if(print){ 
        printf("Input:\n");
        printf("| p*q input_data | w_i w_j w_s w_e | r_i_p0 r_j_p0 r_s_p0 - |...\n");
        for ( i=0;i<c.p*c.q+4+4*c.r_ports;i++){
            printf("%d ",input_data[i]);
        }
        printf("\n");
   }
    data_type* output_data = (data_type*) malloc(sizeof(data_type)*c.p*c.q*input_data_size*c.r_ports);
    ExampleProject( input_data_size , input_data,output_data);

    if(print){
        printf("Output:\n");
        for ( i=0;i<c.p*c.q*c.r_ports;i++){
            printf("%d ",output_data[i]);
        }
        printf("\n");
    }
    return output_data;
}
data_type* all_accesses(int p, int q, acc_type shape, int N, int M, int *el_counter, int separate_RW,int r_ports){
   // input_data contains p*q integers of input_data;
   // i,j coordinates of the block access
   // int of access type ( RECTANGLE ->0 , ROW -> 1, COL->2, MAIN_DIAG -> 3, SECONDARY_DIAG ->4, TRANS_RECTANGLE -> 5
   // write enable signal 1 or 0
   // repeated size time (min 4  probably )
	data_type *input_data;
    int i,j,tot_accesses,k,counter=0;

    volatile int input_data_size=0;

    int ADD_INPUT_NB=0;
    for(i=0;i<N;i++){
        for(j=0;j<M;j++){

            Address2d* elements_to_access = AGU(i,j,p,q,shape);
            if(checkBoundaries(p,q,elements_to_access)){
                        input_data_size++;
            }
        }
    }
    if(separate_RW){
        ADD_INPUT_NB=4+4*r_ports;
    }else{
        ADD_INPUT_NB=4;
    }
    /*
     * The size of every stream going to and coming from the maxelerboard has to be a multiple of 16 bytes. 
     * Hence there is the need to do padding.
     * Malloc takes a value that represents the size of data in bits.
     */
    //The division by 8 convets the number of bits in number of bytes, the modulo 16 computes the reminder of the division
    //this reminder is the patting in bytes. It gets converted in bits moltiplying again by 8.
    //
    //volatile int padding = ((((sizeof(data_type)*((p*q)+ADD_INPUT_NB))*input_data_size)/8)%16)*8;
    input_data =(data_type*) malloc(((sizeof(data_type)*((p*q)+ADD_INPUT_NB))*input_data_size));
    for(i=0;i<N;i++){
        for(j=0;j<M;j++){
            Address2d* elements_to_access = AGU(i,j,p,q,shape);
            if(checkBoundaries(p,q,elements_to_access)){
              //      printf("Element (%d,%d):",i,j);
                    //Fill the data fields to 0, this part will be ignored
                    //by the PRF because a read is being performed
            //            printf(" | ");
                    for(k=0;k<p*q;k++){
                        input_data[counter*(p*q+ADD_INPUT_NB)+k]=0;
                //        printf("0 ");
                    }
                  //      printf(" | ");

                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q] = i;
                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q+1] = j;
                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q+2] = shape;
                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q+3] = 0; // read 
                    //printf("%d %d %d %d ",i,j,shape,0);
                      //  printf(" | ");
                    if(separate_RW){
                        for(int port =0;port<r_ports;port++){ 
                            Address2d* elements_to_access_1 = AGU(i,j+port,p,q,shape);
                            if(checkBoundaries(p,q,elements_to_access_1)){

                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+4+4*port] = i;
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+5+4*port] = j+port;//Access different items in each read
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+6+4*port] = shape;
                        //    printf(" %d %d %d ",i,j,shape);
                          //  printf(" | ");
                            }else{
                                input_data[counter*(p*q+ADD_INPUT_NB)+p*q+4+4*port] = i;
                                input_data[counter*(p*q+ADD_INPUT_NB)+p*q+5+4*port] = j;//Access same item to not go outside bounds
                                input_data[counter*(p*q+ADD_INPUT_NB)+p*q+6+4*port] = shape;
                            }

                        }
                    }
                    counter++;
            }
       // printf("\n");
        }
    }
    *el_counter = counter;
    return input_data;
}

//Inputs
//A_test contains the matrix used to initialize the PRF
//p,q,scheme refer to the parameters the PRF was configured with
//expected_output is a 

//data_type* generate_test_for_decoupled_read_write(int p, int q, scheme s, int **A_test, data_type** expected_output ){
//    int i,j;
//    for (int i=0; i<N*M ; i++){
//        for(int j=0; j<N*M ;j++){
//            for (int si=0;si<scheme_nb;si++)
//                for (int sj=0;sj<scheme_nb;sj++){
                    
                
//                }
//        }
//    }
//}

data_type* fillPRF(int p, int q, scheme s, int **A_test, int *el_counter, int separate_RW,int r_ports){
   // input_data contains p*q integers of input_data;
   // i,j coordinates of the block access
   // int of access type ( RECTANGLE ->0 , ROW -> 1, COL->2, MAIN_DIAG -> 3, SECONDARY_DIAG ->4, TRANS_RECTANGLE -> 5
   // write enable signal 1 or 0
   // repeated size time (min 4  probably )
	data_type *input_data;
	data_type i,j;
    int tot_accesses,k,counter=0;
    int ADD_INPUT_NB=0;
    switch(s){
        case RECT_ROW:
            tot_accesses = totAccessToScanMatrix(p,q,RECTANGLE);
            if(separate_RW)
                ADD_INPUT_NB=4+4*r_ports;
            else
                ADD_INPUT_NB=4;

            input_data = (data_type*) malloc((sizeof(data_type)*p*q+ADD_INPUT_NB*sizeof(data_type))*tot_accesses);

            for(i=0;i<N;i+=p){
                for(j=0;j<M;j+=q){
                    Address2d *elements_to_write = AGU(i,j,p,q,RECTANGLE);
                    for(k=0;k<p*q;k++){
                        input_data[counter*(p*q+ADD_INPUT_NB)+k]=A_test[elements_to_write[k].i][elements_to_write[k].j];
                    }
                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q] = i;
                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q+1] = j;
                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q+2] = RECTANGLE;
                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q+3] = 0xFFFFFFFF; // Write
                    if(separate_RW){
                        for(int port =0;port<r_ports;port++){ 
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+4+4*port] = i;
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+5+4*port] = j;
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+6+4*port] = RECTANGLE;
                        }
                    }
                    counter++;
                }
            }
            //Write the right edge
            if ( N%p != 0 ){
                j = M-q;
                for(i=0; i<N ; i+=p ){
                    Address2d *elements_to_write = AGU(i,j,p,q,RECTANGLE);
                    for(k=0;k<p*q;k++){
                        input_data[counter*(p*q+ADD_INPUT_NB)+k]=A_test[elements_to_write[k].i][elements_to_write[k].j];
                    }
                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q] = i;
                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q+1] = j;
                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q+2] = RECTANGLE;
                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q+3] = 1; // write 
                    if(separate_RW){
                        for(int port =0;port<r_ports;port++){ 
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+4+4*port] = i;
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+5+4*port] = j;
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+6+4*port] = RECTANGLE;
                        }
                    }
                    counter++;
                }
            }
            //Write the bottom edge
            if ( M%q != 0 ){
                i = N-p;
                for(j=0; j<M ; j+=q){
                    Address2d *elements_to_write = AGU(i,j,p,q,RECTANGLE);
                    for(k=0;k<p*q;k++){
                        input_data[counter*(p*q+ADD_INPUT_NB)+k]=A_test[elements_to_write[k].i][elements_to_write[k].j];
                    }
                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q] = i;
                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q+1] = j;
                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q+2] = RECTANGLE;
                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q+3] = 1; // write 
                    if(separate_RW){
                        for(int port =0;port<r_ports;port++){ 
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+4+4*port] = i;
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+5+4*port] = j;
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+6+4*port] = RECTANGLE;
                        }
                    }
                    counter++; 
                }
            }
            //Write bottom right edge
            if(N%p != 0 && M%q!=0){
                i= N-p;
                j= M-q;
                Address2d *elements_to_write = AGU(i,j,p,q,RECTANGLE);
                for(k=0;k<p*q;k++){
                    input_data[counter*(p*q+ADD_INPUT_NB)+k]=A_test[elements_to_write[k].i][elements_to_write[k].j];
                }
                input_data[counter*(p*q+ADD_INPUT_NB)+p*q] = i;
                input_data[counter*(p*q+ADD_INPUT_NB)+p*q+1] = j;
                input_data[counter*(p*q+ADD_INPUT_NB)+p*q+2] = RECTANGLE;
                input_data[counter*(p*q+ADD_INPUT_NB)+p*q+3] = 1; // write 
                if(separate_RW){
                        for(int port =0;port<r_ports;port++){ 
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+4+4*port] = i;
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+5+4*port] = j;
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+6+4*port] = RECTANGLE;
                        }

                }
                counter++;
            
            }
            break;

        case RECT_COL:
                    input_data = fillPRF(p, q, RECT_ROW, A_test, el_counter, separate_RW,r_ports);
                    break;

        case ROW_COL:
                    tot_accesses = totAccessToScanMatrix(p,q,ROW);
                    input_data = (data_type*) malloc((sizeof(data_type)*p*q+4*sizeof(data_type))*tot_accesses);
                    for(i=0; i<N ; i++){
                        for( j=0; j<M;j+=p*q){
                            Address2d *elements_to_write = AGU(i,j,p,q,ROW);
                            for(k=0;k<p*q;k++){
                                input_data[counter*(p*q+ADD_INPUT_NB)+k]=A_test[elements_to_write[k].i][elements_to_write[k].j];
                            }
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q] = (data_type)i;
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+1] = (data_type)j;
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+2] = ROW;
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+3] = 1; // write 
                            if(separate_RW){
                                for(int port =0;port<r_ports;port++){ 
                                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q+4+4*port] = i;
                                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q+5+4*port] = j;
                                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q+6+4*port] = RECTANGLE;
                                }
                            }
                            counter++;
                        }
                    }
                    //Write right edge
                    if(j%(p*q) != 0 ){
                        j=M-(p*q);
                        for(i=0;i<N;i++){
                            Address2d *elements_to_write = AGU(i,j,p,q,ROW);
                            for(k=0;k<p*q;k++){
                                input_data[counter*(p*q+ADD_INPUT_NB)+k]=A_test[elements_to_write[k].i][elements_to_write[k].j];
                            }
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q] = (data_type)i;
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+1] = (data_type)j;
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+2] = ROW;
                            input_data[counter*(p*q+ADD_INPUT_NB)+p*q+3] = 1; // write 
                            if(separate_RW){
                                for(int port =0;port<r_ports;port++){ 
                                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q+4+4*port] = i;
                                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q+5+4*port] = j;
                                    input_data[counter*(p*q+ADD_INPUT_NB)+p*q+6+4*port] = RECTANGLE;
                                }
                            }
                            counter++;

                        }
                    }
                    break;

        case RECT_TRECT:
                    input_data = fillPRF(p,q,RECT_ROW, A_test, el_counter,separate_RW,r_ports);
                    break;
    }
    *el_counter=counter;
    return input_data;
}

data_type* generate_expected_output(int p, int q, int** A_test, data_type* input_data, int size, int separate_RW,int r_ports){
    volatile int i,j;
    data_type* expected_output = (data_type*) malloc(sizeof(data_type)*p*q*size*r_ports);
    int ADD_INPUT_NB;
    if(separate_RW)
        ADD_INPUT_NB=4+4*r_ports;
    else{
        ADD_INPUT_NB=4;
        //Overwrite read ports if RW addresses are not separated
        r_ports=1;
    }
    for(i=0;i<size;i++){
        for(int port=0;port<r_ports;port++){
            int i_coordinate = input_data[i*(p*q+ADD_INPUT_NB)+p*q+4+4*port];
            int j_coordinate = input_data[i*(p*q+ADD_INPUT_NB)+p*q+5+4*port];
                            //input_data[counter*(p*q+ADD_INPUT_NB)+p*q+5+4*port] = j+port;//Access different items in each read
            acc_type shape = input_data[i*(p*q+ADD_INPUT_NB)+p*q+6+4*port];
            //printf("Port %d: i_coordinate=%d, j_coordinate=%d, shape=%d\n",port,i_coordinate,j_coordinate,shape);
            Address2d *elements_to_access = AGU(i_coordinate,j_coordinate,p,q,shape);
            for (j=0;j<p*q;j++){
                Address2d current = elements_to_access[j];
                int element = A_test[current.i][current.j];
                expected_output[i*(p*q)*r_ports+j+p*q*port]=element;
            }
        }
    }
    return expected_output;
}
