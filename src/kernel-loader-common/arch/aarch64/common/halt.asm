.global basicHalt

.section ".text"

basicHalt:
    wfe
    b basicHalt
