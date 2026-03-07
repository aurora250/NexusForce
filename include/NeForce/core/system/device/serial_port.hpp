#ifndef NEFORCE_CORE_SYSTEM_DEVICE_SERIAL_PORT_HPP__
#define NEFORCE_CORE_SYSTEM_DEVICE_SERIAL_PORT_HPP__
#include "device.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API serial_port final : public device {
public:
    struct serial_config {
        uint32_t baud_rate = 115200u;
        byte_t data_bits = 8u;
        byte_t stop_bits = 1u;
        char parity = 'N';
        bool flow_control = false;
        bool xon_xoff = false;
        bool break_enable = false;
        bool dsr_sensitivity = false;
        bool dtr_control = true;
        bool rts_control = true;

        milliseconds read_interval_timeout{0};
        milliseconds read_total_timeout_multiplier{0};
        milliseconds read_total_timeout_constant{0};
        milliseconds write_total_timeout_multiplier{0};
        milliseconds write_total_timeout_constant{0};

        explicit serial_config(uint32_t baud_rate = 115200)
            : baud_rate(baud_rate) {}
    };

    struct modem_status {
        bool cts{false};
        bool dsr{false};
        bool ri{false};
        bool dcd{false};
    };

    struct line_status {
        bool framing_error{false};
        bool parity_error{false};
        bool overrun_error{false};
        bool break_detected{false};
    };

    explicit serial_port(const string& port_name,
        const serial_config& config = serial_config{},
        DEVICE_OPEN_FLAG flags = DEVICE_OPEN_FLAG::NONE);

    void configure(const serial_config& config);
    serial_config get_configuration() const;

    void set_rts(bool state);
    void set_dtr(bool state);
    bool get_cts() const;
    bool get_dsr() const;
    bool get_ri() const;
    bool get_dcd() const;
    modem_status get_modem_status() const;

    void set_break(bool enable);
    line_status get_line_status() const;

    void purge_rx_buffer();
    void purge_tx_buffer();
    void purge_both_buffers();

    size_t get_rx_queue_size() const;
    size_t get_tx_queue_size() const;

private:
    serial_config current_config_{};

#ifdef NEFORCE_PLATFORM_WINDOWS
    void configure_windows(const serial_config& config);
    modem_status get_modem_status_windows() const;
#else
    void configure_linux(const serial_config& config);
    modem_status get_modem_status_linux() const;
    line_status get_line_status_linux() const;
#endif
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_DEVICE_SERIAL_PORT_HPP__
