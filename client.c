#include<stdio.h>

void createSpreadsheet();

int main() {
    createSpreadsheet();
    return 0;
}

void createSpreadsheet() {
    char * const COLUMN_HEADINGS = "      A        B        C        D        E        F        G        H        I";
    char * const HORIZONTAL_LINE = "  +--------+--------+--------+--------+--------+--------+--------+--------+--------+";
    char * const VERTICAL__LINES = "|        |        |        |        |        |        |        |        |        |";
    
    printf("%s\n", COLUMN_HEADINGS);
    for(int i = 0; i < 9; i++) {
        printf("%s\n", HORIZONTAL_LINE);
        printf("%d %s\n",i+1, VERTICAL__LINES);
    }
    printf("%s\n", HORIZONTAL_LINE);
}