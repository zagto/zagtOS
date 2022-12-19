static inline struct pthread *__pthread_self()
{
	struct pthread *self;
	__asm__ ("mrs %0,tpidr_el0" : "=r"(self));
	return self;
}

#define TLS_ABOVE_TP
#define GAP_ABOVE_TP 16

#define MC_PC pc

