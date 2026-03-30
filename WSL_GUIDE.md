# WSL Setup Guide for System Development Labs

## 1. Open WSL
Type `wsl` in Windows Terminal, or search "Ubuntu" in the Start menu.

## 2. Navigate to the project folder
```bash
cd /mnt/c/Users/offec/Downloads/SystemDevelopmentLabs/LabA_Ofek
```

## 3. Install required tools (one-time)
```bash
sudo apt update && sudo apt install gcc-multilib nasm make
```

## 4. Build
```bash
make          # build all targets
make clean    # remove compiled files
```

## 5. Run
```bash
./main              # run the main program (Part 0)
./my_echo aa b c    # run my_echo
```

## Tips
- Files on your Windows drive are accessible under `/mnt/c/...`
- Edit files in VS Code on Windows; compile and run in WSL
- Use `make clean` before rebuilding from scratch
