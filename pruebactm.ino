#include <Ethernet.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include "HX711.h"  //Libreria para las celdas de carga

byte mac_addr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress server_addr(192,168,1,122);  // IP of the MySQL *server* here
char user[] = "root";              // MySQL user login username
char password[] = "SQL080498";        // MySQL user login password

// Sample query
char Insert_peso[] = "INSERT INTO arduino.peso (valor_peso,fecha) VALUES (%s,NOW())";
char Insert_gas[] = "INSERT INTO arduino.gas (valor_gas,fecha) VALUES (%s,NOW())";
char Insert_cortes[] = "INSERT INTO arduino.cortes (motivo,fecha) VALUES (%s,NOW())";

#define DOUT  A1  //Puerto analogico que usa la celda de carga
#define CLK  A0   //Puerto analogico que usa la celda de carga
HX711 balanza(DOUT, CLK);

float peso;    // esta variable almacenará el peso
float gas;
int vibracion;

EthernetClient client;
MySQL_Connection conn((Client *)&client);
 
void display(){ // esta funcion se encarga de registrar el peso e imprimirlo en el display, te retorna el peso actual
  // Todo referente a peso
  peso = balanza.get_units(20)*-1,3;
  Serial.print("Peso: ");
  Serial.print(peso);
  Serial.println(" kg");
}

void leer_gas(){
  gas=analogRead(A2);//CAMBIAR AL PUERTO QUE CORRESPONDA  
}
    
void setup() {
  Serial.begin(115200);
  while (!Serial); // wait for serial port to connect
  Ethernet.begin(mac_addr);
  Serial.println("Connecting...");
  if (conn.connect(server_addr, 3306, user, password)) {
    delay(1000);
    Serial.print("Conexion Realizada.");
    delay(200);
  }
  else{
    Serial.println("Conexion Fallida.");
  }
  Serial.print("Lectura del valor del ADC:  ");
  Serial.println(balanza.read());
  Serial.println("No ponga ningun  objeto sobre la balanza");
  Serial.println("Destarando...");
  Serial.println("...");
  balanza.set_scale(22516.66666666667); // Establecemos la escala
  balanza.tare(20);  //El peso actual es considerado Tara.
  Serial.println("Listo para pesar."); 
  }



void loop() {
  delay(2000);
  
 Serial.println("Recording data.");
  display();
  leer_gas();
  
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  char peticion_peso[4];
  char peticion_gas[4];
  char peticion_vib[1];
  char query[128];
  /*INSERTAR DATOS DEL PESO EN LA BD*/
  dtostrf(peso,2,2,peticion_peso);//float,int,decimal,string
  sprintf(query,Insert_peso,peticion_peso);
  cur_mem->execute(query);
  /*INSERTAR DATOS DEL GAS EN LA BD*/
  dtostrf(gas,2,2,peticion_gas);//float,int,decimal,string
  sprintf(query,Insert_gas,peticion_gas);
  cur_mem->execute(query);
  /*UNA VEZ MANDADOS LOS DATOS A LA BD, REVISAMOS QUE NO HAYA FUGAS.
  SI LOS SENSORES DETECTAN ALGO SOSPECHOSO SE PROCEDE A CORTAR EL SUMINISTRO DE GAS.
  */
  if(gas > 350 or vibracion != 0){
    /*
    
    AQUI VA EL CODIGO QUE APAGA LA VALVULA SOLENOIDE A TRAVES DEL RELE.
    
    */
    //ENVIAR LOS DATOS DEL PROBLEMA A LA BD
    if(gas >350){
      char motivo[] = "Se produce un corte por presencia de gas";
      sprintf(query,Insert_cortes,motivo);
      cur_mem->execute(query);
    }
    if(vibracion !=0 ){
      char motivo[] = "Se produce el corte por posible movimiento";
      sprintf(query,Insert_cortes,motivo);
      cur_mem->execute(query);   
    }
  }
 
  delete cur_mem;
}
