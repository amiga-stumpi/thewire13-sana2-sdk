	XDEF	_sdk_call_buffer

	section	"text",code

_sdk_call_buffer:
	movem.l	a2,-(sp)
	move.l	8(sp),a2
	move.l	12(sp),a0
	move.l	16(sp),a1
	move.l	20(sp),d0
	jsr	(a2)
	movem.l	(sp)+,a2
	rts
