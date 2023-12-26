## The matrix, in your terminal
A console aplication to render a rainfall of characters, like in the movie.<br>

![screencast](https://img.saulv.es/matrix.gif)

## Arguments
- `--ascii`: use ascii characters only. <br>
- `--char-seed <hex-code>`: uses the given char as the "seed". This means that the
                            streams of text will be generated starting from that character. <br>
- `--step <number>`: Sets how many characters since the "char seed" to use for the stream generation. <br>
- `--stream <string>`: use the string as the character set for the stream generation. <br>
- `--number-of-streams <number>`: sets the number of streams <br>

You can also use the standard input to set a custom set of characters. <br>
For example you could pipe a file: `matrix < sushi-recipes.txt` <br>
Or the output of a command: `openssl rand 50 | base64 | matrix`


