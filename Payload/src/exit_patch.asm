; http://lallouslab.net/2016/01/11/introduction-to-writing-x64-assembly-in-visual-studio/
;
; Alright Bobby, have a seat. Here's the deal: unloading the dll is rather tricky. We must unhook
; all  hooks with MH_Uninitialize(), and obviously unload ourself. Assumming that we only have to
; deal  with  only  one  game thread (thank god), how do we ensure it doesn't get stuck somewhere
; where it shouldn't be (e.g. MinHook's trampolines) when we unhook everything? Because if it is,
; the  game  will crash. We can't use a mutex (or any other synchronization primitives) to send a
; signal from the game thread that it's no longer in a detour, because that would require sending
; a  signal  from  outside the detour in game code which we can't edit (aside from adding another
; detour, which of course doesn't solve the problem).
;
; The  solution  that a good fellow named mike came up with is to unhook everything from the game
; thread  and  have  it call FreeLibrary itself. Then unhooking is not a problem, (since the game
; thread  is  in  perfectly static code) but calling FreeLibrary is a bit tricky from the payload
; since  a call would return to this dll, which will of course would be unloaded. That's why this
; needs to be written in good ol' asm. Here's what the function below does:
;
; 1) If the exit flag is not set, jump to the C++ implementation of the detour.
;
; 2) Call PreExitCleanup() - this is where we put MH_Uninitialize() and other cleanup stuff.
;
; 3) Set  the  "return" value of getLimitedDt to 0 (this is easy and just means physics won't run
;    for  a  tick). This value is returned in the xmm0 register, which we can (hopefully) rely on
;    to not get clobbered by the FreeLibrary call.
;
; 4) Get our module handle as a param to FreeLibrary.
;
; 5) Destroy  our  stack  frame  and  jump  to  FreeLibrary. This makes FreeLibrary return to the
;    original  caller  of  getLimitedDt in the game code. Even though FreeLibrary returns a bool,
;    it'll put that in rax. We can only hope that it never uses the xmm0 register. If it does, we
;    should hook a function that returns void (or a useless bool) instead.

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
