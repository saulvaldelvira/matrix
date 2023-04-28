## The matrix, in your terminal
A console aplication to render a rainfall of characters, just like in the movie. <br>

![screencast](https://img.saulv.es/matrix.gif)

### Pros
- Supports unicode <br>
- Simple. Only uses ANSI escapes, no need for third-party libraries. <br>
- Flexible. The screen can be resized, and the character set is customizable. <br>

### Cons
- For now, it only works on linux (maybe mac, I haven't tried it) <br>

## Arguments
- `--ascii`: use ascii characters only. <br>
- `--char-seed <hex-code>`: uses the given char as the "seed". This means, the
                            streams of text will be generated starting from that character. <br>
- `--step <number>`: Sets how many characters since the "char seed" to use for the stream generation. <br>
- `--stream <string>`: use the given string as the stream. <br>
- `--number-of-streams <number>`: sets the number of streams on the screen <br>

### Examples: <br>
- `matrix --char-seed 41 --step 4` will generate random streams of text using the characters from `A` (0x41) to `D` (0x44) <br>
- `matrix --stream hello-world!` will generate streams of "hello-world!". The length of the stream is random,
                                 some of them may be "hell" or "hello-wo".
