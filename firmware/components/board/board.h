/*
 * board.h — Board-level initialization and GPIO abstraction
 */
#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize all board GPIOs, UART, I2C peripherals.
 *        Sets relays to default OFF state.
 */
void board_init(void);

/**
 * @brief Set relay state for a given slot.
 * @param slot  0-based slot index (0..NUM_SLOTS-1)
 * @param on    true = relay ON (socket powered), false = OFF
 */
void board_relay_set(uint8_t slot, bool on);

/**
 * @brief Toggle relay state for a given slot.
 * @param slot  0-based slot index
 */
void board_relay_toggle(uint8_t slot);

/**
 * @brief Get relay state for a given slot.
 * @param slot  0-based slot index
 * @return true if relay is ON
 */
bool board_relay_get_state(uint8_t slot);

/**
 * @brief Read the INT pin state for a given slot.
 * @param slot  0-based slot index
 * @return true if INT is asserted (module present/changed)
 */
bool board_slot_int_read(uint8_t slot);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
