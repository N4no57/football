#include "ball.h"

#define MAGNUS_K 0.5f
#define BALL_DRAG_COEFFICIENT 0.25f
#define ANGULAR_DRAG 0.5f
#define FRICTION_COEFFICIENT 0.5f

void apply_strike(Ball *ball, const StrikeAction *strike) {
    const Vector3 r = Vector3Scale(
        strike-> strike_point,
        ball->radius
    );

    const Vector3 d = Vector3Normalize(strike->strike_vector);

    const float impulse_magnitude = strike->power * 10;

    const Vector3 J = Vector3Scale(d, impulse_magnitude);

    ball->velocity = Vector3Scale(J, 1.0f / ball->mass);

    const Vector3 angular_impulse = Vector3CrossProduct(r, J);

    const float inertia = 0.4f * ball->mass * ball->radius * ball->radius;

    ball->angular_velocity = Vector3Scale(angular_impulse, 1.0f / inertia);
}

void update_ball(Ball *ball, const float dt) {
    const float speed = Vector3Length(ball->velocity);

    if (speed > 0.001f) {
        constexpr float air_density = 1.225f;

        const float area = PI * ball->radius * ball->radius;

        const Vector3 velocity_direction =
            Vector3Normalize(ball->velocity);

        const float drag_force =
            0.5f *
            air_density *
            BALL_DRAG_COEFFICIENT *
            area *
            speed * speed;

        const Vector3 drag =
            Vector3Scale(
                velocity_direction,
                -drag_force
            );

        ball->velocity = Vector3Add(
            ball->velocity,
            Vector3Scale(
                drag,
                dt / ball->mass
            )
        );

        const float omega = Vector3Length(ball->angular_velocity);
        if (omega > 0.001f) {
            const float spin_ratio = omega * ball->radius / speed;

            const float lift_coefficient = MAGNUS_K * spin_ratio;

            const Vector3 lift_direction =
                Vector3Normalize(
                    Vector3CrossProduct(ball->angular_velocity, ball->velocity)
                );

            const float magnus_force_magnitude =
                0.5f *
                air_density *
                area *
                lift_coefficient *
                speed * speed;

            const Vector3 magnus_force =
                Vector3Scale(
                    lift_direction,
                    magnus_force_magnitude
                );

            const Vector3 magnus_acceleration =
                Vector3Scale(
                    magnus_force,
                    1.0f / ball->mass
                );

            ball->velocity = Vector3Add(
                ball->velocity,
                Vector3Scale(magnus_acceleration, dt)
            );
        }
    }

    ball->velocity.y += -9.81f * dt;

    ball->position = Vector3Add(
        ball->position,
        Vector3Scale(
            ball->velocity,
            dt
        )
    );

    if (ball->position.y <= ball->radius) {
        ball->position.y = ball->radius;

        if (ball->velocity.y < 0.0f)
            ball->velocity.y *= -0.5f;

        const Vector3 r = {
            0,
            -ball->radius,
            0
        };

        Vector3 contact_velocity = Vector3Add(
            ball->velocity,
            Vector3CrossProduct(
                ball->angular_velocity,
                r
            )
        );

        contact_velocity.y = 0;

        const float contact_speed = Vector3Length(contact_velocity);

        if (contact_speed > 0.001f) {
            const Vector3 friction_direction = Vector3Scale(
                Vector3Normalize(contact_velocity),
                -1
            );

            const float normal_force = ball->mass * 9.81f;

            const float friction_force = FRICTION_COEFFICIENT * normal_force;

            const Vector3 friction = Vector3Scale(
                friction_direction,
                friction_force
            );

            ball->velocity = Vector3Add(
                ball->velocity,
                Vector3Scale(
                    friction,
                    dt / ball->mass
                )
            );

            const float inertia =
                0.4f *
                ball->mass *
                ball->radius *
                ball->radius;

            const Vector3 torque = Vector3CrossProduct(
                r,
                friction
            );

            const Vector3 angular_acceleration =Vector3Scale(
                torque,
                1.0f / inertia
            );

            ball->angular_velocity = Vector3Add(
                ball->angular_velocity,
                Vector3Scale(
                    angular_acceleration,
                    dt
                )
            );
        }
    }

    ball->angular_velocity =
    Vector3Scale(
        ball->angular_velocity,
        expf(-ANGULAR_DRAG * dt)
    );

    ball->rotation = Vector3Add(
        ball->rotation,
        Vector3Scale(
            ball->angular_velocity,
            dt
        )
    );
}
