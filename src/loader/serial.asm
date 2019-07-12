; Serial Ports interaction
; for more info, see:
; https://wiki.osdev.org/Serial_Ports
;
[bits 64]

global InitSerial
global WriteSerial

%define SERIAL_PORT 0x3f8

section .text

InitSerial:
    ; disable interrupts
    mov dx, SERIAL_PORT + 1
    mov ax, 0
    out dx, al

    ; enable DLAB
    mov dx, SERIAL_PORT + 3
    mov ax, 0x80
    out dx, al

    ; set divisor to 3 (low byte)
    mov dx, SERIAL_PORT
    mov ax, 3
    out dx, al

    ; high byte
    mov dx, SERIAL_PORT + 1
    mov ax, 0
    out dx, al

    ; 8 bits, no parity, 1 stop bit
    mov dx, SERIAL_PORT + 3
    mov ax, 0x03
    out dx, al

    ; Enable FIFO, clear them, with 14-byte threshold
    mov dx, SERIAL_PORT + 2
    mov ax, 0xc7
    out dx, al

    ; IRQs enabled, RTS/DSR set
    mov dx, SERIAL_PORT + 4
    mov ax, 0x0b
    out dx, al

    ret


WriteSerial:
    mov dx, SERIAL_PORT + 5
.checkTransmitEmpty:
    in al, dx
    test al, 0x20
    je .checkTransmitEmpty

    mov dx, SERIAL_PORT
    mov ax, di
    out dx, al

    ret

