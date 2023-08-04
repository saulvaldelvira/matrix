## The matrix, in your terminal
A console aplication to render a rainfall of characters, like in the movie.<br>

![screencast](https://img.saulv.es/matrix.gif)

- Supports unicode <br>
- Only uses ANSI escapes, no external dependencies <br>

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
                                 some of them may be "hell" or "hello-wo". <br>

**NOTE:** On Windows, the output to console is VERY slow, specially when using unicode characters. You'll probably need
to use `--ascii` parameter or `--number-of-streams` with a really small number, like 12 or 15.