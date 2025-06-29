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
    DShotRMT esc(GPIO_NUM_18, DSHOT600);
    esc.begin();
    printf("[INFO] ESC inizializzato con DShot600 su GPIO 33 (motore M1)\n");

    // Test graduale di tutte le velocità di throttle
    printf("[INFO] === TEST GRADUALE THROTTLE (incrementi di 100) ===\n");

    int test_count = 0;
    while (true)
    {
        test_count++;
        printf("[INFO] === CICLO #%d ===\n", test_count);

        // Arming all'inizio di ogni ciclo per sicurezza
        printf("[INFO] Arming ESC (4 secondi)...\n");
        for (int i = 0; i < 2000; i++)
        {
            esc.setThrottle(0);
            if (i % 200 == 0)
            {
                printf("[DEBUG] arming... %d ms\n", i * 2);
            }
            vTaskDelay(pdMS_TO_TICKS(2)); // 2ms x 2000 = 4000ms = 4s
        }

        // Test tutte le velocità da 500 a 2000, incrementi di 500
        for (int throttle = 500; throttle <= 2000; throttle += 500)
        {
            printf("[INFO] Throttle: %d - Test 2 secondi\n", throttle);
            esc.setThrottle(throttle);
            vTaskDelay(pdMS_TO_TICKS(2000)); // 2 secondo per ogni velocità

            printf("[INFO] Pausa 1 secondo...\n");
            esc.setThrottle(0);
            vTaskDelay(pdMS_TO_TICKS(1000)); // Pausa per raffreddare
        }

        printf("[INFO] Fine ciclo completo. Riparto da capo...\n");
        vTaskDelay(pdMS_TO_TICKS(3000)); // 3 secondi di pausa tra i cicli
    }

    printf("[INFO] Test completato. Loop idle.\n");

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000)); // idle per evitare reboot
    }
}
