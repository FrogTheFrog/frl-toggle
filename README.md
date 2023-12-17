# frl-toggle ![Status](https://github.com/FrogTheFrog/frl-toggle/actions/workflows/publish.yaml/badge.svg)

App for Windows to toggle Nvidia's Frame Rate Limiter (FRL) via command line

# How to use
### Make sure that the global FRL setting has been saved at least once via the Nvidia control panel!

----

Run the app in terminal for usage instruction, but in case you're lazy:
```
  Usage example:
    frltoggle status                          prints the current FRL value. Value 0 means it's disabled.
    frltoggle 0                               turns off the framerate limiter.
    frltoggle 60                              sets the FPS limit to 60 (allowed values are [0, 1023]).
    frltoggle 60 --save-previous              sets the FPS limit to 60 and saves the previous value to a file.
    frltoggle 60 --save-previous-or-reuse     sets the FPS limit to 60 and saves the previous value to a file.
                                              If the file already exists, its value will be validated and reused instead.
    frltoggle load-file                       loads the value from file (e.g., saved using "--save-previous") and uses it to set FRL.
                                              File is removed afterwards if no errors occur.
```
