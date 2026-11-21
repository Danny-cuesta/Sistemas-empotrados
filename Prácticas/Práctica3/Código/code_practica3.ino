//--PRÁCTICA 3 S.EMPOTRADOS


#include <Thread.h> //--> Para ArduinoThread
#include <StaticThreadController.h>
#include <ThreadController.h>
#include <avr/wdt.h> //--> Watchdog
#include <LiquidCrystal.h> //--> Para la pantalla LCD
#include <DHT.h> //--> Para el sensor de temperatura y humedad


//--HILOS NECESARIOS
ThreadController controller; //-->  Maneja y controla los hilos (agrupa varias tareas y decide cuáles toca ejecutar)
Thread threadDistance; //--> Lanza el pulso del ultrasonidos y guarda la distancia en una variable
Thread threadTemperature; //--> Lee temperatura y humedad y las guarda en variables
Thread threadJoystickBoton; //--> Mira el joystick y el botón externo, y apunta “ha pasado esto”
Thread threadLCD; //--> Dibuja en el LCD lo que toque según el estado actual
Thread threadMachine; //--> Decide en qué estado está el sistema y cuándo se cambia de uno a otro, máquina de estados

//--VARIABLES QUE INDICAN EL ESTADO EN EL QUE ESTÁ EL SISTEMA y lo que pueden hacer los distintos modos
enum Modo{
  ARRANQUE,
  SERVICIO,
  ADMIN
};

enum Servicio{
  ESPERANDO,
  ENTORNO,
  CATALOGO,
  PREPARANDO,
  RETIRAR
};

enum Admin{
  MENU,
  VER_TEMP,
  VER_DIST,
  VER_CONT,
  PRECIOS_LISTA,
  PRECIOS_EDIT
};

//--Inicialización de las instancias de los enumeradores
Modo modo = ARRANQUE;
Servicio servicio = ESPERANDO;
Admin admin = MENU;


//--Variables de los leds
const byte ledVerdePin = 3;
const byte ledAzulPin = 4;

//--Variables del botón
const byte botonPin = 2;
volatile bool pulsado = false;

//--Variables del sensor de ultrasonidos
const byte PIN_TRIGGER = 5;
const byte PIN_ECHO = 6;
const float V_SOUND_AIR = 0.0343f; // cm/us
float distance = 999.0f; //--> Lo inicizalizamos a una distancia grande porque indica "no hay nadie a la vista"
long duration  = 0;

//--Variables del sensor de temperatura
//const byte temperaturePin = A0;
//DHT dht(dhtPin, DHT11);
//float temperatura = 0, humedad = 0;

//--Variables de la pantalla y lo que se imprimirá en ella
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);
//                 RS, E, D4, D5, D6, D7
// VSS: GND
// VDD: 5V
// V0: Sirve para controlar el contraste (se conecta al potenciómetro)
// RS: Register Select, le dice a la pantalla si lo que se envía es un comando (borrar, mover cursor...) o un carácter para mostrar
// RW: GND
// E: Enable habilita la lectura de datos
// D4-D7: Por aquí Arduino envía los datos
// A: 5V
// K: GND
const char* products["Café Solo", "Café Cortado", "Café Doble", "Café Premium", "Chocolate"]
const float* costs[1.00, 1.10, 1.25, 1.50, 2.00]
unsigned long duracionPrep = 0; //--> Indica cuanto durará la preparación de la bebida

//--Variables del joystick
const int pinX = A4;
const int pinY = A5;
const int pinSW = 7;

//--Variables que indican cambios de eventos del joystick y del botón externo
bool up = false;
bool down = false;
bool left = false;
bool admin = false;
unsigned long durationBoton = 0; //--> Para comprobar la duración del botón


//_________________________________________________________________________________________________________
//_________________________________________________________________________________________________________



//--FUNCIONES EXTERNAS NECESARIAS DEL SISTEMA

//--Función que LEE la distancia
void readDistance(){

  //--Para el SENSOR DE ULTRASONIDOS, generamos pulsos en el disparador
  digitalWrite(PIN_TRIGGER, LOW); //--> Nos aseguramos de que no haya ninguna señal y que el HIGH salga limpio
  delayMicroseconds(2);
  digitalWrite(PIN_TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIGGER, LOW);

  //--Leer el pulso de duración y la distancia
  duration = pulseIn(PIN_ECHO, HIGH);

  if (duration == 0){ //--> No ha devuelto ningún eco (no ha rebotado la señal), no hay nadie
    distancia = 999.0f;
  }
  else{
    distance = (duration * V_SOUND_AIR) / 2.0f;
  }

}


//--------------------------------------------------------------------------------------------------------------


void readTemperature(){


}


//--------------------------------------------------------------------------------------------------------------



//--Función que lee la entrada del joystick y del botón
void readJoystickBoton(){

  //--Definición de variables (joystick)
  int x = analogRead(pinX);
  int y = analogRead(pinY);
  int botonJoystick = digitalRead(pinSW);

  //--Evaluar los eventos segun los valores del joystick
  if (y < 300){ //--> Arriba
    up = true;
  }

  if (y > 700){ //--> Abajo
    down = true;
  }

  if (x < 300){ //--> izquierda
    left = true;
  }
  

  //--Botón externo para entrar en el modo admin (si está presionado más de 5 segundos)
  durationBoton = millis()
  if (millis() - durationBoton >= 5000){ // **
    admin = true;
  }


}


//--------------------------------------------------------------------------------------------------------------



//--Función que imprime en la pantalla lo que toque segun el estado actual
void printLCD(){

  switch (modo){

    case ARRANQUE:
      linea(0, "CARGANDO...");
      linea(1, "");
      break;


    case SERVICIO:

      switch(Servicio){ //--> Contemplar cada modo del servicio
        case ESPERANDO:
          linea(0, "ESPERANDO CLIENTE...");
          linea(1, "");
          break;

        case ENTORNO:
          break;

        case CATALOGO:
          break;

        case PREPARANDO:
          linea(0, "PREPARANDO BEBIDA..."); //--> Falta añadir el nombre de la selección
          linea(1, "");
          break;

        case RETIRAR:
          linea(0, "RETIRE SU BEBIDA.");
          linea(1, "");
          break;

      }

      break;


    case ADMIN:
      linea(0, "ADMIN...");
      linea(1, "");
      break;

  }

}


//--------------------------------------------------------------------------------------------------------------



//--Función que actua como máquina de estados y decide cuando el sistema cambia de uno a otro
void stateMachine(){

  //--Evaluar los valores del joysitck
  switch (modo){

    case ARRANQUE:
      
      break;


    case SERVICIO:

      switch(Servicio){ //--> Contemplar cada modo del servicio
        case ESPERANDO:
         
          break;

        case ENTORNO:
          break;

        case CATALOGO:
          break;

        case PREPARANDO:
        
          break;

        case RETIRAR:
         
          break;

      }

      break;


    case ADMIN:
     
      break;

  }

  up = down = left = false; //-> Resetear variables

}



//_________________________________________________________________________________________________________
//_________________________________________________________________________________________________________




//--FUNCIÓN DE INICIO
void setup() {
  Serial.begin(9600);
  pinMode(ledVerdePin, OUTPUT);
  pinMode(ledAzulPin, OUTPUT);
  pinMode(botonPin, INPUT_PULLUP); //--> Tiene resistencia interna
  pinMode(PIN_TRIGGER, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(pinSW, INPUT_PULLUP);

  digitalWrite(PIN_TRIGGER, LOW); //--> Nos aseguramos que no envíe ninguna señal
  //--Al empezar, todos los leds están apagados
  digitalWrite(ledVerdePin, LOW);
  digitalWrite(ledAzulPin, LOW);

  //--Preparar la pantalla
  lcd.begin(16, 2); //--> Son las dimensiones de la pantalla

  attachInterrupt(digitalPinToInterrupt(botonPin), interrupcionBoton, FALLING); //--> Interrupción para el botón


  //--Configurar los hilos y añadirlos a la cola del controlador (scheduler)
  threadDistance.onRun(readDistance);
  threadDistance.setInterval(20);

  threadTemperature.onRun(readTemperature);
  threadTemperature.setInterval(300);

  threadJoystickBoton.onRun(readJoystickBoton);
  threadJoystickBoton.setInterval(1500);

  threadLCD.onRun(printLCD);
  threadLCD.setInterval(400);

  threadMachine.onRun(stateMachine);
  threadMachine.setInterval(100);

  controller.add(&threadDistance);
  controller.add(&threadTemperature);
  controller.add(&threadJoystickBoton);
  controller.add(&threadLCD);
  controller.add(&threadMachine);


}




//_________________________________________________________________________________________________________
//_________________________________________________________________________________________________________





//--FUNCIÓN DEL BUCLE PRINCIPAL
void loop() {

  controller.run(); //--> Ejecuta cada hilo en función de como lo tenga colocado en la cola

}



//_________________________________________________________________________________________________________
//_________________________________________________________________________________________________________




//--Función llamada por la interrupción
void interrupcionBoton() {
  pulsado = true;
}

