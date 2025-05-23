#include <U8glib.h>

// Configuración robusta para ST7920
U8GLIB_ST7920_128X64_1X u8g(12, 11, 10); // EN, RW, RS (modo 1X para mejor sincronización)

void setup() {
  delay(100); // Espera inicial para estabilizar hardware
  
  // Fuerza inicialización en modo occidental (comando 0x30)
  u8g.begin();  
  u8g.enableCursor();  // Algunos displays necesitan esto
  u8g.setColorIndex(1);
  u8g.setFont(u8g_font_8x13B); // Fuente sólida
  
  // Comando adicional para limpiar artefactos
  u8g.firstPage();
  do {} while (u8g.nextPage());
}

void loop() {
  u8g.firstPage();
  do {
    // Dibuja un fondo limpio (opcional)
    u8g.drawBox(0, 0, 128, 64); // Fondo negro
    u8g.setColorIndex(0);        // Color blanco para texto
    
    // Texto centrado (ajusta coordenadas Y si está corrido)
    u8g.drawStr(1, 12, "yyyyyyyyyyyyyyyyyyyyyyyyyy");
    u8g.drawStr(1, 60, "xxxxxxxxxxxxxxxxxxxxxxxxx");
    
    // Dibuja un marco para referencia visual
    u8g.setColorIndex(1);
    u8g.drawFrame(0, 0, 128, 64);
  } while (u8g.nextPage());

  delay(2000);
}
