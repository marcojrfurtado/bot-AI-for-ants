#!/usr/bin/env sh
#./playgame.py --player_seed 42 --end_wait=0.25 --verbose --log_dir game_logs --turns 1000 --map_file maps/maze/maze_04p_01.map "$@" "python sample_bots/python/HunterBot.py" "python sample_bots/python/LeftyBot.py" "python sample_bots/python/HunterBot.py" "python sample_bots/python/GreedyBot.py"
#./playgame.py --player_seed 42 --end_wait=0.25 --verbose --log_dir game_logs --turns 1000 --map_file maps/maze/maze_04p_01.map "$@" "python sample_bots/python/HunterBot.py" "java -jar xhatis.jar" "python sample_bots/python/HunterBot.py" "java -jar SixPoolRush.jar"

#./playgame.py --player_seed 599 --end_wait=0.5 --verbose --log_dir game_logs --turns 300 --map_file ./cell_maze_p07_14.map  "$@" "../notgreat/NotGreat" "java -jar xhatis.jar" "python sample_bots/python/HunterBot.py" "java -jar SixPoolRush.jar" "../oldman/oldman" "../notgreat-ori/NotGreatOriginal" "../parasprites/MyBot"
#./playgame.py --player_seed 42 --end_wait=0.25 --verbose --log_dir game_logs --turns 1000 --map_file ./maps/maze/maze_02p_02.map  "$@" "../oldman/oldman" "../notgreat/NotGreat" 
./playgame.py --player_seed 42 --end_wait=0.25 --verbose --log_dir game_logs --turns 1000 --map_file ./maps/maze/maze_02p_02.map  "$@" "../notgreat/NotGreat" "../notgreat-ori/NotGreatOriginal" 
