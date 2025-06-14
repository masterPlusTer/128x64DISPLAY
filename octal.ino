// ingresa un valor octal por consola y el display te mostrara lo que hay en esa direccion de memoria 


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
enum InputMode { DECIMAL, OCTAL, HEXADECIMAL } inputMode = OCTAL; // Modo octal por defecto

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
  if(c == 127) return "Delete";
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

  updateDisplayMode();
  Serial.println("\nSistema listo en modo octal. Ingrese valores octales (0-177777).");
  Serial.println("Comandos: d=Decimal, o=Octal, h=Hexadecimal");
}

void updateDisplayMode() {
  clearScreen();
  switch(inputMode) {
    case DECIMAL:
      showText(LCD_LINE0, "Modo Decimal");
      showText(LCD_LINE1, "Ingrese 0-255");
      showText(LCD_LINE2, "o 0-65535");
      Serial.println("\nModo decimal activado");
      break;
    case OCTAL:
      showText(LCD_LINE0, "Modo Octal");
      showText(LCD_LINE1, "Ingrese 0-377");
      showText(LCD_LINE2, "o 0-177777");
      Serial.println("\nModo octal activado");
      break;
    case HEXADECIMAL:
      showText(LCD_LINE0, "Modo Hex");
      showText(LCD_LINE1, "Ingrese 00-FF");
      showText(LCD_LINE2, "o 0000-FFFF");
      Serial.println("\nModo hexadecimal activado");
      break;
  }
}

void showFullCharInfo(uint16_t value) {
  clearScreen();
  char lineBuffer[17];
  byte highByte = value >> 8;
  byte lowByte = value & 0xFF;
  
  // Línea 0: Valor ingresado en formato octal con prefijo
  snprintf(lineBuffer, sizeof(lineBuffer), "Oct:0%o", value);
  showText(LCD_LINE0, lineBuffer);
  
  // Línea 1: Equivalente decimal y hexadecimal
  snprintf(lineBuffer, sizeof(lineBuffer), "Dec:%u Hex:0x%X", value, value);
  showText(LCD_LINE1, lineBuffer);
  
  // Línea 2: Tipo de carácter
  if(value <= 0xFF) {
    byte c = value;
    if(c >= 32 && c <= 126) {
      snprintf(lineBuffer, sizeof(lineBuffer), "'%c' %s", c, getCharType(c));
    } else {
      snprintf(lineBuffer, sizeof(lineBuffer), "%s", getCharType(c));
    }
    showText(LCD_LINE2, lineBuffer);
  } else {
    snprintf(lineBuffer, sizeof(lineBuffer), "Chino 2 bytes");
    showText(LCD_LINE2, lineBuffer);
    sendCmd(LCD_LINE3);
    sendData(highByte);
    sendData(lowByte);
    return;
  }
  
  // Línea 3: Información adicional
  sendCmd(LCD_LINE3);
  if(value <= 0xFF) {
    byte c = value;
    if(c < 32) {
      const char* controlNames[] = {
        "NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
        "BS", "HT", "LF", "VT", "FF", "CR", "SO", "SI",
        "DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
        "CAN", "EM", "SUB", "ESC", "FS", "GS", "RS", "US"
      };
      snprintf(lineBuffer, sizeof(lineBuffer), "Ctrl: %s", controlNames[c]);
    } else if(c == 127) {
      snprintf(lineBuffer, sizeof(lineBuffer), "DELETE");
    } else {
      snprintf(lineBuffer, sizeof(lineBuffer), "Bin: %08b", c);
    }
    showText(LCD_LINE3, lineBuffer);
  }
  
  // Feedback por serial
  Serial.print("\nValor ingresado (Octal): 0");
  Serial.println(value, OCT);
  
  Serial.print("Decimal: ");
  Serial.println(value);
  
  Serial.print("Hexadecimal: 0x");
  Serial.println(value, HEX);
  
  if(value <= 0xFF) {
    byte c = value;
    Serial.print("Tipo: ");
    Serial.println(getCharType(c));
    
    if(c < 32) {
      Serial.println("Carácter de control no imprimible");
    } else if(c == 127) {
      Serial.println("Carácter DELETE");
    } else if(c >= 32 && c <= 126) {
      Serial.print("Carácter ASCII imprimible: '");
      Serial.print((char)c);
      Serial.println("'");
    }
    
    Serial.print("Binario: ");
    Serial.println(c, BIN);
  } else {
    Serial.println("Carácter chino de 2 bytes");
    Serial.print("Byte alto: 0x");
    Serial.println(highByte, HEX);
    Serial.print("Byte bajo: 0x");
    Serial.println(lowByte, HEX);
  }
  
  Serial.println("\nIngrese nuevo valor octal...");
}

void processSerialInput() {
  while(Serial.available() > 0) {
    char c = Serial.read();
    
    if(c == '\n' || c == '\r') {
      if(inputIndex > 0) {
        inputBuffer[inputIndex] = '\0';
        uint16_t value = 0;
        bool valid = true;
        
        // Validación estricta para modo octal
        if(inputMode == OCTAL) {
          for(byte i = 0; i < inputIndex; i++) {
            if(inputBuffer[i] < '0' || inputBuffer[i] > '7') {
              valid = false;
              break;
            }
          }
          if(valid) {
            value = strtoul(inputBuffer, NULL, 8); // Conversión octal estricta
          }
        } 
        else if(inputMode == DECIMAL) {
          for(byte i = 0; i < inputIndex; i++) {
            if(!isdigit(inputBuffer[i])) {
              valid = false;
              break;
            }
          }
          if(valid) value = atoi(inputBuffer);
        }
        else { // Hexadecimal
          for(byte i = 0; i < inputIndex; i++) {
            if(!isxdigit(inputBuffer[i])) {
              valid = false;
              break;
            }
          }
          if(valid) {
            value = strtoul(inputBuffer, NULL, 16);
          }
        }
        
        if(valid) {
          showFullCharInfo(value);
        } else {
          Serial.println("\nError: Valor no válido para el modo actual");
          showText(LCD_LINE0, "Error!");
          showText(LCD_LINE1, "Valor no valido");
          delay(2000);
          updateDisplayMode();
        }
        
        inputIndex = 0;
      }
    } 
    else if(isValidInputChar(c) && inputIndex < sizeof(inputBuffer) - 1) {
      inputBuffer[inputIndex++] = c;
    }
    else if(tolower(c) == 'd') {
      inputMode = DECIMAL;
      updateDisplayMode();
      inputIndex = 0;
    }
    else if(tolower(c) == 'o') {
      inputMode = OCTAL;
      updateDisplayMode();
      inputIndex = 0;
    }
    else if(tolower(c) == 'h') {
      inputMode = HEXADECIMAL;
      updateDisplayMode();
      inputIndex = 0;
    }
  }
}

bool isValidInputChar(char c) {
  switch(inputMode) {
    case DECIMAL:
      return isdigit(c);
    case OCTAL:
      return c >= '0' && c <= '7'; // Solo acepta dígitos octales
    case HEXADECIMAL:
      return isxdigit(c);
  }
  return false;
}

void setup() {
  LCD_init();
}

void loop() {
  processSerialInput();
}
