#pragma once

namespace mrsd
{
	struct Enemy
	{
		int x, y;
		float turret_angle, min_angle, max_angle, turret_speed;
		int min_force, max_force;
		float firing_speed;
		float firing_chance;
		float firing_delay;
		float firing_randomness;
		Enemy():x(0), y(0),
			turret_angle(0),
			min_angle(0),
			max_angle(0),
			turret_speed(.01),
			min_force(10),
			max_force(11),
			firing_speed(0),
			firing_chance(0),
			firing_delay(0),
			firing_randomness(0)
		{}
	};
}
