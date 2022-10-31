# mp2-ishaansehgal99
mp2-ishaansehgal99 created by GitHub Classroom

This is the Rate Monotonic Scheduling Assignment for course CS 423. 

# How to setup the project

## Start by cloning the repo: 
```
git clone https://github.com/uiuc-cs423-fall22/mp2-ishaansehgal99.git
```

## Then build the kernel module
```
cd mp2-ishaansehgal99
make
```

## Install the kernel module
```
sudo insmod mp2.ko
```

## Run the userapp
### We can now test the kernel module through user space code.
```
./userapp
```

## Check processes
### While the user app is running we check the processes registered, in the format: 
```
<pid 1>: <period 1>, <processing time 1>
<pid 2>: <period 2>, <processing time 2>
...
<pid n>: <period n>, <processing time n>
```
### By doing
```
cat /proc/mp2/status
```

## Remove Module
### We can uninstall the kernel module by doing 
```
sudo rmmod mp2.ko
```



