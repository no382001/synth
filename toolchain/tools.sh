valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=logs/valgrind_`git rev-parse HEAD`.log \
         ./synth res/Sinewave.mp3 