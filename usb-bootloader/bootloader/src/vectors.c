void __bad_interrupt(void) __attribute__((naked, used, externally_visible));
void __reset(void)         __attribute__((naked, weak, signal, used, externally_visible));
void __vector_1(void)      __attribute__((naked, weak, signal, used, externally_visible));


#pragma weak __reset    = __bad_interrupt
#pragma weak __vector_1 = __bad_interrupt


void __bad_interrupt(void) {
	__asm__ __volatile__ ("rjmp __vectors \n\t"::);
}


static void __vectors(void) __attribute__ ((section( ".vectors" ), naked, used));
static void __vectors(void) {

    __asm__ __volatile__ (
        "jmp __reset"    "\n\t"
    	"jmp __vector_1" "\n\t" // INT0
        ::
    );
}
