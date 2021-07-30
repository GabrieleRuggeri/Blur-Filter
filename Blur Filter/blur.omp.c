#include <stdio.h>
#include <omp.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>



#if ((0x100 & 0xf) == 0x0)
#define I_M_LITTLE_ENDIAN 1
#define swap(mem) (( (mem) & (short int)0xff00) >> 8) +	\
  ( ((mem) & (short int)0x00ff) << 8)
#else
#define I_M_LITTLE_ENDIAN 0
#define swap(mem) (mem)
#endif

#define PI 3.14159
#define XWIDTH 256
#define YWIDTH 256
#define MAXVAL 65535

void write_pgm_image( void *image, int maxval, int xsize, int ysize, const char *image_name)
/*
 * image        : a pointer to the memory region that contains the image
 * maxval       : either 255 or 65536
 * xsize, ysize : x and y dimensions of the image
 * image_name   : the name of the file to be written
 *
 */
{
  FILE* image_file; 
  image_file = fopen(image_name, "w"); 
  
  // Writing header
  // The header's format is as follows, all in ASCII.
  // "whitespace" is either a blank or a TAB or a CF or a LF
  // - The Magic Number (see below the magic numbers)
  // - the image's width
  // - the height
  // - a white space
  // - the image's height
  // - a whitespace
  // - the maximum color value, which must be between 0 and 65535
  //
  // if he maximum color value is in the range [0-255], then
  // a pixel will be expressed by a single byte; if the maximum is
  // larger than 255, then 2 bytes will be needed for each pixel
  //

  int color_depth = 1 + ( maxval > 255 );

  fprintf(image_file, "P5\n# generated by\n# Gabriele Ruggeri\n%d %d\n%d\n", xsize, ysize, maxval);
  
  // Writing file
  fwrite( image, 1, xsize*ysize*color_depth, image_file);  

  fclose(image_file); 
  return ;

  /* ---------------------------------------------------------------

     TYPE    MAGIC NUM     EXTENSION   COLOR RANGE
           ASCII  BINARY

     PBM   P1     P4       .pbm        [0-1]
     PGM   P2     P5       .pgm        [0-255]
     PPM   P3     P6       .ppm        [0-2^16[
  
  ------------------------------------------------------------------ */
}


void read_pgm_image( void **image, int *maxval, int *xsize, int *ysize, const char *image_name)
/*
 * image        : a pointer to the pointer that will contain the image
 * maxval       : a pointer to the int that will store the maximum intensity in the image
 * xsize, ysize : pointers to the x and y sizes
 * image_name   : the name of the file to be read
 *
 */
{
  FILE* image_file; 
  image_file = fopen(image_name, "r"); 

  *image = NULL;
  *xsize = *ysize = *maxval = 0;
  
  char    MagicN[2];
  char   *line = NULL;
  size_t  k, n = 0;
  
  // get the Magic Number
  k = fscanf(image_file, "%2s%*c", MagicN );

  // skip all the comments
  k = getline( &line, &n, image_file);
  while ( (k > 0) && (line[0]=='#') )
    k = getline( &line, &n, image_file);

  if (k > 0)
    {
      k = sscanf(line, "%d%*c%d%*c%d%*c", xsize, ysize, maxval);
      if ( k < 3 )
	fscanf(image_file, "%d%*c", maxval);
    }
  else
    {
      *maxval = -1;         // this is the signal that there was an I/O error
			    // while reading the image header
      free( line );
      return;
    }
  free( line );
  
  int color_depth = 1 + ( *maxval > 255 );
  unsigned int size = *xsize * *ysize * color_depth;
  
  if ( (*image = (char*)malloc( size )) == NULL )
    {
      fclose(image_file);
      *maxval = -2;         // this is the signal that memory was insufficient
      *xsize  = 0;
      *ysize  = 0;
      return;
    }
  
  if ( fread( *image, 1, size, image_file) != size )
    {
      free( image );
      image   = NULL;
      *maxval = -3;         // this is the signal that there was an i/o error
      *xsize  = 0;
      *ysize  = 0;
    }  

  fclose(image_file);
  return;
}

void swap_image( void *image, int xsize, int ysize, int maxval )
/*
 * This routine swaps the endianism of the memory area pointed
 * to by ptr, by blocks of 2 bytes
 *
 */
{
  if ( maxval > 255 )
    {
      // pgm files has the short int written in
      // big endian;
      // here we swap the content of the image from
      // one to another
      //
      unsigned int size = xsize * ysize;
      for ( int i = 0; i < size; i+= 1 )
  	((unsigned short int*)image)[i] = swap(((unsigned short int*)image)[i]);
    }
  return;
}


void * generate_gradient( int maxval, int xsize, int ysize )
/*
 * just and example about how to generate a vertical gradient
 * maxval is either 255 or 65536, xsize and ysize are the
 * x and y dimensions of the image to be generated.
 * The memory region that will contain the image is returned
 * by the function as a void *

 */
{
  char      *cImage;   // the image when a single byte is used for each pixel
  short int *sImage;   // the image when a two bytes are used for each pixel
  void      *ptr;
  
  int minval      = 0; 
  int delta       = (maxval - minval) / ysize;
  
  if(delta < 1 )
    delta = 1;
  
  if( maxval < 256 )
    // generate a gradient with 1 byte of color depth
    {
      cImage = (char*)calloc( xsize*ysize, sizeof(char) );
      unsigned char _maxval = (char)maxval;
      int idx = 0;
      for ( int yy = 0; yy < ysize; yy++ )
	{
	  unsigned char value = minval + yy*delta;
	  for( int xx = 0; xx < xsize; xx++ )
	    cImage[idx++] = (value > _maxval)?_maxval:value;
	}
      ptr = (void*)cImage;
    }
  else
    // generate a gradient with 2 bytes of color depth
    {
      sImage = (unsigned short int*)calloc( xsize*ysize, sizeof(short int) );
      unsigned short int _maxval = swap((unsigned short int)maxval);
      int idx = 0;
      for ( int yy = 0; yy < ysize; yy++ )
	{
	  unsigned short int value  = (short int) (minval+ yy*delta);
	  unsigned short int _value = swap( value );    // swap high and low bytes, the format expect big-endianism
	  
	  for( int xx = 0; xx < xsize; xx++ )
	    sImage[idx++] = (value > maxval)?_maxval:_value;
	}
      ptr = (void*)sImage;	
    }

  return ptr;
}


unsigned short int apply_kernel(const unsigned short int* A, const int R, const int C, const int i, const int j,const float* K, const int k_size, int margin){

    unsigned short int blurred = 0;
    float cont = 0;
    int row_k;
    int col_k;

    //#pragma omp parallel for collapse(2) reduction(+:cont) schedule(static)
    for(int row = i - margin; row < i + margin + 1; row++){
        for(int col = j - margin; col < j + margin + 1; col++){
            row_k = row - i + margin;
            col_k = col - j + margin;
            //#pragma omp atomic update
            cont += ((float)A[row*C+col]) * K[row_k*k_size+col_k];
        }
    }
    blurred = (unsigned short int)cont;

    return blurred;
}

float* define_kernel(float* k, const int k_size, const int k_type, float f){
    float h = 0;
    if(k_type==0){
        h = 1./(k_size*k_size);
        for(int i = 0; i < k_size; i++){
            for(int j = 0; j < k_size; j++){
                k[i*k_size+j] = h;
            }
        }
    }
    if(k_type==1){
        h = 1./(k_size*k_size-1);
        int middle = (k_size-1)/2;
        for(int i = 0; i < k_size; i++){
            for(int j = 0; j < k_size; j++){
                if(i==j && i == middle){
                    k[i*k_size+j] = f;
                }
                else{
                    k[i*k_size+j] = (1-f)*h;
                }
            }
        }
    }
    if(k_type==2){
        float mu = (k_size-1)/2;
        float sigma = 10;
        h = 1./(2*PI*sigma*sigma);
        for(int i = 0; i < k_size; i++){
            for(int j = 0; j < k_size; j++){
                k[i*k_size+j] = h * exp( - ((i-mu)*(i-mu)+(j-mu)*(j-mu)) / (2*sigma*sigma) );
            }
        }
    }

    return k;
}

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

char* make_name1(const char *s1,const char *s2,const char *s3,const char *s4,const char *s5,const char *s6,const char *s7,const char *s8,const char *s9,const char *s10)
{
  char* res = concat(s1,s2);
  res = concat(res,s3);
  res = concat(res,s4);
  res = concat(res,s5);
  res = concat(res,s6);
  res = concat(res,s7);
  res = concat(res,s8);
  res = concat(res,s9);
  res = concat(res,s10);
  return res;
}

char* make_name(const char *s1,const char *s2,const char *s3,const char *s4,const char *s5,const char *s6,const char *s7,const char *s8)
{
  char* res = concat(s1,s2);
  res = concat(res,s3);
  res = concat(res,s4);
  res = concat(res,s5);
  res = concat(res,s6);
  res = concat(res,s7);
  res = concat(res,s8);
  return res;
}



//-MAIN-########################################################################################################################

int main(int argc, char** argv)
{
    if(argc < 2){
        printf("ERROR:\nUsage:height, width, kernel_type, kernel_size, kernel_parameter(optional), input_file_name, output_file_name(optional)");
        return 0;
    }
    //variables to store cmd line args
    int k_type,k_size;
    //optional kernel param
    float f=1;
    
    //img params
    int xsize      = XWIDTH;
    int ysize      = YWIDTH;
    int maxval     = MAXVAL;
    //naming params
    char* input_name;
    char* output_name;
    char name[50];
    //kernel params
    k_type = atoi(argv[1]);
    k_size = atoi(argv[2]);
    double start, finish;

    
    //store comm line args
    if(argc == 4){
      input_name = argv[3];
      for(int i = 0; i < strlen(input_name)-4;i++){
        name[i] = input_name[i];
      }
      name[strlen(input_name)-4]='\0';
      strcat(name,".b_");
      output_name = (char*)make_name(name,argv[1],"_",argv[2],"x",argv[2],".omp",".pgm");
    }
    if(argc == 5 & k_type == 1){
      f = atof(argv[3]);
      input_name = argv[4];
      for(int i = 0; i < strlen(input_name)-4;i++){
        name[i] = input_name[i];
      }
      name[strlen(input_name)-4]='\0';
      strcat(name,".b_");
      char fn[10];
      fn[0]=argv[3][2];
      fn[1]='\0';
      output_name = (char*)make_name1(name,argv[1],"_",argv[2],"x",argv[2],"_0",fn,".omp",".pgm");
    }
    if(argc == 5 & k_type != 1){
      input_name = argv[3];
      output_name = argv[4];
    }
    if(argc == 6){
      f = atof(argv[3]);
      input_name = argv[4];
      output_name = argv[5];
    }


    

    start = omp_get_wtime();
    //void ptr used for img
    void *ptr = generate_gradient( maxval, xsize, ysize );
    write_pgm_image( ptr, maxval, xsize, ysize, "image.pgm" );
    free(ptr);

    xsize = 0;
    ysize = 0;
    maxval = 0;

    read_pgm_image( &ptr, &maxval, &xsize, &ysize, "image.pgm");
    free( ptr );
    read_pgm_image(&ptr, &maxval, &xsize, &ysize, (char*)input_name);


    int margin = (k_size - 1)/2;                      //margin of the noul frame 
    int size_ii = (xsize+2*margin)*(ysize+2*margin);  //size of the bigger matrix

    
    

    
    //the input image will be given a noul frame to make easier the kernel computations
    unsigned short int *big_image_i = (unsigned short int*)calloc(size_ii,sizeof(unsigned short int));
    //here we store the blurred image
    short int *image_o = (unsigned short int*)malloc(((xsize)*(ysize))*sizeof(unsigned short int));
    //kernel
    float *kernel = (float*)malloc((k_size*k_size)*sizeof(float));
    
    
    
    //the master touches and initialize the kernel 
    kernel = define_kernel(kernel, k_size, k_type, f);
    // swap the endianism
    if ( I_M_LITTLE_ENDIAN ){
      swap_image( ptr, xsize, ysize, maxval);
    }
    //threads read the image
    #pragma omp parallel 
    {
      #pragma omp for collapse(2) schedule(static)
      for(int i = 0; i < ysize; i++){
        for(int j = 0; j < xsize; j++){
          big_image_i[(i+margin)*(xsize+2*margin)+j+margin] = ((unsigned short int*)ptr)[i*xsize+j];    
        }
      }
    }
    //the threads blur the image
    #pragma omp parallel 
    {
      #pragma omp for collapse(2) schedule(dynamic) 
      for(int i = 0; i < ysize; i++){
        for(int j = 0; j < xsize; j++){
          image_o[i*xsize+j] = apply_kernel(big_image_i, ysize+2*margin, xsize+2*margin, i+margin, j+margin, kernel, k_size, margin);
        }
      }
    }
    finish = omp_get_wtime();
    printf("\nexecution time = %f\n", finish-start);

    // swap the endianism
    if ( I_M_LITTLE_ENDIAN ){
        swap_image( image_o, xsize, ysize, maxval); 
    }
            
        
    write_pgm_image( image_o, maxval, xsize, ysize, (char*)output_name);
    //free the memory
    free(ptr);
    free(big_image_i);
    free(image_o);
    free(kernel);

    return 0;
}
