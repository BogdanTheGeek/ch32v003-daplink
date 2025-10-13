

SWD Test code:
```c
#if 0
    extern void SWJ_Sequence(uint32_t bitCount, const uint8_t *data);
    extern void SWD_Sequence(uint32_t info, const uint8_t *swdo, uint8_t *swdi);
    extern uint8_t SWD_Transfer(uint32_t request, uint32_t * data);

    const uint8_t data[] = "Hello, World!";
    static uint8_t buffer[64];
    memset(buffer, 0xFF, sizeof(buffer));

    uint8_t jtagswd[2] = {0x9E, 0xE7};
    uint8_t nil[2] = {0x00, 0x00};
    uint32_t idcode = DAP_TRANSFER_RnW;
    LOGI(TAG, "SWD Test icode 0x%x", idcode);

    uint32_t val = 0;
    PORT_SWD_SETUP();
    while (1)
    {
        SWJ_Sequence(50, buffer);
        Delay_Us(500);
        SWJ_Sequence(16, jtagswd);
        Delay_Us(500);
        SWJ_Sequence(50, buffer);
        Delay_Us(500);
        SWJ_Sequence(12, nil);
        Delay_Us(500);
        uint8_t ack = SWD_Transfer(idcode, &val);
        LOGI(TAG, "ack=%02X val=%08X", ack, val);
        // SWD_Sequence(sizeof(data), (const uint8_t *)data, (uint8_t *)buffer);
        Delay_Ms(2000);
    }
#endif
```
