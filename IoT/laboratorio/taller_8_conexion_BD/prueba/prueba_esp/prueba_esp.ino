void setup() {
  // Iniciamos la comunicación serie a 115200 baudios
  Serial.begin(115200);
  
  // Configuramos el pin 2 como salida (la mayoría de las placas ESP32 tienen un LED azul aquí)
  pinMode(2, OUTPUT); 
}

void loop() {
  // Imprimimos un mensaje en el Monitor Serie
  Serial.println("¡Éxito! Tu Arduino IDE y el ESP32 están configurados correctamente.");
  
  // Encendemos el LED
  digitalWrite(2, HIGH);
  delay(1000); // Esperamos 1 segundo
  
  // Apagamos el LED
  digitalWrite(2, LOW);
  delay(1000); // Esperamos 1 segundo
}