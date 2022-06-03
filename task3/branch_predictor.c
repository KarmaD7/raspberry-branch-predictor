#include "branch_predictor.h"

/* ************** */
/* TASK3: 2019011265 */
/* ************** */

// CPHT 2 ** 8 * 2 = 512
// GHR  8
// BHT  2 ** 8 * 8 = 2048
// PHT  2 ** 8 * 2 = 512
// BTB  2 ** 5 * 128 = 4096

char* predictor_space;
char* cpht_space;
char* ghr_space;
char* bht_space;
char* pht_space;
btb_item_t* btb_space;
int global_choose;
int local_choose;

#define STRONG_TAKEN     0x3
#define WEAK_TAKEN       0x2
#define WEAK_NOT_TAKEN   0x1
#define STRONG_NOT_TAKEN 0x0
#define BHR_WIDTH        8
#define BTB_INDEX_WIDTH  4
#define CPHT_IDX_WIDTH   8
#define BHT_IDX_WIDTH    8
#define CPHT_OFFSET      0
#define CPHT_SIZE        512
#define GHR_OFFSET       CPHT_SIZE
#define GHR_SIZE         8
#define BHT_OFFSET       (CPHT_SIZE + GHR_SIZE)
#define BHT_SIZE         2048
#define PHT_OFFSET       (BHT_OFFSET + BHT_SIZE)
#define PHT_SIZE         512
#define BTB_OFFSET       (PHT_OFFSET + PHT_SIZE)
#define BTB_SIZE         4096

struct btb_item_t {
    va_t pc;
    va_t tgt;
}

void branch_predictor_init(void) {
    predictor_space = vmalloc(PREDICTOR_SIZE * sizeof(char));
    // TODO: other code if necessary
    cpht_space = predictor_space + (CPHT_OFFSET + 7) / 8;
    ghr_space  = predictor_space + (GHR_OFFSET + 7) / 8;
    bht_space  = predictor_space + (BHT_OFFSET + 7) / 8;
    pht_space  = predictor_space + (PHT_OFFSET + 7) / 8;
    btb_space  = (btb_item_t*) (predictor_space + (BTB_OFFSET + 7) / 8);
}

va_t get_pc_idx(reg64_t pc, int width) {
    return (pc & (((1 << width) - 1) << 2)) >> 2; 
}

va_t get_xor_idx(va_t pc_idx, va_t bhr) {
    return pc_idx ^ bhr;
}

va_t get_bits(uint8_t byte, int start_bit, int end_bit) {
    // 7 6 5 4 3 2 1 0 [start, end]
    uint8_t mask;
    mask = (1 << (start_bit - end_bit + 1)) - 1;
    mask = mask << end_bit;
    return (byte & mask) >> end_bit; 
}

void set_bits(uint8_t* byte, int new_bits, int start_bit, int end_bit) {
    uint8_t mask;
    mask = (1 << (start_bit - end_bit + 1)) - 1;
    mask = mask << end_bit;
    mask = ~mask;
    *byte = *byte & mask;
    new_bits = new_bits << end_bit;
    *byte |= new_bits;
}

uint8_t get_two_bits(char* arr, int idx) {
    int arr_idx = idx / 4;
    int bit_offset = idx % 4;
    int byte = arr[arr_idx];
    int bits = get_bits(byte, 7 - 2 * bit_offset, 7 - 2 * bit_offset - 1);
    return bits;
}

void set_two_bits(char* arr, int idx, int jmp) {
    int arr_idx = idx / 4;
    int bit_offset = idx % 4;
    uint8_t* byte = arr + arr_idx;
    int bits = get_bits(*byte, 7 - 2 * bit_offset, 7 - 2 * bit_offset - 1);
    update_two_bits(&bits, jmp);
    set_two_bits(byte, bits, 7 - 2 * bit_offset, 7 - 2 * bit_offset - 1)
}

void update_two_bits(int* bits, int jmp) {
    if (jmp) {
        if (bits != 3)
            *bits = (*bits + 1) % 4;
    } else {
        if (bits != 0)
            *bits = (*bits + 3) % 4;
    }
}

va_t get_result_from_btb(va_t pc) {
    int btb_idx = get_pc_idx(pc, BTB_INDEX_WIDTH);
    btb_item_t item = btb_space[btb_idx];
    if (btb_idx == get_pc_idx(item.pc, BTB_INDEX_WIDTH)) {
        return item.target;
    }
    return pc + 8;
}

void set_btb(va_t pc, va_t target) {
    int btb_idx = get_pc_idx(pc, BTB_INDEX_WIDTH);
    btb_space[btb_idx].target = target;
}

va_t get_from_branch_predictor(reg64_t pc) {
    // TODO: once your branch predictor get an instruction address, it should give a predicted target address
    int pc_idx = get_pc_idx(pc, CPHT_IDX_WIDTH);
    int cpht_bits = get_two_bits(cpht_space, pc_idx);
    int g_bits = get_two_bits(pht_space, pc_idx ^ *ghr_space);
    int bhr = bht_space[pc_idx];
    int l_bits = get_two_bits(pht_space, pc_idx ^ bhr);
    global_choose = g_bits >= WEAK_TAKEN ? 1 : 0;
    local_choose = l_bits >= WEAK_TAKEN ? 1 : 0;
    int choose = cphr_bits >= WEAK_TAKEN ? global_choose : local_choose;
    va_t result;
    // index width is same for cpht, bht, pht    
    result = choose ? get_result_from_btb(pc) : pc + 4;
    return result;
}

void branch_predictor_update(reg64_t pc, va_t target) {
    // TODO: the debugger will give the correct target address once a branch instrucion executed, and you need to update the branch predictor
    int jmp = target != (pc + 4);
    int pc_idx = get_pc_idx(pc, CPHT_IDX_WIDTH);
    set_two_bits(cpht_space, pc_idx, jmp);
    set_two_bits(pht_space, pc_idx ^ *ghr_space, jmp);
    uint8_t *bhr = bht_space + pc_idx;
    set_two_bits(pht_space, pc_idx ^ *bhr, jmp);
    *pht_space = (*pht_space << 1) | 1;
    *bhr = (*bhr << 1) | 1;
    // bhr, ghr
    if (jmp) {
        set_btb(pc, target);
    }
}

void branch_predictor_release(void) {
    vfree(predictor_space);
    // TODO: other code if necessary
}