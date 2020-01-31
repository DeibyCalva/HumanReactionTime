/*
 * HumanReactionTime.c
 *
 * Created: 28/1/2020 17:47:04
 * Author : Deiby Calva
 */ 

//Putty: emulador de terminal, consola en serie aplicacion y la transferencia de archivos

#define F_CPU 16000000UL   //Reloj master de 16000000
#include <inttypes.h>     //utiliza si quieres usar nombres de tipos independientes de cpu como char(u_int8 es lo mismo que char)
#include <avr/io.h>      //nombres lógicos de cada nombre de pin (OCR0A, OCIE0A)
#include <avr/interrupt.h> //sólo dos macros para configurar ISR
#include <stdio.h>        //archivos de cabecera para las interfaces en serie, getchar putchar
#include <avr/eeprom.h>  //Es un tipo de memoria ROM que puede ser programada, borrada y reprogramada eléctricamente
// libreria  de comunicación serial
#include "uart.h"  // Transmisor receptor asincrono universal, controla los puertos y dispositivos en seri
#define begin {
#define end   }
//nombres de los estados de las máquinas de estado
#define NoPush 1 //no presionado
#define MaybePush 2//puede que este precionado
#define Pushed 3 //presionado
#define MaybeNoPush 4//Puede que no este presionadao

//Definir la lectura y escritura de a EEEPROM
#define eeprom_true 0
#define eeprom_data 1

volatile unsigned char time1, time2, time3;		//contadores de tiempo //estas variables deben ser compartidas 
unsigned char PushState;						//estado de la maquina
unsigned int time;								//timepo de lectuura del boton  // un valor de milisegundos desde el último reinicio
unsigned int state;								//comprobar la maquina de estado
unsigned int Tiempo_espe ;						///tiempo de retardo
int nAleat ;

// Descriptor del archivo UART
//La función getchar recibe un carácter, mientras que la función putchar imprime un carácter.
//  Permiten manipular de distintas maneras archivos y caracteres. 
// putchar y getchar están en uart.c
FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

void initialize(void){
	//configurar los puertos
	DDRD = 0x00;			// todos los puertos DDRD como entrada
	DDRC = 0xff;	 		// todos los puertos DDRC como salida
	//OCR0A=16000000/(64*1000)-1=249
	//OCR0A: carga el valor hasta el cual se quiere que llegue el registro TCNT0 el el modo CTN
	OCR0A = 249;
	TIMSK0= (1<<OCIE0A);	//turn on timer 0 cmp match ISR
	//Envia al prescaler el valor de 64
	//pone a CS02=0    CS01=1    CS00=1  activa  los dos primeros bits del registro TCCR0B
	TCCR0B= 3;				//elige el presaclar a utilizar para obtener en cuanto tiempo se quiere que el registro TCNT0
	//sea igual al registro OCR0A
	// enciende on clear-on-match
	TCCR0A= (1<<WGM01) ;
	Tiempo_espe=0;
	PushState = NoPush;//Empujar el estado.
	state = 0; //progama inicia en 0
	
	
	uart_init();
	stdout = stdin = stderr = &uart_str; //envias un mensaje a la puerta serial
	fprintf(stdout,"Pulse el boton para encender el led \n\r  Atencion al Led o Sonido..! \n");
	//poner en marcha los ISR
	sei() ;

}
//*Interrupt Service Routine
//llamado controlador de interrupciones
ISR (TIMER0_COMPA_vect) {
	if (time1>0) 	--time1;
	if (time2>0)	--time2;  // decrementan los time si son mayores a cero
	if (time3>0)	--time3;
}


void tarea1(void)
begin
time1=1;						//reset the task timer
if (state==2){				//si  el estado es igual a 2 entoces
	PORTC |= (1<<3);			//SE enciende el led en el puerto 3 Esto activa el Bit y deja el resto a 0
	PORTC |= (1<<0);			//SE enciende el buzeer en el puerto 0
	time++;					//el tiempo se incrementa
	if(Tiempo_espe<3000){	//se entra al if anidado donde si el tiempo de espera es menor a 3000
		fprintf(stdout,"Te adelantaste!! \n Pulsa de nuevo.. \n\r");  //se imprime un mensaje de error
		state=4;				// se pasa al estado 4 dode todo se pone a cero
	}
}
else{						//en caso de que el estado no sea igual a 2
	PORTC=0x00;				//todos lo pines va a estar en 0
}
if (time>10000){				//si el tiempo  es mayor al tiempo aleatorio que se genero en el la tarea tskdelay
	//genera un mesaje de que el tiempo  supero el limete para pulsar el boton
	fprintf(stdout,"Tiempo limite Excedido..! \n Pulsa de nuevo.. \n\r");
	state=4;					// y pasa al estdo 4  donde todo se pone a 0
}
if (state==3)					// si el estado es igual a 3
{
	// escribe algunos datos en la bandera "written"
	int tiempoAnteriror = eeprom_read_word((uint16_t*)eeprom_data);
	fprintf(stdout,"Su tiempo Anterior es en ms: ");	// imprime un mesaje del tiempo anterrior  el ususario en pulsar el boton cuando el led esta encendido
	fprintf(stdout,"%d \n\r", tiempoAnteriror) ;
	
	
	eeprom_write_word((uint16_t*)eeprom_data,time);
	fprintf(stdout,"Su tiempo actual es: ");	// imprime un mesaje del tiempo que tarda el ususario en pulsar el boton cuando el led esta encendido
	fprintf(stdout,"%d \n\r", eeprom_read_word((uint16_t*)eeprom_data)) ;
	
	
	if (time<tiempoAnteriror)
	{
		fprintf(stdout,"FUISTES MAS RAPIDO, EXCELENTE");
	} 
	else
	{
		fprintf(stdout,"FUISTES MAS LENTO, ¡LO SIENTO!");
	}
	
	fprintf(stdout,"\n Pulse para intentarlo de nuevo.. ! \n ");/// imprime un mensaje para volver a intentar de nuevo medir el tiempo de reaccion
	
	state=4;				// despues de imprimir los mejase se va al estado 4 donde todo se pone  0
}
if (state==4){				//si el estado es igual a 4
	state=0;					// el estado se resetea y se pone a 0
	Tiempo_espe=0;				// el tiempo de espera se resetea y se pone a 0
	time=0;						// y el time se recetea y se pone a 0
}
end

void tarea2(void){
	time2=1;     //reset the task timer
	switch (PushState){//estado del boton
		case NoPush:				//1
		if (~PIND & 0x02){
			PushState=MaybePush;
			}else{
			PushState=NoPush ;
		}
		break;
		case MaybePush:			//2
		if (~PIND & 0x02){		//en esta caso PIND vale 00000000 pero al negarlo vale  111111/00000 como esta afirmacion es falsa
			PushState=Pushed;	// pushState vale 3, es decir se va al etado 3
			state++;				// estado incrementa
		}
		else{
			PushState=NoPush;
		}
		break;
		case Pushed:				//3
		if (~PIND & 0x02){
			PushState=Pushed;
			}else{
			PushState=MaybeNoPush;
		}
		break;
		case MaybeNoPush:			//4
		if (~PIND & 0x02){
			PushState=Pushed;
			}else{
			PushState=NoPush;
		}
		break;
	}
}

void tskdelay(void){
	time3=1;
	if (state==1){									// si el estado es igual a 1
		nAleat=(9000-5000) + rand ()% 4000;       // se genera valores aleatorios  entre 4 y 8 seg y se entra al segundo if
		if (Tiempo_espe< nAleat){					// si el tiempo de espera es menor al valor generado aleatoriamente
			Tiempo_espe++;							// el tiempo de espara se incrementa
			}else{										// si el tiempo de espera es igual o mayor al valor generado aleatoriamente
			state=2;								// se va al estado que vale 2 de la maquina de estado que esta en la tarea 2
		}
	}
}

//programa principal
int main(void){
	initialize();
	while(1){
		if (time1==0){		//si time1 llega a 0
			tarea1();		// se ejecuta la primera tarea
		}
		if (time2==0){		//si time2 llega a 0
			tarea2();		//se ejecuta la segunda tarea
		}
		if(time3==0){		////si time3 llega a 0
			tskdelay();		//se ejecuta la ultima tarea
		}
	}
}


