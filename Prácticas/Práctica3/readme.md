
# PRÁCTICA 3: Controlador máquina expendedora

## Sensores utilizados en la práctica:

• 2 LEDs estándar  
• Pantalla LCD + potenciómetro para regular el contraste  
• Módulo Joystick  
• Sensor de Temperatura y Humedad DHT11  
• Sensor de Distancia por Ultrasonidos  
• Pulsador / Botón externo  

## Detalles del código y la solución del sistema:

Para esta práctica, he usado Threads para poder controlar varios elementos simultáneamente sin tener que usar delay() y bloquear el programa. Para los modos y estados del sistemas he utilizado enumeradores para poder controlar mejor las acciones de la máquina. Tenemos los modos: ARRANQUE, SERVICIO y ADMIN; y los submodos de cada uno, en SERVICIO se encuentran: esperando a ser atendido, entorno (temperatura y humedad), el catálogo de productos, preparando (cuando uno de los leds aumentará su brillo progresivamente) y retirar bebida. En ADMIN están: el menú de funciones del admin, y sus distintas funciones como "ver temperatura", "ver distancia", "ver contador", ver los precios de la lista y poder editarlos.

He creado los hilos en función de los sensores que hay, distancia, temperatura, joystick, pantalla LCD y a parte también necesito otro para la máquina de estados del sistema, el cual cambiará de acción a medida que se lo vayamos indicando. A parte también es muy importante tener un ThreadController que organice en una lista todos los hilos y los ejecute en función se guarden.

Por cada hilo he añadido su función correspondiente y a parte también he necesitado otras externas que me ayudan a tener mejor limpieza, como por ejemplo, para reiniciar el servicio, cambiar de subestado en el servicio o imprimir en la pantalla de forma limpia.

Por otro lado, también he añadido una interrupción para el botón externo ya que las interrupciones nos ayudan a reaccionar a eventos de forma rápida sin tener que estar constantemente revisandolo en el bucle, en el momento que se pulsa el botón, el código se interrumpe y ejecuta la función de la interrupción.



Por último, he integrado el mecanismo de seguridad de **watchdog**, el cual reinicia el sistema si nadie refresca su variable, evitando que el programa se quede en un bucle eterno.
Para probar este mecanismo he puesto estas lineas de código (luego se deben borrar para que todo funcione correctamente):

if (evSelection && modo == ADMIN){  
        while (true);  
}  

Indica que cuando el sistema está en modo admin y pulsamos el botón del joystick, entra en un bucle infinito, pero esto se evita con este mecanismo y se reinicia todo el programa.

## Esquema del circuito (con Fritzing):

<img width="1355" height="962" alt="cap_esquema_empotrados_practica3" src="https://github.com/user-attachments/assets/545f8b44-b82f-4ad4-a416-063e3f367774" />

Cada color del cable representa un sensor.  
El sensor de temperatura está colocado distinto en el circuito real por falta de cables.

## Videos del montaje y su funcionamiento:

(El modo arranque se ve en los demás modos).

### -Modo servicio:

https://github.com/user-attachments/assets/ecd14f1d-a8a1-4b4f-b513-dcffa7cab693

### -Modo admin:
**Navegamos por el menú y vemos las funciones simples:**

https://github.com/user-attachments/assets/c72e0eaf-4797-4353-ba4b-c890bdeb595c 

**Modificamos los precios de los productos:**

https://github.com/user-attachments/assets/82b3e9b1-4954-478b-952a-3f01f480740a

Observamos que después de editar los precios, salen modificados en el catálogo al volver al modo servicio.

### -Prueba con el Watchdog:

https://github.com/user-attachments/assets/42d172bd-e657-4a76-a225-714c3d2fc7b2

Vemos que cuando activamos el modo admin y pulsamos cualquier selección, el programa se queda congelado por poquito, pero gracias al watchdog, se reinicia. 
