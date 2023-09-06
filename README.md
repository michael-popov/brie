# brie
Binary reader utility.

Brie reads values from binary data and prints them into stdout. It allows integrating binary data into pipeline processing by other utilities in Linux environment.

Brie can read complex data formats using built-in functions and scripts running on Lua engine. This utility can read data from files and shared memory segments (Posix and SysV). It can process many files on a single run and generate reports based on aggregated data. Brie can generate complex outputs based on Lua scripts and special print functions. It also can run in REPL mode to provide interacive access to binary data.

There is a Wiki with documentation [here](https://github.com/michael-popov/brie/wiki).

# Examples

### 1. Obligatory “hello, world!” example
```
$ brie 'print("Hello, world!")'
Hello, world!
```

### 2. A bit more sopfhisticated “hello, world!” example
```
$ cat > one.brie
#!/home/mpopov/bin/brie
print("Hello, another world!")
^D
$ chmod +x one.brie
$ ./one.brie
Hello, another world!
```

### 3. One liner to read an integer value from a file (value starts at offset 3)
```
$ echo -n -e \\x00\\x00\\x00\\\x2A\\x00\\x00\\x00 > two.dat
$ brie "scanf('%d', 'void#3 i32')" two.dat
42
```

### 4. Demo script generating a CSV file with information about BMP files
#### Script:
```
$ cat > bmps.brie
-- Declare a “header” struct
decl('header', 'str#2:signature u32:file_size void#4 void#4')
-- Declare an “info” struct
info_part1 = 'u32:size u32:width u32:height u16:planes u16:bits_per_pixel u32:compression '
info_part2 = 'u32:image_size u32:x_pixels u32:y_pixels u32:colors_used u32:important_colors'
info_descr = info_part1 .. info_part2
decl('info', info_descr)
-- Define a counter
counter = 0
-- Print a header of a file
println('Path,Size,Width,Height,BitsPerPixel')
%% -- All lines above are executed only once in the beginning of the run
-- All lines below (until the next %%) are executed for each input file
-- Read “header” struct
header = read('header')
-- Read “info” struct
info = read('info')
-- Print a CSV line with info data
println('%s,%d,%d,%d,%d', BRIE_PATH, info.size, info.width, info.height, info.bits_per_pixel)
counter = counter + 1
%% -- All lines below are executed only once when all files are processed
println("=========================================")
println("%d files", counter)
^D
```

### Running:
```
$ brie bmps.brie /home/mpopov/data/bmp/*.bmp
Path,Size,Width,Height,BitsPerPixel
/home/mpopov/data/bmp/test-16-color.bmp,40,1112,702,4
/home/mpopov/data/bmp/test-24-bit.bmp,40,1112,702,24
/home/mpopov/data/bmp/test-256-color.bmp,40,1112,702,8
/home/mpopov/data/bmp/test-monochrome.bmp,40,1112,702,1
=========================================
4 files
```

# Status
The code is in a clean state and reasonably tested.
There are always things to improve. For example, error messages can be more clear, but I have no strong incentives at the moment to make them better. The same goes to other areas that may benefit from extended functionality or better testing.
If there will be demand to make some things better, I will do it.

# Building
The utility has following dependencies:
- Lua engine (**sudo apt-get install -y liblua5.3-dev**)
- ncurses library (**sudo apt install libncurses5-dev**)
- readline library (**sudo apt install libreadline-dev**)
- Google test framework (https://www.eriksmistad.no/getting-started-with-google-test-on-ubuntu/)

```
$ git clone  https://github.com/michael-popov/brie.git
$ cd brie
$ export PROJECT_HOME=`pwd`
$ cd src
$ make
$ make test
```



