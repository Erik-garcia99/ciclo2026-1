#include<stdio.h>
#include<stdlib.h>
#include<string.h>

void ofuscado(char *cad_original, char *clave, char *cad_resultado);

int main(){

	int n=0; 
	char input[256];
	char clave[256];

	printf("ingrese la cadena a encriptar:"); 	
	fgets(input, sizeof(input),stdin);
	

	printf("ingrese la clave de ofuscacion:");
	fgets(clave, sizeof(clave),stdin);

	//necesitamos varibales que solo ocupan el esacio necesario 
	
	//fgets incute '\n' el enteer uqe se da, mas aparte agrega el caracter nulo '\0' entonces, strelen llega hasta el caracter nulo 
	// por lo que debemos debemos de elimnar uno menos en len y cunado asingamos memoria agrega ese mas en donde strcpy agrega el caracter nulo 
	
	//mensaje a encriptar 
	int len=0;
	len = strlen(input);
	
	printf("len de la cadena orignial: %i\n", len);
	char *msg_original = (char*)malloc(sizeof(len-1));
	strcpy(msg_original, input);

	printf("cadena orignial: %s\n", msg_original);

	
	int len_clave = strlen(clave);
	char *clave_ofuscada =(char*)malloc(sizeof(len_clave-1));
	strcpy(clave_ofuscada, clave);
	
	printf("len de la clave: %i\n", len);
	printf("cadena clave: %s\n", clave_ofuscada);


	char *cadena_cifrada = NULL; 
	printf("llamando a ofuscado\n");

	ofuscado(msg_original, clave_ofuscada, cadena_cifrada);
	
	//despues vuelve e imrpmirmos 
	

	printf("clave cifrada: %s", cadena_cifrada);	

}


//se aplica un xor a lo largo de toda la cadena, el ciclo se rige por la cadena a cifrar

void ofuscado(char *cad_original, char *clave, char *cad_resultado){

	
	printf("\nentro:");
	//como el algorimtos indinca que se aplicara un or del tamanio de la clave pero el mensaje original se puede extender mucho mas 
	//ocupo un offset en donde me indique 
	
	//esto es lo que se va a move en cada iteracion 
	int len_clave = strlen(clave);
	//el while sera del tamanio del mensaje a encriptar/ofuscar 
	int len_msg_original = strlen(cad_original);
	int offset = 0;
	
	char *aux = NULL;
	char *frame_original = NULL;

	while(offset < len_msg_original){
		
		memcpy(&frame_original,&cad_original, offset);

		*aux = *clave ^ *frame_original;
	       	
		memcpy(&cad_resultado, &aux, offset);

		offset += len_clave;
	}



	printf("\nsalio");

}




