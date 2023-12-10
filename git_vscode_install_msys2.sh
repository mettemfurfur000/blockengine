#!/bin/bash

# actualy this doesnt work, just install git on windows from official website

file="C:/Users/$USER/AppData/Roaming/Code/User/settings.json"
echo "" > $file
echo "{" >> $file
echo '  "git.path": "C:\\msys64\\usr\\bin\\git",' >> $file
echo '  "window.zoomLevel": -2' >> $file
echo "}" >> $file
