#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#define N_PARADAS 5 // número de paradas de la ruta
#define EN_RUTA 0 // autobús en ruta
#define EN_PARADA 1 // autobús en la parada
#define MAX_USUARIOS 40 // capacidad del autobús
#define USUARIOS 4 // numero de usuarios

int estado = EN_RUTA; // estado inicial
int parada_actual = 0; // parada en la que se encuentra el autobus
int n_ocupantes = 0; // ocupantes que tiene el autobús

int esperando_parada[N_PARADAS]; //= {0,0,...0};  // personas que desean subir en cada parada

int esperando_bajar[N_PARADAS]; //= {0,0,...0};  // personas que desean bajar en cada parada

pthread_mutex_t tCerrojo;
pthread_cond_t finBajadaSubida;
pthread_cond_t busEnParada[N_PARADAS];
pthread_t tusuarios[USUARIOS];
pthread_t tAutoBus;

void Autobus_En_Parada();
void Conducir_Hasta_Siguiente_Parada();
void Subir_Autobus(int id_usuario, int origen);
void Bajar_Autobus(int id_usuario, int destino);
void Usuario(int id_usuario, int origen, int destino);

void * thread_autobus(void * args) {   // Definiciones globales (comunicación y sincronización)

	while (1) {
		// esperar a que los viajeros
		// suban y bajen
		Autobus_En_Parada();
		// conducir hasta siguiente parada
		// insertar sleep(1) para retardo
		Conducir_Hasta_Siguiente_Parada();
	}
}
void * thread_usuario(void * arg) {
int id_usuario, origen, destino;
// obtener el id del usario
id_usuario = (int)arg;
	while (1) {
		origen=rand() %N_PARADAS;
		do{destino=rand() %N_PARADAS;}while(origen==destino);
		printf("NUEVO USUARIO: %d, con ORIGEN: %d  y DESTINO: %d\n" , id_usuario, origen, destino);
		Usuario(id_usuario,origen,destino);
	}
}
void Usuario(int id_usuario, int origen, int destino) { 
	// Esperar a que el autobus esté en parada origen para subir
	Subir_Autobus(id_usuario, origen);
	// Bajarme en estación destino
	Bajar_Autobus(id_usuario, destino);
}

void Autobus_En_Parada(){
	/* Ajustar el estado y bloquear al autobús hasta que no haya pasajeros que quieran
	bajar y/o subir la parada actual. Después se pone en marcha */
	
	//printf("Ocupantes al entrar: %d \n" , n_ocupantes);
	
	//printf("Esperando en la parada: %d \n" , esperando_parada[parada_actual]);

	pthread_mutex_lock(&tCerrojo);
	estado = EN_PARADA;
	pthread_cond_broadcast(&busEnParada[parada_actual]);		// Envía la señal a todos los hilos de que ha llegado a la parada
	while(esperando_bajar[parada_actual] > 0 ||
		(esperando_parada[parada_actual] > 0 && n_ocupantes <= MAX_USUARIOS)) 
		pthread_cond_wait(&finBajadaSubida, &tCerrojo);			// Espera a que no haya más usuarios pendientes de subir/bajar


	pthread_mutex_unlock(&tCerrojo);
	//printf("Ocupantes al salir: %d \n" , n_ocupantes);


}
void Conducir_Hasta_Siguiente_Parada(){
	pthread_mutex_lock(&tCerrojo);
	parada_actual = (parada_actual + 1)%N_PARADAS;
	printf("Conduciendo a parada: %d \n" , parada_actual);
	estado = EN_RUTA;
	// Retardo
	sleep(4);
	// Actualizar numero de parada
	pthread_mutex_unlock(&tCerrojo);
}
void Subir_Autobus(int id_usuario, int origen){
	/* El usuario indicará que quiere subir en la parada ’origen’, esperará a que el
	autobús se pare en dicha parada y subirá. El id_usuario puede utilizarse para
	proporcionar información de depuración */
	
	pthread_mutex_lock(&tCerrojo);
	
	esperando_parada[origen]++;

	pthread_cond_wait(&busEnParada[origen], &tCerrojo);		// Espera a la señal de cuando el bus esté en la parada indicada
	/*
	while(estado != EN_PARADA ||  parada_actual != origen){
		pthread_cond_wait(&busEnParada[origen], &tCerrojo);
	}
	*/

	n_ocupantes++;
	esperando_parada[origen]--;
	if(esperando_parada[origen] == 0)pthread_cond_signal(&finBajadaSubida);			// Envía la señal de que este usuario ha terminado de subir
	printf("Se ha subido: %d\n" , id_usuario);
	pthread_mutex_unlock(&tCerrojo);
	
}
void Bajar_Autobus(int id_usuario, int destino){
	/* El usuario indicará que quiere bajar en la parada ’destino’, esperará a que el
	autobús se pare en dicha parada y bajará. El id_usuario puede utilizarse para
	proporcionar información de depuración */
	pthread_mutex_lock(&tCerrojo);
	esperando_bajar[destino]++;
	/*
	while(estado != EN_PARADA || parada_actual != destino)
		pthread_cond_wait(&busEnParada[destino], &tCerrojo);
	*/
	pthread_cond_wait(&busEnParada[destino], &tCerrojo);		// Espera a la señal de cuando el bus esté en la parada indicada
	n_ocupantes--;
	esperando_bajar[destino]--;
	printf("Se ha bajado: %d \n" , id_usuario);
	if (esperando_bajar[destino] == 0)pthread_cond_signal(&finBajadaSubida);			// Envía la señal de que este usuario ha terminado de bajar
	pthread_mutex_unlock(&tCerrojo);

}


int main(int argc, char *argv[]) {
	int i;

	// Definición de variables locales a main
	// Opcional: obtener de los argumentos del programa la capacidad del autobus, el
	//numero de usuarios y el numero de paradas
	// Crear el thread Autobus
	pthread_create(&tAutoBus,NULL,thread_autobus, NULL);
	pthread_mutex_init(&tCerrojo,NULL);
	pthread_cond_init(&finBajadaSubida,NULL);
	for (int i = 0; i < N_PARADAS;i++){
		pthread_cond_init(&busEnParada[i],NULL);
	}
	
	for (i = 0; i < USUARIOS; i++){
		// Crear thread para el usuario i
		// Esperar terminación de los hilos
		pthread_create(&tusuarios[i], NULL, thread_usuario, (void*)i);
	}
	for(i = 0; i < USUARIOS; i++){
		pthread_join(tusuarios[i],NULL);
	}
	return 0;
}
