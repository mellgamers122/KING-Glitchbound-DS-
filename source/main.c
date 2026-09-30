#include <nds.h>
#include <stdio.h>

int main(void)
{
    int presses = 0;

    consoleDemoInit();

    iprintf("KING: GLITCHBOUND\n");
    iprintf("DIAGNOSTICO DS\n\n");
    iprintf("Se voce esta lendo,\n");
    iprintf("a ROM abriu no DS.\n\n");
    iprintf("Aperte A para testar input.\n");

    while (1) {
        scanKeys();

        if (keysDown() & KEY_A) {
            presses++;
            iprintf("A detectado: %d\n", presses);
        }

        swiWaitForVBlank();
    }

    return 0;
}
