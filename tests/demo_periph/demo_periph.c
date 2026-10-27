// Demo system peripherals: GPIO output readback, timer interrupts and UART
// output (shown as uart0 output by the Makefile).

#include "gpio.h"
#include "test_utils.h"
#include "timer.h"
#include "uart.h"

// Simulation UART settings, from ibex-demo-system/dv/verilator/top_verilator.sv
#define CLOCK_HZ 50000000
#define BAUD_RATE 115200
#define CYCLES_PER_CHAR (10 * CLOCK_HZ / BAUD_RATE)  // start + 8 data + stop bits

static void uart_puts(const char *s) {
  while (*s) {
    uart_out(DEFAULT_UART, *s++);
  }
}

// The UART has no "transmit done" status, only "TX FIFO full". Wait long
// enough for n characters to leave the FIFO before the simulation is halted.
static void uart_wait_sent(uint32_t n) {
  uint32_t start = read_cycles();
  while (read_cycles() - start < (n + 1) * CYCLES_PER_CHAR) {
  }
}

int main(void) {
  // GPIO: outputs read back through the GPIO_OUT register
  set_outputs(GPIO_OUT, 0xA5);
  CHECK_EQ(read_gpio(GPIO_OUT), 0xA5);
  set_output_bit(GPIO_OUT, 1, 1);
  CHECK_EQ(get_output_bit(GPIO_OUT, 1), 1);
  CHECK_EQ(read_gpio(GPIO_OUT), 0xA7);

  // Timer: wait for 3 timer interrupts
  timer_init();
  timer_enable(500);
  uint32_t start = read_cycles();
  while (get_elapsed_time() < 3) {
    CHECK(read_cycles() - start < 100000);  // interrupts must arrive
    asm volatile("wfi");
  }
  timer_disable();
  puts("timer interrupts: 0x");
  puthex((uint32_t)get_elapsed_time());
  putchar('\n');

  // UART: transmit a line. In simulation it ends up in uart0.log.
  static const char msg[] = "Hello over UART\r\n";
  uart_puts(msg);
  uart_wait_sent(sizeof(msg) - 1);

  test_pass();
  return 0;
}
