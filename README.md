# Wheel of Misfortune: Multiplayer

Wheel of Misfortune: Multiplayer is a multiplayer game in which the users must guess the 
word chosen by the computer in dictionary.txt. The difference is that the strategy of the computer is removed, thus the computer chooses at a random index of dictionary.txt and does not at any point change the word chosen. Link for the single-player version can be found here: https://github.com/Jodlua/Wheel-of-Misfortune


Players can connect to a server and take turns playing the game. To showcase this, you may run this on multiple terminals in the following order:

1. Git clone the repo in a directory of your choice. Then, direct the main terminal to the directory you are using to play this game. In the file wordsrv.c and Makefile, you may change the PORT on both to a number between 50000-59999, but both must be the same number. Additionally, you may see the hostname of the main terminal by typing and entering "hostname -f". The output may come in handy for part 4.

2. Open n additional terminals, where n is the number of players. 

3. In the main terminal, type and enter "make wordsrv" to compile the game. Then, run "./wordsrv dictionary" in the main terminal. This terminal is now the server of the game. It will then print the index of the word chosen in the dictionary.

4. In the other terminals type in "nc -C [-c for Mac] hostname [found in 1B] XXXXX" where hostname is the output of 1B, and XXXXX is the port number you've changed in 1, otherwise you can replace XXXXX = '55807'. If you are sure the terminals are running in the same host, just replace hostname with 'localhost'. This terminal will now become a player of the game. Enter a name that is at least 1 char long with no special characters (alphanumeric only). Once the player has entered a valid name, it will join the game.

5. Players then take turns guessing the word, with each incorrect guess costing 1 guess (the input will only take 1 character from the English alphabet). If a player guesses incorrectly and the number of guesses go to 0, then it is game over and another game will start again. If a player guesses correctly, they can go again, and if a player guesses the last word, then that player wins the game and another game will start again.

6. Players can join and exit at any point during the game, so if a player leaves during their turn, the turn goes to the next one in the queue. If a player comes back or if a new player joins, the next turn will be theirs to make.
