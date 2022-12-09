.global returnFromInterrupt
.global syscallEntry

.section ".text"

returnFromInterrupt:
    b returnFromInterrupt

syscallEntry:
    b syscallEntry
