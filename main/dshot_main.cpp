#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "DShotRMT.h" // include anche dshot_send_command + DSHOT_CMD_BEACONx

extern "C" void app_main()
{
    printf("[BOOT] ESP32 avviato\n");

    // Disabilita il task watchdog per evitare reset durante il debug
    esp_task_wdt_deinit();
    printf("[INFO] Task watchdog disabilitato\n");

    // Inizializza ESC su GPIO 33 con protocollo DSHOT600
    DShotRMT esc(GPIO_NUM_33, DSHOT600);
    esc.begin();
    printf("[INFO] ESC inizializzato con DShot600 su GPIO 33 (motore M1)\n");

    // Arming: invia throttle 0 per 2 secondi (obbligatorio per ESC Bluejay)
    printf("[INFO] Arming ESC Bluejay (2 secondi)...\n");
    for (int i = 0; i < 1000; i++)
    {
        esc.setThrottle(0);
        vTaskDelay(pdMS_TO_TICKS(2)); // 2ms x 1000 = 2s
    }

    // Test diagnostico sistematico
    printf("[INFO] === TEST DIAGNOSTICO COMUNICAZIONE ESC ===\n");

    int test_count = 0;
    while (true)
    {
        test_count++;
        printf("[INFO] === TEST #%d ===\n", test_count);

        // Test 1: Throttle molto basso per vedere se il motore si muove
        printf("[INFO] Test 1: Throttle basso (100) - Il motore dovrebbe muoversi leggermente\n");
        esc.setThrottle(100);
        vTaskDelay(pdMS_TO_TICKS(2000)); // 2 secondi per osservare

        // Test 2: Throttle medio
        printf("[INFO] Test 2: Throttle medio (500) - Il motore dovrebbe girare più veloce\n");
        esc.setThrottle(500);
        vTaskDelay(pdMS_TO_TICKS(2000)); // 2 secondi per osservare

        // Test 3: Throttle alto
        printf("[INFO] Test 3: Throttle alto (1000) - Il motore dovrebbe girare veloce\n");
        esc.setThrottle(1000);
        vTaskDelay(pdMS_TO_TICKS(2000)); // 2 secondi per osservare

        // Test 4: Torna a zero
        printf("[INFO] Test 4: Torna a zero - Il motore dovrebbe fermarsi\n");
        esc.setThrottle(0);
        vTaskDelay(pdMS_TO_TICKS(2000)); // 2 secondi per osservare

        printf("[INFO] Fine ciclo test. Se il motore non si muove, c'è un problema di comunicazione.\n");
        printf("[INFO] Controlla: cablaggio, alimentazione ESC, configurazione DShot.\n");
        vTaskDelay(pdMS_TO_TICKS(3000)); // 3 secondi di pausa
    }

    printf("[INFO] Test completato. Loop idle.\n");

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000)); // idle per evitare reboot
    }
}
