#include <cstddef>
#include <cstdint>
#include <cstring>

#include "mbed.h"
#include "spirit.h"

static constexpr float default_kp = 0.30f;
static constexpr float default_ki = 0.80f;

static constexpr uint32_t pc_baud   = 11'5200;
static constexpr auto     loop_rate = 100ms;
static constexpr uint32_t dip_sw    = 0x00;
static constexpr float    max_rps   = 3.0f;

/**
 * @brief モーターのデータをCANに書き込む
 * @param can CAN bus
 * @param motor モーター
 * @retval 0 書き込み失敗
*  @retval 1 書き込み成功
 */
int write(CAN &can, spirit::Motor &motor)
{
    spirit::FakeUdpConverter   fake_udp;
    spirit::MotorDataConverter motor_data;
    const uint32_t             can_id = spirit::can::get_motor_id(1, 0, dip_sw);

    constexpr std::size_t max_motor_data_buffer_size = 64;
    uint8_t               motor_data_buffer[8]       = {};
    std::size_t           motor_data_buffer_size;

    constexpr std::size_t max_fake_udp_buffer_size = 64;
    uint8_t               fake_udp_buffer[8]       = {};
    std::size_t           fake_udp_buffer_size;

    motor_data.encode(motor, max_motor_data_buffer_size, motor_data_buffer, motor_data_buffer_size);

    fake_udp.encode(motor_data_buffer, motor_data_buffer_size, max_fake_udp_buffer_size, fake_udp_buffer,
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

    motor.control_system(spirit::Motor::ControlSystem::PWM);
    motor.pid_gain_factor(default_kp, default_ki, default_ki / 4.0f);

    motor.state(spirit::Motor::State::Brake);
    motor.duty_cycle(0.00F);

    write(can, motor);

    BufferedSerial pc(USBTX, USBRX, pc_baud);

    while (true) {
        if (pc.readable()) {
            char pc_data;
            pc.read(&pc_data, 1);
            if (pc_data >= '0' && pc_data <= '9') {
                if (motor.get_control_system() == spirit::Motor::ControlSystem::PWM) {
                    motor.duty_cycle((pc_data - '0') * 0.10f);
                    debug("duty : 0.%d0\r\n", (pc_data - '0'));
                } else if (motor.get_control_system() == spirit::Motor::ControlSystem::Speed) {
                    float rps = (pc_data - '0') / (9.0f / max_rps);
                    motor.speed(rps);
                    debug("rps  : %d.%d\r\n", (int)rps, (int)((rps - (int)rps) * 100));
                } else {
                    debug("control system error\r\n");
                }
            } else {
                switch (pc_data) {
                    case 'q':
                        motor.state(spirit::Motor::State::Coast);
                        debug("state : Coast\r\n");
                        break;
                    case 'w':
                        motor.state(spirit::Motor::State::CW);
                        debug("state : CW\r\n");
                        break;
                    case 'e':
                        motor.state(spirit::Motor::State::CCW);
                        debug("state : CCW\r\n");
                        break;
                    case 'r':
                        motor.state(spirit::Motor::State::Brake);
                        debug("state : Brake\r\n");
                        break;
                    case 'd':
                        motor.control_system(spirit::Motor::ControlSystem::PWM);
                        motor.state(spirit::Motor::State::Brake);
                        motor.duty_cycle(0.00F);
                        debug("PWM Mode\r\n");
                        break;
                    case 's':
                        motor.control_system(spirit::Motor::ControlSystem::Speed);
                        motor.state(spirit::Motor::State::Brake);
                        motor.speed(0.00f);
                        debug("Speed Control Mode\r\n");
                        break;
                    case 'g':
                        if (motor.get_control_system() == spirit::Motor::ControlSystem::PWM) {
                            debug("PWM Mode (No gain settings required)\r\n");
                            break;
                        }

                        float kp, ki;

                        motor.state(spirit::Motor::State::Brake);
                        write(can, motor);

                        debug("Gain Change Mode\r\nKp : ");

                        if (scanf("%f", &kp) != 1) {
                            kp = default_kp;
                            debug("scan error [kp = default_kp]\r\n");
                        }

                        debug("Ki : ");

                        if (scanf("%f", &ki)) {
                            ki = default_ki;
                            debug("scan error [ki = default_ki]\r\n");
                        }

                        motor.pid_gain_factor(kp, ki, ki / 4.0f);

                        debug("Speed Control Mode\r\n");

                        break;
                    default:
                        motor.state(spirit::Motor::State::Brake);
                        debug("***Input Error: Change state to Brake.***\r\n");
                        break;
                }
            }
        }

        write(can, motor);
        ThisThread::sleep_for(loop_rate);
    }
}
