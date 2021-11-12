/*
* @file main.c
* @author Renat Kagal <kagal@itspartner.net>
*
* Assembling : gcc -Wall main.c libhuffmancompress.a -o main
*
* Description : Huffman algo
*
* Copyright (c) 2021, ITS Partner LLC.
* All rights reserved.
*
* This software is the confidential and proprietary information of
* ITS Partner LLC. ("Confidential Information"). You shall not
* disclose such Confidential Information and shall use it only in
* accordance with the terms of the license agreement you entered into
* with ITS Partner.
*/
#define N 16
#define SIZE_OF_XL_XR_ARR 1024

struct init_data {
   char* key;
   int keybytes;
};

struct xl_xr {
   unsigned long xl;
   unsigned long xr;
   int size;
};

#include "head.h"

#define SIZE_OF_READ 17
#define CHAR_SIZE 8

#define LONG_FROM_CHAR(buff, offset) (((unsigned long)((unsigned char)buff[(offset)])) | ((unsigned long)((unsigned char)buff[(offset) + 1]) << 8) | ((unsigned long)((unsigned char)buff[(offset) + 2]) << 16) | ((unsigned long)((unsigned char)buff[(offset) + 3]) << 24) | ((unsigned long)((unsigned char)buff[(offset) + 4]) << 32) | ((unsigned long)((unsigned char)buff[(offset) + 5]) << 40) | ((unsigned long)((unsigned char)buff[(offset) + 6]) << 48) | ((unsigned long)((unsigned char)buff[(offset) + 7]) << 56))


struct xl_xr arr_encip[SIZE_OF_XL_XR_ARR];
struct xl_xr arr_write[SIZE_OF_XL_XR_ARR];
struct xl_xr arr_read[SIZE_OF_XL_XR_ARR];

pthread_mutex_t mtx_init = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_encip = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_write = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_read = PTHREAD_MUTEX_INITIALIZER;

int flag_read = 0;
int flag_write = 0;

// #1 2.3 GB - 253 sec

int main () {
   char key[] = "rieupnvsqerjfasdf";
   int keybytes = strlen (key);

   blowfish (key, keybytes);

   printf("%ld\n", clock () / CLOCKS_PER_SEC);
}

void blowfish (char key[], int keybytes) {
   FILE*             fp;
   int               offset = 0;
   int               i;
   char              read_buff [SIZE_OF_READ];
   struct xl_xr*     data;
   unsigned long     xr;
   unsigned long     xl;
   pthread_t         thr_init;
   pthread_t         thr_encip;
   struct init_data  data_key;

   if ((fp = fopen ("text.txt", "r")) == NULL) {
      puts ("Failed fopen");
      exit (EXIT_FAILURE);
   }
   data_key.key = key;
   data_key.keybytes = keybytes;

   if (pthread_create (&thr_init, NULL, blowfish_init, (void*) &data_key)) {
      puts ("Failed create thr");
      exit (EXIT_FAILURE);
   }

   memset (read_buff, '\0', SIZE_OF_READ);

   if (pthread_mutex_lock (&mtx_encip)) {
      puts ("Failed mutex lock");
      exit (EXIT_FAILURE);
   }
   if (pthread_mutex_lock (&mtx_write)) {
      puts ("Failed mutex lock");
      exit (EXIT_FAILURE);
   }
   data = arr_encip;
   offset = 0;
   flag_read = 1;

   if (pthread_create (&thr_encip, NULL, blowfish_encipher_thr, NULL)) {
      puts ("Failed create thr");
      exit (EXIT_FAILURE);
   }

   for (i = 0; fgets (read_buff, SIZE_OF_READ - 1, fp); i++) {
      data[i].xl = LONG_FROM_CHAR (read_buff, offset);  
      offset += 8;
      data[i].xr = LONG_FROM_CHAR (read_buff, offset);
      offset = 0;
      memset (read_buff, '\0', SIZE_OF_READ);

      if (i == SIZE_OF_XL_XR_ARR - 1) {
         i = 0;
         data[0].size = SIZE_OF_XL_XR_ARR;
         change_mutex_and_data (&data);
      }
   }
   data[0].size = i + 1;
   free_mutex_and_data (&data);

   if (pthread_mutex_lock (&mtx_write)) {
      puts ("Failed mutex lock");
      exit (EXIT_FAILURE);
   }
   flag_read = 0;
   if (pthread_mutex_unlock (&mtx_write)) {
      puts ("Failed mutex lock");
      exit (EXIT_FAILURE);
   }

   if (pthread_join (thr_encip, NULL)) {
      puts ("Failed join");
      exit (EXIT_FAILURE);
   }
   if (fclose (fp)) {
      puts ("Failed fclose");
      exit (EXIT_FAILURE);
   }

   if ((fp = fopen ("encip.txt", "r")) == NULL) {
      puts ("Failed fopen");
      exit (EXIT_FAILURE);
   }
   FILE* fp_write;
   if ((fp_write = fopen ("decip.txt", "w")) == NULL) {
      puts ("Failed fopne");
      exit (EXIT_FAILURE);
   }
   while (feof (fp) == 0) {
      if (xl != 0) {
         fprintf (fp_write, "%s", read_buff);
         fflush (fp_write);
         offset = 0;
         memset (read_buff, '\0', SIZE_OF_READ);
         xl = 0;
         xr = 0;
      }
      fscanf (fp, "%ld%ld", &xl, &xr);
      blowfish_decipher (&xl, &xr);
      
      for (int j = 0; j < sizeof (unsigned long); j++, offset++) {
         read_buff[offset] = (0xff & (xl >> (j * CHAR_SIZE)));
      }
      for (int j = 0; j < sizeof (unsigned long); j++, offset++) {
         read_buff[offset] = (0xff & (xr >> (j * CHAR_SIZE)));
      }
   }
   fclose (fp);
   fclose (fp_write);
}

void blowfish_encipher_thr () {
   struct xl_xr* data;
   pthread_t     thr_write;

   if (pthread_mutex_lock (&mtx_encip)) {
      puts ("Failed mutex lock");
      exit (EXIT_FAILURE);
   }
   if (pthread_mutex_lock (&mtx_read)) {
      puts ("Failed mutex lock");
      exit (EXIT_FAILURE);
   }
   data = arr_encip;
   flag_write = 1;

   if (pthread_create (&thr_write, NULL, blowfish_write, NULL)) {
      puts ("Failed create thr");
      exit (EXIT_FAILURE);
   }

   if (pthread_mutex_lock (&mtx_init)) {
      puts ("Failed mutex lock");
      exit (EXIT_FAILURE);
   }
   if (pthread_mutex_unlock (&mtx_write)) {
         puts ("Failed mutex lock");
         exit (EXIT_FAILURE);
   }
   while (flag_read == 1) {
      for (int i = 0; i < data[0].size; i++) {
         blowfish_encipher (&data[i].xl, &data[i].xr);
      }
      /*for (int i = 0; i  < SIZE_OF_XL_XR_ARR; i++) {
         printf ("%ld %ld", data[i].xl, data[i].xr);
      }*/
      change_mutex_and_data (&data);
   }
   free_mutex_and_data (&data);
   if (pthread_mutex_lock (&mtx_read)) {
      puts ("Failed mutex lock");
      exit (EXIT_FAILURE);
   }
   flag_write = 0;
   if (pthread_mutex_unlock (&mtx_read)) {
      puts ("Failed mutex lock");
      exit (EXIT_FAILURE);
   }

   if (pthread_join (thr_write, NULL)) {
      puts ("Failed join");
      exit (EXIT_FAILURE);
   }

   if (pthread_mutex_unlock (&mtx_init)) {
      puts ("Failed mutex unlock");
      exit (EXIT_FAILURE);
   }
}

void blowfish_write () {
   struct xl_xr* data;
   FILE*         fp_write; 

   if ((fp_write = fopen ("encip.txt", "w")) == NULL) {
         puts ("Failed fopne");
         exit (EXIT_FAILURE);
   }
   data = arr_encip;

   if (pthread_mutex_lock (&mtx_encip)) {
      puts ("Failed mutex lock");
      exit (EXIT_FAILURE);
   }
   if (pthread_mutex_unlock (&mtx_read)) {
         puts ("Failed mutex lock");
         exit (EXIT_FAILURE);
   }
   while (flag_write == 1) {
      for (int i = 0; i < data[0].size; i++) {
         fprintf(fp_write, "%ld %ld ", data[i].xl, data[i].xr);
         fflush (fp_write);
      }
      change_mutex_and_data (&data);
   }
   free_mutex_and_data (&data);
}

void blowfish_encipher(unsigned long *xl, unsigned long *xr) {
   unsigned long  Xl;
   unsigned long  Xr;
   unsigned long  temp;
   short          i;

   Xl = *xl;
   Xr = *xr;

   for (i = 0; i < N; ++i) {
      Xl = Xl ^ P[i];
      Xr = f(Xl) ^ Xr;

      temp = Xl;
      Xl = Xr;
      Xr = temp;
   }

   temp = Xl;
   Xl = Xr;
   Xr = temp;

   Xr = Xr ^ P[N];
   Xl = Xl ^ P[N + 1];
  
   *xl = Xl;
   *xr = Xr;
}

void blowfish_decipher(unsigned long *xl, unsigned long *xr) {
   unsigned long  Xl;
   unsigned long  Xr;
   unsigned long  temp;
   short          i;

   Xl = *xl;
   Xr = *xr;

   for (i = N + 1; i > 1; --i) {
      Xl = Xl ^ P[i];
      Xr = f(Xl) ^ Xr;

      temp = Xl;
      Xl = Xr;
      Xr = temp;
   }

   temp = Xl;
   Xl = Xr;
   Xr = temp;

   Xr = Xr ^ P[1];
   Xl = Xl ^ P[0];

   *xl = Xl;
   *xr = Xr;
}

void blowfish_init (struct init_data* data_key) {
   if (pthread_mutex_lock (&mtx_init)) {
      puts ("Failed mutex lock");
      exit (EXIT_FAILURE);
   }

   int               bytes_count;
   unsigned long     l;
   unsigned long     r;
   unsigned long     data;

   bytes_count = 0;
   for (int i = 0; i < N + 2; i++ ) {
      data = 0;
      for (int k = 0; k < 4; k++) {
         data = (data << CHAR_SIZE) | data_key->key[bytes_count++];

         if (bytes_count >= data_key->keybytes) {
            bytes_count = 0;
         }
      }
      P[i] = P[i] ^ data;
   }
   l = 0;
   r = 0;

   for (int i = 0; i < N + 2; i+=2 ) {
      blowfish_encipher (&l, &r);

      l = P[i];
      r = P[i + 1];
   }
   for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 256; j+=2 ) {
         blowfish_encipher(&l, &r);
   
         S[i][j] = l;
         S[i][j + 1] = r;
      }
   }
   if (pthread_mutex_unlock (&mtx_init)) {
      puts ("Failed mutex unlock");
      exit (EXIT_FAILURE);
   }
}

unsigned long f(unsigned long x) {
   unsigned short a;
   unsigned short b;
   unsigned short c;
   unsigned short d;
   unsigned long  y;

   d = x & 0x00FF;
   x >>= 8;
   c = x & 0x00FF;
   x >>= 8;
   b = x & 0x00FF;
   x >>= 8;
   a = x & 0x00FF;
  
   y = S[0][a] + S[1][b];
   y = y ^ S[2][c];
   y = y + S[3][d];

   return y;
}

void change_mutex_and_data (struct xl_xr** data) {
   if (*data == arr_encip) {
         if (pthread_mutex_unlock (&mtx_encip)) {
            puts ("Failed mutex unlock");
            exit (EXIT_FAILURE);
         }
         if (pthread_mutex_lock (&mtx_write)) {
            puts ("Failed mutex lock");
            exit (EXIT_FAILURE);
         }
         *data = arr_write;
      }
      else if (*data == arr_write){
         if (pthread_mutex_unlock (&mtx_write)) {
            puts ("Failed mutex unlock");
            exit (EXIT_FAILURE);
         }
         if (pthread_mutex_lock (&mtx_read)) {
            puts ("Failed mutex");
            exit (EXIT_FAILURE);
         }
         *data = arr_read;
      }
      else {
         if (pthread_mutex_unlock (&mtx_read)) {
            puts ("Failed mutex unlock");
            exit (EXIT_FAILURE);
         }
         if (pthread_mutex_lock (&mtx_encip)) {
            puts ("Failed mutex");
            exit (EXIT_FAILURE);
         }
         *data = arr_encip;
      }
}

void free_mutex_and_data (struct xl_xr** data) {
   if (*data == arr_encip) {
         if (pthread_mutex_unlock (&mtx_encip)) {
            puts ("Failed mutex unlock");
            exit (EXIT_FAILURE);
         }
         *data = NULL;
      }
      else if (*data == arr_write){
         if (pthread_mutex_unlock (&mtx_write)) {
            puts ("Failed mutex unlock");
            exit (EXIT_FAILURE);
         }
         *data = NULL;
      }
      else {
         if (pthread_mutex_unlock (&mtx_read)) {
            puts ("Failed mutex unlock");
            exit (EXIT_FAILURE);
         }
         *data = NULL;
      }
}