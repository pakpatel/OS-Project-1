# Project-1
We are writing a shell interpreter for ECE40800/CSCI40300 Operating Systems at IUPUI.

## Repository Structure
```
┌ Base  
├─┬ src  
| ├── main.cpp  
| ├── process_batch.c  
| └── process_batch.h  
├── obj  
├── bin  
├ Makefile  
├ README.md  
└ .gitignore  
```

## Building
Simply run `make` in your the top level directory. `make clean` to remove build files.

### If you do not have make installed
Probably just ask one of us that does what to do, install directions are different per system.

If you have Ubuntu or similar with the apt package manager you will likely run `sudo apt-get install make`.
