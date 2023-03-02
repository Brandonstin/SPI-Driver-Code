/*
 * eeprom.c
 *
 */

#include <eeprom.h>

/** Initializes the IO pins used to control the CS of the
 *  EEPROM
 *
 *
 *
 */
cy_rslt_t eeprom_cs_init(void)
{
    return cyhal_gpio_init(EEPROM_CS_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, true);
}

/** Determine if the EEPROM is busy writing the last
 *  transaction to non-volatile storage
 *
 * @param
 *
 */
cy_rslt_t eeprom_wait_for_write(void)
{
    uint8_t transmit_data[2] = {EEPROM_CMD_RDSR, 0x0F};
    uint8_t receive_data[2];
    cyhal_gpio_write(EEPROM_CS_PIN, 0);
    uint8_t wip;
    cy_rslt_t rslt;

    while (wip)
    {
        cyhal_gpio_write(EEPROM_CS_PIN, 0);

        rslt = cyhal_spi_transfer(&mSPI, transmit_data, 4u, receive_data, 4u, 0xFF);
        wip = (receive_data[1] == 0x01);

        cyhal_gpio_write(EEPROM_CS_PIN, 1);
    }
    return rslt;
}

/** Enables Writes to the EEPROM
 *
 * @param
 *
 */
cy_rslt_t eeprom_write_enable(void)
{
    cyhal_gpio_write(EEPROM_CS_PIN, 0);
    if (CY_RSLT_SUCCESS == eeprom_wait_for_write())
    {
        if (CY_RSLT_SUCCESS == cyhal_spi_send(&mSPI, EEPROM_CMD_WREN))
        {
            cyhal_gpio_write(EEPROM_CS_PIN, 1);
            return CY_RSLT_SUCCESS;
        }
    }
    return 0;
}

/** Disable Writes to the EEPROM
 *
 * @param
 *
 */
cy_rslt_t eeprom_write_disable(void)
{
    cyhal_gpio_write(EEPROM_CS_PIN, 0);
    if (CY_RSLT_SUCCESS == eeprom_wait_for_write())
    {
        if (CY_RSLT_SUCCESS == cyhal_spi_send(&mSPI, EEPROM_CMD_WRDI))
        {
            cyhal_gpio_write(EEPROM_CS_PIN, 1);
            return CY_RSLT_SUCCESS;
        }
    }
    return 0;
}

/** Writes a single byte to the specified address
 *
 * @param address -- 16 bit address in the EEPROM
 * @param data    -- value to write into memory
 *
 */
cy_rslt_t eeprom_write_byte(uint16_t address, uint8_t data)
{
	uint8_t MSBaddr = (address & 0x0F00)>>8;
	uint8_t LSBaddr = (address & 0x00FF);
    eeprom_write_enable();
    cyhal_gpio_write(EEPROM_CS_PIN, 0);
    eeprom_wait_for_write();
    // Sends instruction and makes sure result is returned correctly.
    if (CY_RSLT_SUCCESS == cyhal_spi_send(&mSPI, EEPROM_CMD_WRITE))
    {
        // Sends the upper byte and makes sure result is returned correctly.
        if (CY_RSLT_SUCCESS == cyhal_spi_send(&mSPI, MSBaddr))
        {
            // Sends the lower byte and makes sure result is returned correctly.
            if (CY_RSLT_SUCCESS == cyhal_spi_send(&mSPI, LSBaddr))
            {
                // Now that we have the address, sends the data.
                if (CY_RSLT_SUCCESS == cyhal_spi_send(&mSPI, data))
                {
                    // disable the pin now that we don't need it.
                    cyhal_gpio_write(EEPROM_CS_PIN, 1);
                    return CY_RSLT_SUCCESS;
                }
            }
        }
    }

    return 0;
}

/** Reads a single byte to the specified address
 *
 * @param address -- 16 bit address in the EEPROM
 * @param data    -- value read from memory
 *
 */
cy_rslt_t eeprom_read_byte(uint16_t address, uint8_t *data)
{
	uint8_t MSBaddr = (address & 0x0F00)>>8;
	uint8_t LSBaddr = (address & 0x00FF);
	eeprom_wait_for_write();
	cyhal_gpio_write(EEPROM_CS_PIN, 0);
	if (CY_RSLT_SUCCESS == cyhal_spi_send(&mSPI, EEPROM_CMD_WRITE)){
		if(CY_RSLT_SUCCESS == cyhal_spi_send(&mSPI, MSBaddr)){
			if(CY_RSLT_SUCCESS == cyhal_spi_send(&mSPI, LSBaddr)){
				uint8_t* temp = data;
				if(CY_RSLT_SUCCESS == cyhal_spi_recv(&mSPI, temp)){
					cyhal_gpio_write(EEPROM_CS_PIN, 1);
					return CY_RSLT_SUCCESS;
				}
			}
		}
	}
	return 0;
}

/** Tests Writing and Reading the EEPROM
 *
 * @param
 *
 */
cy_rslt_t eeprom_test(void)
{
    uint8_t i;
    uint16_t addr;
    cy_rslt_t rslt;
    uint8_t data;
    uint8_t expected_data;

    // Write the data to the eeprom.
    addr = 0x20;
    data = 0x10;
    for (i = 0; i < 20; i++)
    {
        rslt = eeprom_write_byte(addr, data);
        if (rslt != CY_RSLT_SUCCESS)
        {
            printf("* -- EEPROM WRITE FAILURE\n\r");
            return -1;
        }
        addr++;
        data++;
    }

    // Read the data back and verify everything matches what was written
    addr = 0x20;
    expected_data = 0x10;
    for (i = 0; i < 20; i++)
    {
        rslt = eeprom_read_byte(addr, &data);
        if (rslt != CY_RSLT_SUCCESS)
        {
            printf("* -- EEPROM READ FAILURE\n\r");
            return -1;
        }
        if (expected_data != data)
        {
            printf("%d \n", data);
            printf("%d \n", expected_data);
            printf("* -- EEPROM READ DATA DOES NOT MATCH\n\r");
            return -1;
        }

        addr++;
        expected_data++;
    }
    printf("* -- EEPROM Test Passed\n\r");
    return CY_RSLT_SUCCESS;
}
