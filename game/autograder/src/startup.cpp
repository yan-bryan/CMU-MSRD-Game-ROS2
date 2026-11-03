#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "Game.h"
#include "gl_frontend.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;
using namespace mrsd;

void setupScenarioEasy(Game &);
void setupScenarioMedium(Game &);
void setupScenarioHard(Game &);
void setupScenarioVeryHard(Game &);
void setupScenarioImpossible(Game &);

int executeScenario(gl::gl_frontend &glfe, Game &g){
	int iterations = 0;
	while (iterations < (int) (90/g.getTimeStep()) && !g.playerDead)
	{
		g.tick();
		// glfe.setupDraw();
		// glfe.drawGame(g);
		// glfe.finishDraw();
		glfe.update(g, g.getTimeStep());
		iterations += 1;
	}
	return iterations;
}


int main(void)
{
	gl::gl_frontend glfe;
	Game g(200, 200, .1f);
	glfe.init(g);
	glfe.startai(g);

	glfe.projectileScale = 1.f;
	glfe.playerScale = 1.f;
	glfe.enemyScale = 3.f;

	std::function<void(Game &)> setupScenarios[] = {
        setupScenarioEasy,
        setupScenarioMedium,
        setupScenarioHard,
        setupScenarioVeryHard,
        setupScenarioImpossible
    };

	float score = 0;
	int maxLevelReached = 0;
	int totalIterations = 0;
	// time in seconds
	
	for(int i = 0; i < 5; i++){
		maxLevelReached = i;
		setupScenarios[i](g);
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		int time_lasted = executeScenario(glfe, g);
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		float timer = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
		totalIterations += time_lasted;
		score += (maxLevelReached + 1) * time_lasted / timer;
		std::cout << "Score: " << score << std::endl;
		if(g.playerDead){
			break;
		}
	}
	
	

	std::string level = "";
	switch(maxLevelReached){
		case 0:
			level = "Easy";
			break;
		case 1:
			level = "Medium";
			break;
		case 2:
			level = "Hard";
			break;
		case 3:
			level = "Very Hard";
			break;
		case 4:
			level = "Impossible";
			break;
		default:
			level = "Easy";
			break;
	}
	std::cout << "Final Score: " << score << std::endl;
	std::cout << "Max Level Reached: " << level << std::endl;

	json j;
	// std::ofstream autograder_result("results.json");
	std::ofstream autograder_result("/autograder/results/results.json");

	json leaderboardArray = json::array();
	leaderboardArray.push_back({{"name", "Score"}, {"value", score}, {"order", "desc"}});
    leaderboardArray.push_back({{"name", "Maximum Level Reached"}, {"value", maxLevelReached}});

	j["leaderboard"] = leaderboardArray;
	if(score >= 2700){
		j["score"] = 5;
	}
	else{
		j["score"] = 0;
	}
	// if (timer < 90) // Fail
	// 	j["score"] = 0;
	// else // Success
	// 	j["score"] = 5;

	autograder_result << std::setw(4) << j << std::endl;

	return 0;
}

void setupScenarioEasy(Game &g)
{
	// Setup Game Constants
	g.explosionTime = 2.f;
	g.explosionSize = 5.f;
	g.playerSpeed = .25f;

	// Construct Enemies
	Enemy &e = g.newEnemy(g.getWidth() / 2, 10);
	e.min_angle = 3.14159f / 2 - .3f;
	e.max_angle = 3.14159f / 2 + .3f;
	e.min_force = 50;
	e.max_force = 51;
	e.firing_randomness = .1f;
	e.firing_speed = 1e-2f;
}

void setupScenarioMedium(Game &g)
{
	// Setup Game Constants
	g.explosionTime = 2.f;
	g.explosionSize = 5.f;
	g.playerSpeed = .25f;

	// Construct Enemies
	Enemy &e = g.newEnemy(g.getWidth() / 2, 10);
	e.min_angle = 3.14159f / 2 - .3f;
	e.max_angle = 3.14159f / 2 + .3f;
	e.min_force = 50;
	e.max_force = 51;
	e.firing_speed = 5e-2f;
	e.firing_randomness = .6f;
}

void setupScenarioHard(Game &g)
{
	// Setup Game Constants
	g.explosionTime = 5.f;
	g.explosionSize = 5.f;
	g.playerSpeed = .25f;

	// Construct Enemies
	Enemy &e = g.newEnemy(g.getWidth() / 2, 10);
	e.min_angle = 3.14159f / 2 - .3f;
	e.max_angle = 3.14159f / 2 + .3f;
	e.min_force = 50;
	e.max_force = 51;
	e.firing_speed = 3e-2f;
	e.firing_randomness = .8f;
	e.turret_speed = .15f;

	Enemy &e1 = g.newEnemy(g.getWidth() * .1f, 10);
	e1.min_angle = 3.14159f / 2 - .5f;
	e1.max_angle = 3.14159f / 2;
	e1.min_force = 60;
	e1.max_force = 61;
	e1.firing_speed = 1e-1f;
	e1.firing_randomness = .0f;
	e1.turret_speed = .05f;
}

void setupScenarioVeryHard(Game &g)
{
	// Setup Game Constants
	g.explosionTime = 1.f;
	g.explosionSize = 5.f;
	g.playerSpeed = .25f;

	// Construct Enemies
	Enemy &e = g.newEnemy(g.getWidth() / 2, 10);
	e.min_angle = 3.14159f / 2 - .05f;
	e.max_angle = 3.14159f / 2 + .05f;
	e.min_force = 100;
	e.max_force = 101;
	e.firing_speed = 8e-2f;
	e.firing_randomness = .4f;
	e.turret_speed = 0.05f;

	Enemy &e1 = g.newEnemy(g.getWidth() * .1f, 10);
	e1.min_angle = 3.14159f / 2 - .1f;
	e1.max_angle = 3.14159f / 2;
	e1.min_force = 100;
	e1.max_force = 101;
	e1.firing_speed = 1e-1f;
	e1.firing_randomness = .0f;
	e1.turret_speed = .01f;

	Enemy &e2 = g.newEnemy(g.getWidth() * .9f, 10);
	e2.min_angle = 3.14159f / 2;
	e2.max_angle = 3.14159f / 2 + .1f;
	e2.min_force = 100;
	e2.max_force = 101;
	e2.firing_speed = 5e-2f;
	e2.firing_randomness = .5f;
	e2.turret_speed = .01f;
}

void setupScenarioImpossible(Game &g)
{
	// Setup Game Constants
	g.explosionTime = 4.f;
	g.explosionSize = 3.f;
	g.playerSpeed = 1.0f;

	// Construct Enemies
	Enemy &e = g.newEnemy(g.getWidth() / 2, 10);
	e.min_angle = 3.14159f / 2 - .3f;
	e.max_angle = 3.14159f / 2 + .3f;
	e.min_force = 50;
	e.max_force = 51;
	e.firing_speed = 8e-2f;
	e.firing_randomness = .8f;
	e.turret_speed = .85f;

	Enemy &e1 = g.newEnemy(g.getWidth() * .1f, 10);
	e1.min_angle = 3.14159f / 2 - .05f;
	e1.max_angle = 3.14159f / 2;
	e1.min_force = 200;
	e1.max_force = 201;
	e1.firing_speed = 1e-1f;
	e1.firing_randomness = .0f;
	e1.turret_speed = .002f;

	Enemy &e2 = g.newEnemy(g.getWidth() * .9f, 10);
	e2.min_angle = 3.14159f / 2;
	e2.max_angle = 3.14159f / 2 + .05f;
	e2.min_force = 200;
	e2.max_force = 201;
	e2.firing_speed = 1e-1f;
	e2.firing_randomness = .3f;
	e2.turret_speed = .0025f;
}
