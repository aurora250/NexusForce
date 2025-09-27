IFDEF _WIN64
.code
ELSE
.386P
.xmm
.model flat, c
.code
ENDIF

PUBLIC masm_memory_copy
PUBLIC masm_memory_copy_offset
PUBLIC masm_memory_char_copy
PUBLIC masm_memory_compare
PUBLIC masm_memory_compare_ignore_case
PUBLIC masm_memory_char
PUBLIC masm_memory_move
PUBLIC masm_memory_set
PUBLIC masm_memory_zero
PUBLIC masm_explicit_memory_zero
PUBLIC masm_memory_in_memory
PUBLIC masm_memory_frobnicate

masm_memory_copy PROC
    IFDEF _WIN64
        test rcx, rcx
        jz done
        test rdx, rdx
        jz done
        test r8, r8
        jz done

        push rdi
        push rsi
        mov rsi, rdx
        mov rdi, rcx
        mov rcx, r8
        cld
        rep movsb
        mov rax, rdi
        sub rax, r8
        pop rsi
        pop rdi
    ELSE
        push ebp
        mov ebp, esp
        push edi
        push esi

        mov edi, [ebp+8]
        mov esi, [ebp+12]
        mov ecx, [ebp+16]

        test edi, edi
        jz x86_done
        test esi, esi
        jz x86_done

        cld
        rep movsb
        mov eax, [ebp+8]

x86_done:
        pop esi
        pop edi
        pop ebp
    ENDIF
done:
    ret
masm_memory_copy ENDP

masm_memory_copy_offset PROC
    IFDEF _WIN64
        test rcx, rcx
        jz done
        test rdx, rdx
        jz done
        test r8, r8
        jz done

        push rdi
        push rsi
        mov rsi, rdx
        mov rdi, rcx
        mov rcx, r8
        cld
        rep movsb
        mov rax, rdi
        pop rsi
        pop rdi
    ELSE
        push ebp
        mov ebp, esp
        push edi
        push esi

        mov edi, [ebp+8]
        mov esi, [ebp+12]
        mov ecx, [ebp+16]

        test edi, edi
        jz x86_done
        test esi, esi
        jz x86_done

        cld
        rep movsb
        mov eax, edi

x86_done:
        pop esi
        pop edi
        pop ebp
    ENDIF
done:
    ret
masm_memory_copy_offset ENDP

masm_memory_char_copy PROC
    IFDEF _WIN64
        test rcx, rcx
        jz done_null
        test rdx, rdx
        jz done_null
        test r9, r9
        jz done_null

        push rdi
        push rsi
        mov rsi, rdx
        mov rdi, rcx
        mov rax, r9
        mov dl, r8b

loop_start:
        test rax, rax
        jz done_null_pop
        lodsb
        stosb
        cmp al, dl
        je done_found
        dec rax
        jmp loop_start

done_found:
        mov rax, rdi
        pop rsi
        pop rdi
        ret

done_null_pop:
        pop rsi
        pop rdi
done_null:
    xor rax, rax
    ret
    ELSE
        push ebp
        mov ebp, esp
        push edi
        push esi
        push ebx

        mov edi, [ebp+8]
        mov esi, [ebp+12]
        mov bl, [ebp+16]
        mov ecx, [ebp+20]

        test edi, edi
        jz x86_done_null
        test esi, esi
        jz x86_done_null

x86_loop:
        test ecx, ecx
        jz x86_done_null
        lodsb
        stosb
        cmp al, bl
        je x86_done_found
        dec ecx
        jmp x86_loop

x86_done_found:
        mov eax, edi
        pop ebx
        pop esi
        pop edi
        pop ebp
        ret

x86_done_null:
        xor eax, eax
        pop ebx
        pop esi
        pop edi
        pop ebp
        ret
    ENDIF
masm_memory_char_copy ENDP

masm_memory_compare PROC
    IFDEF _WIN64
        test rcx, rcx
        jz lh_null
        test rdx, rdx
        jz rh_null
        test r8, r8
        jz equal

        push rdi
        push rsi
        mov rsi, rcx
        mov rdi, rdx
        mov rcx, r8
        cld
        repe cmpsb
        je equal_pop

        movzx rax, byte ptr [rsi-1]
        movzx rdx, byte ptr [rdi-1]
        sub rax, rdx
        pop rsi
        pop rdi
        ret

equal_pop:
        pop rsi
        pop rdi
equal:
        xor rax, rax
        ret
lh_null:
        test rdx, rdx
        jz equal
        mov rax, -1
        ret
rh_null:
        mov rax, 1
        ret
    ELSE
        push ebp
        mov ebp, esp
        push edi
        push esi

        mov esi, [ebp+8]
        mov edi, [ebp+12]
        mov ecx, [ebp+16]

        test esi, esi
        jz x86_lh_null
        test edi, edi
        jz x86_rh_null
        test ecx, ecx
        jz x86_equal

        cld
        repe cmpsb
        je x86_equal

        movzx eax, byte ptr [esi-1]
        movzx edx, byte ptr [edi-1]
        sub eax, edx
        pop esi
        pop edi
        pop ebp
        ret

x86_lh_null:
        test edi, edi
        jz x86_equal
        mov eax, -1
        pop esi
        pop edi
        pop ebp
        ret
x86_rh_null:
        mov eax, 1
        pop esi
        pop edi
        pop ebp
        ret
x86_equal:
        xor eax, eax
        pop esi
        pop edi
        pop ebp
        ret
    ENDIF
masm_memory_compare ENDP

masm_memory_compare_ignore_case PROC
    IFDEF _WIN64
        test rcx, rcx
        jz p1_null
        test rdx, rdx
        jz p2_null
        test r8, r8
        jz equal

        push rdi
        push rsi
        push rbx
        mov rsi, rcx
        mov rdi, rdx
        mov rcx, r8

loop_icase:
        test rcx, rcx
        jz loop_end
        lodsb
        mov bl, al
        or bl, 20h
        mov al, [rdi]
        or al, 20h
        cmp bl, al
        jne not_equal
        inc rdi
        dec rcx
        jmp loop_icase

loop_end:
        xor rax, rax
        pop rbx
        pop rsi
        pop rdi
        ret
not_equal:
        movzx rax, bl
        movzx rdx, al
        sub rax, rdx
        pop rbx
        pop rsi
        pop rdi
        ret
p1_null:
        test rdx, rdx
        jz equal
        mov rax, -1
        ret
p2_null:
        mov rax, 1
        ret
equal:
        xor rax, rax
        ret
    ELSE
        push ebp
        mov ebp, esp
        push edi
        push esi
        push ebx

        mov esi, [ebp+8]
        mov edi, [ebp+12]
        mov ecx, [ebp+16]

        test esi, esi
        jz x86_p1_null
        test edi, edi
        jz x86_p2_null
        test ecx, ecx
        jz x86_equal

x86_loop_icase:
        test ecx, ecx
        jz x86_loop_end
        lodsb
        mov bl, al
        or bl, 20h
        mov al, [edi]
        or al, 20h
        cmp bl, al
        jne x86_not_equal
        inc edi
        dec ecx
        jmp x86_loop_icase

x86_loop_end:
        xor eax, eax
        pop ebx
        pop esi
        pop edi
        pop ebp
        ret
x86_not_equal:
        movzx eax, bl
        movzx edx, al
        sub eax, edx
        pop ebx
        pop esi
        pop edi
        pop ebp
        ret
x86_p1_null:
        test edi, edi
        jz x86_equal
        mov eax, -1
        pop ebx
        pop esi
        pop edi
        pop ebp
        ret
x86_p2_null:
        mov eax, 1
        pop ebx
        pop esi
        pop edi
        pop ebp
        ret
x86_equal:
        xor eax, eax
        pop ebx
        pop esi
        pop edi
        pop ebp
        ret
    ENDIF
masm_memory_compare_ignore_case ENDP

masm_memory_char PROC
    IFDEF _WIN64
        test rcx, rcx
        jz done_null
        test r8, r8
        jz done_null

        push rdi
        mov al, dl
        mov rdi, rcx
        mov rcx, r8
        cld
        repne scasb
        je found
        pop rdi
        xor rax, rax
        ret
found:
        lea rax, [rdi-1]
        pop rdi
        ret
done_null:
        xor rax, rax
        ret
    ELSE
        push ebp
        mov ebp, esp
        push edi

        mov edi, [ebp+8]
        mov al, [ebp+12]
        mov ecx, [ebp+16]

        test edi, edi
        jz x86_done_null
        test ecx, ecx
        jz x86_done_null

        cld
        repne scasb
        je x86_found
        xor eax, eax
        pop edi
        pop ebp
        ret
x86_found:
        lea eax, [edi-1]
        pop edi
        pop ebp
        ret
x86_done_null:
        xor eax, eax
        pop edi
        pop ebp
        ret
    ENDIF
masm_memory_char ENDP

masm_memory_move PROC
    IFDEF _WIN64
        test rcx, rcx
        jz done
        test rdx, rdx
        jz done
        test r8, r8
        jz done

        push rdi
        push rsi
        mov rsi, rdx
        mov rdi, rcx
        mov rax, rcx
        cmp rdi, rsi
        jbe forward_copy

        add rsi, r8
        add rdi, r8
        dec rsi
        dec rdi
        mov rcx, r8
        std
        rep movsb
        cld
        pop rsi
        pop rdi
        ret

forward_copy:
        mov rcx, r8
        cld
        rep movsb
        pop rsi
        pop rdi
    ELSE
        push ebp
        mov ebp, esp
        push edi
        push esi

        mov edi, [ebp+8]
        mov esi, [ebp+12]
        mov ecx, [ebp+16]
        mov eax, [ebp+8]

        test edi, edi
        jz x86_done
        test esi, esi
        jz x86_done
        test ecx, ecx
        jz x86_done

        cmp edi, esi
        jbe x86_forward

        add esi, ecx
        add edi, ecx
        dec esi
        dec edi
        std
        rep movsb
        cld
        pop esi
        pop edi
        pop ebp
        ret

x86_forward:
        cld
        rep movsb
x86_done:
        pop esi
        pop edi
        pop ebp
        ret
    ENDIF
done:
    ret
masm_memory_move ENDP

masm_memory_set PROC
    IFDEF _WIN64
        test rcx, rcx
        jz done
        test r8, r8
        jz done

        push rdi
        mov al, dl
        mov rdi, rcx
        mov rdx, rcx
        mov rcx, r8
        cld
        rep stosb
        mov rax, rdx
        pop rdi
    ELSE
        push ebp
        mov ebp, esp
        push edi

        mov edi, [ebp+8]
        mov al, [ebp+12]
        mov ecx, [ebp+16]
        mov edx, [ebp+8]

        test edi, edi
        jz x86_done
        test ecx, ecx
        jz x86_done

        cld
        rep stosb
        mov eax, edx

x86_done:
        pop edi
        pop ebp
        ret
    ENDIF
done:
    ret
masm_memory_set ENDP

masm_memory_zero PROC
    IFDEF _WIN64
        test rcx, rcx
        jz done
        test rdx, rdx
        jz done

        push rdi
        xor al, al
        mov rdi, rcx
        mov rax, rcx
        mov rcx, rdx
        cld
        rep stosb
        pop rdi
    ELSE
        push ebp
        mov ebp, esp
        push edi

        mov edi, [ebp+8]
        mov ecx, [ebp+12]
        mov eax, [ebp+8]

        test edi, edi
        jz x86_done
        test ecx, ecx
        jz x86_done

        xor al, al
        cld
        rep stosb

x86_done:
        pop edi
        pop ebp
        ret
    ENDIF
done:
    ret
masm_memory_zero ENDP

masm_explicit_memory_zero PROC
    IFDEF _WIN64
        test rcx, rcx
        jz done
        test rdx, rdx
        jz done

        push rdi
        xor al, al
        mov rdi, rcx
        mov rax, rcx
        mov rcx, rdx
        cld
        rep stosb
        mfence
        pop rdi
    ELSE
        push ebp
        mov ebp, esp
        push edi

        mov edi, [ebp+8]
        mov ecx, [ebp+12]
        mov eax, [ebp+8]

        test edi, edi
        jz x86_done
        test ecx, ecx
        jz x86_done

        xor al, al
        cld
        rep stosb
        mfence

x86_done:
        pop edi
        pop ebp
        ret
    ENDIF
done:
    ret
masm_explicit_memory_zero ENDP

masm_memory_in_memory PROC
    IFDEF _WIN64
        test rcx, rcx
        jz done_null
        test r8, r8
        jz done_null
        cmp r9, rdx
        jg done_null
        test r9, r9
        jz done_null

        push rdi
        push rsi
        push rbx
        push r10
        push r11

        mov r10, rcx
        mov r11, rdx
        sub r11, r9
        inc r11

outer_loop:
        test r11, r11
        jz not_found
        mov rsi, r10
        mov rdi, r8
        mov rcx, r9
        cld
        repe cmpsb
        je found_match
        inc r10
        dec r11
        jmp outer_loop

found_match:
        mov rax, r10
        pop r11
        pop r10
        pop rbx
        pop rsi
        pop rdi
        ret

not_found:
        pop r11
        pop r10
        pop rbx
        pop rsi
        pop rdi
done_null:
        xor rax, rax
        ret
    ELSE
        push ebp
        mov ebp, esp
        push edi
        push esi
        push ebx
        push ecx
        push edx

        mov esi, [ebp+8]
        mov edx, [ebp+12]
        mov edi, [ebp+16]
        mov ebx, [ebp+20]

        test esi, esi
        jz x86_done_null
        test edi, edi
        jz x86_done_null
        cmp ebx, edx
        jg x86_done_null
        test ebx, ebx
        jz x86_done_null

        sub edx, ebx
        inc edx

x86_outer:
        test edx, edx
        jz x86_not_found
        push esi
        push edi
        mov ecx, ebx
        cld
        repe cmpsb
        pop edi
        pop esi
        je x86_found_match
        inc esi
        dec edx
        jmp x86_outer

x86_found_match:
        mov eax, esi
        pop edx
        pop ecx
        pop ebx
        pop esi
        pop edi
        pop ebp
        ret

x86_not_found:
x86_done_null:
        xor eax, eax
        pop edx
        pop ecx
        pop ebx
        pop esi
        pop edi
        pop ebp
        ret
    ENDIF
masm_memory_in_memory ENDP

masm_memory_frobnicate PROC
    IFDEF _WIN64
        test rcx, rcx
        jz done
        test rdx, rdx
        jz done

        push rdi
        mov rdi, rcx
        mov rax, rcx
        mov rcx, rdx

loop_frob:
        test rcx, rcx
        jz frob_done
        xor byte ptr [rdi], 42
        inc rdi
        dec rcx
        jmp loop_frob

frob_done:
        pop rdi
    ELSE
        push ebp
        mov ebp, esp
        push edi

        mov edi, [ebp+8]
        mov ecx, [ebp+12]
        mov eax, [ebp+8]

        test edi, edi
        jz x86_done
        test ecx, ecx
        jz x86_done

x86_loop_frob:
        test ecx, ecx
        jz x86_done
        xor byte ptr [edi], 42
        inc edi
        dec ecx
        jmp x86_loop_frob

x86_done:
        pop edi
        pop ebp
        ret
    ENDIF
done:
    ret
masm_memory_frobnicate ENDP

END
