#include "type.h"
#include "protect.h"
#include "string.h"
#include "irq.h"
#include "proc.h"
#include "syscall.h"

/* -----------------------------全局变量-----------------------------*/
/* GDT，全局段描述符表 */
u8 gdtPtr[6];                       /* 0~15:Limit  16~47:Base */
struct SegmentDescriptor gdt[GDT_SIZE];    /* GDT */

/* LDT，中断向量表 */
u8 idtPtr[6];
struct Gate idt[IDT_SIZE];

/* 用于显示字符，暂且这么用 */
int dispPos;

/* 进程描述表 */
struct Process procTable[NR_TASKS]; /* 进程描述表 */
struct Process* procReady;  /* 准备就绪的进程 */

/* TSS */
struct TSS tss;

/* 控制中断可重入 */
int kreenter;

/* 中断处理函数 */
irqHandler irqTable[NR_IRQ];

/* 系统调用 */
systemCall sysCallTable[NR_SYS_CALL] = {
    sysGetTicks
};

int ticks;
/* ------------------------------------------------------------------*/

extern void restart();

static void testA();
static void testB();
static void testC();

void kinit()
{
    dispPos = 0;
    kreenter = 0;
    ticks = 0;
    
    /* 将GDT拷贝到新地址处 */
    memcpy(&gdt, (void*)(*(u32*)(&gdtPtr[2])),  *((u16*)(&gdtPtr[0]))+1);

    /* 更新GDT的位置 */
    u16* gdtLimit = (u16*)(&gdtPtr[0]);
    u32* gdtBase = (u32*)(&gdtPtr[2]);
    *gdtLimit = GDT_SIZE * sizeof(struct SegmentDescriptor) - 1;
    *gdtBase = (u32)(&gdt);

    /* 更新中断向量表的位置 */
    u16* idtLimit = (u16*)(&idtPtr[0]);
    u32* idtBase = (u32*)(&idtPtr[2]);
    *idtLimit = IDT_SIZE * sizeof(struct Gate) - 1;
    *idtBase = (u32)(&idt);

    initProt();

}

int kmain()
{
    dispStr("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n----start kernel----\n");

    struct Process* proc = procTable;
    int i;

    for(i = 0; i < NR_TASKS; ++i)
    {
        /* 初始化寄存器的值，cs指向LDT中第一个段描述符，其它指向第二个 */
        proc[i].regs.cs = SELECTOR_USER_CS;
        proc[i].regs.ds = SELECTOR_USER_DS;
        proc[i].regs.es = SELECTOR_USER_DS;
        proc[i].regs.fs = SELECTOR_USER_DS;
        proc[i].regs.ss = SELECTOR_USER_DS;
        proc[i].regs.gs = SELECTOR_GS;
        proc[i].regs.esp = (u32)proc[i].stack + STACK_SIZE;
        proc[i].regs.eflags = 0x1202;	/* IF=1, IOPL=1, bit 2 is always 1.iret后，会打开中断和设置IO允许位 */
    }

    proc[0].regs.eip = (u32)testA;
    proc[1].regs.eip = (u32)testB;
    proc[2].regs.eip = (u32)testC;

    procReady = proc; /* 设置下一个调度的进程 */

    /* 初始化定时器，使其10ms中断一次 */
    outByte(TIMER_MODE, RATE_GENERATOR);
    outByte(TIMER0, (u8)(TIMER_FREQ/HZ));
    outByte(TIMER0, (u8)((TIMER_FREQ/HZ)>>8));

    putIrqHandler(CLOCK_IRQ, scheduleTick);
    enableIrq(CLOCK_IRQ);

    restart(); /* 启动第一个进程 */

    while(1)
    {}

    return 0;
}

/* 进程A */
static void testA()
{
    while(1)
    {
        dispStr("A ");
        dispInt(getTicks()); /* 系统调用 */
        mdelay(1000);
    }
}

/* 进程B */
static void testB()
{
    while(1)
    {
        dispStr("B ");
        mdelay(1000);
    }
}

/* 进程C */
static void testC()
{
    while(1)
    {
        dispStr("C ");
        mdelay(1000);
    }
}
