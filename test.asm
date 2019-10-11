
fib:
	MOVE R2 SP
	SUB R2 R2 2
	MOVE R1 [R2]
	
	COMPARE R1 2
	GOTO_IF_LESS_THAN else
		MOVE R3 R1
		SUB R3 R3 1
		PUSH R3
		CALL fib
		POP R3

		MOVE R2 SP
		SUB R2 R2 2
		MOVE R1 [R2]
		ADD R0 R0 R1
		GOTO end
	else:
		MOVE R0 1

	end:
	INTERUPT #0
	RETURN

start:
PUSH 5
CALL fib
POP R3

INTERUPT #0

