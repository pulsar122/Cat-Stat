/*
Gruppe: PERSôNLICHE
ID  : P63777@K2
Wg. : Cat-Stat
Von : Dirk Steins @ K2 (Mi, 26.05.93 10:46)
An  : Timm Ganske @ F
 
Kommentar zu A36716@F in der Gruppe CAT

TG>Hmmmm, muû ich das Rad unbedingt neu erfinden? Sprich: Ein
TG>Assemblersource wÑre mir ganz angenehm.

Kein Problem:

(*$L-,Z+*)
PROCEDURE CalcCrcArray (a : BigTextPtr; howMuch : CARDINAL):CARDINAL;
BEGIN
  ASSEMBLER
*/
; D2 durch D8 und D3 durch D9 ersetzt
; danach D4 durch D2 und D5 durch D3

; Es wird nur noch ein Pointer in A0 Åbergeben
export crc16

crc16:
	move.l      d3, -(sp)								; Leider D3 retten
	MOVEQ       #0,D0   ; d
	MOVEQ       #0,D1   ; e
;	MOVE.W      -(A3),D8  ; ZÑhler  (howMuch)			; Pure C braucht das nicht
;	TST.W       D8              ; D8 = 0				; C-String
;	BEQ.S       ret             ; Direkt 0 zurÅckgeben	; Dito
;	MOVEQ       #0,D9           ; i in Schleife			; unnîtig, ich zÑhle in A0
;	MOVE.L      -(A3),A0        ; Basisadresse fÅr String  (a) ; Pure C nicht
loop:
;	CMP.W       D8,D9           ; Schleifenende erreicht? ; Stattdessen lieber:

	tst.b		(A0)			; Zeichen ist 0
	beq.s		retx
	cmp.b		#'@',(A0)		; Zeichen ist @

	BEQ.S       retx
	MOVE.W      D0,D2           ; D2 =  temp
;	 temp := XOR(ORD(a^[i]), d) MOD 256;
;	MOVE.B      0(A0,D9.W),D3				; ich brauche D9 nicht
	move.b		(A0), D3					; geht um einiges schneller
;	EXT.W       D3 							; unnîtig, Ergebnis wird eh weggeworfen.
	EOR.W       D3,D2
	ANDI.W      #$FF,D2
;	 temp := XOR(temp, temp DIV 16) MOD 256;
	MOVE.W      D2,D3
	LSR.W       #4,D3
	EOR.W       D3,D2
	ANDI.W      #$FF,D2
;	 d := XOR(XOR((temp * 16) MOD 256, temp DIV 8), e) MOD 256;
	MOVE.W      D2,D3
	MOVE.W      D2,D0
	LSL.W       #4,D3
	ANDI.W      #$FF,D3     ; * kann man weglassen
	LSR.W       #3,D0
	EOR.W       D3,D0
	EOR.W       D1,D0
	ANDI.W      #$FF,D0
;	 e := XOR(temp, (temp * 32) MOD 256) MOD 256;
	MOVE.W      D2,D3
	LSL.W       #5,D3
	ANDI.W      #$FF,D3     ; * kann man weglassen
	EOR.W       D3,D2
	ANDI.W      #$FF,D2
	MOVE.W      D2,D1
;	ADDQ.W      #1,D9						; Ich zÑhle lieber A0 hoch
	addq.w      #1,A0       				; Tja, hat sich was mit .W, was soll's.
	BRA.S       loop
retx:
	LSL.W       #8,D0
	ADD.W       D1,D0   ; Ergebnis am Ende in D0
ret:
	move.l      (sp)+,D3					; D3 wiedrherstellen
	RTS

/*
  END;
END CalcCrcArray;
(*$L=*)

Das ist genau die CRC, die in CAT verwendet wird. Die Parameter werden Åber A3
Åbergeben, das ist in Megamax-Modula nunmal so. Man kann das sicherlich noch
optimieren, aber dazu hatte ich keine Lust.

Die beiden ANDI.W #$FF, die im Kommentar mit * gekennzeichnet sind, kînnen auch
weggelassen werden, die sind eigentlich ÅberflÅssig, die hatte Jo leider in den
M2 Source ÅberflÅssigerweise eingebaut. Ohne die kommen ab und zu andere
Ergebnisse raus, aber fast immer die gleichen. Und da ja nicht die CAT-CRC
berechnet werden muû, geht es ohne die beiden etwas schneller.

 Gruû, Dirk
*/