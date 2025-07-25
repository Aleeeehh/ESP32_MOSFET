/**
 * @file DShotRMT.cpp
 * @brief DShot signal generation using ESP32 RMT with continuous repeat and pause between frames, including BiDirectional support
 * @author Wastl Kraus
 * @date 2025-06-11
 * @license MIT
 */

#include <DShotRMT.h>

//
DShotRMT::DShotRMT(gpio_num_t gpio, dshot_mode_t mode, bool isBidirectional)
    : _gpio(gpio), _mode(mode), _isBidirectional(isBidirectional) {}

// Sets up RMT TX and RX channels as well as encoder configuration
void DShotRMT::begin()
{
    // RX RMT Channel Configuration (for BiDirectional DShot)
    if (_isBidirectional)
    {
        _rmt_rx_channel_config = {
            .gpio_num = _gpio,
            .clk_src = DSHOT_CLOCK_SRC_DEFAULT,
            .resolution_hz = DSHOT_RMT_RESOLUTION,
            .mem_block_symbols = 64,
            .flags = {
                .invert_in = false,
                .with_dma = false}};

        rmt_new_rx_channel(&_rmt_rx_channel_config, &_rmt_rx_channel);
        rmt_enable(_rmt_rx_channel);

        _receive_config.signal_range_min_ns = 1000;
        _receive_config.signal_range_max_ns = 15000;
    }

    // TX RMT Channel Configuration
    _rmt_tx_channel_config = {
        .gpio_num = _gpio,
        .clk_src = DSHOT_CLOCK_SRC_DEFAULT,
        .resolution_hz = DSHOT_RMT_RESOLUTION,
        .mem_block_symbols = 64,
        .trans_queue_depth = 1,
        .flags = {
            // invert Signal if BiDirectional DShot Mode
            .invert_out = _isBidirectional,
            .with_dma = false}};

    rmt_new_tx_channel(&_rmt_tx_channel_config, &_rmt_tx_channel);
    rmt_enable(_rmt_tx_channel);

    // Use a copy encoder to send raw symbols
    if (!_dshot_encoder)
    {
        rmt_copy_encoder_config_t enc_cfg = {};
        rmt_new_copy_encoder(&enc_cfg, &_dshot_encoder);
    }

    // Configure transmission looping
    _transmit_config.loop_count = -1;
    _transmit_config.flags.eot_level = _isBidirectional;
}

// Encodes and transmits a valid DShot Throttle value (48 - 2047)
void DShotRMT::setThrottle(uint16_t throttle)
{
    // Safety first - double check input range and 11 bit "translation"
    if (throttle < DSHOT_THROTTLE_MIN) throttle = DSHOT_THROTTLE_MIN;
    if (throttle > DSHOT_THROTTLE_MAX) throttle = DSHOT_THROTTLE_MAX;
    throttle = throttle & 0b0000011111111111;

    // Has Throttle really changed?
    if (throttle == _lastThrottle)
        return;

    _lastThrottle = throttle;

    // Convert throttle value to DShot Paket Format
    _tx_packet = assambleDShotPaket(_lastThrottle);

    // Encode RMT symbols
    size_t count = 0;
    encodeDShotTX(_tx_packet, _tx_symbols, count);

    rmt_disable(_rmt_tx_channel);
    rmt_enable(_rmt_tx_channel);
    rmt_transmit(_rmt_tx_channel, _dshot_encoder, _tx_symbols, count * sizeof(rmt_symbol_word_t), &_transmit_config);
}

// --- Get eRPM from ESC ---
// Receives and decodes a response frame from ESC containing eRPM info
uint32_t DShotRMT::getERPM()
{
    if (_isBidirectional)
    {
        static size_t rx_size = sizeof(_rx_symbols);

        if (_rmt_rx_channel == nullptr)
            return _last_erpm;

        // Attempt to receive a new frame
        if (!rmt_receive(_rmt_rx_channel, _rx_symbols, rx_size, &_receive_config))
            return _last_erpm;

        uint16_t received_bits = 0;
        _received_packet = 0;

        // Decode raw RMT encoded bits
        for (int i = 0; i < DSHOT_BITS_PER_FRAME; ++i)
        {
            rmt_symbol_word_t symbols = _rx_symbols[i];

            // Validate signal polarity
            if (symbols.level0 != 1 || symbols.level1 != 0)
                break;

            uint32_t total_ticks = symbols.duration0 + symbols.duration1;
            bool bit = (symbols.duration0 > (total_ticks / 2));

            _received_packet <<= 1;
            _received_packet |= bit ? 1 : 0;

            received_bits++;
        }

        if (received_bits < 16)
            return _last_erpm;

        // Extract data & checksum from packet
        uint16_t packet_data = _received_packet >> 4;
        uint8_t recalc_packet_crc = (packet_data ^ (packet_data >> 4) ^ (packet_data >> 8)) & 0x0F;
        uint8_t packet_crc = _received_packet & 0x0F;

        if (recalc_packet_crc != packet_crc)
            return _last_erpm;

        // Assume received value is DShot eRPM
        uint16_t throttle = packet_data >> 1;

        // Filter noise values
        if (throttle < DSHOT_THROTTLE_MIN || throttle > DSHOT_THROTTLE_MAX)
            return _last_erpm;

        // Approximate eRPM (ESC dependent, scale factor can be tuned)
        _last_erpm = throttle * 100;
        return _last_erpm;
    }
    // Nothing to do here
    return _last_erpm;
}

// Translate eRPM value to RPM taking magnet count as parameter
uint32_t DShotRMT::getMotorRPM(uint8_t magnet_count)
{
    uint8_t pole_count = magnet_count / 2;

    if (pole_count == 0)
        pole_count = 1;

    uint32_t rpm = getERPM() / pole_count;
    return rpm;
}

// Calculate CRC for DShot Paket
uint16_t DShotRMT::calculateCRC(uint16_t dshot_packet)
{
    // Clear container before new calculation
    _packet_crc = DSHOT_NULL_PACKET;

    // CRC is inverted for biDirectional DSHot
    _packet_crc = _isBidirectional
                      ? (~(dshot_packet ^ (dshot_packet >> 4) ^ (dshot_packet >> 8))) & 0x0F
                      : (dshot_packet ^ (dshot_packet >> 4) ^ (dshot_packet >> 8)) & 0x0F;

    return _packet_crc;
}

// Assamble DShot Paket (10 bit throttle + 1 bit telemetry request + 4 bit crc)
uint16_t DShotRMT::assambleDShotPaket(uint16_t value)
{
    // Clear container
    _tx_packet = DSHOT_NULL_PACKET;

    // dummy 11bit convertion
    _tx_packet = value & 0b0000011111111111;

    // Assemble raw DShot packet and add checksum
    _tx_packet = (value << 1) | (_isBidirectional ? 1 : 0);
    _packet_crc = calculateCRC(_tx_packet);

    _tx_packet = (_tx_packet << 4) | _packet_crc;

    return _tx_packet;
}

// --- Encode DShot TX Frame ---
// Converts a 16-bit packet into a valid DShot Frame for RMT
void DShotRMT::encodeDShotTX(uint16_t dshot_packet, rmt_symbol_word_t *symbols, size_t &count)
{
    // Always start encoding from the top
    count = 0;

    //
    uint32_t ticks_per_bit = 0;
    uint32_t ticks_zero_high = 0;
    uint32_t ticks_one_high = 0;

    switch (_mode)
    {
    case DSHOT150:
        ticks_per_bit = 64;
        ticks_zero_high = 24;
        ticks_one_high = 48;
        break;
    case DSHOT300:
        ticks_per_bit = 32;
        ticks_zero_high = 12;
        ticks_one_high = 24;
        break;
    case DSHOT600:
        ticks_per_bit = 16;
        ticks_zero_high = 6;
        ticks_one_high = 12;
        break;
    case DSHOT1200:
        ticks_per_bit = 8;
        ticks_zero_high = 3;
        ticks_one_high = 6;
        break;
    // Safety first
    case DSHOT_OFF:
    default:
        ticks_per_bit = 0;
        ticks_zero_high = 0;
        ticks_one_high = 0;
        break;
    }

    //
    uint32_t ticks_zero_low = ticks_per_bit - ticks_zero_high;
    uint32_t ticks_one_low = ticks_per_bit - ticks_one_high;

    // Fill the 16 DShot-Bits Array with selected timings
    for (int i = 15; i >= 0; i--)
    {
        bool bit = (dshot_packet >> i) & 0x01;
        symbols[count].level0 = 1;
        symbols[count].duration0 = bit ? ticks_one_high : ticks_zero_high;
        symbols[count].level1 = 0;
        symbols[count].duration1 = bit ? ticks_one_low : ticks_zero_low;
        count++;
    }

    // Append the Pause Bits
    symbols[count].level0 = 0;
    symbols[count].duration0 = ticks_per_bit * PAUSE_BITS;
    symbols[count].level1 = 0;
    symbols[count].duration1 = 0;
    count++;
}
