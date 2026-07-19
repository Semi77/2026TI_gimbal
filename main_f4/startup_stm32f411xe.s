Stack_Size      EQU     0x00000400

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp

Heap_Size       EQU     0x00000200

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit

                PRESERVE8
                THUMB

                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size

__Vectors       DCD     __initial_sp
                DCD     Reset_Handler
                DCD     Default_Handler
                DCD     Default_Handler
                DCD     Default_Handler
                DCD     Default_Handler
                DCD     Default_Handler
                DCD     0
                DCD     0
                DCD     0
                DCD     0
                DCD     Default_Handler
                DCD     Default_Handler
                DCD     0
                DCD     Default_Handler
                DCD     Default_Handler
__Vectors_End
__Vectors_Size  EQU     __Vectors_End - __Vectors

                AREA    |.text|, CODE, READONLY

; 复位处理函数进入C运行库并最终调用main函数。
Reset_Handler   PROC
                EXPORT  Reset_Handler
                IMPORT  __main
                B       __main
                ENDP

; 默认异常处理函数在意外异常发生后停留在原地。
Default_Handler PROC
                EXPORT  Default_Handler
                B       .
                ENDP

                ALIGN

                IF      :DEF:__MICROLIB
                EXPORT  __initial_sp
                EXPORT  __heap_base
                EXPORT  __heap_limit
                ELSE
                IMPORT  __use_two_region_memory
                EXPORT  __user_initial_stackheap

; 栈和堆初始化函数向C运行库提供内存边界。
__user_initial_stackheap PROC
                LDR     R0, =Heap_Mem
                LDR     R1, =(Stack_Mem + Stack_Size)
                LDR     R2, =(Heap_Mem + Heap_Size)
                LDR     R3, =Stack_Mem
                BX      LR
                NOP
                ENDP
                ENDIF

                ALIGN   2
                END
