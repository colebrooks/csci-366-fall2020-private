        global    find_max
        section   .text
find_max:
        ;; rdi has the array in it
        ;; rsi has the length in it
        ;; you will need to loop through the array and
        ;; compare each value with rax to determine if it is greater
        ;; after the comparison, decrement the count, bump the
        ;; array pointer by 8 (long long = 64 bits = 8 bytes)
        ;; and if the counter is greater than zero, loop

.rep:   mov rax, [rdi]      ;; move array value into rax
.skip:  sub rsi, 1          ;; decrement array counter
        cmp rsi, 0          ;; compare counter with 0
        je stop             ;; if equal, array is over, return
        add rdi, 8          ;; else, move rdi to next value
        cmp rax, [rdi]      ;; compare old greatest value with next value
        jl .rep             ;; if old value is less, jump to replace logic
        jmp .skip           ;; else, jump to next array value
stop:   ret