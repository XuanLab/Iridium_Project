#include <ntddk.h>
#include <intrin.h>

//CR0 Operation

KIRQL DisableWriteProtection() {
    KIRQL oldIrql;
    oldIrql = KeRaiseIrqlToDpcLevel();

    // 使用编译器内置函数读写 CR0
    UINT64 cr0 = __readcr0();
    cr0 &= ~(1ULL << 16);
    __writecr0(cr0);

    _disable();

    __invlpg(0);

    return oldIrql;
} //关闭CR0写保护


void EnableWriteProtection(KIRQL irq) {
    UINT64 cr0 = __readcr0();
    cr0 |= (1ULL << 16);
    __writecr0(cr0);

    _enable();

    KeLowerIrql(irq);
} //启用CR0写保护

