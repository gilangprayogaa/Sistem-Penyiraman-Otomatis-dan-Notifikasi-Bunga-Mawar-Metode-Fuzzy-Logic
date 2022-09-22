//library
#include "CTBot.h"
#include <DHT.h>
#include <WiFi.h>
#include <Fuzzy.h>

//sensor dht22
#define DHTPIN 33
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//telegrambot
CTBot myBot;
TBMessage msg;

const char* ssid = "kiya2306";    //ssid wifi
const char* pass = "kiya1234";  //password wifi
String token = "YOUR_TOKEN";    //token bot telegram
const int id = 1311707317;      //id telegram

//sensor kelembapan tanah
int sensorPin = 34;
int kelTanah;
float hum_u;
float last_hum_u;

//sensor suhu udara
const int err_pin = 12;
float t;
float last_t;

//pompa dan kipas
const int pompa = 13;
const int kipas = 14;

//setpoint suhu dan humidity
byte spsuhu;
byte sphum;

//waktu
long current_time;
long last_time;
long waktu;

byte mode1;
byte state;

// Fuzzy
Fuzzy *fuzzy = new Fuzzy();

// FuzzyInput Kelembapan Tanah
FuzzySet *negatif_hum = new FuzzySet(-1000, -50, -30, 0);
FuzzySet *netral_hum = new FuzzySet(-30, 0, 0, 25);
FuzzySet *positif_hum = new FuzzySet(0, 30, 100, 100);

// FuzzyInput Suhu Udara
FuzzySet *negatif_suhu = new FuzzySet(-100, -50, -25, 0);
FuzzySet *netral_suhu = new FuzzySet(-25, 0, 0, 25);
FuzzySet *positif_suhu = new FuzzySet(0, 25, 100, 100);

// FuzzyOutput Pompa
FuzzySet *pendek1 = new FuzzySet(0, 0, 1, 3);
FuzzySet *cukup1 = new FuzzySet(1, 3, 3, 5);
FuzzySet *lama1 = new FuzzySet(3, 5, 8, 20);

// FuzzyOutput Kipas
FuzzySet *pendek2 = new FuzzySet(0, 0, 1, 3);
FuzzySet *cukup2 = new FuzzySet(1, 3, 3, 5);
FuzzySet *lama2 = new FuzzySet(3, 5, 10, 20);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(50);
  
  dht.begin();

  //error pin
  last_t = 27;
  last_hum_u = 40;
  pinMode(err_pin, OUTPUT);
  digitalWrite(err_pin, LOW);

  //pompa
  pinMode(pompa, OUTPUT);
  digitalWrite(pompa, LOW);

  //kipas
  pinMode(kipas, OUTPUT); 
  digitalWrite(kipas, LOW);

  setupFuzzy();

  waktu = 5000;

  sphum = 50;
  spsuhu = 27;

  // wifi
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  //telegrambot
  myBot.wifiConnect(ssid, pass);
  myBot.setTelegramToken(token);
  
  //cek koneksi
  if (myBot.testConnection()) {
    Serial.println("Koneksi Bagus");
  } else {
    Serial.println("Koneksi Jelek");
  }

  mode1 = 0;
  delay(2000);
  state = 0; //kondisi awal
}

void loop() {
  // put your main code here, to run repeatedly:
  current_time = millis();
  switch (state)
  {
    case 0:
            bacaSensor();
            
            //waktu
            if (current_time - last_time >= waktu){
              penyiraman();
              last_time = current_time;
            }
            break;
  }
}

// function pembacaan sensor suhu dan kelembapan
void bacaSensor() {
  t = dht.readTemperature(); 
  hum_u = dht.readHumidity(); 
  
  //deklarasi error suhu
  if (isnan(t)){ 
    // jika error nilai pembacaan suhu akan default 27 maka led nyala
    t = last_t;
    digitalWrite(err_pin, HIGH);
  }else { 
    // jika tidak error nilai pembacaan suhu berdasarkan sensor maka led mati
    last_t = t;
    digitalWrite(err_pin, LOW);
  }
  
  //deklarasi error humidity
  if (isnan(hum_u)){
    // jika error nilai pembacaan humidity akan default 27
    hum_u = last_hum_u;
  }else {
    // jika tidak error nilai pembacaan humidity berdasarkan sensor
    last_hum_u = hum_u;
  }

  //perhitungan sensor kelembapan
  int nilaiSensor = analogRead(sensorPin);
  kelTanah = (100-((nilaiSensor/4095.00)*100));

  //menampilkan suhu aktual pada serial monitor
  Serial.print("Suhu saat ini : ");
  Serial.print(t);
  Serial.println(" *C");

  //menampilkan kelembapan aktual pada serial monitor
  Serial.print("Kelembapan Tanah : ");
  Serial.print(kelTanah);
  Serial.println(" %");

  //telegrambot
  String suhu = "Suhu : ";
  suhu += int(t);
  suhu += " *C & Kelembapan Udara : ";
  suhu += int(hum_u);
  suhu += "% & Kelembapan Tanah : ";
  suhu += int(kelTanah);
  suhu += "%\n";

  if (myBot.getNewMessage(msg)) {
    if (msg.messageType == CTBotMessageText) {       
      if (msg.text.equalsIgnoreCase("mode 1")) {   //mode 1 adalah mode otomatis
        mode1 = 1;                                 //maka hasil pembacaan sensor suhu dan kelembapan
        myBot.sendMessage(id, "ok mode otomatis"); //akan tampil di telegram selama 5 detik secara terus menerus
      } else if (msg.text.equalsIgnoreCase("mode 0")) { //mode 0 adalah mode manual
        mode1 = 0;
        myBot.sendMessage(id, "ok mode manual");
      } else if (msg.text.equalsIgnoreCase("data")) { //sama hal nya dengan mode otomatis tetapi hasil pembacaan
          if (mode1 == 0){                            //sensor suhu dan kelembapan akan tampil cuma sekali
            myBot.sendMessage(id, suhu, "");
            }
      } else if (msg.text.equalsIgnoreCase("sp suhu")) { //setting setpoint suhu
          mode1 = 2;
          myBot.sendMessage(id, "setting setpoint suhu"); // setelah notifikasi ini muncul lalu ke blok A1
      } else if (msg.text.equalsIgnoreCase("sp hum")) { // setting setpoint humidity
          mode1 = 3;
          myBot.sendMessage(id, "setting setpoint kelembapan"); // setelah notifikasi ini muncul lalu ke blok A2
      }
      else if (msg.text.equalsIgnoreCase("waktu")) { //setting waktu
          mode1 = 4;
          myBot.sendMessage(id, "setting durasi waktu"); // setelah notifikasi ini muncul lalu ke blok A3
      }
      else if(mode1 == 2){ //blok A1
        spsuhu = msg.text.toInt(); //user memasukkan setpoint suhu dari telegram
        Serial.println(spsuhu); //hasil input dari user ditampilkan di serial monitor
        myBot.sendMessage(id, String(spsuhu)); //hasil input dari user ditampilkan di telegram
        mode1 = 0; // kembali ke mode manual
        myBot.sendMessage(id, "mode manual");
     }
     else if(mode1 == 4){ //blok A3
        waktu = msg.text.toInt(); //user memasukkan set waktu dari telegram
        Serial.println(waktu); //hasil input dari user ditampilkan di serial monitor
        myBot.sendMessage(id, String(waktu)); //hasil input dari user ditampilkan di telegram
        mode1 = 0; // kembali ke mode manual
        myBot.sendMessage(id, "mode manual");
     }
     else if(mode1 == 3){ //blok A2
        sphum = msg.text.toInt(); //user memasukkan setpoint humidity dari telegram
        Serial.println(sphum); //hasil input dari user ditampilkan di serial monitor
        myBot.sendMessage(id, String(sphum)); //hasil input dari user ditampilkan di telegram
        mode1 = 0; // kembali ke mode manual
        myBot.sendMessage(id, "mode manuals");
     }
    }
  }

  // mode 1
  if (mode1 == 1){
    delay(5000);
    myBot.sendMessage(id, suhu, "");
    }
}

//function durasi penyiraman dan durasi kipas
void penyiraman(){
  int nilaiSensor = analogRead(sensorPin);
  kelTanah = (100-((nilaiSensor/4095.00)*100));
  
  if (isnan(hum_u)){
    hum_u = last_hum_u;
  }else {
    last_hum_u = hum_u;
  }

  t = dht.readTemperature();
  hum_u = dht.readHumidity();

  if (isnan(t)){
    t = last_t;
  }else {
    last_t = t;
  }

  //fuzzy menggunakan rumus delta
  int delta_hum = kelTanah - sphum; // delta humidity = kelembapan aktual - setpoint humidity
  int delta_suhu = t - spsuhu; // delta suhu = suhu aktual - setpoint suhu

  fuzzy->setInput(1, delta_hum); // fuzzy input 1 humidity
  fuzzy->setInput(2, delta_suhu); //fuzzy input 2 suhu

  fuzzy->fuzzify();

  Serial.print("\tKelembapan: Negatif_Hum-> ");
  Serial.print(negatif_hum->getPertinence());
  Serial.print("\t Setting Point Hum ");
  Serial.println(sphum);
  Serial.print("\t Delta Hum ");
  Serial.println(delta_hum);
  Serial.print(", Netral_Hum-> ");
  Serial.print(netral_hum->getPertinence());
  Serial.print(", Positif_Hum-> ");
  Serial.println(positif_hum->getPertinence());

  Serial.print("\tSuhu: Negatif_suhu-> ");
  Serial.print(negatif_suhu->getPertinence());
  Serial.print("\t Setting Point Suhu ");
  Serial.println(spsuhu);
  Serial.print("\t Delta Suhu ");
  Serial.println(delta_suhu);
  Serial.print(", Netral_suhu-> ");
  Serial.print(netral_suhu->getPertinence());
  Serial.print(", Positif_suhu-> ");
  Serial.println(positif_suhu->getPertinence());
  
  //output defuzzyfikasi
  float output1 = fuzzy->defuzzify(1);
  float output2 = fuzzy->defuzzify(2);

  Serial.println("Output: ");
  Serial.print("\tDurasi 1 (pompa): Pendek 1-> ");
  Serial.print(pendek1->getPertinence());
  Serial.print(", Cukup 1-> ");
  Serial.print(cukup1->getPertinence());
  Serial.print(", Lama 1-> ");
  Serial.println(lama1->getPertinence());

  Serial.print("\tDurasi 2 (kipas): Pendek 2-> ");
  Serial.print(pendek2->getPertinence());
  Serial.print(",  Cukup 2-> ");
  Serial.print(cukup2->getPertinence());
  Serial.print(",  Lama 2-> ");
  Serial.print(lama2->getPertinence());

  Serial.print("Durasi Penyiraman : ");
  Serial.print(output1);
  Serial.println(" Detik");
  myBot.sendMessage(id, "Durasi Penyiraman");
  myBot.sendMessage(id, String(output1));

  Serial.print("Durasi Kipas : ");
  Serial.print(output2);
  Serial.println(" Detik");
  myBot.sendMessage(id, "Durasi Kipas");
  myBot.sendMessage(id, String(output2));

  int semprot = output1*1000; //hasil durasi penyiraman dalam milisecond harus dikali 1000 biar ke detik
  digitalWrite(pompa, HIGH); // pompa nyala
  delay(semprot);
  digitalWrite(pompa, LOW); // pompa mati

  int nyala_kipas = output1*1000; //hasil durasi kipas dalam milisecond harus dikali 1000 biar ke detik
  digitalWrite(kipas, HIGH); //kipas nyala
  delay(nyala_kipas);
  digitalWrite(kipas, LOW); //kipas mati
}

//function fuzzy setup
void setupFuzzy(){
//  FuzzyInput KelembapanTanah
  FuzzyInput *kelembapan = new FuzzyInput(1);
  kelembapan->addFuzzySet(negatif_hum);
  kelembapan->addFuzzySet(netral_hum);
  kelembapan->addFuzzySet(positif_hum);
  fuzzy->addFuzzyInput(kelembapan);

//  FuzzyInput SuhuUdara
  FuzzyInput *suhu = new FuzzyInput(2);
  suhu->addFuzzySet(negatif_suhu);
  suhu->addFuzzySet(netral_suhu);
  suhu->addFuzzySet(positif_suhu);
  fuzzy->addFuzzyInput(suhu);

  // FuzzyOutput Durasi-1 (pompa)
  FuzzyOutput *durasi1 = new FuzzyOutput(1);
  durasi1->addFuzzySet(pendek1);
  durasi1->addFuzzySet(cukup1);
  durasi1->addFuzzySet(lama1);
  fuzzy->addFuzzyOutput(durasi1);

  // FuzzyOutput Durasi-2 (kipas)
  FuzzyOutput *durasi2 = new FuzzyOutput(2);
  durasi2->addFuzzySet(pendek2);
  durasi2->addFuzzySet(cukup2);
  durasi2->addFuzzySet(lama2);
  fuzzy->addFuzzyOutput(durasi2);

  // Building FuzzyRule 1 'if kelembapan negatif_hum and suhu negatif_suhu then durasi cukup1'
  FuzzyRuleAntecedent *ifKelembapannegatif_humAndSuhunegatif_suhu = new FuzzyRuleAntecedent();
  ifKelembapannegatif_humAndSuhunegatif_suhu->joinWithAND(negatif_hum, negatif_suhu);
  FuzzyRuleConsequent *thenDurasiCukup1 = new FuzzyRuleConsequent();
  thenDurasiCukup1->addOutput(cukup1);
  FuzzyRule *fuzzyRule1 = new FuzzyRule(1, ifKelembapannegatif_humAndSuhunegatif_suhu, thenDurasiCukup1);
  fuzzy->addFuzzyRule(fuzzyRule1);

  // Building FuzzyRule 2 'if kelembapan netral_hum and suhu negatif_suhu then durasi pendek1'
  FuzzyRuleAntecedent *ifKelembapannetral_humAndSuhunegatif_suhu = new FuzzyRuleAntecedent();
  ifKelembapannetral_humAndSuhunegatif_suhu->joinWithAND(netral_hum, negatif_suhu);
  FuzzyRuleConsequent *thenDurasiPendek1 = new FuzzyRuleConsequent();
  thenDurasiPendek1->addOutput(pendek1);
  FuzzyRule *fuzzyRule2 = new FuzzyRule(2, ifKelembapannetral_humAndSuhunegatif_suhu, thenDurasiPendek1);
  fuzzy->addFuzzyRule(fuzzyRule2);

  // Building FuzzyRule 3 'if kelembapan positif_hum and suhu negatif_suhu then durasi pendek1'
  FuzzyRuleAntecedent *ifKelembapanpositif_humAndSuhunegatif_suhu = new FuzzyRuleAntecedent();
  ifKelembapanpositif_humAndSuhunegatif_suhu->joinWithAND(positif_hum, negatif_suhu);
  FuzzyRuleConsequent *thenDurasiPendek2 = new FuzzyRuleConsequent();
  thenDurasiPendek2->addOutput(pendek1);
  FuzzyRule *fuzzyRule3 = new FuzzyRule(3, ifKelembapanpositif_humAndSuhunegatif_suhu, thenDurasiPendek2);
  fuzzy->addFuzzyRule(fuzzyRule3);

  //FuzzyRule4-->if negatif_hum and netral_suhu then lama1
  FuzzyRuleAntecedent *ifKelembapannegatif_humAndSuhunetral_suhu = new FuzzyRuleAntecedent();
  ifKelembapannegatif_humAndSuhunetral_suhu->joinWithAND(negatif_hum, netral_suhu);
  FuzzyRuleConsequent *thenDurasiLama1 = new FuzzyRuleConsequent();
  thenDurasiLama1->addOutput(lama1);
  FuzzyRule *fuzzyRule4 = new FuzzyRule(4, ifKelembapannegatif_humAndSuhunetral_suhu, thenDurasiLama1);
  fuzzy->addFuzzyRule(fuzzyRule4);

  //FuzzyRule5-->if netral_hum and netral_suhu then pendek3
  FuzzyRuleAntecedent *ifKelembapannetral_humAndSuhunetral_suhu = new FuzzyRuleAntecedent();
  ifKelembapannetral_humAndSuhunetral_suhu->joinWithAND(netral_hum, netral_suhu);
  FuzzyRuleConsequent *thenDurasiPendek3 = new FuzzyRuleConsequent();
  thenDurasiPendek3->addOutput(pendek1);
  FuzzyRule *fuzzyRule5 = new FuzzyRule(5, ifKelembapannetral_humAndSuhunetral_suhu, thenDurasiPendek3);
  fuzzy->addFuzzyRule(fuzzyRule5);

  //FuzzyRule6-->if positif_hum and netral_suhu then pendek4
  FuzzyRuleAntecedent *ifKelembapanpositif_humAndSuhunetral_suhu = new FuzzyRuleAntecedent();
  ifKelembapanpositif_humAndSuhunetral_suhu->joinWithAND(positif_hum, netral_suhu);
  FuzzyRuleConsequent *thenDurasiPendek4 = new FuzzyRuleConsequent();
  thenDurasiPendek4->addOutput(pendek1);
  FuzzyRule *fuzzyRule6 = new FuzzyRule(6, ifKelembapanpositif_humAndSuhunetral_suhu, thenDurasiPendek4);
  fuzzy->addFuzzyRule(fuzzyRule6);

  //FuzzyRule7-->if negatif_hum and positif_suhu then lama2
  FuzzyRuleAntecedent *ifKelembapannegatif_humAndSuhupositif_suhu = new FuzzyRuleAntecedent();
  ifKelembapannegatif_humAndSuhupositif_suhu->joinWithAND(negatif_hum, positif_suhu);
  FuzzyRuleConsequent *thenDurasiLama2 = new FuzzyRuleConsequent();
  thenDurasiLama2->addOutput(lama1);
  FuzzyRule *fuzzyRule7 = new FuzzyRule(7, ifKelembapannegatif_humAndSuhupositif_suhu, thenDurasiLama2);
  fuzzy->addFuzzyRule(fuzzyRule7);

  //FuzzyRule8-->if netral_hum and positif_suhu then pendek5
  FuzzyRuleAntecedent *ifKelembapannetral_humAndSuhupositif_suhu = new FuzzyRuleAntecedent();
  ifKelembapannetral_humAndSuhupositif_suhu->joinWithAND(netral_hum, positif_suhu);
  FuzzyRuleConsequent *thenDurasiPendek5 = new FuzzyRuleConsequent();
  thenDurasiPendek5->addOutput(pendek1);
  FuzzyRule *fuzzyRule8 = new FuzzyRule(8, ifKelembapannetral_humAndSuhupositif_suhu, thenDurasiPendek5);
  fuzzy->addFuzzyRule(fuzzyRule8);

  //FuzzyRule9-->if positif_hum and positif_suhu then pendek6
  FuzzyRuleAntecedent *ifKelembapanpositif_humAndSuhupositif_suhu = new FuzzyRuleAntecedent();
  ifKelembapanpositif_humAndSuhupositif_suhu->joinWithAND(positif_hum, positif_suhu);
  FuzzyRuleConsequent *thenDurasiPendek6 = new FuzzyRuleConsequent();
  thenDurasiPendek6->addOutput(pendek1);
  FuzzyRule *fuzzyRule9 = new FuzzyRule(9, ifKelembapanpositif_humAndSuhupositif_suhu, thenDurasiPendek6);
  fuzzy->addFuzzyRule(fuzzyRule9);



  //BuildingFuzzyKipas
  //FuzzyRule10-->if negatif_hum and negatif_suhu then pendek7
  FuzzyRuleAntecedent *ifKelembapannegatif_humAndSuhunegatif_suhu2 = new FuzzyRuleAntecedent();
  ifKelembapannegatif_humAndSuhunegatif_suhu2->joinWithAND(negatif_hum, negatif_suhu);
  FuzzyRuleConsequent *thenDurasiPendek7 = new FuzzyRuleConsequent();
  thenDurasiPendek7->addOutput(pendek2);
  FuzzyRule *fuzzyRule10 = new FuzzyRule(10, ifKelembapannegatif_humAndSuhunegatif_suhu2, thenDurasiPendek7);
  fuzzy->addFuzzyRule(fuzzyRule10);

  //FuzzyRule11-->if netral_hum and negatif_suhu then pendek8
  FuzzyRuleAntecedent *ifKelembapannetral_humAndSuhunegatif_suhu2 = new FuzzyRuleAntecedent();
  ifKelembapannetral_humAndSuhunegatif_suhu2->joinWithAND(netral_hum, negatif_suhu);
  FuzzyRuleConsequent *thenDurasiPendek8 = new FuzzyRuleConsequent();
  thenDurasiPendek8->addOutput(pendek2);
  FuzzyRule *fuzzyRule11 = new FuzzyRule(11, ifKelembapannetral_humAndSuhunegatif_suhu2, thenDurasiPendek8);
  fuzzy->addFuzzyRule(fuzzyRule11);

  //FuzzyRule12-->if positif_hum and negatif_suhu then pendek9
  FuzzyRuleAntecedent *ifKelembapanpositif_humAndSuhunegatif_suhu2 = new FuzzyRuleAntecedent();
  ifKelembapanpositif_humAndSuhunegatif_suhu2->joinWithAND(positif_hum, negatif_suhu);
  FuzzyRuleConsequent *thenDurasiPendek9 = new FuzzyRuleConsequent();
  thenDurasiPendek9->addOutput(pendek2);
  FuzzyRule *fuzzyRule12 = new FuzzyRule(12, ifKelembapanpositif_humAndSuhunegatif_suhu2, thenDurasiPendek9);
  fuzzy->addFuzzyRule(fuzzyRule12);

  //FuzzyRule13-->if negatif_hum and netral_suhu then cukup2
  FuzzyRuleAntecedent *ifKelembapannegatif_humAndSuhunetral_suhu2 = new FuzzyRuleAntecedent();
  ifKelembapannegatif_humAndSuhunetral_suhu2->joinWithAND(negatif_hum, netral_suhu);
  FuzzyRuleConsequent *thenDurasiCukup2 = new FuzzyRuleConsequent();
  thenDurasiCukup2->addOutput(cukup2);
  FuzzyRule *fuzzyRule13 = new FuzzyRule(13, ifKelembapannegatif_humAndSuhunetral_suhu2, thenDurasiCukup2);
  fuzzy->addFuzzyRule(fuzzyRule13);

  //FuzzyRule14-->if netral_hum and netral_suhu then cukup3
  FuzzyRuleAntecedent *ifKelembapannetral_humAndSuhunetral_suhu2 = new FuzzyRuleAntecedent();
  ifKelembapannetral_humAndSuhunetral_suhu2->joinWithAND(netral_hum, netral_suhu);
  FuzzyRuleConsequent *thenDurasiCukup3 = new FuzzyRuleConsequent();
  thenDurasiCukup3->addOutput(cukup2);
  FuzzyRule *fuzzyRule14 = new FuzzyRule(14, ifKelembapannetral_humAndSuhunetral_suhu2, thenDurasiCukup3);
  fuzzy->addFuzzyRule(fuzzyRule14);

  //FuzzyRule15-->if positif_hum and netral_suhu then cukup4
  FuzzyRuleAntecedent *ifKelembapanpositif_humAndSuhunetral_suhu2 = new FuzzyRuleAntecedent();
  ifKelembapanpositif_humAndSuhunetral_suhu2->joinWithAND(positif_hum, netral_suhu);
  FuzzyRuleConsequent *thenDurasiCukup4 = new FuzzyRuleConsequent();
  thenDurasiCukup4->addOutput(cukup2);
  FuzzyRule *fuzzyRule15 = new FuzzyRule(15, ifKelembapanpositif_humAndSuhunetral_suhu2, thenDurasiCukup4);
  fuzzy->addFuzzyRule(fuzzyRule15);

  //FuzzyRule16-->if negatif_hum and positif_suhu then lama3
  FuzzyRuleAntecedent *ifKelembapannegatif_humAndSuhupositif_suhu2 = new FuzzyRuleAntecedent();
  ifKelembapannegatif_humAndSuhupositif_suhu2->joinWithAND(negatif_hum, positif_suhu);
  FuzzyRuleConsequent *thenDurasiLama3 = new FuzzyRuleConsequent();
  thenDurasiLama3->addOutput(lama2);
  FuzzyRule *fuzzyRule16 = new FuzzyRule(16, ifKelembapannegatif_humAndSuhupositif_suhu2, thenDurasiLama3);
  fuzzy->addFuzzyRule(fuzzyRule16);

  //FuzzyRule17-->if netral_hum and positif_suhu then lama4
  FuzzyRuleAntecedent *ifKelembapannetral_humAndSuhupositif_suhu2 = new FuzzyRuleAntecedent();
  ifKelembapannetral_humAndSuhupositif_suhu2->joinWithAND(netral_hum, positif_suhu);
  FuzzyRuleConsequent *thenDurasiLama4 = new FuzzyRuleConsequent();
  thenDurasiLama4->addOutput(lama2);
  FuzzyRule *fuzzyRule17 = new FuzzyRule(17, ifKelembapannetral_humAndSuhupositif_suhu2, thenDurasiLama4);
  fuzzy->addFuzzyRule(fuzzyRule17);

  //FuzzyRule18-->if positif_hum and positif_suhu then lama5
  FuzzyRuleAntecedent *ifKelembapanpositif_humAndSuhupositif_suhu2 = new FuzzyRuleAntecedent();
  ifKelembapanpositif_humAndSuhupositif_suhu2->joinWithAND(positif_hum, positif_suhu);
  FuzzyRuleConsequent *thenDurasiLama5 = new FuzzyRuleConsequent();
  thenDurasiLama5->addOutput(lama2);
  FuzzyRule *fuzzyRule18 = new FuzzyRule(18, ifKelembapanpositif_humAndSuhupositif_suhu2, thenDurasiLama5);
  fuzzy->addFuzzyRule(fuzzyRule18);
}
