#include "Game.h"
#include <cmath>
#include <cstdlib>
#include "Frontend.h"
#include <iostream>

namespace mrsd
{
	Game::Game(int w, int h, float timestep) : w(w), h(h), timestep(timestep)
	{
		gravity = -9.81f;
		dangerZone = new int[w + 1];
		for (int i = 0; i < w + 1; ++i)
			dangerZone[i] = 0;
	}

	Game::~Game()
	{
		delete[] dangerZone;
	}

	void Game::newPlayer(Player *p)
	{
		players.push_back(p);
	}

	Enemy &Game::newEnemy(int x, int y)
	{
		Enemy e;
		e.x = x;
		e.y = y;
		e.firing_randomness = 0;
		e.firing_chance = 0;
		e.firing_speed = 5;
		e.min_force = 0;
		e.max_force = 10;
		e.turret_speed = .05f;
		enemies.push_back(e);
		return enemies.back();
	}

	bool Game::tickEnemy(Enemy &e)
	{
		const float t = timestep;
		e.turret_angle = std::fmod(e.turret_angle + e.turret_speed * t, e.max_angle - e.min_angle);
		e.firing_chance = e.firing_chance + e.firing_speed;
		int shouldFire = (1 - e.firing_randomness) * 100 + e.firing_randomness * (std::rand() % 100);
		if (e.firing_chance * 100 > shouldFire)
		{
			e.firing_chance = -e.firing_delay;
			float force = (float)(std::rand() % (e.max_force - e.min_force)) + e.min_force;
			Projectile p;
			p.x = e.x;
			p.y = e.y;
			p.vx = force * std::cos(e.turret_angle + e.min_angle);
			p.vy = force * std::sin(e.turret_angle + e.min_angle);
			projectiles.push_back(p);
		}
		return true;
	}

	bool Game::tickPlayer(Player &p)
	{
		p.x = std::min((float)w, std::max(0.f, p.x));
		if (dangerZone[(int)std::floor(p.x)] > 0 &&
			dangerZone[(int)std::ceil(p.x)] > 0)
		{
			p.dead = true;
			playerDead = true;
			return false;
		}
		return true;
	}

	bool Game::tickExplosion(Explosion &e)
	{
		const float t = timestep;
		e.time += t;
		if (e.time > explosionTime)
		{
			for (int i = std::floor(e.x - explosionSize);
				 i <= std::ceil(e.x + explosionSize);
				 ++i)
			{
				if (i >= 0 && i <= w)
					dangerZone[i]--;
			}
			return false;
		}
		return true;
	}

	bool Game::tickProjectile(Projectile &p)
	{
		const float t = timestep;
		p.x += p.vx * t;
		p.y += p.vy * t + .5f * t * t * gravity;
		p.vy += t * gravity;
		if (p.y <= 0)
		{
			explode(p.x);
			return false;
		}
		return true;
	}

	void Game::tick()
	{
		time += timestep;
		// Update all projectiles
		for (std::list<Projectile>::iterator it = projectiles.begin();
			 it != projectiles.end();)
		{
			if (!tickProjectile(*it))
				it = projectiles.erase(it);
			else
				++it;
		}
		for (std::list<Explosion>::iterator it = explosions.begin();
			 it != explosions.end();)
		{
			if (!tickExplosion(*it))
				it = explosions.erase(it);
			else
				++it;
		}
		// Check if any players are dead
		for (std::vector<Player *>::iterator it = players.begin(); it != players.end();)
		{
			if (!tickPlayer(**it))
				it = players.erase(it);
			else
				++it;
		}

		// Update all enemies
		for (std::vector<Enemy>::iterator it = enemies.begin();
			 it != enemies.end();)
		{
			if (!tickEnemy(*it))
				it = enemies.erase(it);
			else
				++it;
		}
	}

	void Game::explode(float x)
	{
		explosions.push_back(Explosion());
		Explosion &exp = explosions.back();
		exp.time = 0;
		exp.x = x;
		exp.y = 0;
		for (int i = std::floor(exp.x - explosionSize);
			 i <= std::ceil(exp.x + explosionSize);
			 ++i)
		{
			if (i >= 0 && i <= w)
				dangerZone[i]++;
		}
	}

}
