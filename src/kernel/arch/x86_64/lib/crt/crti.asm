[bits 64]

global _init
global _fini

section .init
_init:
    xchg bx, bx

section .fini
_fini:
