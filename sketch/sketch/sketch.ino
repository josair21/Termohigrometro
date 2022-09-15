#include <Arduino.h>
#include <U8g2lib.h>
#include <LowPower.h>
#include "DHT.h"
#include "RTClib.h"
#include <EEPROM.h>
//#include <SD.h>

int DHTPIN = 11;
int DHTTYPE = DHT22 ;
unsigned long t=0;
float temperatura, humedad;
int minuto=0, hora = 0, minuto_anterior=0, random_p=0, random_q=0, seleccion = 1, periodo_leer_datos=5, modo = 1, pantalla=0, segundo=100, segundo_anterior_leer_datos=0;
int periodo_leer_bateria=10, minuto_anterior_modo_1=0, periodo_modo_1 = EEPROM.read(1), ultimo_modo = 1, configuracion = 0, tiempo_standby = EEPROM.read(2), dia, mes, ano, configurando = 0;
int lectura_bateria[] = {2, 4, 8, 3, 6}, segundo_anterior_leer_bateria, interrupt_delay = 250;
float eq_a_hr, eq_b_hr, eq_a_t, eq_b_t, ultimo_hr;
int aplicar_calibracion = 0;
bool eq_b_hr_negativo = EEPROM.read(19), eq_b_t_negativo = EEPROM.read(20);
bool menu = 0, ultimo_menu=0, ultima_interrupcion = 0, aplicar_configuracion_1 = 0, aplicar_configuracion_2 = 0;
bool grabar_valor_periodo_modo_1 = 0, grabar_valor_tiempo_standby = 0, regresar = 0, cargando;
char daysOfTheWeek[7][4] = {"Do.", "Lu.", "Ma.", "Mi.", "Ju.", "Vi.", "Sa."};

//File myFile;
RTC_DS3231 rtc;
DHT dht(DHTPIN, DHTTYPE);
DateTime now;
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
//U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display
void setup() {
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(9, INPUT);
  pinMode(A2, INPUT);
  Serial.begin(9600);
  Serial.println("INICIADO");
  dht.begin();
  u8g2.begin();  
  rtc.begin();
  //u8g2.enableUTF8Print();
  randomSeed(analogRead(1));
  leer_datos();
  delay(5000);
  EEPROM.get(3,eq_a_hr);
  EEPROM.get(7,eq_b_hr);
  EEPROM.get(11,eq_a_t);
  EEPROM.get(15,eq_b_t);
  attachInterrupt(0, boton_seleccion_configurando, RISING);
  attachInterrupt(1, boton_menu_configurando, RISING);
  //for(int i=0;i<5;i++){
  //  lectura_bateria[i] = analogRead(0);
  //}
  
  modo = 1;
  configuracion = 0;
  configurando = 0;
  
}
/////////////////////////////////////////////////
void loop() {
  leer_datos();
  t++;
  if(t>(tiempo_standby*15+2) & tiempo_standby != 0) u8g2.setPowerSave(1);
  else u8g2.setPowerSave(0);
  if(modo == 1){
    pantalla_principal();
    modo_1();
  }
  else if(modo == 2){
    modo_2();
  }
  if(t>2 && ultima_interrupcion == 0)LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
  ultima_interrupcion = 0;
}
///////////////////////////////////////////////
void modo_1(){
  if(abs(minuto-minuto_anterior_modo_1)>=periodo_modo_1 & abs(minuto-minuto_anterior_modo_1)<=60-periodo_modo_1){
    if(hora<10){Serial.print("0");Serial.print(hora);}
    else Serial.print(hora);
    Serial.print(':');
    if(minuto<10){Serial.print("0");Serial.print(minuto);}
    else Serial.print(minuto);
    Serial.print(':');
    if(segundo<10){Serial.print("0");Serial.print(segundo);}
    else Serial.print(segundo);
    Serial.print(",");
    Serial.print(temperatura,1);
    Serial.print(",");
    Serial.println(humedad,1);
    minuto_anterior_modo_1 = minuto;
    delay(200);
  }
}
void modo_2(){
  while(digitalRead(2)==1){}
  while(1){
    if(configurando == 0 & aplicar_configuracion_1 == 0 & aplicar_configuracion_2 == 0){
      leer_datos();
    }
    if(configuracion == 1){
      /*now = rtc.now();
      hora = now.hour();
      minuto = now.minute();
      segundo = now.second();*/
      if(aplicar_configuracion_1 == 1){
        rtc.adjust(DateTime(ano,mes,dia,hora,minuto,segundo));
        aplicar_configuracion_1 = 0;
      }
        u8g2.firstPage();
      do{
        u8g2.setFont(u8g2_font_lubR08_tr);
        u8g2.setCursor(60,16);
        u8g2.print("  A. Fecha");
        if(configurando == 1){
          u8g2.drawLine(59,58,71,58);
        }
        else if(configurando == 2){
          u8g2.drawLine(79,58,91,58);
        }
        else if(configurando == 3){
          u8g2.drawLine(113,58,125,58);
        }
        u8g2.setCursor(59,56);
        if(dia < 10){u8g2.print("0"); u8g2.print(dia);}
        else u8g2.print(dia);
        u8g2.print("/");
        if(mes < 10){ u8g2.print("0"); u8g2.print(mes);}
        else u8g2.print(mes);
        u8g2.print("/");
        u8g2.print(ano);
        u8g2.drawCircle(30,32,25,U8G2_DRAW_ALL);
        u8g2.setFont(u8g2_font_open_iconic_app_4x_t);
        u8g2.drawGlyph(17,48,66);
        u8g2.setFont(u8g2_font_open_iconic_app_2x_t);
        u8g2.drawGlyph(64,40,69);
        u8g2.drawGlyph(87,40,72);
        u8g2.drawGlyph(110,40,71);
        
      }while(u8g2.nextPage());  
    }
    else if(configuracion == 2){
      /*now = rtc.now();
      dia = now.day();
      mes = now.month();
      ano = now.year();*/
      if(aplicar_configuracion_2 == 1){
        rtc.adjust(DateTime(ano,mes,dia,hora,minuto,segundo));
        aplicar_configuracion_2 = 0;
      }
        u8g2.firstPage();
      do{
        u8g2.setFont(u8g2_font_lubR08_tr);
        u8g2.setCursor(60,16);
        u8g2.print("   A. Hora");
        if(configurando == 1){
          u8g2.drawLine(59+7,58,71+7,58);
        }
        else if(configurando == 2){
          u8g2.drawLine(79+4,58,91+4,58);
        }
        else if(configurando == 3){
          u8g2.drawLine(113-13,58,125-13,58);
        }
        u8g2.setCursor(66,56);
        if(hora < 10){u8g2.print("0"); u8g2.print(hora);}
        else u8g2.print(hora);
        u8g2.print(":");
        if(minuto < 10){ u8g2.print("0"); u8g2.print(minuto);}
        else u8g2.print(minuto);
        u8g2.print(":");
        if(segundo < 10){ u8g2.print("0"); u8g2.print(segundo);}
        else u8g2.print(segundo);
        u8g2.drawCircle(30,32,25,U8G2_DRAW_ALL);
        u8g2.setFont(u8g2_font_open_iconic_app_4x_t);
        u8g2.drawGlyph(15,48,69);
        u8g2.setFont(u8g2_font_open_iconic_app_2x_t);
        u8g2.drawGlyph(64,40,72);
        u8g2.drawGlyph(87,40,71);
        u8g2.setFont(u8g2_font_open_iconic_embedded_2x_t);
        u8g2.drawGlyph(110,40,72);
      }while(u8g2.nextPage());  
    }
    else if(configuracion == 3){
      if(grabar_valor_periodo_modo_1 == 1){
        EEPROM.update(1, periodo_modo_1);
        grabar_valor_periodo_modo_1 = 0;
      }
      u8g2.firstPage();
      do{
        u8g2.setFont(u8g2_font_lubR08_tr);
        u8g2.setCursor(60,16);
        u8g2.print("A. Periodo");
        if(configurando == 1){
          u8g2.drawLine(60,58,72,58);
        }
        u8g2.setCursor(60,56);
        if(periodo_modo_1 < 10){u8g2.print("0");u8g2.print(periodo_modo_1);}
        else u8g2.print(periodo_modo_1);
        u8g2.print(" minutos");
        u8g2.drawCircle(30,32,25,U8G2_DRAW_ALL);
        u8g2.setFont(u8g2_font_open_iconic_app_4x_t);
        u8g2.drawGlyph(17,48,72);
        u8g2.setFont(u8g2_font_open_iconic_app_2x_t);
        u8g2.drawGlyph(64,40,71);
        u8g2.setFont(u8g2_font_open_iconic_embedded_2x_t);
        u8g2.drawGlyph(87,40,72);
        u8g2.drawGlyph(110,40,68);
      }while(u8g2.nextPage());  
    }
    else if(configuracion == 4){
      if(grabar_valor_tiempo_standby == 1){
        EEPROM.update(2, tiempo_standby);
        grabar_valor_tiempo_standby = 0;
      }
        u8g2.firstPage();
      do{
        u8g2.setFont(u8g2_font_lubR08_tr);
        u8g2.setCursor(60,16);
        u8g2.print("A. Standby");
        if(configurando == 1){
          u8g2.drawLine(60,58,72,58); 
        }
        u8g2.setCursor(60,56);
        if(tiempo_standby < 10){u8g2.print("0");u8g2.print(tiempo_standby);}
        else u8g2.print(tiempo_standby);
        u8g2.print(" minutos");
        u8g2.drawCircle(30,32,25,U8G2_DRAW_ALL);
        u8g2.setFont(u8g2_font_open_iconic_app_4x_t);
        u8g2.drawGlyph(15,48,71);
        u8g2.setFont(u8g2_font_open_iconic_embedded_2x_t);
        u8g2.drawGlyph(64,40,72);
        u8g2.drawGlyph(87,40,68);
        u8g2.setFont(u8g2_font_open_iconic_app_2x_t);
        u8g2.drawGlyph(110,40,66);
      }while(u8g2.nextPage());  
    }
    else if(configuracion == 5){
      if(configurando == 0){
        u8g2.firstPage();
        do{
          u8g2.setFont(u8g2_font_lubR08_tr);
          u8g2.setCursor(60,16);
          u8g2.print("Calibracion");
          u8g2.drawCircle(30,32,25,U8G2_DRAW_ALL);
          u8g2.setFont(u8g2_font_open_iconic_embedded_4x_t);
          u8g2.drawGlyph(15,48,72);
          u8g2.setFont(u8g2_font_open_iconic_embedded_2x_t);
          u8g2.drawGlyph(64,40,68);
          u8g2.setFont(u8g2_font_open_iconic_app_2x_t);
          u8g2.drawGlyph(87,40,66);
          u8g2.drawGlyph(110,40,69);
        }while(u8g2.nextPage());  
      }
      else{
        u8g2.firstPage();
        do{
          u8g2.setFont(u8g2_font_lubR08_tr);
          u8g2.setCursor(0,32-2);
          u8g2.print("Eq. HR");
          u8g2.setCursor(0,64-2);
          u8g2.print("Eq. Te.");
          
          u8g2.setFont(u8g2_font_profont12_mn);
          
          u8g2.setCursor(44,32-2);
          u8g2.print(eq_a_hr,4);
          u8g2.print("*");
          if(eq_b_hr_negativo == 0) u8g2.print("+");
          else u8g2.print("-");
          u8g2.print(eq_b_hr,4);
          
          u8g2.setCursor(44,64-2);
          u8g2.print(eq_a_t,4);
          u8g2.print("*");
          if(eq_b_t_negativo ==0) u8g2.print("+");
          else u8g2.print("-");
          u8g2.print(eq_b_t,4);

          
           if(configurando==1){
            u8g2.drawLine(44,31,48,31);
          }
          else if(configurando==2){
            u8g2.drawLine(56,31,60,31);
          }
          else if(configurando==3){
            u8g2.drawLine(62,31,66,31);
          }
          else if(configurando==4){
            u8g2.drawLine(68,31,72,31);
          }
          else if(configurando==5){
            u8g2.drawLine(74,31,78,31);
          }
          
          else if(configurando==6){
            u8g2.drawLine(86,31,90,31);
          }
          else if(configurando==7){
            u8g2.drawLine(92,31,96,31);
          }
          else if(configurando==8){
            u8g2.drawLine(104,31,108,31);
          }
          else if(configurando==9){
            u8g2.drawLine(110,31,114,31);
          }
          else if(configurando==10){
            u8g2.drawLine(116,31,120,31);
          }
          else if(configurando==11){
            u8g2.drawLine(122,31,126,31);
          }
          
          
          else if(configurando==12){
            u8g2.drawLine(44,63,48,63);
          }
          else if(configurando==13){
            u8g2.drawLine(56,63,60,63);
          }
          else if(configurando==14){
            u8g2.drawLine(62,63,66,63);
          }
          else if(configurando==15){
            u8g2.drawLine(68,63,72,63);
          }
          else if(configurando==16){
            u8g2.drawLine(74,63,78,63);
          }
          
          else if(configurando==17){
            u8g2.drawLine(86,63,90,63);
          }
          else if(configurando==18){
            u8g2.drawLine(92,63,96,63);
          }
          else if(configurando==19){
            u8g2.drawLine(104,63,108,63);
          }
          else if(configurando==20){
            u8g2.drawLine(110,63,114,63);
          }
          else if(configurando==21){
            u8g2.drawLine(116,63,120,63);
          }
          else if(configurando==22){
            u8g2.drawLine(122,63,126,63);
          }

        }while(u8g2.nextPage());  
      }
    }
    else if(configuracion == 6){
      u8g2.firstPage();
      do{
        u8g2.setFont(u8g2_font_lubR08_tr);
        u8g2.setCursor(60,16);
        u8g2.print("    Incio");
        u8g2.drawCircle(30,32,25,U8G2_DRAW_ALL);
        u8g2.setFont(u8g2_font_open_iconic_embedded_4x_t);
        u8g2.drawGlyph(15,48,68);
        u8g2.setFont(u8g2_font_open_iconic_app_2x_t);
        u8g2.drawGlyph(64,40,66);
        u8g2.drawGlyph(87,40,69);
        u8g2.drawGlyph(110,40,72);
      }while(u8g2.nextPage());  
      if(regresar == 1){
        regresar = 0;
        modo = 1;
        t=0;
        configuracion = 0;
        configurando = 0;
        break;
      }
    }
  }
}
void boton_menu_configurando(){
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > interrupt_delay) 
  {
    detachInterrupt(0);
    detachInterrupt(1);
    if(tiempo_standby == 0) t = 0;
    if(modo == 1 & t<(tiempo_standby*15+2)){
      modo = 2;
      configuracion=1;
    }
    else if(configurando==0){
      configuracion++;
      if(configuracion>6)configuracion = 1;
    }
    else if(configuracion==1){
      if(configurando==1){
        dia++;
      if(dia>31) dia = 1;
      }
      if(configurando==2){
        mes++;
        if(mes>12) mes = 1;
      }
      if(configurando==3){
        ano++;
        if(ano>2032) ano = 2022;
      }
    }
    else if(configuracion==2){
      if(configurando==1){
        hora++;
        if(hora>24) hora = 0;
      }
      if(configurando==2){
        minuto++;
        if(minuto>59) minuto = 0;
      }
      if(configurando==3){
        segundo++;
        if(segundo>59) segundo = 0;
      }
    }
    else if(configuracion==3){
      if(configurando==1){
        periodo_modo_1++;
        if(periodo_modo_1 > 30) periodo_modo_1 = 1;
      }
    }
    else if(configuracion==4){
      if(configurando==1){
        tiempo_standby++;
        if(tiempo_standby > 30) tiempo_standby = 0;
      }
    }
    else if(configuracion==5){
      if(configurando==1){
        eq_a_hr = 1.0 + eq_a_hr;
      }
      else if(configurando==2){
        ultimo_hr = eq_a_hr;
        eq_a_hr = 0.1 + eq_a_hr;
        if(int(ultimo_hr) != int(eq_a_hr)) eq_a_hr = eq_a_hr-1.0;
      }
      else if(configurando==3){
        ultimo_hr = eq_a_hr;
        eq_a_hr = 0.01 + eq_a_hr;
        if(int(ultimo_hr*10) != int(eq_a_hr*10)) eq_a_hr = eq_a_hr-0.1;
      }
      else if(configurando==4){
        ultimo_hr = eq_a_hr;
        eq_a_hr = 0.001 + eq_a_hr;
        if(int(ultimo_hr*100) != int(eq_a_hr*100)) eq_a_hr = eq_a_hr-0.01;
      }
      else if(configurando==5){
        ultimo_hr = eq_a_hr;
        eq_a_hr = 0.0001 + eq_a_hr;
        if(int(ultimo_hr*1000) != int(eq_a_hr*1000)) eq_a_hr = eq_a_hr-0.001;
      }
      else if(configurando==6){
        if(eq_b_hr_negativo==0)eq_b_hr_negativo = 1;
        else eq_b_hr_negativo = 0;
      }
      else if(configurando==7){
        eq_b_hr = 1.0 + eq_b_hr;
      }
      else if(configurando==8){
        ultimo_hr = eq_b_hr;
        eq_b_hr = 0.1 + eq_b_hr;
        if(int(ultimo_hr) != int(eq_b_hr)) eq_b_hr = eq_b_hr-1.;
      }
      else if(configurando==9){
        ultimo_hr = eq_b_hr;
        eq_b_hr = 0.01 + eq_b_hr;
        if(int(ultimo_hr*10) != int(eq_b_hr*10)) eq_b_hr = eq_b_hr-0.1;
      }
      else if(configurando==10){
        ultimo_hr = eq_b_hr;
        eq_b_hr = 0.001 + eq_b_hr;
        if(int(ultimo_hr*100) != int(eq_b_hr*100)) eq_b_hr = eq_b_hr-0.01;
      }
      else if(configurando==11){
        ultimo_hr = eq_b_hr;
        eq_b_hr = 0.0001 + eq_b_hr;
        if(int(ultimo_hr*1000) != int(eq_b_hr*1000)) eq_b_hr = eq_b_hr-0.001;
      }
      else if(configurando==12){
        eq_a_t = 1.0 + eq_a_t;
      }
      else if(configurando==13){
        ultimo_hr = eq_a_t;
        eq_a_t = 0.1 + eq_a_t;
        if(int(ultimo_hr) != int(eq_a_t)) eq_a_t = eq_a_t-1.0;
      }
      else if(configurando==14){
        ultimo_hr = eq_a_t;
        eq_a_t = 0.01 + eq_a_t;
        if(int(ultimo_hr*10) != int(eq_a_t*10)) eq_a_t = eq_a_t-0.1;
      }
      else if(configurando==15){
        ultimo_hr = eq_a_t;
        eq_a_t = 0.001 + eq_a_t;
        if(int(ultimo_hr*100) != int(eq_a_t*100)) eq_a_t = eq_a_t-0.01;
      }
      else if(configurando==16){
        ultimo_hr = eq_a_t;
        eq_a_t = 0.0001 + eq_a_t;
        if(int(ultimo_hr*1000) != int(eq_a_t*1000)) eq_a_t = eq_a_t-0.001;
      }
      else if(configurando==17){
        if(eq_b_t_negativo == 0)eq_b_t_negativo = 1;
        else eq_b_t_negativo = 0;
      }
      else if(configurando==18){
        eq_b_t = 1.0 + eq_b_t;
      }
      else if(configurando==19){
        ultimo_hr = eq_b_t;
        eq_b_t = 0.1 + eq_b_t;
        if(int(ultimo_hr) != int(eq_b_t)) eq_b_t = eq_b_t-1.0;
      }
      else if(configurando==20){
        ultimo_hr = eq_b_t;
        eq_b_t = 0.01 + eq_b_t;
        if(int(ultimo_hr*10) != int(eq_b_t*10)) eq_b_t = eq_b_t-0.1;
      }
      else if(configurando==21){
        ultimo_hr = eq_b_t;
        eq_b_t = 0.001 + eq_b_t;
        if(int(ultimo_hr*100) != int(eq_b_t*100)) eq_b_t = eq_b_t-0.01;
      }
      else if(configurando==22){
        ultimo_hr = eq_b_t;
        eq_b_t = 0.0001 + eq_b_t;
        if(int(ultimo_hr*1000) != int(eq_b_t*1000)) eq_b_t = eq_b_t-0.001;
      }
      if(eq_a_hr>=10.0)eq_a_hr= eq_a_hr-10.0;
       if(eq_b_hr>=10.0)eq_b_hr= eq_b_hr-10.0;
       if(eq_a_t>=10.0)eq_a_t= eq_a_t-10.0;
       if(eq_b_t>=10.0)eq_b_t= eq_b_t-10.0;
    }
  attachInterrupt(0, boton_seleccion_configurando, RISING);
  attachInterrupt(1, boton_menu_configurando, RISING);
  }
  last_interrupt_time = interrupt_time;
}
void boton_seleccion_configurando(){
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > interrupt_delay) 
  {
    detachInterrupt(0);
    detachInterrupt(1);
    if(t>(tiempo_standby*15+2)) t=0;
    else if(configuracion!=6 & configuracion !=0){
      configurando++;
    }
    else if(configuracion==6){
      regresar = 1;
    }
    if(configurando>1 & configuracion==3){
      configurando = 0;
      grabar_valor_periodo_modo_1 = 1;
    }
    else if(configurando>1 & configuracion==4){
      configurando = 0;
      grabar_valor_tiempo_standby = 1;
    }
    else if(configurando>3 & configuracion==1){
      aplicar_configuracion_1 = 1;
      configurando = 0;
    }
    else if(configurando>3 & configuracion==2){
      aplicar_configuracion_2 = 1;
      configurando = 0;
    }
    else if(configurando>22 & configuracion==5){
      EEPROM.put(3,eq_a_hr);
      EEPROM.put(7,eq_b_hr);
      EEPROM.put(11,eq_a_t);
      EEPROM.put(15,eq_b_t);
      EEPROM.write(19,eq_b_hr_negativo);
      EEPROM.write(20,eq_b_t_negativo);
      configurando = 0;
      configuracion = 5;
      aplicar_calibracion = 1;
    }
    attachInterrupt(0, boton_seleccion_configurando, RISING);
    attachInterrupt(1, boton_menu_configurando, RISING);
  }
  last_interrupt_time = interrupt_time;
}
void leer_datos(){
  cargando = digitalRead(9);
  now = rtc.now();
  dia = now.day();
  mes = now.month();
  ano = now.year();
  hora = now.hour();
  minuto = now.minute();
  segundo = now.second();
  if(abs(segundo-segundo_anterior_leer_datos)>=periodo_leer_datos & abs(segundo-segundo_anterior_leer_datos)<=60-periodo_leer_datos){
    segundo_anterior_leer_datos = segundo;
    if(eq_b_hr_negativo==1) humedad = eq_a_hr*dht.readHumidity()-eq_b_hr;
    else humedad = eq_a_hr*dht.readHumidity()+eq_b_hr;
    if(eq_b_t_negativo==1)temperatura = eq_a_t*dht.readTemperature()-eq_b_t;
    else temperatura = eq_a_t*dht.readTemperature()+eq_b_t;
    cambiar_icono();
    //if(abs(segundo-segundo_anterior_leer_bateria)>=periodo_leer_bateria & abs(segundo-segundo_anterior_leer_bateria)<=60-periodo_leer_bateria){
      //segundo_anterior_leer_bateria = segundo;
     // Serial.println("CACA");
      for(int i=0;i<5;i++){
        if(i==4){
          lectura_bateria[i] = analogRead(A2);
          //Serial.println(lectura_bateria[i]);
        }
        else lectura_bateria[i] = lectura_bateria[i+1];
      }
  }
  if(cargando==1){
    for(int i=0;i<5;i++){
      lectura_bateria[i] = analogRead(A2);
    }
  }
}
void cambiar_icono(){
  random_p = random(0,5);
  if(random_p==4) random_q = 5;
  else random_q = random_p;
}
void revisar_bateria(){
  if(cargando==1){
      u8g2.setFont(u8g2_font_battery24_tr);
      u8g2.drawGlyph(108, 27, 63);
  }
  else{
      if(lectura_bateria[0]<875&lectura_bateria[1]<875&lectura_bateria[2]<875&lectura_bateria[3]<875&lectura_bateria[4]<875){
        u8g2.setFont(u8g2_font_battery24_tr);
        u8g2.drawGlyph(108, 27, 48);
      }
      else if(lectura_bateria[0]<895&lectura_bateria[1]<895&lectura_bateria[2]<895&lectura_bateria[3]<895&lectura_bateria[4]<895){
        u8g2.setFont(u8g2_font_battery24_tr);
        u8g2.drawGlyph(108, 27, 54);
      }
      else if(lectura_bateria[0]<922&lectura_bateria[1]<922&lectura_bateria[2]<922&lectura_bateria[3]<922&lectura_bateria[4]<922){
        u8g2.setFont(u8g2_font_battery24_tr);
        u8g2.drawGlyph(108, 27, 56);
      }
      else if(lectura_bateria[0]<960&lectura_bateria[1]<960&lectura_bateria[2]<960&lectura_bateria[3]<960&lectura_bateria[4]<960){
        u8g2.setFont(u8g2_font_battery24_tr);
        u8g2.drawGlyph(108, 27, 58);
      }
      else{
        u8g2.setFont(u8g2_font_battery24_tr);
        u8g2.drawGlyph(108, 27, 60);
      }
    }
  //if(digitalRead(4)==1)
}
void pantalla_principal(){
  u8g2.firstPage();
  do{
    u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
    u8g2.drawGlyph(0, 64, 64+random_q);
    u8g2.setFont(u8g2_font_m2icon_9_tf);
    u8g2.setCursor(0,13);
    for(int r=0;r<5;r++){
      if(random_p == r) u8g2.print("R");
      else u8g2.print("Q");
    }
    //Humedad
    u8g2.setFont(u8g2_font_luBIS12_tn);
    u8g2.setCursor(48+1, 29);
    u8g2.print(humedad,1);
    u8g2.setFont(u8g2_font_lubR08_tr);
    u8g2.setCursor(94,27);
    u8g2.print("%");
    //Temperatura
    u8g2.setFont(u8g2_font_logisoso32_tn);
    u8g2.setCursor(48+1, 64);
    u8g2.print(temperatura,1);
    u8g2.setCursor(120, 64-2);
    u8g2.setFont(u8g2_font_lubR08_tr);
    u8g2.setCursor(122,38);
    u8g2.print("o");
    revisar_bateria();
    //Fecha y hora
    u8g2.setFont(u8g2_font_lubR08_tr);
    u8g2.setCursor(48+5,13);
    u8g2.print(daysOfTheWeek[now.dayOfTheWeek()]);
    u8g2.print(" ");
    if(hora<10){u8g2.print("0");u8g2.print(hora);}
    else u8g2.print(hora);
    u8g2.print(":");
    if(minuto<10){u8g2.print("0");u8g2.print(minuto);}
    else u8g2.print(minuto);
  }
  while(u8g2.nextPage());
}
