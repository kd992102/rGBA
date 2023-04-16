//rom start at 0x8000000
#include <stdint.h>

typedef uint64_t u64;
typedef int64_t i64;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint8_t u8;
typedef int8_t i8;

#define CART_BASE_ADDR 0x8000000
#define DataProcess_eigen 0xc000000
#define Multiply_eigen 0xfc000f0
#define MultiplyLong_eigen 0xf8000f0
#define SingleDataSwap_eigen 0xfb00ff0
#define BranchAndExchange_eigen 0xffffff0
#define HalfwordDataTransfer_eigen 0xe000090
//#define HalfwordDataTransferIO_eigen 0xe400090
#define SingleDataTransfer_eigen 0xc000000
#define Undefined_eigen 0xe000010
#define BlockDataTransfer_eigen 0xe000000
#define Branch_eigen 0xe000000
#define CoprocessorDataT_eigen 0xe000000
#define CoprocessorDataO_eigen 0xf000010
#define CoprocessorRegisterT_eigen 0xf000010
#define SoftwareInterrupt_eigen 0xf000000

//Exception Vectors
#define __RESET__ 0x00000000
#define __UNDEFINED__ 0x00000004
#define __SOFTWARE_INTERRUPT__ 0x00000008
#define __ABORT_PREFETCH__ 0x0000000C
#define __ABORT_DATA__ 0x00000010
#define __RESERVED__ 0x00000014
#define __IRQ__ 0x00000018
#define __FIQ__ 0x0000001C

//Address
#define Base_addr 0x00000000

char cond_code[16][3] = {"EQ\0",
                         "NE\0",
                         "CS\0", 
                         "CC\0",
                         "MI\0",
                         "PL\0",
                         "VS\0",
                         "VC\0",
                         "HI\0",
                         "LS\0",
                         "GE\0",
                         "LT\0",
                         "GT\0",
                         "LE\0",
                         "\0",
                         "\0"};

struct gba_rom_header{
    u32 Entry_point;
    u8 Nintendo_Logo[156];
    u8 Game_title[12];
    u32 Game_code;
    u8 Maker_code[2];
    u8 Fixed_value;
    u8 Main_unit_code;
    u8 Device_type;
    u8 Reserved_area[7];
    u8 Software_version;
    u8 Complement_check;
    u16 Reserved_area_end;
};

struct gba_rom{
    struct gba_rom_header *header;
    u32 *gba_rom_buf;
    int FileSize;
    char *FileName;
};


