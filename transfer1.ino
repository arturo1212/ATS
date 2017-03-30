
/* 
 *  TransferATS.ino
 *  Descripcion: Programa que monitorea el estado de la red y controla el comportamiento
 *  de un generador y la red de forma automatica. Tambien permite controlar la red de forma
 *  manual.
 *  
 *  Ultima actualizacion : 25/07/2016
 *  
 *  Autores: 
 *    Maria Victoria Jorge Mauriello.
 *    Arturo Antonio Toro Ruiz.
 */
 
// CONSTANTES.
const float VOLTAJEref = 5;                   // Voltaje de referencia.
const int prec = 1, M1 = 6, M3 = 7, M2 = 5, interruptor = A6;

// VARIABLES GLOBALES.
float VpGlob = 0, encendido = 0 ;

//////////////////////////////////////  LEER   /////////////////////////////////////////
float leer(int fase){
  /* Funcion que lee el voltaje de una fase*/
  int valor = analogRead(fase);               // Leer el potenciometro
  return(valor * ( VOLTAJEref / 1023.0));     // Calcular Voltaje del potenciometro     
}

///////////////////////////////////////  RMS   /////////////////////////////////////////
float voltajeRMS(float voltaje){
  /* Funcion que, dado un voltaje, devuelve el voltaje RMS.*/
  float result = voltaje/sqrt(2); // Formula de Voltaje RMS.
  return(result);                 // Entregar el resultado.
}

///////////////////////////////////  Frecuencia   //////////////////////////////////////
float frecuencia(int fase){
  /* Funcion que, dada una fase, devuelve la frecuencia. */
  // Declaracion de variables
  float promedio=0,frec, Vp, dif,actual = 0;
  unsigned long tiempoTotal=0, tiempoInicio=0,tiempoMax = 0;
  
  //----------------Calcular el primer Maximo.-----------------//
  while(actual<=1){               // Ciclo para saber que pasamos por el minimo
    actual = puntoMaximo(fase);}
    
  while(actual>=3){               // Obtener el maximo
    actual = puntoMaximo(fase);
    if(actual>=Vp){               // Si el actual es mayor, actualizar.
      Vp = actual;                
      tiempoMax = micros();}}
  tiempoInicio = tiempoMax; // Tiempo de referencia inicial.

  //-------------  Calcular tiempo entre Maximos  -------------//  
  for(int i = 0; i < prec ; i++){   // Numero de muestras a tomar.
    Vp = 0;                         // Reinicializamos maximo en cero para cada muestra.
    actual = puntoMaximo(fase);     // Valor inicial.
    
    while(actual<=1){               // Ciclo para saber que pasamos por el minimo
      actual = puntoMaximo(fase);}  // Actualizar valor.
      
    while(actual>=3){              // Obtener el maximo.
      actual = puntoMaximo(fase);  // Actualizar valor.
      if(actual>=Vp){              // Si el actual es mayor, actualizar.
        Vp = actual;               // Actualizar valor.
        tiempoMax = (micros());    // Actualizar tiempo.
      }  
    }
    VpGlob = Vp;                    // Almacenar el voltaje maximo.
    dif = (tiempoMax-tiempoInicio); // tiempo transcurrido entre maximos.
    tiempoTotal+= dif;              
    tiempoInicio = tiempoMax;    
  }
  //-------------  Calcular promedio entre Maximos  -------------// 
  promedio = ((float)tiempoTotal/1000000.0)/(float)prec;
  frec = 1.0/promedio;
  Serial.print(" Frecuencia: ");
  Serial.print(frec);
  Serial.println("Hz");
  return (1.0/promedio);
}

///////////////////////////////////  Secuencia    //////////////////////////////////////
int secuencia(int R, int S, int T){
  unsigned long inicio=0,S_Tt=0,R_St=0;
  float promedioRS= 0, promedioST = 0, frecRS = 0,frecST = 0,S_T;
  float R_S = puntoMaximo(R); // Hallamos punto maximo de R
  R_S = puntoMaximo(R);     // Punto Maximo de R
  if(R_S <= 1){             // Si esta bajando,
    R_S = puntoMaximo(R);   // Buscar cuando suba
  }
  inicio = micros();        // marcar el tiempo de inicio.
  
  R_S = puntoMaximo(S);     // Hallamos el punto Maximo de S
  if(R_S <= 1){             // Si va bajando, buscar el que sube
    R_S = puntoMaximo(S); 
  }  
  R_St = micros() - inicio; // Ver el tiempo que paso desde R hasta S
  inicio = micros();        // Actualizar el inicio.
  
  S_T = puntoMaximo(T);     
  if(S_T <= 1){
    S_T = puntoMaximo(T);
  }
  S_Tt = micros() - inicio;

  promedioST = ((float)S_Tt/1000000.0);
  frecST = 1.0/promedioST;
  promedioRS = ((float)R_St/1000000.0);
  frecRS = 1.0/promedioRS;
  /*Serial.println("FRECUENCIAS: ");
  Serial.println(frecST);
  Serial.println(frecRS);*/
  if(frecRS>=19.3 and frecRS<=20.7 and frecST>=19.3 and frecST<=20.7){
    Serial.println("Secuencia OK");
    return(0);
  }
  else{
    Serial.println("Secuencia FALLO");
    return(-1);
  }
}

/////////////////////////////////////  Chequeos   //////////////////////////////////////
int check(int R,int S, int T){
  float frecRed = 0;
  // VERIFICAR LOS RANGOS CON EL PROFESOR.
  frecRed = frecuencia(R);              // Calculamos la frecuencia de la fase R.
  if( VpGlob <= 4 and VpGlob>=2){       // Si el voltaje es correcto, todo bien.
    Serial.println("Voltaje OK"); 
    if (frecRed<=62 and frecRed>=59){   // 60.3   59.3
      Serial.println("Frecuencia OK");  // Si la frecuencia esta en el rango, todo bien.
      if (secuencia(R,S,T) == 0){       // Si la secuencia esta correcta, terminar el check.
        Serial.println("CHECK OK");
        return(0);
      }
      else{                             // Si la secuencia falla. error
        Serial.println("Falla de Secuencia");
        return(-1);
      }
    }    
    else{                               // Si la frecuencia no esta en el rango, error.
      Serial.println(frecRed);
      Serial.print("Problema de Frecuencia.");
      return(-1);
    }
  }
  Serial.println(VpGlob);               // Si el voltaje no esta en el rango, error.
  Serial.println("Falla de voltaje.");
  return(-1);
}

//////////////////////////////////  Punto Extremo   ////////////////////////////////////
float puntoMaximo(int fase){
  bool subiendo,bajando;
  float valorInicial = leer(fase);
  float valorActual = valorInicial;
  while(valorInicial == valorActual){
    valorActual = leer(fase);}
  if (valorActual>valorInicial){
    subiendo = true;}
  else{
    bajando = true;}
  while(subiendo == true || bajando == true){
    valorInicial = valorActual;
    while(valorInicial == valorActual){
      valorActual = leer(fase);}
    // Si iba subiendo y acaba de empezar a bajar, encontramos un maximo.
    if (subiendo == true && valorActual < valorInicial){
      if (valorActual>3){
        bajando = true;
        subiendo = false;
        return(valorInicial);}}
    // Si iba bajando y acaba de empezar a subir, encontramos un minimo.
    else if(bajando == true && valorActual > valorInicial){
      if (valorActual <= 1){
        subiendo = true;
        bajando = false;
        return(valorInicial);}}}}

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

void setup() {Serial.begin(9600);}

///////////////////////////////  PROGRAMA PRINCIPAL  //////////////////////////////////
void loop() {
  float manual = leer(interruptor);
  if(manual < 1){                           // Si esta en modo AUTOMATICO.
    Serial.println("Automatico");
    if(check(A0,A1,A2)==0){encenderRed();}  // Si la red esta bien. encender.   
    else{                                   // Si la red no esta bien.              
      digitalWrite(M2,HIGH);                  // Encender el Generador.
      delay(2000);                            // Esperar dos segundos y verificar.
      if (check(A3,A4,A5 == 0)){              // Hacer los chequeos sobre el generador.
        encenderGen();                        // Si todo sale bien, conectar generador.
        encendido = millis();                 // Marcar tiempo de encendido
        while(millis()-encendido < 60000*10){ // Monitorear el generador durante 10 minutos
           if (check(A3,A4,A5 == -1)){        // Si ocurre un error,
              apagarGen();                      // Apagar el generador.
              break;                            // Salir del ciclo.
           }
        }
        // Luego de que transcurrieron 10 minutos,
        while(check(A0,A1,A2) == -1){     // Verificar la red hasta que se arregle.
           if (check(A3,A4,A5) == -1){    // Si falla el generador,
              apagarGen();                //  Apagar el generador.
              break;}}}}}                 //  Salir del ciclo.
  else{}    // Si esta en modo manual, no hacer nada.
}
