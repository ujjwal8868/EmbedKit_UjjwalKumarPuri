C Codes:

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define BUFFER_SIZE     8U  /* Must be a power of 2 for bitwise optimization */
#define BUFFER_MASK     (BUFFER_SIZE - 1U)

/* Status and Error Return Codes */
typedef enum {
    RINGBUF_SUCCESS = 0,
    RINGBUF_ERR_FULL = -1,
    RINGBUF_ERR_EMPTY = -2
} ringbuf_status_t;

/* Ring Buffer Structure definition */
typedef struct {
    uint8_t storage[BUFFER_SIZE];
    uint16_t head;   /* Write index */
    uint16_t tail;   /* Read index */
    uint16_t count;  /* Current element count */
} ring_buffer_t;

/**
 * @brief Initializes the ring buffer to an empty state.
 * @param buf Pointer to the ring buffer structure.
 */
void ringbuf_init(ring_buffer_t *buf) {
    if (buf != NULL) {
        buf->head = 0;
        buf->tail = 0;
        buf->count = 0;
    }
}

/**
 * @brief Checks if the buffer is completely full.
 */
bool ringbuf_is_full(const ring_buffer_t *buf) {
    return (buf->count == BUFFER_SIZE);
}

/**
 * @brief Checks if the buffer is completely empty.
 */
bool ringbuf_is_empty(const ring_buffer_t *buf) {
    return (buf->count == 0);
}

/**
 * @brief Queries the current number of bytes stored.
 */
uint16_t ringbuf_get_count(const ring_buffer_t *buf) {
    return buf->count;
}

/**
 * @brief Writes one byte into the buffer.
 * @param buf Pointer to the ring buffer structure.
 * @param data Byte value to write.
 * @return RINGBUF_SUCCESS on success, or RINGBUF_ERR_FULL if buffer is full.
 */
ringbuf_status_t ringbuf_write(ring_buffer_t *buf, uint8_t data) {
    if (ringbuf_is_full(buf)) {
        return RINGBUF_ERR_FULL;
    }

    buf->storage[buf->head] = data;

    /* * BONUS TASK OPTIMIZATION: 
     * Instead of using 'buf->head = (buf->head + 1) % BUFFER_SIZE;', we use a bitwise AND.
     * * Why it's faster: The modulo (%) operator translates into a hardware integer division instruction
     * on most architectures. On low-end or traditional Microcontrollers (MCUs) lacking a dedicated hardware 
     * divider, division is software-emulated, draining dozens of CPU cycles. The bitwise AND (&) maps to a 
     * single, single-cycle assembly instruction executed instantly inside an interrupt service routine (ISR).
     * * Why it only works for power of 2: A power of 2 number minus 1 yields a continuous block of 1s in 
     * binary (e.g., 8 is 00001000, 8 - 1 = 7, which is 00000111). Bitwise ANDing with this mask safely keeps the 
     * value within the binary range [0 to BUFFER_SIZE-1] and flips it perfectly back to 0 upon overflow.
     */
    buf->head = (buf->head + 1U) & BUFFER_MASK;
    buf->count++;

    return RINGBUF_SUCCESS;
}

/**
 * @brief Reads one byte from the buffer.
 * @param buf Pointer to the ring buffer structure.
 * @param data Pointer to store the read byte value.
 * @return RINGBUF_SUCCESS on success, or RINGBUF_ERR_EMPTY if buffer is empty.
 */
ringbuf_status_t ringbuf_read(ring_buffer_t *buf, uint8_t *data) {
    if (ringbuf_is_empty(buf)) {
        return RINGBUF_ERR_EMPTY;
    }

    *data = buf->storage[buf->tail];
    buf->tail = (buf->tail + 1U) & BUFFER_MASK; /* Bitwise wrap-around optimization */
    buf->count--;

    return RINGBUF_SUCCESS;
}

/**
 * @brief Main function executing the strict evaluation workflow sequence.
 */
int main(void) {
    ring_buffer_t my_buffer;
    ringbuf_init(&my_buffer);
    
    // 1. Write 8 bytes (0x41 to 0x48) one at a time
    uint8_t initial_bytes[8] = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48};
    for (uint32_t i = 0; i < 8; i++) {
        if (ringbuf_write(&my_buffer, initial_bytes[i]) == RINGBUF_SUCCESS) {
            printf("[WRITE] 0x%02X -> OK (count=%u)%s\n", 
                   initial_bytes[i], 
                   ringbuf_get_count(&my_buffer),
                   ringbuf_is_full(&my_buffer) ? " FULL" : "");
        }
    }

    // 2. Attempt to write one more byte (0x99) to a full buffer
    uint8_t overflow_byte = 0x99;
    if (ringbuf_write(&my_buffer, overflow_byte) == RINGBUF_ERR_FULL) {
        printf("[WRITE] 0x%02X -> FAIL (buffer full)\n", overflow_byte);
    } else {
        printf("[WRITE] 0x%02X -> OK (count=%u)\n", overflow_byte, ringbuf_get_count(&my_buffer));
    }

    // 3. Read 3 bytes one at a time
    uint8_t read_val = 0;
    for (uint32_t i = 0; i < 3; i++) {
        if (ringbuf_read(&my_buffer, &read_val) == RINGBUF_SUCCESS) {
            printf("[READ] -> 0x%02X (count=%u)\n", read_val, ringbuf_get_count(&my_buffer));
        }
    }

    // 4. Write 3 new bytes: 0x49, 0x4A, 0x4B
    uint8_t next_bytes[3] = {0x49, 0x4A, 0x4B};
    for (uint32_t i = 0; i < 3; i++) {
        if (ringbuf_write(&my_buffer, next_bytes[i]) == RINGBUF_SUCCESS) {
            printf("[WRITE] 0x%02X -> OK (count=%u)%s\n", 
                   next_bytes[i], 
                   ringbuf_get_count(&my_buffer),
                   ringbuf_is_full(&my_buffer) ? " FULL" : "");
        }
    }

    // 5. Read all remaining 8 bytes one at a time
    while (!ringbuf_is_empty(&my_buffer)) {
        if (ringbuf_read(&my_buffer, &read_val) == RINGBUF_SUCCESS) {
            printf("[READ] -> 0x%02X (count=%u)\n", read_val, ringbuf_get_count(&my_buffer));
        }
    }

    // 6. Attempt to read from the now empty buffer
    if (ringbuf_read(&my_buffer, &read_val) == RINGBUF_ERR_EMPTY) {
        printf("[READ] (empty) -> FAIL (buffer empty)\n");
    }

    return 0;
}
