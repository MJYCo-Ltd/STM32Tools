/*
 * main.c
 *
 *  Created on: Dec 9, 2025
 *      Author: yty
 */
#include "Auxiliary.h"
 #include "ECSense.h"
#include <stdio.h>
int main(void) {
    printf("ECSense Test Application\n");
    ECSense_Init();
    while (1) {
        ECSense_Process();
    }
    return 0;
}
