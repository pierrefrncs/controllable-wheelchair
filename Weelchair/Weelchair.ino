/*/
 * author : Pierre Francois
 */
#include <Servo.h>
#include <Wire.h>
#include "rgb_lcd.h"
#include <SoftwareSerial.h>
#include <IRremote.h>

#define JOYSTICK_X A1
#define JOYSTICK_Y A0

#define CAPTEUR_LUM A2

#define US_AVANT_IN 2//trigger = pulse
#define US_AVANT_OUT 3//echo = reception
#define US_ARRIERE_IN 5//echo
#define US_ARRIERE_OUT 4//trigger

#define BUZZER 8

#define VMaxD 0
#define VMinD 65
#define VMaxG 180
#define VMinG 96

//joystick value variables
int X, Y, rawX, rawY; //joystick positions

int valB, valA;

//control variables
int distance1, distance2;//US reading
boolean b;//seating verification
long echo_lecture;//

//Infra Red connecting pin 
int RECV_IR=4;

// buzzer pin
int buzz=8;

//US commands pins
int trigAV=US_AVANT_IN;
int echoAV=US_AVANT_OUT;
int trigAR=US_ARRIERE_IN;
int echoAR=US_ARRIERE_OUT;

//leds pin 
int ledAV2=6;
int ledAV1=7;
int ledAR1=10;
int ledAR2=11;

//servomoteur
Servo SGauche;
Servo SDroit;

//Infra Red
IRrecv irrecv(RECV_IR);

decode_results results;

//grove LCD screen 
rgb_lcd lcd;
const int colorR = 50;
const int colorG = 50;
const int colorB = 50;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.write("hello");
  
  irrecv.enableIRIn();
  
  SGauche.detach();  
  SDroit.detach();  

  //set UR AV  
  pinMode(trigAV, OUTPUT);
  digitalWrite(trigAV,LOW);
  pinMode(echoAV, INPUT);
  
  //set US AR
  pinMode(trigAR, OUTPUT);
  digitalWrite(trigAR,LOW);
  pinMode(echoAR, INPUT);

  pinMode(ledAV1, OUTPUT);
  digitalWrite(ledAV1, LOW);
  pinMode(ledAV2, OUTPUT);
  digitalWrite(ledAV2, LOW);
  
  pinMode(ledAR1, OUTPUT);
  digitalWrite(ledAR1, LOW);
  pinMode(ledAR2, OUTPUT);
  digitalWrite(ledAR2, LOW);
  
  
  //set buzzer
  pinMode(buzz, OUTPUT);
  analogWrite(buzz, 0);

  valB=8925;
  
  X=513;
  Y=513;
  lcd.clear();
}

void loop() {
  lcd.clear();
  if(erreur()==true){
    if(irrecv.decode(&results)){//1fois appuie touche    
        valA=results.value;
        Serial.print("valA = ");
        Serial.print(valA);
        irrecv.resume();
      }
      
    switch(valB){
      case 8925:
          Serial.print("Joystick : ");    
          deplacementJ();
      break;
  
      case -15811:
          Serial.print("Infrarouge : ");
          deplacementIR(valA);
      break;
    }
    if(valA==8925  || valA==-15811){
      valB=valA;
       switch(valB){
      case 8925:
          Serial.print("Joystick");
          lcd.print("JOYSTICK");
         
      break;
  
      case -15811:
          Serial.print("Infrarouge : ");
          lcd.print("INFRAROUGE");
          delay(1000);
         
      break;
    }   
    }
  }
delay(250);

}

//moving with the joystick
void deplacementJ(){
  rawX= analogRead(JOYSTICK_X)-X;
  //Serial.print(rawX);
  rawY= analogRead(JOYSTICK_Y)-Y;
  //Serial.println(rawY);
  lcd.setCursor(0,1);
   
//Avance recule tourne lent
  //Avance
  if((rawX>100 && rawX<510)){
    if (50>rawY && rawY>-50){
      avanceLent();
    }
    if (rawY<-50){     
      gaucheLent();
    }
    if (50<rawY){     
      droiteLent();
    }
  }

  if(rawX==510){
    if (50>rawY && rawY>-50){
    avance();
    }
  }

  //Recule
  if (rawX<-100){
    if (50>rawY && rawY>-50){
        recule();       
      }
  }

//tourne droite gauche
  if(rawY>100){
    if (50>rawX && rawX>-50){
      droite(); 
    }    
  }
  
  if(rawY<-100){
    if (50>rawX && rawX>-50){
      gauche();    
    } 
  }

//Arret
  if(50>rawY && rawY>-50 && 50>rawX && rawX>-50){
    arret();
   }
}

//moving with the controller
void deplacementIR(int valA){
  lcd.setCursor(0,1);
  switch(valA){
    case 765:// + button
      avance();
      break;  
        
    case -26521:// - button
      recule();
      break;

    case -8161://rwd button
      gauche();
      break;

   case -28561:// fwd button
      droite();
      break;

   case -22441://play button
      arret();
      break;

    default : 
      arret();
      break;
  }  
}

//usage condition testing
boolean erreur(){   
  int sensorValue = analogRead(CAPTEUR_LUM);
  lcd.setCursor(0,0);
  if(sensorValue > 500){
    Serial.println(" Le fauteuil est libre ");
    lcd.write("Fauteuil Libre");
    lcd.setRGB(255,0,0);
    analogWrite(buzz, 1000);
    arret();
    return false;
  } 
  lcd.write("Fauteuil Occupe"); 
  lcd.setRGB(0,255,0); 
  return true; 
 
  else{
    int temp=distanceObsAV();
    int temp2=distanceObsAR();
    Serial.print(" Le fauteil est occupe ");
    
    if (temp>25 && temp2>25){
      Serial.print(" pas d'obstacle ");
      lcd.write("TOUT OK ");
      lcd.setRGB(0,255,0);
      analogWrite(buzz, 0);
      return true;
    }
    
    else if (temp>15 && temp2>15){
      Serial.print(" obstacle proche attention ");
      lcd.write("OBS PROCHE");
      lcd.setRGB(255,165,0);
     beep(100); 
      return true;
    }
    else{
      Serial.print(" obstacle juste devant ");
      lcd.write("DANGER OBS");
      lcd.setRGB(255,0,0);
      //digitalWrite(buzz, HIGH);
      return false;
    }
  }   
}

//Front US
int distanceObsAV(){
  digitalWrite(trigAV, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigAV, LOW);
   
  echo_lecture = pulseIn(echoAV, HIGH);
 
  distance1 = echo_lecture / 58;
  Serial.print("Distance Front = ");
  Serial.print(distance1);
  return  40;
}

//Back US 
int distanceObsAR(){
  digitalWrite(trigAR, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigAR, LOW);
 
  echo_lecture = pulseIn(echoAR, HIGH);
 
  distance2 = echo_lecture / 58;
  Serial.print("Distance Back = ");
  Serial.print(distance2);
  return distance2;
}

//moving functions
//stop 
void arret(){
    Serial.println(" ARRET ");
    lcd.write("ARRET");

    digitalWrite(ledAV1,LOW);
    digitalWrite(ledAV2,LOW);
    digitalWrite(ledAR1,LOW);
    digitalWrite(ledAR2,LOW);
    
    SGauche.detach();
    SDroit.detach();  
}

//slow moving forward 
void avanceLent(){

    Serial.println(" AVANCE Lent ");
    lcd.write("Avance Lent");
    SGauche.attach(2);
    SDroit.attach(3);

    digitalWrite(ledAV1,HIGH);
    digitalWrite(ledAV2,HIGH); 
    digitalWrite(ledAR1,LOW);
    digitalWrite(ledAR2,LOW);
    

    SGauche.write(VMinG);
    SDroit.write(VMinD);
}

//moving forward
void avance(){

    Serial.println(" AVANCE ");
    lcd.write("AVANCE");
    SGauche.attach(2);
    SDroit.attach(3);

    digitalWrite(ledAV1,HIGH);
    digitalWrite(ledAV2,HIGH);
    
    digitalWrite(ledAR1,LOW);
    digitalWrite(ledAR2,LOW);
    
    SGauche.write(VMaxG);
    SDroit.write(VMaxD);
}

//moving backwards
void recule(){
    Serial.println(" RECULE ");
    lcd.write("RECULE");
    SGauche.attach(2);
    SDroit.attach(3);

    digitalWrite(ledAR1,HIGH);
    digitalWrite(ledAR2,HIGH);
    digitalWrite(ledAV1,LOW);
    digitalWrite(ledAV2,LOW);
    
    SGauche.write(65);
    SDroit.write(98);
}

//turning right slowly 
void droiteLent(){
    Serial.println(" DROITE Lent ");
    lcd.write("DROITE LENT");
    SGauche.attach(2);
       
    SGauche.write(VMinG);
}

//turning right
void droite(){
    Serial.println(" DROITE ");
    lcd.write("DROITE");
    SGauche.attach(2);
    SGauche.write(VMaxG);
}

//turning left slowly
void gaucheLent(){
    Serial.println(" GAUCHE  Lent");
    lcd.write("GAUCHE LENT");
    SDroit.attach(3);
      
    SDroit.write(VMinD);      
}

//turning left 
void gauche(){
    Serial.println(" GAUCHE ");
    lcd.write("GAUCHE");
    SDroit.attach(3);
 
    SDroit.write(VMaxD);      
}
