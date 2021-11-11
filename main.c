/*
* @file main.c
* @author Renat Kagal <kagal@itspartner.net>
*
* Assembling : gcc -Wall main.c -o main
*
* Description : blowfish algo
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

#include "head.h"

#define SIZE_OF_READ 17
#define CHAR_SIZE 8

#define LONG_FROM_CHAR(buff, offset) (((unsigned long)((unsigned char)buff[(offset)])) | ((unsigned long)((unsigned char)buff[(offset) + 1]) << 8) | ((unsigned long)((unsigned char)buff[(offset) + 2]) << 16) | ((unsigned long)((unsigned char)buff[(offset) + 3]) << 24) | ((unsigned long)((unsigned char)buff[(offset) + 4]) << 32) | ((unsigned long)((unsigned char)buff[(offset) + 5]) << 40) | ((unsigned long)((unsigned char)buff[(offset) + 6]) << 48) | ((unsigned long)((unsigned char)buff[(offset) + 7]) << 56))

struct char_list {
	char str[SIZE_OF_READ];
	struct char_list* next;
};

struct long_list {
	unsigned long xl;
	unsigned long xr;
	struct long_list* next;
};

struct char_list* top_char = NULL;
struct long_list* top_long = NULL;

struct char_list* end_char = NULL;
struct long_list*	end_long = NULL;

int flag_read = 0;
int flag_write = 0;

pthread_mutex_t mtx1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx2 = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;

// #1 2.3 GB - 253 sec

int main () {
	char key[] = "rieupnvsqerjfasdf";
	int keybytes = strlen (key);

	blowfish (key, keybytes);

	printf("%ld\n", clock () / CLOCKS_PER_SEC);
}

void file_read_char () {
	FILE* 				fp;
	char					read_buff[SIZE_OF_READ];

	if ((fp = fopen ("text.txt", "r")) == NULL) {
		puts ("Failed fopen");
		exit (EXIT_FAILURE);
	}
	while (fgets (read_buff, SIZE_OF_READ - 1, fp)) {
		/*if (pthread_mutex_lock (&mtx2)) {
        puts ("Failed mutex lock");
        exit (EXIT_FAILURE);
   	}*/
		create_char_list (read_buff);
	/*
		if (pthread_mutex_unlock (&mtx2)) {
        puts ("Failed mutex lock");
        exit (EXIT_FAILURE);
   	}

   	if (pthread_cond_signal (&cond2)) {
        puts ("Failed cond signal");
        exit (EXIT_FAILURE);
   	}*/
	}

	if (fclose (fp)) {
		puts ("Failed fclose");
		exit (EXIT_FAILURE);
	}
	flag_read = 0;
}

void file_write_long () {
	FILE* 		fp;
	struct long_list* tmp;

	if ((fp = fopen ("encip.txt", "w")) == NULL) {
		puts ("Failed fopen");
		exit (EXIT_FAILURE);
	}
	while (flag_write == 1) {
		/*if (pthread_mutex_lock (&mtx1)) {
			puts ("Failed mutex lock");
			exit (EXIT_FAILURE);
		} 

		while (top_long == NULL) {
			if (pthread_cond_wait (&cond1, &mtx1)) {
                puts ("Failed cond wait");
                exit (EXIT_FAILURE);
            }
		}

		if (pthread_mutex_unlock (&mtx1)) {
			puts ("Failed mutex unlock");
			exit (EXIT_FAILURE);
		}*/
		while (top_long != NULL) {
			fprintf (fp, "%ld %ld ", top_long->xl, top_long->xr);    ////////////////////////////////
			fflush (fp);
			tmp = top_long;
			top_long = top_long->next;
			free (tmp);
		}
	}

	while (top_long != NULL) {
		fprintf (fp, "%ld %ld ", top_long->xl, top_long->xr);
		tmp = top_long;
		top_long = top_long->next;
		free (tmp);
	}

	if (fclose (fp)) {
		puts ("Failed fclose");
		exit (EXIT_FAILURE);
	}
}

void blowfish (char key[], int keybytes) {
	int 						offset = 0;
	char 						read_buff [SIZE_OF_READ];
	unsigned long 			xr;
	unsigned long			xl;
	pthread_t 				thr_read_text;
	pthread_t 				thr_write_text;
	struct char_list* 	tmp_char;

	flag_read = 1;
	flag_write = 1;

	pthread_create (&thr_read_text, NULL, file_read_char, NULL);
	pthread_create (&thr_write_text, NULL, file_write_long, NULL);
	blowfish_init (key, keybytes);

	while (flag_read == 1) {
		/*if (pthread_mutex_lock (&mtx2)) {
			puts ("Failed mutex lock");
			exit (EXIT_FAILURE);	
		}
		while (top_char == NULL) {
			if (pthread_cond_wait (&cond2, &mtx2)) {
                puts ("Failed cond wait");
                exit (EXIT_FAILURE);
            }
		}
		if (pthread_mutex_unlock (&mtx2)) {
				puts ("Failed mutex unlock");
				exit (EXIT_FAILURE);	
		}*/
		while (top_char != NULL) {
			xl = LONG_FROM_CHAR (top_char->str, offset);	
			offset += 8;
			xr = LONG_FROM_CHAR (top_char->str, offset);
		
			tmp_char = top_char;
			top_char = top_char->next;
			free (tmp_char);

			blowfish_encipher (&xl, &xr);

			/*if (pthread_mutex_lock (&mtx1)) {
				puts ("Failed mutex lock");
				exit (EXIT_FAILURE);	
			}*/
			create_long_list (xl, xr);
/*
			if (pthread_mutex_unlock (&mtx1)) {
				puts ("Failed mutex unlock");	
				exit (EXIT_FAILURE);	
			}

			if (pthread_cond_signal (&cond1)) {
  	      	puts ("Failed cond signal");
 	  	     	exit (EXIT_FAILURE);
   		}*/
   		xl = 0;
			xr = 0;
			offset = 0;
   	}
	}	
	/*if (pthread_join (thr_read_text, NULL)) {
      puts ("Failed join ");
      exit (EXIT_FAILURE);
   }*/

	while (top_char != NULL) {
		xl = LONG_FROM_CHAR (top_char->str, offset);	
		offset += 8;
		xr = LONG_FROM_CHAR (top_char->str, offset);
		
		tmp_char = top_char;
		top_char = top_char->next;
		free (tmp_char);

		blowfish_encipher (&xl, &xr);
	
		/*if (pthread_mutex_lock (&mtx1)) {
			puts ("Failed mutex unlock");
			exit (EXIT_FAILURE);	
		}*/
		create_long_list (xl, xr);

		/*if (pthread_mutex_unlock (&mtx1)) {
			puts ("Failed mutex unlock");
			exit (EXIT_FAILURE);	
		}
		if (pthread_cond_signal (&cond1)) {
        puts ("Failed cond signal");
        exit (EXIT_FAILURE);
   	}*/
		xl = 0;
		xr = 0;
		offset = 0;
	}
	flag_write = 0;

	
   if (pthread_join (thr_write_text, NULL)) {
      puts ("Failed join ");
      exit (EXIT_FAILURE);
   }

	FILE* fp_write = fopen ("decip.txt", "w");
	FILE* fp = fopen ("encip.txt", "r");

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

void blowfish_init (char key[], int keybytes) {
	int 					bytes_count;
	unsigned long  	l;
	unsigned long  	r;
	unsigned long  	data;

	bytes_count = 0;

	for (int i = 0; i < N + 2; i++ ) {
		data = 0;
		for (int k = 0; k < 4; k++) {
			data = (data << 8) | key[bytes_count++];

			if (bytes_count >= keybytes) {
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

void create_char_list (char str[]) {
	struct char_list* tmp = (struct char_list*)malloc (sizeof (struct char_list));
	
	memset (tmp->str, '\0', SIZE_OF_READ);
	strcat (tmp->str, str);

	tmp->next = NULL;

	if (top_char == NULL) {
		top_char = tmp;
		end_char = top_char;
	}
	else {
		end_char->next = tmp;
		end_char = end_char->next;
	}
}

void create_long_list (unsigned long xl, unsigned long xr) {
	struct long_list* tmp = (struct long_list*)malloc (sizeof (struct long_list));

	tmp->xl = xl;
	tmp->xr = xr;
	tmp->next = NULL;

	if (top_long == NULL) {
		top_long = tmp;
		end_long = top_long;
	}
	else {
		end_long->next = tmp;
		end_long = end_long->next;
	}
}