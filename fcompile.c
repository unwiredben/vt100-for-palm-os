/* Font Compiler Util */

#include <stdio.h>

#define WIDTH 4
#define HEIGHT 6
#define CCOUNT 128
#define CPERLINE 16

void die(char *s)
{
    fprintf(stderr,"%s\n",s);
    exit(1);    
}

int main(int argc, char *argv[])
{
    unsigned char font[HEIGHT][CCOUNT];
    int i,j,k,c;
    FILE *fp;
    char line[128];
    
    if(argc != 3) die("usage: fcompile <in> <out>");

        /* load font data from textfile */
    if(!(fp = fopen(argv[1],"r"))) die("error: cannot open infile");
    for(i=0;i < (CCOUNT/CPERLINE);i++){
        for(j=0;j<HEIGHT;j++){            
            fgets(line,128,fp);
            c = i*CPERLINE;            
            for(k=0;k<CPERLINE*WIDTH;k+=WIDTH){
                font[j][c++] =
                    (line[k+0] == '#' ? 0x88 : 0) |
                    (line[k+1] == '#' ? 0x44 : 0) |
                    (line[k+2] == '#' ? 0x22 : 0) |
                    (line[k+3] == '#' ? 0x11 : 0);
            }
        }
    }
    fclose(fp);

        /* write out font data as a C source file */
    if(!(fp = fopen(argv[2],"w"))) die("error: cannot open outfile");
    fprintf(fp,"#define FontWIDTH %d\n#define FontHEIGHT %d\n",WIDTH,HEIGHT);
    fprintf(fp,"#define FontCOUNT %d\n\n",CCOUNT);    
    fprintf(fp,"unsigned char FontData[FontHEIGHT][FontCOUNT] = {\n");
    for(i=0;i<HEIGHT;i++){
        fprintf(fp,"    { ");
        for(j=0;j<CCOUNT;j++){
            fprintf(fp,"0x%02x%s",font[i][j],j == CCOUNT-1 ? " },\n" : ", ");
        }
    }
    fprintf(fp,"};\n");    
    fclose(fp);
    
}
