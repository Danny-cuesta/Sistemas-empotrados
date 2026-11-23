//--PRÁCTICA 3 S.EMPOTRADOS


#include <Thread.h> //--> Para ArduinoThread
#include <StaticThreadController.h>
#include <ThreadController.h>
#include <avr/wdt.h> //--> Watchdog
#include <LiquidCrystal.h> //--> Para la pantalla LCD
#include <DHT.h> //--> Para el sensor de temperatura y humedad

#define DHTTYPE DHT11 // 3. Definir el modelo exacto del sensor (DHT11)

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
unsigned long tEstado = 0;   //--> Sirve para controlar el tiempo del estado actual


//--Variables de los leds
const byte ledVerdePin = 3;
const byte ledAzulPin = 4;

//--Variables del sensor de ultrasonidos
const byte PIN_TRIGGER = 5;
const byte PIN_ECHO = 6;
const float V_SOUND_AIR = 0.0343f; // cm/us
float distance = 999.0f; //--> Lo inicizalizamos a una distancia grande porque indica "no hay nadie a la vista"
long duration = 0;

//--Variables del sensor de temperatura
const byte dhtPin = A0;
DHT dht(dhtPin, DHT11);
float temperature = 0, humidity = 0;

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
const char* products[5] = {"Café Solo", "Café Cortado", "Café Doble", "Café Premium", "Chocolate"};
int costs[5] = {100, 110, 125, 150, 200};
unsigned long duracionPrep = 0; //--> Indica cuanto durará la preparación de la bebida
byte selection = 0; //--> Producto marcado en el catálogo

//--Variables del joystick
const int pinX = A4;
const int pinY = A5;
const int pinSW = 7;

//--Variables del botón
const byte botonPin = 2;
volatile bool pulsado = false;
volatile unsigned long tPulsado = 0;
volatile unsigned long tSoltado = 0;

//--Variables que indican cambios de eventos del joystick y del botón externo
bool evUp = false;
bool evDown = false;
bool evLeft = false;
bool evSelection = false;
bool evCancelar = false; //--> Evento que indica que se encuentra en estado de reinicio
//--Variables prev
bool prevUp = false;
bool prevDown = false;
bool prevLeft = false;
bool prevSelection = false;
bool prevBoton = false;

bool evAdmin = false;
bool admin1Time = false; //--> Para que cada pulsación larga, admin se active una vez
//unsigned long durationBoton = 0; //--> Para comprobar la duración del botón

//--Parpadeo del arranque
const unsigned long T_MEDIO_CICLO = 500;  //--> El LED1 cambia cada 500 ms
byte cambiosLed = 0; //--> Contador de los cambios de los leds


//_________________________________________________________________________________________________________
//_________________________________________________________________________________________________________



//--FUNCIONES EXTERNAS NECESARIAS DEL SISTEMA


//--Función para ir de un estado a otro
void irA(Servicio s){
  servicio = s;
  tEstado = millis();
}

//--------------------------------------------------


//--Función que devuleve el sistema al punto de partida
void reiniciarServicio(){
  digitalWrite(ledVerdePin, LOW);
  digitalWrite(ledAzulPin, LOW);
  irA(ESPERANDO);
}



//--------------------------------------------------------------------------------------------------------------


//--FUNCIONES HILOS DEL SISTEMA



//--Función que LEE la distancia
void readDistance(){

  //--Para el SENSOR DE ULTRASONIDOS, generamos pulsos en el disparador
  digitalWrite(PIN_TRIGGER, LOW); //--> Nos aseguramos de que no haya ninguna señal y que el HIGH salga limpio
  delayMicroseconds(2);
  digitalWrite(PIN_TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIGGER, LOW);

  //--Leer el pulso de duración y la distancia
  duration = pulseIn(PIN_ECHO, HIGH, 6000UL); //--> 6000UL es el límite de tiempo de espera

  if (duration == 0){ //--> No ha devuelto ningún eco (no ha rebotado la señal), no hay nadie
    distance = 999.0f;
  }
  else{
    distance = (duration * V_SOUND_AIR) / 2.0f;
  }

}


//--------------------------------------------------------------------------------------------------------------


//--Función que LEE la temperatura y la humedad
void readDht(){

  //--Variables que leen cada cosa
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  //--Comprobar que los números son válidos (NaN significa Not a Number)
  if(!isnan(t)){
    temperature = t;
  }
  if(!isnan(h)){
    humidity = h;
  }

}


//--------------------------------------------------------------------------------------------------------------



//--Función que lee la entrada del joystick y del botón
void readJoystickBoton(){

  //--Definición de variables (joystick)
  int x = analogRead(pinX);
  int y = analogRead(pinY);
  int botonJoystick = digitalRead(pinSW);
  bool up = false;
  bool down = false;
  bool left = false;
  bool selection = (digitalRead(pinSW) == LOW); //--> Se define con LOW porque está confirgurado con INPUT_PULLUP y se invierte la lógica

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

  //--Un evento solo cuando se pasa de "reposo" a "movido", si solo se cambia una vez debe pasar un producto
  if(up && !prevUp){
    evUp = true;
  }
  if(down && !prevDown){
    evDown = true;
  }
  if(left && !prevLeft){
    evLeft = true;
  }
  if(selection && !prevSelection){
    evSelection = true;
  }
  //--Definir valor de los eventos anteriores
  prevUp = up;
  prevDown = down;
  prevLeft = left;
  prevSelection = selection;
  

  //--Botón externo para entrar en el modo admin (si está presionado más de 5 segundos)
  noInterrupts(); //--> Desactivamos las interrupciones por un momento para poder copiar los valores correctamente
  bool bot = pulsado;
  unsigned long t0 = tPulsado;
  unsigned long t1 = tSoltado;
  interrupts(); //--> Se vuelven a activar las interrupciones del código

  //--5s seguidos pulsado: entrar o salir de Admin
  if (bot  && !admin1Time && millis() - t0 >= 5000){
    evAdmin = true;
    admin1Time = true;
  }

  //--Comprobar si al pulsar el botón, se quiso reiniciar el sistema
  if (!bot && prevBoton){
    unsigned long d = t1 - t0;
    if (d >= 2000 && d <= 3000){ //--> Si se pulsó de 2 a 3s, reiniciar sistema
      evCancelar = true;
    }
  }

  //--Al soltar, se prepara la siguiente pulsación (siempre después de la comprobación anterior)
  if (!bot){
    admin1Time = false;
  }

  prevBoton = bot; //--> Actualizamos el pulsado anterior del boton

}


//--------------------------------------------------------------------------------------------------------------



//--Función que imprime en la pantalla lo que toque segun el estado actual
void printLCD(){

  switch (modo){

    case ARRANQUE:
      lcd.print(0, "CARGANDO...");
      lcd.print(1, "");
      break;


    case SERVICIO:

      switch (servicio){ //--> Contemplar cada modo del servicio
        case ESPERANDO:
          lcd.print(0, "ESPERANDO CLIENTE...");
          lcd.print(1, "");
          break;

        case ENTORNO:
          break;

        case CATALOGO:
          break;

        case PREPARANDO:
          lcd.print(0, "PREPARANDO BEBIDA..."); //--> Falta añadir el nombre de la selección
          lcd.print(1, "");
          break;

        case RETIRAR:
          lcd.print(0, "RETIRE SU BEBIDA.");
          lcd.print(1, "");
          break;

      }
      break;


      case ADMIN:

        switch(admin){
          case MENU:
         
            break;

          case VER_TEMP:

            break;

          case VER_DIST:

            break;

          case VER_CONT:
        
            break;

          case PRECIOS_LISTA:
         
            break;

          case PRECIOS_EDIT:
         
            break;

        }
        break;

  }

}


//--------------------------------------------------------------------------------------------------------------



//--Función que actua como máquina de estados y decide cuando el sistema cambia de uno a otro
void stateMachine(){

  //--Comprobar si está en modo admin
  if(evAdmin){ //--> Se ha pulsado previamente el botón durante 5 segundos

    //--Cambiar de estado en función de si está en uno o en otro
    if (modo == ADMIN){ //--> Salir de Admin
      modo = SERVICIO;
      digitalWrite(led1Pin, LOW);
      digitalWrite(led2Pin, LOW);
      reiniciarServicio();
    }
    else if (modo == SERVICIO){ //--> Entrar en admin
      modo = ADMIN;
      admin = MENU;
      //menuSel = 0;
      //--Se encienden los dos leds
      digitalWrite(led1Pin, HIGH);
      digitalWrite(led2Pin, HIGH);
    }
  }
  else if(evCancelar && modo == SERVICIO){ //--> Reiniciar sistema
    reiniciarServicio();
  }


  //--Evaluar los valores del joysitck
  switch (modo){

    case ARRANQUE:

      //--¿Han pasado 500 ms desde la última vez que cambió el LED?
      if (millis() - tEstado >= T_MEDIO_CICLO){
        tEstado = millis(); //-_> Apuntar la "hora" actual para empezar a contar los siguientes 500ms

        //--Invertir el estado del LED, medimos los cambios
        int estadoActual = digitalRead(led1Pin);
        digitalWrite(led1Pin, !estadoActual);
        cambiosLed++;

        //--Comprobar los cambios
        if (cambiosLed >= 6){ //--> Se enciende (cambio 1), Se apaga (cambio 2) -->  3 * 2 = 6 cambios (se ha encendido 3 veces)
          digitalWrite(ledVerdePin, LOW);
          modo = SERVICIO;
          irA(ESPERANDO); //--> Comenzamos esperando a clientes
        }
      }
      break;


    case SERVICIO:

      switch(servicio){ //--> Contemplar cada modo del servicio
        case ESPERANDO:
          //--Se detecta a un cliente si la distancia es inferior a 1m
          if(distance < 100){
            irA(ENTORNO);
          }
          break;

        case ENTORNO:
          //--Reservar 5 segundos para ver la tempetarura y la humedad (eso lo imprimirá la función printLCD())
          if(millis() - tEstado >= 5000){ //--> Cuando pasen los 5s, ir al catálogo de productos
            irA(CATALOGO);
          }
          break;

        case CATALOGO:

          break;

        case PREPARANDO:
        
          break;

        case RETIRAR:
          //--Esperar 3s y volver a detectar clientes
          if(millis() - tEstado >= 3000){
            irA(ESPERANDO);
          }
          break;

      }
      break;


    case ADMIN:

      switch(admin){
        case MENU:
         
          break;

        //--No indicamos nada porque solo hay que imprimir
        case VER_TEMP:
        case VER_DIST:

        case VER_CONT:
        
          break;

        case PRECIOS_LISTA:
         
          break;

        case PRECIOS_EDIT:
         
          break;

      }
      break;

  }

  //--Resetear variables
  evUp = evDown = evLeft = evSelection = false;
  evAdmin = evCancelar = false;

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

  dht.begin(); //--> Preparar sensor de temperatura

  attachInterrupt(digitalPinToInterrupt(botonPin), interrupcionBoton, CHANGE); //--> Interrupción para el botón


  //--Configurar los hilos y añadirlos a la cola del controlador (scheduler)
  threadDistance.onRun(readDistance);
  threadDistance.setInterval(200);

  threadTemperature.onRun(readDht);
  threadTemperature.setInterval(2000);

  threadJoystickBoton.onRun(readJoystickBoton);
  threadJoystickBoton.setInterval(30);

  threadLCD.onRun(printLCD);
  threadLCD.setInterval(200);

  threadMachine.onRun(stateMachine);
  threadMachine.setInterval(50);

  controller.add(&threadDistance);
  controller.add(&threadTemperature);
  controller.add(&threadJoystickBoton);
  controller.add(&threadLCD);
  controller.add(&threadMachine);

  modo = ARRANQUE;
  tEstado = millis();


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

  if (digitalRead(botonPin) == LOW){
    tPulsado = millis(); //--> Guarda el tiempo en el que se pulsó
    pulsado = true;
  }
  else{
    tSoltado = millis(); //--> Guarda el tiempo en el que se soltó
    pulsado = false;
  }

}




