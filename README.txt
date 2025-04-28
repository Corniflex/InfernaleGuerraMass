Pulled from my end of studies project.

Files included this repo are the ones that are mainly mine and contain the core of my work on this project.
As the project isn't complete, files may contain unclear elements used for debugging / unused elements that haven't been cleaned-up. For this, I apologize in advance and will update the repo again as the project reaches it's end.

A quick overview of the project : 
We're making an RTS game that gets rid of the micro-management aspect. The game also has the ambition of displaying large amounts of units on screen to give the sensation of leading hordes into battle.
The main challenges I've faced were optimizing the units and allowing the two main plugins we chose to work in pair.

There are three main elements in this repo : 

1 - Mass, the core of my work. Uses the Mass plugin for Unreal Engine, allows us to implement behaviour for a large amount of units while keeping performances up.

2 - The visualisation manager, used to instantiate and replicate visual elements for our units. It uses Niagara Systems (particle emitters) for the units' visuals.

3 - A element from the different buildings we've implemented, called "SoulBeacon". It serves as an exemple of how I approached adding an element to the game that interacts with our unit system.