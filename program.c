#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

int cars_bridge_amnt = 0;
int car_on_bridge = 0;
int A_city = 0;
int A_waiting = 0;
int B_city = 0;
int B_waiting = 0;

typedef enum {
	DRIVING_AB,
	DRIVING_BA,
	EMPTY
} bridge_state;
bridge_state = EMPTY;

void print_state() {
	switch (bridge_state) {
		case EMPTY:
			printf("A-%d %d>>> [BRAK] <<<%d %d-B", A_city, A_waiting, B_waiting, B_city);

		case DRIVING_AB:
			printf("A-%d %d>>> [>>%d>>] <<<%d %d-B", A_city, A_waiting, car_on_bridge, B_waiting, B_city);

		case DRIVING_BA:
			printf("A-%d %d>>> [<<%d<<] <<<%d %d-B", A_city, A_waiting, car_on_bridge, B_waiting, B_city);
	}
}

void* car_thread(void* arg) {
    int car_id = *(int*)arg;
    free(arg);

    printf("[Samochód %d] Uruchomiłem się i zaczynam jazdę!\n", car_id);

    for (int i = 0; i < 3; i++) {
        printf("[Samochód %d] Dojechałem do mostu i czekam...\n", car_id);

	while (cars_on_bridge != 0) {
		sleep(1);
	}

        printf("[Samochód %d] >>> Przejeżdżam przez most >>>\n", car_id);
	cars_on_bridge += 1;
        usleep(500000);

        printf("[Samochód %d] Zjechałem z mostu, jadę odpocząć.\n", car_id);
	cars_on_bridge -= 1;
        sleep(rand() % 3 + 1);
    }

    printf("[Samochód %d] Kończę działanie.\n", car_id);
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Użycie: %s <liczba_samochodów_N>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    if (N <= 0) {
        printf("Liczba samochodów musi być większa od 0.\n");
        return 1;
    }

    srand(time(NULL));

    pthread_t* threads = malloc(N * sizeof(pthread_t));
    printf("Tworzenie %d wątków (samochodów)...\n", N);

    for (int i = 0; i < N; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;

        if (pthread_create(&threads[i], NULL, car_thread, id) != 0) {
            perror("Błąd przy tworzeniu wątku");
            return 2;
        }
    }

    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    printf("Wszystkie samochody zakończyły jazdę.\n");

    return 0;
}
