# Symocraft - an OpenGL Minecraft clone
An undergradute C++ course exercise project

# Control
| Keys        | Description                                        |
| ----------- | -------------------------------------------------- |
| W A S D     | Move character in the corresponding direction      |
| Left click  | Remove a block from the game world                 |
| Right click | Place a block in the game world                    |
| Q and E     | Toggle between different types of blocks available |
| Space       | Jump                                               |
| CapsLock    | Noclip                                             |

# Requirements
- GPU with OpenGL 4.6 support, 2GB VRAM(4GB or higher recommended)
- 4GB DRAM(8GB or higher recommended)

# Technical Features
- Full Direct-State-Acess(DSA) approach
- Procedural terrain generation based on Simplex noise
- Batch processing
- Basic face culling
- Entity Component System

# References
1. OpenGL guide https://learnopengl.com/ and *OpenGL SuperBible*
2. Procedural generation tutorial https://www.youtube.com/watch?v=wbpMiKiSKm8
3. Face culling algorithm https://0fps.net/2012/06/30/meshing-in-a-minecraft-game/
