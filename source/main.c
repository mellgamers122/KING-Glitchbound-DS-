#include <nds.h>
#include <stdio.h>

typedef struct { int x,y,vx,vy,onGround,facing,fragments,lives; } Player;
static Player king={28,120,0,0,1,1,0,3};

int main(void){
    PrintConsole top,bottom;
    consoleInit(&top,0,BgType_Text4bpp,BgSize_T_256x256,31,0,true,true);
    consoleInit(&bottom,0,BgType_Text4bpp,BgSize_T_256x256,31,0,false,true);
    while(1){
        scanKeys(); int held=keysHeld(), down=keysDown();
        if(held&KEY_LEFT){king.x-=2;king.facing=-1;}
        if(held&KEY_RIGHT){king.x+=2;king.facing=1;}
        if((down&KEY_A)&&king.onGround){king.vy=-7;king.onGround=0;}
        if(held&KEY_B) king.x+=3*king.facing;
        king.vy++; king.y+=king.vy;
        if(king.y>=120){king.y=120;king.vy=0;king.onGround=1;}
        if(king.x<8)king.x=8; if(king.x>240)king.x=240;
        if(king.x>214&&king.fragments==0)king.fragments=1;

        consoleSelect(&top); consoleClear();
        iprintf("\x1b[1;6HKING: GLITCHBOUND");
        iprintf("\x1b[3;3HBUILD 0.1 - NEXUS AWAKENS");
        iprintf("\x1b[6;2HVIDAS: %d",king.lives);
        iprintf("\x1b[7;2HFRAGMENTOS: %d/5",king.fragments);
        iprintf("\x1b[10;2HOBJETIVO: ENCONTRE O SAVE");

        consoleSelect(&bottom); consoleClear();
        iprintf("\x1b[1;5HREINO COGUMELO.EXE");
        iprintf("\x1b[4;2HD-PAD mover  A pular  B dash");
        iprintf("\x1b[16;1H================================");
        int col=king.x/8; if(col<1)col=1; if(col>30)col=30;
        iprintf("\x1b[15;%dH@",col); iprintf("\x1b[13;28H*");
        swiWaitForVBlank();
    }
    return 0;
}
