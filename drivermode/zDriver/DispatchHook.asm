.code 
DispatchHook proc
	add rsp, 8h
	mov rax, 0DEADBEEFCAFEBEEFh
	jmp rax
DispatchHook endp
 
end