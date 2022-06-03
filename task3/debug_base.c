#include "debug_base.h"

struct base_addr_t *param;
struct task_struct **tsk;

void debugger_init(void) {
    param = kmalloc(sizeof(struct base_addr_t), GFP_KERNEL);
    tsk = kmalloc(sizeof(void*) * KTHREAD_NUM, GFP_KERNEL);
    // Mapping the debug and cross trigger registers into virtual memory space 
    param->debug_register = ioremap(DEBUG_REGISTER_ADDR_CPU3, DEBUG_REGISTER_SIZE_CPU3);
    param->cti_register = ioremap(CTI_REGISTER_ADDR_CPU3, CTI_REGISTER_SIZE_CPU3);
}

void debugger_exit(void) {
    iounmap(param->cti_register);
    iounmap(param->debug_register);
    kfree(param);
    kfree(tsk);
}

void execute_ins_via_itr(void __iomem* debug, ins_t ins) {
    reg32_t reg;
    // clear previous errors 
    iowrite32(CSE, debug + EDRCR_EL1);
    // Write instruction to EDITR register to execute it
    iowrite32(ins, debug + EDITR_EL1);
    // Wait until the execution is finished
    reg = ioread32(debug + EDSCR_EL1);
    while ((reg & ITE) != ITE) 
        reg = ioread32(debug + EDSCR_EL1);
    if ((reg & ERR) == ERR) 
        printk(KERN_ERR "%s failed! instruction: 0x%08x EDSCR: 0x%08x\n",  __func__, ins, reg);
}

/* ************** */
/* TASK2: 2019011265 */
/* ************** */


reg64_t read_64bit_from_target(void __iomem* debug, ins_t ins) {
    // you can add some variables if necessary
    reg32_t reg;
    uint64_t val = 0;
    /* TASK2 */
    // TODO: Complete this function.
    // Step1. transfer target value from Xt to DBGDTR using 32bit instruction @ins (Done)
    execute_ins_via_itr(debug, ins);

    // Step 2. check EDSCR,TXfull before each transfer [hint: use circulation obstruction] (Your code)
    while (true) {
        reg = ioread32(debug + EDSCR_EL1);
        if ((reg & TXfull) == TXfull) {
            break;
        }
    }

    // Step 3. read value from DBGDTRRX and DBGDTRTX [note: you must take reading order into account, first read ... , and then read ...] (Your code)
    val |= ((uint64_t) ioread32(debug + DBGDTRRX_EL1) << 32);
    val |= ioread32(debug + DBGDTRTX_EL1);
    // reg &= ~TXfull;
    // iowrite32(reg, debug + EDSCR_EL1);

    return val;
}

void send_64bit_to_target(void __iomem* debug, ins_t ins, reg64_t val) {
    // you can add some variables if necessary
    reg32_t reg;
    uint32_t hval, lval;
    hval = (val >> 32) & 0xffffffff;  // higher 32 bits of val
    lval = val & 0xffffffff; // lower 32 bits of val

    /* TASK2 */
    // TODO: Complete this function.
    // Step 1. write value to DBGDTRRX and DBGDTRTX [note: you must take writing order into account, first write ... , and then write ...] (Your code)
    iowrite32(lval, debug + DBGDTRRX_EL1);
    iowrite32(hval, debug + DBGDTRTX_EL1);

    // Step 2. check EDSCR.{RXfull/TXfull} before each transfer (Your code)
    while (true) {
        reg = ioread32(debug + EDSCR_EL1);
        if ((reg & RXfull) == RXfull) {
            break;
        }
        /* code */
    }
    

    // Step3. transfer target value from DBGDTR to Xt using 32bit instruction @ins (Done)
    execute_ins_via_itr(debug, ins);
}

uint32_t read_memory_via_dcc(void __iomem* debug, va_t va) {
    // you can add some variables if necessary
    reg32_t reg;
    uint32_t hva, lva;
    hva = (va >> 32) & 0xffffffff;
    lva = va & 0xffffffff;
    // TODO: Complete this function.
    // Step 1. write the address to DBGDTR_EL0 via the memory mapped interface (Your code)
    iowrite32(hva, debug + DBGDTRTX_EL1);
    iowrite32(lva, debug + DBGDTRRX_EL1);
    // Step 2. put the memory address to X0 [hint: execute_ins_via_itr(debug, ...); 0xd5330400 <=> mrs X0, DBGDTR_EL0] (Your code)
    execute_ins_via_itr(debug, 0xd5330400);
    // Step 3. a dummy instruction to set the EDSCR.TXfull bit [hint: 0xd5130400 <=> msr DBGDTR_EL0, X0] (Your code)
    execute_ins_via_itr(debug, 0xd5130400);
    
    // Step 4. switch to memory access mode (Your code)
    reg = ioread32(debug + EDSCR_EL1);
    reg |= MA;
    iowrite32(reg, debug + EDSCR_EL1);

    // Step 5. discard the first read (Your code)
    ioread32(debug + DBGDTRTX_EL1);
    
    // Step 6. switch to normal access mode (Your code)
    reg = ioread32(debug + EDSCR_EL1);
    reg &= ~MA;
    iowrite32(reg, debug + EDSCR_EL1);

    // Step 7. read DBGDTRTX_EL0 again to get the value at the target address (Done)
    return ioread32(debug + DBGDTRTX_EL1);
}