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

    // Inizializza ESC su GPIO 18 con protocollo DSHOT600
    DShotRMT esc(GPIO_NUM_18, DSHOT600);
    esc.begin();
    printf("[INFO] ESC inizializzato con DShot600 su GPIO 18 (motore M1)\n");

    int test_count = 0;
    while (true)
    {
        test_count++;
        printf("[INFO] === CICLO #%d ===\n", test_count);

        //printf("senza arming\n");


        // Arming all'inizio di ogni ciclo per sicurezza
        /*
        printf("[INFO] Arming ESC (diversi secondi)...\n");
        for (int i = 0; i < 1000; i++)
        {
            esc.setThrottle(0);
            if (i % 200 == 0)
            {
                printf("[DEBUG] arming... %d ms\n", i * 2);
            }
            vTaskDelay(pdMS_TO_TICKS(2));
        }
            */
            
        
//Calibrazione ESC, per ora disabilitata
        /*
        printf("[INFO] Calibrazione ESC...\n");
        esc.setThrottle(2000);           // Massimo throttle
        vTaskDelay(pdMS_TO_TICKS(2000)); // 2 secondi
        esc.setThrottle(0);              // Minimo throttle
        vTaskDelay(pdMS_TO_TICKS(2000)); // 2 secondi
        */

        // Test tutte le velocità
        for (int throttle = 100; throttle <= 250; throttle += 50)
        {
            printf("[INFO] Throttle: %d - Test 5 secondi\n", throttle);
            esc.setThrottle(throttle);
            if (throttle > 200)
            {
                printf("[INFO] Throttle: %d - Test 3 secondi\n", throttle);
                for (int second = 1; second <= 5; second++)
             {
                vTaskDelay(pdMS_TO_TICKS(1000)); // 1 secondo
                printf("[DEBUG] Throttle %d - Secondo %d/3\n", throttle, second);
             }
            }
            else{
            // Print ogni secondo durante il test
             for (int second = 1; second <= 3; second++)
             {
                vTaskDelay(pdMS_TO_TICKS(1000)); // 1 secondo
                printf("[DEBUG] Throttle %d - Secondo %d/3\n", throttle, second);
             }
            }
        

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
