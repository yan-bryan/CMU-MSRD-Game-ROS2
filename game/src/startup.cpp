#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "Game.h"
#include "gl_frontend.h"

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joy.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "game_interfaces/msg/game_state.hpp"
#include <cv_bridge/cv_bridge.hpp>
#include <opencv2/opencv.hpp>
#include <functional>
#include <unordered_map>
#include <iostream>
#include <chrono>
#include <thread>
#include <string>



// #include "game/Enemy.h"
using namespace mrsd;

void setupScenarioEasy(Game&);
void setupScenarioMedium(Game&);
void setupScenarioHard(Game&);
void setupScenarioVeryHard(Game&);
void setupScenarioImpossible(Game&);


class GameNode : public rclcpp::Node {
public:
  GameNode() : Node("game_node")
  {
    this->declare_parameter("difficulty", "easy");

    joy_sub_ = this->create_subscription<sensor_msgs::msg::Joy>(
      "joy", 10,
      [this](const sensor_msgs::msg::Joy::ConstSharedPtr msg) {
        if (!msg->axes.empty()) {
          curr_dir_ = msg->axes[0];
        }
      });

    state_pub_ = this->create_publisher<game_interfaces::msg::GameState>("game_state", 10);
    image_pub_ = this->create_publisher<sensor_msgs::msg::Image>("game_image", 1);
  }

  float direction() const { return curr_dir_; }

  void publishImage(const cv::Mat & img);
  void publishState(Game & g);

private:
  float curr_dir_ = 0.0f;
  cv::Mat flipped_;   // reused every frame
  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub_;
  rclcpp::Publisher<game_interfaces::msg::GameState>::SharedPtr state_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_pub_;
};

void GameNode::publishImage(const cv::Mat & img)
{
  cv::flip(img, flipped_, 0);
  auto msg = cv_bridge::CvImage(std_msgs::msg::Header(), "bgr8", flipped_).toImageMsg();
  msg->header.stamp = this->now();
  image_pub_->publish(*msg);
}

void GameNode::publishState(Game & g)
{
  game_interfaces::msg::GameState msg;
  msg.gravity = g.getGravity();
  msg.w = g.getWidth();
  msg.h = g.getHeight();
  msg.time_step = g.getTimeStep();
  msg.time = g.getGameTime();
  msg.explosion_size = g.explosionSize;
  msg.explosion_time = g.explosionTime;

  if (!g.getPlayers().empty()) {
    game_interfaces::msg::Player p;
    p.x = g.getPlayers()[0]->x;
    p.dead = g.getPlayers()[0]->dead;
    msg.players.push_back(p);
  }

  msg.enemies.reserve(g.getEnemies().size());
  for (const auto & e : g.getEnemies()) {
    game_interfaces::msg::Enemy em;
    em.x = e.x;
    em.y = e.y;
    em.turret_angle = e.turret_angle;
    em.turret_speed = e.turret_speed;
    em.min_angle = e.min_angle;
    em.max_angle = e.max_angle;
    em.min_force = e.min_force;
    em.max_force = e.max_force;
    em.firing_speed = e.firing_speed;
    em.firing_randomness = e.firing_randomness;
    msg.enemies.push_back(em);
  }

  msg.projectiles.reserve(g.getProjectiles().size());
  for (const auto & p : g.getProjectiles()) {
    game_interfaces::msg::Projectile pm;
    pm.x = p.x;
    pm.y = p.y;
    pm.vx = p.vx;
    pm.vy = p.vy;
    msg.projectiles.push_back(pm);
  }

  msg.explosions.reserve(g.getExplosions().size());
  for (const auto & e : g.getExplosions()) {
    game_interfaces::msg::Explosion xm;
    xm.x = e.x;
    xm.y = e.y;
    xm.time = e.time;
    msg.explosions.push_back(xm);
  }

  state_pub_->publish(msg);
}

int main(int argc, char ** argv)
{
  std::cout << "Usage:\n"
            << "Press 'C' to spawn an AI controlled player\n"
            << "Press 'Space' to spawn a user controlled player\n"
            << "Use Left and Right arrows to control your character\n";

  rclcpp::init(argc, argv);
  auto node = std::make_shared<GameNode>();
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node);

  gl::gl_frontend glfe;
  Game g(200, 200, .1f);
  glfe.init(g);
  glfe.projectileScale = 1.f;
  glfe.playerScale = 1.f;
  glfe.enemyScale = 3.f;

  const std::unordered_map<std::string, std::function<void(Game &)>> scenarios = {
    {"easy", setupScenarioEasy},
    {"medium", setupScenarioMedium},
    {"hard", setupScenarioHard},
    {"veryhard", setupScenarioVeryHard},
    {"impossible", setupScenarioImpossible},
  };

  const auto difficulty = node->get_parameter("difficulty").as_string();
  const auto scenario = scenarios.find(difficulty);
  if (scenario == scenarios.end()) {
    RCLCPP_ERROR(node->get_logger(), "Invalid difficulty: '%s'", difficulty.c_str());
    rclcpp::shutdown();
    return 1;
  }
  scenario->second(g);

  auto next_frame = std::chrono::steady_clock::now();
  const std::chrono::microseconds frame_period(1000000 / 60);
  cv::Mat img(200, 200, CV_8UC3);

  while (rclcpp::ok() && !glfe.shouldClose(g)) {
    g.tick();
    glfe.setupDraw();
    img.setTo(cv::Scalar::all(0));
    glfe.drawGame(img, g);
    glfe.finishDraw();
    node->publishImage(img);
    glfe.update(g, g.getTimeStep());
    glfe.input(g);

    next_frame += frame_period;
    std::this_thread::sleep_until(next_frame);

    if(!rclcpp::ok()){
      break;
    }
    executor.spin_some();

    if (!g.getPlayers().empty()) {
      if (node->direction() > 0) {
        g.getPlayers()[0]->x++;
      } else if (node->direction() < 0) {
        g.getPlayers()[0]->x--;
      }
    }
    node->publishState(g);
  }

  glfe.finish(g);
  rclcpp::shutdown();
  return 0;
}


void setupScenarioEasy(Game& g)
{
	//Setup Game Constants
	g.explosionTime = 2.f;
	g.explosionSize = 5.f;
	g.playerSpeed = .25f;

	//Construct Enemies
	Enemy& e = g.newEnemy(g.getWidth()/2, 10);
	e.min_angle = 3.14159f/2 - .3f;
	e.max_angle = 3.14159f/2 + .3f;
	e.min_force = 50;
	e.max_force = 51;
	e.firing_randomness = .1f;
	e.firing_speed = 1e-2f;
}

void setupScenarioMedium(Game& g)
{
	//Setup Game Constants
	g.explosionTime = 2.f;
	g.explosionSize = 5.f;
	g.playerSpeed = .25f;

	//Construct Enemies
	Enemy& e = g.newEnemy(g.getWidth()/2, 10);
	e.min_angle = 3.14159f/2 - .3f;
	e.max_angle = 3.14159f/2 + .3f;
	e.min_force = 50;
	e.max_force = 51;
	e.firing_speed = 5e-2f;
	e.firing_randomness = .6f;
}

void setupScenarioHard(Game& g)
{
	//Setup Game Constants
	g.explosionTime = 5.f;
	g.explosionSize = 5.f;
	g.playerSpeed = .25f;

	//Construct Enemies
	Enemy& e = g.newEnemy(g.getWidth()/2, 10);
	e.min_angle = 3.14159f/2 - .3f;
	e.max_angle = 3.14159f/2 + .3f;
	e.min_force = 50;
	e.max_force = 51;
	e.firing_speed = 3e-2f;
	e.firing_randomness = .8f;
	e.turret_speed = .15f;

	Enemy& e1 = g.newEnemy(g.getWidth()*.1f, 10);
	e1.min_angle = 3.14159f/2 - .5f;
	e1.max_angle = 3.14159f/2;
	e1.min_force = 60;
	e1.max_force = 61;
	e1.firing_speed = 1e-1f;
	e1.firing_randomness = .0f;
	e1.turret_speed = .05f;
}

void setupScenarioVeryHard(Game& g)
{
	//Setup Game Constants
	g.explosionTime = 1.f;
	g.explosionSize = 5.f;
	g.playerSpeed = .25f;

	//Construct Enemies
	Enemy& e = g.newEnemy(g.getWidth()/2, 10);
	e.min_angle = 3.14159f/2 - .05f;
	e.max_angle = 3.14159f/2 + .05f;
	e.min_force = 100;
	e.max_force = 101;
	e.firing_speed = 8e-2f;
	e.firing_randomness = .4f;
	e.turret_speed = 0.05f;

	Enemy& e1 = g.newEnemy(g.getWidth()*.1f, 10);
	e1.min_angle = 3.14159f/2 - .1f;
	e1.max_angle = 3.14159f/2;
	e1.min_force = 100;
	e1.max_force = 101;
	e1.firing_speed = 1e-1f;
	e1.firing_randomness = .0f;
	e1.turret_speed = .01f;

	Enemy& e2 = g.newEnemy(g.getWidth()*.9f, 10);
	e2.min_angle = 3.14159f/2;
	e2.max_angle = 3.14159f/2 + .1f;
	e2.min_force = 100;
	e2.max_force = 101;
	e2.firing_speed = 5e-2f;
	e2.firing_randomness = .5f;
	e2.turret_speed = .01f;
}

void setupScenarioImpossible(Game& g)
{
	//Setup Game Constants
	g.explosionTime = 4.f;
	g.explosionSize = 3.f;
	g.playerSpeed = 1.0f;

	//Construct Enemies
	Enemy& e = g.newEnemy(g.getWidth()/2, 10);
	e.min_angle = 3.14159f/2 - .3f;
	e.max_angle = 3.14159f/2 + .3f;
	e.min_force = 50;
	e.max_force = 51;
	e.firing_speed = 8e-2f;
	e.firing_randomness = .8f;
	e.turret_speed = .85f;

	Enemy& e1 = g.newEnemy(g.getWidth()*.1f, 10);
	e1.min_angle = 3.14159f/2 - .05f;
	e1.max_angle = 3.14159f/2;
	e1.min_force = 200;
	e1.max_force = 201;
	e1.firing_speed = 1e-1f;
	e1.firing_randomness = .0f;
	e1.turret_speed = .002f;

	Enemy& e2 = g.newEnemy(g.getWidth()*.9f, 10);
	e2.min_angle = 3.14159f/2;
	e2.max_angle = 3.14159f/2 + .05f;
	e2.min_force = 200;
	e2.max_force = 201;
	e2.firing_speed = 1e-1f;
	e2.firing_randomness = .3f;
	e2.turret_speed = .0025f;
}
