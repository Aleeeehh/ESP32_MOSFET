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
        printf("[INFO] Arming ESC (2 secondi)...\n");
        for (int i = 0; i < 40000; i++)
        {
            esc.setThrottle(0);
            if (i % 500 == 0)
            {
                printf("[DEBUG] arming... %d ms\n", i * 2);
            }
            vTaskDelay(pdMS_TO_TICKS(2));
        }        
        

        // Salita da 100 a 500
        for (int throttle = 100; throttle <= 500; throttle += 5) 
        {
            esc.setThrottle(throttle);
            
            //1 secondo per velocità
            for (int second = 1; second <= 1; second++)
            {
                vTaskDelay(pdMS_TO_TICKS(1000)); // 1 secondo
                printf("[DEBUG] Throttle %d [SALITA]\n", throttle);
            }
        }

        // Discesa da 500 a 100
        for (int throttle = 500; throttle >= 100; throttle -= 5) 
        {
            esc.setThrottle(throttle);
            
            //1 secondo per velocità
            for (int second = 1; second <= 1; second++)
            {
                vTaskDelay(pdMS_TO_TICKS(1000)); // 1 secondo
                printf("[DEBUG] Throttle %d [DISCESA]\n", throttle);
            }
        }

        printf("[INFO] Fine ciclo completo. Pausa lunga 10 secondi...\n");
        esc.setThrottle(0);
        vTaskDelay(pdMS_TO_TICKS(10000)); // 10 secondi di pausa tra i cicli
    }

    printf("[INFO] Test completato. Loop idle.\n");

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000)); // idle per evitare reboot
    }
}
