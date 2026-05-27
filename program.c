#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

int cars_bridge_amt = 0;
int car_on_bridge = -1;
int A_city = 10;
int A_waiting = 0;
int B_city = 0;
int B_waiting = 0;

typedef enum {
	DRIVING_AB,
	DRIVING_BA,
	EMPTY
} bridge_state;
bridge_state current_state = EMPTY;

sem_t bridge_sem;
sem_t state_sem;

void print_state() {
	switch (bridge_state) {
		case EMPTY:
			printf("A-%d %d>>> [BRAK] <<<%d %d-B\n", A_city, A_waiting, B_waiting, B_city);
			break;

		case DRIVING_AB:
			printf("A-%d %d>>> [>>%d>>] <<<%d %d-B\n", A_city, A_waiting, car_on_bridge, B_waiting, B_city);
			break;

		case DRIVING_BA:
			printf("A-%d %d>>> [<<%d<<] <<<%d %d-B\n", A_city, A_waiting, car_on_bridge, B_waiting, B_city);
			break;
	}
}

void* car_thread(void* arg) {
    int car_id = *(int*)arg;
    char city = 'A';
    free(arg);

    printf("[Samochód %d] Skonczyl inicjalizacje\n", car_id);

    for (int i = 0; i < 3; i++) {
        printf("[Samochód %d] Stoi w miescie %c\n", car_id, city);
	sleep(rand() % 5 + 1);

	// Proba wjazdu
	// Popros o sem do zmiany danych
	if (city == 'A') {
		A_city--;
		A_waiting++;
	}
	else {
		B_city--;
		B_waiting++;
	}
	// Odblokuj sem do zmiany danych

	// Popros o sem do mostu
	// Popros o sem do danych

	// Sekcja krytyczna !
	if (city == 'A') {
		A_waiting--;
		current_state = DRIVING_AB;
	}
	else {
		B_waiting--;
		current_state = DRIVING_BA;
	}

	car_on_bridge = car_id;
	print_state();
        printf("[Samochód %d] Przejezdza przez most\n", car_id);
	// odblokuj sem do zmiany danych

	sleep(rand() % 5 + 1);
	current_state = EMPTY;
	// odblokuj sem do mostu
	// Sekcja krytyczna !

	// Dopisac
	if (city == 'A') city = 'B';
	else city = 'A';
        printf("[Samochód %d] Skonczyl przejazd, jest teraz w %c\n", car_id, city);
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
