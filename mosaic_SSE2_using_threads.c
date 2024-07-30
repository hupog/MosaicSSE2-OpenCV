#include <stdio.h>
#include <stdlib.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <emmintrin.h>
#include <time.h>

#include <pthread.h> /* POSIX Threads */

#define NTHREADS 8

#define ALTO 16
#define ANCHO 16

IplImage* Img1;
IplImage* Img2;
IplImage* ImgR;
void copiarBloque(int filaO, int colO, IplImage* imgO, int filaD, int colD, IplImage* imgD, int alto, int ancho);
int compararBloque(int filaO, int colO, IplImage* imgO, int filaD, int colD, IplImage* imgD, int alto, int ancho);
void mosaico_thread(void *ptr);

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Error: Usage %s image_file_name\n", argv[0]);
        return EXIT_FAILURE;
    }

    Img1 = cvLoadImage(argv[1], CV_LOAD_IMAGE_UNCHANGED);
    Img2 = cvLoadImage(argv[2], CV_LOAD_IMAGE_UNCHANGED);

    if (!Img1 || !Img2) {
        printf("Error: file not found\n");
        return EXIT_FAILURE;
    }
    
    ImgR = cvCreateImage(cvSize(Img1->width, Img1->height), Img1->depth, Img1->nChannels);
    struct timespec start, finish;
    float time;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    pthread_t threads[NTHREADS];
    int filas[NTHREADS];
    int i;
    for (i = 0; i < NTHREADS; i++) {
        filas[i] = i;
        pthread_create(&threads[i], NULL, (void *) &mosaico_thread, (void *) &filas[i]);
    }
    for (i = 0; i < NTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &finish);
    time = finish.tv_sec - start.tv_sec;
    time += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Tiempo %f", time);
    cvShowImage(argv[1], ImgR);
    cvWaitKey(0);
    cvReleaseImage(&ImgR);
    cvDestroyWindow(argv[1]);

    return (EXIT_SUCCESS);
}

void copiarBloque(int filaO, int colO, IplImage* imgO, int filaD, int colD, IplImage* imgD, int alto, int ancho) {
    int i, j;
    for (i = 0; i < alto; i++) {
        __m128i *pImgO = (__m128i *) (imgO->imageData + (filaO + i) * imgO->widthStep + colO * imgO->nChannels);
        __m128i *pImgD = (__m128i *) (imgD->imageData + (filaD + i) * imgD->widthStep + colD * imgD->nChannels);
        for (j = 0; j < ancho * imgO->nChannels; j += 16) {
            *pImgD++ = *pImgO++;
        }
    }
}

int compararBloque(int filaO, int colO, IplImage* imgO, int filaD, int colD, IplImage* imgD, int alto, int ancho) {
    int i, j, r = 0;
    __m128i sumAbsolute;
    int sad;
    for (i = 0; i < alto; i++) {
        __m128i *pImgO = (__m128i *) (imgO->imageData + (filaO + i) * imgO->widthStep + colO * imgO->nChannels);
        __m128i *pImgD = (__m128i *) (imgD->imageData + (filaD + i) * imgD->widthStep + colD * imgD->nChannels);
        for (j = 0; j < ancho * imgO->nChannels; j+=16) {
            sumAbsolute = _mm_sad_epu8(*pImgO++, *pImgD++);
            sad = _mm_cvtsi128_si32(_mm_add_epi32(sumAbsolute, _mm_srli_si128(sumAbsolute, 8)));
            r += sad;
        }
    }
    return r;
}

void mosaico_thread(void *ptr) {
    //ptr apunta a un entero que indica el mutiplo de la fila a sustituir
    int *fila = (int *) ptr;
    // TO DO
    int i, j, row, col, b, cb, rowB, colB;
    for (i = *fila * 16; i < Img1->height; i+=ALTO*NTHREADS) {
        for (j = 0; j < Img1->width; j+=ANCHO) {
            b = INT_MAX;
            for (row = 0; row < Img2->height; row += ALTO) {
                for (col = 0; col < Img2->width; col += ANCHO) {
                    cb = compararBloque(i, j, Img1, row, col, Img2, ALTO, ANCHO);
                    if (cb < b) {
                        b = cb;
                        rowB = row;
                        colB = col;
                    }
                }
            }
            copiarBloque(rowB, colB, Img2, i, j, ImgR, ALTO, ANCHO);
        }
    }
}


