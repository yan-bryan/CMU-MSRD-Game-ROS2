#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joy.hpp"
#include "game_interfaces/msg/game_state.hpp"
#include <cmath>
#include <algorithm>

struct Prediction {
    int tick; 
    double x_position;
};


class GameAiNode : public rclcpp::Node {
    public:
        using GameState = game_interfaces::msg::GameState;
        
        explicit GameAiNode(const rclcpp::NodeOptions &options = rclcpp::NodeOptions() ) : Node("controller_full_state", options) {
            subscriber_ = create_subscription<game_interfaces::msg::GameState>("game_state",10,
                [this](const GameState::ConstSharedPtr msg) {
                    int temp_width = static_cast<int>(std::lround(msg->w) );
                    const size_t danger_size = static_cast<size_t>(horizon_ * (temp_width + 1) );
                    if(danger_.size() != danger_size ) {
                        width_ = temp_width;
                        danger_.assign(danger_size, 0);
                        reached_.assign(danger_size, 0);
                        first_move_.assign(danger_size, 0);
                        
                    }

                    if(explosion_ticks_ <= 0) {
                        explosion_ticks_ = static_cast<int>( std::lround(msg->explosion_time / msg->time_step) );
                    }

                    std::fill(danger_.begin(), danger_.end(), 0);
                    std::fill(reached_.begin(), reached_.end(), false);
                    std::fill(first_move_.begin(), first_move_.end(), 0);

                    for(size_t i = 0 ; i < msg->explosions.size() ; i++){
                        double explosion_x = msg->explosions[i].x;

                        int cell_start = static_cast<int>(floor(explosion_x - msg->explosion_size) );
                        int cell_end = static_cast<int>(ceil(explosion_x + msg->explosion_size) );

                        const double age = msg->explosions[i].time;
                        int tick_end = static_cast<int>(floor((msg->explosion_time - age) / msg->time_step + 1e-6 ) );

                        markDangerous(0, tick_end, cell_start, cell_end);
                    }

                    for(size_t i = 0 ; i < msg->projectiles.size() ; i++){
                        auto pred = trackProjectile(msg->projectiles[i], msg);
                        //testing
                        // const int curr_tick = static_cast<int>(std::round(msg->time / msg->time_step ) );
                        // RCLCPP_INFO(get_logger(), "curr_tick tick is: %d", curr_tick );
                        // const int impact_tick = curr_tick + pred.tick;
                        // RCLCPP_INFO(get_logger(), "Impact_tick is: %d, Pred x is %lf", impact_tick, pred.x_position );

                        // RCLCPP_INFO(get_logger(), "Pred tick is: %d", pred.tick );

                        auto cell_start = static_cast<int>(floor(pred.x_position - msg->explosion_size) );
                        auto cell_end = static_cast<int>(ceil(pred.x_position + msg->explosion_size) );

                        int tick_start = pred.tick - 1;
                        int tick_end = pred.tick + explosion_ticks_;

                        markDangerous(tick_start, tick_end, cell_start, cell_end);
                    }

                    //test danger matrix
                    // std::string text;
                    // for (int t = 0; t < horizon_; t += 10) {
                    //     text += "t=" + std::to_string(t) + "\t";
                    //     for (int c = 0; c <= width_; ++c) {
                    //         text += (cellAt(t, c) > 0) ? '#' : '.';
                    //     }
                    //     text += '\n';
                    // }
                    // RCLCPP_INFO_THROTTLE(get_logger(), *get_clock(), 1000, "\n%s", text.c_str());

                    //test isSafe

                    //begin decision logic
                    int start = round(msg->players[0].x);
                    reached_[0 * (width_ + 1) + start] = true; // mark current position as reached
                    
                    for(int ticks = 0 ; ticks < horizon_ - 1 ; ticks++){
                        for(int cells = 0 ; cells <= width_ ; cells++ ){
                            if(!reached_[getIndex(ticks, cells)]) {
                                continue;
                            }

                            for(int moves : {0, -1, 1}) {//Checks neighbors of curr_pos
                                int clamp_move = std::clamp(cells + moves, 0, width_);//makes sure we're within bounds
                                const int next = getIndex(ticks+1, clamp_move);

                                if(reached_[next]){ //this path has already been checked 
                                    continue;
                                }
                                if(cellAt(ticks + 1, clamp_move != 0) ){
                                    continue;
                                }
                                
                                reached_[next] = true;
                                first_move_[next] = 
                                    (ticks == 0) ? moves : first_move_[getIndex(ticks, cells)];
                            }
                        }

                    }
                }
            );
        }

    private:
        bool isSafe(int t, double px) const{
            if(t >= horizon_){
                return true;
            }

            auto px_clamp = std::clamp(px, 0.0, static_cast<double>(width_) );
            return !(cellAt(t, static_cast<int>(std::floor(px_clamp)) ) > 0 && cellAt(t, static_cast<int>(std::ceil(px_clamp)) ) > 0 );

            // return true;

        }

        Prediction trackProjectile(const game_interfaces::msg::Projectile &p, const GameState::ConstSharedPtr g) { 
            Prediction newPred = Prediction();
            double y_predict = ( -p.vy - std::sqrt( (p.vy * p.vy) - (2*g->gravity*p.y) ) ) / g->gravity;
            newPred.tick = std::max(1, static_cast<int>(std::ceil(y_predict / g->time_step - 1e-6 ) ) );
            newPred.x_position = p.x + p.vx*(newPred.tick * g->time_step);
            return newPred;
        }

        uint8_t& cellAt(int tick, int cell) {
            return danger_[getIndex(tick, cell) ];
        }

        uint8_t cellAt(int tick, int cell) const{
            return danger_[getIndex(tick, cell) ];
        }

        size_t getIndex(int tick, int cell) const{
            return static_cast<size_t>(tick * (width_+1) + cell );
        }

        void markDangerous(int tick_start, int tick_end, int cell_start, int cell_end ){
            tick_start = std::max(tick_start, 0);
            tick_end = std::min(tick_end, horizon_-1);
            cell_start = std::max(cell_start, 0);
            cell_end = std::min(cell_end, width_);

            for(int i = tick_start ; i <= tick_end ; i++) {               
                for(int j = cell_start ; j <= cell_end ; j++) {
                    cellAt(i,j)++;                                        
                } 
            }
        }

        int width_ = 0;
        const int horizon_ = 200;
        int explosion_ticks_ = 0;
        
        std::vector<uint8_t> danger_;
        std::vector<uint8_t> reached_;
        std::vector<int8_t> first_move_;

        rclcpp::Subscription<game_interfaces::msg::GameState>::SharedPtr subscriber_;
        rclcpp::Publisher<sensor_msgs::msg::Joy>::SharedPtr publisher_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc,argv);
    auto node = std::make_shared<GameAiNode>();
    rclcpp::spin(node);

    rclcpp::shutdown();
    return 0;
}