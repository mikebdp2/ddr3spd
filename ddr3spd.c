/* ======= DDR3SPD: ======= */
/* gcc -o ddr3spd ddr3spd.c */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <linux/ioctl.h>
#include <linux/types.h>

static void say(unsigned char *stuff) {
    /* UNFORMATTED STUFF ONLY! */
    fprintf(stdout, "%s", stuff);
      fflush(stdout);
}

#define SPD_MEMORY_SIZE  256

static unsigned long int checksize(FILE *pFile, unsigned const char *str) {
    fseek(pFile, 0L, SEEK_END);
    unsigned long int filesize = ftell(pFile);
    if (filesize != SPD_MEMORY_SIZE) {
        fprintf(stdout, "ERROR: %s size is %li bytes, should be %i\n",
                        str, filesize, SPD_MEMORY_SIZE);
          fflush(stdout);
        return 1;
    }
    return 0;
}

static unsigned long int rotate(unsigned long int value, unsigned long int size) {
    unsigned long int i, j;
    unsigned char *c_in, *c_out;
    unsigned long int retval = 0x0;
    c_in = (unsigned char *)&value;
    c_out = (unsigned char *)&retval;
    for (i = 0, j = size - 1; i < size; i++, j--)
        c_out[i] = c_in[j];
    return retval;
}

static unsigned int print_value(FILE *pFile, unsigned char* name, unsigned int offset,
        unsigned int size, unsigned int rotflag, unsigned int asciiflag) {
    unsigned long int i;
    unsigned char c;
    unsigned int value = 0;
    rewind(pFile);
    fseek(pFile, offset, SEEK_SET);
    if (asciiflag) {
        fprintf(stdout, "0x%02X | ---> | %s : ", offset, name);
        for (i = 0; i < size; i++) {
            fread(&c, 1, 1, pFile);
            fprintf(stdout, "%c", c);
        }
        say("\n");
        return 0;
    }
    else {
        fread(&value, size, 1, pFile);
        if (rotflag) value = rotate(value, size);
        if (value < 0x100)
            fprintf(stdout, "0x%02X | 0x%02X | %s : ", offset, value, name);
        else
            fprintf(stdout, "0x%02X | ---> | %s : ", offset, name);
          fflush(stdout);
        return value;
    }
}

#define BITS_IN_BYTE       8

static unsigned int bits(unsigned int value,
        unsigned int highbit, unsigned int lowbit) {
    unsigned int coolbit = ((BITS_IN_BYTE * sizeof(unsigned int)) - highbit - 1);
    return ((value << coolbit) >> (coolbit + lowbit));
}

#define SPD_BYTE_USED      0
#define SPD_REVISION       1    /* EXTRA */
#define SPD_TYPE           2    /* SPD byte read location */
#define SPD_DIMM_TYPE      3

#define SPD_DENSITY        4    /* bits [3:0] */
#define SPD_L_BANKS        4    /* bits [7:4] */

#define SPD_COL_SZ         5    /* bits [2:0] */
#define SPD_ROW_SZ         5    /* bits [5:3] */

#define SPD_VOLTAGE        6    /* EXTRA */
#define XMP1_VOLTAGE     185

#define SPD_DEV_WIDTH      7    /* bits [2:0] */
#define SPD_RANKS          7    /* bits [5:3] */

#define SPD_BUS_WIDTH      8    /* bits [2:0] */
#define SPD_ECCBITS        8    /* bits [4:3] */

#define SPD_FTB            9

#define SPD_DIVIDENT      10
#define XMP1_DIVIDENT    180

#define SPD_DIVISOR       11
#define XMP1_DIVISOR     181

#define SPD_TCK           12
#define XMP1_TCK         186

#define SPD_CASLO         14
#define XMP1_CASLO       188
#define SPD_CASHI         15
#define XMP1_CASHI       189

#define SPD_TAA           16
#define XMP1_TAA         187

#define SPD_TWR           17
#define XMP1_TWR         193

#define SPD_TRCD          18
#define XMP1_TRCD        192

#define SPD_TRRD          19
#define XMP1_TRRD        202

#define SPD_TRP           20
#define XMP1_TRP         191

#define SPD_UPPER_TRAS    21    /* bits [3:0] */
#define XMP1_UPPER_TRAS  194    /* bits [3:0] */
#define SPD_UPPER_TRC     21    /* bits [7:4] */
#define XMP1_UPPER_TRC   194    /* bits [7:4] */
#define SPD_TRAS          22
#define XMP1_TRAS        195
#define SPD_TRC           23
#define XMP1_TRC         196

#define SPD_TRFC_LO       24
#define XMP1_TRFC_LO     199
#define SPD_TRFC_HI       25
#define XMP1_TRFC_HI     200

#define SPD_TWTR          26
#define XMP1_TWTR        205

#define SPD_TRTP          27
#define XMP1_TRTP        201

#define SPD_UPPER_TFAW    28    /* bits [3:0] */
#define XMP1_UPPER_TFAW  203
#define SPD_TFAW          29
#define XMP1_TFAW        204

#define SPD_TCK_FTB       34
#define SPD_TAA_FTB       35
#define SPD_TRCD_FTB      36
#define SPD_TRP_FTB       37
#define SPD_TRC_FTB       38

#define SPD_RAWCARD       62
#define SPD_ADDRMAP       63

#define SPD_CTLWRD03      70    /* bits [7:4] */
#define SPD_CTLWRD04      71    /* bits [3:0] */
#define SPD_CTLWRD05      71    /* bits [7:4] */

#define SERIALNUM        122
#define SERIALNUM_SIZE     4

#define PARTNUM          128
#define PARTNUM_SIZE      17

#define XMP_ID_LO        176
#define XMP_ID_LO_VAL   0x0C
#define XMP_ID_HI        177
#define XMP_ID_HI_VAL   0x4A
#define XMP_ID_VAL      (XMP_ID_HI_VAL << 8) + XMP_ID_LO_VAL

#define XMP_PROFILES     178
#define XMP1_CMD_RATE    208

static void print_info(FILE *pFile) {
    unsigned int value, i, sdram_capacity, sdram_width, ranks, primary_bus_width,
        total_capacity, spd_divident, spd_divisor, xmp1_divident, xmp1_divisor, spd_tck,
        xmp1_tck, spd_clock, xmp1_clock, spd_taa, xmp1_taa, spd_twr, xmp1_twr, spd_trcd,
        xmp1_trcd, spd_trrd, xmp1_trrd, spd_trp, xmp1_trp, spd_tras_upper, spd_tras_lower,
        spd_tras, xmp1_tras_upper, xmp1_tras_lower, xmp1_tras, spd_trc_upper,
        spd_trc_lower, spd_trc, xmp1_trc_upper, xmp1_trc_lower, xmp1_trc, spd_trfc_upper,
        spd_trfc_lower, spd_trfc, xmp1_trfc_upper, xmp1_trfc_lower, xmp1_trfc, spd_twtr,
        xmp1_twtr, spd_trtp, xmp1_trtp, spd_tfaw_upper, spd_tfaw_lower, spd_tfaw,
        xmp1_tfaw_upper, xmp1_tfaw_lower, xmp1_tfaw, xmp_id_upper, xmp_id_lower, xmp_id,
        xmp1_cmd_rate;
      say("ADDR | VALUE | MEANING\n");
      say("----------------------\n");
/* ==================================================================================== */
    value = print_value(pFile, "SPD_BYTE_USED", SPD_BYTE_USED, 1, 0, 0);
    switch(bits(value, 3, 0)) {
         case 0: say(" ? "); break;
         case 1: say("128"); break;
         case 2: say("176"); break;
         case 3: say("256"); break;
        default: say("???"); break;
    }
      say(" bytes used of ");
    switch(bits(value, 6, 4)) {
         case 0: say(" ? "); break;
         case 1: say("256"); break;
        default: say("???"); break;
    }
      say(", CRC covers ");
    switch(bits(value, 7, 7)) {
         case 0: say("0-125 (+ID)"); break;
        default: say("0-116 (-ID)"); break;
    }
      say("\n");
/* ==================================================================================== */
    value = print_value(pFile, "SPD_REVISION", SPD_REVISION, 1, 0, 0);
      fprintf(stdout, "%i.%i\n", bits(value, 7, 4), bits(value, 3, 0));
/* ==================================================================================== */
    value = print_value(pFile, "SPD_TYPE", SPD_TYPE, 1, 0, 0);
    switch(bits(value, 7, 0)) {
         case  0: say(" ? "); break;
         case  1: say("Standard FPM DRAM"); break;
         case  2: say("EDO"); break;
         case  3: say("Pipelined Nibble"); break;
         case  4: say("SDRAM"); break;
         case  5: say("ROM"); break;
         case  6: say("DDR SGRAM"); break;
         case  7: say("DDR SDRAM"); break;
         case  8: say("DDR2 SDRAM"); break;
         case  9: say("DDR2 SDRAM FB-DIMM"); break;
         case 10: say("DDR2 SDRAM FB-DIMM PROBE"); break;
         case 11: say("DDR3 SDRAM"); break;
        default: say("???"); break;
    }
      say("\n");
/* ==================================================================================== */
    value = print_value(pFile, "SPD_DIMM_TYPE", SPD_DIMM_TYPE, 1, 0, 0);
    switch(bits(value, 3, 0)) {
         case  0: say(" ? "); break;
         case  1: say("RDIMM (133.35 mm)"); break;
         case  2: say("UDIMM (133.35 mm)"); break;
         case  3: say("SO-DIMM (67.6 mm)"); break;
         case  4: say("Micro-DIMM (45.5 mm)"); break;
         case  5: say("Mini-RDIMM (82.0 mm)"); break;
         case  6: say("Mini-UDIMM (82.0 mm)"); break;
         case  7: say("Mini-CDIMM (67.6 mm)"); break;
         case  8: say("72b-SO-UDIMM (67.6 mm)"); break;
         case  9: say("72b-SO-RDIMM (67.6 mm)"); break;
         case 10: say("72b-SO-CDIMM (67.6 mm)"); break;
         case 11: say("LRDIMM (133.35 mm)"); break;
         case 12: say("16b-SO-DIMM (67.6 mm)"); break;
         case 13: say("32b-SO-DIMM (67.6 mm)"); break;
        default: say("???"); break;
    }
      say("\n");
/* ==================================================================================== */
    value = print_value(pFile, "[3:0] SPD_DENSITY", SPD_DENSITY, 1, 0, 0);
    sdram_capacity = bits(value, 3, 0);
    switch(bits(value, 3, 0)) {
         case  0: say("256 Mega"); sdram_capacity = 256; break;
         case  1: say("512 Mega"); sdram_capacity = 512; break;
         case  2: say("1 Giga");   sdram_capacity = 1024; break;
         case  3: say("2 Giga");   sdram_capacity = 2048; break;
         case  4: say("4 Giga");   sdram_capacity = 4096; break;
         case  5: say("8 Giga");   sdram_capacity = 8192; break;
         case  6: say("16 Giga");  sdram_capacity = 16384; break;
        default: say("???"); sdram_capacity = 0; break;
    }
      say("bits\n");
/* ==================================================================================== */
    value = print_value(pFile, "[7:4] SPD_L_BANKS", SPD_L_BANKS, 1, 0, 0);
    switch(bits(value, 7, 4)) {
         case  0: say("2^3 = 8"); break;
         case  1: say("2^4 = 16"); break;
         case  2: say("2^5 = 32"); break;
         case  3: say("2^6 = 64"); break;
        default: say("???"); break;
    }
      say(" banks\n");
/* ==================================================================================== */
    value = print_value(pFile, "[2:0] SPD_COL_SZ", SPD_COL_SZ, 1, 0, 0);
    switch(bits(value, 2, 0)) {
         case  0: say("9"); break;
         case  1: say("10"); break;
         case  2: say("11"); break;
         case  3: say("12"); break;
        default: say("???"); break;
    }
      say(" column address bits\n");
/* ==================================================================================== */
    value = print_value(pFile, "[5:3] SPD_ROW_SZ", SPD_ROW_SZ, 1, 0, 0);
    switch(bits(value, 5, 3)) {
         case  0: say("12"); break;
         case  1: say("13"); break;
         case  2: say("14"); break;
         case  3: say("15"); break;
         case  4: say("16"); break;
        default: say("???"); break;
    }
      say(" row address bits\n");
/* ==================================================================================== */
    value = print_value(pFile, "SPD_VOLTAGE", SPD_VOLTAGE, 1, 0, 0);
    switch(bits(value, 1, 0)) {
         case  0: say("1.5V "); break;
         case  1: say(""); break;
        default: say("???"); break;
    }
    switch(bits(value, 2, 1)) {
         case  0: say(""); break;
         case  1: say("1.35V "); break;
        default: say("???"); break;
    }
    switch(bits(value, 2, 1)) {
         case  0: say(""); break;
         case  1: say("1.25V "); break;
        default: say("???"); break;
    }
      say("acceptable voltages\n");
/* ==================================================================================== */
    value = print_value(pFile, "[2:0] SPD_DEV_WIDTH", SPD_DEV_WIDTH, 1, 0, 0);
    switch(bits(value, 2, 0)) {
         case  0: say("4");  sdram_width = 4; break;
         case  1: say("8");  sdram_width = 8; break;
         case  2: say("16"); sdram_width = 16; break;
         case  3: say("32"); sdram_width = 32; break;
        default: say("???"); sdram_width = 0; break;
    }
      say(" bits\n");
/* ==================================================================================== */
    value = print_value(pFile, "[5:3] SPD_RANKS", SPD_RANKS, 1, 0, 0);
    switch(bits(value, 5, 3)) {
         case  0: say("1"); ranks = 1; break;
         case  1: say("2"); ranks = 2; break;
         case  2: say("3"); ranks = 3; break;
         case  3: say("4"); ranks = 4; break;
         case  4: say("8"); ranks = 8; break;
        default: say("???"); ranks = 0; break;
    }
      say(" ranks\n");
/* ==================================================================================== */
    value = print_value(pFile, "[2:0] SPD_BUS_WIDTH", SPD_BUS_WIDTH, 1, 0, 0);
    switch(bits(value, 2, 0)) {
         case  0: say("8");  primary_bus_width = 8; break;
         case  1: say("16"); primary_bus_width = 16; break;
         case  2: say("32"); primary_bus_width = 32; break;
         case  3: say("64"); primary_bus_width = 64; break;
        default: say("???"); primary_bus_width = 0; break;
    }
      say(" bits\n");
/* ==================================================================================== */
    value = print_value(pFile, "[5:3] SPD_ECCBITS", SPD_ECCBITS, 1, 0, 0);
    switch(bits(value, 5, 3)) {
         case  0: say("0 - no"); break;
         case  1: say("1 - 8 bits ECC"); break;
        default: say("???"); break;
    }
      say(" extension\n");
/* ==================================================================================== */
    if (sdram_width != 0)
        total_capacity = (ranks * primary_bus_width * sdram_capacity)/(8 * sdram_width);
    else
        total_capacity = 0;
    fprintf(stdout, " Module capacity = %i GigaBytes\n", total_capacity/1024);
/* ==================================================================================== */
    value = print_value(pFile, "SPD_FTB", SPD_FTB, 1, 0, 0);
      fprintf(stdout, "%i/%i ps\n", bits(value, 7, 4), bits(value, 3, 0));
/* ==================================================================================== */
    spd_divident = print_value(pFile, "SPD_DIVIDENT", SPD_DIVIDENT, 1, 0, 0);
      fprintf(stdout, "%i\n", spd_divident);
    spd_divisor = print_value(pFile, "SPD_DIVISOR", SPD_DIVISOR, 1, 0, 0);
      fprintf(stdout, "%i\n", spd_divisor);
    fprintf(stdout, " SPD's Medium Timebase MTB = %i/%i ns\n", spd_divident, spd_divisor);
/* ==================================================================================== */
    spd_tck = print_value(pFile, "SPD_TCK", SPD_TCK, 1, 0, 0);
      fprintf(stdout, "%i\n", spd_tck);
    if ((spd_tck != 0) && (spd_divident != 0))
        spd_clock = (1000 * spd_divisor)/(spd_tck * spd_divident);
    else
        spd_clock = 0;
      fprintf(stdout, " SPD's Clock = %i MHz - %i MT/s\n", spd_clock, 2 * spd_clock);
/* ==================================================================================== */
    value = print_value(pFile, "SPD_CASLO", SPD_CASLO, 1, 0, 0);
    if (value != 0) {
        for (i = 0; i < 8; i++) {
            switch(bits(value, i, i)) {
                case 0: break;
                case 1: fprintf(stdout, "%i, ", i+4); break;
                break;
            }
        }
        say("- CAS latencies supported\n");
    }
    else {
        say("-\n");
    }
/* ==================================================================================== */
    value = print_value(pFile, "SPD_CASHI", SPD_CASHI, 1, 0, 0);
    if (value != 0) {
        for (i = 0; i < 7; i++) {
            switch(bits(value, i, i)) {
                case 0: break;
                case 1: fprintf(stdout, "%i, ", i+12); break;
                break;
            }
        }
        say("- CAS latencies supported\n");
    }
    else {
        say("-\n");
    }
/* ==================================================================================== */
    spd_taa = print_value(pFile, "SPD_TAA", SPD_TAA, 1, 0, 0);
      fprintf(stdout, "%i\n", spd_taa);
/* ==================================================================================== */
    spd_twr = print_value(pFile, "SPD_TWR", SPD_TWR, 1, 0, 0);
      fprintf(stdout, "%i\n", spd_twr);
/* ==================================================================================== */
    spd_trcd = print_value(pFile, "SPD_TRCD", SPD_TRCD, 1, 0, 0);
      fprintf(stdout, "%i\n", spd_trcd);
/* ==================================================================================== */
    spd_trrd = print_value(pFile, "SPD_TRRD", SPD_TRRD, 1, 0, 0);
      fprintf(stdout, "%i\n", spd_trrd);
/* ==================================================================================== */
    spd_trp = print_value(pFile, "SPD_TRP", SPD_TRP, 1, 0, 0);
      fprintf(stdout, "%i\n", spd_trp);
/* ==================================================================================== */
    value = print_value(pFile, "[3:0] SPD_UPPER_TRAS", SPD_UPPER_TRAS, 1, 0, 0);
    spd_tras_upper = bits(value, 3, 0);
      fprintf(stdout, "%i\n", spd_tras_upper);
/* ==================================================================================== */
    value = print_value(pFile, "[7:4] SPD_UPPER_TRC", SPD_UPPER_TRC, 1, 0, 0);
    spd_trc_upper = bits(value, 7, 4);
      fprintf(stdout, "%i\n", spd_trc_upper);
/* ==================================================================================== */
    spd_tras_lower = print_value(pFile, "SPD_TRAS", SPD_TRAS, 1, 0, 0);
      fprintf(stdout, "%i\n", spd_tras_lower);
/* ==================================================================================== */
    spd_trc_lower = print_value(pFile, "SPD_TRC", SPD_TRC, 1, 0, 0);
      fprintf(stdout, "%i\n", spd_trc_lower);
/* ==================================================================================== */
    spd_tras = (spd_tras_upper << 8) + spd_tras_lower;
      fprintf(stdout, " SPD_TRAS = 0x%04X = %i\n", spd_tras, spd_tras);
/* ==================================================================================== */
    spd_trc = (spd_trc_upper << 8) + spd_trc_lower;
      fprintf(stdout, " SPD_TRC = 0x%04X = %i\n", spd_trc, spd_trc);
/* ==================================================================================== */
    spd_trfc_lower = print_value(pFile, "SPD_TRFC_LO", SPD_TRFC_LO, 1, 0, 0);
      fprintf(stdout, "%i\n", spd_trfc_lower);
/* ==================================================================================== */
    spd_trfc_upper = print_value(pFile, "SPD_TRFC_HI", SPD_TRFC_HI, 1, 0, 0);
      fprintf(stdout, "%i\n", spd_trfc_upper);
/* ==================================================================================== */
    spd_trfc = (spd_trfc_upper << 8) + spd_trfc_lower;
      fprintf(stdout, " SPD_TRFC = 0x%04X = %i\n", spd_trfc, spd_trfc);
/* ==================================================================================== */
    spd_twtr = print_value(pFile, "SPD_TWTR", SPD_TWTR, 1, 0, 0);
      fprintf(stdout, "%i\n", spd_twtr);
/* ==================================================================================== */
    spd_trtp = print_value(pFile, "SPD_TRTP", SPD_TRTP, 1, 0, 0);
      fprintf(stdout, "%i\n", spd_trtp);
/* ==================================================================================== */
    spd_tfaw_upper = print_value(pFile, "SPD_UPPER_TFAW", SPD_UPPER_TFAW, 1, 0, 0);
      fprintf(stdout, "%i\n", spd_tfaw_upper);
/* ==================================================================================== */
    spd_tfaw_lower = print_value(pFile, "SPD_TFAW", SPD_TFAW, 1, 0, 0);
      fprintf(stdout, "%i\n", spd_tfaw_lower);
/* ==================================================================================== */
    spd_tfaw = (spd_tfaw_upper << 8) + spd_tfaw_lower;
      fprintf(stdout, " SPD_TFAW = 0x%04X = %i\n", spd_tfaw, spd_tfaw);
/* ==================================================================================== */
    value = print_value(pFile, "SPD_TCK_FTB offset", SPD_TCK_FTB, 1, 0, 0);
      fprintf(stdout, "%i\n", value);
/* ==================================================================================== */
    value = print_value(pFile, "SPD_TAA_FTB offset", SPD_TAA_FTB, 1, 0, 0);
      fprintf(stdout, "%i\n", value);
/* ==================================================================================== */
    value = print_value(pFile, "SPD_TRCD_FTB offset", SPD_TRCD_FTB, 1, 0, 0);
      fprintf(stdout, "%i\n", value);
/* ==================================================================================== */
    value = print_value(pFile, "SPD_TRP_FTB offset", SPD_TRP_FTB, 1, 0, 0);
      fprintf(stdout, "%i\n", value);
/* ==================================================================================== */
    value = print_value(pFile, "SPD_TRC_FTB offset", SPD_TRC_FTB, 1, 0, 0);
      fprintf(stdout, "%i\n", value);
/* ==================================================================================== */
    value = print_value(pFile, "SPD_RAWCARD", SPD_RAWCARD, 1, 0, 0);
      fprintf(stdout, "%i\n", value);
/* ==================================================================================== */
    value = print_value(pFile, "SPD_ADDRMAP", SPD_ADDRMAP, 1, 0, 0);
    switch(value) {
         case  0: say("standard"); break;
         case  1: say("mirrored"); break;
        default: say("???"); break;
    }
      say("\n");
/* ==================================================================================== */
    value = print_value(pFile, "[7:4] SPD_CTLWRD03", SPD_CTLWRD03, 1, 0, 0);
      fprintf(stdout, "%i\n", bits(value, 7, 4));
/* ==================================================================================== */
    value = print_value(pFile, "[7:4] SPD_CTLWRD04", SPD_CTLWRD04, 1, 0, 0);
      fprintf(stdout, "%i\n", bits(value, 3, 0));
/* ==================================================================================== */
    value = print_value(pFile, "[7:4] SPD_CTLWRD05", SPD_CTLWRD05, 1, 0, 0);
      fprintf(stdout, "%i\n", bits(value, 7, 4));
/* ==================================================================================== */
    value = print_value(pFile, "SERIALNUM", SERIALNUM, SERIALNUM_SIZE, 1, 0);
      fprintf(stdout, "0x%08X\n", value);
/* ==================================================================================== */
    print_value(pFile, "PARTNUM", PARTNUM, PARTNUM_SIZE, 0, 1);
/* ==================================================================================== */
    fprintf(stdout, "================================================================\n");
/* ==================================================================================== */
    xmp_id_lower = print_value(pFile, "XMP_ID_LO", XMP_ID_LO, 1, 0, 0);
      fprintf(stdout, "%i\n", xmp_id_lower);
/* ==================================================================================== */
    xmp_id_upper = print_value(pFile, "XMP_ID_HI", XMP_ID_HI, 1, 0, 0);
      fprintf(stdout, "%i\n", xmp_id_upper);
/* ==================================================================================== */
    xmp_id = (xmp_id_upper << 8) + xmp_id_lower;
      fprintf(stdout, " XMP ID is 0x%04X : ", xmp_id);
    if ((xmp_id_lower == XMP_ID_LO_VAL) && (xmp_id_upper == XMP_ID_HI_VAL))
        say("CORRECT\n");
    else
        fprintf(stdout, "INCORRECT, != %i\n", XMP_ID_VAL);
/* ==================================================================================== */
    value = print_value(pFile, "XMP_PROFILES", XMP_PROFILES, 1, 0, 0);
      say("\n XMP PROFILE 1: ");
    switch(bits(value, 1, 0)) {
         case  0: say("disabled"); break;
         case  1: say("enabled"); break;
        default: say("???"); break;
    }
      say("\n XMP PROFILE 2: ");
    switch(bits(value, 2, 1)) {
         case  0: say("disabled"); break;
         case  1: say("enabled"); break;
        default: say("???"); break;
    }
      say("\n");
    if (bits(value, 1, 0) == 1) {
/* ==================================================================================== */
    xmp1_divident = print_value(pFile, "XMP1_DIVIDENT", XMP1_DIVIDENT, 1, 0, 0);
      fprintf(stdout, "%i\n", xmp1_divident);
    xmp1_divisor = print_value(pFile, "XMP1_DIVISOR", XMP1_DIVISOR, 1, 0, 0);
      fprintf(stdout, "%i\n", xmp1_divisor);
    fprintf(stdout, " XMP1's Medium Timebase MTB = %i/%i ns (SPD's was %i/%i ns)\n",
        xmp1_divident, xmp1_divisor, spd_divident, spd_divisor);
/* ==================================================================================== */
    value = print_value(pFile, "XMP1_VOLTAGE", XMP1_VOLTAGE, 1, 0, 0);
      fprintf(stdout, "%i.%02i\n", bits(value, 6, 5), 5*bits(value, 4, 0));
/* ==================================================================================== */
    xmp1_tck = print_value(pFile, "XMP1_TCK", XMP1_TCK, 1, 0, 0);
      fprintf(stdout, "%i (SPD_TCK was %i)\n", xmp1_tck, spd_tck);
    if ((xmp1_tck != 0) && (xmp1_divident != 0))
        xmp1_clock = (1000 * xmp1_divisor)/(xmp1_tck * xmp1_divident);
    else
        xmp1_clock = 0;
      fprintf(stdout, " XMP1's Clock = %i MHz - %i MT/s (SPD's was %i MHz - %i MT/s)\n",
                      xmp1_clock, 2 * xmp1_clock, spd_clock, 2 * spd_clock);
/* ==================================================================================== */
    xmp1_taa = print_value(pFile, "XMP1_TAA", XMP1_TAA, 1, 0, 0);
      fprintf(stdout, "%i (SPD_TAA was %i)\n", xmp1_taa, spd_taa);
/* ==================================================================================== */
    value = print_value(pFile, "XMP1_CASLO", XMP1_CASLO, 1, 0, 0);
    if (value != 0) {
        for (i = 0; i < 8; i++) {
            switch(bits(value, i, i)) {
                case 0: break;
                case 1: fprintf(stdout, "%i, ", i+4); break;
                break;
            }
        }
        say("- CAS latencies supported\n");
    }
    else {
        say("-\n");
    }
/* ==================================================================================== */
    value = print_value(pFile, "XMP1_CASHI", XMP1_CASHI, 1, 0, 0);
    if (value != 0) {
        for (i = 0; i < 7; i++) {
            switch(bits(value, i, i)) {
                case 0: break;
                case 1: fprintf(stdout, "%i, ", i+12); break;
                break;
            }
        }
        say("- CAS latencies supported\n");
    }
    else {
        say("-\n");
    }
/* ==================================================================================== */
    xmp1_trp = print_value(pFile, "XMP1_TRP", XMP1_TRP, 1, 0, 0);
      fprintf(stdout, "%i (SPD_TRP was %i)\n", xmp1_trp, spd_trp);
/* ==================================================================================== */
    xmp1_trcd = print_value(pFile, "XMP1_TRCD", XMP1_TRCD, 1, 0, 0);
      fprintf(stdout, "%i (SPD_TRCD was %i)\n", xmp1_trcd, spd_trcd);
/* ==================================================================================== */
    xmp1_twr = print_value(pFile, "XMP1_TWR", XMP1_TWR, 1, 0, 0);
      fprintf(stdout, "%i (SPD_TWR was %i)\n", xmp1_twr, spd_twr);
/* ==================================================================================== */
    value = print_value(pFile, "[3:0] XMP1_UPPER_TRAS", XMP1_UPPER_TRAS, 1, 0, 0);
    xmp1_tras_upper = bits(value, 3, 0);
      fprintf(stdout, "%i\n", xmp1_tras_upper);
/* ==================================================================================== */
    value = print_value(pFile, "[7:4] XMP1_UPPER_TRC", XMP1_UPPER_TRC, 1, 0, 0);
    xmp1_trc_upper = bits(value, 7, 4);
      fprintf(stdout, "%i\n", xmp1_trc_upper);
/* ==================================================================================== */
    xmp1_tras_lower = print_value(pFile, "XMP1_TRAS", XMP1_TRAS, 1, 0, 0);
      fprintf(stdout, "%i\n", xmp1_tras_lower);
/* ==================================================================================== */
    xmp1_trc_lower = print_value(pFile, "XMP1_TRC", XMP1_TRC, 1, 0, 0);
      fprintf(stdout, "%i\n", xmp1_trc_lower);
/* ==================================================================================== */
    xmp1_tras = (xmp1_tras_upper << 8) + xmp1_tras_lower;
      fprintf(stdout, " XMP1_TRAS = 0x%04X = %i (SPD_TRAS was 0x%04X = %i)\n",
                      xmp1_tras, xmp1_tras, spd_tras, spd_tras);
/* ==================================================================================== */
    xmp1_trc = (xmp1_trc_upper << 8) + xmp1_trc_lower;
      fprintf(stdout, " XMP1_TRC = 0x%04X = %i (SPD_TRC was 0x%04X = %i)\n",
                      xmp1_trc, xmp1_trc, spd_trc, spd_trc);
/* ==================================================================================== */
    xmp1_trfc_lower = print_value(pFile, "XMP1_TRFC_LO", XMP1_TRFC_LO, 1, 0, 0);
      fprintf(stdout, "%i\n", xmp1_trfc_lower);
/* ==================================================================================== */
    xmp1_trfc_upper = print_value(pFile, "XMP1_TRFC_HI", XMP1_TRFC_HI, 1, 0, 0);
      fprintf(stdout, "%i\n", xmp1_trfc_upper);
/* ==================================================================================== */
    xmp1_trfc = (xmp1_trfc_upper << 8) + xmp1_trfc_lower;
      fprintf(stdout, " XMP1_TRFC = 0x%04X = %i (SPD_TRFC was 0x%04X = %i)\n",
                      xmp1_trfc, xmp1_trfc, spd_trfc, spd_trfc);
/* ==================================================================================== */
    xmp1_trtp = print_value(pFile, "XMP1_TRTP", XMP1_TRTP, 1, 0, 0);
      fprintf(stdout, "%i (SPD_TRTP was %i)\n", xmp1_trtp, spd_trtp);
/* ==================================================================================== */
    xmp1_trrd = print_value(pFile, "XMP1_TRRD", XMP1_TRRD, 1, 0, 0);
      fprintf(stdout, "%i (SPD_TRRD was %i)\n", xmp1_trrd, spd_trrd);
/* ==================================================================================== */
    xmp1_tfaw_upper = print_value(pFile, "XMP1_UPPER_TFAW", XMP1_UPPER_TFAW, 1, 0, 0);
      fprintf(stdout, "%i\n", xmp1_tfaw_upper);
/* ==================================================================================== */
    xmp1_tfaw_lower = print_value(pFile, "XMP1_TFAW", XMP1_TFAW, 1, 0, 0);
      fprintf(stdout, "%i\n", xmp1_tfaw_lower);
/* ==================================================================================== */
    xmp1_tfaw = (xmp1_tfaw_upper << 8) + xmp1_tfaw_lower;
      fprintf(stdout, " XMP1_TFAW = 0x%04X = %i (SPD_TFAW was 0x%04X = %i)\n",
                      xmp1_tfaw, xmp1_tfaw, spd_tfaw, spd_tfaw);
/* ==================================================================================== */
    xmp1_twtr = print_value(pFile, "XMP1_TWTR", XMP1_TWTR, 1, 0, 0);
      fprintf(stdout, "%i (SPD_TWTR was %i)\n", xmp1_twtr, spd_twtr);
/* ==================================================================================== */
    xmp1_cmd_rate = print_value(pFile, "XMP1_CMD_RATE", XMP1_CMD_RATE, 1, 0, 0);
    if (xmp1_divisor != 0)
        fprintf(stdout, "%i * %i/%i = %iT\n", xmp1_cmd_rate, xmp1_divident, xmp1_divisor,
                      xmp1_cmd_rate * xmp1_divident / xmp1_divisor);
    else
        fprintf(stdout, "%i * %i/0 = ???T\n", xmp1_cmd_rate, xmp1_divident);
/* ==================================================================================== */
    }
      say("\n");
}

static void disable_buffering(FILE *pFile) {
    setvbuf(pFile, NULL, _IONBF, 0);
}

static void usage(unsigned const char *str) {
    fprintf(stdout, "USAGE: %s myspd.bin\n", str);
      fflush(stdout);
}

int main(int argc, unsigned char *argv[]) {
    FILE * pFile_in;
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }
    else {
        pFile_in = fopen(argv[1], "r");
        if (pFile_in == NULL) {
            fprintf(stdout, "ERROR: cannot open file %s for reading\n", argv[1]);
              fflush(stdout);
            return 1;
        }
        if (checksize(pFile_in, argv[1])) {
            fclose(pFile_in);
            return 1;
        }
        disable_buffering(pFile_in);
        fprintf(stdout, "\n=== %s\n\n", argv[1]);
          fflush(stdout);
        print_info(pFile_in);
        fclose(pFile_in);
    }
    return 0;
}
