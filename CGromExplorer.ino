#include <SPI.h>

#define LCD_RST    8  // optional, not necessary
#define LCD_CS     10

// Definiciones de comandos
#define LCD_CLS         0x01
#define LCD_HOME        0x02
#define LCD_ADDRINC     0x06
#define LCD_DISPLAYON   0x0C
#define LCD_CURSORON    0x0E
#define LCD_CURSORBLINK 0x0F
#define LCD_BASIC       0x30
#define LCD_EXTEND      0x34
#define LCD_LINE0       0x80
#define LCD_LINE1       0x90
#define LCD_LINE2       0x88
#define LCD_LINE3       0x98

// Buffer para entrada serial
char inputBuffer[10];
byte inputIndex = 0;

void sendCmd(byte b) {
  SPI.beginTransaction(SPISettings(200000UL, MSBFIRST, SPI_MODE3));
  digitalWrite(LCD_CS, HIGH);
  SPI.transfer(0xF8);
  SPI.transfer(b & 0xF0);
  SPI.transfer(b << 4);
  digitalWrite(LCD_CS, LOW);
  SPI.endTransaction();
  delayMicroseconds(72);
}

void sendData(byte b) {
  SPI.beginTransaction(SPISettings(200000UL, MSBFIRST, SPI_MODE3));
  digitalWrite(LCD_CS, HIGH);
  SPI.transfer(0xFA);
  SPI.transfer(b & 0xF0);
  SPI.transfer(b << 4);
  digitalWrite(LCD_CS, LOW);
  SPI.endTransaction();
  delayMicroseconds(72);
}

void showText(byte line, const char* text) {
  sendCmd(line);
  for(int i = 0; text[i] != '\0'; i++) {
    sendData(text[i]);
  }
}

void clearScreen() {
  sendCmd(LCD_CLS);
  delay(2);
}

const char* getCharType(byte c) {
  if(c < 32) return "Control";
  if(c < 127) return "ASCII";
  if(c >= 0xA1 && c <= 0xF7) return "Chino (1er)";
  if(c >= 0xA1 && c <= 0xFE) return "Chino (2do)";
  return "Especial";
}

void LCD_init() {
  Serial.begin(115200);
  while(!Serial);
  
  pinMode(LCD_CS, OUTPUT);
  digitalWrite(LCD_CS, LOW);
  SPI.begin();

  sendCmd(LCD_BASIC);
  sendCmd(LCD_BASIC);
  sendCmd(LCD_CLS);
  delay(2);
  sendCmd(LCD_ADDRINC);
  sendCmd(LCD_DISPLAYON);

  Serial.println("Sistema listo. Ingrese direccion hexadecimal:");
  showText(LCD_LINE0, "Ingrese dir. hex");
  showText(LCD_LINE1, "Ej: 41 o A1B0");
}

void showFullCharInfo(uint16_t address) {
  clearScreen();
  char lineBuffer[17];
  byte highByte = address >> 8;
  byte lowByte = address & 0xFF;
  
  // Línea 0: Dirección completa
  snprintf(lineBuffer, sizeof(lineBuffer), "DIR:0x%04X", address);
  showText(LCD_LINE0, lineBuffer);
  
  // Línea 1: Bytes y tipo
  if(address <= 0xFF) {
    // Carácter de 1 byte
    snprintf(lineBuffer, sizeof(lineBuffer), "Byte:0x%02X %s", 
             lowByte, getCharType(lowByte));
  } else {
    // Carácter de 2 bytes
    snprintf(lineBuffer, sizeof(lineBuffer), "Bytes:%02X %02X", 
             highByte, lowByte);
  }
  showText(LCD_LINE1, lineBuffer);
  
  // Línea 2: Representación del carácter
  sendCmd(LCD_LINE2);
  if(address <= 0x7F) {
    // ASCII
    byte c = address;
    if(c >= 32 && c <= 126) {
      snprintf(lineBuffer, sizeof(lineBuffer), "Char:'%c'", c);
    } else {
      snprintf(lineBuffer, sizeof(lineBuffer), "Code:0x%02X", c);
    }
    showText(LCD_LINE2, lineBuffer);
  } else {
    // Chino - mostrar los bytes como caracteres
    sendData(highByte);
    sendData(lowByte);
  }
  
  // Línea 3: Información adicional
  sendCmd(LCD_LINE3);
  if(address <= 0xFF) {
    byte c = address;
    if(c >= 32 && c <= 126) {
      snprintf(lineBuffer, sizeof(lineBuffer), "Dec:%d", c);
    } else {
      snprintf(lineBuffer, sizeof(lineBuffer), "Bin:%08b", c);
    }
  } else {
    snprintf(lineBuffer, sizeof(lineBuffer), "Chino:U+%04X", address);
  }
  showText(LCD_LINE3, lineBuffer);
  
  // Feedback por serial
  Serial.print("\nDireccion: 0x");
  Serial.println(address, HEX);
  if(address <= 0xFF) {
    Serial.print("Byte: 0x");
    Serial.println(lowByte, HEX);
    Serial.print("Tipo: ");
    Serial.println(getCharType(lowByte));
    if(lowByte >= 32 && lowByte <= 126) {
      Serial.print("Caracter ASCII: '");
      Serial.print((char)lowByte);
      Serial.println("'");
    }
    Serial.print("Decimal: ");
    Serial.println(lowByte);
    Serial.print("Binario: ");
    Serial.println(lowByte, BIN);
  } else {
    Serial.print("Bytes: 0x");
    Serial.print(highByte, HEX);
    Serial.print(" 0x");
    Serial.println(lowByte, HEX);
    Serial.println("Tipo: Caracter chino");
    Serial.print("Unicode aproximado: U+");
    Serial.println(address, HEX);
  }
  Serial.println("\nIngrese nueva direccion...");
}

void processSerialInput() {
  while(Serial.available() > 0) {
    char c = Serial.read();
    
    if(c == '\n' || c == '\r') {
      if(inputIndex > 0) {
        inputBuffer[inputIndex] = '\0';
        uint16_t address = (uint16_t)strtoul(inputBuffer, NULL, 16);
        showFullCharInfo(address);
        inputIndex = 0;
        
        // Preparar para nueva entrada
      //  showText(LCD_LINE0, "Nueva dir. hex");
      //  showText(LCD_LINE1, "Ej: 41 o A1B0");
      }
    } 
    else if(isxdigit(c) && inputIndex < sizeof(inputBuffer) - 1) {
      inputBuffer[inputIndex++] = toupper(c);
    }
  }
}

void setup() {
  LCD_init();
}

void loop() {
  processSerialInput();
}
