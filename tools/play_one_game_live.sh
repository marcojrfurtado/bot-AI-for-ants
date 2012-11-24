#!/usr/bin/env sh
./playgame.py -So --player_seed 42 --end_wait=0.25 --verbose --log_dir game_logs --turns 1000 --map_file maps/maze/maze_04p_01.map "$@" \
	"./Parasprites" \
	"java -jar SixPoolRush.jar" \
	"python sample_bots/python/HunterBot.py" \
	"java -jar xhatis.jar" |
java -jar visualizer.jar
