# Open BF3

This is a hobbyist side project focused on porting several key systems and functions from the *Star Wars Battlefront 3* Build R70217 Xbox 360 build to x86 hardware.

## Disclaimer

I do not have any major plans for this project. This is something I wanted to experiment with for fun, and because a friend of mine wanted to see how feasible something like this would actually be.  
(Thanks, Keira.)

## How to Use

1. Clone the repository and its submodules using:

```bash
git clone --recurse-submodules REPO_URL
```

2. Install the DirectX 9 Runtime from Microsoft's website (also linked in the third-party section).

3. Compile the solution using Visual Studio 2022 or another compatible IDE in **x86 mode**.

4. Create a new folder called `Game` in the same directory as the executable.

5. Place your R70217 Xbox 360 build inside the root of the `Game` directory.

6. Run the executable and watch the console begin logging as the game boots to the familiar purple screen.


## Extra Info

- While this is an attempt to port the R7 build to PC, several liberties had to be taken due to hardware differences between the Xbox 360 and modern PC hardware, especially regarding rendering systems currently being worked on.

- This is a very early and bare-bones proof of concept demonstrating what may be possible if someone wants to take the project further. I am open-sourcing what I currently have in case it is useful to anyone else. I may also continue adding to it whenever I get the occasional itch in my brain.

- While basic `.res` parsing does exist (thanks entirely to AI — I absolutely was not doing that manually), it has mostly been stripped out and deprecated in favor of directly defining files and structures as classes. I currently have no intention of implementing a full resource parser, as directly editing the structures is significantly more practical for this project's goals. Functionality over obsessive accuracy.

- The project currently uses DirectX 9 because it makes testing and porting certain Xbox-specific rendering systems easier. This may change in the future if someone decides to expand on the project.

- **WARNING:** As stated above, this project is extremely bare minimum and may ultimately serve little practical purpose. However, it is still interesting to explore how modern tooling has made projects like this far more achievable than they once seemed.

## Reference

![Screen Shot](https://i.imgur.com/3kkBaJn.png)

## Third-Party Software

- [Zlib](https://github.com/madler/zlib/tree/v1.2.1)
- [DirectX Runtime](https://www.microsoft.com/en-us/download/details.aspx?id=8109)
