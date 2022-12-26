# Command

A command consist of type and parameters. ex: `F:FreeMono9pt7b` In here, `F` is the command type and `FreeMono9pt7b` is the command parameter.
Here is the list of commands. `command1_type:command1_parameter;command2_type:command2_parameter;...`
example: `CL;F:FreeMono9pt7b;...`

details of commands:
create a table in markdown

| type | function                          | parameter        | example                   |
| ---- | --------------------------------- | ---------------- | ------------------------- |
| CL   | clear                             | None             | CL                        |
| BR   | brightness                        | 0~100            | BR:60                     |
| FT   | font type                         | font type        | FT:FreeMono9pt7b          |
| SZ   | font size                         | size             | SZ:2                      |
| BG   | background clor                   | red,green,blue   | BG:255,0,0                |
| TC   | text color                        | red,green,blue   | TC:0,0,255                |
| CR   | cursor                            | x,y              | CR:10,20                  |
| GR   | grid                              |                  | GR                        |
| PT   | print a string(left)              |                  | PT:Hello World            |
| PC   | print a string(horizontal center) |                  | PC:Hello\World            |
| SC   | print a string(screen center)     |                  | SC:Hello\World            |
| IM   | a image to load                   | image name       | IM:smile.bmp              |
| CI   | fill circle                       | x,y,radius,r,g,b | CI:x,y,radius,r,g,b       |
| RT   | fill rectangle                    | x,y,w,h,r,g,b    | CI:x,y,width,height,r,g,b |
| FL   | flush all command                 | --               | FL                        |
| HP   | string information                | string           | HP:46 or HP:SLOW          |
| DL   | delay time                        | time ms          | DL:2000                   |
| FS   | flash 4 rectangles                | times,delay      | FS:5,300 (5times, 300ms)  |

# Type of Fonts

FreeMono9pt7b, FreeMono12pt7b, FreeSans9pt7b, FreeSans12pt7b, FreeSerif9pt7b, FreeSerif12pt7b

example commands:

```
CL;P:Hello;D:1000;CL;CR:10,20;P:World;D:2000;CL;
CL;I:sample1_96x64.bmp;D:2000;CL;CR:10,20;TC:0,255,0;P:Hello,World;D:1000;CL;
CL;CR:15,0;I:face2_96x64.bmp;D:3000;
```

; Get the width and height of a string
command - FT:FreeSerif12pt7b;HP:Hello
result - x=?,y=?,x1=?,y1=?,w=?,h=? (w: width, h:height)

Caution:
Users should be careful when fonts are changed.

# Code formatting

Instasll clang-format v15.

```sh
clang-format -i src/*.* -style=file:.clang-format
```

For pre-commit, use the following:

```sh
clang-format -i src/*.* -style=file:.clang-format -n -Werror
```
