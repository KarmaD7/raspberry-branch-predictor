#include "branch_predictor.h"

/* ************** */
/* TASK3: YOUR ID */
/* ************** */

char* predictor_space;

void branch_predictor_init(void) {
    predictor_space = vmalloc(PREDICTOR_SIZE * sizeof(char));
    // TODO: other code if necessary
}

va_t get_from_branch_predictor(reg64_t pc) {
    // TODO: once your branch predictor get an instruction address, it should give a predicted target address
    return 0;
}

void branch_predictor_update(reg64_t pc, va_t target) {
    // TODO: the debugger will give the correct target address once a branch instrucion executed, and you need to update the branch predictor
}

void branch_predictor_release(void) {
    vfree(predictor_space);
    // TODO: other code if necessary
}