#include <cstddef>
#include <cstdint>
#include <cstring>

#include "mbed.h"
#include "spirit/include/FakeUdpConverter.h"
#include "spirit/include/Id.h"
#include "spirit/include/Motor.h"
#include "spirit/include/PwmDataConverter.h"

static constexpr uint32_t pc_baud   = 11'5200;
static constexpr auto     loop_rate = 100ms;
static constexpr uint32_t dip_sw    = 0x00;

/**
 * @brief モーターのデータをCANに書き込む
 * @param can CAN bus
 * @param motor モーター
 * @retval 0 書き込み失敗
*  @retval 1 書き込み成功
 */
int write(CAN &can, spirit::Motor &motor)
{
    spirit::FakeUdpConverter fake_udp;
    spirit::PwmDataConverter pwm_data;
    const uint32_t           can_id = spirit::can::get_motor_id(1, 0, dip_sw);

    constexpr std::size_t max_pwm_data_buffer_size = 64;
    uint8_t               pwm_data_buffer[8]       = {};
    std::size_t           pwm_data_buffer_size;

    constexpr std::size_t max_fake_udp_buffer_size = 64;
    uint8_t               fake_udp_buffer[8]       = {};
    std::size_t           fake_udp_buffer_size;

    pwm_data.encode(motor, max_pwm_data_buffer_size, pwm_data_buffer, pwm_data_buffer_size);
    fake_udp.encode(pwm_data_buffer, pwm_data_buffer_size, max_fake_udp_buffer_size, fake_udp_buffer,
                    fake_udp_buffer_size);

    CANMessage msg;
    msg.id = can_id;
    if (fake_udp_buffer_size % 8) {
        msg.len = (fake_udp_buffer_size / 8) + 1;
    } else {
        msg.len = fake_udp_buffer_size / 8;
    }
    memmove(msg.data, fake_udp_buffer, 8);

    return can.write(msg);
}

int main()
{
    CAN can(p30, p29);

    spirit::Motor motor;
    motor.state(spirit::Motor::State::Brake);
    motor.duty_cycle(0.00F);

    write(can, motor);

    BufferedSerial pc(USBTX, USBRX, pc_baud);

    while (true) {
        // TODO PCから操作して逐次データを送信

        write(can, motor);
        ThisThread::sleep_for(loop_rate);
    }
}
