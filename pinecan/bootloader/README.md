Complete bootloader project in here. Any bootloader common code here and all other code inside of board specific folders
These board specifics will be a lot more specific than the common/boards as those are for HAL versions while bootloaders are very hardware specific
The bootloader itself will be completely identical on all projects for that board except for id information like name etc. That it will pick up maybe through a config file passed through make or smthng that part is easy

The application code that relates to the bootloader (i.e. jumping to the bootloader on update request) will still live in the common/boards files as it is pretty simple application logic that is common across. Any stuff related to having to jump to a very specific line dependent on board and not the board series can be handled as the problem appears, most likely solved through a config value being passed through.

Any further architectural magic such as how much space we want to give to the bootloader vs the app vs if we want redundancy of multiple apps in flash etc again can be solved as it comes up and when we support that on our bootloader. The solution will likely again be some information being passed through config files so that we can make the appropriate decisions at compile time for the bootloader for that specific target

Also expectation is that the bootloader code will use a lot of common code too similar to a target since it is more or less just another stm32 dronecan application.
