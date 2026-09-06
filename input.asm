NEWVAR a
NEWVAR b

SETV a, 42
SETV b, 10

; a > b
CMPVV a, b
JP vv_ok
JMP fail_1

vv_ok:
; a == r0
SETR r0, 42
CMPVR a, r0
JZ vr_ok
JMP fail_2

vr_ok:
; r1 < a
SETR r1, 41
CMPRV r1, a
JN rv_ok
JMP fail_3

rv_ok:
SETR r15, 0
JMP done

fail_1:
SETR r15, 0xE1000001
JMP done

fail_2:
SETR r15, 0xE1000002
JMP done

fail_3:
SETR r15, 0xE1000003

done:
DBGREG
DBGVAR
HALT