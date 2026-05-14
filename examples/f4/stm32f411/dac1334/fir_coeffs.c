
#include <stdio.h>
#include <math.h>

#define N 256              // número de coeficientes
#define FS 44100        // frequência de amostragem
#define FC 400       // frequência de corte

int main(){
    FILE *f = fopen("fir_coeffs.h", "w");

    fprintf(f, "#ifndef FIR_COEFFS_H\n#define FIR_COEFFS_H\n\n");
    fprintf(f, "#define FIR_TAP_NUM %d\n\n", N);
    fprintf(f, "static const float fir_coeffs[FIR_TAP_NUM] = {\n");
    /* double h[N]; */
    int M = N - 1;
    double wc = 2.0 * M_PI * FC / FS;

    for (int n = 0; n < N; n++) {
        double hd;

        // centro do filtro
        if (n == M / 2) {
            hd = wc / M_PI;
        } else {
            hd = sin(wc * (n - M / 2.0)) / (M_PI * (n - M / 2.0));
        }

        // janela de Hamming
        double w = 0.54 - 0.46 * cos(2.0 * M_PI * n / M);

        // coeficiente final
        double h = hd;

	fprintf(f, "    %.8ff", (float)h);

        if (n != N - 1)
            fprintf(f, ",\n");
    }

    /* // printar coeficientes */
    /* for (int i = 0; i < N; i++) { */
    /*     printf("h[%d] = %f\n", i, h[i]); */
    /* } */

    fprintf(f, "\n};\n\n#endif\n");

    fclose(f);
    
    return 0;
}
