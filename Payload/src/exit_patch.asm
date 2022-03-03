; http://lallouslab.net/2016/01/11/introduction-to-writing-x64-assembly-in-visual-studio/
;
; Here's the deal: we want to provide functionality to unload the dll from the game via a message
; from  IPC.  To do this, we must unhook all hooks with MH_Uninitialize(). The problem is that if
; we call that while a game thread is in a detour, that will cause a crash. It is not possible to
; use  a  mutex  (or  any other synthronization feature) to know when there are no threads in the
; detours, since that would require us sending a signal AFTER a detour (i.e. in game code - which
; we  can't  edit  aside  from adding another detour, which of course doesn't solve the problem).
;
; The solution is to exit using the game's main thread. DETOUR_MainLoop__getLimitedDt gets called
; every  engine loop. If the exit flag is not set then we'll just jump to the C++ implementation.
; Otherwise, we do the following:
;
; 1) Call PreExitCleanup() - this is where we put MH_Uninitialize() and other cleanup stuff.
;
; 2) Set the "return" value of getLimitedDt to 0. This value is returned in the xmm0 register, we
;    which can (hopefully) rely on to not get clobbered by the FreeLibrary call.
;
; 3) Get our module handle as a param to FreeLibrary.
;
; 4) Destroy  our  stack  frame  and  jump  to  FreeLibrary. This makes FreeLibrary return to the
;    orignal  caller  of  getLimitedDt  in the game code. Even though FreeLibrary returns a bool,
;    it'll put that in rax. We can only hope that it never uses the xmm0 register. If it does, we
;    should hook a function that returns void (or a unless bool) instead.

.code

EXTERN g_bQueueExit: byte

EXTERN FreeLibrary: PROC
EXTERN DETOUR_MainLoop__getLimitedDt_Func: PROC
EXTERN PreExitCleanup: PROC
EXTERN GetSelfModuleHandle: PROC

DETOUR_MainLoop__getLimitedDt PROC
	cmp byte ptr g_bQueueExit, 0
	je DETOUR_MainLoop__getLimitedDt_Func ; if we're not exiting, just go to the C++ detour implementation
	push rbp             ; setup stack frame
	mov rbp, rsp
	sub rsp, 8 * (4 + 2) ; allocate shadow register area + 2 QWORDs for stack alignment
	call PreExitCleanup
	xor rcx, rcx
	movq xmm0, rcx       ; 0 -> xmm0 - this is what we're "returning" from the detour, hope nothing overrides it
	call GetSelfModuleHandle
	mov rcx, rax         ; rcx <- our module handle, param to FreeLibrary
	add rsp, 8 * (4 + 2) ; remove shadow register area
	mov rsp, rbp         ; destroy our stack frame
	pop rbp
	jmp FreeLibrary
DETOUR_MainLoop__getLimitedDt ENDP

END
