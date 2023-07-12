#include <stdio.h>
#include <stdlib.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#define ALTO 16
#define ANCHO 32

void copiarBloque(int i, int j, IplImage * imagenOrigen, int k, int l, IplImage *imagenDestino){

    int fila, cc;
    
    for (fila = 0; fila < ALTO; fila++) {
        
        __m128i *pL = (__m128i *) (imagenOrigen->imageData + (i + fila) * imagenOrigen->widthStep + j * imagenOrigen->nChannels);
        __m128i *pE = (__m128i *) (imagenDestino->imageData + (k + fila) * imagenDestino->widthStep + l * imagenDestino->nChannels);
        
        
        for (cc = 0; cc < ANCHO * 3; cc += 16) {
            
            *pE++ = *pL++;
        }
    }    
}

int compararBloques(int i, int j, IplImage * imagenOrigen, int k, int l, IplImage *imagenDestino){

    int fila, cc, diferencia = 0;
    __m128i A, B, C, D, E;
    
    for (fila = 0; fila < ALTO; fila++) {
        
        __m128i *pImagenOrigen = (__m128i *) (imagenOrigen->imageData + (i + fila) * imagenOrigen->widthStep + j * imagenOrigen->nChannels);
        __m128i *pImagenDestino = (__m128i *) (imagenDestino->imageData + (k + fila) * imagenDestino->widthStep + l * imagenDestino->nChannels);
        
        for (cc = 0; cc < ANCHO * 3; cc+=16) {
            
            A = _mm_load_si128(pImagenOrigen);
            B = _mm_load_si128(pImagenDestino);
            
            C = _mm_sad_epu8(A, B);
            
            D = _mm_srli_si128(C, 8);
            
            E = _mm_add_epi32(C, D);
            
            diferencia += _mm_cvtsi128_si32(E);
            
            pImagenOrigen++;
            pImagenDestino++;
        }
    }
    
    return diferencia;
}

int main(int argc, char **argv) {

    if (argc != 3) {
        printf("Error: Usage %s image_file_name\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    IplImage* imagenOrigen = cvLoadImage(argv[1], CV_LOAD_IMAGE_UNCHANGED);
    IplImage* imagenDestino = cvLoadImage(argv[2], CV_LOAD_IMAGE_UNCHANGED);

    // Always check if the program can find the image file
    if (!imagenOrigen) {
        printf("Error: file %s not found\n", argv[1]);
        return EXIT_FAILURE;
    }
    
    if (!imagenDestino) {
        printf("Error: file %s not found\n", argv[1]);
        return EXIT_FAILURE;
    }
    
    cvNamedWindow(argv[2],  CV_WINDOW_AUTOSIZE);
    
    int fila1, columna1, fila2, columna2, nuevaDiferencia, mejorFila, mejorColumna;
    int diferenciaMaxima = ANCHO*ALTO*255*3+1;
    int diferencia = diferenciaMaxima;
    
    for (fila1 = 0; fila1 < imagenDestino->height; fila1 += ALTO) {
        
        for (columna1 = 0; columna1 < imagenDestino->width; columna1 += ANCHO) {
            
            for (fila2 = 0; fila2 < imagenOrigen->height; fila2 += ALTO) {
            
                for (columna2 = 0; columna2 < imagenOrigen->width; columna2 += ANCHO) {
                
                    if ((fila1/ALTO)%2 != 0){
                
                        if(columna1 != imagenDestino->width - ANCHO){

                            nuevaDiferencia = compararBloques(fila2, columna2, imagenOrigen, fila1, columna1+(ANCHO/2), imagenDestino);
                        }                                

                    }else{

                        nuevaDiferencia = compararBloques(fila2, columna2, imagenOrigen, fila1, columna1, imagenDestino);
                    }
                    
                    
                    
                    if (nuevaDiferencia < diferencia){
                    
                        diferencia = nuevaDiferencia;
                        mejorFila = fila2;
                        mejorColumna = columna2;
                    }
                }
            }
            
            if ((fila1/ALTO)%2 != 0){
                
                if(columna1 != imagenDestino->width - ANCHO){
                
                    copiarBloque(mejorFila, mejorColumna, imagenOrigen, fila1, columna1+(ANCHO/2), imagenDestino);
                }                                
                
            }else{
            
                copiarBloque(mejorFila, mejorColumna, imagenOrigen, fila1, columna1, imagenDestino);
            }
            
            cvShowImage(argv[2], imagenDestino);
            cvWaitKey(1);
            diferencia = diferenciaMaxima;
        }
    }  

    cvShowImage(argv[2], imagenDestino);
    cvWaitKey(0);

    // memory release for images before exiting the application
    cvReleaseImage(&imagenOrigen);
    cvReleaseImage(&imagenDestino);

    // Self-explanatory
    cvDestroyWindow(argv[1]);

    return EXIT_SUCCESS;
}

