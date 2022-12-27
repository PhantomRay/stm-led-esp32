# Command List

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
| IM   | display image                     | image name       | IM:smile.bmp              |
| CI   | fill circle                       | x,y,radius,r,g,b | CI:x,y,radius,r,g,b       |
| RT   | fill rectangle                    | x,y,w,h,r,g,b    | CI:x,y,width,height,r,g,b |
| FL   | flush all command                 |                  | FL                        |
| HP   | string information                | string           | HP:46 or HP:SLOW          |
| DL   | delay time                        | in ms            | DL:2000                   |
| FS   | flash 4 rectangles                | times,delay      | FS:5,300 (5times, 300ms)  |

# Type of Fonts

- default
- HWYGOTH14pt7b
- HWYGOTH16pt7b
- HWYGOTH18pt7b
- HWYGNRRW14pt7b
- HWYGNRRW16pt7b
- HWYGNRRW18pt7b
- HWYGNRRW42pt7b

; Get the width and height of a string
command - FT:FreeSerif12pt7b;HP:Hello
result - x=?,y=?,x1=?,y1=?,w=?,h=? (w: width, h:height)

Caution:
Users should be careful when fonts are changed.

# Code Formatting

Instasll clang-format v15.

```sh
clang-format -i src/*.* -style=file:.clang-format
```

For pre-commit, use the following:

```sh
clang-format -i src/*.* -style=file:.clang-format -n -Werror
```
