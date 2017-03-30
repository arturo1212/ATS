
/* 
 *  TransferATS.ino
 *  Descripcion: Programa que monitorea el estado de la red y controla el comportamiento
 *  de un generador y la red de forma automatica. Tambien permite controlar la red de forma
 *  manual.
 *  
 *  Ultima actualizacion : 6/10/2016
 *  
 *  Autores: 
 *    Arturo Antonio Toro Ruiz.
 */
#include <LiquidCrystal.h>

// CONSTANTES.
// NOTA: M1 conecta la red, M2 enciende el generador, M3 conecta el generador.
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);        // Display.
const int espRX = 8, espTX = 9;             // ESP8266 Serial.
const int M1 = 10, M2 = 11, M3 = 12;        // Contactores.
const int interruptor = A6, modoVolt = A7;  // Interruptores.
 
// Parametros fijos.
const float VOLTAJEref = 5;                 // Voltaje de referencia.
const int prec = 5;                         // Precision para Frecuencia      

// VARIABLES GLOBALES.
float encendido = 0 ;           // Voltaje pico, tiempo de encendido.

//--------------------------- ESCRIBIR ---------------------------//
void escribir(String msj){
  lcd.clear();              // Limpiar Pantalla.
  lcd.print(msj);           // Mostrar mensaje.
}

//----------------------------  LEER   ---------------------------//
// Preguntar si esta lectura es con respecto al neutro. DUDA
float leer(int fase){
  /* Funcion que lee el voltaje de una fase*/
  int valor = analogRead(fase);               // Leer el potenciometro
  return(valor * ( VOLTAJEref / 1023.0));     // Calcular Voltaje del potenciometro     
}

// ---------------------------  RMS  ----------------------------//
float voltajeRMS(float voltaje){
  /* Funcion que, dado un voltaje, devuelve el voltaje RMS.*/
  float result = voltaje/sqrt(2);             // Formula de Voltaje RMS.
  return(result);                             // Entregar el resultado.
}


///////////////////////////////////  EncenderRed  //////////////////////////////////////
int encenderRed(){
  apagarGen();                     // Apagar el Generador.
  digitalWrite(M1,HIGH);           // Conectar la Red.
}

///////////////////////////////////  EncenderGen  //////////////////////////////////////
int encenderGen(){
  digitalWrite(M1,LOW);            // Desconectamos la red
  delay(3000);                     // Esperamos 3 segundos.
  digitalWrite(M3,HIGH);           // Conectamos el Generador.
}

///////////////////////////////////  Apagar Gen  //////////////////////////////////////
int apagarGen(){
  digitalWrite(M3,LOW);   // Desconectar el generador.  
  delay(3000);            // Esperar 3 segundos.
  digitalWrite(M2,LOW);   // Apagar el generador.
}

//////////////////////////////////  Punto Extremo   ////////////////////////////////////
float puntoMaximo(int fase){
  bool subiendo,bajando;          
  float valorInicial = leer(fase);
  float valorActual = valorInicial;
  int timeout = 0;

  // Buscar el primer cambio de valor.
  while(valorActual == valorInicial){
    valorActual = leer(fase);
    // Si el valor no cambia despues de 5 iteraciones,
    // asumimos que el voltaje no esta variando.
    if (timeout >= 5){
      return -1;
    }
    timeout += 1;
  }

  // Determinamos si vamos subiendo o bajando.
  if (valorActual>valorInicial){
    subiendo = true;
  }
  else{
    subiendo = false;
  }
  // Si la lectura llega al tope entonces esta leyendo un voltaje muy alto.
  while(valorActual<5){
    valorInicial = valorActual;
    valorActual = leer(fase);
    // Si iba subiendo y acaba de empezar a bajar, encontramos un maximo.
    if (subiendo == true and valorActual < valorInicial){
      return(valorInicial);
    }
    // Si iba bajando y acaba de empezar a subir, encontramos un minimo.
    else if(subiendo == false && valorActual > valorInicial){
      subiendo = true;
    }
  }
  return -1;
}

//----------------------------------  FRECUENCIA   ---------------------------------//
float frecuencia(int fase){
  /* Funcion que, dada una fase, devuelve la frecuencia. */
  // Variables
  float promedio = 0, frec, dif;
  unsigned long tiempoTotal = 0, tiempoInicio = 0, tiempoMax = 0;
  // Calcular maximo y guardar tiempo.
  puntoMaximo(fase);
  tiempoInicio = micros();

  //-------------  Calcular tiempo entre Maximos  -------------//  
  for(int i = 0; i < prec ; i++){   // Numero de muestras a tomar.
    puntoMaximo(fase);              // Buscar punto maximo.
    tiempoMax = (micros());         // Guardar tiempo actual.
    dif = (tiempoMax-tiempoInicio); // Tiempo transcurrido entre maximos.
    tiempoTotal+= dif;              // Sumar tiempo.
    tiempoInicio = tiempoMax;       // Actualizar tiempo inicial.    
  }
  //------------------  Calcular frecuencia  ------------------//
  promedio = ((float)tiempoTotal/1000000.0)/(float)prec;
  frec = 1.0/promedio;
  return frec;
}


///////////////////////////////////  Secuencia    //////////////////////////////////////
int secuencia(int R, int S, int T){
  unsigned long inicio = 0, aux = 0, dif = 0;
  int error = 3000, valor = 5555;

  // Marcar punto de inicio.
  puntoMaximo(R);         // Punto Maximo de R
  inicio = micros();      // Marcar el tiempo de inicio.
  
  // Tiempo entre R y S 
  puntoMaximo(S);         // Hallamos el punto Maximo de S
  aux = micros();         // Guardo el tiempo actual.
  dif = aux - inicio;     // Tiempo entre fases.
  inicio = aux;           // Actualizar el inicio.
  if (not((valor - error <= dif) && (dif <= valor + error))) {
    return -1;
  }

  // Tiempo entre S y T
  puntoMaximo(T);         // Hallar el punto maximo en T.
  aux = micros();         // Guardo el tiempo actual.
  dif = aux - inicio;     // Tiempo entre fases.
  inicio = aux;           // Actualizar el inicio.
  if (not((valor - error <= dif) && (dif <= valor + error))) {
    return -1;
  }

  // Tiempo entre T y R
  puntoMaximo(R);            // Hallamos punto maximo de R.
  dif = micros() - inicio;   // Tiempo entre fases.
  if (not((valor - error <= dif) && (dif <= valor + error))) {
    return -1;
  }  
  return 0;
}

/////////////////////////////////////  Chequeos   //////////////////////////////////////
int check(int R,int S, int T){
  float vR = 0,vS = 0, vT = 0, frec = 0;
  String mensajeS;

  // Red o generador.
  if (R == A0){
    lcd.setCursor(0, 1);    
  }
  else{
    lcd.setCursor(0, 0);
  }

  // Leer voltajes
  vR = puntoMaximo(R);
  vS = puntoMaximo(S);
  vT = puntoMaximo(T);
  
  // Verificaciones de voltajes  
  if(not(vR <= 4 && vR>=2)){
    escribir("S ERR: "+String(vR));
    return -1;
  }
  if(not(vS <= 4 && vS>=2)){
    escribir("S ERR: "+String(vS));
    return -1;
  }  
  if(not(vT <= 4 && vT>=2)){
    escribir("S ERR: "+String(vR));
    return -1;
  }
  mensajeS = String(vR)+" "+String(vS)+" "+String(vT)+ " ";
  escribir("Voltaje Ok");
  // Verificar frecuencia
  frec = frecuencia(R);
  if (not(frec<=62 && frec>=58)){
    escribir("freq ERR "+String(frec));
    return -2;
  }
  mensajeS += frec;
  // Verificar secuencia
  if (secuencia(R,S,T) != 0){
    escribir("sec ERR");
    return -3;
  }
  mensajeS += " Ok";
  escribir(mensajeS);
  return(0);
}


void setup() {Serial.begin(9600);
  lcd.begin(16, 2);  // Inicializar pantalla LCD.
  apagarGen();      // Apagar generador al iniciar por seguridad.
}

///////////////////////////////  PROGRAMA PRINCIPAL  //////////////////////////////////
void loop() {
  float manual = leer(interruptor);
  if(manual < 1){                             // Si esta en modo AUTOMATICO.
    if(check(A0,A1,A2) == 0){                   // Si la red esta bien. encender.
      encenderRed();                           
    }
    else{                                     // Si la red no esta bien.              
      digitalWrite(M2,HIGH);                  // Encender el Generador.
      delay(2000);                            // Esperar dos segundos y verificar.
      if (check(A3,A4,A5) == 0){              // Hacer los chequeos sobre el generador.
        encenderGen();                        // Si todo sale bien, conectar generador.
        encendido = millis();                 // Marcar tiempo de encendido
        while(millis()-encendido < 60000*10){ // Monitorear el generador durante 10 minutos
           if (check(A3,A4,A5) == -1){        // Si ocurre un error,
              apagarGen();                    // Apagar el generador.
              break;                          // Salir del ciclo.
           }
        }
        // Luego de que transcurrieron 10 minutos,
        while(check(A0,A1,A2) == -1){         // Verificar la red hasta que se arregle.
          if (check(A3,A4,A5) == -1){         // Si falla el generador,
            apagarGen();                      //  Apagar el generador.
            break;
          }
        }
      }
    }
  }                 //  Salir del ciclo.
  else{}            // Si esta en modo manual, no hacer nada.
  
  /*
  Serial.println(frecuencia(A2));
  float a1 = VpGlob;
  float a2 = leer(A1);
  //Serial.print(" Diferencia: ");
  //Serial.println(voltajeRMS(a1+a2));
  */
}
