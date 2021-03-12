#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

char *in_filename_julia;
char *in_filename_mandelbrot;
char *out_filename_julia;
char *out_filename_mandelbrot;

// structura pentru un numar complex
typedef struct _complex {
	double a;
	double b;
} complex;

// structura pentru parametrii unei rulari
typedef struct _params {
	int is_julia, iterations;
	double x_min, x_max, y_min, y_max, resolution;
	complex c_julia;
} params;

int P; // numarul de threaduri
params parJulia, parMandelbrot;
int widthMandelbrot, heightMandelbrot;
int widthJulia, heightJulia;
int **resultJulia, **resultMandelbrot; // matricile de rezultat
pthread_barrier_t barrier1, barrier2, barrier3; // barierele pt. paralelizare


// citeste argumentele programului
void get_args(int argc, char **argv)
{
	if (argc < 6) {
		printf("Numar insuficient de parametri:\n\t"
				"./tema1 fisier_intrare_julia fisier_iesire_julia "
				"fisier_intrare_mandelbrot fisier_iesire_mandelbrot\n");
		exit(1);
	}

	in_filename_julia = argv[1];
	out_filename_julia = argv[2];
	in_filename_mandelbrot = argv[3];
	out_filename_mandelbrot = argv[4];
	P = atoi(argv[5]);
}

// citeste fisierul de intrare
void read_input_file(char *in_filename, params* par)
{
	FILE *file = fopen(in_filename, "r");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de intrare!\n");
		exit(1);
	}

	fscanf(file, "%d", &par->is_julia);
	fscanf(file, "%lf %lf %lf %lf",
			&par->x_min, &par->x_max, &par->y_min, &par->y_max);
	fscanf(file, "%lf", &par->resolution);
	fscanf(file, "%d", &par->iterations);

	if (par->is_julia) {
		fscanf(file, "%lf %lf", &par->c_julia.a, &par->c_julia.b);
	}

	fclose(file);
}

// scrie rezultatul in fisierul de iesire
void write_output_file(char *out_filename, int **result, int width, int height)
{
	int i, j;

	FILE *file = fopen(out_filename, "w");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de iesire!\n");
		return;
	}

	fprintf(file, "P2\n%d %d\n255\n", width, height);
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			fprintf(file, "%d ", result[i][j]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
}

// aloca memorie pentru rezultat
int **allocate_memory(int width, int height)
{
	int **result;
	int i;

	result = malloc(height * sizeof(int*));
	if (result == NULL) {
		printf("Eroare la malloc!\n");
		exit(1);
	}

	for (i = 0; i < height; i++) {
		result[i] = malloc(width * sizeof(int));
		if (result[i] == NULL) {
			printf("Eroare la malloc!\n");
			exit(1);
		}
	}

	return result;
}

// elibereaza memoria alocata
void free_memory(int **result, int height)
{
	int i;

	for (i = 0; i < height; i++) {
		free(result[i]);
	}
	free(result);
}
// minimul dintre 2 intregi
int min (int a, int b) {
	if (a < b) {
		return a;
	}
	return b;
}

// functie pentru calculul paralel al multimii Julia
void Julia(int thread_id) {
	int w, h, i;
	int start = thread_id * (double) heightJulia / P;
	int end = min((thread_id + 1) * (double) heightJulia / P, heightJulia);
	for (w = 0; w < widthJulia; w++) {
		for (h = start; h < end; h++) {
			int step = 0;
			complex z = { .a = w * parJulia.resolution + parJulia.x_min,
							.b = h * parJulia.resolution + parJulia.y_min };

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < parJulia.iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = pow(z_aux.a, 2) - pow(z_aux.b, 2) + parJulia.c_julia.a;
				z.b = 2 * z_aux.a * z_aux.b + parJulia.c_julia.b;

				step++;
			}

			resultJulia[h][w] = step % 256;

		}

	}
	// se asteapta ca toate threadurile sa fi terminat de calculat multimea Julia
	pthread_barrier_wait(&barrier1);
	// se recalculeaza start si end pentru a se realiza
	// trecerea din coordonate matematice in coordonate ecran
	start = thread_id * (double) heightJulia / 2 * P;
	end = min((thread_id + 1) * (double) heightJulia / 2 * P, heightJulia / 2);
	for (i = start; i < end; i++) {
		int *aux = resultJulia[i];
		resultJulia[i] = resultJulia[heightJulia - i - 1];
		resultJulia[heightJulia - i - 1] = aux;
	}

	
	
	
}
// functie pentru calculul paralel al multimii Mandelbrot
void Mandelbrot(int thread_id) {
	int w, h, i;
	int start = thread_id * (double) heightMandelbrot/ P;
	int end = min((thread_id + 1) * (double) heightMandelbrot / P, heightMandelbrot);

	for (w = 0; w < widthMandelbrot; w++) {
		for (h = start; h < end; h++) {
			complex c = { .a = w * parMandelbrot.resolution + parMandelbrot.x_min,
							.b = h * parMandelbrot.resolution + parMandelbrot.y_min };
			complex z = { .a = 0, .b = 0 };
			int step = 0;

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < parMandelbrot.iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = pow(z_aux.a, 2.0) - pow(z_aux.b, 2.0) + c.a;
				z.b = 2.0 * z_aux.a * z_aux.b + c.b;

				step++;
			}

			resultMandelbrot[h][w] = step % 256;
		}
	}

	// se asteapta ca toate threadurile sa fi terminat de calculat multimea Mandelbrot
	pthread_barrier_wait(&barrier1);

   	// se recalculeaza start si end pentru a se realiza
	// trecerea din coordonate matematice in coordonate ecran
	start = thread_id * (double) heightMandelbrot / 2 * P;
	end = min((thread_id + 1) * (double) heightMandelbrot / 2 * P, heightMandelbrot / 2);
	for (i = start; i < end ; i++) {
		int *aux = resultMandelbrot[i];
		resultMandelbrot[i] = resultMandelbrot[heightMandelbrot - i - 1];
		resultMandelbrot[heightMandelbrot - i - 1] = aux;
	}
	
	


	
}

// functie pentru threaduri care va efectua calculul paralel astfel
// calculeaza Julia
// scrie in fisier rezultatul calculului multimii Julia
// calculeaza Mandelbrot
// scrie in fisier rezultatul calculului multimii Mandelbrot
void *thread_function(void *arg) {
	int thread_id = *(int *)arg;
	// se calculeaza multimea Julia
	Julia(thread_id);
	// se asteapta ca toate threadurile sa fi terminat de calculat multimea Julia
	pthread_barrier_wait(&barrier2);
	// de scrierea in fisier a rezultatului se va ocupa un singur thread
	if (thread_id == 0) {
		write_output_file(out_filename_julia, resultJulia, widthJulia, heightJulia);
	}
	// se calculeaza multimea Mandelbrot
	Mandelbrot(thread_id);
	// se asteapta ca toata threadurile sa fi terminat de calculat multimea Mandelbrot
	pthread_barrier_wait(&barrier3);
	// un singur thread va realiza scierea in fisiser a rezultatului
	if(thread_id == 0) {
		write_output_file(out_filename_mandelbrot, resultMandelbrot, widthMandelbrot, heightMandelbrot);
	}
	
}

int main(int argc, char *argv[]) {


	// se citesc argumentele programului
	get_args(argc, argv);

	pthread_t threads[P];
	int thread_id[P];
  
    // creez barierele
	pthread_barrier_init(&barrier1, NULL, P);
	pthread_barrier_init(&barrier2, NULL, P);
	pthread_barrier_init(&barrier3, NULL, P);

	// citesc parametrii de rulare pentru cele doua multimi
	// aloc memorie pentru matricile rezultat ale celor 2 multimi
	read_input_file(in_filename_julia, &parJulia);
	read_input_file(in_filename_mandelbrot, &parMandelbrot);
	widthJulia = (parJulia.x_max - parJulia.x_min) / parJulia.resolution;
	heightJulia = (parJulia.y_max - parJulia.y_min) / parJulia.resolution; 
	resultJulia = allocate_memory(widthJulia, heightJulia);
	widthMandelbrot = (parMandelbrot.x_max - parMandelbrot.x_min) / parMandelbrot.resolution;
	heightMandelbrot = (parMandelbrot.y_max - parMandelbrot.y_min) / parMandelbrot.resolution; 
	resultMandelbrot = allocate_memory(widthMandelbrot, heightMandelbrot);


	// creez threadurile
	for (int i = 0; i < P; i++) {
		thread_id[i] = i;
		pthread_create(&threads[i], NULL, thread_function, &thread_id[i]);

	}

	// se asteapta threadurile
	for (int i = 0; i < P; i++) {
		pthread_join(threads[i], NULL);
	}

	// distrug barierle
	pthread_barrier_destroy(&barrier1);
	pthread_barrier_destroy(&barrier2);
	pthread_barrier_destroy(&barrier3);
	
	// eliberez memoria
	free_memory(resultJulia, heightJulia);
	free_memory(resultMandelbrot, heightMandelbrot);
	

	return 0;


}