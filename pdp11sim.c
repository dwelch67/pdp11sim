
//-------------------------------------------------------------------
//-------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WATCH_READ_BYTE         1
#define WATCH_WRITE_BYTE        1
#define WATCH_FETCH             1
#define WATCH_READ_WORD         1
#define WATCH_WRITE_WORD        1
#define WATCH_READ_REGISTER     1
#define WATCH_WRITE_REGISTER    1
#define WATCH_PSW               1

const unsigned char hexchar[256]=
{
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,
0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

#define MEMMASK 0xFFFF
unsigned char mem[MEMMASK+1];
char newline[1024];
unsigned char linedata[0x110];

unsigned int reg[8];

#define N_FLAG 8
#define Z_FLAG 4
#define V_FLAG 2
#define C_FLAG 1

unsigned int psw;
unsigned int xaddr;

unsigned int start_address;

unsigned int fetch_count;
unsigned int read_count;
unsigned int write_count;
unsigned int read_reg_count;
unsigned int write_reg_count;


FILE *fp;

//-------------------------------------------------------------------
unsigned char mem_read_byte ( unsigned int address )
{
    unsigned int data;

    read_count++;
    address&=MEMMASK;
    data=mem[address];
#ifdef WATCH_READ_BYTE
    printf("mem_read_byte(0x%04X) = 0x%02X\n",address,data);
#endif
    return(data);
}
//-------------------------------------------------------------------
void mem_write_byte ( unsigned int address, unsigned int data )
{
    write_count++;
    address&=MEMMASK;
    data&=0xFF;
#ifdef WATCH_WRITE_BYTE
    printf("mem_write_byte(0x%04X,0x%02X);\n",address,data);
#endif
    mem[address]=data;
}
//-------------------------------------------------------------------
unsigned int mem_fetch ( unsigned int address )
{
    unsigned int data;

    fetch_count++;
    address&=MEMMASK;
    if(address&1)
    {
        printf("ERROR unaligned mem_fetch(0x%04X);\n",address);
        exit(1);
    }
    data=mem[address+1];
    data<<=8;
    data|=mem[address+0];
#ifdef WATCH_FETCH
    printf("\nmem_fetch(0x%04X) = 0x%04X\n",address,data);
#endif
    return(data);
}
//-------------------------------------------------------------------
unsigned int mem_read_word ( unsigned int address )
{
    unsigned int data;

    read_count+=2;
    address&=MEMMASK;
    if(address&1)
    {
        printf("ERROR unaligned mem_read_word(0x%04X);\n",address);
        exit(1);
    }
    data=mem[address+1];
    data<<=8;
    data|=mem[address+0];
#ifdef WATCH_READ_WORD
    printf("mem_read_word(0x%04X) = 0x%04X\n",address,data);
#endif
    return(data);
}
//-------------------------------------------------------------------
void mem_write_word ( unsigned int address, unsigned int data )
{
    write_count+=2;

    address&=MEMMASK;
    data&=0xFFFF;
    if(address&1)
    {
        printf("ERROR unaligned mem_write_word(0x%04X);\n",address);
        exit(1);
    }
#ifdef WATCH_WRITE_WORD
    printf("mem_write_word(0x%04X,0x%04X);\n",address,data);
#endif
    mem[address]=data;
}
//-------------------------------------------------------------------
unsigned int read_register ( unsigned int rn )
{
    unsigned int data;

    read_reg_count++;
    rn&=7;
    data=reg[rn];
#ifdef WATCH_READ_REGISTER
    printf("read_register(r%u) = 0x%04X\n",rn,data);
#endif
    return(data);
}
//-------------------------------------------------------------------
unsigned int write_register ( unsigned int rn, unsigned int data)
{
    write_reg_count++;
    rn&=7;
    data&=0xFFFF;
#ifdef WATCH_WRITE_REGISTER
    printf("write_register(r%u,0x%04X);\n",rn,data);
#endif
    reg[rn]=data;
}
//-------------------------------------------------------------------
void set_new_psw ( unsigned int x )
{
    psw=x;
#ifdef WATCH_PSW
    printf("psw=0x%04X ",psw);
    if(psw&N_FLAG) printf("N"); else printf("n");
    if(psw&Z_FLAG) printf("Z"); else printf("z");
    if(psw&V_FLAG) printf("V"); else printf("v");
    if(psw&C_FLAG) printf("C"); else printf("c");
    printf("\n");
#endif
}
//-------------------------------------------------------------------
void reset_cpu ( void )
{
    printf("\n---- reset ----\n");

    reg[0]=0;
    reg[1]=0;
    reg[2]=0;
    reg[3]=0;
    reg[4]=0;
    reg[5]=0;
    reg[6]=0xE000; //stack pointer
    reg[7]=start_address; //0x0200; //program counter
    psw=0;

    fetch_count=0;
    read_count=0;
    write_count=0;
    read_reg_count=0;
    write_reg_count=0;
}
//-------------------------------------------------------------------
unsigned int get_data ( unsigned int xmode, unsigned int xreg, unsigned int size )
{
    unsigned int data;
    unsigned int rdata;
    unsigned int pc;

    xmode&=7;
    xreg&=7;
    switch(xmode)
    {
        case 0:
        {
            //OPR Rn
            data=read_register(xreg);
            break;
        }
        case 1:
        {
            //OPR (Rn)
            data=read_register(xreg);
            if(size) data=mem_read_byte(data);
            else     data=mem_read_word(data);
            break;
        }
        case 2:
        {
            if(xreg==7) size=0;
            //OPR (Rn)+;
            rdata=read_register(xreg);
            if(size)
            {
                data=mem_read_byte(rdata);
                rdata++;
                write_register(xreg,rdata);
            }
            else
            {
                data=mem_read_word(rdata);
                rdata+=2;
                write_register(xreg,rdata);
            }
            break;
        }
        case 3:
        {
            if(xreg==7) size=0;
            rdata=read_register(xreg);
            data=mem_read_word(rdata);
            if(size)
            {
                data=mem_read_byte(data);
            }
            else
            {
                data=mem_read_word(data);
            }
            rdata+=2;
            write_register(xreg,rdata);
            break;
        }
        case 4:
        {
            rdata=read_register(xreg);
            if(size)
            {
                rdata--;
                write_register(xreg,rdata);
                data=mem_read_byte(rdata);
            }
            else
            {
                rdata-=2;
                write_register(xreg,rdata);
                data=mem_read_word(rdata);
            }
            break;
        }
        case 5:
        {
            rdata=read_register(xreg);
            rdata-=2;
            write_register(xreg,rdata);
            data=mem_read_word(rdata);
            if(size)
            {
                data=mem_read_byte(data);
            }
            else
            {
                data=mem_read_word(data);
            }
            break;
        }
        case 6:
        {
            if(xreg==7) size=0;
            pc=read_register(7);
            xaddr=mem_read_word(pc);
            pc+=2; //here or after?
            write_register(7,pc); //here or after, what if xreg is r7?
            rdata=read_register(xreg);
            data=rdata+xaddr;
            if(size)
            {
                data=mem_read_byte(data);
            }
            else
            {
                data=mem_read_word(data);
            }
            break;
        }
        case 7:
        {
            if(xreg==7) size=0;
            pc=read_register(7);
            xaddr=mem_read_word(pc);
            pc+=2; //here or after?
            write_register(7,pc); //here or after, what if xreg is r7?
            rdata=read_register(xreg);
            data=rdata+xaddr;
            data=mem_read_word(data);
            if(size)
            {
                data=mem_read_byte(data);
            }
            else
            {
                data=mem_read_word(data);
            }
            break;
        }
    }
    return(data);
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
unsigned int put_data ( unsigned int xmode, unsigned int xreg, unsigned int data, unsigned int xaddr, unsigned int size )
{
    unsigned int rdata;
    unsigned int xdata;
    unsigned int pc;

    xmode&=7;
    xreg&=7;
    switch(xmode)
    {
        case 0:
        {
            write_register(xreg,data);
            break;
        }
        case 1:
        {
            rdata=read_register(xreg);
            if(size) mem_write_byte(rdata,data);
            else     mem_write_word(rdata,data);
            break;
        }
        case 2:
        {
            if(xreg==7) size=0;
            rdata=read_register(xreg);
            if(size)
            {
                mem_write_byte(rdata,data);
                rdata++;
                write_register(xreg,rdata);
            }
            else
            {
                mem_write_word(rdata,data);
                rdata+=2;
                write_register(xreg,rdata);
            }
            break;
        }
        case 3:
        {
            if(xreg==7) size=0;
            rdata=read_register(xreg);
            xdata=mem_read_word(rdata);
            if(size)
            {
                mem_write_byte(xdata,data);
            }
            else
            {
                mem_write_word(xdata,data);
            }
            rdata+=2;
            write_register(xreg,rdata);
            break;
        }
        case 4:
        {
            rdata=read_register(xreg);
            if(size)
            {
                rdata--;
                write_register(xreg,rdata);
                mem_write_byte(rdata,data);
            }
            else
            {
                rdata-=2;
                write_register(xreg,rdata);
                mem_write_word(rdata,data);
            }
            break;
        }
        case 5:
        {
            rdata=read_register(xreg);
            rdata-=2;
            write_register(xreg,rdata);
            xdata=mem_read_word(rdata);
            if(size)
            {
                mem_write_byte(xdata,data);
            }
            else
            {
                mem_write_word(xdata,data);
            }
            break;
        }
        case 6:
        {
            if(xreg==7) size=0;
            //pc=read_register(7);
            //xaddr=mem_read_word(pc);
            //pc+=2; //here or after?
            //write_register(7,pc); //here or after, what if xreg is r7?
            rdata=read_register(xreg);
            xdata=rdata+xaddr;
            if(size)
            {
                mem_write_byte(xdata,data);
            }
            else
            {
                mem_write_word(xdata,data);
            }
            break;
        }
        case 7:
        {
            if(xreg==7) size=0;
            //pc=read_register(7);
            //xaddr=mem_read_word(pc);
            //pc+=2; //here or after?
            //write_register(7,pc); //here or after, what if xreg is r7?
            rdata=read_register(xreg);
            xdata=rdata+xaddr;
            xdata=mem_read_word(xdata);
            if(size)
            {
                mem_write_byte(xdata,data);
            }
            else
            {
                mem_write_word(xdata,data);
            }
            break;
        }
    }
    return(data);
}
//-------------------------------------------------------------------
unsigned int execute ( void )
{
    unsigned int pc;
    unsigned int pc_base;
    unsigned int inst;
    unsigned int smode;
    unsigned int sreg;
    unsigned int sdata;
    unsigned int dmode;
    unsigned int dreg;
    unsigned int ddata;
    unsigned int saddr;
    unsigned int daddr;
    unsigned int result;
    unsigned int newpsw;


    //pc=reg[7]; //hide no stats
    pc=read_register(7);
    pc_base=pc;
    if(pc&1)
    {
        printf("Error invalid pc 0x%04X\n",pc);
        return(1);
    }
    inst=mem_fetch(pc);
    pc+=2;
    //reg[7]=pc; //hide no stats
    write_register(7,pc);
    switch((inst>>12)&0xF)
    {
        case 0x6: //ADD
        {
            smode=(inst>>9)&7;
            sreg=(inst>>6)&7;
            dmode=(inst>>3)&7;
            dreg=(inst>>0)&7;
            //408:  6cf4 0004 0006  add 4(r3), 6(r4)
            sdata=get_data(smode,sreg,0); saddr=xaddr;
            ddata=get_data(dmode,dreg,0); daddr=xaddr;
            result=ddata+sdata;
            newpsw=0;
            if(result&0x8000) newpsw|=N_FLAG;
            if(result==0) newpsw|=Z_FLAG;
            if(result&0x10000) newpsw|=C_FLAG;
            if(((ddata&0x8000)==(sdata&0x8000))&&((sdata&0x8000)!=(result&0x8000)))
                newpsw|=V_FLAG;
            set_new_psw(newpsw);
            result&=0xFFFF;
            put_data(dmode,dreg,result,daddr,0);
            break;
        }
        case 0xE: //SUB
        {
            smode=(inst>>9)&7;
            sreg=(inst>>6)&7;
            dmode=(inst>>3)&7;
            dreg=(inst>>0)&7;
            //e:    e000            sub r0, r0
            sdata=get_data(smode,sreg,0); saddr=xaddr;
            ddata=get_data(dmode,dreg,0); daddr=xaddr;
            result=ddata-sdata;
            newpsw=0;
            if(result&0x8000) newpsw|=N_FLAG;
            if(result==0) newpsw|=Z_FLAG;
            if(result&0x10000) newpsw|=C_FLAG;
            if(((ddata&0x8000)!=(sdata&0x8000))&&((sdata&0x8000)==(result&0x8000)))
                newpsw|=V_FLAG;
            set_new_psw(newpsw);
            result&=0xFFFF;
            put_data(dmode,dreg,result,daddr,0);
            break;
        }
        default:
        {
            printf("Error unknown opcode 0x%04X\n",inst);
            return(1);
        }
    }
    return(0);
}
//-------------------------------------------------------------------
unsigned int readhex ( void )
{
    unsigned int ra;
    unsigned int rb;
    unsigned int rc;
    unsigned int rd;
    unsigned int line;
    unsigned int length;
    unsigned int address;
    unsigned int type;
    unsigned int checksum;

    memset(mem,0xFF,sizeof(mem));

    start_address=0x0000;
    line=0;
    while(fgets(newline,sizeof(newline)-1,fp))
    {
        line++;
        if(newline[0]!=':')
        {
            printf("Skipping line %u\n",line);
        }
        for(ra=0;newline[ra];ra++) if(newline[ra]<=0x20) break;
        newline[ra]=0;
        for(rb=1;rb<ra;rb+=2)
        {
            if(!hexchar[newline[rb+0]])
            {
                printf("Bad character in line %u 0x%02X 0x%02X\n",line,rb,newline[rb+0]);
                return(1);
            }
            rd=(unsigned int)newline[rb+0];
            if(rd>0x39) rd-=7;
            rd&=0xF;
            rc=rd<<4;
            if(!hexchar[newline[rb+1]])
            {
                printf("Bad character in line %u 0x%02X 0x%02X\n",line,rb,newline[rb+0]);
                return(1);
            }
            rd=(unsigned int)newline[rb+1];
            if(rd>0x39) rd-=7;
            rd&=0xF;
            rc|=rd;
            linedata[rb>>1]=rc&0xFF;
        }
        rb>>=1;
        if(rb!=(1+linedata[0]+3+1))
        {
            printf("Bad line length %u\n",line);
            return(1);
        }
        rc=0; for(ra=0;ra<rb;ra++) rc+=linedata[ra]; rc&=0xFF;
        if(rc)
        {
            printf("Checksum error %u\n",line);
        }
        length=linedata[0];
        address=linedata[1]; address<<=8; address|=linedata[2];
        type=linedata[3];
        switch(type)
        {
            case 0x00:
            {
                for(ra=0;ra<length;ra++)
                {
                    printf("0x%04X: 0x%02X\n",address,linedata[ra+4]);
                    mem[address&MEMMASK]=linedata[ra+4];
                    address++;
                }
                break;
            }
            case 0x01:
            {
                //end of file
                break;
            }
            case 0x03:
            {
                start_address=linedata[6]; start_address<<=8; start_address|=linedata[7];
                break;
            }
            default:
            {
                printf("Unknown line type %u 0x%02X\n",line,type);
            }
        }
        if(type==0x01) break;
    }

printf("start_address 0x%04X\n",start_address);
    return(0);
}
//-------------------------------------------------------------------
int main ( int argc, char *argv[] )
{
    unsigned int ra;

    if(argc<2)
    {
        printf(".hex file not specified\n");
        return(1);
    }
    fp=fopen(argv[1],"rt");
    if(fp==NULL)
    {
        printf("Error opening file [%s]\n",argv[1]);
        return(1);
    }
    ra=readhex();
    fclose(fp);
    if(ra) return(1);
    reset_cpu();

ra=0;
    while(1)
    {
if(++ra>30) break;
        if(execute()) break;
    }


    printf("\n--------\n");
    printf("fetch_count     %u\n",fetch_count    );
    printf("read_count      %u\n",read_count     );
    printf("write_count     %u\n",write_count    );
    printf("read_reg_count  %u\n",read_reg_count );
    printf("write_reg_count %u\n",write_reg_count);

    return(0);
}



//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------



