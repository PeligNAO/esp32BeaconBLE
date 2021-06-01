/* 
 * SW per i bracciali del Parco Maiella 
 * Il sw invia un beacon Eddystone UID ogni DEEP_SLEEP_DURATION per una durata di 100 ms
 * Ogni 5 beacon UID invia anche un beacon TLM con i dati di temperatura e tensione batteria
 * Quindi va in sleep mode per risparmiare energia.
 * Versione 1.0 del 18-05-21 by Team PeligNAO
 * 
 * Copyright (c) 2021 PeligNAO. All Rights Reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>. *
 * 
 */
 
// File di 'include' relativi alle librerie utilizzate

#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "esp_sleep.h"
#include "WiFi.h"

// Parametri del programma

#define DEEP_SLEEP_DURATION     1   // durata della fase di sleep
#define ADVERTISING_DURATION   100  // durate della fase di trasmissione
#define VOLTAGE_PIN 34              // GPIO dell'ESP per la lettura della tensione della batteria

RTC_DATA_ATTR static uint32_t bootcount; // numero di riavvi salvato nella memoria permanente 

BLEAdvertising *annuncio;  //Crea la variabile globale 'annuncio' di tipo puntatore alla classe BLEAdvertising

/*
 * setBeaconTLM()
 * La seguente funzione configura il messaggio di tipo TLM
 * Non è possibile inserire altre informazioni nel messaggio Eddystone in quanto restano pochi byte disponibili
 * Per eventuali altre informazioni si dovrebbe abilitare la ScanResponse, configurare un secondo messaggio 
 * ed utilizzare il metodo setScanResponseData
 */
  
void setBeaconTLM() {
  char beacon_data[22];         //contiene i dati Eddystone TLM
  uint16_t beconUUID = 0xFEAA;  //specifica che il messaggio è di tipo Eddystone
  uint16_t volt; 
  uint16_t temp = (uint16_t)((float)23.00); //la temperatura è fissa, al momento è solo dimostrativa
  volt=analogRead(VOLTAGE_PIN);             //legge la tensione della batteria dal pin analogico
  volt=volt*3300/4096;                      //esegue la conversione
  Serial.println(volt);
  
  BLEAdvertisementData messaggio = BLEAdvertisementData(); // crea un'istanza della classe BLEAdvertisementData
  
  messaggio.setFlags(0x06); // //inserisce nella variabile 'messaggio' i dati dei flag GENERAL_DISC_MODE 0x02 | BR_EDR_NOT_SUPPORTED 0x04
  messaggio.setCompleteServices(BLEUUID(beconUUID)); //inserisce nella variabile 'messaggio' il tipo di messaggio (Eddystone)
  
  beacon_data[0] = 0x20;  // Eddystone-TLM
  beacon_data[1] = 0x00;  // Versione TLM
  beacon_data[2] = (volt>>8);  // Tensione della batteria, 1 mV/bit i.e. 0x0CE4 = 3300mV = 3.3V
  beacon_data[3] = (volt&0xFF);  // 
  beacon_data[4] = (temp&0xFF);  // Temperatura
  beacon_data[5] = (temp>>8);    // 
  beacon_data[6] = ((bootcount & 0xFF000000) >> 24);  // Valore della variabile bootcount
  beacon_data[7] = ((bootcount & 0xFF0000) >> 16);    // in una versione successiva dovrà incrementarsi sempre  
  beacon_data[8] = ((bootcount & 0xFF00) >> 8);       // per specifiche del TLM
  beacon_data[9] = (bootcount&0xFF);  // 
  beacon_data[10] = 0x00;  // Tempo dal reboot, non utilizzato al momento
  beacon_data[11] = 0x00;  // ma da utilizzare per specifiche TLM
  beacon_data[12] = 0x00;  // 
  beacon_data[13] = 0x00;  // 
  
  messaggio.setServiceData(BLEUUID(beconUUID), std::string(beacon_data, 14));//inserisce nella variabile 'messaggio' i dati Eddystone da inviare
  annuncio->setScanResponse(0);                 //disabilita la scan response per risparmiare batteria
  annuncio->setAdvertisementData(messaggio);    //prepara il messaggio per l'invio
 
}

/*
 * setBeaconUID()
 * La seguente funzione configura il messaggio UID
 * Non è possibile inserire altre informazioni nel messaggio Eddystone in quanto restano pochi byte disponibili
 * Per eventuali altre informazioni si dovrebbe abilitare la ScanResponse, configurare un secondo messaggio 
 * ed utilizzare il metodo setScanResponseData
 */

void setBeaconUID() {
  char beacon_data[20];         //contiene i dati Eddystone UID
  uint16_t beconUUID = 0xFEAA;  //specifica che il messaggio è di tipo Eddystone
    
  BLEAdvertisementData messaggio = BLEAdvertisementData();  //crea un'istanza della classe BLEAdvertisementData
  
  messaggio.setFlags(0x06); //inserisce nella variabile 'messaggio' i dati dei flag GENERAL_DISC_MODE 0x02 | BR_EDR_NOT_SUPPORTED 0x04
  messaggio.setCompleteServices(BLEUUID(beconUUID));  //inserisce nella variabile 'messaggio' il tipo di messaggio (Eddystone)

  beacon_data[0] = 0x00;    // Eddystone UID
  beacon_data[1] = 0xEC;    // Potenza di trasmissione a 0m per calcolare la distanza
                            // il valore è codificato in un byte con complemento a due
  beacon_data[2] = 'P';     // UID 10 byte
  beacon_data[3] = '.';   
  beacon_data[4] = ' ';   
  beacon_data[5] = 'M';   
  beacon_data[6] = 'a';   
  beacon_data[7] = 'i';   
  beacon_data[8] = 'e';   
  beacon_data[9] = 'l';   
  beacon_data[10] = 'l';   
  beacon_data[11] = 'a';   
  beacon_data[12] = 'P';    // ID 6 byte
  beacon_data[13] = 'e';   
  beacon_data[14] = 'o';   
  beacon_data[15] = 'n';   
  beacon_data[16] = 'i';   
  beacon_data[17] = 'a';   
  beacon_data[18] = 0x00;  // Riservato
  beacon_data[19] = 0x00;  // Riservato
  
  messaggio.setServiceData(BLEUUID(beconUUID), std::string(beacon_data, 20));   //inserisce nella variabile 'messaggio' i dati Eddystone da inviare
  annuncio->setScanResponse(0);                 //disabilita la scan response per risparmiare batteria
  annuncio->setAdvertisementData(messaggio);    //prepara il messaggio per l'invio
}

/*
 * setup()
 * La funzione setup svolge tutto il lavoro in quanto al termine il uC va in sleep
 * e si riavvia completamente al termine della fase di sleep
 */

void setup() {
  //Disabilita la wifi
  WiFi.mode(WIFI_OFF);
  // Configura la seriale  
  Serial.begin(115200);

  // Configura il nome del dispositivo BLE
  BLEDevice::init("PeligNAO");
 
  // Crea il server BLE
  BLEServer *mioServer = BLEDevice::createServer();

  annuncio = mioServer->getAdvertising(); //crea un'istanza della classe BLEAdvertising
  bootcount++;                            // incrementa il contatore dei riavvii
  // Richiama la funzione che configura il messaggio UID
  setBeaconUID();
   // Invio beacon UID
  annuncio->start();
  Serial.println("Invio advertising tipo UID ...");
  delay(ADVERTISING_DURATION);
  annuncio->stop();
  if (bootcount == 4) {  // ogni 5 beacon UID viene inviato anche un beacon TLM
    // Richiama la funzione che configura il messaggio TLM
    setBeaconTLM();
    // Invio beacon UID
    annuncio->start();
    Serial.println("Invio advertising tipo TLM ...");
    delay(ADVERTISING_DURATION);
    annuncio->stop();
    bootcount=0;  //riazzera il contatore dei riavvii
  }

  esp_deep_sleep(0100000LL * DEEP_SLEEP_DURATION);  //va in sleep mode

}

/*
 * loop()
 * La funzione loop non è utilizzata
 * Vedi commento della funzione sleep()
 */
 
void loop() {
}
