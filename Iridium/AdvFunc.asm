.CODE

_NtClose PROC
	mov r10, rcx
	mov eax, 0Fh       ; NtClose系统调用号
	syscall
	ret
_NtClose ENDP

Int3Breakpoint PROC
	int 3
	ret
Int3Breakpoint ENDP

AnyJump PROC
    mov rax, rcx
    jmp rax
AnyJump ENDP

EXTERN g_BaseCycles:QWORD   ; 在 C++ 中定义并初始化为 0

EXTERN DebuggerCheckEnabled:DWORD   ; BOOL 实际上是 int，32位
EXTERN g_BaseCycles:QWORD           ; 基准值，64位

.CODE

; 函数：CheckDebugger
; 返回值：eax = 0（无调试器或检测禁用），1（可能被调试）
CheckDebugger PROC

    ; 首先检查调试检测是否启用
    cmp     DebuggerCheckEnabled, 0
    jne     enabled
    ; 如果禁用，直接返回 0
    xor     eax, eax
    ret

enabled:
    push    rbx
    push    rsi
    push    rdi
    push    r12

    ; 先检查是否已校准
    cmp     g_BaseCycles, 0
    jne     measure

    ; --- 校准阶段 ---
    call    get_cycles      ; 测量当前耗时，结果在 rax
    mov     g_BaseCycles, rax
    xor     eax, eax        ; 校准期间返回 0（无调试器）
    jmp     done

measure:
    ; --- 正常检测 ---
    mov     rbx, 10         ; 测量次数
    xor     r12d, r12d      ; 可疑计数
    mov     rsi, g_BaseCycles
    imul    rsi, rsi, 3     ; 阈值 = 基准 * 3

measure_loop:
    call    get_cycles      ; 返回耗时在 rax
    cmp     rax, rsi
    jbe     good
    inc     r12d
good:
    dec     rbx
    jnz     measure_loop

    cmp     r12d, 5         ; 过半则判定
    jae     detected
    xor     eax, eax
    jmp     done

detected:
    mov     eax, 1
done:
    pop     r12
    pop     rdi
    pop     rsi
    pop     rbx
    ret

; 辅助函数：执行一段指令序列并返回耗时（周期数）
get_cycles PROC
    push    rbx
    ; 第一次 rdtsc
    rdtsc
    shl     rdx, 32
    or      rax, rdx
    mov     rbx, rax

    ; 增强指令序列：包含内存访问，放大差异
    sub     rsp, 32         ; 分配局部空间
    mov     qword ptr [rsp], 12345678h
    mov     qword ptr [rsp+8], 87654321h
    mov     rax, [rsp]
    xor     rax, [rsp+8]
    add     rax, 1
    imul    rax, rax, 2
    mov     [rsp+16], rax
    add     rsp, 32

    ; 第二次 rdtsc
    rdtsc
    shl     rdx, 32
    or      rax, rdx
    sub     rax, rbx        ; rax = 耗时
    pop     rbx
    ret
get_cycles ENDP

CheckDebugger ENDP

END
